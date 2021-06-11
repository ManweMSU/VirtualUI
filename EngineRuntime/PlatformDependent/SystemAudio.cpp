#include "../Interfaces/SystemAudio.h"

#include <Windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "uuid.lib")

#undef CreateSemaphore

using namespace Engine::Streaming;

namespace Engine
{
	namespace Audio
	{
		void _select_stream_format(StreamDesc & format, WAVEFORMATEX * wave)
		{
			WAVEFORMATEXTENSIBLE * wave_ex = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(wave);
			format.ChannelCount = wave->nChannels;
			format.FramesPerSecond = wave->nSamplesPerSec;
			if (wave->wFormatTag == WAVE_FORMAT_PCM) {
				if (wave->wBitsPerSample == 8) format.Format = SampleFormat::S8_snorm;
				else if (wave->wBitsPerSample == 16) format.Format = SampleFormat::S16_snorm;
				else if (wave->wBitsPerSample == 24) format.Format = SampleFormat::S24_snorm;
				else if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_snorm;
				else throw Exception();
			} else if (wave->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
				if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_float;
				else if (wave->wBitsPerSample == 64) format.Format = SampleFormat::S64_float;
				else throw Exception();
			} else if (wave->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
				if (wave_ex->Samples.wValidBitsPerSample != wave->wBitsPerSample) throw Exception();
				if (wave_ex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
					if (wave->wBitsPerSample == 8) format.Format = SampleFormat::S8_snorm;
					else if (wave->wBitsPerSample == 16) format.Format = SampleFormat::S16_snorm;
					else if (wave->wBitsPerSample == 24) format.Format = SampleFormat::S24_snorm;
					else if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_snorm;
					else throw Exception();
				} else if (wave_ex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
					if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_float;
					else if (wave->wBitsPerSample == 64) format.Format = SampleFormat::S64_float;
					else throw Exception();
				} else throw Exception();
			} else throw Exception();
		}
		void _select_custom_format(StreamDesc & format, WAVEFORMATEX * wave, DWORD & convert)
		{
			wave->wFormatTag = WAVE_FORMAT_PCM;
			wave->wBitsPerSample = 32;
			wave->nBlockAlign = 4 * wave->nChannels;
			wave->nAvgBytesPerSec = wave->nBlockAlign * wave->nSamplesPerSec;
			wave->cbSize = 0;
			format.Format = SampleFormat::S32_snorm;
			format.ChannelCount = wave->nChannels;
			format.FramesPerSecond = wave->nSamplesPerSec;
			convert = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
		}
		struct _dispatch_task {
			SafePointer<WaveBuffer> buffer;
			SafePointer<Semaphore> open;
			SafePointer<IDispatchTask> task;
			bool * status;
			_dispatch_task * next;
		};
		class CoreAudioOutputDevice : public IAudioOutputDevice
		{
			StreamDesc format;
			IAudioClient * client;
			IAudioRenderClient * render;
			IAudioStreamVolume * volume;
			SafePointer<Thread> thread;
			SafePointer<Semaphore> task_count;
			SafePointer<Semaphore> access;
			_dispatch_task * task_first;
			_dispatch_task * task_last;
			HANDLE event;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioOutputDevice *>(arg);
				if (device->client->GetBufferSize(&buffer_size) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioRenderClient, reinterpret_cast<void **>(&device->render)) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioStreamVolume, reinterpret_cast<void **>(&device->volume)) != S_OK) {
					device->render->Release();
					device->render = 0;
					SetEvent(device->event);
					return 1;
				}
				SetEvent(device->event);
				while (true) {
					_dispatch_task * current = 0;
					device->task_count->Wait();
					device->access->Wait();
					if (device->command) {
						auto command_local = device->command;
						auto task_local = device->task_first;
						device->task_first = device->task_last = 0;
						if (device->command & 2) device->command &= ~2;
						device->access->Open();
						while (task_local) {
							if (task_local->status) *task_local->status = false;
							if (task_local->open) task_local->open->Open();
							if (task_local->task) task_local->task->DoTask(0);
							device->task_count->Wait();
							auto task_delete = task_local;
							task_local = task_local->next;
							delete task_delete;
						}
						ResetEvent(device->event);
						if (command_local & 1) break;
						continue;
					} else {
						current = device->task_first;
						device->task_first = device->task_first->next;
						if (!device->task_first) device->task_last = 0;
					}
					device->access->Open();
					uint64 current_frame = 0;
					bool local_status = true;
					while (current_frame < current->buffer->FramesUsed()) {
						WaitForSingleObject(device->event, INFINITE);
						if (device->command) { local_status = false; break; }
						UINT32 padding;
						if (device->client->GetCurrentPadding(&padding) == S_OK) {
							UINT32 frames_write = buffer_size - padding;
							uint64 frames_exists = current->buffer->FramesUsed() - current_frame;
							if (frames_write > frames_exists) frames_write = UINT32(frames_exists);
							BYTE * data;
							if (device->render->GetBuffer(frames_write, &data) != S_OK) { local_status = false; break; }
							auto frame_size = StreamFrameByteSize(current->buffer->GetFormatDescriptor());
							MemoryCopy(data, current->buffer->GetData() + frame_size * current_frame, frame_size * frames_write);
							if (device->render->ReleaseBuffer(frames_write, 0) != S_OK) { local_status = false; break; }
							current_frame += frames_write;
						} else { local_status = false; break; }
					}
					if (current->status) *current->status = local_status;
					if (current->open) current->open->Open();
					if (current->task) current->task->DoTask(0);
					delete current;
				}
				device->volume->Release();
				device->volume = 0;
				device->render->Release();
				device->render = 0;
				return 0;
			}
			void _append_dispatch(WaveBuffer * buffer, Semaphore * open, IDispatchTask * task, bool * result, int _command)
			{
				if (!_command) {
					access->Wait();
					try {
						_dispatch_task * new_task = new _dispatch_task;
						new_task->buffer.SetRetain(buffer);
						new_task->open.SetRetain(open);
						new_task->task.SetRetain(task);
						new_task->status = result;
						new_task->next = 0;
						if (task_last) {
							task_last->next = new_task;
							task_last = new_task;
						} else {
							task_first = task_last = new_task;
						}
					} catch (...) { access->Open(); throw; }
					task_count->Open();
					access->Open();
				} else {
					access->Wait();
					if (_command & 1 && !(command & 1)) task_count->Open();
					if (_command & 2 && !(command & 2)) task_count->Open();
					command |= _command;
					access->Open();
					SetEvent(event);
				}
			}
		public:
			CoreAudioOutputDevice(IMMDevice * device)
			{
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; render = 0; command = 0;
				task_first = task_last = 0;
				if (client->GetMixFormat(&wave) != S_OK) { client->Release(); throw Exception(); }
				try { _select_stream_format(format, wave); } catch (...) { _select_custom_format(format, wave, convert); }
				if (client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | convert |
					AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED,
					0, 0, wave, 0) != S_OK) { CoTaskMemFree(wave); client->Release(); throw Exception(); }
				CoTaskMemFree(wave);
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { client->Release(); throw Exception(); }
				event = CreateEventW(0, FALSE, FALSE, 0);
				if (!event) { client->Release(); throw Exception(); }
				thread = CreateThread(_dispatch_thread, this);
				if (!thread) { CloseHandle(event); client->Release(); throw Exception(); }
				WaitForSingleObject(event, INFINITE);
				if (!volume || !render) { CloseHandle(event); client->Release(); throw Exception(); }
				if (client->SetEventHandle(event) != S_OK) {
					_append_dispatch(0, 0, 0, 0, 1);
					thread->Wait();
					CloseHandle(event);
					client->Release();
					throw Exception();
				}
			}
			virtual ~CoreAudioOutputDevice(void) override
			{
				client->Stop();
				client->Reset();
				_append_dispatch(0, 0, 0, 0, 1);
				thread->Wait();
				CloseHandle(event);
				client->Release();
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return format; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceOutput; }
			virtual double GetVolume(void) noexcept override
			{
				float level;
				if (volume->GetChannelVolume(0, &level) == S_OK) return level;
				return 0.0;
			}
			virtual void SetVolume(double _volume) noexcept override
			{
				try {
					UINT32 count;
					if (volume->GetChannelCount(&count) != S_OK) return;
					Array<float> levels(count);
					for (UINT32 i = 0; i < count; i++) levels << float(_volume);
					volume->SetAllVolumes(count, levels.GetBuffer());
				} catch (...) {}
			}
			virtual bool StartProcessing(void) noexcept override
			{
				if (client->Start() != S_OK) return false;
				return true;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (client->Stop() != S_OK) return false;
				return true;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				client->Stop();
				if (client->Reset() != S_OK) return false;
				_append_dispatch(0, 0, 0, 0, 2);
				return true;
			}
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				bool status;
				if (!semaphore || !WriteFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, open_on_processed, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
		};
		class CoreAudioInputDevice : public IAudioInputDevice
		{
			StreamDesc format;
			IAudioClient * client;
			IAudioCaptureClient * capture;
			IAudioStreamVolume * volume;
			SafePointer<Thread> thread;
			SafePointer<Semaphore> task_count;
			SafePointer<Semaphore> access;
			_dispatch_task * task_first;
			_dispatch_task * task_last;
			HANDLE event;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				uint8 * buffer_small;
				uint32 buffer_small_unread, buffer_small_size;
				buffer_small_unread = buffer_small_size = 0;
				const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioInputDevice *>(arg);
				if (device->client->GetBufferSize(&buffer_size) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				auto frame_size = StreamFrameByteSize(device->format);
				buffer_small = reinterpret_cast<uint8 *>(malloc(buffer_size * frame_size));
				if (!buffer_small) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioCaptureClient, reinterpret_cast<void **>(&device->capture)) != S_OK) {
					free(buffer_small);
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioStreamVolume, reinterpret_cast<void **>(&device->volume)) != S_OK) {
					free(buffer_small);
					device->capture->Release();
					device->capture = 0;
					SetEvent(device->event);
					return 1;
				}
				SetEvent(device->event);
				while (true) {
					_dispatch_task * current = 0;
					device->task_count->Wait();
					device->access->Wait();
					if (device->command) {
						auto command_local = device->command;
						auto task_local = device->task_first;
						device->task_first = device->task_last = 0;
						if (device->command & 2) device->command &= ~2;
						device->access->Open();
						while (task_local) {
							task_local->buffer->FramesUsed() = 0;
							if (task_local->status) *task_local->status = false;
							if (task_local->open) task_local->open->Open();
							if (task_local->task) task_local->task->DoTask(0);
							device->task_count->Wait();
							auto task_delete = task_local;
							task_local = task_local->next;
							delete task_delete;
						}
						ResetEvent(device->event);
						if (command_local & 1) break;
						continue;
					} else {
						current = device->task_first;
						device->task_first = device->task_first->next;
						if (!device->task_first) device->task_last = 0;
					}
					device->access->Open();
					bool local_status = true;
					current->buffer->FramesUsed() = 0;
					if (buffer_small_unread < buffer_small_size) {
						uint64 buffer_free = current->buffer->GetSizeInFrames();
						uint32 write = buffer_small_size - buffer_small_unread;
						if (write > buffer_free) write = uint32(buffer_free);
						MemoryCopy(current->buffer->GetData(), buffer_small + frame_size * buffer_small_unread, frame_size * write);
						current->buffer->FramesUsed() = write;
						buffer_small_unread += write;
					}
					while (current->buffer->FramesUsed() < current->buffer->GetSizeInFrames()) {
						WaitForSingleObject(device->event, INFINITE);
						if (device->command) break;
						UINT32 read_ready;
						if (device->capture->GetNextPacketSize(&read_ready) == S_OK) {
							HRESULT int_status = S_OK;
							while (read_ready) {
								LPBYTE data_ptr;
								UINT32 data_frame_size;
								DWORD data_flags;
								int_status = device->capture->GetBuffer(&data_ptr, &data_frame_size, &data_flags, 0, 0);
								if (int_status != S_OK) { local_status = false; break; }
								buffer_small_unread = 0;
								buffer_small_size = data_frame_size;
								if (data_flags & AUDCLNT_BUFFERFLAGS_SILENT) ZeroMemory(buffer_small, data_frame_size * frame_size);
								else MemoryCopy(buffer_small, data_ptr, data_frame_size * frame_size);
								int_status = device->capture->ReleaseBuffer(data_frame_size);
								if (int_status != S_OK) { local_status = false; break; }
								uint64 buffer_free = current->buffer->GetSizeInFrames() - current->buffer->FramesUsed();
								uint32 copy = buffer_small_size;
								if (copy > buffer_free) copy = uint32(buffer_free);
								if (copy) {
									MemoryCopy(current->buffer->GetData() + current->buffer->FramesUsed() * frame_size, buffer_small, copy * frame_size);
									current->buffer->FramesUsed() += copy;
									buffer_small_unread += copy;
								}
								if (buffer_small_unread < buffer_small_size) { int_status = E_UNEXPECTED; break; }
								int_status = device->capture->GetNextPacketSize(&read_ready);
								if (int_status != S_OK) { local_status = false; break; }
							}
							if (int_status != S_OK) break;
						} else { local_status = false; break; }
					}
					if (current->status) *current->status = local_status;
					if (current->open) current->open->Open();
					if (current->task) current->task->DoTask(0);
					delete current;
				}
				device->volume->Release();
				device->volume = 0;
				device->capture->Release();
				device->capture = 0;
				free(buffer_small);
				return 0;
			}
			void _append_dispatch(WaveBuffer * buffer, Semaphore * open, IDispatchTask * task, bool * result, int _command)
			{
				if (!_command) {
					access->Wait();
					try {
						_dispatch_task * new_task = new _dispatch_task;
						new_task->buffer.SetRetain(buffer);
						new_task->open.SetRetain(open);
						new_task->task.SetRetain(task);
						new_task->status = result;
						new_task->next = 0;
						if (task_last) {
							task_last->next = new_task;
							task_last = new_task;
						} else {
							task_first = task_last = new_task;
						}
					} catch (...) { access->Open(); throw; }
					task_count->Open();
					access->Open();
				} else {
					access->Wait();
					if (_command & 1 && !(command & 1)) task_count->Open();
					if (_command & 2 && !(command & 2)) task_count->Open();
					command |= _command;
					access->Open();
					SetEvent(event);
				}
			}
		public:
			CoreAudioInputDevice(IMMDevice * device)
			{
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; capture = 0; command = 0;
				task_first = task_last = 0;
				if (client->GetMixFormat(&wave) != S_OK) { client->Release(); throw Exception(); }
				try { _select_stream_format(format, wave); } catch (...) { _select_custom_format(format, wave, convert); }
				if (client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | convert |
					AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED,
					0, 0, wave, 0) != S_OK) {
					CoTaskMemFree(wave); client->Release(); throw Exception();
				}
				CoTaskMemFree(wave);
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { client->Release(); throw Exception(); }
				event = CreateEventW(0, FALSE, FALSE, 0);
				if (!event) { client->Release(); throw Exception(); }
				thread = CreateThread(_dispatch_thread, this);
				if (!thread) { CloseHandle(event); client->Release(); throw Exception(); }
				WaitForSingleObject(event, INFINITE);
				if (!volume || !capture) { CloseHandle(event); client->Release(); throw Exception(); }
				if (client->SetEventHandle(event) != S_OK) {
					_append_dispatch(0, 0, 0, 0, 1);
					thread->Wait();
					CloseHandle(event);
					client->Release();
					throw Exception();
				}
			}
			virtual ~CoreAudioInputDevice(void) override
			{
				client->Stop();
				client->Reset();
				_append_dispatch(0, 0, 0, 0, 1);
				thread->Wait();
				CloseHandle(event);
				client->Release();
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return format; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceInput; }
			virtual double GetVolume(void) noexcept override
			{
				float level;
				if (volume->GetChannelVolume(0, &level) == S_OK) return level;
				return 0.0;
			}
			virtual void SetVolume(double _volume) noexcept override
			{
				try {
					UINT32 count;
					if (volume->GetChannelCount(&count) != S_OK) return;
					Array<float> levels(count);
					for (UINT32 i = 0; i < count; i++) levels << float(_volume);
					volume->SetAllVolumes(count, levels.GetBuffer());
				} catch (...) {}
			}
			virtual bool StartProcessing(void) noexcept override
			{
				if (client->Start() != S_OK) return false;
				return true;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (client->Stop() != S_OK) return false;
				return true;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				client->Stop();
				if (client->Reset() != S_OK) return false;
				_append_dispatch(0, 0, 0, 0, 2);
				return true;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				bool status;
				if (!semaphore || !ReadFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, open_on_processed, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
		};
		class CoreAudioDeviceFactory : public IAudioDeviceFactory
		{
			IMMDeviceEnumerator * enumerator;
			Dictionary::PlainDictionary<string, string> * _retr_collection(EDataFlow flow)
			{
				IMMDeviceCollection * collection;
				if (enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection) != S_OK) return 0;
				try {
					SafePointer< Dictionary::PlainDictionary<string, string> > dict = new Dictionary::PlainDictionary<string, string>(0x20);
					UINT num_devices;
					if (collection->GetCount(&num_devices) != S_OK) throw Exception();
					for (UINT i = 0; i < num_devices; i++) {
						IMMDevice * device = 0;
						IPropertyStore * store = 0;
						try {
							if (collection->Item(i, &device) != S_OK) throw Exception();
							LPWSTR uid;
							string uid_str;
							if (device->GetId(&uid) != S_OK) throw Exception();
							try { uid_str = string(uid); } catch (...) { CoTaskMemFree(uid); throw; }
							CoTaskMemFree(uid);
							if (device->OpenPropertyStore(STGM_READ, &store) != S_OK) throw Exception();
							PROPVARIANT var;
							PropVariantInit(&var);
							if (store->GetValue(PKEY_Device_FriendlyName, &var) != S_OK) {
								PropVariantClear(&var);
								throw Exception();
							}
							try { dict->Append(uid_str, var.pwszVal); } catch (...) { PropVariantClear(&var); throw; }
							PropVariantClear(&var);
						} catch (...) { if (device) device->Release(); if (store) store->Release(); throw; }
						if (device) device->Release();
						if (store) store->Release();
					}
					dict->Retain();
					collection->Release();
					return dict;
				} catch (...) { collection->Release(); return 0; }
			}
		public:
			CoreAudioDeviceFactory(void)
			{
				const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
				const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
				auto status = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
				if (status != S_OK) throw Exception();
			}
			virtual ~CoreAudioDeviceFactory(void) override { enumerator->Release(); }
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableOutputDevices(void) noexcept override { return _retr_collection(eRender); }
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableInputDevices(void) noexcept override { return _retr_collection(eCapture); }
			virtual IAudioOutputDevice * CreateOutputDevice(const string & identifier) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDevice(identifier, &device) != S_OK) return 0;
				IAudioOutputDevice * result;
				try { result = new CoreAudioOutputDevice(device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device) != S_OK) return 0;
				IAudioOutputDevice * result;
				try { result = new CoreAudioOutputDevice(device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDevice(identifier, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
		};

		IAudioCodec * InitializeSystemCodec(void)
		{
			return nullptr;
			// TODO: IMPLEMENT
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return new CoreAudioDeviceFactory(); }
		void SystemBeep(void) { MessageBeep(0); }
	}
}