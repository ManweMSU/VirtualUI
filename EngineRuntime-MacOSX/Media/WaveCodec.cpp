#include "WaveCodec.h"

namespace Engine
{
	namespace Audio
	{
		ENGINE_PACKED_STRUCTURE(WaveFullHeader)
			char riff_sign[4];        // "RIFF"
			uint32 file_size;
			char wave_sign[4];        // "WAVE"
			char wave_header_sign[4]; // "fmt "
			uint32 header_size;       // 16
			uint16 data_format;       // 1 - snorm PCM
			uint16 num_channels;
			uint32 frames_per_second;
			uint32 bytes_per_second;
			uint16 bytes_per_frame;
			uint16 bits_per_sample;
			char data_sign[4];        // "data"
			uint32 data_length;
		ENGINE_END_PACKED_STRUCTURE
		ENGINE_PACKED_STRUCTURE(WaveCoreHeader)
			uint16 data_format;
			uint16 num_channels;
			uint32 frames_per_second;
			uint32 bytes_per_second;
			uint16 bytes_per_frame;
			uint16 bits_per_sample;
		ENGINE_END_PACKED_STRUCTURE
		ENGINE_PACKED_STRUCTURE(EngineRawAudioHeader)
			char signature[8];        // "erawau\0\0"
			uint32 version;           // 0
			uint32 fd_sample_format;
			uint32 fd_channel_count;
			uint32 fd_frame_rate;
			uint64 num_frames;
		ENGINE_END_PACKED_STRUCTURE

