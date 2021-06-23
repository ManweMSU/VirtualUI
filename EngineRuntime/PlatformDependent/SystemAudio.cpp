#include "../Interfaces/SystemAudio.h"

#include <Windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")

#undef CreateSemaphore

using namespace Engine::Streaming;

namespace Engine
{
	namespace Audio
	{
		class MediaFoundationStream : public IMFByteStream
		{
			class _async_read_callback : public IMFAsyncCallback
			{
			public:
				SafePointer<Stream> _stream;
				ULONG _ref_cnt;
				BYTE * _buffer_ptr;
				ULONG _read_size;

				_async_read_callback(void) : _ref_cnt(1), _buffer_ptr(0), _read_size(0) {}
				virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
				{
					if (riid == IID_IUnknown) {
						*ppvObject = static_cast<IUnknown *>(this);
						AddRef();
						return S_OK;
					} else if (riid == __uuidof(IMFAsyncCallback)) {
						*ppvObject = static_cast<IMFAsyncCallback *>(this);
						AddRef();
						return S_OK;
					} else return E_NOINTERFACE;
				}
				virtual ULONG __stdcall AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
				virtual ULONG __stdcall Release(void) override
				{
					auto result = InterlockedDecrement(&_ref_cnt);
					if (!result) delete this;
					return result;
				}
				virtual HRESULT __stdcall GetParameters(DWORD * pdwFlags, DWORD * pdwQueue) override { return E_NOTIMPL; }
				virtual HRESULT __stdcall Invoke(IMFAsyncResult * pAsyncResult) override
				{
					IUnknown * state = 0;
					IMFAsyncResult * caller_result = 0;
					auto status = pAsyncResult->GetState(&state);
					if (status == S_OK) {
						status = state->QueryInterface(IID_PPV_ARGS(&caller_result));
						if (status == S_OK) {
							try {
								_stream->Read(_buffer_ptr, _read_size);
								status = S_OK;
							} catch (IO::FileReadEndOfFileException & e) {
								_read_size = e.DataRead;
								status = S_OK;
							} catch (...) { status = E_FAIL; }
						}
					}
					if (caller_result) {
						caller_result->SetStatus(status);
						MFInvokeCallback(caller_result);
						caller_result->Release();
					}
					if (state) state->Release();
					return S_OK;
				}
			};
			class _async_write_callback : public IMFAsyncCallback
			{
			public:
				SafePointer<Stream> _stream;
				ULONG _ref_cnt;
				const BYTE * _buffer_ptr;
				ULONG _write_size;

