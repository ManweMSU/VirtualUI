#include "Audio.h"

#include "../Math/Vector.h"

#include "../Interfaces/SystemAudio.h"
#include "WaveCodec.h"

namespace Engine
{
	namespace Audio
	{
		bool IsFloatingPointFormat(SampleFormat format) { return (reinterpret_cast<uint &>(format) & 0xFFFF0000) == 0x00020000; }
		uint SampleFormatBitSize(SampleFormat format) { return reinterpret_cast<uint &>(format) & 0xFFFF; }
		uint SampleFormatByteSize(SampleFormat format) { return SampleFormatBitSize(format) / 8; }
		uint StreamFrameByteSize(const StreamDesc & format) { return SampleFormatByteSize(format.Format) * format.ChannelCount; }
		uint64 ConvertFrameCount(uint64 frame_count, uint old_frame_rate, uint new_frame_rate)
		{
			double ratio = double(old_frame_rate) / double(new_frame_rate);
			uint64 new_frames = uint64(double(frame_count) / ratio);
			if (!new_frames) new_frames = 1;
			return new_frames;
		}
		StreamDesc::StreamDesc(void) {}
		StreamDesc::StreamDesc(SampleFormat format, uint num_channels, uint frames_per_second) : Format(format), ChannelCount(num_channels), FramesPerSecond(frames_per_second) {}
		StreamDesc::operator string(void) const
		{
			string fmt, ch, fps;
			if (Format != SampleFormat::Invalid) {
				fmt = string(SampleFormatBitSize(Format)) + string(L"-bit ") + (IsFloatingPointFormat(Format) ? L"floating point" : L"normalized integer");
			} else fmt = L"unspecified";
			if (ChannelCount) ch = string(ChannelCount); else ch = L"unspecified";
			if (FramesPerSecond) fps = string(FramesPerSecond); else fps = L"unspecified";
			return L"Stream descriptor: sample format: " + fmt + L", channels: " + ch + L", frames per second: " + fps;
		}

