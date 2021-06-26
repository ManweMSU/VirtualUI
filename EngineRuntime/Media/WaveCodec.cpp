#include "WaveCodec.h"

namespace Engine
{
	namespace Audio
	{
		class WaveDecoder : public IAudioDecoder
		{
			SafePointer<IAudioCodec> _parent;
			StreamDesc _encoded, _outer;
			string _format;
			uint _channel_layout, _current_frame, _frames_pending;
			ObjectArray<WaveBuffer> _output;
		public:
			WaveDecoder(IAudioCodec * codec, const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) : _output(0x40)
			{
				_current_frame = _frames_pending = 0;
				if (format.GetTrackClass() != Media::TrackClass::Audio) throw InvalidFormatException();
				if (format.GetTrackCodec() != AudioFormatEngineWaveform && format.GetTrackCodec() != AudioFormatMicrosoftWaveform) throw InvalidFormatException();
				auto & ad = format.As<Media::AudioTrackFormatDesc>();
				_parent.SetRetain(codec);
				_format = ad.GetTrackCodec();
				_encoded = ad.GetStreamDescriptor();
				_channel_layout = ad.GetChannelLayout();
				if (desired_desc) {
					_outer.Format = (desired_desc->Format != SampleFormat::Invalid) ? desired_desc->Format : _encoded.Format;
					_outer.ChannelCount = (desired_desc->ChannelCount) ? desired_desc->ChannelCount : _encoded.ChannelCount;
					_outer.FramesPerSecond = (desired_desc->FramesPerSecond) ? desired_desc->FramesPerSecond : _encoded.FramesPerSecond;
				} else _outer = _encoded;
			}
			virtual ~WaveDecoder(void) override {}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _outer; }
			virtual uint GetChannelLayout(void) const noexcept override { return _channel_layout; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Decoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _encoded; }
			virtual bool Reset(void) noexcept override { _output.Clear(); _current_frame = _frames_pending = 0; return true; }
			virtual int GetPendingPacketsCount(void) const noexcept override { return 0; }
			virtual int GetPendingFramesCount(void) const noexcept override { return _frames_pending; }
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept override
			{
				if (packet.PacketDataActuallyUsed) {
					if (!packet.PacketData) return false;
					try {
						SafePointer<WaveBuffer> wave = new WaveBuffer(_encoded, packet.PacketDataActuallyUsed / StreamFrameByteSize(_encoded));
						MemoryCopy(wave->GetData(), packet.PacketData->GetBuffer(), wave->GetAllocatedSizeInBytes());
						wave->FramesUsed() = wave->GetSizeInFrames();
						if (_encoded.Format != _outer.Format) wave = wave->ConvertFormat(_outer.Format);
						if (_encoded.ChannelCount != _outer.ChannelCount) wave = wave->ReallocateChannels(_outer.ChannelCount);
						if (_encoded.FramesPerSecond != _outer.FramesPerSecond) wave = wave->ConvertFrameRate(_outer.FramesPerSecond);
						_output.Append(wave);
						_frames_pending += wave->FramesUsed();
						return true;
					} catch (...) { return false; }
				} else return false;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (_outer.Format != desc.Format || _outer.ChannelCount != desc.ChannelCount || _outer.FramesPerSecond != desc.FramesPerSecond) return false;
				buffer->FramesUsed() = 0;
				auto frames_read = min(uint(buffer->GetSizeInFrames()), _frames_pending);
				uint write_at = 0;
				while (frames_read) {
					auto drain = _output.FirstElement();
					auto available_now = min(frames_read, uint(drain->FramesUsed() - _current_frame));
					MemoryCopy(buffer->GetData() + write_at * StreamFrameByteSize(_outer), drain->GetData() + _current_frame * StreamFrameByteSize(_outer),
						available_now * StreamFrameByteSize(_outer));
					frames_read -= available_now;
					_frames_pending -= available_now;
					write_at += available_now;
					_current_frame += available_now;
					buffer->FramesUsed() += available_now;
					if (drain->FramesUsed() == _current_frame) {
						_current_frame = 0;
						_output.RemoveFirst();
					}
				}
				return true;
			}
		};
		class WaveEncoder : public IAudioEncoder
		{
			SafePointer<IAudioCodec> _parent;
			StreamDesc _encoded, _outer;
			string _format;
			uint _channel_layout, _frames_produced;
			SafePointer<Media::AudioTrackFormatDesc> _track;
			ObjectArray<WaveBuffer> _packets;
		public:
			WaveEncoder(IAudioCodec * codec, const string & format, const StreamDesc & desc, uint num_options, const uint * options) : _packets(0x20)
			{
				_frames_produced = 0;
				if (format != AudioFormatEngineWaveform && format != AudioFormatMicrosoftWaveform) throw InvalidFormatException();
				_parent.SetRetain(codec);
				_format = format;
				_outer = desc;
				_encoded = desc;
				if (format == AudioFormatMicrosoftWaveform) {
					if (_encoded.Format == SampleFormat::S8_snorm) _encoded.Format = SampleFormat::S8_unorm;
					else if (_encoded.Format == SampleFormat::S16_unorm) _encoded.Format = SampleFormat::S16_snorm;
					else if (_encoded.Format == SampleFormat::S24_unorm) _encoded.Format = SampleFormat::S24_snorm;
					else if (_encoded.Format == SampleFormat::S32_unorm) _encoded.Format = SampleFormat::S32_snorm;
				}
				_channel_layout = 0;
				for (uint i = 0; i < num_options; i++) if (options[2 * i] == Media::MediaEncoderChannelLayout) {
					_channel_layout = options[2 * i + 1];
				}
				_track = new Media::AudioTrackFormatDesc(_format, _encoded, _channel_layout);
			}
			virtual ~WaveEncoder(void) override {}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _outer; }
			virtual uint GetChannelLayout(void) const noexcept override { return _channel_layout; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Encoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _encoded; }
			virtual bool Reset(void) noexcept override { _frames_produced = 0; _packets.Clear(); return true; }
			virtual int GetPendingPacketsCount(void) const noexcept override { return _packets.Length(); }
			virtual int GetPendingFramesCount(void) const noexcept override { return 0; }
			virtual const Media::AudioTrackFormatDesc & GetFullEncodedDescriptor(void) const noexcept override { return *_track; }
			virtual const DataBlock * GetCodecMagic(void) noexcept override { return 0; }
			virtual bool SupplyFrames(const WaveBuffer * buffer) noexcept override
			{
				try {
					if (!buffer) return false;
					auto & desc = buffer->GetFormatDescriptor();
					if (_outer.Format != desc.Format || _outer.ChannelCount != desc.ChannelCount || _outer.FramesPerSecond != desc.FramesPerSecond) return false;
					if (!buffer->FramesUsed()) return false;
					SafePointer<WaveBuffer> wave;
					if (_outer.Format == _encoded.Format) {
						wave = new WaveBuffer(_encoded, buffer->FramesUsed());
						MemoryCopy(wave->GetData(), buffer->GetData(), wave->GetAllocatedSizeInBytes());
						wave->FramesUsed() = wave->GetSizeInFrames();
					} else wave = buffer->ConvertFormat(_encoded.Format);
					_packets.Append(wave);
					return true;
				} catch (...) { return false; }
			}
			virtual bool SupplyEndOfStream(void) noexcept override { return true; }
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept override
			{
				try {
					if (_packets.Length()) {
						auto wave = _packets.FirstElement();
						if (!packet.PacketData) packet.PacketData = new DataBlock(0x10000);
						if (packet.PacketData->Length() < wave->GetUsedSizeInBytes()) packet.PacketData->SetLength(wave->GetUsedSizeInBytes());
						MemoryCopy(packet.PacketData->GetBuffer(), wave->GetData(), wave->GetUsedSizeInBytes());
						packet.PacketDataActuallyUsed = wave->GetUsedSizeInBytes();
						packet.PacketIsKey = true;
						packet.PacketDecodeTime = packet.PacketRenderTime = _frames_produced;
						packet.PacketRenderDuration = wave->FramesUsed();
						_frames_produced += wave->FramesUsed();
						_packets.RemoveFirst();
					} else {
						packet.PacketDataActuallyUsed = 0;
						packet.PacketIsKey = true;
						packet.PacketDecodeTime = packet.PacketRenderTime = _frames_produced;
						packet.PacketRenderDuration = 0;
					}
					return true;
				} catch (...) { return false; }
			}
		};
		class WaveCodec : public IAudioCodec
		{
		public:
			WaveCodec(void) {}
			virtual ~WaveCodec(void) override {}
			virtual bool CanEncode(const string & format) const noexcept override { return format == AudioFormatEngineWaveform || format == AudioFormatMicrosoftWaveform; }
			virtual bool CanDecode(const string & format) const noexcept override { return format == AudioFormatEngineWaveform || format == AudioFormatMicrosoftWaveform; }
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(2);
				result->Append(AudioFormatEngineWaveform);
				result->Append(AudioFormatMicrosoftWaveform);
				result->Retain();
				return result;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(2);
				result->Append(AudioFormatEngineWaveform);
				result->Append(AudioFormatMicrosoftWaveform);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine Waveform Codec"; }
			virtual IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) noexcept override { try { return new WaveDecoder(this, format, desired_desc); } catch (...) { return 0; } }
			virtual IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options, const uint * options) noexcept override { try { return new WaveEncoder(this, format, desc, num_options, options); } catch (...) { return 0; } }
		};

		SafePointer<IAudioCodec> _engine_wave_codec;
		IAudioCodec * InitializeWaveCodec(void)
		{
			if (!_engine_wave_codec) {
				_engine_wave_codec = new WaveCodec;
				RegisterCodec(_engine_wave_codec);
			}
			return _engine_wave_codec;
		}
	}
	namespace Media
	{
		namespace Format {
			ENGINE_PACKED_STRUCTURE(WaveCoreHeader)
				uint16 data_format;  // 0x0001 - integral PCM, 0x0003 - float, 0xFFFE - extended
				uint16 num_channels;
				uint32 frames_per_second;
				uint32 bytes_per_second;
				uint16 bytes_per_frame;
				uint16 bits_per_sample;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(WaveExtendedHeader)
				uint16 extended_data_length;  // 22
				uint16 valid_bits_per_sample;
				uint32 channel_layout;
				uint32 format_guid_data1;     // same as data_format
				uint16 format_guid_data2;     // 0x0000
				uint16 format_guid_data3;     // 0x0010
				uint64 format_guid_data4;     // 0x719B3800AA000080
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(WaveFileHeader)
				char riff_sign[4];        // "RIFF"
				uint32 file_size;
				char wave_sign[4];        // "WAVE"
				char wave_header_sign[4]; // "fmt "
				uint32 header_size;       // 16 or 40
				WaveCoreHeader core_header;
				char data_sign[4];        // "data"
				uint32 data_length;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(WaveFileExtendedHeader)
				char riff_sign[4];        // "RIFF"
				uint32 file_size;
				char wave_sign[4];        // "WAVE"
				char wave_header_sign[4]; // "fmt "
				uint32 header_size;       // 16 or 40
				WaveCoreHeader core_header;
				WaveExtendedHeader extended_header;
				char data_sign[4];        // "data"
				uint32 data_length;
			ENGINE_END_PACKED_STRUCTURE
		}

		class RIFFTrackSink : public IMediaTrackSink
		{
			friend class RIFFContainerSink;

			IMediaContainerSink * _parent;
			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<AudioTrackFormatDesc> _track_desc;
			Audio::StreamDesc _desc;
			uint _channel_layout;
			uint _header_length;
			bool _sealed;
		public:
			RIFFTrackSink(Streaming::Stream * stream, IMediaContainerSink * parent, const TrackFormatDesc & desc) : _sealed(false)
			{
				if (desc.GetTrackClass() != TrackClass::Audio || desc.GetTrackCodec() != Audio::AudioFormatMicrosoftWaveform) throw InvalidFormatException();
				auto & ad = desc.As<AudioTrackFormatDesc>();
				_desc = ad.GetStreamDescriptor();
				_channel_layout = ad.GetChannelLayout();
				_parent = parent;
				_codec.SetRetain(parent->GetParentCodec());
				_stream.SetRetain(stream);
				_track_desc = new AudioTrackFormatDesc(Audio::AudioFormatMicrosoftWaveform, _desc, _channel_layout);
				_header_length = sizeof(Format::WaveFileHeader);
				if (_channel_layout) {
					if (_desc.ChannelCount > 2) _header_length = sizeof(Format::WaveFileExtendedHeader);
					else if (_desc.ChannelCount == 2 && _channel_layout != (Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight)) _header_length = sizeof(Format::WaveFileExtendedHeader);
					else if (_desc.ChannelCount == 1 && _channel_layout != Audio::ChannelLayoutCenter) _header_length = sizeof(Format::WaveFileExtendedHeader);
				}
				_stream->SetLength(_header_length);
				_stream->Seek(_header_length, Streaming::Begin);
			}
			virtual ~RIFFTrackSink(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _parent; }
			virtual TrackClass GetTrackClass(void) const noexcept override { return TrackClass::Audio; }
			virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_track_desc; }
			virtual string GetTrackName(void) const override { return L""; }
			virtual string GetTrackLanguage(void) const override { return L""; }
			virtual bool IsTrackVisible(void) const noexcept override { return true; }
			virtual bool IsTrackAutoselectable(void) const noexcept override { return true; }
			virtual int GetTrackGroup(void) const noexcept override { return 0; }
			virtual bool SetTrackName(const string & name) noexcept override { return true; }
			virtual bool SetTrackLanguage(const string & language) noexcept override { return true; }
			virtual void MakeTrackVisible(bool make) noexcept override {}
			virtual void MakeTrackAutoselectable(bool make) noexcept override {}
			virtual void SetTrackGroup(int group) noexcept override {}
			virtual bool WritePacket(const PacketBuffer & buffer) noexcept override
			{
				if (_sealed) return false;
				try {
					if (buffer.PacketDataActuallyUsed) _stream->Write(buffer.PacketData->GetBuffer(), buffer.PacketDataActuallyUsed);
					return true;
				} catch (...) { return false; }
			}
			virtual bool UpdateCodecMagic(const DataBlock * data) noexcept override { return true; }
			virtual bool Sealed(void) const noexcept override { return _sealed; }
			virtual bool Finalize(void) noexcept override
			{
				if (_sealed) return false;
				try {
					auto pos = _stream->Seek(0, Streaming::Current);
					uint8 zero = 0;
					_stream->Write(&zero, 1);
					_sealed = true;
					if (_parent->IsAutofinalizable()) return _parent->Finalize(); else return true;
				} catch (...) { return false; }
			}
		};
		class RIFFContainerSink : public IMediaContainerSink
		{
			SafePointer<Streaming::Stream> _stream;
			SafePointer<IMediaContainerCodec> _parent;
			SafePointer<RIFFTrackSink> _track;
			ContainerClassDesc _desc;
			bool _auto_finalize, _sealed;
		public:
			RIFFContainerSink(Streaming::Stream * stream, IMediaContainerCodec * parent, const ContainerClassDesc & desc) : _auto_finalize(false), _sealed(false)
			{
				_stream.SetRetain(stream);
				_parent.SetRetain(parent);
				_desc = desc;
			}
			virtual ~RIFFContainerSink(void) {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _parent; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { return 0; }
			virtual int GetTrackCount(void) const noexcept override { return _track ? 1 : 0; }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { if (index) return 0; return _track; }
			virtual void WriteMetadata(const Metadata * metadata) noexcept override {}
			virtual IMediaTrackSink * CreateTrack(const TrackFormatDesc & desc) noexcept override
			{
				if (_track) return 0;
				try {
					_track = new RIFFTrackSink(_stream, this, desc);
					_track->Retain();
					return _track;
				} catch (...) { return 0; }
			}
			virtual void SetAutofinalize(bool set) noexcept override { _auto_finalize = set; }
			virtual bool IsAutofinalizable(void) const noexcept override { return _auto_finalize; }
			virtual bool Finalize(void) noexcept override
			{
				if (_sealed) return false;
				if (!_track) return false;
				if (!_track->_sealed) return false;
				try {
					uint64 end = _stream->Seek(0, Streaming::Current);
					uint64 length = end - _track->_header_length;
					if (end >= 0x100000000) return false;
					if (_track->_header_length == sizeof(Format::WaveFileHeader)) {
						_stream->Seek(0, Streaming::Begin);
						Format::WaveFileHeader hdr;
						MemoryCopy(&hdr.riff_sign, "RIFF", 4);
						hdr.file_size = length + sizeof(hdr) - 8;
						MemoryCopy(&hdr.wave_sign, "WAVE", 4);
						MemoryCopy(&hdr.wave_header_sign, "fmt ", 4);
						hdr.header_size = 16;
						hdr.core_header.data_format = Audio::IsFloatingPointFormat(_track->_desc.Format) ? 0x0003 : 0x0001;
						hdr.core_header.num_channels = _track->_desc.ChannelCount;
						hdr.core_header.frames_per_second = _track->_desc.FramesPerSecond;
						hdr.core_header.bytes_per_second = Audio::StreamFrameByteSize(_track->_desc) * _track->_desc.FramesPerSecond;
						hdr.core_header.bytes_per_frame = Audio::StreamFrameByteSize(_track->_desc);
						hdr.core_header.bits_per_sample = Audio::SampleFormatBitSize(_track->_desc.Format);
						MemoryCopy(&hdr.data_sign, "data", 4);
						hdr.data_length = length;
						_stream->Write(&hdr, sizeof(hdr));
					} else if (_track->_header_length == sizeof(Format::WaveFileExtendedHeader)) {
						_stream->Seek(0, Streaming::Begin);
						Format::WaveFileExtendedHeader hdr;
						MemoryCopy(&hdr.riff_sign, "RIFF", 4);
						hdr.file_size = length + sizeof(hdr) - 8;
						MemoryCopy(&hdr.wave_sign, "WAVE", 4);
						MemoryCopy(&hdr.wave_header_sign, "fmt ", 4);
						hdr.header_size = 40;
						hdr.core_header.data_format = 0xFFFE;
						hdr.core_header.num_channels = _track->_desc.ChannelCount;
						hdr.core_header.frames_per_second = _track->_desc.FramesPerSecond;
						hdr.core_header.bytes_per_second = Audio::StreamFrameByteSize(_track->_desc) * _track->_desc.FramesPerSecond;
						hdr.core_header.bytes_per_frame = Audio::StreamFrameByteSize(_track->_desc);
						hdr.core_header.bits_per_sample = Audio::SampleFormatBitSize(_track->_desc.Format);
						hdr.extended_header.extended_data_length = 22;
						hdr.extended_header.valid_bits_per_sample = Audio::SampleFormatBitSize(_track->_desc.Format);
						hdr.extended_header.channel_layout = _track->_channel_layout;
						hdr.extended_header.format_guid_data1 = Audio::IsFloatingPointFormat(_track->_desc.Format) ? 0x0003 : 0x0001;
						hdr.extended_header.format_guid_data2 = 0x0000;
						hdr.extended_header.format_guid_data3 = 0x0010;
						hdr.extended_header.format_guid_data4 = 0x719B3800AA000080;
						MemoryCopy(&hdr.data_sign, "data", 4);
						hdr.data_length = length;
						_stream->Write(&hdr, sizeof(hdr));
					} else return false;
					return true;
				} catch (...) { return false; }
			}
		};

		class RIFFTrackSource : public IMediaTrackSource
		{
			friend class RIFFContainerSource;
			
			IMediaContainerSource * _parent;
			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<AudioTrackFormatDesc> _track;
			SafePointer<Streaming::Stream> _stream;
			uint64 _data_offset, _data_size, _num_frames;
			uint64 _current_frame;
			Audio::StreamDesc _desc;
		public:
			RIFFTrackSource(void) : _current_frame(0) {}
			virtual ~RIFFTrackSource(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _parent; }
			virtual TrackClass GetTrackClass(void) const noexcept override { return TrackClass::Audio; }
			virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_track; }
			virtual string GetTrackName(void) const override { return L""; }
			virtual string GetTrackLanguage(void) const override { return L""; }
			virtual bool IsTrackVisible(void) const noexcept override { return true; }
			virtual bool IsTrackAutoselectable(void) const noexcept override { return true; }
			virtual int GetTrackGroup(void) const noexcept override { return 0; }
			virtual uint64 GetTimeScale(void) const noexcept override { return _desc.FramesPerSecond; }
			virtual uint64 GetDuration(void) const noexcept override { return _num_frames; }
			virtual uint64 GetPosition(void) const noexcept override { return _current_frame; }
			virtual uint64 Seek(uint64 time) noexcept override
			{
				auto seek = time;
				if (seek >= _num_frames) seek = _num_frames - 1;
				_current_frame = seek;
				return _current_frame;
			}
			virtual uint64 GetCurrentPacket(void) const noexcept override { return _current_frame; }
			virtual uint64 GetPacketCount(void) const noexcept override { return _num_frames; }
			virtual bool ReadPacket(PacketBuffer & buffer) noexcept override
			{
				try {
					if (_current_frame >= _num_frames) {
						buffer.PacketDataActuallyUsed = 0;
						buffer.PacketIsKey = true;
						buffer.PacketDecodeTime = buffer.PacketRenderTime = _num_frames;
						buffer.PacketRenderDuration = 0;
					} else {
						uint64 frames_left = _num_frames - _current_frame;
						uint length = min(frames_left, uint64(_desc.FramesPerSecond));
						uint bytes = length * Audio::StreamFrameByteSize(_desc);
						if (!buffer.PacketData) buffer.PacketData = new DataBlock(Audio::StreamFrameByteSize(_desc) * _desc.FramesPerSecond);
						if (buffer.PacketData->Length() < bytes) buffer.PacketData->SetLength(bytes);
						_stream->Seek(_data_offset + _current_frame * Audio::StreamFrameByteSize(_desc), Streaming::Begin);
						_stream->Read(buffer.PacketData->GetBuffer(), bytes);
						buffer.PacketDataActuallyUsed = bytes;
						buffer.PacketIsKey = true;
						buffer.PacketDecodeTime = buffer.PacketRenderTime = _current_frame;
						buffer.PacketRenderDuration = length;
						_current_frame += length;
					}
					return true;
				} catch (...) { return false; }
			}
		};
		class RIFFContainerSource : public IMediaContainerSource
		{
			SafePointer<IMediaContainerCodec> _parent;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<RIFFTrackSource> _track;
			ContainerClassDesc _class_desc;
		public:
			RIFFContainerSource(Streaming::Stream * stream, IMediaContainerCodec * parent, const ContainerClassDesc & desc)
			{
				_parent.SetRetain(parent);
				_stream.SetRetain(stream);
				_class_desc = desc;
				char sign[12];
				_stream->Seek(0, Streaming::Begin);
				_stream->Read(sign, 12);
				_stream->Seek(0, Streaming::Begin);
				if (MemoryCompare(sign, "RIFF", 4) == 0 && MemoryCompare(sign + 8, "WAVE", 4) == 0) {
					_stream->Seek(4, Streaming::Begin);
					uint32 main_size, chunk_size;
					char chunk_name[4];
					_stream->Seek(4, Streaming::Begin);
					_stream->Read(&main_size, 4);
					_stream->Seek(12, Streaming::Begin);
					_track = new RIFFTrackSource;
					_track->_parent = this;
					_track->_codec = _parent;
					_track->_stream = _stream;
					while (_stream->Seek(0, Streaming::Current) <= int64(main_size)) {
						_stream->Read(chunk_name, 4);
						_stream->Read(&chunk_size, 4);
						if (MemoryCompare(chunk_name, "fmt ", 4) == 0) {
							if (chunk_size < 16) throw Exception();
							Format::WaveCoreHeader header;
							_stream->Read(&header, sizeof(header));
							if (!header.num_channels) throw Exception();
							if (!header.frames_per_second) throw Exception();
							_track->_desc.ChannelCount = header.num_channels;
							_track->_desc.FramesPerSecond = header.frames_per_second;
							uint cl = 0;
							if (header.data_format == 1) {
								if (header.bits_per_sample == 32) _track->_desc.Format = Audio::SampleFormat::S32_snorm;
								else if (header.bits_per_sample == 24) _track->_desc.Format = Audio::SampleFormat::S24_snorm;
								else if (header.bits_per_sample == 16) _track->_desc.Format = Audio::SampleFormat::S16_snorm;
								else if (header.bits_per_sample == 8) _track->_desc.Format = Audio::SampleFormat::S8_unorm;
								else throw InvalidFormatException();
							} else if (header.data_format == 3) {
								if (header.bits_per_sample == 32) _track->_desc.Format = Audio::SampleFormat::S32_float;
								else if (header.bits_per_sample == 64) _track->_desc.Format = Audio::SampleFormat::S64_float;
								else throw InvalidFormatException();
							} else if (header.data_format == 0xFFFE) {
								Format::WaveExtendedHeader hex;
								_stream->Read(&hex, sizeof(hex));
								cl = hex.channel_layout;
								if (hex.format_guid_data2 != 0x0000 && hex.format_guid_data3 != 0x0010 && hex.format_guid_data4 != 0x719B3800AA000080) {
									throw InvalidFormatException();
								}
								if (hex.format_guid_data1 == 0x00000001) {
									if (header.bits_per_sample == 32) _track->_desc.Format = Audio::SampleFormat::S32_snorm;
									else if (header.bits_per_sample == 24) _track->_desc.Format = Audio::SampleFormat::S24_snorm;
									else if (header.bits_per_sample == 16) _track->_desc.Format = Audio::SampleFormat::S16_snorm;
									else if (header.bits_per_sample == 8) _track->_desc.Format = Audio::SampleFormat::S8_unorm;
									else throw InvalidFormatException();
								} else if (hex.format_guid_data1 == 0x00000003) {
									if (header.bits_per_sample == 32) _track->_desc.Format = Audio::SampleFormat::S32_float;
									else if (header.bits_per_sample == 64) _track->_desc.Format = Audio::SampleFormat::S64_float;
									else throw InvalidFormatException();
								} else throw InvalidFormatException();
							} else throw InvalidFormatException();
							if (header.bytes_per_frame != Audio::StreamFrameByteSize(_track->_desc)) throw Exception();
							if (!cl) {
								if (_track->_desc.ChannelCount == 1) cl = Audio::ChannelLayoutCenter;
								else if (_track->_desc.ChannelCount == 2) cl = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight;
							}
							_track->_track = new AudioTrackFormatDesc(Audio::AudioFormatMicrosoftWaveform, _track->_desc, cl);
						} else if (MemoryCompare(chunk_name, "data", 4) == 0) {
							_track->_data_offset = _stream->Seek(0, Streaming::Current);
							_track->_data_size = chunk_size;
							if (chunk_size & 1) chunk_size++;
							_stream->Seek(chunk_size, Streaming::Current);
						} else {
							if (chunk_size & 1) chunk_size++;
							_stream->Seek(chunk_size, Streaming::Current);
						}
					}
					if (!_track->_track) throw InvalidFormatException();
					_track->_num_frames = _track->_data_size / Audio::StreamFrameByteSize(_track->_desc);
				} else throw InvalidFormatException();
			}
			virtual ~RIFFContainerSource(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _parent; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _class_desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { return 0; }
			virtual int GetTrackCount(void) const noexcept override { return 1; }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { if (index) return 0; return _track; }
			virtual IMediaTrackSource * OpenTrack(int index) const noexcept override { if (index) return 0; _track->Retain(); return _track; }
			virtual uint64 GetDuration(void) const noexcept override { return (_track->GetDuration() * 1000 + _track->GetTimeScale() - 1) / _track->GetTimeScale(); }
			virtual bool PatchMetadata(const Metadata * metadata) noexcept override { return false; }
			virtual bool PatchMetadata(const Metadata * metadata, Streaming::Stream * dest) const noexcept override { return false; }
		};
		
		class EngineRIFFCodec : public IMediaContainerCodec
		{
			ContainerClassDesc riff_wav;
		public:
			EngineRIFFCodec(void)
			{
				riff_wav.ContainerFormatIdentifier = ContainerFormatRIFF_WAV;
				riff_wav.FormatCapabilities = ContainerClassCapabilityHoldAudio;
				riff_wav.MaximalTrackCount = 1;
				riff_wav.MetadataFormatIdentifier = L"";
			}
			virtual ~EngineRIFFCodec(void) override {}
			virtual bool CanEncode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatRIFF_WAV) {
					if (desc) *desc = riff_wav;
					return true;
				} else return false;
			}
			virtual bool CanDecode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatRIFF_WAV) {
					if (desc) *desc = riff_wav;
					return true;
				} else return false;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(1);
				result->Append(riff_wav);
				result->Retain();
				return result;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(1);
				result->Append(riff_wav);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine RIFF Multiplexor"; }
			virtual IMediaContainerSource * OpenContainer(Streaming::Stream * source) noexcept override { try { return new RIFFContainerSource(source, this, riff_wav); } catch (...) { return 0; } }
			virtual IMediaContainerSink * CreateContainer(Streaming::Stream * dest, const string & format) noexcept override
			{
				if (format == ContainerFormatRIFF_WAV) {
					try { return new RIFFContainerSink(dest, this, riff_wav); } catch (...) { return 0; }
				} else return 0;
			}
		};

		SafePointer<IMediaContainerCodec> _engine_riff_codec;
		IMediaContainerCodec * InitializeRIFFCodec(void)
		{
			if (!_engine_riff_codec) {
				_engine_riff_codec = new EngineRIFFCodec;
				RegisterCodec(_engine_riff_codec);
			}
			return _engine_riff_codec;
		}
	}
}