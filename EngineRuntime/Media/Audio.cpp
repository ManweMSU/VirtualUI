#include "Audio.h"

#include "../Math/Vector.h"

#include "../Interfaces/SystemAudio.h"
#include "WaveCodec.h"

namespace Engine
{
	namespace Audio
	{
		ObjectArray<IAudioCodec> _audio_codecs(0x10);

		void InitializeDefaultCodecs(void)
		{
			InitializeSystemCodec();
			InitializeWaveCodec();
		}
		void RegisterCodec(IAudioCodec * codec) { _audio_codecs.Append(codec); }
		void UnregisterCodec(IAudioCodec * codec) { for (int i = 0; i < _audio_codecs.Length(); i++) if (_audio_codecs.ElementAt(i) == codec) { _audio_codecs.Remove(i); break; } }
		Array<string> * GetEncodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _audio_codecs) {
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
			for (auto & codec : _audio_codecs) {
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
		IAudioCodec * FindEncoder(const string & format) { for (auto & codec : _audio_codecs) if (codec.CanEncode(format)) return &codec; return 0; }
		IAudioCodec * FindDecoder(const string & format) { for (auto & codec : _audio_codecs) if (codec.CanDecode(format)) return &codec; return 0; }
		IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc)
		{
			if (format.GetTrackClass() != Media::TrackClass::Audio) return 0;
			auto codec = FindDecoder(format.GetTrackCodec());
			if (!codec) return 0;
			auto decoder = codec->CreateDecoder(format, desired_desc);
			return decoder;
		}
		IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options, const uint * options)
		{
			auto codec = FindEncoder(format);
			if (!codec) return 0;
			auto encoder = codec->CreateEncoder(format, desc, num_options, options);
			return encoder;
		}
		IAudioDeviceFactory * CreateAudioDeviceFactory(void) { return CreateSystemAudioDeviceFactory(); }
		void Beep(void) { SystemBeep(); }