		WaveBuffer::WaveBuffer(const WaveBuffer & src)
		{
			desc = src.desc;
			frames_allocated = src.frames_allocated;
			frames_used = src.frames_used;
			intptr size = intptr(frames_allocated) * StreamFrameByteSize(desc);
			frame_buffer = reinterpret_cast<uint8 *>(malloc(size));
			if (!frame_buffer) throw OutOfMemoryException();
			MemoryCopy(frame_buffer, src.frame_buffer, size);
		}
		WaveBuffer::WaveBuffer(const WaveBuffer * src) : WaveBuffer(*src) {}
		WaveBuffer::WaveBuffer(SampleFormat format, uint num_channels, uint frames_per_second, uint64 size_frames)
		{
			if (format == SampleFormat::Invalid || !num_channels || !frames_per_second || !size_frames) throw InvalidArgumentException();
			desc.Format = format;
			desc.ChannelCount = num_channels;
			desc.FramesPerSecond = frames_per_second;
			frames_allocated = intptr(size_frames);
			frames_used = 0;
			intptr size = intptr(frames_allocated) * StreamFrameByteSize(desc);
			frame_buffer = reinterpret_cast<uint8 *>(malloc(size));
			if (!frame_buffer) throw OutOfMemoryException();
		}
		WaveBuffer::WaveBuffer(const StreamDesc & _desc, uint64 size_frames)
		{
			if (_desc.Format == SampleFormat::Invalid || !_desc.ChannelCount || !_desc.FramesPerSecond || !size_frames) throw InvalidArgumentException();
			desc = _desc;
			frames_allocated = size_frames;
			frames_used = 0;
			intptr size = intptr(frames_allocated) * StreamFrameByteSize(desc);
			frame_buffer = reinterpret_cast<uint8 *>(malloc(size));
			if (!frame_buffer) throw OutOfMemoryException();
		}
		WaveBuffer::~WaveBuffer(void) { free(frame_buffer); }
		const StreamDesc & WaveBuffer::GetFormatDescriptor(void) const { return desc; }
		uint64 WaveBuffer::GetSizeInFrames(void) const { return frames_allocated; }
		uint64 & WaveBuffer::FramesUsed(void) { return frames_used; }
		uint64 WaveBuffer::FramesUsed(void) const { return frames_used; }
		uint8 * WaveBuffer::GetData(void) { return frame_buffer; }
		const uint8 * WaveBuffer::GetData(void) const { return frame_buffer; }
		uint64 WaveBuffer::GetAllocatedSizeInBytes(void) const { return StreamFrameByteSize(desc) * frames_allocated; }
		uint64 WaveBuffer::GetUsedSizeInBytes(void) const { return StreamFrameByteSize(desc) * frames_used; }
		void WaveBuffer::ReinterpretFrames(uint frames_per_second) { if (!frames_per_second) throw InvalidArgumentException(); desc.FramesPerSecond = frames_per_second; }
		int32 WaveBuffer::ReadSampleInteger(uint64 frame_index, uint num_channel) const
		{
			intptr sample_index = intptr((frame_index * desc.ChannelCount + num_channel) * SampleFormatByteSize(desc.Format));
			if (desc.Format == SampleFormat::S8_snorm) {
				uint32 result = uint32(*reinterpret_cast<uint8 *>(frame_buffer + sample_index)) << 24;
				result |= (result & 0x7F000000) >> 7;
				result |= (result & 0x7FFE0000) >> 14;
				result |= (result & 0x7FFFFFF8) >> 28;
				return result;
			} else if (desc.Format == SampleFormat::S16_snorm) {
				uint32 result = uint32(*reinterpret_cast<uint16 *>(frame_buffer + sample_index)) << 16;
				result |= (result & 0x7FFF0000) >> 15;
				result |= (result & 0x7FFFFFFE) >> 30;
				return result;
			} else if (desc.Format == SampleFormat::S24_snorm) {
				uint32 result = (uint32(*reinterpret_cast<uint16 *>(frame_buffer + sample_index)) << 8) | (uint32(*reinterpret_cast<uint8 *>(frame_buffer + sample_index + 2)) << 24);
				result |= (result & 0x7FFFFF00) >> 23;
				return result;
			} else if (desc.Format == SampleFormat::S32_snorm) {
				int32 result = *reinterpret_cast<int32 *>(frame_buffer + sample_index);
				return result;
			} else if (desc.Format == SampleFormat::S32_float) {
				int32 result = int32(*reinterpret_cast<float *>(frame_buffer + sample_index) * 2147483648.0f);
				return result;
			} else if (desc.Format == SampleFormat::S64_float) {
				int32 result = int32(*reinterpret_cast<double *>(frame_buffer + sample_index) * 2147483648.0);
				return result;
			} else return 0;
		}
		double WaveBuffer::ReadSampleFloat(uint64 frame_index, uint num_channel) const
		{
			intptr sample_index = intptr((frame_index * desc.ChannelCount + num_channel) * SampleFormatByteSize(desc.Format));
			if (desc.Format == SampleFormat::S8_snorm) {
				int32 result = *reinterpret_cast<int8 *>(frame_buffer + sample_index);
				return double(result) / 128.0;
			} else if (desc.Format == SampleFormat::S16_snorm) {
				int32 result = *reinterpret_cast<int16 *>(frame_buffer + sample_index);
				return double(result) / 32768.0;
			} else if (desc.Format == SampleFormat::S24_snorm) {
				int32 result = (uint32(*reinterpret_cast<uint16 *>(frame_buffer + sample_index)) << 8) | (uint32(*reinterpret_cast<uint8 *>(frame_buffer + sample_index + 2)) << 24);
				result |= (result & 0x7FFFFF00) >> 23;
				return double(result) / 2147483648.0;
			} else if (desc.Format == SampleFormat::S32_snorm) {
				int32 result = *reinterpret_cast<int32 *>(frame_buffer + sample_index);
				return double(result) / 2147483648.0;
			} else if (desc.Format == SampleFormat::S32_float) {
				return double(*reinterpret_cast<float *>(frame_buffer + sample_index));
			} else if (desc.Format == SampleFormat::S64_float) {
				return *reinterpret_cast<double *>(frame_buffer + sample_index);
			} else return 0;
		}
		void WaveBuffer::WriteSample(uint64 frame_index, uint num_channel, int32 value)
		{
			intptr sample_index = intptr((frame_index * desc.ChannelCount + num_channel) * SampleFormatByteSize(desc.Format));
			if (desc.Format == SampleFormat::S8_snorm) {
				*reinterpret_cast<int8 *>(frame_buffer + sample_index) = (value >> 24) & 0xFF;
			} else if (desc.Format == SampleFormat::S16_snorm) {
				*reinterpret_cast<int16 *>(frame_buffer + sample_index) = (value >> 16) & 0xFFFF;
			} else if (desc.Format == SampleFormat::S24_snorm) {
				*reinterpret_cast<int16 *>(frame_buffer + sample_index) = (value >> 8) & 0xFFFF;
				*reinterpret_cast<int8 *>(frame_buffer + sample_index + 2) = (value >> 24) & 0xFF;
			} else if (desc.Format == SampleFormat::S32_snorm) {
				*reinterpret_cast<int32 *>(frame_buffer + sample_index) = value;
			} else if (desc.Format == SampleFormat::S32_float) {
				*reinterpret_cast<float *>(frame_buffer + sample_index) = float(value) / 2147483648.0f;
			} else if (desc.Format == SampleFormat::S64_float) {
				*reinterpret_cast<double *>(frame_buffer + sample_index) = double(value) / 2147483648.0;
			}
		}
		void WaveBuffer::WriteSample(uint64 frame_index, uint num_channel, double value)
		{
			intptr sample_index = intptr((frame_index * desc.ChannelCount + num_channel) * SampleFormatByteSize(desc.Format));
			if (desc.Format == SampleFormat::S8_snorm) {
				int32 ivalue = int32(value * 2147483648.0);
				*reinterpret_cast<int8 *>(frame_buffer + sample_index) = ivalue >> 24;
			} else if (desc.Format == SampleFormat::S16_snorm) {
				int32 ivalue = int32(value * 2147483648.0);
				*reinterpret_cast<int16 *>(frame_buffer + sample_index) = ivalue >> 16;
			} else if (desc.Format == SampleFormat::S24_snorm) {
				int32 ivalue = int32(value * 2147483648.0);
				*reinterpret_cast<int16 *>(frame_buffer + sample_index) = (ivalue >> 8) & 0xFFFF;
				*reinterpret_cast<int8 *>(frame_buffer + sample_index + 2) = ivalue >> 24;
			} else if (desc.Format == SampleFormat::S32_snorm) {
				int32 ivalue = int32(value * 2147483648.0);
				*reinterpret_cast<int32 *>(frame_buffer + sample_index) = ivalue;
			} else if (desc.Format == SampleFormat::S32_float) {
				*reinterpret_cast<float *>(frame_buffer + sample_index) = float(value);
			} else if (desc.Format == SampleFormat::S64_float) {
				*reinterpret_cast<double *>(frame_buffer + sample_index) = value;
			}
		}
		WaveBuffer * WaveBuffer::ConvertFormat(SampleFormat format) const
		{
			SafePointer<WaveBuffer> buffer = new WaveBuffer(format, desc.ChannelCount, desc.FramesPerSecond, frames_allocated);
			ConvertFormat(buffer, format);
			buffer->Retain();
			return buffer;
		}
		WaveBuffer * WaveBuffer::ConvertFrameRate(uint32 frames_per_second) const
		{
			if (!frames_per_second) throw InvalidArgumentException();
			double ratio = double(desc.FramesPerSecond) / double(frames_per_second);
			uint64 new_frames = uint64(double(frames_used) / ratio);
			if (!new_frames) new_frames = 1;
			SafePointer<WaveBuffer> buffer = new WaveBuffer(desc.Format, desc.ChannelCount, frames_per_second, new_frames);
			ConvertFrameRate(buffer, frames_per_second);
			buffer->Retain();
			return buffer;
		}
		WaveBuffer * WaveBuffer::RemuxChannels(uint num_channels, const double * remux_matrix) const
		{
			SafePointer<WaveBuffer> buffer = new WaveBuffer(desc.Format, num_channels, desc.FramesPerSecond, frames_allocated);
			RemuxChannels(buffer, num_channels, remux_matrix);
			buffer->Retain();
			return buffer;
		}
		WaveBuffer * WaveBuffer::ReorderChannels(uint num_channels, const uint * indicies) const
		{
			SafePointer<WaveBuffer> buffer = new WaveBuffer(desc.Format, num_channels, desc.FramesPerSecond, frames_allocated);
			ReorderChannels(buffer, num_channels, indicies);
			buffer->Retain();
			return buffer;
		}
		WaveBuffer * WaveBuffer::ReallocateChannels(uint num_channels) const
		{
			SafePointer<WaveBuffer> buffer = new WaveBuffer(desc.Format, num_channels, desc.FramesPerSecond, frames_allocated);
			ReallocateChannels(buffer, num_channels);
			buffer->Retain();
			return buffer;
		}
		void WaveBuffer::ConvertFormat(WaveBuffer * buffer, SampleFormat format) const
		{
			if (buffer->desc.Format != format || buffer->desc.ChannelCount != desc.ChannelCount || buffer->desc.FramesPerSecond != desc.FramesPerSecond) throw InvalidArgumentException();
			if (buffer->frames_allocated < frames_used) throw InvalidArgumentException();
			buffer->frames_used = frames_used;
			if (IsFloatingPointFormat(format)) {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < desc.ChannelCount; j++) {
					auto value = ReadSampleFloat(i, j);
					buffer->WriteSample(i, j, value);
				}
			} else {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < desc.ChannelCount; j++) {
					auto value = ReadSampleInteger(i, j);
					buffer->WriteSample(i, j, value);
				}
			}
		}
		void WaveBuffer::ConvertFrameRate(WaveBuffer * buffer, uint32 frames_per_second) const
		{
			if (buffer->desc.Format != desc.Format || buffer->desc.ChannelCount != desc.ChannelCount || buffer->desc.FramesPerSecond != frames_per_second) throw InvalidArgumentException();
			if (!frames_per_second) throw InvalidArgumentException();
			double ratio = double(desc.FramesPerSecond) / double(frames_per_second);
			uint64 new_frames = uint64(double(frames_used) / ratio);
			if (!new_frames) new_frames = 1;
			if (buffer->frames_allocated < new_frames) throw InvalidArgumentException();
			buffer->frames_used = new_frames;
			if (ratio <= 1.0) {
				for (intptr i = 0; i < new_frames; i++) for (uint j = 0; j < desc.ChannelCount; j++) {
					double sample_coord = 0.5 + double(i);
					double source_sample_index = sample_coord * ratio - 0.5;
					uint64 left = uint64(source_sample_index);
					uint64 right = left + 1;
					double dt = source_sample_index - double(left);
					double left_sample = ReadSampleFloat(min(max(left, 0ULL), frames_used - 1), j);
					double right_sample = ReadSampleFloat(min(max(right, 0ULL), frames_used - 1), j);
					buffer->WriteSample(i, j, Math::lerp(left_sample, right_sample, dt));
				}
			} else {
				for (intptr i = 0; i < new_frames; i++) for (uint j = 0; j < desc.ChannelCount; j++) {
					double src_seg_min = double(i) * ratio;
					double src_seg_max = double(i + 1) * ratio;
					uint64 int_seg_min = min(max(uint64(src_seg_min) + 1, 0ULL), frames_used);
					uint64 int_seg_max = min(max(uint64(src_seg_max), 0ULL), frames_used);
					double weight = 0.0;
					double sum = 0.0;
					for (uint64 k = int_seg_min; k < int_seg_max; k++) {
						sum += ReadSampleFloat(k, j);
						weight += 1.0;
					}
					if (int_seg_min > 0) {
						double w = double(int_seg_min) - src_seg_min;
						sum += ReadSampleFloat(int_seg_min - 1, j) * w;
						weight += w;
					}
					if (int_seg_max < frames_used) {
						double w = src_seg_max - double(int_seg_max);
						sum += ReadSampleFloat(int_seg_max, j) * w;
						weight += w;
					}
					if (weight) buffer->WriteSample(i, j, sum / weight);
					else buffer->WriteSample(i, j, 0.0);
				}
			}
		}
		void WaveBuffer::RemuxChannels(WaveBuffer * buffer, uint num_channels, const double * remux_matrix) const
		{
			if (buffer->desc.Format != desc.Format || buffer->desc.ChannelCount != num_channels || buffer->desc.FramesPerSecond != desc.FramesPerSecond) throw InvalidArgumentException();
			if (buffer->frames_allocated < frames_used) throw InvalidArgumentException();
			buffer->frames_used = frames_used;
			for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < num_channels; j++) {
				double sum = 0.0;
				for (uint k = 0; k < desc.ChannelCount; k++) {
					auto value = ReadSampleFloat(i, k);
					sum += remux_matrix[j * desc.ChannelCount + k] * value;
				}
				buffer->WriteSample(i, j, sum);
			}
		}
		void WaveBuffer::ReorderChannels(WaveBuffer * buffer, uint num_channels, const uint * indicies) const
		{
			if (buffer->desc.Format != desc.Format || buffer->desc.ChannelCount != num_channels || buffer->desc.FramesPerSecond != desc.FramesPerSecond) throw InvalidArgumentException();
			if (buffer->frames_allocated < frames_used) throw InvalidArgumentException();
			buffer->frames_used = frames_used;
			if (IsFloatingPointFormat(desc.Format)) {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < num_channels; j++) {
					auto value = ReadSampleFloat(i, indicies[j]);
					buffer->WriteSample(i, j, value);
				}
			} else {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < num_channels; j++) {
					auto value = ReadSampleInteger(i, indicies[j]);
					buffer->WriteSample(i, j, value);
				}
			}
		}
		void WaveBuffer::ReallocateChannels(WaveBuffer * buffer, uint num_channels) const
		{
			if (buffer->desc.Format != desc.Format || buffer->desc.ChannelCount != num_channels || buffer->desc.FramesPerSecond != desc.FramesPerSecond) throw InvalidArgumentException();
			if (buffer->frames_allocated < frames_used) throw InvalidArgumentException();
			buffer->frames_used = frames_used;
			if (IsFloatingPointFormat(desc.Format)) {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < num_channels; j++) {
					auto value = (j < desc.ChannelCount) ? ReadSampleFloat(i, j) : 0.0;
					buffer->WriteSample(i, j, value);
				}
			} else {
				for (intptr i = 0; i < frames_used; i++) for (uint j = 0; j < num_channels; j++) {
					auto value = (j < desc.ChannelCount) ? ReadSampleInteger(i, j) : 0;
					buffer->WriteSample(i, j, value);
				}
			}
		}

		ObjectArray<IAudioCodec> _audio_codecs(0x10);

		void InitializeDefaultCodecs(void)
		{
			InitializeSystemCodec();
			InitializeWaveCodec();
		}
		void RegisterCodec(IAudioCodec * codec) { _audio_codecs.Append(codec); }
		void UnregisterCodec(IAudioCodec * codec) { for (int i = 0; i < _audio_codecs.Length(); i++) if (_audio_codecs.ElementAt(i) == codec) { _audio_codecs.Remove(i); break; } }
		IAudioCodec * FindEncoder(const string & format) { for (auto & codec : _audio_codecs) if (codec.CanEncode(format)) return &codec; return 0; }
		IAudioCodec * FindDecoder(const string & format) { for (auto & codec : _audio_codecs) if (codec.CanDecode(format)) return &codec; return 0; }
		IAudioDecoderStream * DecodeAudio(Streaming::Stream * stream, const StreamDesc * desired_desc)
		{
			for (auto & codec : _audio_codecs) {
				auto decoder = codec.TryDecode(stream, desired_desc);
				if (decoder) return decoder;
			}
			return 0;
		}
		IAudioEncoderStream * EncodeAudio(Streaming::Stream * stream, const string & format, const StreamDesc & desc)
		{
			auto codec = FindEncoder(format);
			if (!codec) return 0;
			auto encoder = codec->Encode(stream, format, desc);
			return encoder;
		}
		IAudioDeviceFactory * CreateAudioDeviceFactory(void) { return CreateSystemAudioDeviceFactory(); }
		void Beep(void) { SystemBeep(); }
	}
}