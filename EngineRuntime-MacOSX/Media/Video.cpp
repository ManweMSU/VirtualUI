#include "Video.h"

#include "../Miscellaneous/DynamicString.h"
#include "../Interfaces/SystemVideo.h"

namespace Engine
{
	namespace Video
	{
		VideoObjectDesc::VideoObjectDesc(void) {}
		VideoObjectDesc::VideoObjectDesc(uint width, uint height, uint presentation, uint duration, uint scale, Graphics::IDevice * device) :
			Width(width), Height(height), FramePresentation(presentation), FrameDuration(duration), TimeScale(scale), Device(device) {}
		VideoObjectDesc::operator string(void) const
		{
			return FormatString(L"Video descriptor: resolution: %0 x %1, frame rate: %2/%3, device attached: %4",
				Width, Height, FrameDuration, TimeScale, Device ? L"yes" : L"no");
		}
		
		ObjectArray<IVideoCodec> _video_codecs(0x10);

		void InitializeDefaultCodecs(void) { InitializeSystemCodec(); }
		void RegisterCodec(IVideoCodec * codec) { _video_codecs.Append(codec); }
		void UnregisterCodec(IVideoCodec * codec) { for (int i = 0; i < _video_codecs.Length(); i++) if (_video_codecs.ElementAt(i) == codec) { _video_codecs.Remove(i); break; } }

