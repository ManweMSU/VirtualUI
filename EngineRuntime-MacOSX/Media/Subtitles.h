#pragma once

#include "Media.h"

namespace Engine
{
	namespace Subtitles
	{
		constexpr const widechar * SubtitleFormat3GPPTimedText = L"3GTX";

		enum SubtitleFlags : uint {
			SubtitleFlagNormal = 0x0000,
			SubtitleFlagSampleForced = 0x0001,
			SubtitleFlagAllForced = 0x0002,
			SubtitleFlagAllowWrap = 0x0004,
		};

		struct SubtitleDesc
		{
			uint TimeScale;
			uint Flags;

			SubtitleDesc(void);
			SubtitleDesc(uint time_scale, uint flags);
			operator string(void) const;
		};
		struct SubtitleSample
		{
			string Text;
			uint Flags;
			uint TimeScale;
			uint TimePresent;
			uint Duration;
		};

		class ISubtitleSession;
		class ISubtitleDecoder;
		class ISubtitleEncoder;
		class ISubtitleCodec;

		class ISubtitleSession : public Object
		{
		public:
			virtual const SubtitleDesc & GetObjectDescriptor(void) const noexcept = 0;
			virtual ISubtitleCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
			virtual bool Reset(void) noexcept = 0;
			virtual int GetPendingPacketsCount(void) const noexcept = 0;
			virtual int GetPendingSamplesCount(void) const noexcept = 0;
		};
		class ISubtitleDecoder : public ISubtitleSession
		{
		public:
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept = 0;
			virtual bool ReadSample(SubtitleSample & sample) noexcept = 0;
		};
		class ISubtitleEncoder : public ISubtitleSession
		{
		public:
			virtual const Media::SubtitleTrackFormatDesc & GetEncodedDescriptor(void) const noexcept = 0;
			virtual const DataBlock * GetCodecMagic(void) noexcept = 0;
			virtual bool SupplySample(const SubtitleSample & sample) noexcept = 0;
			virtual bool SupplyEndOfStream(void) noexcept = 0;
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept = 0;
		};
		class ISubtitleCodec : public Object
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept = 0;
			virtual bool CanDecode(const string & format) const noexcept = 0;
			virtual Array<string> * GetFormatsCanEncode(void) const = 0;
			virtual Array<string> * GetFormatsCanDecode(void) const = 0;
			virtual string GetCodecName(void) const = 0;
			virtual ISubtitleDecoder * CreateDecoder(const Media::TrackFormatDesc & format) noexcept = 0;
			virtual ISubtitleEncoder * CreateEncoder(const string & format, const SubtitleDesc & desc) noexcept = 0;
		};

		class ISubtitleStream : public Object
		{
		public:
			virtual const SubtitleDesc & GetObjectDescriptor(void) const noexcept = 0;
			virtual ISubtitleCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
		};
		class SubtitleDecoderStream : public ISubtitleStream
		{
			SafePointer<Media::IMediaContainerSource> _container;
			SafePointer<Media::IMediaTrackSource> _track;
			SafePointer<ISubtitleDecoder> _decoder;
			Media::PacketBuffer _packet;
			bool _eos;
		public:
			SubtitleDecoderStream(Media::IMediaTrackSource * source);
			SubtitleDecoderStream(Media::IMediaContainerSource * source);
			SubtitleDecoderStream(Streaming::Stream * source);
			virtual ~SubtitleDecoderStream(void) override;

			virtual const SubtitleDesc & GetObjectDescriptor(void) const noexcept override;
			virtual ISubtitleCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;

			virtual Media::IMediaTrackSource * GetSourceTrack(void) noexcept;
			virtual bool ReadSample(SubtitleSample & sample) noexcept;
			virtual uint64 GetDuration(void) const noexcept;
			virtual uint64 GetCurrentTime(void) const noexcept;
			virtual uint64 SetCurrentTime(uint64 time) noexcept;
		};
		class SubtitleEncoderStream : public ISubtitleStream
		{
			SafePointer<Media::IMediaContainerSink> _container;
			SafePointer<Media::IMediaTrackSink> _track;
			SafePointer<ISubtitleEncoder> _encoder;

			void _init(Media::IMediaContainerSink * dest, const string & format, const SubtitleDesc & desc);
			bool _drain_packets(void) noexcept;
		public:
			SubtitleEncoderStream(Media::IMediaContainerSink * dest, const string & format, const SubtitleDesc & desc);
			SubtitleEncoderStream(Streaming::Stream * dest, const string & media_format, const string & subtitle_format, const SubtitleDesc & desc);
			virtual ~SubtitleEncoderStream(void) override;

			virtual const SubtitleDesc & GetObjectDescriptor(void) const noexcept override;
			virtual ISubtitleCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;

			virtual Media::IMediaTrackSink * GetDestinationSink(void) noexcept;
			virtual bool WriteFrames(const SubtitleSample & sample) noexcept;
			virtual bool Finalize(void) noexcept;
		};

		void InitializeDefaultCodecs(void);
		void RegisterCodec(ISubtitleCodec * codec);
		void UnregisterCodec(ISubtitleCodec * codec);

		Array<string> * GetEncodeFormats(void);
		Array<string> * GetDecodeFormats(void);
		ISubtitleCodec * FindEncoder(const string & format);
		ISubtitleCodec * FindDecoder(const string & format);
		ISubtitleDecoder * CreateDecoder(const Media::TrackFormatDesc & format);
		ISubtitleEncoder * CreateEncoder(const string & format, const SubtitleDesc & desc);
	}
}