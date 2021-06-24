#include "../Interfaces/SystemAudio.h"

#include <AudioToolbox/AudioToolbox.h>

namespace Engine
{
	namespace Audio
	{
		class CoreAudioFileWrapper : public Object
		{
		public:
			SafePointer<Streaming::Stream> inner;
			AudioFileID file_ref;
			ExtAudioFileRef file_ex_ref;

			static OSStatus Stream_ReadCallback(handle stream, int64 offset, uint32 size, void * buffer, uint32 * actually_read)
			{
				try {
					reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->Seek(offset, Streaming::Begin);
					reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->Read(buffer, size);
					*actually_read = size;
					return kAudioCodecNoError;
				} catch (IO::FileReadEndOfFileException & eof) {
					*actually_read = eof.DataRead;
					if (eof.DataRead) return kAudioCodecNoError;
					else return kAudioFileEndOfFileError;
				} catch (...) {
					return kAudioFileOperationNotSupportedError;
				}
			}
			static OSStatus Stream_WriteCallback(handle stream, int64 offset, uint32 size, const void * buffer, uint32 * actually_wrote)
			{
				try {
					reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->Seek(offset, Streaming::Begin);
					reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->Write(buffer, size);
					*actually_wrote = size;
					return kAudioCodecNoError;
				} catch (...) {
					return kAudioFileOperationNotSupportedError;
				}
			}
			static int64 Stream_GetSizeCallback(handle stream)
			{
				return reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->Length();
			}
			static OSStatus Stream_SetSizeCallback(handle stream, int64 size)
			{
				try {
					reinterpret_cast<CoreAudioFileWrapper *>(stream)->inner->SetLength(size);
					return kAudioCodecNoError;
				} catch (...) {
					return kAudioFileOperationNotSupportedError;
				}
			}

			CoreAudioFileWrapper(Streaming::Stream * stream, bool record_new, AudioFileTypeID type_id, const AudioStreamBasicDescription * desc)
			{
				inner.SetRetain(stream);
				OSStatus status;
				if (record_new) {
					status = AudioFileInitializeWithCallbacks(this, Stream_ReadCallback, Stream_WriteCallback,
						Stream_GetSizeCallback, Stream_SetSizeCallback, type_id, desc, kAudioFileFlags_DontPageAlignAudioData, &file_ref);
				} else {
					status = AudioFileOpenWithCallbacks(this, Stream_ReadCallback, 0, Stream_GetSizeCallback, 0, type_id, &file_ref);
				}
				if (status != kAudioCodecNoError) {
					if (status == kAudioFileUnsupportedFileTypeError || status == kAudioFileUnsupportedDataFormatError || status == kAudioFileInvalidFileError)
						throw InvalidFormatException();
					if (status == kAudioFileOperationNotSupportedError) throw IO::FileAccessException(IO::Error::Unknown);
					throw Exception();
				}
				status = ExtAudioFileWrapAudioFileID(file_ref, record_new, &file_ex_ref);
				if (status != kAudioCodecNoError) {
					AudioFileClose(file_ref);
					if (status == kExtAudioFileError_NonPCMClientFormat) throw InvalidFormatException();
					throw Exception();
				}
			}
			virtual ~CoreAudioFileWrapper(void) override
			{
				if (file_ex_ref) ExtAudioFileDispose(file_ex_ref);
				if (file_ref) AudioFileClose(file_ref);
			}