		class WaveDecoderStream : public IAudioDecoderStream
		{
			SafePointer<IAudioCodec> parent;
			SafePointer<Streaming::Stream> stream;
			StreamDesc input, output;
			string format;
			uint64 frames_read, frames_count;
			uint64 data_offset, data_size;
			friend class WaveCodec;
			bool _internal_read_frames(WaveBuffer * buffer) noexcept
			{
				try {
					stream->Seek(data_offset + frames_read * StreamFrameByteSize(input), Streaming::Begin);
					uint64 frames_can_read = min(frames_count - frames_read, buffer->GetSizeInFrames());
					uint64 byte_size = frames_can_read * StreamFrameByteSize(input);
					stream->Read(buffer->GetData(), uint32(byte_size));
					frames_read += frames_can_read;
					buffer->FramesUsed() = frames_can_read;
					return true;
				} catch (...) { return false; }
			}
		public:
			WaveDecoderStream(void) : frames_read(0) {}
			virtual ~WaveDecoderStream(void) override {}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return output; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamDecoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return format; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return input; }
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (buffer_desc.Format != output.Format || buffer_desc.ChannelCount != output.ChannelCount || buffer_desc.FramesPerSecond != output.FramesPerSecond) return false;
				auto prev_frames_read = frames_read;
				try {
					if (input.Format == output.Format && input.ChannelCount == output.ChannelCount && input.FramesPerSecond == output.FramesPerSecond) {
						return _internal_read_frames(buffer);
					} else {
						if (input.FramesPerSecond == output.FramesPerSecond) {
							uint64 frames_can_read = min(frames_count - frames_read, buffer->GetSizeInFrames());
							if (!frames_can_read) { buffer->FramesUsed() = 0; return true; }
							SafePointer<WaveBuffer> local = new WaveBuffer(input, frames_can_read);
							if (!_internal_read_frames(local)) { frames_read = prev_frames_read; return false; }
							if (input.Format != output.Format && input.ChannelCount == output.ChannelCount) {
								local->ConvertFormat(buffer, output.Format);
							} else if (input.Format == output.Format && input.ChannelCount != output.ChannelCount) {
								local->ReallocateChannels(buffer, output.ChannelCount);
							} else {
								local = local->ConvertFormat(output.Format);
								local->ReallocateChannels(buffer, output.ChannelCount);
							}
							return true;
						} else {
							uint64 frames_equivalent = ConvertFrameCount(buffer->GetSizeInFrames(), output.FramesPerSecond, input.FramesPerSecond);
							uint64 frames_can_read = min(frames_count - frames_read, frames_equivalent);
							if (!frames_can_read) { buffer->FramesUsed() = 0; return true; }
							SafePointer<WaveBuffer> local = new WaveBuffer(input, frames_can_read);
							if (!_internal_read_frames(local)) { frames_read = prev_frames_read; return false; }
							local = local->ConvertFrameRate(output.FramesPerSecond);
							if (local->FramesUsed() > buffer->GetSizeInFrames()) local->FramesUsed() = buffer->GetSizeInFrames();
							if (input.Format != output.Format && input.ChannelCount == output.ChannelCount) {
								local->ConvertFormat(buffer, output.Format);
							} else if (input.Format == output.Format && input.ChannelCount != output.ChannelCount) {
								local->ReallocateChannels(buffer, output.ChannelCount);
							} else if (input.Format != output.Format && input.ChannelCount != output.ChannelCount) {
								local = local->ConvertFormat(output.Format);
								local->ReallocateChannels(buffer, output.ChannelCount);
							} else {
								MemoryCopy(buffer->GetData(), local->GetData(), intptr(local->GetUsedSizeInBytes()));
								buffer->FramesUsed() = local->FramesUsed();
							}
							return true;
						}
					}
				} catch (...) { frames_read = prev_frames_read; return false; }
			}
			virtual uint64 GetFramesCount(void) const override { return frames_count; }
			virtual uint64 GetCurrentFrame(void) const override { return frames_read; }
			virtual bool SetCurrentFrame(uint64 frame_index) override { if (frame_index > frames_count) return false; frames_read = frame_index; return true; }
		};
		class WaveEncoderStream : public IAudioEncoderStream
		{
			string internal_format;
			StreamDesc input_desc, output_desc;
			SafePointer<IAudioCodec> parent;
			SafePointer<Streaming::Stream> output;
			WaveFullHeader header;
			EngineRawAudioHeader engine_header;
			int state; // 0 - sync, 1 - async, 2 - finilized
			uint64 bytes_written;

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
				if (buffer_desc.Format != input_desc.Format || buffer_desc.ChannelCount != input_desc.ChannelCount || buffer_desc.FramesPerSecond != input_desc.FramesPerSecond) return false;
				try {
					uint64 buffer_size;
					if (input_desc.Format != output_desc.Format) {
						SafePointer<WaveBuffer> final_output = buffer->ConvertFormat(output_desc.Format);
						buffer_size = final_output->GetUsedSizeInBytes();
						output->Write(final_output->GetData(), uint32(buffer_size));
					} else {
						buffer_size = buffer->GetUsedSizeInBytes();
						output->Write(buffer->GetData(), uint32(buffer_size));
					}
					bytes_written += buffer_size;
				} catch (...) { return false; }
				return true;
			}
			static int _internal_encoder_thread(void * arg)
			{
				auto self = reinterpret_cast<WaveEncoderStream *>(arg);
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
			WaveEncoderStream(IAudioCodec * codec, Streaming::Stream * stream, const StreamDesc & desc, bool engine_raw) :
				encode_queue(0x40), status(0x40), on_encoded_open(0x40), on_encoded_exec(0x40)
			{
				if (engine_raw) internal_format = AudioFormatEngineRaw; else internal_format = AudioFormatWaveform;
				if (!desc.ChannelCount || desc.Format == SampleFormat::Invalid || !desc.FramesPerSecond) throw InvalidArgumentException();
				input_desc = desc;
				output_desc = desc;
				if (IsFloatingPointFormat(output_desc.Format) && !engine_raw) output_desc.Format = SampleFormat::S32_snorm;
				state = 0;
				bytes_written = 0;
				parent.SetRetain(codec);
				output.SetRetain(stream);
				ZeroMemory(&header, sizeof(header));
				ZeroMemory(&engine_header, sizeof(engine_header));
				if (engine_raw) {
					output->Write(&engine_header, sizeof(engine_header));
					MemoryCopy(engine_header.signature, "erawau\0\0", 8);
					engine_header.fd_sample_format = static_cast<uint32>(output_desc.Format);
					engine_header.fd_channel_count = output_desc.ChannelCount;
					engine_header.fd_frame_rate = output_desc.FramesPerSecond;
				} else {
					output->Write(&header, sizeof(header));
					MemoryCopy(header.riff_sign, "RIFF", 4);
					MemoryCopy(header.wave_sign, "WAVE", 4);
					MemoryCopy(header.wave_header_sign, "fmt ", 4);
					MemoryCopy(header.data_sign, "data", 4);
					header.header_size = 16;
					header.data_format = 1;
					header.num_channels = output_desc.ChannelCount;
					header.frames_per_second = output_desc.FramesPerSecond;
					header.bytes_per_frame = StreamFrameByteSize(output_desc);
					header.bytes_per_second = uint32(header.bytes_per_frame) * header.frames_per_second;
					header.bits_per_sample = SampleFormatBitSize(output_desc.Format);
				}
			}
			virtual ~WaveEncoderStream(void) override { Finalize(); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return input_desc; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::StreamEncoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return parent; }
			virtual string GetInternalFormat(void) const override { return internal_format; }
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept override { return output_desc; }
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
				if (internal_format == AudioFormatWaveform) {
					if (bytes_written > 0xFFFFFFFF - sizeof(header)) return false;
					header.data_length = uint32(bytes_written);
					header.file_size = uint32(bytes_written + sizeof(header) - 8);
					try {
						if (bytes_written & 1) {
							uint8 padding = 0;
							output->Write(&padding, 1);
						}
						output->Seek(0, Streaming::Begin);
						output->Write(&header, sizeof(header));
						output->Seek(0, Streaming::End);
						output.SetReference(0);
					} catch (...) { return false; }
					state = 2;
					return true;
				} else {
					engine_header.num_frames = bytes_written / StreamFrameByteSize(output_desc);
					try {
						output->Seek(0, Streaming::Begin);
						output->Write(&engine_header, sizeof(engine_header));
						output->Seek(0, Streaming::End);
						output.SetReference(0);
					} catch (...) { return false; }
					state = 2;
					return true;
				}
			}
		};
		class WaveCodec : public IAudioCodec
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept override
			{
				if (format == AudioFormatWaveform || format == AudioFormatEngineRaw) return true;
				else return false;
			}
			virtual bool CanDecode(const string & format) const noexcept override
			{
				if (format == AudioFormatWaveform || format == AudioFormatEngineRaw) return true;
				else return false;
			}
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatWaveform);
				list->Append(AudioFormatEngineRaw);
				list->Retain();
				return list;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatWaveform);
				list->Append(AudioFormatEngineRaw);
				list->Retain();
				return list;
			}
			virtual string GetCodecName(void) const override { return L"Engine Waveform Codec"; }
			virtual IAudioDecoderStream * TryDecode(Streaming::Stream * source, const StreamDesc * desired_desc) noexcept override
			{
				try {
					char sign[12];
					StreamDesc source_desc, out_desc;
					uint64 data_offset, data_size, num_frames;
					string format_code;
					source->Seek(0, Streaming::Begin);
					source->Read(sign, 12);
					source->Seek(0, Streaming::Begin);
					if (MemoryCompare(sign, "RIFF", 4) == 0 && MemoryCompare(sign + 8, "WAVE", 4) == 0) {
						format_code = AudioFormatWaveform;
						uint32 main_size, chunk_size;
						char chunk_name[4];
						source->Seek(4, Streaming::Begin);
						source->Read(&main_size, 4);
						source->Seek(12, Streaming::Begin);
						while (source->Seek(0, Streaming::Current) <= int64(main_size)) {
							source->Read(chunk_name, 4);
							source->Read(&chunk_size, 4);
							if (MemoryCompare(chunk_name, "fmt ", 4) == 0) {
								if (chunk_size != 16) throw Exception();
								WaveCoreHeader header;
								source->Read(&header, sizeof(header));
								if (header.data_format != 1) throw Exception();
								if (!header.num_channels) throw Exception();
								if (!header.frames_per_second) throw Exception();
								source_desc.ChannelCount = header.num_channels;
								source_desc.FramesPerSecond = header.frames_per_second;
								if (header.bits_per_sample == 32) source_desc.Format = SampleFormat::S32_snorm;
								else if (header.bits_per_sample == 24) source_desc.Format = SampleFormat::S24_snorm;
								else if (header.bits_per_sample == 16) source_desc.Format = SampleFormat::S16_snorm;
								else if (header.bits_per_sample == 8) source_desc.Format = SampleFormat::S8_snorm;
								else throw Exception();
								if (header.bytes_per_frame != StreamFrameByteSize(source_desc)) throw Exception();
							} else if (MemoryCompare(chunk_name, "data", 4) == 0) {
								data_offset = source->Seek(0, Streaming::Current);
								data_size = chunk_size;
								if (chunk_size & 1) chunk_size++;
								source->Seek(chunk_size, Streaming::Current);
							} else {
								if (chunk_size & 1) chunk_size++;
								source->Seek(chunk_size, Streaming::Current);
							}
						}
						num_frames = data_size / StreamFrameByteSize(source_desc);
					} else if (MemoryCompare(sign, "erawau\0\0\0\0\0\0", 12) == 0) {
						format_code = AudioFormatEngineRaw;
						EngineRawAudioHeader header;
						source->Read(&header, sizeof(header));
						source_desc.Format = static_cast<SampleFormat>(header.fd_sample_format);
						source_desc.ChannelCount = header.fd_channel_count;
						source_desc.FramesPerSecond = header.fd_frame_rate;
						if (source_desc.Format == SampleFormat::Invalid) throw Exception();
						if (!source_desc.ChannelCount) throw Exception();
						if (!source_desc.FramesPerSecond) throw Exception();
						num_frames = header.num_frames;
						data_offset = sizeof(header);
						data_size = num_frames * StreamFrameByteSize(source_desc);
					} else return 0;
					if (desired_desc) {
						if (desired_desc->Format != SampleFormat::Invalid) out_desc.Format = desired_desc->Format; else out_desc.Format = source_desc.Format;
						if (desired_desc->ChannelCount) out_desc.ChannelCount = desired_desc->ChannelCount; else out_desc.ChannelCount = source_desc.ChannelCount;
						if (desired_desc->FramesPerSecond) out_desc.FramesPerSecond = desired_desc->FramesPerSecond; else out_desc.FramesPerSecond = source_desc.FramesPerSecond;
					} else out_desc = source_desc;
					SafePointer<WaveDecoderStream> decoder = new WaveDecoderStream;
					decoder->parent.SetRetain(this);
					decoder->stream.SetRetain(source);
					decoder->input = source_desc;
					decoder->output = out_desc;
					decoder->format = format_code;
					decoder->frames_count = num_frames;
					decoder->data_offset = data_offset;
					decoder->data_size = data_size;
					decoder->Retain();
					return decoder;
				} catch (...) { try { source->Seek(0, Streaming::Begin); } catch (...) {} return 0; }
			}
			virtual IAudioEncoderStream * Encode(Streaming::Stream * dest, const string & format, const StreamDesc & desc) noexcept override
			{
				try {
					IAudioEncoderStream * result = 0;
					if (format == AudioFormatWaveform) result = new WaveEncoderStream(this, dest, desc, false);
					else if (format == AudioFormatEngineRaw) result = new WaveEncoderStream(this, dest, desc, true);
					return result;
				} catch (...) { return 0; }
			}
		};

		SafePointer<IAudioCodec> _engine_wave_codec;
		IAudioCodec * InitializeWaveCodec(void)
		{
			if (!_engine_wave_codec) {
				_engine_wave_codec = new WaveCodec;
				RegisterCodec(_engine_wave_codec);
			}
			return _engine_wave_codec;
		}
	}
}