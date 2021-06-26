#pragma once

#include "Media.h"
#include "../Interfaces/SystemWindows.h"

namespace Engine
{
	namespace Video
	{
		constexpr const widechar * VideoFormatRGB = L"RGB";
		constexpr const widechar * VideoFormatH264 = L"H264";

		enum class VideoObjectType { Unknown, Device, Frame, Encoder, Decoder, StreamEncoder, StreamDecoder };

		struct VideoObjectDesc
		{
			uint Width;
			uint Height;
			uint FramePresentation;
			uint FrameDuration;
			uint TimeScale;
			Graphics::IDevice * Device;

			VideoObjectDesc(void);
			VideoObjectDesc(uint width, uint height, uint presentation, uint duration, uint scale, Graphics::IDevice * device = 0);
			operator string(void) const;
		};

		class IVideoFrame;
		class IVideoCodec;
		class IVideoSession;
		class IVideoDecoder;
		class IVideoEncoder;
		class IVideoFactory;
		class IVideoDevice;

		class IVideoObject : public Object
		{
		public:
			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept = 0;
			virtual VideoObjectType GetObjectType(void) const noexcept = 0;
			virtual handle GetBufferFormat(void) const noexcept = 0;
		};
		class IVideoFrame : public IVideoObject
		{
		public:
			virtual void SetFramePresentation(uint duration) noexcept = 0;
			virtual void SetFrameDuration(uint duration) noexcept = 0;
			virtual void SetTimeScale(uint scale) noexcept = 0;
			virtual Codec::Frame * QueryFrame(void) const noexcept = 0;
		};
		class IVideoFrameBlt : public Object
		{
		public:
			virtual bool SetInputFormat(const IVideoObject * format_provider) noexcept = 0;
			virtual bool SetInputFormat(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept = 0;
			virtual bool SetOutputFormat(const IVideoObject * format_provider) noexcept = 0;
			virtual bool SetOutputFormat(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept = 0;
			virtual bool Reset(void) noexcept = 0;
			virtual bool IsInitialized(void) const noexcept = 0;
			virtual bool Process(IVideoFrame * dest, Graphics::ITexture * from, Graphics::SubresourceIndex subres) noexcept = 0;
			virtual bool Process(Graphics::ITexture * dest, const IVideoFrame * from, Graphics::SubresourceIndex subres) noexcept = 0;
		};

		class IVideoSession : public IVideoObject
		{
		public:
			virtual IVideoCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
			virtual bool Reset(void) noexcept = 0;
			virtual bool IsOutputAvailable(void) const noexcept = 0;
		};
		class IVideoDecoder : public IVideoSession
		{
		public:
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept = 0;
			virtual bool ReadFrame(IVideoFrame ** frame) noexcept = 0;
		};
		class IVideoEncoder : public IVideoSession
		{
		public:
			virtual const Media::VideoTrackFormatDesc & GetEncodedDescriptor(void) const noexcept = 0;
			virtual const DataBlock * GetCodecMagic(void) noexcept = 0;
			virtual bool SupplyFrame(const IVideoFrame * frame, bool encode_keyframe = false) noexcept = 0;
			virtual bool SupplyEndOfStream(void) noexcept = 0;
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept = 0;
		};
		class IVideoCodec : public Object
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept = 0;
			virtual bool CanDecode(const string & format) const noexcept = 0;
			virtual Array<string> * GetFormatsCanEncode(void) const = 0;
			virtual Array<string> * GetFormatsCanDecode(void) const = 0;
			virtual string GetCodecName(void) const = 0;
			virtual IVideoDecoder * CreateDecoder(const Media::TrackFormatDesc & format, Graphics::IDevice * acceleration_device = 0) noexcept = 0;
			virtual IVideoEncoder * CreateEncoder(const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options) noexcept = 0;
		};

		class IVideoStream : public IVideoObject
		{
		public:
			virtual IVideoCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
		};
		class VideoDecoderStream : public IVideoStream
		{
			SafePointer<Media::IMediaContainerSource> _container;
			SafePointer<Media::IMediaTrackSource> _track;
			SafePointer<IVideoDecoder> _decoder;
			Media::PacketBuffer _packet;
			bool _eos;
		public:
			VideoDecoderStream(Media::IMediaTrackSource * source, Graphics::IDevice * device = 0);
			VideoDecoderStream(Media::IMediaContainerSource * source, Graphics::IDevice * device = 0);
			VideoDecoderStream(Streaming::Stream * source, Graphics::IDevice * device = 0);
			virtual ~VideoDecoderStream(void) override;

			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override;
			virtual VideoObjectType GetObjectType(void) const noexcept override;
			virtual handle GetBufferFormat(void) const noexcept override;
			virtual IVideoCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;

