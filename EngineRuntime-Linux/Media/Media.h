#pragma once

#include "AudioBase.h"
#include "Metadata.h"
#include "MediaOptions.h"
#include "../Streaming.h"

namespace Engine
{
	namespace Media
	{
		constexpr const widechar * ContainerFormatMP3 = L"MP3";
		constexpr const widechar * ContainerFormatMPEG4 = L"MPEG4";
		constexpr const widechar * ContainerFormatFreeLossless = L"FLAC";
		constexpr const widechar * ContainerFormatRIFF_WAV = L"RIFF/WAV";

		constexpr const widechar * MetadataFormatID3 = L"ID3";
		constexpr const widechar * MetadataFormatiTunes = L"iTunes";
		constexpr const widechar * MetadataFormatVorbisComment = L"VC";

		class IMediaTrack;
		class IMediaTrackSource;
		class IMediaTrackSink;
		class IMediaContainer;
		class IMediaContainerSource;
		class IMediaContainerSink;
		class IMediaContainerCodec;

		enum class TrackClass { Unknown, Audio, Video, Subtitles };
		enum class ContainerObjectType { Source, Sink };
		enum ContainerClassCapabilities : uint {
			ContainerClassCapabilityHoldAudio = 0x0001,
			ContainerClassCapabilityHoldVideo = 0x0002,
			ContainerClassCapabilityHoldSubtitles = 0x0004,
			ContainerClassCapabilityHoldMetadata = 0x0008,
			ContainerClassCapabilityInterleavedTracks = 0x0010,
			ContainerClassCapabilityKeyFrames = 0x0020
		};
		struct ContainerClassDesc
		{
			string ContainerFormatIdentifier;
			string MetadataFormatIdentifier;
			uint FormatCapabilities;
			uint MaximalTrackCount;
			operator string(void) const;
		};
		class TrackFormatDesc : public Object
		{
			SafePointer<DataBlock> _codec_magic;
		public:
			virtual TrackClass GetTrackClass(void) const noexcept = 0;
			virtual string GetTrackCodec(void) const = 0;
			virtual TrackFormatDesc * Clone(void) const = 0;
			const DataBlock * GetCodecMagic(void) const noexcept;
			void SetCodecMagic(const DataBlock * data);
			template <class T> const T & As(void) const noexcept { return *static_cast<const T *>(this); }
		};
		class AudioTrackFormatDesc : public TrackFormatDesc
		{
			string _codec;
			Audio::StreamDesc _desc;
			uint _channel_layout;
		public:
			AudioTrackFormatDesc(const string & codec, const Audio::StreamDesc & desc, uint channel_layout = 0);
			virtual ~AudioTrackFormatDesc(void) override;
			virtual TrackClass GetTrackClass(void) const noexcept override;
			virtual string GetTrackCodec(void) const override;
			virtual TrackFormatDesc * Clone(void) const override;
			virtual string ToString(void) const override;
			const Audio::StreamDesc & GetStreamDescriptor(void) const noexcept;
			uint GetChannelLayout(void) const noexcept;
		};
		class VideoTrackFormatDesc : public TrackFormatDesc
		{
			string _codec;
			uint _width, _height, _rate_numerator, _rate_denominator;
			uint _pixel_aspect_horz, _pixel_aspect_vert;
		public:
			VideoTrackFormatDesc(const string & codec, uint width, uint height, uint frame_rate_numerator, uint frame_rate_denominator, uint pixel_aspect_horz = 1, uint pixel_aspect_vert = 1);
			virtual ~VideoTrackFormatDesc(void) override;
			virtual TrackClass GetTrackClass(void) const noexcept override;
			virtual string GetTrackCodec(void) const override;
			virtual TrackFormatDesc * Clone(void) const override;
			virtual string ToString(void) const override;
			uint GetWidth(void) const noexcept;
			uint GetHeight(void) const noexcept;
			uint GetFrameRateNumerator(void) const noexcept;
			uint GetFrameRateDenominator(void) const noexcept;
			uint GetPixelAspectHorizontal(void) const noexcept;
			uint GetPixelAspectVertical(void) const noexcept;
		};
		class SubtitleTrackFormatDesc : public TrackFormatDesc
		{
			string _codec;
			uint _time_scale;
			uint _flags;
		public:
			SubtitleTrackFormatDesc(const string & codec, uint time_scale, uint flags);
			virtual ~SubtitleTrackFormatDesc(void) override;
			virtual TrackClass GetTrackClass(void) const noexcept override;
			virtual string GetTrackCodec(void) const override;
			virtual TrackFormatDesc * Clone(void) const override;
			virtual string ToString(void) const override;
			uint GetTimeScale(void) const noexcept;
			uint GetFlags(void) const noexcept;
		};
		struct PacketBuffer
		{
			SafePointer<DataBlock> PacketData;
			int PacketDataActuallyUsed;
			bool PacketIsKey;
			uint64 PacketDecodeTime;
			uint64 PacketRenderTime;
			uint64 PacketRenderDuration;
		};
		
