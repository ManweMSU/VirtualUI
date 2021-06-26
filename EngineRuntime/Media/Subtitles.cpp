#include "Subtitles.h"

#include "MPEG4.h"

namespace Engine
{
	namespace Subtitles
	{
		SubtitleDesc::SubtitleDesc(void) {}
		SubtitleDesc::SubtitleDesc(uint time_scale, uint flags) : TimeScale(time_scale), Flags(flags) {}
		SubtitleDesc::operator string(void) const { return L"Subtitle descriptor: time scale: " + string(TimeScale) + L", flags: " + string(Flags, BinaryBase, 4); }

		ObjectArray<ISubtitleCodec> _subtitle_codecs(0x10);

		void InitializeDefaultCodecs(void) { Initialize3GPPTTCodec(); }
		void RegisterCodec(ISubtitleCodec * codec) { _subtitle_codecs.Append(codec); }
		void UnregisterCodec(ISubtitleCodec * codec) { for (int i = 0; i < _subtitle_codecs.Length(); i++) if (_subtitle_codecs.ElementAt(i) == codec) { _subtitle_codecs.Remove(i); break; } }

		Array<string> * GetEncodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _subtitle_codecs) {
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
			for (auto & codec : _subtitle_codecs) {
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
		ISubtitleCodec * FindEncoder(const string & format) { for (auto & codec : _subtitle_codecs) if (codec.CanEncode(format)) return &codec; return 0; }
		ISubtitleCodec * FindDecoder(const string & format) { for (auto & codec : _subtitle_codecs) if (codec.CanDecode(format)) return &codec; return 0; }
		ISubtitleDecoder * CreateDecoder(const Media::TrackFormatDesc & format)
		{
			if (format.GetTrackClass() != Media::TrackClass::Subtitles) return 0;
			auto codec = FindDecoder(format.GetTrackCodec());
			if (!codec) return 0;
			auto decoder = codec->CreateDecoder(format);
			return decoder;
		}
		ISubtitleEncoder * CreateEncoder(const string & format, const SubtitleDesc & desc)
		{
			auto codec = FindEncoder(format);
			if (!codec) return 0;
			auto encoder = codec->CreateEncoder(format, desc);
			return encoder;
		}
		
		SubtitleDecoderStream::SubtitleDecoderStream(Media::IMediaTrackSource * source) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_track.SetRetain(source);
			_container.SetRetain(static_cast<Media::IMediaContainerSource *>(_track->GetParentContainer()));
			_decoder = CreateDecoder(source->GetFormatDescriptor());
			if (!_decoder) throw InvalidFormatException();
		}
		SubtitleDecoderStream::SubtitleDecoderStream(Media::IMediaContainerSource * source) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container.SetRetain(source);
			for (int i = 0; i < source->GetTrackCount(); i++) if (source->GetTrack(i)->GetTrackClass() == Media::TrackClass::Subtitles) {
				_track = source->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor());
			if (!_decoder) throw InvalidFormatException();
		}
		SubtitleDecoderStream::SubtitleDecoderStream(Streaming::Stream * source) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container = Media::OpenContainer(source);
			for (int i = 0; i < _container->GetTrackCount(); i++) if (_container->GetTrack(i)->GetTrackClass() == Media::TrackClass::Subtitles) {
				_track = _container->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor());
			if (!_decoder) throw InvalidFormatException();
		}
		SubtitleDecoderStream::~SubtitleDecoderStream(void) {}
		const SubtitleDesc & SubtitleDecoderStream::GetObjectDescriptor(void) const noexcept { return _decoder->GetObjectDescriptor(); }
		ISubtitleCodec * SubtitleDecoderStream::GetParentCodec(void) const { return _decoder->GetParentCodec(); }
		string SubtitleDecoderStream::GetEncodedFormat(void) const { return _decoder->GetEncodedFormat(); }
		Media::IMediaTrackSource * SubtitleDecoderStream::GetSourceTrack(void) noexcept { return _track; }
		bool SubtitleDecoderStream::ReadSample(SubtitleSample & sample) noexcept
		{
			while (!_decoder->GetPendingSamplesCount() && !_eos) {
				if (!_track->ReadPacket(_packet)) return false;
				if (!_decoder->SupplyPacket(_packet)) return false;
				if (!_packet.PacketDataActuallyUsed) _eos = true;
			}
			if (_eos) {
				sample.Text = L"";
				sample.Flags = sample.Duration = sample.TimePresent = 0;
				sample.TimeScale = GetObjectDescriptor().TimeScale;
			} else {
				if (!_decoder->ReadSample(sample)) return false;
			}
			return true;
		}
		uint64 SubtitleDecoderStream::GetDuration(void) const noexcept { return _track->GetDuration(); }
		uint64 SubtitleDecoderStream::GetCurrentTime(void) const noexcept { return _track->GetPosition(); }
		uint64 SubtitleDecoderStream::SetCurrentTime(uint64 time) noexcept
		{
			auto result = _track->Seek(time);
			if (_decoder->GetPendingSamplesCount()) _decoder->Reset();
			_eos = false;
			return result;
		}

		void SubtitleEncoderStream::_init(Media::IMediaContainerSink * dest, const string & format, const SubtitleDesc & desc)
		{
			if (!dest) throw InvalidArgumentException();
			_container.SetRetain(dest);
			_encoder = CreateEncoder(format, desc);
			if (!_encoder) throw InvalidFormatException();
			_track = _container->CreateTrack(_encoder->GetEncodedDescriptor());
			if (!_track) throw InvalidFormatException();
		}
		bool SubtitleEncoderStream::_drain_packets(void) noexcept
		{
			while (_encoder->GetPendingPacketsCount()) {
				Media::PacketBuffer packet;
				if (!_encoder->ReadPacket(packet)) return false;
				if (!_track->WritePacket(packet)) return false;
			}
			return true;
		}
		SubtitleEncoderStream::SubtitleEncoderStream(Media::IMediaContainerSink * dest, const string & format, const SubtitleDesc & desc)
		{
			_init(dest, format, desc);
		}
		SubtitleEncoderStream::SubtitleEncoderStream(Streaming::Stream * dest, const string & media_format, const string & subtitle_format, const SubtitleDesc & desc)
		{
			SafePointer<Media::IMediaContainerSink> sink = Media::CreateContainer(dest, media_format);
			sink->SetAutofinalize(true);
			_init(sink, subtitle_format, desc);
		}
		SubtitleEncoderStream::~SubtitleEncoderStream(void) {}
		const SubtitleDesc & SubtitleEncoderStream::GetObjectDescriptor(void) const noexcept { return _encoder->GetObjectDescriptor(); }
		ISubtitleCodec * SubtitleEncoderStream::GetParentCodec(void) const { return _encoder->GetParentCodec(); }
		string SubtitleEncoderStream::GetEncodedFormat(void) const { return _encoder->GetEncodedFormat(); }
		Media::IMediaTrackSink * SubtitleEncoderStream::GetDestinationSink(void) noexcept { return _track; }
		bool SubtitleEncoderStream::WriteFrames(const SubtitleSample & sample) noexcept
		{
			if (!_encoder->SupplySample(sample)) return false;
			return _drain_packets();
		}
		bool SubtitleEncoderStream::Finalize(void) noexcept
		{
			if (!_encoder->SupplyEndOfStream()) return false;
			if (!_drain_packets()) return false;
			if (!_track->UpdateCodecMagic(_encoder->GetCodecMagic())) return false;
			return _track->Finalize();
		}
	}
}