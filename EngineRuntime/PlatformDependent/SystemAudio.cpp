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
		class CoreAudioOutputDevice : public IAudioOutputDevice
		{
			struct _dispatch_task {
				SafePointer<WaveBuffer> buffer;
				SafePointer<Semaphore> open;
				SafePointer<IDispatchTask> task;
				bool * status;
			};

			StreamDesc format;
			IAudioClient * client;
			IAudioRenderClient * render;
			IAudioStreamVolume * volume;
			SafePointer<Thread> thread;
			SafePointer<Semaphore> task_count;
			SafePointer<Semaphore> access;
			Array<_dispatch_task> _dispatch_tasks;
			HANDLE event;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioOutputDevice *>(arg);
				auto vol_status = device->client->GetService(IID_IAudioStreamVolume, reinterpret_cast<void **>(&device->volume));
				auto size_status = device->client->GetBufferSize(&buffer_size);
				if (size_status != S_OK) { device->volume->Release(); device->volume = 0; }
				SetEvent(device->event);
				if (vol_status != S_OK || size_status != S_OK) return 1;
				while (true) {
					_dispatch_task current;
					device->task_count->Wait();
					device->access->Wait();
					if (device->command) {
						auto command_local = device->command;
						auto tasks_local = device->_dispatch_tasks;
						device->_dispatch_tasks.Clear();
						if (device->command & 2) device->command &= ~2;
						device->access->Open();
						for (int i = 0; i < tasks_local.Length(); i++) {
							auto & task = tasks_local[i];
							if (task.status) *task.status = false;
							if (task.open) task.open->Open();
							if (task.task) task.task->DoTask(0);
							device->task_count->Wait();
						}
						tasks_local.Clear();
						ResetEvent(device->event);
						if (command_local & 1) break;
						continue;
					} else {
						current = device->_dispatch_tasks.FirstElement();
						device->_dispatch_tasks.RemoveFirst();
					}
					device->access->Open();
					uint64 current_frame = 0;
					while (current_frame < current.buffer->FramesUsed()) {
						WaitForSingleObject(device->event, INFINITE);
						if (device->command) break;
						UINT32 padding;
						if (device->client->GetCurrentPadding(&padding) == S_OK) {
							UINT32 frames_write = buffer_size - padding;
							uint64 frames_exists = current.buffer->FramesUsed() - current_frame;
							if (frames_write > frames_exists) frames_write = UINT32(frames_exists);
							BYTE * data;
							if (device->render->GetBuffer(frames_write, &data) != S_OK) break;
							auto frame_size = StreamFrameByteSize(current.buffer->GetFormatDescriptor());
							MemoryCopy(data, current.buffer->GetData() + frame_size * current_frame, frame_size * frames_write);
							if (device->render->ReleaseBuffer(frames_write, 0) != S_OK) break;
							current_frame += frames_write;
						} else break;						
					}
					if (current.status) *current.status = true;
					if (current.open) current.open->Open();
					if (current.task) current.task->DoTask(0);
				}
				device->volume->Release();
				device->volume = 0;
				return 0;
			}
			void _append_dispatch(WaveBuffer * buffer, Semaphore * open, IDispatchTask * task, bool * result, int _command)
			{
				if (!_command) {
					access->Wait();
					try {
						_dispatch_tasks << _dispatch_task();
						_dispatch_tasks.LastElement().buffer.SetRetain(buffer);
						_dispatch_tasks.LastElement().open.SetRetain(open);
						_dispatch_tasks.LastElement().task.SetRetain(task);
						_dispatch_tasks.LastElement().status = result;
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
			CoreAudioOutputDevice(IMMDevice * device) : _dispatch_tasks(0x100)
			{
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; command = 0;
				if (client->GetMixFormat(&wave) != S_OK) { client->Release(); throw Exception(); }
				try { _select_stream_format(format, wave); } catch (...) { _select_custom_format(format, wave, convert); }
				if (client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | convert |
					AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED,
					0, 0, wave, 0) != S_OK) { CoTaskMemFree(wave); client->Release(); throw Exception(); }
				CoTaskMemFree(wave);
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { client->Release(); throw Exception(); }
				if (client->GetService(IID_IAudioRenderClient, reinterpret_cast<void **>(&render)) != S_OK) { client->Release(); throw Exception(); }
				event = CreateEventW(0, FALSE, FALSE, 0);
				if (!event) { render->Release(); client->Release(); throw Exception(); }
				thread = CreateThread(_dispatch_thread, this);
				if (!thread) { CloseHandle(event); render->Release(); client->Release(); throw Exception(); }
				WaitForSingleObject(event, INFINITE);
				if (!volume) { CloseHandle(event); render->Release(); client->Release(); throw Exception(); }
				if (client->SetEventHandle(event) != S_OK) {
					_append_dispatch(0, 0, 0, 0, 1);
					thread->Wait();
					CloseHandle(event);
					render->Release();
					client->Release();
					throw Exception();
				}
			}
			virtual ~CoreAudioOutputDevice(void) override
			{
				_append_dispatch(0, 0, 0, 0, 1);
				thread->Wait();
				CloseHandle(event);
				render->Release();
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
		public:
			CoreAudioInputDevice(IMMDevice * device)
			{
				throw Exception();
				// TODO: IMPLEMENT
			}
			virtual ~CoreAudioInputDevice(void) override
			{
				// TODO: IMPLEMENT
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return format; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceInput; }
			virtual double GetVolume(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0.0;
			}
			virtual void SetVolume(double volume) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual bool StartProcessing(void) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
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