		Array<string> * GetEncodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _video_codecs) {
				SafePointer< Array<string> > formats = codec.GetFormatsCanEncode();
				for (auto & format : formats->Elements()) {
					bool present = false;
					for (auto & added : result->Elements()) if (added == format) { present = true; break; }
					if (!present) result->Append(format);
				}
			}
			result->Retain();
			return result;
		}
		Array<string> * GetDecodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _video_codecs) {
				SafePointer< Array<string> > formats = codec.GetFormatsCanDecode();
				for (auto & format : formats->Elements()) {
					bool present = false;
					for (auto & added : result->Elements()) if (added == format) { present = true; break; }
					if (!present) result->Append(format);
				}
			}
			result->Retain();
			return result;
		}
		IVideoCodec * FindEncoder(const string & format) { for (auto & codec : _video_codecs) if (codec.CanEncode(format)) return &codec; return 0; }
		IVideoCodec * FindDecoder(const string & format) { for (auto & codec : _video_codecs) if (codec.CanDecode(format)) return &codec; return 0; }
		IVideoDecoder * CreateDecoder(const Media::TrackFormatDesc & format, Graphics::IDevice * acceleration_device)
		{
			if (format.GetTrackClass() != Media::TrackClass::Video) return 0;
			auto codec = FindDecoder(format.GetTrackCodec());
			if (!codec) return 0;
			auto decoder = codec->CreateDecoder(format, acceleration_device);
			return decoder;
		}
		IVideoEncoder * CreateEncoder(const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options)
		{
			auto codec = FindEncoder(format);
			if (!codec) return 0;
			auto encoder = codec->CreateEncoder(format, desc, num_options, options);
			return encoder;
		}

		IVideoFactory * CreateVideoFactory(void) { return CreateSystemVideoFactory(); }

		VideoDecoderStream::VideoDecoderStream(Media::IMediaTrackSource * source, Graphics::IDevice * device) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_track.SetRetain(source);
			_container.SetRetain(static_cast<Media::IMediaContainerSource *>(_track->GetParentContainer()));
			_decoder = CreateDecoder(source->GetFormatDescriptor(), device);
			if (!_decoder) throw InvalidFormatException();
		}
		VideoDecoderStream::VideoDecoderStream(Media::IMediaContainerSource * source, Graphics::IDevice * device) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container.SetRetain(source);
			for (int i = 0; i < source->GetTrackCount(); i++) if (source->GetTrack(i)->GetTrackClass() == Media::TrackClass::Video) {
				_track = source->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor(), device);
			if (!_decoder) throw InvalidFormatException();
		}
		VideoDecoderStream::VideoDecoderStream(Streaming::Stream * source, Graphics::IDevice * device) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container = Media::OpenContainer(source);
			for (int i = 0; i < _container->GetTrackCount(); i++) if (_container->GetTrack(i)->GetTrackClass() == Media::TrackClass::Video) {
				_track = _container->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor(), device);
			if (!_decoder) throw InvalidFormatException();
		}
		VideoDecoderStream::~VideoDecoderStream(void) {}
		const VideoObjectDesc & VideoDecoderStream::GetObjectDescriptor(void) const noexcept { return _decoder->GetObjectDescriptor(); }
		VideoObjectType VideoDecoderStream::GetObjectType(void) const noexcept { return VideoObjectType::StreamDecoder; }
		handle VideoDecoderStream::GetBufferFormat(void) const noexcept { return _decoder->GetBufferFormat(); }
		IVideoCodec * VideoDecoderStream::GetParentCodec(void) const { return _decoder->GetParentCodec(); }
		string VideoDecoderStream::GetEncodedFormat(void) const { return _decoder->GetEncodedFormat(); }
		Media::IMediaTrackSource * VideoDecoderStream::GetSourceTrack(void) noexcept { return _track; }
		bool VideoDecoderStream::ReadFrame(IVideoFrame ** frame) noexcept
		{
			if (!frame) return false;
			while (!_decoder->IsOutputAvailable() && !_eos) {
				if (!_track->ReadPacket(_packet)) return false;
				if (!_decoder->SupplyPacket(_packet)) return false;
				if (!_packet.PacketDataActuallyUsed) _eos = true;
			}
			if (_decoder->IsOutputAvailable()) {
				if (!_decoder->ReadFrame(frame)) return false;
			} else *frame = 0;
			return true;
		}
		uint64 VideoDecoderStream::GetTimeScale(void) const noexcept { return _track->GetTimeScale(); }
		uint64 VideoDecoderStream::GetDuration(void) const noexcept { return _track->GetDuration(); }
		uint64 VideoDecoderStream::GetCurrentTime(void) const noexcept { return _track->GetPosition(); }
		uint64 VideoDecoderStream::SetCurrentTime(uint64 time) noexcept
		{
			_decoder->Reset();
			auto result = _track->Seek(time);
			_eos = false;
			return result;
		}
		
		void VideoEncoderStream::_init(Media::IMediaContainerSink * dest, const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options)
		{
			if (!dest) throw InvalidArgumentException();
			_container.SetRetain(dest);
			_encoder = CreateEncoder(format, desc, num_options, options);
			if (!_encoder) throw InvalidFormatException();
			_track = _container->CreateTrack(_encoder->GetEncodedDescriptor());
			if (!_track) throw InvalidFormatException();
		}
		bool VideoEncoderStream::_drain_packets(void) noexcept
		{
			while (_encoder->IsOutputAvailable()) {
				Media::PacketBuffer packet;
				if (!_encoder->ReadPacket(packet)) return false;
				if (!_track->WritePacket(packet)) return false;
			}
			return true;
		}
		VideoEncoderStream::VideoEncoderStream(Media::IMediaContainerSink * dest, const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options)
		{
			_init(dest, format, desc, num_options, options);
		}
		VideoEncoderStream::VideoEncoderStream(Streaming::Stream * dest, const string & media_format, const string & video_format, const VideoObjectDesc & desc, uint num_options, const uint * options)
		{
			SafePointer<Media::IMediaContainerSink> sink = Media::CreateContainer(dest, media_format);
			sink->SetAutofinalize(true);
			_init(sink, video_format, desc, num_options, options);
		}
		VideoEncoderStream::~VideoEncoderStream(void) {}
		const VideoObjectDesc & VideoEncoderStream::GetObjectDescriptor(void) const noexcept { return _encoder->GetObjectDescriptor(); }
		VideoObjectType VideoEncoderStream::GetObjectType(void) const noexcept { return VideoObjectType::StreamEncoder; }
		handle VideoEncoderStream::GetBufferFormat(void) const noexcept { return _encoder->GetBufferFormat(); }
		IVideoCodec * VideoEncoderStream::GetParentCodec(void) const { return _encoder->GetParentCodec(); }
		string VideoEncoderStream::GetEncodedFormat(void) const { return _encoder->GetEncodedFormat(); }
		Media::IMediaTrackSink * VideoEncoderStream::GetDestinationSink(void) noexcept { return _track; }
		bool VideoEncoderStream::WriteFrame(const IVideoFrame * frame, bool encode_keyframe) noexcept
		{
			if (!frame) return false;
			if (!_encoder->SupplyFrame(frame, encode_keyframe)) return false;
			return _drain_packets();
		}
		bool VideoEncoderStream::Finalize(void) noexcept
		{
			if (!_encoder->SupplyEndOfStream()) return false;
			if (!_drain_packets()) return false;
			if (!_track->UpdateCodecMagic(_encoder->GetCodecMagic())) return false;
			return _track->Finalize();
		}
	}
}