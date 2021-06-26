#include "EngineMedia.h"

namespace Engine
{
	namespace Media
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(emc_header)
				uint64 signature;  // "emedia\0\0"
				uint32 version;    // 0
				uint32 reserved;   // 0
				uint64 media_data_offset;
				uint64 media_data_size;
				uint64 track_table_offset;
				uint64 track_table_size;
				uint64 metadata_table_offset;
				uint64 metadata_table_size;
				uint64 reserved_1; // 0
				uint64 reserved_2; // 0
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(emc_packet_header)
				uint64 offset;
				uint32 size;
				uint32 flags;
				uint64 decode_time;
				uint64 present_time;
				uint64 duration;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(emc_track_header)
				uint32 contents;
				uint32 flags;
				uint32 name_offset;
				uint32 name_size;
				uint32 language;
				uint32 group;
				uint64 time_scale;
				uint64 duration;
				uint64 data_size;
				uint64 packet_encoder;
				union {
					struct {
						uint16 sample_format;
						uint16 bits_per_sample;
						uint32 num_channels;
						uint32 channel_layout;
					} audio_info;
					struct {
						uint32 width;
						uint32 height;
						uint32 frame_duration;
					} video_info;
					struct {
						uint32 flags;
						uint32 unused_1; // 0
						uint32 unused_2; // 0
					} subtitle_info;
				};
				uint32 reserved_1; // 0
				uint32 reserved_2; // 0
				uint32 codec_magic_offset;
				uint32 codec_magic_size;
				uint32 packet_table_offset;
				uint32 packet_count;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(emc_track_table)
				uint32 num_tracks;
				emc_track_header tracks[1];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(emc_metadata_header)
				uint32 field_type;
				uint32 field_offset;
				uint32 field_size;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(emc_metadata_table)
				uint32 num_fields;
				emc_metadata_header fields[1];
			ENGINE_END_PACKED_STRUCTURE
		}

		class EngineMediaCodec : public IMediaContainerCodec
		{
			ContainerClassDesc _emc;
		public:
			EngineMediaCodec(void)
			{
				_emc.ContainerFormatIdentifier = ContainerFormatEngine;
				_emc.MaximalTrackCount = 0;
				_emc.MetadataFormatIdentifier = MetadataFormatEngine;
				_emc.FormatCapabilities = ContainerClassCapabilityHoldAudio | ContainerClassCapabilityHoldVideo | ContainerClassCapabilityHoldSubtitles |
					ContainerClassCapabilityHoldMetadata | ContainerClassCapabilityInterleavedTracks | ContainerClassCapabilityKeyFrames;
			}
			virtual ~EngineMediaCodec(void) override {}
			virtual bool CanEncode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatEngine) {
					if (desc) *desc = _emc;
					return true;
				} else return false;
			}
			virtual bool CanDecode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatEngine) {
					if (desc) *desc = _emc;
					return true;
				} else return false;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(1);
				result->Append(_emc);
				result->Retain();
				return result;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(1);
				result->Append(_emc);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine Media Multiplexor"; }
			virtual IMediaContainerSource * OpenContainer(Streaming::Stream * source) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IMediaContainerSink * CreateContainer(Streaming::Stream * dest, const string & format) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
		};

		SafePointer<IMediaContainerCodec> _engine_media_codec;
		IMediaContainerCodec * InitializeEngineMediaCodec(void)
		{
			if (!_engine_media_codec) {
				_engine_media_codec = new EngineMediaCodec;
				RegisterCodec(_engine_media_codec);
			}
			return _engine_media_codec;
		}
	}
}