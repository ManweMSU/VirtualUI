#include "../Interfaces/SystemAudio.h"

#include "../Interfaces/Socket.h"

#include <Windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <wmcodecdsp.h>
#include <Mferror.h>
#include <functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

#undef CreateSemaphore

using namespace Engine::Streaming;

namespace Engine
{
	namespace Audio
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(alac_spec_config)
				uint32 frames_per_packet;
				uint8 com_version; // set to 0
				uint8 bits_per_sample; // 8, 16, 24 or 32
				uint8 bp; // set to 40
				uint8 mb; // set to 10
				uint8 kb; // set to 14
				uint8 channel_count;
				uint16 max_run; // set to 255
				uint32 max_packet_size;
				uint32 ave_bit_rate;
				uint32 frames_per_second;
			ENGINE_END_PACKED_STRUCTURE
		}
		class MediaFoundationDecoder : public IAudioDecoder
		{
			SafePointer<IAudioCodec> _parent;
			string _format;
			StreamDesc _encoded, _internal, _outer;
			IMFTransform * _transform;
			SafePointer<DataBlock> _codec_data;
			uint _channel_layout;
			uint _packets_pending;
			uint _frames_pending;
			uint _local_frame_offset;
			ObjectArray<WaveBuffer> _frames;
			bool _first_packet;

			void _select_formats(const Media::AudioTrackFormatDesc & in_desc, const StreamDesc * desired_desc)
			{
				_encoded = in_desc.GetStreamDescriptor();
				_internal = _encoded;
				if (_internal.Format == SampleFormat::Invalid) _internal.Format = SampleFormat::S16_snorm;
				if (desired_desc) {
					if (desired_desc->Format != SampleFormat::Invalid) _outer.Format = desired_desc->Format;
					else _outer.Format = _internal.Format;
					if (desired_desc->ChannelCount) _outer.ChannelCount = desired_desc->ChannelCount;
					else _outer.ChannelCount = _internal.ChannelCount;
					if (desired_desc->FramesPerSecond) _outer.FramesPerSecond = desired_desc->FramesPerSecond;
					else _outer.FramesPerSecond = _internal.FramesPerSecond;
				} else _outer = _internal;
			}
			void _transform_recreate(void)
			{
				if (_transform) _transform->Release();
				_transform = 0;
				if (_format == AudioFormatMP3) {
					if (CoCreateInstance(CLSID_CMP3DecMediaObject, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else if (_format == AudioFormatAAC) {
					if (CoCreateInstance(CLSID_CMSAACDecMFT, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else if (_format == AudioFormatFreeLossless) {
					if (CoCreateInstance(CLSID_CMSFLACDecMFT, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else if (_format == AudioFormatAppleLossless) {
					if (CoCreateInstance(CLSID_CMSALACDecMFT, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else throw InvalidFormatException();
			}
			void _setup_inputs(void)
			{
				if (_format == AudioFormatMP3) {
					DataBlock user_data(0x20);
					user_data.Append(1);
					for (int i = 0; i < 11; i++) user_data.Append(0);
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetBlob(MF_MT_USER_DATA, user_data.GetBuffer(), user_data.Length()) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					int index = 0;
					while (true) {
						if (_transform->GetOutputAvailableType(0, index, &type) == S_OK) {
							GUID subtype;
							UINT32 bps, channels, fps, frame_size;
							if (type->GetGUID(MF_MT_SUBTYPE, &subtype) != S_OK) { type->Release(); throw Exception(); }
							if (type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bps) != S_OK) { type->Release(); throw Exception(); }
							if (type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels) != S_OK) { type->Release(); throw Exception(); }
							if (type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &fps) != S_OK) { type->Release(); throw Exception(); }
							if (type->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &frame_size) != S_OK) { type->Release(); throw Exception(); }
							if (subtype == MFAudioFormat_PCM && bps == 16 && channels == _internal.ChannelCount && fps == _internal.FramesPerSecond && frame_size == _internal.ChannelCount * 2) {
								if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
								type->Release();
								break;
							}
							type->Release();
						} else throw Exception();
						index++;
					}
				} else if (_format == AudioFormatAAC) {
					DataBlock user_data(0x20);
					for (int i = 0; i < 12; i++) user_data.Append(0);
					user_data.Append(*_codec_data);
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetBlob(MF_MT_USER_DATA, user_data.GetBuffer(), user_data.Length()) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
				} else if (_format == AudioFormatFreeLossless) {
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_FLAC) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, SampleFormatBitSize(_internal.Format)) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, StreamFrameByteSize(_internal)) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					if (_transform->GetOutputAvailableType(0, 0, &type) != S_OK) throw Exception();
					if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
				} else if (_format == AudioFormatAppleLossless) {
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_ALAC) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, SampleFormatBitSize(_internal.Format)) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetBlob(MF_MT_USER_DATA, _codec_data->GetBuffer(), _codec_data->Length()) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					if (_transform->GetOutputAvailableType(0, 0, &type) != S_OK) throw Exception();
					if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
				}
			}
		public:
			MediaFoundationDecoder(IAudioCodec * codec, const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) : _frames(0x20)
			{
				if (format.GetTrackClass() != Media::TrackClass::Audio) throw InvalidFormatException();
				if (MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET) != S_OK) throw Exception();
				try {
					_first_packet = true;
					_transform = 0;
					_channel_layout = format.As<Media::AudioTrackFormatDesc>().GetChannelLayout();
					_packets_pending = _frames_pending = _local_frame_offset = 0;
					_parent.SetRetain(codec);
					_format = format.GetTrackCodec();
					_select_formats(format.As<Media::AudioTrackFormatDesc>(), desired_desc);
					_transform_recreate();
					if (_format == AudioFormatMP3) {
						_setup_inputs();
					} else if (_format == AudioFormatAAC) {
						if (!format.GetCodecMagic()) throw InvalidFormatException();
						_codec_data = new DataBlock(*format.GetCodecMagic());
						_setup_inputs();
					} else if (_format == AudioFormatFreeLossless) {
						if (!format.GetCodecMagic()) throw InvalidFormatException();
						_codec_data = new DataBlock(*format.GetCodecMagic());
						_setup_inputs();
					} else if (_format == AudioFormatAppleLossless) {
						if (!format.GetCodecMagic()) throw InvalidFormatException();
						_codec_data = new DataBlock(*format.GetCodecMagic());
						_setup_inputs();
					}
				} catch (...) { if (_transform) _transform->Release(); MFShutdown(); throw; }
			}
			virtual ~MediaFoundationDecoder(void) override { if (_transform) _transform->Release(); MFShutdown(); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _outer; }
			virtual uint GetChannelLayout(void) const noexcept override { return _channel_layout; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Decoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _encoded; }
			virtual bool Reset(void) noexcept override
			{
				try {
					_first_packet = true;
					_packets_pending = _frames_pending = _local_frame_offset = 0;
					_frames.Clear();
					if (_format == AudioFormatFreeLossless) {
						_transform_recreate();
						_setup_inputs();
					} else {
						if (_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0) != S_OK) return false;
					}
					return true;
				} catch (...) { return false; }
			}
			virtual int GetPendingPacketsCount(void) const noexcept override { return _packets_pending; }
			virtual int GetPendingFramesCount(void) const noexcept override { return _frames_pending; }
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept override
			{
				if (packet.PacketDataActuallyUsed) {
					if (!packet.PacketData) return false;
					SafePointer<DataBlock> prepend;
					if (_first_packet) {
						try {
							if (_format == AudioFormatFreeLossless) {
								uint size = _codec_data->Length() + 8;
								prepend = new DataBlock(size);
								prepend->SetLength(size);
								prepend->ElementAt(0) = 'f'; prepend->ElementAt(1) = 'L'; prepend->ElementAt(2) = 'a'; prepend->ElementAt(3) = 'C';
								prepend->ElementAt(4) = 0x80; prepend->ElementAt(5) = 0; prepend->ElementAt(6) = 0; prepend->ElementAt(7) = _codec_data->Length();
								MemoryCopy(prepend->GetBuffer() + 8, _codec_data->GetBuffer(), _codec_data->Length());
							}
						} catch (...) { return false; }
						_first_packet = false;
					}
					IMFSample * sample;
					IMFMediaBuffer * buffer;
					LPBYTE data_ptr;
					uint prepend_size = prepend ? prepend->Length() : 0;
					uint buffer_size = packet.PacketDataActuallyUsed;
					if (prepend) buffer_size += prepend->Length();
					if (MFCreateMemoryBuffer(buffer_size, &buffer) != S_OK) return false;
					if (buffer->Lock(&data_ptr, 0, 0) != S_OK) { buffer->Release(); return false; }
					if (prepend) MemoryCopy(data_ptr, prepend->GetBuffer(), prepend->Length());
					MemoryCopy(data_ptr + prepend_size, packet.PacketData->GetBuffer(), packet.PacketDataActuallyUsed);
					buffer->Unlock();
					if (buffer->SetCurrentLength(buffer_size) != S_OK) { buffer->Release(); return false; }
					if (MFCreateSample(&sample) != S_OK) { buffer->Release(); return false; }
					if (sample->AddBuffer(buffer) != S_OK) { buffer->Release(); sample->Release(); return false; }
					buffer->Release();
					if (sample->SetSampleDuration(uint64(packet.PacketRenderDuration) * 10000000 / _encoded.FramesPerSecond) != S_OK) { sample->Release(); return false; }
					if (sample->SetSampleTime(uint64(packet.PacketRenderTime) * 10000000 / _encoded.FramesPerSecond) != S_OK) { sample->Release(); return false; }
					if (_transform->ProcessInput(0, sample, 0) != S_OK) { sample->Release(); return false; }
					sample->Release();
					_packets_pending++;
				} else {
					if (_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0) != S_OK) return false;
					if (_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0) != S_OK) return false;
					_packets_pending = 0;
				}
				MFT_OUTPUT_STREAM_INFO info;
				if (_transform->GetOutputStreamInfo(0, &info) != S_OK) return false;
				IMFSample * sample;
				IMFMediaBuffer * buffer = 0;
				if (info.cbSize) {
					if (MFCreateMemoryBuffer(info.cbSize, &buffer) != S_OK) return false;
					if (MFCreateSample(&sample) != S_OK) { buffer->Release(); return false; }
					if (sample->AddBuffer(buffer) != S_OK) { buffer->Release(); sample->Release(); return false; }
				} else {
					if (MFCreateSample(&sample) != S_OK) return false;
				}
				while (true) {
					MFT_OUTPUT_DATA_BUFFER output;
					DWORD status;
					ZeroMemory(&output, sizeof(output));
					output.pSample = sample;
					auto result = _transform->ProcessOutput(0, 1, &output, &status);
					if (result == MF_E_TRANSFORM_NEED_MORE_INPUT) break;
					if (result != S_OK) { sample->Release(); if (buffer) buffer->Release(); return false; }
					_packets_pending = 0;
					if (output.pEvents) output.pEvents->Release();
					try {
						DWORD length;
						LPBYTE data_ptr;
						if (!buffer && sample->GetBufferByIndex(0, &buffer) != S_OK) throw Exception();
						if (buffer->GetCurrentLength(&length) != S_OK) throw Exception();
						auto num_frames = length / StreamFrameByteSize(_internal);
						SafePointer<WaveBuffer> wave = new WaveBuffer(_internal, num_frames);
						if (buffer->Lock(&data_ptr, 0, 0) != S_OK) throw Exception();
						MemoryCopy(wave->GetData(), data_ptr, length);
						buffer->Unlock();
						wave->FramesUsed() = num_frames;
						if (wave->GetFormatDescriptor().ChannelCount != _outer.ChannelCount) wave = wave->ReallocateChannels(_outer.ChannelCount);
						if (wave->GetFormatDescriptor().FramesPerSecond != _outer.FramesPerSecond) wave = wave->ConvertFrameRate(_outer.FramesPerSecond);
						if (wave->GetFormatDescriptor().Format != _outer.Format) wave = wave->ConvertFormat(_outer.Format);
						_frames.Append(wave);
						_frames_pending += wave->FramesUsed();
					} catch (...) { sample->Release(); if (buffer) buffer->Release(); return false; }
				}
				sample->Release();
				if (buffer) buffer->Release();
				return true;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != _outer.Format || desc.ChannelCount != _outer.ChannelCount || desc.FramesPerSecond != _outer.FramesPerSecond) return false;
				uint64 write_pos = 0;
				auto free_space = buffer->GetSizeInFrames();
				auto frame_size = StreamFrameByteSize(_outer);
				buffer->FramesUsed() = 0;
				while (free_space && _frames_pending) {
					auto cb = _frames.FirstElement();
					auto cba = cb->FramesUsed();
					auto cbr = cba - _local_frame_offset;
					if (cbr > free_space) cbr = free_space;
					MemoryCopy(buffer->GetData() + frame_size * write_pos, cb->GetData() + frame_size * _local_frame_offset, frame_size * cbr);
					_local_frame_offset += cbr;
					write_pos += cbr;
					buffer->FramesUsed() += cbr;
					free_space -= cbr;
					_frames_pending -= cbr;
					if (_local_frame_offset == cb->FramesUsed()) {
						_frames.RemoveFirst();
						_local_frame_offset = 0;
					}
				}
				return true;
			}
		};
		class MediaFoundationEncoder : public IAudioEncoder
		{
			SafePointer<IAudioCodec> _parent;
			string _format;
			StreamDesc _encoded, _internal, _outer;
			SafePointer<Media::AudioTrackFormatDesc> _track_desc;
			IMFTransform * _transform;
			IMFMediaBuffer * _buffer;
			IMFSample * _sample;
			uint _channel_layout, _avg_bytes_per_second, _aac_profile;
			uint _frames_pending, _frames_feed, _frames_read;
			uint _packet_frame_size;
			uint64 _bytes_produced, _max_packet_size;
			Format::alac_spec_config _alac_config;
			bool _eos_pending;
			Array<Media::PacketBuffer> _packets;

			void _select_formats(const string & format, const StreamDesc & desc)
			{
				if (format == AudioFormatAAC) {
					_encoded.Format = SampleFormat::S16_snorm;
					if (desc.ChannelCount > 1) _encoded.ChannelCount = 2; else _encoded.ChannelCount = 1;
					if (desc.FramesPerSecond <= 44100) _encoded.FramesPerSecond = 44100; else _encoded.FramesPerSecond = 48000;
					_internal = _encoded;
					_outer = desc;
				} else if (format == AudioFormatMP3) {
					_encoded.Format = SampleFormat::Invalid;
					if (desc.ChannelCount > 1) _encoded.ChannelCount = 2; else _encoded.ChannelCount = 1;
					if (desc.FramesPerSecond <= 32000) _encoded.FramesPerSecond = 32000;
					else if (desc.FramesPerSecond <= 44100) _encoded.FramesPerSecond = 44100;
					else _encoded.FramesPerSecond = 48000;
					_internal.Format = SampleFormat::S16_snorm;
					_internal.ChannelCount = _encoded.ChannelCount;
					_internal.FramesPerSecond = _encoded.FramesPerSecond;
					_outer = desc;
				} else if (format == AudioFormatAppleLossless) {
					if (SampleFormatBitSize(desc.Format) <= 16) _encoded.Format = SampleFormat::S16_snorm;
					else if (SampleFormatBitSize(desc.Format) <= 24) _encoded.Format = SampleFormat::S24_snorm;
					else _encoded.Format = SampleFormat::S32_snorm;
					_encoded.FramesPerSecond = desc.FramesPerSecond;
					_encoded.ChannelCount = desc.ChannelCount;
					_internal = _encoded;
					_outer = desc;
				} else throw InvalidFormatException();
			}
			void _transform_recreate(void)
			{
				if (_transform) _transform->Release();
				_transform = 0;
				if (_format == AudioFormatMP3) {
					if (CoCreateInstance(CLSID_MP3ACMCodecWrapper, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else if (_format == AudioFormatAAC) {
					if (CoCreateInstance(CLSID_AACMFTEncoder, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else if (_format == AudioFormatAppleLossless) {
					if (CoCreateInstance(CLSID_CMSALACEncMFT, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_transform)) != S_OK) throw Exception();
				} else throw InvalidFormatException();
			}
			void _setup_inputs_and_output_format(void)
			{
				if (_format == AudioFormatMP3) {
					IMFMediaType * type;
					uint mp3_bitrate = _avg_bytes_per_second ? _avg_bytes_per_second * 8 : 0;
					if (!mp3_bitrate) mp3_bitrate = (_internal.ChannelCount > 1) ? 320000 : 128000;
					if (mp3_bitrate <= 32000) mp3_bitrate = 32000;
					else if (mp3_bitrate <= 40000) mp3_bitrate = 40000;
					else if (mp3_bitrate <= 48000) mp3_bitrate = 48000;
					else if (mp3_bitrate <= 56000) mp3_bitrate = 56000;
					else if (mp3_bitrate <= 64000) mp3_bitrate = 64000;
					else if (mp3_bitrate <= 80000) mp3_bitrate = 80000;
					else if (mp3_bitrate <= 96000) mp3_bitrate = 96000;
					else if (mp3_bitrate <= 112000) mp3_bitrate = 112000;
					else if (mp3_bitrate <= 128000) mp3_bitrate = 128000;
					else if (mp3_bitrate <= 160000) mp3_bitrate = 160000;
					else if (mp3_bitrate <= 192000) mp3_bitrate = 192000;
					else if (mp3_bitrate <= 224000) mp3_bitrate = 224000;
					else if (mp3_bitrate <= 256000) mp3_bitrate = 256000;
					else mp3_bitrate = 320000;
					DataBlock user_data(0x20);
					user_data.SetLength(12);
					uint index = 0;
					while (true) {
						if (_transform->GetOutputAvailableType(0, index, &type) != S_OK) throw Exception();
						UINT32 fps, channels, bitrate;
						if (type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &fps) != S_OK) { type->Release(); throw Exception(); }
						if (type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels) != S_OK) { type->Release(); throw Exception(); }
						if (type->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bitrate) != S_OK) { type->Release(); throw Exception(); }
						if (fps == _internal.FramesPerSecond && channels == _internal.ChannelCount && bitrate == mp3_bitrate / 8) {
							if (type->GetBlob(MF_MT_USER_DATA, user_data.GetBuffer(), 12, 0) != S_OK) { type->Release(); throw Exception(); }
							if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
							type->Release();
							break;
						}
						type->Release();
						index++;
					}
					_packet_frame_size = 1152;
					if (_transform->GetInputAvailableType(0, 0, &type) != S_OK) throw Exception();
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					uint cl;
					if (_encoded.ChannelCount == 1) cl = ChannelLayoutCenter;
					else cl = ChannelLayoutLeft | ChannelLayoutRight;
					_track_desc = new Media::AudioTrackFormatDesc(AudioFormatMP3, _encoded, cl);
					_channel_layout = cl;
				} else if (_format == AudioFormatAAC) {
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					uint aac_profile = _aac_profile ? _aac_profile : 0x2B;
					uint aac_bps = _avg_bytes_per_second ? _avg_bytes_per_second : 16000;
					if (aac_bps <= 12000) aac_bps = 12000;
					else if (aac_bps <= 16000) aac_bps = 16000;
					else if (aac_bps <= 20000) aac_bps = 20000;
					else aac_bps = 24000;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, aac_bps) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, aac_profile) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					_packet_frame_size = 1024;
					uint cl;
					if (_encoded.ChannelCount == 1) cl = ChannelLayoutCenter;
					else cl = ChannelLayoutLeft | ChannelLayoutRight;
					_track_desc = new Media::AudioTrackFormatDesc(AudioFormatAAC, _encoded, cl);
					_channel_layout = cl;
					SafePointer<DataBlock> magic = new DataBlock(0x10);
					if (_transform->GetOutputCurrentType(0, &type) != S_OK) throw Exception();
					UINT32 magic_size;
					if (type->GetBlobSize(MF_MT_USER_DATA, &magic_size) != S_OK) { type->Release(); throw Exception(); }
					try { magic->SetLength(magic_size); } catch (...) { type->Release(); throw; }
					if (type->GetBlob(MF_MT_USER_DATA, magic->GetBuffer(), magic->Length(), 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					if (magic->Length() < 12) throw Exception();
					for (int i = 0; i < 12; i++) magic->RemoveFirst();
					_track_desc->SetCodecMagic(magic);
				} else if (_format == AudioFormatAppleLossless) {
					_track_desc = new Media::AudioTrackFormatDesc(AudioFormatAppleLossless, _encoded, _channel_layout);
					IMFMediaType * type;
					if (MFCreateMediaType(&type) != S_OK) throw Exception();
					if (type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, SampleFormatBitSize(_internal.Format)) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, StreamFrameByteSize(_internal)) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetInputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					if (_transform->GetOutputAvailableType(0, 0, &type) != S_OK) throw Exception();
					if (type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, SampleFormatBitSize(_internal.Format)) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _internal.ChannelCount) != S_OK) { type->Release(); throw Exception(); }
					if (type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _internal.FramesPerSecond) != S_OK) { type->Release(); throw Exception(); }
					if (_transform->SetOutputType(0, type, 0) != S_OK) { type->Release(); throw Exception(); }
					type->Release();
					_packet_frame_size = 4096;
					_alac_config.com_version = 0;
					_alac_config.bits_per_sample = SampleFormatBitSize(_internal.Format);
					_alac_config.bp = 40;
					_alac_config.mb = 10;
					_alac_config.kb = 14;
					_alac_config.channel_count = _internal.ChannelCount;
					_alac_config.max_run = 0xFF00;
					_alac_config.frames_per_second = Network::InverseEndianess(uint32(_internal.FramesPerSecond));
					_alac_config.frames_per_packet = Network::InverseEndianess(uint32(_packet_frame_size));
					_alac_config.max_packet_size = _alac_config.ave_bit_rate = 0;
					SafePointer<DataBlock> magic = new DataBlock(sizeof(_alac_config));
					magic->Append(reinterpret_cast<uint8 *>(&_alac_config), sizeof(_alac_config));
					_track_desc->SetCodecMagic(magic);
				}
			}
			bool _read_packets(void) noexcept
			{
				while (true) {
					if (_buffer) {
						if (_buffer->SetCurrentLength(0) != S_OK) return false;
					} else {
						MFT_OUTPUT_STREAM_INFO info;
						if (_transform->GetOutputStreamInfo(0, &info) != S_OK) return false;
						if (info.cbSize) if (MFCreateMemoryBuffer(info.cbSize, &_buffer) != S_OK) return false;
					}
					if (!_sample) {
						if (MFCreateSample(&_sample) != S_OK) return false;
						if (_buffer) if (_sample->AddBuffer(_buffer) != S_OK) { _sample->Release(); _sample = 0; return false; }
					}
					DWORD status;
					MFT_OUTPUT_DATA_BUFFER buffer;
					ZeroMemory(&buffer, sizeof(buffer));
					buffer.pSample = _sample;
					auto result = _transform->ProcessOutput(0, 1, &buffer, &status);
					if (result == MF_E_TRANSFORM_NEED_MORE_INPUT) {
						if (_eos_pending) {
							Media::PacketBuffer packet;
							packet.PacketDataActuallyUsed = 0;
							packet.PacketIsKey = true;
							packet.PacketDecodeTime = packet.PacketRenderTime = _frames_read;
							packet.PacketRenderDuration = 0;
							try { _packets << packet; } catch (...) { return false; }
							if (_format == AudioFormatAppleLossless) {
								try {
									SafePointer<DataBlock> magic = new DataBlock(sizeof(_alac_config));
									magic->Append(reinterpret_cast<uint8 *>(&_alac_config), sizeof(_alac_config));
									_track_desc->SetCodecMagic(magic);
								} catch (...) {}
							}
							_frames_pending = 0;
							_frames_feed = 0;
							_frames_read = 0;
							_eos_pending = false;
						}
						break;
					} else if (result != S_OK) return false;
					_frames_pending = 0;
					auto frame_base = _frames_read;
					_frames_read += _packet_frame_size;
					if (_frames_read > _frames_feed) _frames_read = _frames_feed;
					auto frame_count = _frames_read - frame_base;
					DWORD packet_size;
					LPBYTE data_ptr;
					Media::PacketBuffer packet;
					IMFMediaBuffer * media_buffer;
					if (!_buffer) {
						if (_sample->GetBufferByIndex(0, &media_buffer) != S_OK) return false;
						_sample->RemoveAllBuffers();
					} else media_buffer = _buffer;
					if (media_buffer->GetCurrentLength(&packet_size) != S_OK) { if (!_buffer) media_buffer->Release(); return false; }
					try { packet.PacketData = new DataBlock(packet_size); packet.PacketData->SetLength(packet_size); } catch (...) { if (!_buffer) media_buffer->Release(); return false; }
					packet.PacketDataActuallyUsed = packet_size;
					if (media_buffer->Lock(&data_ptr, 0, 0) != S_OK) { if (!_buffer) media_buffer->Release(); return false; }
					MemoryCopy(packet.PacketData->GetBuffer(), data_ptr, packet_size);
					media_buffer->Unlock();
					if (!_buffer) media_buffer->Release();
					packet.PacketIsKey = true;
					packet.PacketDecodeTime = packet.PacketRenderTime = frame_base;
					packet.PacketRenderDuration = frame_count;
					_bytes_produced += packet_size;
					if (packet_size > _max_packet_size) _max_packet_size = packet_size;
					_alac_config.max_packet_size = Network::InverseEndianess(uint32(_max_packet_size));
					_alac_config.ave_bit_rate = Network::InverseEndianess(uint32(_bytes_produced * 8 * _internal.FramesPerSecond / _frames_feed));
					try { _packets << packet; } catch (...) { return false; }
				}
				return true;
			}
		public:
			MediaFoundationEncoder(IAudioCodec * codec, const string & format, const StreamDesc & desc, uint num_options, const uint * options) : _packets(0x10)
			{
				_parent.SetRetain(codec);
				_format = format;
				if (MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET) != S_OK) throw Exception();
				try {
					_channel_layout = _avg_bytes_per_second = _aac_profile = 0;
					for (uint i = 0; i < num_options; i++) {
						if (options[2 * i] == Media::MediaEncoderChannelLayout) _channel_layout = options[2 * i + 1];
						else if (options[2 * i] == Media::MediaEncoderSuggestedBytesPerSecond) _avg_bytes_per_second = options[2 * i + 1];
						else if (options[2 * i] == Media::MediaEncoderAACProfile) _aac_profile = options[2 * i + 1];
					}
					_transform = 0;
					_buffer = 0;
					_sample = 0;
					_frames_pending = _frames_feed = _frames_read = 0;
					_bytes_produced = _max_packet_size = 0;
					_eos_pending = false;
					_select_formats(format, desc);
					_transform_recreate();
					_setup_inputs_and_output_format();
				} catch (...) { if (_transform) _transform->Release(); _transform = 0; MFShutdown(); throw; }
			}
			virtual ~MediaFoundationEncoder(void) override
			{
				if (_transform) _transform->Release();
				if (_buffer) _buffer->Release();
				if (_sample) _sample->Release();
				MFShutdown();
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _outer; }
			virtual uint GetChannelLayout(void) const noexcept override { return _channel_layout; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Encoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _encoded; }
			virtual bool Reset(void) noexcept override
			{
				try {
					_packets.Clear();
					_frames_pending = _frames_feed = _frames_read = 0;
					_bytes_produced = _max_packet_size = 0;
					_eos_pending = false;
					if (_transform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0) != S_OK) return false;
					return true;
				} catch (...) { return false; }
			}
			virtual int GetPendingPacketsCount(void) const noexcept override { return _packets.Length(); }
			virtual int GetPendingFramesCount(void) const noexcept override { return _frames_pending; }
			virtual const Media::AudioTrackFormatDesc & GetFullEncodedDescriptor(void) const noexcept override { return *_track_desc; }
			virtual const DataBlock * GetCodecMagic(void) noexcept override { return _track_desc->GetCodecMagic(); }
			virtual bool SupplyFrames(const WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != _outer.Format) return false;
				if (desc.ChannelCount != _outer.ChannelCount) return false;
				if (desc.FramesPerSecond != _outer.FramesPerSecond) return false;
				if (!buffer->FramesUsed()) return false;
				SafePointer<WaveBuffer> local_buffer;
				const void * raw_data;
				uint data_length;
				uint frame_length;
				if (desc.Format == _internal.Format && desc.ChannelCount == _internal.ChannelCount && desc.FramesPerSecond == _internal.FramesPerSecond) {
					raw_data = buffer->GetData();
					data_length = buffer->GetUsedSizeInBytes();
					frame_length = buffer->FramesUsed();
				} else {
					try {
						if (desc.FramesPerSecond != _internal.FramesPerSecond) {
							local_buffer = buffer->ConvertFrameRate(_internal.FramesPerSecond);
							if (desc.ChannelCount != _internal.ChannelCount) local_buffer = local_buffer->ReallocateChannels(_internal.ChannelCount);
							if (desc.Format != _internal.Format) local_buffer = local_buffer->ConvertFormat(_internal.Format);
						} else if (desc.ChannelCount != _internal.ChannelCount) {
							local_buffer = buffer->ReallocateChannels(_internal.ChannelCount);
							if (desc.Format != _internal.Format) local_buffer = local_buffer->ConvertFormat(_internal.Format);
						} else {
							local_buffer = buffer->ConvertFormat(_internal.Format);
						}
						raw_data = local_buffer->GetData();
						data_length = local_buffer->GetUsedSizeInBytes();
						frame_length = local_buffer->FramesUsed();
					} catch (...) { return false; }
				}
				IMFMediaBuffer * media;
				IMFSample * sample;
				LPBYTE data_ptr;
				if (MFCreateMemoryBuffer(data_length, &media) != S_OK) return false;
				if (media->SetCurrentLength(data_length) != S_OK) { media->Release(); return false; }
				if (media->Lock(&data_ptr, 0, 0) != S_OK) { media->Release(); return false; }
				MemoryCopy(data_ptr, raw_data, data_length);
				media->Unlock();
				if (MFCreateSample(&sample) != S_OK) { media->Release(); return false; }
				if (sample->AddBuffer(media) != S_OK) { media->Release(); sample->Release(); return false; }
				media->Release();
				if (sample->SetSampleTime(uint64(_frames_feed) * 10000000 / _internal.FramesPerSecond) != S_OK) { sample->Release(); return false; }
				if (sample->SetSampleDuration(uint64(frame_length) * 10000000 / _internal.FramesPerSecond) != S_OK) { sample->Release(); return false; }
				if (_transform->ProcessInput(0, sample, 0) != S_OK) { sample->Release(); return false; }
				sample->Release();
				_frames_feed += frame_length;
				_frames_pending += frame_length;
				return _read_packets();
			}
			virtual bool SupplyEndOfStream(void) noexcept override
			{
				if (_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0) != S_OK) return false;
				if (_transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0) != S_OK) return false;
				_eos_pending = true;
				return _read_packets();
			}
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept override
			{
				if (!_packets.Length()) return false;
				packet = _packets.FirstElement();
				_packets.RemoveFirst();
				return true;
			}
		};
		class MediaFoundationCodec : public IAudioCodec
		{
		public:
			MediaFoundationCodec(void) {}
			virtual ~MediaFoundationCodec(void) override {}
			virtual bool CanEncode(const string & format) const noexcept override
			{
				if (format == AudioFormatMP3 || format == AudioFormatAAC || format == AudioFormatAppleLossless) return true;
				else return false;
			}
			virtual bool CanDecode(const string & format) const noexcept override
			{
				if (format == AudioFormatMP3 || format == AudioFormatAAC || format == AudioFormatFreeLossless || format == AudioFormatAppleLossless) return true;
				else return 0;
			}
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(4);
				result->Append(AudioFormatMP3);
				result->Append(AudioFormatAAC);
				result->Append(AudioFormatAppleLossless);
				result->Retain();
				return result;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(4);
				result->Append(AudioFormatMP3);
				result->Append(AudioFormatAAC);
				result->Append(AudioFormatFreeLossless);
				result->Append(AudioFormatAppleLossless);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Media Foundation Codec"; }
			virtual IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) noexcept override { try { return new MediaFoundationDecoder(this, format, desired_desc); } catch (...) { return 0; } }
			virtual IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options, const uint * options) noexcept override { try { return new MediaFoundationEncoder(this, format, desc, num_options, options); } catch (...) { return 0; } }
		};

		void _select_stream_format(StreamDesc & format, WAVEFORMATEX * wave, uint & channel_layout)
		{
			WAVEFORMATEXTENSIBLE * wave_ex = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(wave);
			format.ChannelCount = wave->nChannels;
			format.FramesPerSecond = wave->nSamplesPerSec;
			if (wave->wFormatTag == WAVE_FORMAT_PCM) {
				if (wave->wBitsPerSample == 8) format.Format = SampleFormat::S8_unorm;
				else if (wave->wBitsPerSample == 16) format.Format = SampleFormat::S16_snorm;
				else if (wave->wBitsPerSample == 24) format.Format = SampleFormat::S24_snorm;
				else if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_snorm;
				else throw Exception();
			} else if (wave->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
				if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_float;
				else if (wave->wBitsPerSample == 64) format.Format = SampleFormat::S64_float;
				else throw Exception();
			} else if (wave->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
				channel_layout = wave_ex->dwChannelMask;
				if (wave_ex->Samples.wValidBitsPerSample != wave->wBitsPerSample) throw Exception();
				if (wave_ex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
					if (wave->wBitsPerSample == 8) format.Format = SampleFormat::S8_unorm;
					else if (wave->wBitsPerSample == 16) format.Format = SampleFormat::S16_snorm;
					else if (wave->wBitsPerSample == 24) format.Format = SampleFormat::S24_snorm;
					else if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_snorm;
					else throw Exception();
				} else if (wave_ex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
					if (wave->wBitsPerSample == 32) format.Format = SampleFormat::S32_float;
					else if (wave->wBitsPerSample == 64) format.Format = SampleFormat::S64_float;
					else throw Exception();
				} else throw Exception();
			} else throw Exception();
		}
		void _select_custom_format(StreamDesc & format, WAVEFORMATEX * wave, DWORD & convert)
		{
			wave->wFormatTag = WAVE_FORMAT_PCM;
			wave->wBitsPerSample = 32;
			wave->nBlockAlign = 4 * wave->nChannels;
			wave->nAvgBytesPerSec = wave->nBlockAlign * wave->nSamplesPerSec;
			wave->cbSize = 0;
			format.Format = SampleFormat::S32_snorm;
			format.ChannelCount = wave->nChannels;
			format.FramesPerSecond = wave->nSamplesPerSec;
			convert = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
		}
		struct _dispatch_task {
			SafePointer<WaveBuffer> buffer;
			SafePointer<Semaphore> open;
			SafePointer<IDispatchTask> task;
			bool * status;
			_dispatch_task * next;
		};
		class _device_status_notification : public IMMNotificationClient
		{
			IMMDeviceEnumerator * _enumerator;
			string _dev_id;
			HANDLE _event_open;
			ULONG _ref_cnt;
		public:
			_device_status_notification(IMMDeviceEnumerator * enumerator, IMMDevice * device, HANDLE event_open) : _event_open(event_open)
			{
				LPWSTR dev_uid;
				if (device->GetId(&dev_uid) != S_OK) throw Exception();
				try { _dev_id = string(dev_uid); } catch (...) { CoTaskMemFree(dev_uid); throw; }
				CoTaskMemFree(dev_uid);
				if (enumerator->RegisterEndpointNotificationCallback(this) != S_OK) throw Exception();
				_enumerator = enumerator;
				_enumerator->AddRef();
				_ref_cnt = 1;
			}
			~_device_status_notification(void) { _enumerator->UnregisterEndpointNotificationCallback(this); _enumerator->Release(); }
			virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override
			{
				if (riid == __uuidof(IMMNotificationClient)) {
					*ppvObject = static_cast<IMMNotificationClient *>(this);
					AddRef();
					return S_OK;
				} else if (riid == IID_IUnknown) {
					*ppvObject = static_cast<IUnknown *>(this);
					AddRef();
					return S_OK;
				} else return E_NOINTERFACE;
			}
			virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
			virtual ULONG STDMETHODCALLTYPE Release(void) override
			{
				auto result = InterlockedDecrement(&_ref_cnt);
				if (!result) delete this;
				return result;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
			{
				if (dwNewState != DEVICE_STATE_ACTIVE && pwstrDeviceId == _dev_id) SetEvent(_event_open);
				return S_OK;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override { return S_OK; }
			virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override
			{
				if (pwstrDeviceId == _dev_id) SetEvent(_event_open);
				return S_OK;
			}
			virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override { return S_OK; }
			virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override { return S_OK; }
		};
		class CoreAudioOutputDevice : public IAudioOutputDevice
		{
			StreamDesc format;
			IAudioClient * client;
			IAudioRenderClient * render;
			IAudioStreamVolume * volume;
			SafePointer<Thread> thread;
			SafePointer<Semaphore> task_count;
			SafePointer<Semaphore> access;
			_dispatch_task * task_first;
			_dispatch_task * task_last;
			HANDLE event;
			_device_status_notification * notification;
			bool device_dead;
			string device_uid;
			uint channel_layout;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioOutputDevice *>(arg);
				SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
				if (device->client->GetBufferSize(&buffer_size) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioRenderClient, reinterpret_cast<void **>(&device->render)) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioStreamVolume, reinterpret_cast<void **>(&device->volume)) != S_OK) {
					device->render->Release();
					device->render = 0;
					SetEvent(device->event);
					return 1;
				}
				SetEvent(device->event);
				while (true) {
					_dispatch_task * current = 0;
					device->task_count->Wait();
					device->access->Wait();
					if (device->command) {
						auto command_local = device->command;
						auto task_local = device->task_first;
						device->task_first = device->task_last = 0;
						if (device->command & 2) device->command &= ~2;
						device->access->Open();
						while (task_local) {
							if (task_local->status) *task_local->status = false;
							if (task_local->open) task_local->open->Open();
							if (task_local->task) task_local->task->DoTask(0);
							device->task_count->Wait();
							auto task_delete = task_local;
							task_local = task_local->next;
							delete task_delete;
						}
						ResetEvent(device->event);
						if (command_local & 1) break;
						continue;
					} else {
						current = device->task_first;
						device->task_first = device->task_first->next;
						if (!device->task_first) device->task_last = 0;
					}
					device->access->Open();
					uint64 current_frame = 0;
					bool local_status = true, dead_status = false;
					while (current_frame < current->buffer->FramesUsed()) {
						if (device->device_dead) { dead_status = true; local_status = false; break; }
						WaitForSingleObject(device->event, INFINITE);
						if (device->command) { local_status = false; break; }
						UINT32 padding;
						if (device->client->GetCurrentPadding(&padding) == S_OK) {
							UINT32 frames_write = buffer_size - padding;
							uint64 frames_exists = current->buffer->FramesUsed() - current_frame;
							if (frames_write > frames_exists) frames_write = UINT32(frames_exists);
							BYTE * data;
							if (device->render->GetBuffer(frames_write, &data) != S_OK) { dead_status = true; local_status = false; break; }
							auto frame_size = StreamFrameByteSize(current->buffer->GetFormatDescriptor());
							MemoryCopy(data, current->buffer->GetData() + frame_size * current_frame, frame_size * frames_write);
							if (device->render->ReleaseBuffer(frames_write, 0) != S_OK) { dead_status = true; local_status = false; break; }
							current_frame += frames_write;
						} else { dead_status = true; local_status = false; break; }
					}
					if (current->status) *current->status = local_status;
					if (current->open) current->open->Open();
					if (current->task) current->task->DoTask(0);
					delete current;
					if (dead_status) {
						device->access->Wait();
						device->device_dead = true;
						device->command |= 2;
						device->task_count->Open();
						device->access->Open();
					}
				}
				device->volume->Release();
				device->volume = 0;
				device->render->Release();
				device->render = 0;
				return 0;
			}
			void _append_dispatch(WaveBuffer * buffer, Semaphore * open, IDispatchTask * task, bool * result, int _command)
			{
				if (!_command) {
					access->Wait();
					try {
						_dispatch_task * new_task = new _dispatch_task;
						new_task->buffer.SetRetain(buffer);
						new_task->open.SetRetain(open);
						new_task->task.SetRetain(task);
						new_task->status = result;
						new_task->next = 0;
						if (task_last) {
							task_last->next = new_task;
							task_last = new_task;
						} else {
							task_first = task_last = new_task;
						}
					} catch (...) { access->Open(); throw; }
					task_count->Open();
					access->Open();
				} else {
					access->Wait();
					if (_command & 1 && !(command & 1)) task_count->Open();
					if (_command & 2 && !(command & 2)) task_count->Open();
					command |= _command;
					access->Open();
					SetEvent(event);
				}
			}
		public:
			CoreAudioOutputDevice(IMMDeviceEnumerator * enumerator, IMMDevice * device)
			{
				channel_layout = 0;
				LPWSTR dev_id;
				if (device->GetId(&dev_id) != S_OK) throw Exception();
				try { device_uid = string(dev_id); } catch (...) { CoTaskMemFree(dev_id); throw; }
				CoTaskMemFree(dev_id);
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; render = 0; command = 0;
				device_dead = false;
				task_first = task_last = 0;
				if (client->GetMixFormat(&wave) != S_OK) { client->Release(); throw Exception(); }
				try { _select_stream_format(format, wave, channel_layout); } catch (...) { _select_custom_format(format, wave, convert); }
				if (client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | convert |
					AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED,
					0, 0, wave, 0) != S_OK) { CoTaskMemFree(wave); client->Release(); throw Exception(); }
				CoTaskMemFree(wave);
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { client->Release(); throw Exception(); }
				event = CreateEventW(0, FALSE, FALSE, 0);
				if (!event) { client->Release(); throw Exception(); }
				thread = CreateThread(_dispatch_thread, this);
				if (!thread) { CloseHandle(event); client->Release(); throw Exception(); }
				WaitForSingleObject(event, INFINITE);
				if (!volume || !render) { CloseHandle(event); client->Release(); throw Exception(); }
				try { notification = new _device_status_notification(enumerator, device, event); } catch (...) { notification = 0; }
				if (client->SetEventHandle(event) != S_OK || !notification) {
					if (notification) notification->Release();
					_append_dispatch(0, 0, 0, 0, 1);
					thread->Wait();
					CloseHandle(event);
					client->Release();
					throw Exception();
				}
			}
			virtual ~CoreAudioOutputDevice(void) override
			{
				client->Stop();
				client->Reset();
				_append_dispatch(0, 0, 0, 0, 1);
				thread->Wait();
				CloseHandle(event);
				client->Release();
				notification->Release();
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return format; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceOutput; }
			virtual string GetDeviceIdentifier(void) const override { return device_uid; }
			virtual uint GetChannelLayout(void) const noexcept override { return channel_layout; }
			virtual double GetVolume(void) noexcept override
			{
				float level;
				if (volume->GetChannelVolume(0, &level) == S_OK) return level;
				return 0.0;
			}
			virtual void SetVolume(double _volume) noexcept override
			{
				try {
					UINT32 count;
					if (volume->GetChannelCount(&count) != S_OK) return;
					Array<float> levels(count);
					for (UINT32 i = 0; i < count; i++) levels << float(_volume);
					volume->SetAllVolumes(count, levels.GetBuffer());
				} catch (...) {}
			}
			virtual bool StartProcessing(void) noexcept override
			{
				if (client->Start() != S_OK) return false;
				return true;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (client->Stop() != S_OK) return false;
				return true;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				client->Stop();
				if (client->Reset() != S_OK) return false;
				_append_dispatch(0, 0, 0, 0, 2);
				return true;
			}
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				bool status;
				if (!semaphore || !WriteFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer || !execute_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, write_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer || !open_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, open_on_processed, 0, write_status, 0); } catch (...) { return false; }
				return true;
			}
		};
		class CoreAudioInputDevice : public IAudioInputDevice
		{
			StreamDesc format;
			IAudioClient * client;
			IAudioCaptureClient * capture;
			IAudioStreamVolume * volume;
			SafePointer<Thread> thread;
			SafePointer<Semaphore> task_count;
			SafePointer<Semaphore> access;
			_dispatch_task * task_first;
			_dispatch_task * task_last;
			HANDLE event;
			_device_status_notification * notification;
			bool device_dead;
			string device_uid;
			uint channel_layout;
			volatile uint command; // 1 - terminate, 2 - drain

			static int _dispatch_thread(void * arg)
			{
				UINT32 buffer_size;
				uint8 * buffer_small;
				uint32 buffer_small_unread, buffer_small_size;
				buffer_small_unread = buffer_small_size = 0;
				const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
				const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);
				auto device = reinterpret_cast<CoreAudioInputDevice *>(arg);
				SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
				if (device->client->GetBufferSize(&buffer_size) != S_OK) {
					SetEvent(device->event);
					return 1;
				}
				auto frame_size = StreamFrameByteSize(device->format);
				buffer_small = reinterpret_cast<uint8 *>(malloc(buffer_size * frame_size));
				if (!buffer_small) {
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioCaptureClient, reinterpret_cast<void **>(&device->capture)) != S_OK) {
					free(buffer_small);
					SetEvent(device->event);
					return 1;
				}
				if (device->client->GetService(IID_IAudioStreamVolume, reinterpret_cast<void **>(&device->volume)) != S_OK) {
					free(buffer_small);
					device->capture->Release();
					device->capture = 0;
					SetEvent(device->event);
					return 1;
				}
				SetEvent(device->event);
				while (true) {
					_dispatch_task * current = 0;
					device->task_count->Wait();
					device->access->Wait();
					if (device->command) {
						auto command_local = device->command;
						auto task_local = device->task_first;
						device->task_first = device->task_last = 0;
						if (device->command & 2) device->command &= ~2;
						device->access->Open();
						while (task_local) {
							task_local->buffer->FramesUsed() = 0;
							if (task_local->status) *task_local->status = false;
							if (task_local->open) task_local->open->Open();
							if (task_local->task) task_local->task->DoTask(0);
							device->task_count->Wait();
							auto task_delete = task_local;
							task_local = task_local->next;
							delete task_delete;
						}
						ResetEvent(device->event);
						if (command_local & 1) break;
						continue;
					} else {
						current = device->task_first;
						device->task_first = device->task_first->next;
						if (!device->task_first) device->task_last = 0;
					}
					device->access->Open();
					bool local_status = true, dead_status = false;
					current->buffer->FramesUsed() = 0;
					if (buffer_small_unread < buffer_small_size) {
						uint64 buffer_free = current->buffer->GetSizeInFrames();
						uint32 write = buffer_small_size - buffer_small_unread;
						if (write > buffer_free) write = uint32(buffer_free);
						MemoryCopy(current->buffer->GetData(), buffer_small + frame_size * buffer_small_unread, frame_size * write);
						current->buffer->FramesUsed() = write;
						buffer_small_unread += write;
					}
					while (current->buffer->FramesUsed() < current->buffer->GetSizeInFrames()) {
						if (device->device_dead) { dead_status = true; local_status = false; break; }
						WaitForSingleObject(device->event, INFINITE);
						if (device->command) break;
						UINT32 read_ready;
						if (device->capture->GetNextPacketSize(&read_ready) == S_OK) {
							HRESULT int_status = S_OK;
							while (read_ready) {
								LPBYTE data_ptr;
								UINT32 data_frame_size;
								DWORD data_flags;
								int_status = device->capture->GetBuffer(&data_ptr, &data_frame_size, &data_flags, 0, 0);
								if (int_status != S_OK) { dead_status = true; local_status = false; break; }
								buffer_small_unread = 0;
								buffer_small_size = data_frame_size;
								if (data_flags & AUDCLNT_BUFFERFLAGS_SILENT) ZeroMemory(buffer_small, data_frame_size * frame_size);
								else MemoryCopy(buffer_small, data_ptr, data_frame_size * frame_size);
								int_status = device->capture->ReleaseBuffer(data_frame_size);
								if (int_status != S_OK) { dead_status = true; local_status = false; break; }
								uint64 buffer_free = current->buffer->GetSizeInFrames() - current->buffer->FramesUsed();
								uint32 copy = buffer_small_size;
								if (copy > buffer_free) copy = uint32(buffer_free);
								if (copy) {
									MemoryCopy(current->buffer->GetData() + current->buffer->FramesUsed() * frame_size, buffer_small, copy * frame_size);
									current->buffer->FramesUsed() += copy;
									buffer_small_unread += copy;
								}
								if (buffer_small_unread < buffer_small_size) { int_status = E_UNEXPECTED; break; }
								int_status = device->capture->GetNextPacketSize(&read_ready);
								if (int_status != S_OK) { dead_status = true; local_status = false; break; }
							}
							if (int_status != S_OK) break;
						} else { dead_status = true; local_status = false; break; }
					}
					if (current->status) *current->status = local_status;
					if (current->open) current->open->Open();
					if (current->task) current->task->DoTask(0);
					delete current;
					if (dead_status) {
						device->access->Wait();
						device->device_dead = true;
						device->command |= 2;
						device->task_count->Open();
						device->access->Open();
					}
				}
				device->volume->Release();
				device->volume = 0;
				device->capture->Release();
				device->capture = 0;
				free(buffer_small);
				return 0;
			}
			void _append_dispatch(WaveBuffer * buffer, Semaphore * open, IDispatchTask * task, bool * result, int _command)
			{
				if (!_command) {
					access->Wait();
					try {
						_dispatch_task * new_task = new _dispatch_task;
						new_task->buffer.SetRetain(buffer);
						new_task->open.SetRetain(open);
						new_task->task.SetRetain(task);
						new_task->status = result;
						new_task->next = 0;
						if (task_last) {
							task_last->next = new_task;
							task_last = new_task;
						} else {
							task_first = task_last = new_task;
						}
					} catch (...) { access->Open(); throw; }
					task_count->Open();
					access->Open();
				} else {
					access->Wait();
					if (_command & 1 && !(command & 1)) task_count->Open();
					if (_command & 2 && !(command & 2)) task_count->Open();
					command |= _command;
					access->Open();
					SetEvent(event);
				}
			}
		public:
			CoreAudioInputDevice(IMMDeviceEnumerator * enumerator, IMMDevice * device)
			{
				channel_layout = 0;
				LPWSTR dev_id;
				if (device->GetId(&dev_id) != S_OK) throw Exception();
				try { device_uid = string(dev_id); } catch (...) { CoTaskMemFree(dev_id); throw; }
				CoTaskMemFree(dev_id);
				const IID IID_IAudioClient = __uuidof(IAudioClient);
				if (device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, reinterpret_cast<void **>(&client)) != S_OK) throw Exception();
				WAVEFORMATEX * wave;
				DWORD convert = 0;
				volume = 0; capture = 0; command = 0;
				device_dead = false;
				task_first = task_last = 0;
				if (client->GetMixFormat(&wave) != S_OK) { client->Release(); throw Exception(); }
				try { _select_stream_format(format, wave, channel_layout); } catch (...) { _select_custom_format(format, wave, convert); }
				if (client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | convert |
					AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED,
					0, 0, wave, 0) != S_OK) {
					CoTaskMemFree(wave); client->Release(); throw Exception();
				}
				CoTaskMemFree(wave);
				access = CreateSemaphore(1);
				task_count = CreateSemaphore(0);
				if (!access || !task_count) { client->Release(); throw Exception(); }
				event = CreateEventW(0, FALSE, FALSE, 0);
				if (!event) { client->Release(); throw Exception(); }
				thread = CreateThread(_dispatch_thread, this);
				if (!thread) { CloseHandle(event); client->Release(); throw Exception(); }
				WaitForSingleObject(event, INFINITE);
				if (!volume || !capture) { CloseHandle(event); client->Release(); throw Exception(); }
				try { notification = new _device_status_notification(enumerator, device, event); } catch (...) { notification = 0; }
				if (client->SetEventHandle(event) != S_OK || !notification) {
					if (notification) notification->Release();
					_append_dispatch(0, 0, 0, 0, 1);
					thread->Wait();
					CloseHandle(event);
					client->Release();
					throw Exception();
				}
			}
			virtual ~CoreAudioInputDevice(void) override
			{
				client->Stop();
				client->Reset();
				_append_dispatch(0, 0, 0, 0, 1);
				thread->Wait();
				CloseHandle(event);
				client->Release();
				notification->Release();
			}
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return format; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceInput; }
			virtual string GetDeviceIdentifier(void) const override { return device_uid; }
			virtual uint GetChannelLayout(void) const noexcept override { return channel_layout; }
			virtual double GetVolume(void) noexcept override
			{
				float level;
				if (volume->GetChannelVolume(0, &level) == S_OK) return level;
				return 0.0;
			}
			virtual void SetVolume(double _volume) noexcept override
			{
				try {
					UINT32 count;
					if (volume->GetChannelCount(&count) != S_OK) return;
					Array<float> levels(count);
					for (UINT32 i = 0; i < count; i++) levels << float(_volume);
					volume->SetAllVolumes(count, levels.GetBuffer());
				} catch (...) {}
			}
			virtual bool StartProcessing(void) noexcept override
			{
				if (client->Start() != S_OK) return false;
				return true;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (client->Stop() != S_OK) return false;
				return true;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				client->Stop();
				if (client->Reset() != S_OK) return false;
				_append_dispatch(0, 0, 0, 0, 2);
				return true;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				bool status;
				if (!semaphore || !ReadFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, 0, read_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!buffer || !execute_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, 0, execute_on_processed, read_status, 0); } catch (...) { return false; }
				return true;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, Semaphore * open_on_processed) noexcept override
			{
				if (!buffer || !open_on_processed) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != format.Format || desc.ChannelCount != format.ChannelCount || desc.FramesPerSecond != format.FramesPerSecond) return false;
				try { _append_dispatch(buffer, open_on_processed, 0, read_status, 0); } catch (...) { return false; }
				return true;
			}
		};
		class CoreAudioDeviceFactory : public IAudioDeviceFactory
		{
			class _device_notification : public IMMNotificationClient
			{
				friend class CoreAudioDeviceFactory;
				IMMDeviceEnumerator * enumerator;
				Array<IAudioEventCallback *> callbacks;
				SafePointer<Semaphore> access_sync;
				ULONG _ref_cnt;
			public:
				_device_notification(void) : callbacks(0x10), _ref_cnt(1), enumerator(0) {}
				~_device_notification(void) {}
				void _raise_event(AudioDeviceEvent event, AudioObjectType type, const string & dev_id)
				{
					access_sync->Wait();
					for (auto & callback : callbacks) callback->OnAudioDeviceEvent(event, type, dev_id);
					access_sync->Open();
				}
				virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) override
				{
					if (riid == __uuidof(IMMNotificationClient)) {
						*ppvObject = static_cast<IMMNotificationClient *>(this);
						AddRef();
						return S_OK;
					} else if (riid == IID_IUnknown) {
						*ppvObject = static_cast<IUnknown *>(this);
						AddRef();
						return S_OK;
					} else return E_NOINTERFACE;
				}
				virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return InterlockedIncrement(&_ref_cnt); }
				virtual ULONG STDMETHODCALLTYPE Release(void) override
				{
					auto result = InterlockedDecrement(&_ref_cnt);
					if (!result) delete this;
					return result;
				}
				virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override
				{
					AudioDeviceEvent event;
					AudioObjectType type;
					if (dwNewState == DEVICE_STATE_ACTIVE) event = AudioDeviceEvent::Activated;
					else event = AudioDeviceEvent::Inactivated;
					IMMDevice * device;
					if (enumerator->GetDevice(pwstrDeviceId, &device) != S_OK) return S_OK;
					IMMEndpoint * endpoint;
					if (device->QueryInterface(IID_PPV_ARGS(&endpoint)) != S_OK) { device->Release(); return S_OK; }
					device->Release();
					EDataFlow flow;
					if (endpoint->GetDataFlow(&flow) != S_OK) { endpoint->Release(); return S_OK; }
					endpoint->Release();
					if (flow == eRender) type = AudioObjectType::DeviceOutput;
					else if (flow == eCapture) type = AudioObjectType::DeviceInput;
					else return S_OK;
					try { _raise_event(event, type, pwstrDeviceId); } catch (...) {}
					return S_OK;
				}
				virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override { return S_OK; }
				virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override { return S_OK; }
				virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override
				{
					try {
						if (role == eConsole) {
							if (flow == eRender) _raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceOutput, pwstrDefaultDeviceId);
							else if (flow == eCapture) _raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceInput, pwstrDefaultDeviceId);
						}
					} catch (...) {}
					return S_OK;
				}
				virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override { return S_OK; }
			};

			IMMDeviceEnumerator * enumerator;
			_device_notification * notification;

			Volumes::Dictionary<string, string> * _retr_collection(EDataFlow flow)
			{
				IMMDeviceCollection * collection;
				if (enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection) != S_OK) return 0;
				try {
					SafePointer< Volumes::Dictionary<string, string> > dict = new Volumes::Dictionary<string, string>;
					UINT num_devices;
					if (collection->GetCount(&num_devices) != S_OK) throw Exception();
					for (UINT i = 0; i < num_devices; i++) {
						IMMDevice * device = 0;
						IPropertyStore * store = 0;
						try {
							if (collection->Item(i, &device) != S_OK) throw Exception();
							LPWSTR uid;
							string uid_str;
							if (device->GetId(&uid) != S_OK) throw Exception();
							try { uid_str = string(uid); } catch (...) { CoTaskMemFree(uid); throw; }
							CoTaskMemFree(uid);
							if (device->OpenPropertyStore(STGM_READ, &store) != S_OK) throw Exception();
							PROPVARIANT var;
							PropVariantInit(&var);
							if (store->GetValue(PKEY_Device_FriendlyName, &var) != S_OK) {
								PropVariantClear(&var);
								throw Exception();
							}
							try { dict->Append(uid_str, var.pwszVal); } catch (...) { PropVariantClear(&var); throw; }
							PropVariantClear(&var);
						} catch (...) { if (device) device->Release(); if (store) store->Release(); throw; }
						if (device) device->Release();
						if (store) store->Release();
					}
					dict->Retain();
					collection->Release();
					return dict;
				} catch (...) { collection->Release(); return 0; }
			}
		public:
			CoreAudioDeviceFactory(void)
			{
				notification = new (std::nothrow) _device_notification;
				if (!notification) throw Exception();
				notification->access_sync = CreateSemaphore(1);
				if (!notification->access_sync) { notification->Release(); throw Exception(); }
				const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
				const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
				auto status = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
				if (status != S_OK) { notification->Release(); throw Exception(); }
				notification->enumerator = enumerator;
				enumerator->RegisterEndpointNotificationCallback(notification);
			}
			virtual ~CoreAudioDeviceFactory(void) override
			{
				enumerator->UnregisterEndpointNotificationCallback(notification);
				notification->Release();
				enumerator->Release();
			}
			virtual Volumes::Dictionary<string, string> * GetAvailableOutputDevices(void) noexcept override { return _retr_collection(eRender); }
			virtual Volumes::Dictionary<string, string> * GetAvailableInputDevices(void) noexcept override { return _retr_collection(eCapture); }
			virtual IAudioOutputDevice * CreateOutputDevice(const string & identifier) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDevice(identifier, &device) != S_OK) return 0;
				IAudioOutputDevice * result;
				try { result = new CoreAudioOutputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device) != S_OK) return 0;
				IAudioOutputDevice * result;
				try { result = new CoreAudioOutputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDevice(identifier, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept override
			{
				IMMDevice * device;
				if (enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device) != S_OK) return 0;
				IAudioInputDevice * result;
				try { result = new CoreAudioInputDevice(enumerator, device); } catch (...) { device->Release(); return 0; }
				device->Release();
				return result;
			}
			virtual bool RegisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				notification->access_sync->Wait();
				try {
					for (auto & cb : notification->callbacks) if (cb == callback) { notification->access_sync->Open(); return true; }
					notification->callbacks.Append(callback);
					notification->access_sync->Open();
					return true;
				} catch (...) { notification->access_sync->Open(); return false; }
			}
			virtual bool UnregisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				notification->access_sync->Wait();
				for (int i = 0; i < notification->callbacks.Length(); i++) if (notification->callbacks[i] == callback) {
					notification->callbacks.Remove(i);
					notification->access_sync->Open();
					return true;
				}
				notification->access_sync->Open();
				return false;
			}
		};

		SafePointer<IAudioCodec> _system_codec;
		IAudioCodec * InitializeSystemCodec(void)
		{
			if (!_system_codec) {
				_system_codec = new MediaFoundationCodec;
				RegisterCodec(_system_codec);
			}
			return _system_codec;
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return new CoreAudioDeviceFactory(); }
		void SystemBeep(void) { MessageBeep(0); }
	}
}