		AudioDecoderStream::AudioDecoderStream(Media::IMediaTrackSource * source, const StreamDesc * desired_desc) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_track.SetRetain(source);
			_container.SetRetain(static_cast<Media::IMediaContainerSource *>(_track->GetParentContainer()));
			_decoder = CreateDecoder(source->GetFormatDescriptor(), desired_desc);
			if (!_decoder) throw InvalidFormatException();
		}
		AudioDecoderStream::AudioDecoderStream(Media::IMediaContainerSource * source, const StreamDesc * desired_desc) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container.SetRetain(source);
			for (int i = 0; i < source->GetTrackCount(); i++) if (source->GetTrack(i)->GetTrackClass() == Media::TrackClass::Audio) {
				_track = source->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor(), desired_desc);
			if (!_decoder) throw InvalidFormatException();
		}
		AudioDecoderStream::AudioDecoderStream(Streaming::Stream * source, const StreamDesc * desired_desc) : _eos(false)
		{
			if (!source) throw InvalidArgumentException();
			_container = Media::OpenContainer(source);
			for (int i = 0; i < _container->GetTrackCount(); i++) if (_container->GetTrack(i)->GetTrackClass() == Media::TrackClass::Audio) {
				_track = _container->OpenTrack(i);
				break;
			}
			if (!_track) throw InvalidFormatException();
			_decoder = CreateDecoder(_track->GetFormatDescriptor(), desired_desc);
			if (!_decoder) throw InvalidFormatException();
		}
		AudioDecoderStream::~AudioDecoderStream(void) {}
		const StreamDesc & AudioDecoderStream::GetFormatDescriptor(void) const noexcept { return _decoder->GetFormatDescriptor(); }
		AudioObjectType AudioDecoderStream::GetObjectType(void) const noexcept { return AudioObjectType::StreamDecoder; }
		IAudioCodec * AudioDecoderStream::GetParentCodec(void) const { return _decoder->GetParentCodec(); }
		string AudioDecoderStream::GetEncodedFormat(void) const { return _decoder->GetEncodedFormat(); }
		const StreamDesc & AudioDecoderStream::GetEncodedDescriptor(void) const noexcept { return _decoder->GetEncodedDescriptor(); }
		uint AudioDecoderStream::GetChannelLayout(void) const noexcept { return _decoder->GetChannelLayout(); }
		Media::IMediaTrackSource * AudioDecoderStream::GetSourceTrack(void) noexcept { return _track; }
		bool AudioDecoderStream::ReadFrames(WaveBuffer * buffer) noexcept
		{
			if (!buffer) return false;
			auto & d1 = buffer->GetFormatDescriptor();
			auto & d2 = GetFormatDescriptor();
			if (d1.Format != d2.Format || d1.ChannelCount != d2.ChannelCount || d1.FramesPerSecond != d2.FramesPerSecond) return false;
			buffer->FramesUsed() = 0;
			while (_decoder->GetPendingFramesCount() < buffer->GetSizeInFrames() && !_eos) {
				if (!_track->ReadPacket(_packet)) return false;
				if (!_decoder->SupplyPacket(_packet)) return false;
				if (!_packet.PacketDataActuallyUsed) _eos = true;
			}
			if (!_decoder->ReadFrames(buffer)) return false;
			return true;
		}
		uint64 AudioDecoderStream::GetFramesCount(void) const noexcept { return _track->GetDuration(); }
		uint64 AudioDecoderStream::GetCurrentFrame(void) const noexcept { return _track->GetPosition(); }
		uint64 AudioDecoderStream::SetCurrentFrame(uint64 frame_index) noexcept
		{
			auto result = _track->Seek(frame_index);
			if (_decoder->GetPendingFramesCount() || _decoder->GetPendingPacketsCount()) _decoder->Reset();
			_eos = false;
			return result;
		}

		void AudioEncoderStream::_init(Media::IMediaContainerSink * dest, const string & format, const StreamDesc & desc, uint num_options, const uint * options)
		{
			if (!dest) throw InvalidArgumentException();
			_container.SetRetain(dest);
			_encoder = CreateEncoder(format, desc, num_options, options);
			if (!_encoder) throw InvalidFormatException();
			_track = _container->CreateTrack(_encoder->GetFullEncodedDescriptor());
			if (!_track) throw InvalidFormatException();
		}
		bool AudioEncoderStream::_drain_packets(void) noexcept
		{
			while (_encoder->GetPendingPacketsCount()) {
				Media::PacketBuffer packet;
				if (!_encoder->ReadPacket(packet)) return false;
				if (!_track->WritePacket(packet)) return false;
			}
			return true;
		}
		AudioEncoderStream::AudioEncoderStream(Media::IMediaContainerSink * dest, const string & format, const StreamDesc & desc, uint num_options, const uint * options)
		{
			_init(dest, format, desc, num_options, options);
		}
		AudioEncoderStream::AudioEncoderStream(Streaming::Stream * dest, const string & media_format, const string & audio_format, const StreamDesc & desc, uint num_options, const uint * options)
		{
			SafePointer<Media::IMediaContainerSink> sink = Media::CreateContainer(dest, media_format);
			sink->SetAutofinalize(true);
			_init(sink, audio_format, desc, num_options, options);
		}
		AudioEncoderStream::~AudioEncoderStream(void) {}
		const StreamDesc & AudioEncoderStream::GetFormatDescriptor(void) const noexcept { return _encoder->GetFormatDescriptor(); }
		AudioObjectType AudioEncoderStream::GetObjectType(void) const noexcept { return AudioObjectType::StreamEncoder; }
		IAudioCodec * AudioEncoderStream::GetParentCodec(void) const { return _encoder->GetParentCodec(); }
		string AudioEncoderStream::GetEncodedFormat(void) const { return _encoder->GetEncodedFormat(); }
		const StreamDesc & AudioEncoderStream::GetEncodedDescriptor(void) const noexcept { return _encoder->GetEncodedDescriptor(); }
		uint AudioEncoderStream::GetChannelLayout(void) const noexcept { return _encoder->GetChannelLayout(); }
		Media::IMediaTrackSink * AudioEncoderStream::GetDestinationSink(void) noexcept { return _track; }
		bool AudioEncoderStream::WriteFrames(const WaveBuffer * buffer) noexcept
		{
			if (!buffer) return false;
			auto & d1 = buffer->GetFormatDescriptor();
			auto & d2 = GetFormatDescriptor();
			if (d1.Format != d2.Format || d1.ChannelCount != d2.ChannelCount || d1.FramesPerSecond != d2.FramesPerSecond) return false;
			if (!_encoder->SupplyFrames(buffer)) return false;
			return _drain_packets();
		}
		bool AudioEncoderStream::Finalize(void) noexcept
		{
			if (!_encoder->SupplyEndOfStream()) return false;
			if (!_drain_packets()) return false;
			if (!_track->UpdateCodecMagic(_encoder->GetCodecMagic())) return false;
			return _track->Finalize();
		}
	}
}