		class IMediaTrack : public Object
		{
		public:
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept = 0;
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept = 0;
			virtual IMediaContainer * GetParentContainer(void) const noexcept = 0;
			virtual TrackClass GetTrackClass(void) const noexcept = 0;
			virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept = 0;
			virtual string GetTrackName(void) const = 0;
			virtual string GetTrackLanguage(void) const = 0;
			virtual bool IsTrackVisible(void) const noexcept = 0;
			virtual bool IsTrackAutoselectable(void) const noexcept = 0;
			virtual int GetTrackGroup(void) const noexcept = 0;
		};
		class IMediaTrackSource : public IMediaTrack
		{
		public:
			virtual uint64 GetTimeScale(void) const noexcept = 0;
			virtual uint64 GetDuration(void) const noexcept = 0;
			virtual uint64 GetPosition(void) const noexcept = 0;
			virtual uint64 Seek(uint64 time) noexcept = 0;
			virtual uint64 GetCurrentPacket(void) const noexcept = 0;
			virtual uint64 GetPacketCount(void) const noexcept = 0;
			virtual bool ReadPacket(PacketBuffer & buffer) noexcept = 0;
		};
		class IMediaTrackSink : public IMediaTrack
		{
		public:
			virtual bool SetTrackName(const string & name) noexcept = 0;
			virtual bool SetTrackLanguage(const string & language) noexcept = 0;
			virtual void MakeTrackVisible(bool make) noexcept = 0;
			virtual void MakeTrackAutoselectable(bool make) noexcept = 0;
			virtual void SetTrackGroup(int group) noexcept = 0;
			virtual bool WritePacket(const PacketBuffer & buffer) noexcept = 0;
			virtual bool UpdateCodecMagic(const DataBlock * data) noexcept = 0;
			virtual bool Sealed(void) const noexcept = 0;
			virtual bool Finalize(void) noexcept = 0;
		};
		class IMediaContainer : public Object
		{
		public:
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept = 0; 
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept = 0;
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept = 0;
			virtual Metadata * ReadMetadata(void) const noexcept = 0;
			virtual int GetTrackCount(void) const noexcept = 0;
			virtual IMediaTrack * GetTrack(int index) const noexcept = 0;
		};
		class IMediaContainerSource : public IMediaContainer
		{
		public:
			virtual IMediaTrackSource * OpenTrack(int index) const noexcept = 0;
			virtual uint64 GetDuration(void) const noexcept = 0;
			virtual bool PatchMetadata(const Metadata * metadata) noexcept = 0;
			virtual bool PatchMetadata(const Metadata * metadata, Streaming::Stream * dest) const noexcept = 0;
		};
		class IMediaContainerSink : public IMediaContainer
		{
		public:
			virtual void WriteMetadata(const Metadata * metadata) noexcept = 0;
			virtual IMediaTrackSink * CreateTrack(const TrackFormatDesc & desc) noexcept = 0;
			virtual void SetAutofinalize(bool set) noexcept = 0;
			virtual bool IsAutofinalizable(void) const noexcept = 0;
			virtual bool Finalize(void) noexcept = 0;
		};
		class IMediaContainerCodec : public Object
		{
		public:
			virtual bool CanEncode(const string & format, ContainerClassDesc * desc = 0) const noexcept = 0;
			virtual bool CanDecode(const string & format, ContainerClassDesc * desc = 0) const noexcept = 0;
			virtual Array<ContainerClassDesc> * GetFormatsCanEncode(void) const = 0;
			virtual Array<ContainerClassDesc> * GetFormatsCanDecode(void) const = 0;
			virtual string GetCodecName(void) const = 0;
			virtual IMediaContainerSource * OpenContainer(Streaming::Stream * source) noexcept = 0;
			virtual IMediaContainerSink * CreateContainer(Streaming::Stream * dest, const string & format) noexcept = 0;
		};

		void InitializeDefaultCodecs(void);
		void RegisterCodec(IMediaContainerCodec * codec);
		void UnregisterCodec(IMediaContainerCodec * codec);

		Array<string> * GetEncodeFormats(void);
		Array<string> * GetDecodeFormats(void);
		IMediaContainerCodec * FindEncoder(const string & format);
		IMediaContainerCodec * FindDecoder(const string & format);
		IMediaContainerSource * OpenContainer(Streaming::Stream * stream);
		IMediaContainerSink * CreateContainer(Streaming::Stream * stream, const string & format);
	}
}