			void GetFileStreamDesc(AudioStreamBasicDescription & desc)
			{
				uint32 prop_size = sizeof(desc);
				OSStatus status = ExtAudioFileGetProperty(file_ex_ref, kExtAudioFileProperty_FileDataFormat, &prop_size, &desc);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
			}
			void SetFileStreamDesc(const AudioStreamBasicDescription & desc)
			{
				OSStatus status = ExtAudioFileSetProperty(file_ex_ref, kExtAudioFileProperty_FileDataFormat, sizeof(desc), &desc);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
			}
			void GetClientStreamDesc(AudioStreamBasicDescription & desc)
			{
				uint32 prop_size = sizeof(desc);
				OSStatus status = ExtAudioFileGetProperty(file_ex_ref, kExtAudioFileProperty_ClientDataFormat, &prop_size, &desc);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
			}
			void SetClientStreamDesc(const AudioStreamBasicDescription & desc)
			{
				OSStatus status = ExtAudioFileSetProperty(file_ex_ref, kExtAudioFileProperty_ClientDataFormat, sizeof(desc), &desc);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
			}
			int64 GetFramesCount(void)
			{
				uint32 prop_size = sizeof(int64);
				int64 result;
				OSStatus status = ExtAudioFileGetProperty(file_ex_ref, kExtAudioFileProperty_FileLengthFrames, &prop_size, &result);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
				return result;
			}
			void SetFramesCount(int64 count)
			{
				OSStatus status = ExtAudioFileSetProperty(file_ex_ref, kExtAudioFileProperty_FileLengthFrames, sizeof(count), &count);
				if (status == kExtAudioFileError_InvalidProperty || status == kExtAudioFileError_InvalidPropertySize) throw InvalidArgumentException();
				else if (status != kAudioCodecNoError) throw Exception();
			}
		};
		class CoreAudioDecoderStream : public IAudioDecoderStream
		{
			SafePointer<CoreAudioFileWrapper> wrapper;
			SafePointer<IAudioCodec> parent;
			string format;
			StreamDesc input, output;
			uint64 frames_read, frames_count;
			friend class CoreAudioCodec;
			bool _internal_read_frames(WaveBuffer * buffer) noexcept
			{
				uint32 frames_to_read = buffer->GetSizeInFrames();
				AudioBufferList list;
				list.mNumberBuffers = 1;
				list.mBuffers[0].mData = buffer->GetData();
				list.mBuffers[0].mDataByteSize = buffer->GetAllocatedSizeInBytes();
				list.mBuffers[0].mNumberChannels = buffer->GetFormatDescriptor().ChannelCount;
				OSStatus status = ExtAudioFileRead(wrapper->file_ex_ref, &frames_to_read, &list);
				if (status) {
					return false;
				} else {
					frames_read += frames_to_read;
					buffer->FramesUsed() = frames_to_read;
					return true;
				}
			}
		public:
			CoreAudioDecoderStream(void) : frames_read(0) {}
			virtual ~CoreAudioDecoderStream(void) override {}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return output; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamDecoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return format; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return input; }
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != output.Format || buffer_desc.ChannelCount != output.ChannelCount || buffer_desc.FramesPerSecond != output.FramesPerSecond) return false;
				try {
					if (output.ChannelCount == input.ChannelCount && output.FramesPerSecond == input.FramesPerSecond) {
						return _internal_read_frames(buffer);
					} else {
						if (output.FramesPerSecond == input.FramesPerSecond) {
							SafePointer<WaveBuffer> local = new WaveBuffer(output.Format, input.ChannelCount, output.FramesPerSecond, buffer->GetSizeInFrames());
							if (!_internal_read_frames(local)) return false;
							local->ReallocateChannels(buffer, output.ChannelCount);
							return true;
						} else {
							uint64 frames_equivalent = ConvertFrameCount(buffer->GetSizeInFrames(), output.FramesPerSecond, input.FramesPerSecond);
							uint64 frames_can_read = min(frames_count - frames_read, frames_equivalent);
							if (!frames_can_read) { buffer->FramesUsed() = 0; return true; }
							SafePointer<WaveBuffer> local = new WaveBuffer(output.Format, input.ChannelCount, input.FramesPerSecond, frames_can_read);
							if (!_internal_read_frames(local)) return false;
							local = local->ConvertFrameRate(output.FramesPerSecond);
							if (local->FramesUsed() > buffer->GetSizeInFrames()) local->FramesUsed() = buffer->GetSizeInFrames();
							if (input.ChannelCount != output.ChannelCount) {
								local->ReallocateChannels(buffer, output.ChannelCount);
							} else {
								MemoryCopy(buffer->GetData(), local->GetData(), local->GetUsedSizeInBytes());
								buffer->FramesUsed() = local->FramesUsed();
							}
							return true;
						}
					}
				} catch (...) { return false; }
			}
			virtual uint64 GetFramesCount(void) const override { return frames_count; }
			virtual uint64 GetCurrentFrame(void) const override { return frames_read; }
			virtual bool SetCurrentFrame(uint64 frame_index) override { if (ExtAudioFileSeek(wrapper->file_ex_ref, frame_index)) return false; return true; }
		};
		class CoreAudioEncoderStream : public IAudioEncoderStream
		{
			SafePointer<IAudioCodec> parent;
			SafePointer<CoreAudioFileWrapper> wrapper;
			string format_name;
			StreamDesc input, output;
			int state;

			volatile bool exit_thread;
			SafePointer<Thread> encoder_thread;
			SafePointer<Semaphore> queue_size_sem;
			SafePointer<Semaphore> access_sem;
			Array<bool *> status;
			ObjectArray<WaveBuffer> encode_queue;
			ObjectArray<Semaphore> on_encoded_open;
			ObjectArray<IDispatchTask> on_encoded_exec;

			bool _internal_write_frames(const WaveBuffer * buffer) noexcept
			{
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != input.Format || buffer_desc.ChannelCount != input.ChannelCount || buffer_desc.FramesPerSecond != input.FramesPerSecond) return false;
				try {
					if (input.ChannelCount != output.ChannelCount) {
						SafePointer<WaveBuffer> final_output = buffer->ReallocateChannels(output.ChannelCount);
						AudioBufferList list;
						list.mNumberBuffers = 1;
						list.mBuffers[0].mData = final_output->GetData();
						list.mBuffers[0].mDataByteSize = final_output->GetUsedSizeInBytes();
						list.mBuffers[0].mNumberChannels = output.ChannelCount;
						auto status = ExtAudioFileWrite(wrapper->file_ex_ref, final_output->FramesUsed(), &list);
						if (status) return false;
					} else {
						AudioBufferList list;
						list.mNumberBuffers = 1;
						list.mBuffers[0].mData = const_cast<void *>(reinterpret_cast<const void *>(buffer->GetData()));
						list.mBuffers[0].mDataByteSize = buffer->GetUsedSizeInBytes();
						list.mBuffers[0].mNumberChannels = output.ChannelCount;
						auto status = ExtAudioFileWrite(wrapper->file_ex_ref, buffer->FramesUsed(), &list);
						if (status) return false;
					}
				} catch (...) { return false; }
				return true;
			}
			static int _internal_encoder_thread(void * arg)
			{
				auto self = reinterpret_cast<CoreAudioEncoderStream *>(arg);
				while (true) {
					self->queue_size_sem->Wait();
					self->access_sem->Wait();
					if (self->encode_queue.Length()) {
						SafePointer<WaveBuffer> buffer;
						SafePointer<Semaphore> sem_open;
						SafePointer<IDispatchTask> task_exec;
						bool * status_ptr = self->status.FirstElement();
						buffer.SetRetain(self->encode_queue.FirstElement());
						sem_open.SetRetain(self->on_encoded_open.FirstElement());
						task_exec.SetRetain(self->on_encoded_exec.FirstElement());
						self->status.RemoveFirst();
						self->encode_queue.RemoveFirst();
						self->on_encoded_open.RemoveFirst();
						self->on_encoded_exec.RemoveFirst();
						self->access_sem->Open();
						bool local_status = self->_internal_write_frames(buffer);
						if (status_ptr) *status_ptr = local_status;
						if (sem_open) sem_open->Open();
						if (task_exec) task_exec->DoTask(0);
					} else if (self->exit_thread) {
						self->access_sem->Open();
						return 0;
					}
				}
				return 0;
			}
			bool _internal_enqueue_buffer(WaveBuffer * buffer, bool * write_status, Semaphore * open, IDispatchTask * task) noexcept
			{
				if (buffer) {
					access_sem->Wait();
					try {
						encode_queue.Append(buffer);
					} catch (...) {
						access_sem->Open();
						return false;
					}
					try {
						on_encoded_open.Append(open);
					} catch (...) {
						encode_queue.RemoveLast();
						access_sem->Open();
						return false;
					}
					try {
						on_encoded_exec.Append(task);
					} catch (...) {
						on_encoded_open.RemoveLast();
						encode_queue.RemoveLast();
						access_sem->Open();
						return false;
					}
					try {
						status.Append(write_status);
					} catch (...) {
						on_encoded_exec.RemoveLast();
						on_encoded_open.RemoveLast();
						encode_queue.RemoveLast();
						access_sem->Open();
						return false;
					}
					queue_size_sem->Open();
					access_sem->Open();
					return true;
				} else {
					exit_thread = true;
					queue_size_sem->Open();
					return true;
				}
			}
			bool _internal_async_init(void) noexcept
			{
				queue_size_sem = CreateSemaphore(0);
				access_sem = CreateSemaphore(1);
				if (!queue_size_sem || !access_sem) return false;
				encoder_thread = CreateThread(_internal_encoder_thread, this);
				if (!encoder_thread) return false;
				state = 1;
				exit_thread = false;
				return true;
			}
		public:
			CoreAudioEncoderStream(IAudioCodec * codec, Streaming::Stream * dest, const string & format, const StreamDesc & desc) :
				encode_queue(0x40), status(0x40), on_encoded_open(0x40), on_encoded_exec(0x40)
			{
				if (desc.Format == SampleFormat::Invalid || !desc.ChannelCount || !desc.FramesPerSecond) throw InvalidArgumentException();
				format_name = format;
				parent.SetRetain(codec);
				AudioStreamBasicDescription descriptor;
				AudioFileTypeID type_id;
				if (format == AudioFormatMPEG4AAC) {
					type_id = kAudioFileMPEG4Type;
					descriptor.mFormatID = kAudioFormatMPEG4AAC;
					descriptor.mFormatFlags = kMPEG4Object_AAC_Main;
				} else if (format == AudioFormatAppleLossless) {
					type_id = kAudioFileMPEG4Type;
					descriptor.mFormatID = kAudioFormatAppleLossless;
					descriptor.mFormatFlags = 0;
				} else throw InvalidArgumentException();
				descriptor.mBitsPerChannel = 0;
				descriptor.mBytesPerFrame = 0;
				descriptor.mBytesPerPacket = 0;
				descriptor.mChannelsPerFrame = desc.ChannelCount;
				descriptor.mReserved = 0;
				descriptor.mSampleRate = desc.FramesPerSecond;
				wrapper = new CoreAudioFileWrapper(dest, true, type_id, &descriptor);
				wrapper->GetFileStreamDesc(descriptor);
				output.Format = SampleFormat::Invalid;
				output.ChannelCount = descriptor.mChannelsPerFrame;
				output.FramesPerSecond = descriptor.mSampleRate;
				input = desc;
				descriptor.mBitsPerChannel = SampleFormatBitSize(input.Format);
				descriptor.mChannelsPerFrame = output.ChannelCount;
				descriptor.mBytesPerFrame = SampleFormatByteSize(input.Format) * output.ChannelCount;
				descriptor.mBytesPerPacket = descriptor.mBytesPerFrame;
				descriptor.mFormatFlags = IsFloatingPointFormat(input.Format) ? kLinearPCMFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger;
				descriptor.mFormatID = kAudioFormatLinearPCM;
				descriptor.mFramesPerPacket = 1;
				descriptor.mReserved = 0;
				descriptor.mSampleRate = input.FramesPerSecond;
				wrapper->SetClientStreamDesc(descriptor);
				state = 0;
			}
			virtual ~CoreAudioEncoderStream(void) override { Finalize(); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return input; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamEncoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return format_name; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return output; }
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				if (state || !buffer) return false;
				return _internal_write_frames(buffer);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				if (!state && !_internal_async_init()) return false;
				if (state != 1 || !buffer) return false;
				return _internal_enqueue_buffer(buffer, write_status, 0, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!state && !_internal_async_init()) return false;
				if (state != 1 || !buffer) return false;
				return _internal_enqueue_buffer(buffer, write_status, 0, execute_on_processed);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!state && !_internal_async_init()) return false;
				if (state != 1 || !buffer) return false;
				return _internal_enqueue_buffer(buffer, write_status, open_on_processed, 0);
			}
			virtual bool Finalize(void) noexcept override
			{
				if (state == 1) {
					if (!_internal_enqueue_buffer(0, 0, 0, 0)) return false;
					encoder_thread->Wait();
					state = 0;
					encoder_thread.SetReference(0);
					queue_size_sem.SetReference(0);
					access_sem.SetReference(0);
				}
				if (state) return false;
				wrapper.SetReference(0);
				state = 2;
				return true;
			}
		};
		class CoreAudioCodec : public IAudioCodec
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept override
			{
				if (format == AudioFormatMPEG4AAC || format == AudioFormatAppleLossless) return true;
				else return false;
			}
			virtual bool CanDecode(const string & format) const noexcept override
			{
				if (format == AudioFormatMPEG3 || format == AudioFormatMPEG4AAC ||
					format == AudioFormatAppleLossless || format == AudioFormatFreeLossless) return true;
				else return false;
			}
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatMPEG4AAC);
				list->Append(AudioFormatAppleLossless);
				list->Retain();
				return list;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatMPEG3);
				list->Append(AudioFormatMPEG4AAC);
				list->Append(AudioFormatAppleLossless);
				list->Append(AudioFormatFreeLossless);
				list->Retain();
				return list;
			}
			virtual string GetCodecName(void) const override { return L"Core Audio Codec"; }
			virtual IAudioDecoderStream * TryDecode(Streaming::Stream * source, const StreamDesc * desired_desc) noexcept override
			{
				try {
					source->Seek(0, Streaming::Begin);
					SafePointer<CoreAudioFileWrapper> wrapper = new CoreAudioFileWrapper(source, false, 0, 0);
					AudioStreamBasicDescription internal, external;
					StreamDesc input, output;
					string format_name;
					wrapper->GetFileStreamDesc(internal);
					if (internal.mFormatID == kAudioFormatMPEGLayer3) format_name = AudioFormatMPEG3;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_ELD) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_ELD_SBR) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_ELD_V2) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_HE) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_HE_V2) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_LD) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatMPEG4AAC_Spatial) format_name = AudioFormatMPEG4AAC;
					else if (internal.mFormatID == kAudioFormatAppleLossless) format_name = AudioFormatAppleLossless;
					else if (internal.mFormatID == kAudioFormatFLAC) format_name = AudioFormatFreeLossless;
					else throw InvalidFormatException();
					input.Format = SampleFormat::Invalid;
					input.ChannelCount = internal.mChannelsPerFrame;
					input.FramesPerSecond = internal.mSampleRate;
					external.mFormatID = kAudioFormatLinearPCM;
					if (desired_desc && desired_desc->Format != SampleFormat::Invalid) {
						output.Format = desired_desc->Format;
						external.mBitsPerChannel = SampleFormatBitSize(desired_desc->Format);
						external.mFormatFlags = IsFloatingPointFormat(desired_desc->Format) ? kLinearPCMFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger;
					} else {
						output.Format = SampleFormat::S16_snorm;
						external.mBitsPerChannel = 16;
						external.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
					}
					external.mChannelsPerFrame = internal.mChannelsPerFrame;
					if (desired_desc && desired_desc->ChannelCount) {
						output.ChannelCount = desired_desc->ChannelCount;
					} else {
						output.ChannelCount = external.mChannelsPerFrame;
					}
					external.mSampleRate = internal.mSampleRate;
					if (desired_desc && desired_desc->FramesPerSecond) {
						output.FramesPerSecond = desired_desc->FramesPerSecond;
					} else {
						output.FramesPerSecond = external.mSampleRate;
					}
					external.mBytesPerFrame = SampleFormatByteSize(desired_desc->Format) * external.mChannelsPerFrame;
					external.mBytesPerPacket = external.mBytesPerFrame;
					external.mFramesPerPacket = 1;
					external.mReserved = 0;
					wrapper->SetClientStreamDesc(external);
					SafePointer<CoreAudioDecoderStream> decoder = new CoreAudioDecoderStream;
					decoder->wrapper = wrapper;
					decoder->parent.SetRetain(this);
					decoder->format = format_name;
					decoder->input = input;
					decoder->output = output;
					decoder->frames_count = wrapper->GetFramesCount();
					decoder->Retain();
					return decoder;
				} catch (...) {
					try { source->Seek(0, Streaming::Begin); } catch (...) {}
					return 0;
				}
			}
			virtual IAudioEncoderStream * Encode(Streaming::Stream * dest, const string & format, const StreamDesc & desc) noexcept override
			{
				try {
					if (format == AudioFormatMPEG4AAC) return new CoreAudioEncoderStream(this, dest, AudioFormatMPEG4AAC, desc);
					else if (format == AudioFormatAppleLossless) return new CoreAudioEncoderStream(this, dest, AudioFormatAppleLossless, desc);
					else return 0;
				} catch (...) { return 0; }
			}
		};

		void CoreAudioProduceStreamDescriptor(AudioStreamBasicDescription & base, StreamDesc & desc)
		{
			desc.Format = SampleFormat::S32_snorm;
			desc.ChannelCount = base.mChannelsPerFrame;
			desc.FramesPerSecond = base.mSampleRate;
			if (base.mFormatID == kAudioFormatLinearPCM) {
				if (base.mFormatFlags & kLinearPCMFormatFlagIsFloat) {
					if (base.mBitsPerChannel == 32) desc.Format = SampleFormat::S32_float;
					else if (base.mBitsPerChannel == 64) desc.Format = SampleFormat::S64_float;
				} else {
					if (base.mBitsPerChannel == 32) desc.Format = SampleFormat::S32_snorm;
					else if (base.mBitsPerChannel == 24) desc.Format = SampleFormat::S24_snorm;
					else if (base.mBitsPerChannel == 16) desc.Format = SampleFormat::S16_snorm;
					else if (base.mBitsPerChannel == 8) desc.Format = SampleFormat::S8_snorm;
				}
			}
			base.mFormatID = kAudioFormatLinearPCM;
			base.mFormatFlags = kLinearPCMFormatFlagIsPacked;
			if (IsFloatingPointFormat(desc.Format)) base.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
			else base.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
			base.mBitsPerChannel = SampleFormatBitSize(desc.Format);
			base.mBytesPerFrame = StreamFrameByteSize(desc);
			base.mBytesPerPacket = base.mBytesPerFrame;
			base.mFramesPerPacket = 1;
			base.mSampleRate = desc.FramesPerSecond;
		}
		class CoreAudioBufferWrapper : public Object
		{
		public:
			AudioQueueBufferRef buffer;
			SafePointer<WaveBuffer> store_at;
			bool * write_status;
			SafePointer<Semaphore> open;
			SafePointer<IDispatchTask> exec;

			CoreAudioBufferWrapper(void) {};
			virtual ~CoreAudioBufferWrapper(void) override {}
			void Finalize(AudioQueueRef queue)
			{
				if (store_at) {
					store_at->FramesUsed() = buffer->mAudioDataByteSize / StreamFrameByteSize(store_at->GetFormatDescriptor());
					MemoryCopy(store_at->GetData(), buffer->mAudioData, buffer->mAudioDataByteSize);
				}
				OSStatus status = AudioQueueFreeBuffer(queue, buffer);
				if (write_status) *write_status = status == 0;
				if (open) open->Open();
				if (exec) exec->DoTask(0);
			}
		};

		class CoreAudioOutputDevice : public IAudioOutputDevice
		{
			StreamDesc desc;
			AudioQueueRef queue;
			AudioDeviceID device; 
			int state; // 0 - stopped, 1 - processing, 2 - paused

			static void Queue_Callback(void * user, AudioQueueRef queue, AudioQueueBufferRef buffer)
			{
				auto wrapper = reinterpret_cast<CoreAudioBufferWrapper *>(buffer->mUserData);
				wrapper->Finalize(queue);
				wrapper->Release();
			}
			bool _internal_write_frames(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed, Semaphore * open_on_processed)
			{
				if (!buffer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (desc.Format != buffer_desc.Format || desc.ChannelCount != buffer_desc.ChannelCount || desc.FramesPerSecond != buffer_desc.FramesPerSecond) return false;
				AudioQueueBufferRef local;
				OSStatus status = AudioQueueAllocateBuffer(queue, buffer->GetAllocatedSizeInBytes(), &local);
				if (status) return false;
				local->mAudioDataByteSize = buffer->GetUsedSizeInBytes();
				MemoryCopy(local->mAudioData, buffer->GetData(), buffer->GetUsedSizeInBytes());
				CoreAudioBufferWrapper * wrapper = new (std::nothrow) CoreAudioBufferWrapper;
				if (!wrapper) { AudioQueueFreeBuffer(queue, local); return false; }
				wrapper->buffer = local;
				local->mUserData = wrapper;
				wrapper->write_status = write_status;
				wrapper->open.SetRetain(open_on_processed);
				wrapper->exec.SetRetain(execute_on_processed);
				status = AudioQueueEnqueueBuffer(queue, local, 0, 0);
				if (status) {
					AudioQueueFreeBuffer(queue, local);
					wrapper->Release();
					return false;
				}
				return true;
			}
		public:
			CoreAudioOutputDevice(AudioDeviceID dev_id, CFStringRef dev_uid, AudioStreamBasicDescription & dev_desc) : device(dev_id)
			{
				CoreAudioProduceStreamDescriptor(dev_desc, desc);
				OSStatus status = AudioQueueNewOutput(&dev_desc, Queue_Callback, this, 0, 0, 0, &queue);
				if (status) throw Exception();
				if (dev_uid) {
					status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &dev_uid, sizeof(dev_uid));
					if (status) { AudioQueueDispose(queue, true); throw Exception(); }
				}
				state = 0;
			}
			virtual ~CoreAudioOutputDevice(void) override { AudioQueueDispose(queue, false); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return desc; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceOutput; }
			virtual string GetDeviceIdentifier(void) const override { return string(device); }
			virtual double GetVolume(void) noexcept override
			{
				AudioQueueParameterValue value;
				OSStatus status = AudioQueueGetParameter(queue, kAudioQueueParam_Volume, &value);
				if (status) return -1.0;
				return value;
			}
			virtual void SetVolume(double volume) noexcept override { AudioQueueSetParameter(queue, kAudioQueueParam_Volume, volume); }
			virtual bool StartProcessing(void) noexcept override
			{
				if (state == 0 || state == 2) {
					OSStatus status = AudioQueueStart(queue, 0);
					if (status) return false;
					state = 1;
					return true;
				} else return false;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (state == 1) {
					OSStatus status = AudioQueuePause(queue);
					if (status) return false;
					state = 2;
					return true;
				} else return false;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				if (state == 1 || state == 2) {
					OSStatus status = AudioQueueStop(queue, true);
					if (status) return false;
					state = 0;
					return true;
				} else return false;
			}
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				if (!semaphore) return false;
				bool status;
				if (!WriteFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				return _internal_write_frames(buffer, write_status, 0, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!execute_on_processed) return false;
				return _internal_write_frames(buffer, write_status, execute_on_processed, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!open_on_processed) return false;
				return _internal_write_frames(buffer, write_status, 0, open_on_processed);
			}
		};
		class CoreAudioInputDevice : public IAudioInputDevice
		{
			StreamDesc desc;
			AudioQueueRef queue;
			AudioDeviceID device;
			int state; // 0 - stopped, 1 - processing, 2 - paused

			static void Queue_Callback(void * user, AudioQueueRef queue, AudioQueueBufferRef buffer, const AudioTimeStamp *, uint32, const AudioStreamPacketDescription *)
			{
				auto wrapper = reinterpret_cast<CoreAudioBufferWrapper *>(buffer->mUserData);
				wrapper->Finalize(queue);
				wrapper->Release();
			}
			bool _internal_read_frames(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed, Semaphore * open_on_processed)
			{
				if (!buffer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (desc.Format != buffer_desc.Format || desc.ChannelCount != buffer_desc.ChannelCount || desc.FramesPerSecond != buffer_desc.FramesPerSecond) return false;
				AudioQueueBufferRef local;
				OSStatus status = AudioQueueAllocateBuffer(queue, buffer->GetAllocatedSizeInBytes(), &local);
				if (status) return false;
				CoreAudioBufferWrapper * wrapper = new (std::nothrow) CoreAudioBufferWrapper;
				if (!wrapper) { AudioQueueFreeBuffer(queue, local); return false; }
				wrapper->buffer = local;
				local->mUserData = wrapper;
				wrapper->store_at.SetRetain(buffer);
				wrapper->write_status = read_status;
				wrapper->open.SetRetain(open_on_processed);
				wrapper->exec.SetRetain(execute_on_processed);
				status = AudioQueueEnqueueBuffer(queue, local, 0, 0);
				if (status) {
					AudioQueueFreeBuffer(queue, local);
					wrapper->Release();
					return false;
				}
				return true;
			}
		public:
			CoreAudioInputDevice(AudioDeviceID dev_id, CFStringRef dev_uid, AudioStreamBasicDescription & dev_desc) : device(dev_id)
			{
				CoreAudioProduceStreamDescriptor(dev_desc, desc);
				OSStatus status = AudioQueueNewInput(&dev_desc, Queue_Callback, this, 0, 0, 0, &queue);
				if (status) throw Exception();
				if (dev_uid) {
					status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &dev_uid, sizeof(dev_uid));
					if (status) { AudioQueueDispose(queue, true); throw Exception(); }
				}
				state = 0;
			}
			virtual ~CoreAudioInputDevice(void) override { AudioQueueDispose(queue, false); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return desc; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceInput; }
			virtual string GetDeviceIdentifier(void) const override { return string(device); }
			virtual double GetVolume(void) noexcept override
			{
				AudioQueueParameterValue value;
				OSStatus status = AudioQueueGetParameter(queue, kAudioQueueParam_Volume, &value);
				if (status) return -1.0;
				return value;
			}
			virtual void SetVolume(double volume) noexcept override { AudioQueueSetParameter(queue, kAudioQueueParam_Volume, volume); }
			virtual bool StartProcessing(void) noexcept override
			{
				if (state == 0 || state == 2) {
					OSStatus status = AudioQueueStart(queue, 0);
					if (status) return false;
					state = 1;
					return true;
				} else return false;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (state == 1) {
					OSStatus status = AudioQueuePause(queue);
					if (status) return false;
					state = 2;
					return true;
				} else return false;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				if (state == 1 || state == 2) {
					OSStatus status = AudioQueueStop(queue, true);
					if (status) return false;
					state = 0;
					return true;
				} else return false;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				if (!semaphore) return false;
				bool status;
				if (!ReadFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status) noexcept override
			{
				return _internal_read_frames(buffer, read_status, 0, 0);
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!execute_on_processed) return false;
				return _internal_read_frames(buffer, read_status, execute_on_processed, 0);
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, Semaphore * open_on_processed) noexcept override
			{
				if (!open_on_processed) return false;
				return _internal_read_frames(buffer, read_status, 0, open_on_processed);
			}
		};
		class CoreAudioDeviceFactory : public IAudioDeviceFactory
		{
			SafePointer<Semaphore> access_sync;
			Array<IAudioEventCallback *> callbacks;
			Array<AudioDeviceID> devs_out, devs_in;

			static bool _check_device_io(AudioDeviceID dev, AudioObjectPropertyScope scope)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyStreamConfiguration;
				address.mScope = scope;
				address.mElement = 0;
				uint32 size;
				OSStatus status = AudioObjectGetPropertyDataSize(dev, &address, 0, 0, &size);
				if (status) return false;
				AudioBufferList * list = reinterpret_cast<AudioBufferList *>(malloc(size));
				if (!list) return false;
				status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, list);
				if (status) { free(list); return false; }
				bool has_io = false;
				for (int i = 0; i < list->mNumberBuffers; i++) if (list->mBuffers[i].mNumberChannels) { has_io = true; break; }
				free(list);
				return has_io;
			}
			static CFStringRef _get_device_uid(AudioDeviceID dev)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyDeviceUID;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				CFStringRef uid;
				uint32 size = sizeof(uid);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &uid);
				if (status) return 0;
				return uid;
			}
			static string _get_device_name(AudioDeviceID dev)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyDeviceNameCFString;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				CFStringRef name;
				uint32 size = sizeof(name);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &name);
				if (status) throw Exception();
				Array<UniChar> buffer(0x100);
				buffer.SetLength(CFStringGetLength(name));
				CFStringGetCharacters(name, CFRangeMake(0, buffer.Length()), buffer.GetBuffer());
				CFRelease(name);
				return string(buffer.GetBuffer(), buffer.Length(), Encoding::UTF16);
			}
			static AudioDeviceID _get_default_device(AudioObjectPropertySelector selector)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = selector;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				AudioDeviceID result;
				uint32 size = sizeof(result);
				OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, 0, &size, &result);
				if (status) throw Exception();
				return result;
			}
			static void _get_device_stream_format(AudioDeviceID dev, AudioObjectPropertyScope scope, AudioStreamBasicDescription & desc)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyStreamFormat;
				address.mScope = scope;
				address.mElement = kAudioObjectPropertyElementMaster;
				uint32 size = sizeof(desc);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &desc);
				if (status) throw Exception();
			}
			static Array<AudioDeviceID> * _list_audio_devices(void)
			{
				SafePointer< Array<AudioDeviceID> > devs = new Array<AudioDeviceID>(0x10);
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				uint32 size;
				OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, 0, &size);
				if (status) throw Exception();
				devs->SetLength(size / sizeof(AudioDeviceID));
				status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, 0, &size, devs->GetBuffer());
				if (status) throw Exception();
				devs->Retain();
				return devs;
			}

			void _raise_event(AudioDeviceEvent event, AudioObjectType type, const string & dev_id) { for (auto & callback : callbacks) callback->OnAudioDeviceEvent(event, type, dev_id); }
			static OSStatus _device_change_listener(AudioObjectID object, uint32 num_address, const AudioObjectPropertyAddress * addresses, void * user)
			{
				auto self = reinterpret_cast<CoreAudioDeviceFactory *>(user);
				self->access_sync->Wait();
				if (object == kAudioObjectSystemObject) for (int i = 0; i < num_address; i++) {
					if (addresses[i].mScope == kAudioObjectPropertyScopeGlobal && addresses[i].mElement == kAudioObjectPropertyElementMaster) {
						if (addresses[i].mSelector == kAudioHardwarePropertyDevices) {
							try {
								SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
								for (auto & dev : self->devs_out) {
									bool present = false;
									for (auto & dev2 : *devs) if (dev2 == dev) { present = true; break; }
									if (!present) self->_raise_event(AudioDeviceEvent::Inactivated, AudioObjectType::DeviceOutput, string(dev));
								}
								for (auto & dev : self->devs_in) {
									bool present = false;
									for (auto & dev2 : *devs) if (dev2 == dev) { present = true; break; }
									if (!present) self->_raise_event(AudioDeviceEvent::Inactivated, AudioObjectType::DeviceInput, string(dev));
								}
								for (auto & dev : *devs) {
									if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) {
										bool present = false;
										for (auto & dev2 : self->devs_out) if (dev2 == dev) { present = true; break; }
										if (!present) self->_raise_event(AudioDeviceEvent::Activated, AudioObjectType::DeviceOutput, string(dev));
									} else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) {
										bool present = false;
										for (auto & dev2 : self->devs_in) if (dev2 == dev) { present = true; break; }
										if (!present) self->_raise_event(AudioDeviceEvent::Activated, AudioObjectType::DeviceInput, string(dev));
									}
								}
								self->devs_in.Clear();
								self->devs_out.Clear();
								for (auto & dev : *devs) {
									if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) self->devs_out << dev;
									else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) self->devs_in << dev;
								}
							} catch (...) {}
						} else if (addresses[i].mSelector == kAudioHardwarePropertyDefaultOutputDevice) {
							try {
								self->_raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceOutput, string(_get_default_device(addresses[i].mSelector)));
							} catch (...) {}
						} else if (addresses[i].mSelector == kAudioHardwarePropertyDefaultInputDevice) {
							try {
								self->_raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceInput, string(_get_default_device(addresses[i].mSelector)));
							} catch (...) {}
						}
					}
				}
				self->access_sync->Open();
				return 0;
			}
		public:
			CoreAudioDeviceFactory(void) : callbacks(0x10), devs_out(0x10), devs_in(0x10)
			{
				access_sync = CreateSemaphore(1);
				if (!access_sync) throw Exception();
				SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
				for (auto & dev : *devs) {
					if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) devs_out << dev;
					else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) devs_in << dev;
				}
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) throw Exception();
				address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) {
					address.mSelector = kAudioHardwarePropertyDevices;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					throw Exception();
				}
				address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) {
					address.mSelector = kAudioHardwarePropertyDevices;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					throw Exception();
				}
			}
			virtual ~CoreAudioDeviceFactory(void) override
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
				address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
				address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
			}
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableOutputDevices(void) noexcept override
			{
				try {
					SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
					SafePointer< Dictionary::PlainDictionary<string, string> > result = new Dictionary::PlainDictionary<string, string>(0x10);
					for (auto & dev : *devs) if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) result->Append(dev, _get_device_name(dev));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableInputDevices(void) noexcept override
			{
				try {
					SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
					SafePointer< Dictionary::PlainDictionary<string, string> > result = new Dictionary::PlainDictionary<string, string>(0x10);
					for (auto & dev : *devs) if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) result->Append(dev, _get_device_name(dev));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IAudioOutputDevice * CreateOutputDevice(const string & identifier) noexcept override
			{
				CFStringRef uid = 0;
				try {
					AudioDeviceID dev_id = identifier.ToUInt32();
					if (!_check_device_io(dev_id, kAudioDevicePropertyScopeOutput)) throw Exception();
					uid = _get_device_uid(dev_id);
					if (!uid) throw Exception();
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeOutput, desc);
					IAudioOutputDevice * device = new CoreAudioOutputDevice(dev_id, uid, desc);
					CFRelease(uid);
					return device;
				} catch (...) { if (uid) CFRelease(uid); return 0; }
			}
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept override
			{
				try {
					AudioDeviceID dev_id = _get_default_device(kAudioHardwarePropertyDefaultOutputDevice);
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeOutput, desc);
					return new CoreAudioOutputDevice(dev_id, 0, desc);
				} catch (...) { return 0; }
			}
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept override
			{
				CFStringRef uid = 0;
				try {
					AudioDeviceID dev_id = identifier.ToUInt32();
					if (!_check_device_io(dev_id, kAudioDevicePropertyScopeInput)) throw Exception();
					uid = _get_device_uid(dev_id);
					if (!uid) throw Exception();
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeInput, desc);
					IAudioInputDevice * device = new CoreAudioInputDevice(dev_id, uid, desc);
					CFRelease(uid);
					return device;
				} catch (...) { if (uid) CFRelease(uid); return 0; }
			}
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept override
			{
				try {
					AudioDeviceID dev_id = _get_default_device(kAudioHardwarePropertyDefaultInputDevice);
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeInput, desc);
					return new CoreAudioInputDevice(dev_id, 0, desc);
				} catch (...) { return 0; }
			}
			virtual bool RegisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				access_sync->Wait();
				try {
					for (auto & cb : callbacks) if (cb == callback) { access_sync->Open(); return true; }
					callbacks.Append(callback);
					access_sync->Open();
					return true;
				} catch (...) { access_sync->Open(); return false; }
			}
			virtual bool UnregisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				access_sync->Wait();
				for (int i = 0; i < callbacks.Length(); i++) if (callbacks[i] == callback) {
					callbacks.Remove(i);
					access_sync->Open();
					return true;
				}
				access_sync->Open();
				return false;
			}
		};

		SafePointer<IAudioCodec> _system_codec;
		IAudioCodec * InitializeSystemCodec(void)
		{
			if (!_system_codec) {
				_system_codec = new CoreAudioCodec;
				RegisterCodec(_system_codec);
			}
			return _system_codec;
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return new CoreAudioDeviceFactory; }
		void SystemBeep(void) { AudioServicesPlayAlertSound(kSystemSoundID_UserPreferredAlert); }
	}
}