			virtual Media::IMediaTrackSource * GetSourceTrack(void) noexcept;
			virtual bool ReadFrame(IVideoFrame ** frame) noexcept;
			virtual uint64 GetTimeScale(void) const noexcept;
			virtual uint64 GetDuration(void) const noexcept;
			virtual uint64 GetCurrentTime(void) const noexcept;
			virtual uint64 SetCurrentTime(uint64 time) noexcept;
		};
		class VideoEncoderStream : public IVideoStream
		{
			SafePointer<Media::IMediaContainerSink> _container;
			SafePointer<Media::IMediaTrackSink> _track;
			SafePointer<IVideoEncoder> _encoder;

			void _init(Media::IMediaContainerSink * dest, const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options);
			bool _drain_packets(void) noexcept;
		public:
			VideoEncoderStream(Media::IMediaContainerSink * dest, const string & format, const VideoObjectDesc & desc, uint num_options = 0, const uint * options = 0);
			VideoEncoderStream(Streaming::Stream * dest, const string & media_format, const string & video_format, const VideoObjectDesc & desc, uint num_options = 0, const uint * options = 0);
			virtual ~VideoEncoderStream(void) override;

			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override;
			virtual VideoObjectType GetObjectType(void) const noexcept override;
			virtual handle GetBufferFormat(void) const noexcept override;
			virtual IVideoCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;

			virtual Media::IMediaTrackSink * GetDestinationSink(void) noexcept;
			virtual bool WriteFrame(const IVideoFrame * frame, bool encode_keyframe = false) noexcept;
			virtual bool Finalize(void) noexcept;
		};

		class IVideoDevice : public IVideoObject
		{
		public:
			virtual string GetDeviceIdentifier(void) const = 0;
			virtual Array<VideoObjectDesc> * GetSupportedFrameFormats(void) const noexcept = 0;
			virtual bool SetFrameFormat(const VideoObjectDesc & desc) noexcept = 0;
			virtual bool GetSupportedFrameRateRange(uint * min_rate_numerator, uint * min_rate_denominator, uint * max_rate_numerator, uint * max_rate_denominator) const noexcept = 0;
			virtual bool SetFrameRate(uint rate_numerator, uint rate_denominator) noexcept = 0;
			virtual bool Initialize(void) noexcept = 0;
			virtual bool StartProcessing(void) noexcept = 0;
			virtual bool PauseProcessing(void) noexcept = 0;
			virtual bool StopProcessing(void) noexcept = 0;
			virtual bool ReadFrame(IVideoFrame ** frame) noexcept = 0;
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status) noexcept = 0;
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status, IDispatchTask * execute_on_processed) noexcept = 0;
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status, Semaphore * open_on_processed) noexcept = 0;
		};
		class IVideoFactory : public Object
		{
		public:
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableDevices(void) noexcept = 0;
			virtual IVideoDevice * CreateDevice(const string & identifier, Graphics::IDevice * acceleration_device = 0) noexcept = 0;
			virtual IVideoDevice * CreateDefaultDevice(Graphics::IDevice * acceleration_device = 0) noexcept = 0;
			virtual IVideoDevice * CreateScreenCaptureDevice(Windows::IScreen * screen, Graphics::IDevice * acceleration_device = 0) noexcept = 0;
			virtual IVideoFrame * CreateFrame(Codec::Frame * frame, Graphics::IDevice * acceleration_device = 0) noexcept = 0;
			virtual IVideoFrame * CreateFrame(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept = 0;
			virtual IVideoFrameBlt * CreateFrameBlt(void) noexcept = 0;
		};
		
		void InitializeDefaultCodecs(void);
		void RegisterCodec(IVideoCodec * codec);
		void UnregisterCodec(IVideoCodec * codec);

		Array<string> * GetEncodeFormats(void);
		Array<string> * GetDecodeFormats(void);
		IVideoCodec * FindEncoder(const string & format);
		IVideoCodec * FindDecoder(const string & format);
		IVideoDecoder * CreateDecoder(const Media::TrackFormatDesc & format, Graphics::IDevice * acceleration_device = 0);
		IVideoEncoder * CreateEncoder(const string & format, const VideoObjectDesc & desc, uint num_options = 0, const uint * options = 0);

		IVideoFactory * CreateVideoFactory(void);
	}
}