				_async_write_callback(void) : _ref_cnt(1), _buffer_ptr(0), _write_size(0) {}
				virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
				{
					if (riid == IID_IUnknown) {
						*ppvObject = static_cast<IUnknown *>(this);
						AddRef();
						return S_OK;
					} else if (riid == __uuidof(IMFAsyncCallback)) {
						*ppvObject = static_cast<IMFAsyncCallback *>(this);
						AddRef();
						return S_OK;
					} else return E_NOINTERFACE;
				}
				virtual ULONG __stdcall AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
				virtual ULONG __stdcall Release(void) override
				{
					auto result = InterlockedDecrement(&_ref_cnt);
					if (!result) delete this;
					return result;
				}
				virtual HRESULT __stdcall GetParameters(DWORD * pdwFlags, DWORD * pdwQueue) override { return E_NOTIMPL; }
				virtual HRESULT __stdcall Invoke(IMFAsyncResult * pAsyncResult) override
				{
					IUnknown * state = 0;
					IMFAsyncResult * caller_result = 0;
					auto status = pAsyncResult->GetState(&state);
					if (status == S_OK) {
						status = state->QueryInterface(IID_PPV_ARGS(&caller_result));
						if (status == S_OK) {
							try {
								_stream->Write(_buffer_ptr, _write_size);
								status = S_OK;
							} catch (...) { status = E_FAIL; }
						}
					}
					if (caller_result) {
						caller_result->SetStatus(status);
						MFInvokeCallback(caller_result);
						caller_result->Release();
					}
					if (state) state->Release();
					return S_OK;
				}
			};
			SafePointer<Stream> _inner;
			ULONG _ref_cnt;
			bool _allow_write;
		public:
			MediaFoundationStream(Stream * inner, bool allow_write) : _ref_cnt(1), _allow_write(allow_write) { _inner.SetRetain(inner); }
			~MediaFoundationStream(void) {}
			virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
			{
				if (riid == IID_IUnknown) {
					*ppvObject = static_cast<IUnknown *>(this);
					AddRef();
					return S_OK;
				} else if (riid == __uuidof(IMFByteStream)) {
					*ppvObject = static_cast<IMFByteStream *>(this);
					AddRef();
					return S_OK;
				} else return E_NOINTERFACE;
			}
			virtual ULONG __stdcall AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
			virtual ULONG __stdcall Release(void) override
			{
				auto val_new = InterlockedDecrement(&_ref_cnt);
				if (!val_new) delete this;
				return val_new;
			}
			virtual HRESULT __stdcall GetCapabilities(DWORD * pdwCapabilities) override
			{
				DWORD result = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;
				if (_allow_write) result |= MFBYTESTREAM_IS_WRITABLE;
				*pdwCapabilities = result;
				return S_OK;
			}
			virtual HRESULT __stdcall GetLength(QWORD * pqwLength) override
			{
				try { *pqwLength = _inner->Length(); } catch (...) { return E_FAIL; }
				return S_OK;
			}
			virtual HRESULT __stdcall SetLength(QWORD qwLength) override
			{
				try { _inner->SetLength(qwLength); } catch (...) { return E_FAIL; }
				return S_OK;
			}
			virtual HRESULT __stdcall GetCurrentPosition(QWORD * pqwPosition) override
			{
				try { *pqwPosition = _inner->Seek(0, Current); } catch (...) { return E_FAIL; }
				return S_OK;
			}
			virtual HRESULT __stdcall SetCurrentPosition(QWORD qwPosition) override
			{
				try { _inner->Seek(qwPosition, Begin); } catch (...) { return E_FAIL; }
				return S_OK;
			}
			virtual HRESULT __stdcall IsEndOfStream(BOOL * pfEndOfStream) override
			{
				try { *pfEndOfStream = (_inner->Length() <= uint64(_inner->Seek(0, Current))); } catch (...) { return E_FAIL; }
				return S_OK;
			}
			virtual HRESULT __stdcall Read(BYTE * pb, ULONG cb, ULONG * pcbRead) override
			{
				try { _inner->Read(pb, cb); *pcbRead = cb; return S_OK; }
				catch (IO::FileReadEndOfFileException & e) { *pcbRead = e.DataRead; return S_OK; }
				catch (...) { return E_FAIL; }
			}
			virtual HRESULT __stdcall BeginRead(BYTE * pb, ULONG cb, IMFAsyncCallback * pCallback, IUnknown * punkState) override
			{
				auto callback = new (std::nothrow) _async_read_callback;
				if (!callback) return E_OUTOFMEMORY;
				callback->_stream.SetRetain(_inner);
				callback->_buffer_ptr = pb;
				callback->_read_size = cb;
				IMFAsyncResult * result = 0;
				auto status = MFCreateAsyncResult(callback, pCallback, punkState, &result);
				if (status == S_OK) {
					status = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, callback, result);
					result->Release();
				}
				callback->Release();
				return status;
			}
			virtual HRESULT __stdcall EndRead(IMFAsyncResult * pResult, ULONG * pcbRead) override
			{
				*pcbRead = 0;
				HRESULT status = pResult->GetStatus();
				if (status != S_OK) return status;
				IUnknown * object = 0;
				status = pResult->GetObjectW(&object);
				if (status != S_OK) return status;
				*pcbRead = static_cast<_async_read_callback *>(object)->_read_size;
				object->Release();
				return status;
			}
			virtual HRESULT __stdcall Write(const BYTE * pb, ULONG cb, ULONG * pcbWritten) override
			{
				try { _inner->Write(pb, cb); } catch (...) { return E_FAIL; }
				*pcbWritten = cb; return S_OK;
			}
			virtual HRESULT __stdcall BeginWrite(const BYTE * pb, ULONG cb, IMFAsyncCallback * pCallback, IUnknown * punkState) override
			{
				auto callback = new (std::nothrow) _async_write_callback;
				if (!callback) return E_OUTOFMEMORY;
				callback->_stream.SetRetain(_inner);
				callback->_buffer_ptr = pb;
				callback->_write_size = cb;
				IMFAsyncResult * result = 0;
				auto status = MFCreateAsyncResult(callback, pCallback, punkState, &result);
				if (status == S_OK) {
					status = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, callback, result);
					result->Release();
				}
				callback->Release();
				return status;
			}
			virtual HRESULT __stdcall EndWrite(IMFAsyncResult * pResult, ULONG * pcbWritten) override
			{
				*pcbWritten = 0;
				HRESULT status = pResult->GetStatus();
				if (status != S_OK) return status;
				IUnknown * object = 0;
				status = pResult->GetObjectW(&object);
				if (status != S_OK) return status;
				*pcbWritten = static_cast<_async_write_callback *>(object)->_write_size;
				object->Release();
				return status;
			}
			virtual HRESULT __stdcall Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD * pqwCurrentPosition) override
			{
				if (SeekOrigin == msoBegin) {
					try { *pqwCurrentPosition = _inner->Seek(llSeekOffset, Begin); } catch (...) { return E_FAIL; }
				} else if (SeekOrigin == msoCurrent) {
					try { *pqwCurrentPosition = _inner->Seek(llSeekOffset, Current); } catch (...) { return E_FAIL; }
				}
				return S_OK;
			}
			virtual HRESULT __stdcall Flush(void) override { try { _inner->Flush(); } catch (...) { return E_FAIL; } return S_OK; }
			virtual HRESULT __stdcall Close(void) override { _inner.SetReference(0); return S_OK; }
		};
		class MediaFoundationDecoderStream : public IAudioDecoderStream
		{
			SafePointer<IAudioCodec> parent;
			IMFSourceReader * reader;
			StreamDesc internal, intermediate, output;
			string format;
			uint64 current_frame, frame_count;
			SafePointer<WaveBuffer> read_cache;
			uint64 read_cache_position;

			friend class MediaFoundationCodec;
			bool _internal_load_cache(void) noexcept
			{
				DWORD stream_index, stream_flags;
				LONGLONG sample_time;
				IMFSample * sample;
				while (true) {
					auto status = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &stream_index, &stream_flags, &sample_time, &sample);
					if (status != S_OK) return false;
					if (!sample) {
						if (stream_flags & MF_SOURCE_READERF_ENDOFSTREAM) {
							read_cache_position = 0;
							read_cache.SetReference(0);
							return true;
						} else continue;
					}
					DWORD byte_length;
					if (sample->GetTotalLength(&byte_length) != S_OK) { sample->Release(); return false; }
					auto num_frames = byte_length / StreamFrameByteSize(intermediate);
					IMFMediaBuffer * buffer;
					if (sample->ConvertToContiguousBuffer(&buffer) != S_OK) { sample->Release(); return false; }
					sample->Release();
					try { read_cache = new WaveBuffer(intermediate, num_frames); } catch (...) { buffer->Release(); return false; }
					read_cache_position = 0;
					read_cache->FramesUsed() = read_cache->GetSizeInFrames();
					PBYTE data;
					if (buffer->Lock(&data, 0, 0) != S_OK) { buffer->Release(); read_cache.SetReference(0); return false; }
					MemoryCopy(read_cache->GetData(), data, intptr(read_cache->GetUsedSizeInBytes()));
					if (buffer->Unlock() != S_OK) { buffer->Release(); read_cache.SetReference(0); return false; }
					buffer->Release();
					return true;
				}
			}
			bool _internal_read_frames(WaveBuffer * buffer) noexcept
			{
				buffer->FramesUsed() = 0;
				while (buffer->FramesUsed() < buffer->GetSizeInFrames()) {
					if (!read_cache) {
						if (!_internal_load_cache()) return false;
						if (!read_cache) { current_frame = frame_count; return true; }
					}
					uint64 cache_avail = read_cache->FramesUsed() - read_cache_position;
					if (!cache_avail) return true;
					uint64 frames_needed = buffer->GetSizeInFrames() - buffer->FramesUsed();
					uint64 read_now = min(frames_needed, cache_avail);
					auto frame_size = StreamFrameByteSize(intermediate);
					MemoryCopy(buffer->GetData() + frame_size * buffer->FramesUsed(),
						read_cache->GetData() + frame_size * read_cache_position, intptr(frame_size * read_now));
					current_frame += read_now;
					read_cache_position += read_now;
					buffer->FramesUsed() += read_now;
					if (read_cache_position == read_cache->FramesUsed()) read_cache.SetReference(0);
				}
				return true;
			}
		public:
			MediaFoundationDecoderStream(IAudioCodec * codec, IMFSourceReader * source_reader, const string & source_format)
			{
				parent.SetRetain(codec);
				reader = source_reader;
				reader->AddRef();
				internal = intermediate = output = StreamDesc(SampleFormat::Invalid, 0, 0);
				format = source_format;
				current_frame = frame_count = read_cache_position = 0;
			}
			virtual ~MediaFoundationDecoderStream(void) override { if (reader) reader->Release(); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return output; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamDecoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return format; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return internal; }
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != output.Format || buffer_desc.ChannelCount != output.ChannelCount || buffer_desc.FramesPerSecond != output.FramesPerSecond) return false;
				try {
					if (output.FramesPerSecond == intermediate.FramesPerSecond) {
						if (output.ChannelCount == intermediate.ChannelCount) {
							if (output.Format == intermediate.Format) {
								return _internal_read_frames(buffer);
							} else {
								SafePointer<WaveBuffer> ssrel = new WaveBuffer(intermediate.Format, output.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
								if (!_internal_read_frames(ssrel)) return false;
								ssrel->ConvertFormat(buffer, output.Format);
								return true;
							}
						} else {
							SafePointer<WaveBuffer> chrem = new WaveBuffer(intermediate.Format, intermediate.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
							if (!_internal_read_frames(chrem)) return false;
							if (output.Format == intermediate.Format) {
								chrem->ReallocateChannels(buffer, output.ChannelCount);
								return true;
							} else {
								SafePointer<WaveBuffer> ssrel = new WaveBuffer(intermediate.Format, output.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
								chrem->ReallocateChannels(ssrel, output.ChannelCount);
								ssrel->ConvertFormat(buffer, output.Format);
								return true;
							}
						}
					} else {
						uint64 frames_equivalent = ConvertFrameCount(buffer->GetSizeInFrames(), output.FramesPerSecond, intermediate.FramesPerSecond);
						uint64 frames_can_read = min(frame_count - current_frame, frames_equivalent);
						if (!frames_can_read) { buffer->FramesUsed() = 0; return true; }
						SafePointer<WaveBuffer> frres = new WaveBuffer(intermediate.Format, intermediate.ChannelCount, intermediate.FramesPerSecond, frames_can_read);
						if (!_internal_read_frames(frres)) return false;
						if (output.ChannelCount == intermediate.ChannelCount) {
							if (output.Format == intermediate.Format) {
								frres->ConvertFrameRate(buffer, output.FramesPerSecond);
								return true;
							} else {
								SafePointer<WaveBuffer> ssrel = new WaveBuffer(intermediate.Format, output.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
								frres->ConvertFrameRate(ssrel, output.FramesPerSecond);
								ssrel->ConvertFormat(buffer, output.Format);
								return true;
							}
						} else {
							SafePointer<WaveBuffer> chrem = new WaveBuffer(intermediate.Format, intermediate.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
							frres->ConvertFrameRate(chrem, output.FramesPerSecond);
							if (output.Format == intermediate.Format) {
								chrem->ReallocateChannels(buffer, output.ChannelCount);
								return true;
							} else {
								SafePointer<WaveBuffer> ssrel = new WaveBuffer(intermediate.Format, output.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
								chrem->ReallocateChannels(ssrel, output.ChannelCount);
								ssrel->ConvertFormat(buffer, output.Format);
								return true;
							}
						}
					}
				} catch (...) { return false; }
			}
			virtual uint64 GetFramesCount(void) const override { return frame_count; }
			virtual uint64 GetCurrentFrame(void) const override { return current_frame; }
			virtual bool SetCurrentFrame(uint64 frame_index) override
			{
				uint64 hns_pos = frame_index * 10000000ULL / intermediate.FramesPerSecond;
				PROPVARIANT prop;
				PropVariantInit(&prop);
				prop.vt = VT_I8;
				prop.uhVal.QuadPart = hns_pos;
				auto status = reader->SetCurrentPosition(GUID_NULL, prop);
				PropVariantClear(&prop);
				if (status == S_OK) {
					current_frame = frame_index;
					read_cache.SetReference(0);
					read_cache_position = 0;
					return true;
				} else return false;
			}
		};
		class MediaFoundationEncoderStream : public IAudioEncoderStream
		{
			struct _async_write_element {
				SafePointer<WaveBuffer> buffer;
				SafePointer<Semaphore> open;
				SafePointer<IDispatchTask> task;
				bool * result;
				_async_write_element * next;
			};
			SafePointer<IAudioCodec> parent;
			StreamDesc input, intermediate, output;
			string format;
			IMFSinkWriter * writer;
			IMFMediaSink * sink;
			LONGLONG write_time;
			int mode; // 0 - regular, 1 - async

			SafePointer<Semaphore> access;
			SafePointer<Semaphore> task_count;
			SafePointer<Thread> thread;
			_async_write_element * first_task;
			_async_write_element * last_task;

			bool _internal_write_frames(WaveBuffer * buffer) noexcept
			{
				SafePointer<WaveBuffer> converted;
				converted.SetRetain(buffer);
				try {
					if (converted->GetFormatDescriptor().FramesPerSecond != intermediate.FramesPerSecond)
						converted = converted->ConvertFrameRate(intermediate.FramesPerSecond);
					if (converted->GetFormatDescriptor().ChannelCount != intermediate.ChannelCount)
						converted = converted->ReallocateChannels(intermediate.ChannelCount);
					if (converted->GetFormatDescriptor().Format != intermediate.Format)
						converted = converted->ConvertFormat(intermediate.Format);
				} catch (...) { return false; }
				LONGLONG duration = (converted->FramesUsed() * 10000000ULL + intermediate.FramesPerSecond / 2) / intermediate.FramesPerSecond;
				IMFMediaBuffer * media_buffer;
				if (MFCreateMemoryBuffer(intptr(converted->GetUsedSizeInBytes()), &media_buffer) != S_OK) return false;
				BYTE * data_ptr;
				if (media_buffer->Lock(&data_ptr, 0, 0) != S_OK) { media_buffer->Release(); return false; }
				MemoryCopy(data_ptr, converted->GetData(), intptr(converted->GetUsedSizeInBytes()));
				if (media_buffer->Unlock() != S_OK) { media_buffer->Release(); return false; }
				if (media_buffer->SetCurrentLength(intptr(converted->GetUsedSizeInBytes())) != S_OK) { media_buffer->Release(); return false; }
				converted.SetReference(0);
				IMFSample * sample;
				if (MFCreateSample(&sample) != S_OK) { media_buffer->Release(); return false; }
				if (sample->AddBuffer(media_buffer) != S_OK) { media_buffer->Release(); sample->Release(); return false; }
				media_buffer->Release();
				if (sample->SetSampleTime(write_time) != S_OK) { sample->Release(); return false; }
				if (sample->SetSampleDuration(duration) != S_OK) { sample->Release(); return false; }
				if (writer->WriteSample(0, sample) != S_OK) { sample->Release(); return false; }
				sample->Release();
				write_time += duration;
				return true;
			}
			static int _internal_dispatch_thread(void * arg)
			{
				MediaFoundationEncoderStream * self = reinterpret_cast<MediaFoundationEncoderStream *>(arg);
				while (true) {
					_async_write_element * element = 0;
					self->task_count->Wait();
					self->access->Wait();
					element = self->first_task;
					self->first_task = self->first_task->next;
					if (!self->first_task) self->last_task = 0;
					self->access->Open();
					if (element->buffer) {
						bool status = self->_internal_write_frames(element->buffer);
						if (element->result) *element->result = status;
						if (element->open) element->open->Open();
						if (element->task) element->task->DoTask(0);
						delete element;
					} else break;
				}
				return 0;
			}
			bool _internal_enqueue_buffer(WaveBuffer * buffer, bool * result, Semaphore * open, IDispatchTask * task) noexcept
			{
				_async_write_element * element = new (std::nothrow) _async_write_element;
				if (!element) return false;
				element->buffer.SetRetain(buffer);
				element->open.SetRetain(open);
				element->task.SetRetain(task);
				element->result = result;
				element->next = 0;
				access->Wait();
				if (last_task) {
					last_task->next = element;
					last_task = element;
				} else first_task = last_task = element;
				task_count->Open();
				access->Open();
				return true;
			}
			bool _internal_async_init(void) noexcept
			{
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { access.SetReference(0); task_count.SetReference(0); return false; }
				thread = CreateThread(_internal_dispatch_thread, this);
				if (!thread) { access.SetReference(0); task_count.SetReference(0); return false; }
				mode = 1;
				return true;
			}
		public:
			MediaFoundationEncoderStream(IAudioCodec * codec, Stream * stream, const string & dest_format, const StreamDesc & dest_desc)
			{
				first_task = last_task = 0;
				write_time = 0;
				mode = 0;
				parent.SetRetain(codec);
				format = dest_format;
				input = dest_desc;
				if (input.Format == SampleFormat::Invalid || !input.ChannelCount || !input.FramesPerSecond) throw Exception();
				MediaFoundationStream * wrapper = new MediaFoundationStream(stream, true);
				if (dest_format == AudioFormatMPEG3) {
					if (input.FramesPerSecond > 44100) output.FramesPerSecond = 48000;
					else if (input.FramesPerSecond > 32000) output.FramesPerSecond = 44100;
					else output.FramesPerSecond = 32000;
					if (input.ChannelCount > 1) output.ChannelCount = 2;
					else output.ChannelCount = 1;
					output.Format = SampleFormat::Invalid;
					if (MFCreateMP3MediaSink(wrapper, &sink) != S_OK) { wrapper->Release(); throw Exception(); }
				} else if (dest_format == AudioFormatMPEG4AAC) {
					IMFMediaType * aac;
					if (MFCreateMediaType(&aac) != S_OK) { wrapper->Release(); throw Exception(); }
					try {
						output = input;
						output.Format = SampleFormat::S16_snorm;
						if (aac->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) throw Exception();
						if (aac->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC) != S_OK) throw Exception();
						if (aac->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16) != S_OK) throw Exception();
						if (aac->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, output.FramesPerSecond) != S_OK) throw Exception();
						if (aac->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, output.ChannelCount) != S_OK) throw Exception();
						if (aac->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000) != S_OK) throw Exception();
						if (MFCreateMPEG4MediaSink(wrapper, 0, aac, &sink) != S_OK) throw Exception();
						aac->Release();
					} catch (...) { wrapper->Release(); aac->Release(); throw; }
				} else { wrapper->Release(); throw Exception(); }
				wrapper->Release();
				if (MFCreateSinkWriterFromMediaSink(sink, 0, &writer) != S_OK) {
					sink->Shutdown(); sink->Release();
					throw Exception();
				}
				intermediate.Format = SampleFormat::S16_snorm;
				intermediate.ChannelCount = output.ChannelCount;
				intermediate.FramesPerSecond = output.FramesPerSecond;
				IMFMediaType * input_type;
				if (MFCreateMediaType(&input_type) != S_OK) {
					writer->Finalize(); writer->Release();
					sink->Shutdown(); sink->Release();
					throw Exception();
				}
				WAVEFORMATEX wave;
				wave.wFormatTag = WAVE_FORMAT_PCM;
				wave.nChannels = intermediate.ChannelCount;
				wave.nSamplesPerSec = intermediate.FramesPerSecond;
				wave.wBitsPerSample = 16;
				wave.nBlockAlign = intermediate.ChannelCount * 2;
				wave.nAvgBytesPerSec = wave.nBlockAlign * wave.nSamplesPerSec;
				wave.cbSize = 0;
				if (MFInitMediaTypeFromWaveFormatEx(input_type, &wave, sizeof(wave)) != S_OK) {
					writer->Finalize(); writer->Release();
					sink->Shutdown(); sink->Release();
					input_type->Release();
					throw Exception();
				}
				if (writer->SetInputMediaType(0, input_type, 0) != S_OK) {
					writer->Finalize(); writer->Release();
					sink->Shutdown(); sink->Release();
					input_type->Release();
					throw Exception();
				}
				input_type->Release();
				if (writer->BeginWriting() != S_OK) {
					writer->Finalize(); writer->Release();
					sink->Shutdown(); sink->Release();
					throw Exception();
				}
			}
			virtual ~MediaFoundationEncoderStream(void) override { Finalize(); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return input; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamEncoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return format; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return output; }
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer || !writer || mode) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != input.Format || buffer_desc.ChannelCount != input.ChannelCount || buffer_desc.FramesPerSecond != input.FramesPerSecond) return false;
				return _internal_write_frames(buffer);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				if (!buffer || !writer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != input.Format || buffer_desc.ChannelCount != input.ChannelCount || buffer_desc.FramesPerSecond != input.FramesPerSecond) return false;
				if (!mode && !_internal_async_init()) return false;
				return _internal_enqueue_buffer(buffer, write_status, 0, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer || !writer || !execute_on_processed) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != input.Format || buffer_desc.ChannelCount != input.ChannelCount || buffer_desc.FramesPerSecond != input.FramesPerSecond) return false;
				if (!mode && !_internal_async_init()) return false;
				return _internal_enqueue_buffer(buffer, write_status, 0, execute_on_processed);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer || !writer || !open_on_processed) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != input.Format || buffer_desc.ChannelCount != input.ChannelCount || buffer_desc.FramesPerSecond != input.FramesPerSecond) return false;
				if (!mode && !_internal_async_init()) return false;
				return _internal_enqueue_buffer(buffer, write_status, open_on_processed, 0);
			}
			virtual bool Finalize(void) noexcept override
			{
				if (mode) {
					access->Wait();
					_async_write_element stop;
					stop.next = 0;
					stop.result = 0;
					if (last_task) {
						last_task->next = &stop;
						last_task = &stop;
					} else last_task = first_task = &stop;
					task_count->Open();
					access->Open();
					thread->Wait();
					access.SetReference(0);
					task_count.SetReference(0);
					thread.SetReference(0);
					first_task = last_task = 0;
					mode = 0;
				}
				bool result = true;
				if (writer) {
					if (writer->Finalize() != S_OK) result = false;
					writer->Release();
					writer = 0;
				}
				if (sink) {
					if (sink->Shutdown() != S_OK) result = false;
					sink->Release();
					sink = 0;
				}
				return result;
			}
		};
		class MediaFoundationCodec : public IAudioCodec
		{
		public:
			MediaFoundationCodec(void) { if (MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET) != S_OK) throw Exception(); }
			virtual ~MediaFoundationCodec(void) override { MFShutdown(); }
			virtual bool CanEncode(const string & format) const noexcept override
			{
				if (format == AudioFormatMPEG3 || format == AudioFormatMPEG4AAC) return true;
				else return false;
			}
			virtual bool CanDecode(const string & format) const noexcept override
			{
				if (format == AudioFormatMPEG3 || format == AudioFormatMPEG4AAC || format == AudioFormatFreeLossless) return true;
				else return 0;
			}
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(4);
				result->Append(AudioFormatMPEG3);
				result->Append(AudioFormatMPEG4AAC);
				result->Retain();
				return result;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(4);
				result->Append(AudioFormatMPEG3);
				result->Append(AudioFormatMPEG4AAC);
				result->Append(AudioFormatFreeLossless);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Media Foundation Codec"; }
			virtual IAudioDecoderStream * TryDecode(Streaming::Stream * source, const StreamDesc * desired_desc) noexcept override
			{
				try {
					source->Seek(0, Begin);
					MediaFoundationStream * wrapper = new MediaFoundationStream(source, false);
					IMFSourceReader * reader;
					if (MFCreateSourceReaderFromByteStream(wrapper, 0, &reader) != S_OK) {
						wrapper->Release();
						throw Exception();
					}
					wrapper->Release();
					IMFMediaType * type;
					if (reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CURRENT_TYPE_INDEX, &type) != S_OK) {
						reader->Release();
						throw Exception();
					}
					GUID audio_type;
					UINT32 frames_per_second, num_channels, bits_per_sample;
					string format;
					if (type->GetGUID(MF_MT_SUBTYPE, &audio_type) != S_OK) {
						type->Release();
						reader->Release();
						throw Exception();
					}
					if (type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &num_channels) != S_OK) num_channels = 0;
					if (type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits_per_sample) != S_OK) bits_per_sample = 0;
					if (type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &frames_per_second) != S_OK) frames_per_second = 0;
					type->Release();
					if (audio_type == MFAudioFormat_AAC) format = AudioFormatMPEG4AAC;
					else if (audio_type == MFAudioFormat_FLAC) format = AudioFormatFreeLossless;
					else if (audio_type == MFAudioFormat_MP3) format = AudioFormatMPEG3;
					if (!format.Length()) {
						reader->Release();
						throw Exception();
					}
					SafePointer<MediaFoundationDecoderStream> result;
					try {
						result = new MediaFoundationDecoderStream(this, reader, format);
					} catch (...) { reader->Release(); throw; }
					reader->Release();
					result->internal.ChannelCount = num_channels;
					result->internal.FramesPerSecond = frames_per_second;
					if (bits_per_sample) {
						if (bits_per_sample == 64) result->internal.Format = SampleFormat::S64_float;
						else if (bits_per_sample == 32) result->internal.Format = SampleFormat::S32_float;
						else if (bits_per_sample == 24) result->internal.Format = SampleFormat::S24_snorm;
						else if (bits_per_sample == 16) result->internal.Format = SampleFormat::S16_snorm;
						else if (bits_per_sample == 8) result->internal.Format = SampleFormat::S8_snorm;
					}
					if (desired_desc) {
						if (desired_desc->FramesPerSecond) result->output.FramesPerSecond = desired_desc->FramesPerSecond;
						else result->output.FramesPerSecond = result->internal.FramesPerSecond;
						if (desired_desc->ChannelCount) result->output.ChannelCount = desired_desc->ChannelCount;
						else result->output.ChannelCount = result->internal.ChannelCount;
						if (desired_desc->Format != SampleFormat::Invalid) result->output.Format = desired_desc->Format;
						else if (result->internal.Format != SampleFormat::Invalid) result->output.Format = result->internal.Format;
						else result->output.Format = SampleFormat::S16_snorm;
					} else {
						result->output = result->internal;
						if (result->output.Format == SampleFormat::Invalid) result->output.Format = SampleFormat::S16_snorm;
					}
					result->intermediate = result->internal;
					if (result->internal.Format != SampleFormat::Invalid) result->intermediate.Format = result->internal.Format;
					else result->intermediate.Format = SampleFormat::S16_snorm;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					WAVEFORMATEX wave;
					if (result->intermediate.Format == SampleFormat::S64_float) {
						wave.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
						wave.wBitsPerSample = 64;
					} else if (result->intermediate.Format == SampleFormat::S32_float) {
						wave.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
						wave.wBitsPerSample = 32;
					} else if (result->intermediate.Format == SampleFormat::S32_snorm) {
						wave.wFormatTag = WAVE_FORMAT_PCM;
						wave.wBitsPerSample = 32;
					} else if (result->intermediate.Format == SampleFormat::S24_snorm) {
						wave.wFormatTag = WAVE_FORMAT_PCM;
						wave.wBitsPerSample = 24;
					} else if (result->intermediate.Format == SampleFormat::S16_snorm) {
						wave.wFormatTag = WAVE_FORMAT_PCM;
						wave.wBitsPerSample = 16;
					} else if (result->intermediate.Format == SampleFormat::S8_snorm) {
						wave.wFormatTag = WAVE_FORMAT_PCM;
						wave.wBitsPerSample = 8;
					}
					wave.nChannels = result->intermediate.ChannelCount;
					wave.nSamplesPerSec = result->intermediate.FramesPerSecond;
					wave.nBlockAlign = (wave.wBitsPerSample * wave.nChannels) / 8;
					wave.nAvgBytesPerSec = wave.nBlockAlign * wave.nSamplesPerSec;
					wave.cbSize = 0;
					if (MFInitMediaTypeFromWaveFormatEx(type, &wave, sizeof(wave)) != S_OK) {
						type->Release();
						throw Exception();
					}
					if (result->reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, type) != S_OK) {
						type->Release();
						throw Exception();
					}
					type->Release();
					ULONGLONG hns_length;
					PROPVARIANT variant;
					PropVariantInit(&variant);
					if (result->reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &variant) != S_OK) {
						PropVariantClear(&variant);
						throw Exception();
					}
					hns_length = variant.uhVal.QuadPart;
					PropVariantClear(&variant);
					result->frame_count = (hns_length * result->internal.FramesPerSecond + 5000000ULL) / 10000000ULL;
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IAudioEncoderStream * Encode(Streaming::Stream * dest, const string & format, const StreamDesc & desc) noexcept override
			{
				try {
					if (CanEncode(format)) return new MediaFoundationEncoderStream(this, dest, format, desc);
					else throw Exception();
				} catch (...) { return 0; }
			}
		};

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
		class _device_status_notification : public IMMNotificationClient
		{
			IMMDeviceEnumerator * _enumerator;
			string _dev_id;
			HANDLE _event_open;
			ULONG _ref_cnt;
		public:
			_device_status_notification(IMMDeviceEnumerator * enumerator, IMMDevice * device, HANDLE event_open) : _event_open(event_open)
			{
				LPWSTR dev_uid;
				if (device->GetId(&dev_uid) != S_OK) throw Exception();
				try { _dev_id = string(dev_uid); } catch (...) { CoTaskMemFree(dev_uid); throw; }
				CoTaskMemFree(dev_uid);
				if (enumerator->RegisterEndpointNotificationCallback(this) != S_OK) throw Exception();
				_enumerator = enumerator;
				_enumerator->AddRef();
				_ref_cnt = 1;
			}
			~_device_status_notification(void) { _enumerator->UnregisterEndpointNotificationCallback(this); _enumerator->Release(); }
			virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override
			{
				if (riid == __uuidof(IMMNotificationClient)) {
					*ppvObject = static_cast<IMMNotificationClient *>(this);
					AddRef();
					return S_OK;
				} else if (riid == IID_IUnknown) {
					*ppvObject = static_cast<IUnknown *>(this);
					AddRef();
					return S_OK;
				} else return E_NOINTERFACE;
			}
			virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
			virtual ULONG STDMETHODCALLTYPE Release(void) override
			{
				auto result = InterlockedDecrement(&_ref_cnt);
				if (!result) delete this;
				return result;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
			{
				if (dwNewState != DEVICE_STATE_ACTIVE && pwstrDeviceId == _dev_id) SetEvent(_event_open);
				return S_OK;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override { return S_OK; }
			virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override
			{
				if (pwstrDeviceId == _dev_id) SetEvent(_event_open);
				return S_OK;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override { return S_OK; }
			virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override { return S_OK; }
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
			_device_status_notification * notification;
			bool device_dead;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioOutputDevice *>(arg);
				SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
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
						if (device->device_dead) { local_status = false; break; }
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
					if (!local_status) {
						device->access->Wait();
						device->device_dead = true;
						device->command |= 2;
						device->task_count->Open();
						device->access->Open();
					}
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
			CoreAudioOutputDevice(IMMDeviceEnumerator * enumerator, IMMDevice * device)
			{
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; render = 0; command = 0;
				device_dead = false;
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
				try { notification = new _device_status_notification(enumerator, device, event); } catch (...) { notification = 0; }
				if (client->SetEventHandle(event) != S_OK || !notification) {
					if (notification) notification->Release();
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
				notification->Release();
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
				if (!buffer || !execute_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer || !open_on_processed) return false;
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
			_device_status_notification * notification;
			bool device_dead;
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
				SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
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
						if (device->device_dead) { local_status = false; break; }
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
					if (!local_status) {
						device->access->Wait();
						device->device_dead = true;
						device->command |= 2;
						device->task_count->Open();
						device->access->Open();
					}
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
			CoreAudioInputDevice(IMMDeviceEnumerator * enumerator, IMMDevice * device)
			{
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; capture = 0; command = 0;
				device_dead = false;
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
				try { notification = new _device_status_notification(enumerator, device, event); } catch (...) { notification = 0; }
				if (client->SetEventHandle(event) != S_OK || !notification) {
					if (notification) notification->Release();
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
				notification->Release();
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
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, 0, read_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer || !execute_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, read_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer || !open_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, open_on_processed, 0, read_status, 0); } catch (...) { return false; }
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
				try { result = new CoreAudioOutputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device) != S_OK) return 0;
				IAudioOutputDevice * result;
				try { result = new CoreAudioOutputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDevice(identifier, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
		};

		SafePointer<IAudioCodec> _system_codec;
		IAudioCodec * InitializeSystemCodec(void)
		{
			if (!_system_codec) {
				_system_codec = new MediaFoundationCodec;
				RegisterCodec(_system_codec);
			}
			return _system_codec;
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return new CoreAudioDeviceFactory(); }
		void SystemBeep(void) { MessageBeep(0); }
	}
}