#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Audio
	{
		enum class SampleFormat : uint {
			Invalid = 0,
			S8_unorm = 0x00030008, S16_unorm = 0x00030010, S24_unorm = 0x00030018, S32_unorm = 0x00030020,
			S8_snorm = 0x00010008, S16_snorm = 0x00010010, S24_snorm = 0x00010018, S32_snorm = 0x00010020,
			S32_float = 0x00020020, S64_float = 0x00020040
		};
		struct StreamDesc
		{
			SampleFormat Format;
			uint ChannelCount;
			uint FramesPerSecond;

			StreamDesc(void);
			StreamDesc(SampleFormat format, uint num_channels, uint frames_per_second);
			operator string(void) const;
		};

		bool IsFloatingPointFormat(SampleFormat format);
		bool IsSignedFormat(SampleFormat format);
		bool IsUnsignedFormat(SampleFormat format);
		uint SampleFormatBitSize(SampleFormat format);
		uint SampleFormatByteSize(SampleFormat format);
		uint StreamFrameByteSize(const StreamDesc & format);
		uint64 ConvertFrameCount(uint64 frame_count, uint old_frame_rate, uint new_frame_rate);

		class WaveBuffer : public Object
		{
			StreamDesc desc;
			uint64 frames_allocated;
			uint64 frames_used;
			uint8 * frame_buffer;
		public:
			WaveBuffer(const WaveBuffer & src);
			WaveBuffer(const WaveBuffer * src);
			WaveBuffer(SampleFormat format, uint num_channels, uint frames_per_second, uint64 size_frames);
			WaveBuffer(const StreamDesc & desc, uint64 size_frames);
			virtual ~WaveBuffer(void) override;

			const StreamDesc & GetFormatDescriptor(void) const;
			uint64 GetSizeInFrames(void) const;
			uint64 & FramesUsed(void);
			uint64 FramesUsed(void) const;

			uint8 * GetData(void);
			const uint8 * GetData(void) const;
			uint64 GetAllocatedSizeInBytes(void) const;
			uint64 GetUsedSizeInBytes(void) const;
			void ReinterpretFrames(uint frames_per_second);

			int32 ReadSampleInteger(uint64 frame_index, uint num_channel) const;
			double ReadSampleFloat(uint64 frame_index, uint num_channel) const;
			void WriteSample(uint64 frame_index, uint num_channel, int32 value);
			void WriteSample(uint64 frame_index, uint num_channel, double value);

			WaveBuffer * ConvertFormat(SampleFormat format) const;
			WaveBuffer * ConvertFrameRate(uint32 frames_per_second) const;
			WaveBuffer * RemuxChannels(uint num_channels, const double * remux_matrix) const;
			WaveBuffer * ReorderChannels(uint num_channels, const uint * indicies) const;
			WaveBuffer * ReallocateChannels(uint num_channels) const;

			void ConvertFormat(WaveBuffer * buffer, SampleFormat format) const;
			void ConvertFrameRate(WaveBuffer * buffer, uint32 frames_per_second) const;
			void RemuxChannels(WaveBuffer * buffer, uint num_channels, const double * remux_matrix) const;
			void ReorderChannels(WaveBuffer * buffer, uint num_channels, const uint * indicies) const;
			void ReallocateChannels(WaveBuffer * buffer, uint num_channels) const;
		};
	}
}