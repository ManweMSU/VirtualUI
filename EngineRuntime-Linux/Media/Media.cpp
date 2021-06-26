#include "Media.h"

#include "../Miscellaneous/DynamicString.h"

#include "MPEG4.h"
#include "WaveCodec.h"
#include "AudioStorage.h"

namespace Engine
{
	namespace Media
	{
		const DataBlock * TrackFormatDesc::GetCodecMagic(void) const noexcept { return _codec_magic; }
		void TrackFormatDesc::SetCodecMagic(const DataBlock * data) { if (data) _codec_magic = new DataBlock(*data); else _codec_magic.SetReference(0); }
		AudioTrackFormatDesc::AudioTrackFormatDesc(const string & codec, const Audio::StreamDesc & desc, uint channel_layout) :
			_codec(codec), _desc(desc), _channel_layout(channel_layout) {}
		AudioTrackFormatDesc::~AudioTrackFormatDesc(void) {}
		TrackClass AudioTrackFormatDesc::GetTrackClass(void) const noexcept { return TrackClass::Audio; }
		string AudioTrackFormatDesc::GetTrackCodec(void) const { return _codec; }
		TrackFormatDesc * AudioTrackFormatDesc::Clone(void) const
		{
			SafePointer<AudioTrackFormatDesc> result = new AudioTrackFormatDesc(_codec, _desc, _channel_layout);
			result->Retain();
			return result;
		}
		string AudioTrackFormatDesc::ToString(void) const
		{
			string fmt, ch, fps;
			if (_desc.Format != Audio::SampleFormat::Invalid) {
				fmt = string(Audio::SampleFormatBitSize(_desc.Format)) + string(L"-bit ");
				if (Audio::IsSignedFormat(_desc.Format)) fmt += L"signed normalized integer";
				else if (Audio::IsUnsignedFormat(_desc.Format)) fmt += L"unsigned normalized integer";
				else if (Audio::IsFloatingPointFormat(_desc.Format)) fmt += L"floating point";
			} else fmt = L"unspecified";
			if (_desc.ChannelCount) ch = string(_desc.ChannelCount); else ch = L"unspecified";
			if (_desc.FramesPerSecond) fps = string(_desc.FramesPerSecond); else fps = L"unspecified";
			if (_channel_layout) return FormatString(L"Audio track: codec: %0, sample format: %1, channel count: %2, channel layout: %3, frames per second: %4", _codec, fmt, ch, string(_channel_layout, HexadecimalBase, 8), fps);
			else return FormatString(L"Audio track: codec: %0, sample format: %1, channel count: %2, channel layout: unspecified, frame rate: %3", _codec, fmt, ch, fps);
		}
		const Audio::StreamDesc & AudioTrackFormatDesc::GetStreamDescriptor(void) const noexcept { return _desc; }
		uint AudioTrackFormatDesc::GetChannelLayout(void) const noexcept { return _channel_layout; }
		VideoTrackFormatDesc::VideoTrackFormatDesc(const string & codec, uint width, uint height, uint frame_rate_numerator, uint frame_rate_denominator, uint pixel_aspect_horz, uint pixel_aspect_vert) :
			_codec(codec), _width(width), _height(height), _rate_numerator(frame_rate_numerator), _rate_denominator(frame_rate_denominator),
			_pixel_aspect_horz(pixel_aspect_horz), _pixel_aspect_vert(pixel_aspect_vert) {}
		VideoTrackFormatDesc::~VideoTrackFormatDesc(void) {}
		TrackClass VideoTrackFormatDesc::GetTrackClass(void) const noexcept { return TrackClass::Video; }
		string VideoTrackFormatDesc::GetTrackCodec(void) const { return _codec; }
		TrackFormatDesc * VideoTrackFormatDesc::Clone(void) const
		{
			SafePointer<VideoTrackFormatDesc> result = new VideoTrackFormatDesc(_codec, _width, _height, _rate_numerator, _rate_denominator, _pixel_aspect_horz, _pixel_aspect_vert);
			result->Retain();
			return result;
		}
		string VideoTrackFormatDesc::ToString(void) const
		{
			if (_pixel_aspect_horz > 1 && _pixel_aspect_vert > 1) return FormatString(L"Video track: codec: %0, resolution: %1 x %2, frame rate: %3/%4, pixel aspect ratio: %5/%6",
				_codec, _width, _height, _rate_numerator, _rate_denominator, _pixel_aspect_horz, _pixel_aspect_vert);
			else return FormatString(L"Video track: codec: %0, resolution: %1 x %2, frame rate: %3/%4", _codec, _width, _height, _rate_numerator, _rate_denominator);
		}
		uint VideoTrackFormatDesc::GetWidth(void) const noexcept { return _width; }
		uint VideoTrackFormatDesc::GetHeight(void) const noexcept { return _height; }
		uint VideoTrackFormatDesc::GetFrameRateNumerator(void) const noexcept { return _rate_numerator; }
		uint VideoTrackFormatDesc::GetFrameRateDenominator(void) const noexcept { return _rate_denominator; }
		uint VideoTrackFormatDesc::GetPixelAspectHorizontal(void) const noexcept { return _pixel_aspect_horz; }
		uint VideoTrackFormatDesc::GetPixelAspectVertical(void) const noexcept { return _pixel_aspect_vert; }
		SubtitleTrackFormatDesc::SubtitleTrackFormatDesc(const string & codec, uint time_scale, uint flags) : _codec(codec), _time_scale(time_scale), _flags(flags) {}
		SubtitleTrackFormatDesc::~SubtitleTrackFormatDesc(void) {}
		TrackClass SubtitleTrackFormatDesc::GetTrackClass(void) const noexcept { return TrackClass::Subtitles; }
		string SubtitleTrackFormatDesc::GetTrackCodec(void) const { return _codec; }
		TrackFormatDesc * SubtitleTrackFormatDesc::Clone(void) const
		{
			SafePointer<SubtitleTrackFormatDesc> result = new SubtitleTrackFormatDesc(_codec, _time_scale, _flags);
			result->Retain();
			return result;
		}
		string SubtitleTrackFormatDesc::ToString(void) const
		{
			return FormatString(L"Subtitle track: codec: %0, frame rate: %1, flags: %2", _codec, _time_scale, string(_flags, BinaryBase, 4));
		}
		uint SubtitleTrackFormatDesc::GetTimeScale(void) const noexcept { return _time_scale; }
		uint SubtitleTrackFormatDesc::GetFlags(void) const noexcept { return _flags; }
		ContainerClassDesc::operator string(void) const
		{
			DynamicString result;
			result << L"Container format: " << ContainerFormatIdentifier;
			result << L", audio supported: " << (FormatCapabilities & ContainerClassCapabilityHoldAudio ? L"yes" : L"no");
			result << L", video supported: " << (FormatCapabilities & ContainerClassCapabilityHoldVideo ? L"yes" : L"no");
			result << L", subtitles supported: " << (FormatCapabilities & ContainerClassCapabilityHoldSubtitles ? L"yes" : L"no");
			result << L", metadata supported: " << (FormatCapabilities & ContainerClassCapabilityHoldMetadata ? string(L"yes, ") + MetadataFormatIdentifier : string(L"no"));
			result << L", interleaved tracks supported: " << (FormatCapabilities & ContainerClassCapabilityInterleavedTracks ? L"yes" : L"no");
			result << L", key frames supported: " << (FormatCapabilities & ContainerClassCapabilityKeyFrames ? L"yes" : L"no");
			result << L", maximal track count: " << (MaximalTrackCount ? string(MaximalTrackCount) : string(L"unlimited"));
			return result.ToString();
		}

		ObjectArray<IMediaContainerCodec> _media_codecs(0x10);

		void InitializeDefaultCodecs(void)
		{
			InitializeMPEG4Codec();
			InitializeRIFFCodec();
			InitializeAudioStorageCodec();
		}
		void RegisterCodec(IMediaContainerCodec * codec) { _media_codecs.Append(codec); }
		void UnregisterCodec(IMediaContainerCodec * codec) { for (int i = 0; i < _media_codecs.Length(); i++) if (_media_codecs.ElementAt(i) == codec) { _media_codecs.Remove(i); break; } }
		Array<string> * GetEncodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _media_codecs) {
				SafePointer< Array<ContainerClassDesc> > formats = codec.GetFormatsCanEncode();
				for (auto & format : formats->Elements()) {
					bool present = false;
					for (auto & added : result->Elements()) if (added == format.ContainerFormatIdentifier) { present = true; break; }
					if (!present) result->Append(format.ContainerFormatIdentifier);
				}
			}
			result->Retain();
			return result;
		}
		Array<string> * GetDecodeFormats(void)
		{
			SafePointer< Array<string> > result = new Array<string>(0x10);
			for (auto & codec : _media_codecs) {
				SafePointer< Array<ContainerClassDesc> > formats = codec.GetFormatsCanDecode();
				for (auto & format : formats->Elements()) {
					bool present = false;
					for (auto & added : result->Elements()) if (added == format.ContainerFormatIdentifier) { present = true; break; }
					if (!present) result->Append(format.ContainerFormatIdentifier);
				}
			}
			result->Retain();
			return result;
		}
		IMediaContainerCodec * FindEncoder(const string & format) { for (auto & codec : _media_codecs) if (codec.CanEncode(format)) return &codec; return 0; }
		IMediaContainerCodec * FindDecoder(const string & format) { for (auto & codec : _media_codecs) if (codec.CanDecode(format)) return &codec; return 0; }
		IMediaContainerSource * OpenContainer(Streaming::Stream * stream)
		{
			for (auto & codec : _media_codecs) {
				auto source = codec.OpenContainer(stream);
				if (source) return source;
			}
			return 0;
		}
		IMediaContainerSink * CreateContainer(Streaming::Stream * stream, const string & format)
		{
			auto codec = FindEncoder(format);
			if (!codec) return 0;
			auto sink = codec->CreateContainer(stream, format);
			return sink;
		}
	}
}