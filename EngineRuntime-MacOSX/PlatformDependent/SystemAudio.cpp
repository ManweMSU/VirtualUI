#include "../Interfaces/SystemAudio.h"

#include "../Interfaces/Socket.h"

#include <AudioToolbox/AudioToolbox.h>

namespace Engine
{
	namespace Audio
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(flac_stream_info)
				uint16 min_block_size;
				uint16 max_block_size;
				uint16 min_frame_size_lo;
				uint8 min_frame_size_hi;
				uint16 max_frame_size_lo;
				uint8 max_frame_size_hi;
				uint16 frame_rate_lo;
				uint8 frame_rate_hi_num_channels_bit_per_sample_lo;
				uint8 bit_per_sample_hi_num_samples_lo;
				uint32 num_samples_hi;
				uint64 hash_lo;
				uint64 hash_hi;
			ENGINE_END_PACKED_STRUCTURE
		}

		void CoreAudioProduceStreamDescriptor(AudioStreamBasicDescription & base, StreamDesc & desc)
		{
			desc.Format = SampleFormat::S32_snorm;
			desc.ChannelCount = base.mChannelsPerFrame;
			desc.FramesPerSecond = base.mSampleRate;
			if (base.mFormatID == kAudioFormatLinearPCM) {
				if (base.mFormatFlags & kLinearPCMFormatFlagIsFloat) {
					if (base.mBitsPerChannel == 32) desc.Format = SampleFormat::S32_float;
					else if (base.mBitsPerChannel == 64) desc.Format = SampleFormat::S64_float;
				} else {
					if (base.mBitsPerChannel == 32) desc.Format = SampleFormat::S32_snorm;
					else if (base.mBitsPerChannel == 24) desc.Format = SampleFormat::S24_snorm;
					else if (base.mBitsPerChannel == 16) desc.Format = SampleFormat::S16_snorm;
					else if (base.mBitsPerChannel == 8) desc.Format = SampleFormat::S8_snorm;
				}
			}
			base.mFormatID = kAudioFormatLinearPCM;
			base.mFormatFlags = kLinearPCMFormatFlagIsPacked;
			if (IsFloatingPointFormat(desc.Format)) base.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
			else base.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
			base.mBitsPerChannel = SampleFormatBitSize(desc.Format);
			base.mBytesPerFrame = StreamFrameByteSize(desc);
			base.mBytesPerPacket = base.mBytesPerFrame;
			base.mFramesPerPacket = 1;
			base.mSampleRate = desc.FramesPerSecond;
		}
		uint CoreAudioGetChannelLayout(AudioDeviceID device, AudioUnitScope scope, AudioUnitElement element)
		{
			AudioUnit unit;
			AudioComponentDescription unit_desc;
			unit_desc.componentType = kAudioUnitType_Output;
			unit_desc.componentSubType = kAudioUnitSubType_HALOutput;
			unit_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
			unit_desc.componentFlags = 0;
			unit_desc.componentFlagsMask = 0;
			auto com = AudioComponentFindNext(0, &unit_desc);
			if (com) {
				if (AudioComponentInstanceNew(com, &unit)) return 0;
				AudioUnitSetProperty(unit, kAudioOutputUnitProperty_CurrentDevice, scope, element, &device, sizeof(device));
				UInt32 size;
				::Boolean writable;
				if (AudioUnitGetPropertyInfo(unit, kAudioUnitProperty_AudioChannelLayout, scope, element, &size, &writable)) {
					AudioComponentInstanceDispose(unit);
					return 0;
				}
				AudioChannelLayout * layout = reinterpret_cast<AudioChannelLayout *>(malloc(size));
				if (!layout) {
					AudioComponentInstanceDispose(unit);
					return 0;
				}
				if (AudioUnitGetProperty(unit, kAudioUnitProperty_AudioChannelLayout, scope, element, layout, &size)) {
					free(layout);
					AudioComponentInstanceDispose(unit);
					return 0;
				}
				AudioComponentInstanceDispose(unit);
				uint result = 0;
				if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelDescriptions) {
					for (int i = 0; i < layout->mNumberChannelDescriptions; i++) {
						uint label = 0;
						if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_Left) label = ChannelLayoutLeft;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_Right) label = ChannelLayoutRight;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_Center) label = ChannelLayoutCenter;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_LFEScreen) label = ChannelLayoutLowFrequency;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_LeftSurround) label = ChannelLayoutBackLeft;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_RightSurround) label = ChannelLayoutBackRight;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_CenterSurround) label = ChannelLayoutBackCenter;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_LeftSurroundDirect) label = ChannelLayoutSideLeft;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_RightSurroundDirect) label = ChannelLayoutSideRight;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_VerticalHeightLeft) label = ChannelLayoutTopFrontLeft;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_VerticalHeightRight) label = ChannelLayoutTopFrontRight;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_VerticalHeightCenter) label = ChannelLayoutTopFrontCenter;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_TopBackLeft) label = ChannelLayoutTopBackLeft;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_TopBackRight) label = ChannelLayoutTopBackRight;
						else if (layout->mChannelDescriptions[i].mChannelLabel == kAudioChannelLabel_TopBackCenter) label = ChannelLayoutTopBackCenter;
						if (label && label > result) result |= label; else { result = 0; break; }
					}
				} else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_UseChannelBitmap) {
					result = layout->mChannelBitmap;
				} else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Mono) {
					result = ChannelLayoutCenter;
				} else if (layout->mChannelLayoutTag == kAudioChannelLayoutTag_Stereo | layout->mChannelLayoutTag == kAudioChannelLayoutTag_StereoHeadphones) {
					result = ChannelLayoutLeft | ChannelLayoutRight;
				}
				free(layout);
				return result;
			} else return 0;
		}
		class CoreAudioBufferWrapper : public Object
		{
		public:
			AudioQueueBufferRef buffer;
			SafePointer<WaveBuffer> store_at;
			bool * write_status;
			SafePointer<Semaphore> open;
			SafePointer<IDispatchTask> exec;

			CoreAudioBufferWrapper(void) {};
			virtual ~CoreAudioBufferWrapper(void) override {}
			void Finalize(AudioQueueRef queue)
			{
				if (store_at) {
					store_at->FramesUsed() = buffer->mAudioDataByteSize / StreamFrameByteSize(store_at->GetFormatDescriptor());
					MemoryCopy(store_at->GetData(), buffer->mAudioData, buffer->mAudioDataByteSize);
				}
				OSStatus status = AudioQueueFreeBuffer(queue, buffer);
				if (write_status) *write_status = status == 0;
				if (open) open->Open();
				if (exec) exec->DoTask(0);
			}
		};

		class CoreAudioDecoder : public IAudioDecoder
		{
			SafePointer<IAudioCodec> _parent;
			string _format;
			StreamDesc _output, _internal, _encoded;
			uint _channel_layout;
			SafePointer<DataBlock> _magic;
			AudioCodec _codec;
			uint _frames_pending;
			uint _local_frame_offset;
			uint _packet_size;
			ObjectArray<WaveBuffer> _frames;

			void _init_decoder(void)
			{
				AudioComponentDescription desc;
				uint aac_object_type = 0;
				if (_format == AudioFormatMP3) {
					MemoryCopy(&desc.componentSubType, "3pm.", 4);
				} else if (_format == AudioFormatAAC) {
					if (!_magic || _magic->Length() < 2) throw InvalidArgumentException();
					aac_object_type = (_magic->ElementAt(0) & 0xF8) >> 3;
					MemoryCopy(&desc.componentSubType, " caa", 4);
				} else if (_format == AudioFormatAppleLossless) {
					if (!_magic) throw InvalidArgumentException();
					MemoryCopy(&desc.componentSubType, "cala", 4);
				} else if (_format == AudioFormatFreeLossless) {
					if (!_magic || _magic->Length() < sizeof(Format::flac_stream_info)) throw InvalidArgumentException();
					MemoryCopy(&desc.componentSubType, "calf", 4);
				} else throw InvalidFormatException();
				MemoryCopy(&desc.componentType, "ceda", 4);
				MemoryCopy(&desc.componentManufacturer, "lppa", 4);
				desc.componentFlags = 0;
				desc.componentFlagsMask = 0;
				auto com = AudioComponentFindNext(0, &desc);
				if (!com) throw Exception();
				if (AudioComponentInstanceNew(com, &_codec)) throw Exception();
				const void * magic = 0;
				uint32 magic_length = 0;
				AudioStreamBasicDescription in, out;
				if (_format == AudioFormatMP3) {
					_internal.Format = SampleFormat::S32_float;
					_internal.ChannelCount = _encoded.ChannelCount;
					_internal.FramesPerSecond = _encoded.FramesPerSecond;
					out.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
					out.mBytesPerFrame = StreamFrameByteSize(_internal);
					out.mBytesPerPacket = out.mBytesPerFrame;
					out.mChannelsPerFrame = _internal.ChannelCount;
					out.mFormatFlags = kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
					out.mFormatID = kAudioFormatLinearPCM;
					out.mFramesPerPacket = 1;
					out.mReserved = 0;
					out.mSampleRate = _internal.FramesPerSecond;
					in.mBitsPerChannel = 0;
					in.mBytesPerFrame = 0;
					in.mBytesPerPacket = 0;
					in.mChannelsPerFrame = _encoded.ChannelCount;
					in.mFormatFlags = 0;
					in.mFormatID = kAudioFormatMPEGLayer3;
					in.mFramesPerPacket = 0;
					in.mReserved = 0;
					in.mSampleRate = _encoded.FramesPerSecond;
				} else if (_format == AudioFormatAAC) {
					_internal.Format = SampleFormat::S16_snorm;
					_internal.ChannelCount = _encoded.ChannelCount;
					_internal.FramesPerSecond = _encoded.FramesPerSecond;
					out.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
					out.mBytesPerFrame = StreamFrameByteSize(_internal);
					out.mBytesPerPacket = out.mBytesPerFrame;
					out.mChannelsPerFrame = _internal.ChannelCount;
					out.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
					out.mFormatID = kAudioFormatLinearPCM;
					out.mFramesPerPacket = 1;
					out.mReserved = 0;
					out.mSampleRate = _internal.FramesPerSecond;
					in.mBitsPerChannel = 0;
					in.mBytesPerFrame = 0;
					in.mBytesPerPacket = 0;
					in.mChannelsPerFrame = _encoded.ChannelCount;
					in.mFormatFlags = aac_object_type;
					in.mFormatID = kAudioFormatMPEG4AAC;
					in.mFramesPerPacket = 0;
					in.mReserved = 0;
					in.mSampleRate = _encoded.FramesPerSecond;
				} else if (_format == AudioFormatAppleLossless) {
					_internal.Format = _encoded.Format;
					_internal.ChannelCount = _encoded.ChannelCount;
					_internal.FramesPerSecond = _encoded.FramesPerSecond;
					out.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
					out.mBytesPerFrame = StreamFrameByteSize(_internal);
					out.mBytesPerPacket = out.mBytesPerFrame;
					out.mChannelsPerFrame = _internal.ChannelCount;
					out.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
					out.mFormatID = kAudioFormatLinearPCM;
					out.mFramesPerPacket = 1;
					out.mReserved = 0;
					out.mSampleRate = _internal.FramesPerSecond;
					in.mBitsPerChannel = 0;
					in.mBytesPerFrame = 0;
					in.mBytesPerPacket = 0;
					in.mChannelsPerFrame = _encoded.ChannelCount;
					in.mFormatFlags = 0;
					in.mFormatID = kAudioFormatAppleLossless;
					in.mFramesPerPacket = 0;
					in.mReserved = 0;
					in.mSampleRate = _encoded.FramesPerSecond;
					magic = _magic->GetBuffer();
					magic_length = _magic->Length();
				} else if (_format == AudioFormatFreeLossless) {
					auto & info = *reinterpret_cast<Format::flac_stream_info *>(_magic->GetBuffer());
					_internal.Format = _encoded.Format;
					_internal.ChannelCount = _encoded.ChannelCount;
					_internal.FramesPerSecond = _encoded.FramesPerSecond;
					out.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
					out.mBytesPerFrame = StreamFrameByteSize(_internal);
					out.mBytesPerPacket = out.mBytesPerFrame;
					out.mChannelsPerFrame = _internal.ChannelCount;
					out.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
					out.mFormatID = kAudioFormatLinearPCM;
					out.mFramesPerPacket = 1;
					out.mReserved = 0;
					out.mSampleRate = _internal.FramesPerSecond;
					in.mBitsPerChannel = 0;
					in.mBytesPerFrame = 0;
					in.mBytesPerPacket = 0;
					in.mChannelsPerFrame = _encoded.ChannelCount;
					in.mFormatFlags = 0;
					in.mFormatID = kAudioFormatFLAC;
					in.mFramesPerPacket = Network::InverseEndianess(info.max_block_size);
					in.mReserved = 0;
					in.mSampleRate = _encoded.FramesPerSecond;
				}
				if (AudioCodecInitialize(_codec, &in, &out, magic, magic_length)) {
					AudioComponentInstanceDispose(_codec);
					throw Exception();
				}
			}
		public:
			CoreAudioDecoder(IAudioCodec * parent, const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) : _frames(0x100)
			{
				if (format.GetTrackClass() != Media::TrackClass::Audio) throw InvalidFormatException();
				_frames_pending = _local_frame_offset = _packet_size = 0;
				_format = format.GetTrackCodec();
				auto & ad = format.As<Media::AudioTrackFormatDesc>();
				_channel_layout = ad.GetChannelLayout();
				_encoded = ad.GetStreamDescriptor();
				if (ad.GetCodecMagic()) _magic = new DataBlock(*ad.GetCodecMagic());
				_parent.SetRetain(parent);
				if (desired_desc) {
					if (desired_desc->Format != SampleFormat::Invalid) _output.Format = desired_desc->Format;
					else if (_encoded.Format != SampleFormat::Invalid) _output.Format = _encoded.Format;
					else _output.Format = SampleFormat::S16_snorm;
					if (desired_desc->ChannelCount) _output.ChannelCount = desired_desc->ChannelCount;
					else _output.ChannelCount = _encoded.ChannelCount;
					if (desired_desc->FramesPerSecond) _output.FramesPerSecond = desired_desc->FramesPerSecond;
					else _output.FramesPerSecond = _encoded.FramesPerSecond;
				} else {
					_output = _encoded;
					if (_output.Format == SampleFormat::Invalid) _output.Format = SampleFormat::S16_snorm;
				}
				_internal = _output;
				if (_internal.Format == SampleFormat::S8_unorm) _internal.Format = SampleFormat::S8_snorm;
				else if (_internal.Format == SampleFormat::S16_unorm) _internal.Format = SampleFormat::S16_snorm;
				else if (_internal.Format == SampleFormat::S24_unorm) _internal.Format = SampleFormat::S24_snorm;
				else if (_internal.Format == SampleFormat::S32_unorm) _internal.Format = SampleFormat::S32_snorm;
				_init_decoder();
			}
			virtual ~CoreAudioDecoder(void) override { AudioComponentInstanceDispose(_codec); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _output; }
			virtual uint GetChannelLayout(void) const noexcept override { return _channel_layout; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Decoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _encoded; }
			virtual bool Reset(void) noexcept override
			{
				if (AudioCodecReset(_codec)) return false;
				_frames_pending = _local_frame_offset = 0;
				_frames.Clear();
				return true;
			}
			virtual int GetPendingPacketsCount(void) const noexcept override { return 0; }
			virtual int GetPendingFramesCount(void) const noexcept override { return _frames_pending; }
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept override
			{
				if (packet.PacketDataActuallyUsed) {
					if (_packet_size != packet.PacketRenderDuration) _packet_size = packet.PacketRenderDuration;
					UInt32 size = packet.PacketDataActuallyUsed, num_packets = 1;
					AudioStreamPacketDescription desc;
					desc.mDataByteSize = packet.PacketDataActuallyUsed;
					desc.mStartOffset = 0;
					desc.mVariableFramesInPacket = packet.PacketRenderDuration;
					if (AudioCodecAppendInputData(_codec, packet.PacketData->GetBuffer(), &size, &num_packets, &desc)) return false;
				} else {
					UInt32 size = 0, num_packets = 0;
					AudioStreamPacketDescription desc;
					desc.mDataByteSize = 0;
					desc.mStartOffset = 0;
					desc.mVariableFramesInPacket = 0;
					if (AudioCodecAppendInputData(_codec, &size, &size, &num_packets, &desc)) return false;
				}
				while (true) {
					try {
						SafePointer<WaveBuffer> wave = new WaveBuffer(_internal, _packet_size);
						UInt32 size = wave->GetAllocatedSizeInBytes();
						UInt32 num_packets = wave->GetSizeInFrames();
						UInt32 status;
						if (AudioCodecProduceOutputPackets(_codec, wave->GetData(), &size, &num_packets, 0, &status)) return false;
						if (status == kAudioCodecProduceOutputPacketAtEOF || status == kAudioCodecProduceOutputPacketNeedsMoreInputData) break;
						if (status == kAudioCodecProduceOutputPacketFailure) return false;
						if (num_packets) {
							wave->FramesUsed() = num_packets;
							if (wave->GetFormatDescriptor().ChannelCount != _output.ChannelCount) wave = wave->ReallocateChannels(_output.ChannelCount);
							if (wave->GetFormatDescriptor().FramesPerSecond != _output.FramesPerSecond) wave = wave->ConvertFrameRate(_output.FramesPerSecond);
							if (wave->GetFormatDescriptor().Format != _output.Format) wave = wave->ConvertFormat(_output.Format);
							_frames.Append(wave);
							_frames_pending += wave->FramesUsed();
						}
					} catch (...) { return false; }
				}
				return true;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != _output.Format || desc.ChannelCount != _output.ChannelCount || desc.FramesPerSecond != _output.FramesPerSecond) return false;
				uint64 write_pos = 0;
				auto free_space = buffer->GetSizeInFrames();
				auto frame_size = StreamFrameByteSize(_output);
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
		class CoreAudioEncoder : public IAudioEncoder
		{
			SafePointer<IAudioCodec> _parent;
			string _format;
			StreamDesc _input, _internal;
			SafePointer<Media::AudioTrackFormatDesc> _desc;
			AudioCodec _codec;
			uint _packet_frame_size;
			bool _eos;
			uint64 _frames_supplied, _frames_returned;
			Array<Media::PacketBuffer> _packets;

			void _update_codec_magic(void)
			{
				if (_format == AudioFormatAAC) {
					uint object_type = 2;
					uint frequency_index = (_internal.FramesPerSecond == 44100) ? 4 : 3;
					uint channel_count = _internal.ChannelCount;
					SafePointer<DataBlock> magic = new DataBlock(2);
					magic->SetLength(2);
					magic->ElementAt(0) = object_type << 3;
					magic->ElementAt(0) |= frequency_index >> 1;
					magic->ElementAt(1) = (frequency_index << 7) & 0x80;
					magic->ElementAt(1) |= channel_count << 3;
					_desc->SetCodecMagic(magic);
				} else if (_format == AudioFormatAppleLossless) {
					UInt32 magic_size;
					::Boolean magic_writable;
					if (AudioCodecGetPropertyInfo(_codec, kAudioCodecPropertyMagicCookie, &magic_size, &magic_writable)) throw Exception();
					SafePointer<DataBlock> magic = new DataBlock(0x100);
					magic->SetLength(magic_size);
					if (AudioCodecGetProperty(_codec, kAudioCodecPropertyMagicCookie, &magic_size, magic->GetBuffer())) throw Exception();
					magic->SetLength(magic_size);
					_desc->SetCodecMagic(magic);
				}
			}
			bool _drain_packets(void) noexcept
			{
				UInt32 packet_size;
				UInt32 prop_size = 4;
				if (AudioCodecGetProperty(_codec, kAudioCodecPropertyMaximumPacketByteSize, &prop_size, &packet_size)) return false;
				while (true) {
					UInt32 size, num_packets, status;
					AudioStreamPacketDescription desc;
					SafePointer<DataBlock> block;
					try {
						block = new DataBlock(0x100);
						block->SetLength(packet_size);
					} catch (...) { return false; }
					size = packet_size;
					num_packets = 1;
					if (AudioCodecProduceOutputPackets(_codec, block->GetBuffer(), &size, &num_packets, &desc, &status)) return false;
					if (status == kAudioCodecProduceOutputPacketAtEOF) {
						Media::PacketBuffer packet;
						packet.PacketDataActuallyUsed = 0;
						packet.PacketIsKey = true;
						packet.PacketDecodeTime = packet.PacketRenderTime = _frames_returned;
						packet.PacketRenderDuration = 0;
						_packets.Append(packet);
					} else if (status == kAudioCodecProduceOutputPacketFailure) {
						return false;
					} else if (status == kAudioCodecProduceOutputPacketNeedsMoreInputData) {
						break;
					} else if (status == kAudioCodecProduceOutputPacketSuccess || status == kAudioCodecProduceOutputPacketSuccessHasMore) {
						if (desc.mStartOffset) return false;
						block->SetLength(desc.mDataByteSize);
						Media::PacketBuffer packet;
						packet.PacketData = block;
						packet.PacketDataActuallyUsed = desc.mDataByteSize;
						packet.PacketIsKey = true;
						packet.PacketDecodeTime = packet.PacketRenderTime = _frames_returned;
						packet.PacketRenderDuration = _packet_frame_size;
						_frames_returned += _packet_frame_size;
						_packets.Append(packet);
					} else return false;
				}
				return true;
			}
		public:
			CoreAudioEncoder(IAudioCodec * parent, const string & format, const StreamDesc & desc, uint num_options, const uint * options) : _packets(0x100)
			{
				_parent.SetRetain(parent);
				_format = format;
				_input = desc;
				if (format == AudioFormatAAC) {
					_internal.Format = SampleFormat::S16_snorm;
					_internal.ChannelCount = min(_input.ChannelCount, 2U);
					if (_input.FramesPerSecond <= 44100) _internal.FramesPerSecond = 44100; else _internal.FramesPerSecond = 48000;
					_desc = new Media::AudioTrackFormatDesc(format, _internal);
					AudioComponentDescription desc;
					MemoryCopy(&desc.componentType, "cnea", 4);
					MemoryCopy(&desc.componentSubType, " caa", 4);
					MemoryCopy(&desc.componentManufacturer, "lppa", 4);
					desc.componentFlags = 0;
					desc.componentFlagsMask = 0;
					auto com = AudioComponentFindNext(0, &desc);
					if (!com) throw Exception();
					if (AudioComponentInstanceNew(com, &_codec)) throw Exception();
					for (uint i = 0; i < num_options; i++) if (options[2 * i] == Media::MediaEncoderSuggestedBytesPerSecond) {
						auto bps = options[2 * i + 1];
						if (bps <= 12000) bps = 12000;
						else if (bps <= 16000) bps = 16000;
						else if (bps <= 20000) bps = 20000;
						else bps = 24000;
						UInt32 size;
						::Boolean writable;
						if (!AudioCodecGetPropertyInfo(_codec, kAudioCodecPropertyCurrentTargetBitRate, &size, &writable) && writable && size == 4) {
							UInt32 bits = bps * 8;
							if (AudioCodecSetProperty(_codec, kAudioCodecPropertyCurrentTargetBitRate, size, &bits)) throw Exception();
						}
					}
					try {
						AudioStreamBasicDescription in, out;
						in.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
						in.mBytesPerFrame = StreamFrameByteSize(_internal);
						in.mBytesPerPacket = in.mBytesPerFrame;
						in.mChannelsPerFrame = _internal.ChannelCount;
						in.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
						in.mFormatID = kAudioFormatLinearPCM;
						in.mFramesPerPacket = 1;
						in.mReserved = 0;
						in.mSampleRate = _internal.FramesPerSecond;
						out.mBitsPerChannel = 0;
						out.mBytesPerFrame = 0;
						out.mBytesPerPacket = 0;
						out.mChannelsPerFrame = _internal.ChannelCount;
						out.mFormatFlags = 2;
						out.mFormatID = kAudioFormatMPEG4AAC;
						out.mFramesPerPacket = 1024;
						out.mReserved = 0;
						out.mSampleRate = _internal.FramesPerSecond;
						if (AudioCodecInitialize(_codec, &in, &out, 0, 0)) throw Exception();
						_update_codec_magic();
						_packet_frame_size = 1024;
					} catch (...) { AudioComponentInstanceDispose(_codec); throw; }
				} else if (format == AudioFormatAppleLossless) {
					if (SampleFormatBitSize(_input.Format) > 24) _internal.Format = SampleFormat::S32_snorm;
					else if (SampleFormatBitSize(_input.Format) > 16) _internal.Format = SampleFormat::S24_snorm;
					else _internal.Format = SampleFormat::S16_snorm;
					_internal.FramesPerSecond = _input.FramesPerSecond;
					_internal.ChannelCount = _input.ChannelCount;
					_desc = new Media::AudioTrackFormatDesc(format, _internal);
					AudioComponentDescription desc;
					MemoryCopy(&desc.componentType, "cnea", 4);
					MemoryCopy(&desc.componentSubType, "cala", 4);
					MemoryCopy(&desc.componentManufacturer, "lppa", 4);
					desc.componentFlags = 0;
					desc.componentFlagsMask = 0;
					auto com = AudioComponentFindNext(0, &desc);
					if (!com) throw Exception();
					if (AudioComponentInstanceNew(com, &_codec)) throw Exception();
					try {
						AudioStreamBasicDescription in, out;
						in.mBitsPerChannel = SampleFormatBitSize(_internal.Format);
						in.mBytesPerFrame = StreamFrameByteSize(_internal);
						in.mBytesPerPacket = in.mBytesPerFrame;
						in.mChannelsPerFrame = _internal.ChannelCount;
						in.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
						in.mFormatID = kAudioFormatLinearPCM;
						in.mFramesPerPacket = 1;
						in.mReserved = 0;
						in.mSampleRate = _internal.FramesPerSecond;
						out.mBitsPerChannel = 0;
						out.mBytesPerFrame = 0;
						out.mBytesPerPacket = 0;
						out.mChannelsPerFrame = _internal.ChannelCount;
						out.mFormatFlags = 0;
						out.mFormatID = kAudioFormatAppleLossless;
						out.mFramesPerPacket = 4096;
						out.mReserved = 0;
						out.mSampleRate = _internal.FramesPerSecond;
						if (AudioCodecInitialize(_codec, &in, &out, 0, 0)) throw Exception();
						_update_codec_magic();
						_packet_frame_size = 4096;
					} catch (...) { AudioComponentInstanceDispose(_codec); throw; }
				} else throw InvalidFormatException();
				_eos = false;
				_frames_supplied = _frames_returned = 0;
			}
			virtual ~CoreAudioEncoder(void) override { AudioComponentInstanceDispose(_codec); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return _input; }
			virtual uint GetChannelLayout(void) const noexcept override { return 0; }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::Encoder; }
			virtual IAudioCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override { return _desc->GetStreamDescriptor(); }
			virtual bool Reset(void) noexcept override
			{
				if (AudioCodecReset(_codec)) return false;
				_eos = false;
				_frames_supplied = _frames_returned = 0;
				_packets.Clear();
				return true;
			}
			virtual int GetPendingPacketsCount(void) const noexcept override { return _packets.Length(); }
			virtual int GetPendingFramesCount(void) const noexcept override { return _frames_supplied - _frames_returned; }
			virtual const Media::AudioTrackFormatDesc & GetFullEncodedDescriptor(void) const noexcept override { return *_desc; }
			virtual const DataBlock * GetCodecMagic(void) noexcept override { return _desc->GetCodecMagic(); }
			virtual bool SupplyFrames(const WaveBuffer * buffer) noexcept override
			{
				if (_eos) return false;
				if (!buffer) return false;
				auto & desc = buffer->GetFormatDescriptor();
				if (desc.Format != _input.Format) return false;
				if (desc.ChannelCount != _input.ChannelCount) return false;
				if (desc.FramesPerSecond != _input.FramesPerSecond) return false;
				if (!buffer->FramesUsed()) return false;
				SafePointer<WaveBuffer> local_buffer;
				const uint8 * raw_data;
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
				uint data_rest = data_length;
				uint frames_rest = frame_length;
				while (data_rest) {
					UInt32 data_size = data_rest, num_packets = frames_rest;
					if (AudioCodecAppendInputData(_codec, raw_data + data_length - data_rest, &data_size, &num_packets, 0)) return false;
					data_rest -= data_size;
					frames_rest -= num_packets;
					_frames_supplied += num_packets;
					if (!_drain_packets()) return false;
				}
				return true;
			}
			virtual bool SupplyEndOfStream(void) noexcept override
			{
				if (_eos) return false;
				UInt32 size = 0, num_packets = 0;
				AudioStreamPacketDescription desc;
				desc.mDataByteSize = 0;
				desc.mStartOffset = 0;
				desc.mVariableFramesInPacket = 0;
				if (AudioCodecAppendInputData(_codec, &size, &size, &num_packets, &desc)) return false;
				return _drain_packets();
			}
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept override
			{
				if (_packets.Length()) {
					packet = _packets.FirstElement();
					_packets.RemoveFirst();
					if (_eos && !packet.PacketDataActuallyUsed) {
						try { _update_codec_magic(); } catch (...) { return false; }
						if (!Reset()) return false;
					}
					return true;
				} else return false;
			}
		};
		class CoreAudioCodec : public IAudioCodec
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept override
			{
				if (format == AudioFormatAAC || format == AudioFormatAppleLossless) return true;
				else return false;
			}
			virtual bool CanDecode(const string & format) const noexcept override
			{
				if (format == AudioFormatMP3 || format == AudioFormatAAC || format == AudioFormatAppleLossless || format == AudioFormatFreeLossless) return true;
				else return false;
			}
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatAAC);
				list->Append(AudioFormatAppleLossless);
				list->Retain();
				return list;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > list = new Array<string>(0x04);
				list->Append(AudioFormatMP3);
				list->Append(AudioFormatAAC);
				list->Append(AudioFormatAppleLossless);
				list->Append(AudioFormatFreeLossless);
				list->Retain();
				return list;
			}
			virtual string GetCodecName(void) const override { return L"Core Audio Codec"; }
			virtual IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) noexcept override { try { return new CoreAudioDecoder(this, format, desired_desc); } catch (...) { return 0; } }
			virtual IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options, const uint * options) noexcept override { try { return new CoreAudioEncoder(this, format, desc, num_options, options); } catch (...) { return 0; } }
		};

		class CoreAudioOutputDevice : public IAudioOutputDevice
		{
			StreamDesc desc;
			AudioQueueRef queue;
			AudioDeviceID device; 
			int state; // 0 - stopped, 1 - processing, 2 - paused

			static void Queue_Callback(void * user, AudioQueueRef queue, AudioQueueBufferRef buffer)
			{
				auto wrapper = reinterpret_cast<CoreAudioBufferWrapper *>(buffer->mUserData);
				wrapper->Finalize(queue);
				wrapper->Release();
			}
			bool _internal_write_frames(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed, Semaphore * open_on_processed)
			{
				if (!buffer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (desc.Format != buffer_desc.Format || desc.ChannelCount != buffer_desc.ChannelCount || desc.FramesPerSecond != buffer_desc.FramesPerSecond) return false;
				AudioQueueBufferRef local;
				OSStatus status = AudioQueueAllocateBuffer(queue, buffer->GetAllocatedSizeInBytes(), &local);
				if (status) return false;
				local->mAudioDataByteSize = buffer->GetUsedSizeInBytes();
				MemoryCopy(local->mAudioData, buffer->GetData(), buffer->GetUsedSizeInBytes());
				CoreAudioBufferWrapper * wrapper = new (std::nothrow) CoreAudioBufferWrapper;
				if (!wrapper) { AudioQueueFreeBuffer(queue, local); return false; }
				wrapper->buffer = local;
				local->mUserData = wrapper;
				wrapper->write_status = write_status;
				wrapper->open.SetRetain(open_on_processed);
				wrapper->exec.SetRetain(execute_on_processed);
				status = AudioQueueEnqueueBuffer(queue, local, 0, 0);
				if (status) {
					AudioQueueFreeBuffer(queue, local);
					wrapper->Release();
					return false;
				}
				return true;
			}
		public:
			CoreAudioOutputDevice(AudioDeviceID dev_id, CFStringRef dev_uid, AudioStreamBasicDescription & dev_desc) : device(dev_id)
			{
				CoreAudioProduceStreamDescriptor(dev_desc, desc);
				OSStatus status = AudioQueueNewOutput(&dev_desc, Queue_Callback, this, 0, 0, 0, &queue);
				if (status) throw Exception();
				if (dev_uid) {
					status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &dev_uid, sizeof(dev_uid));
					if (status) { AudioQueueDispose(queue, true); throw Exception(); }
				}
				state = 0;
			}
			virtual ~CoreAudioOutputDevice(void) override { AudioQueueDispose(queue, false); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return desc; }
			virtual uint GetChannelLayout(void) const noexcept override { return CoreAudioGetChannelLayout(device, kAudioUnitScope_Output, 0); }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceOutput; }
			virtual string GetDeviceIdentifier(void) const override { return string(device); }
			virtual double GetVolume(void) noexcept override
			{
				AudioQueueParameterValue value;
				OSStatus status = AudioQueueGetParameter(queue, kAudioQueueParam_Volume, &value);
				if (status) return -1.0;
				return value;
			}
			virtual void SetVolume(double volume) noexcept override { AudioQueueSetParameter(queue, kAudioQueueParam_Volume, volume); }
			virtual bool StartProcessing(void) noexcept override
			{
				if (state == 0 || state == 2) {
					OSStatus status = AudioQueueStart(queue, 0);
					if (status) return false;
					state = 1;
					return true;
				} else return false;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (state == 1) {
					OSStatus status = AudioQueuePause(queue);
					if (status) return false;
					state = 2;
					return true;
				} else return false;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				if (state == 1 || state == 2) {
					OSStatus status = AudioQueueStop(queue, true);
					if (status) return false;
					state = 0;
					return true;
				} else return false;
			}
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept override
			{
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				if (!semaphore) return false;
				bool status;
				if (!WriteFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept override
			{
				return _internal_write_frames(buffer, write_status, 0, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!execute_on_processed) return false;
				return _internal_write_frames(buffer, write_status, execute_on_processed, 0);
			}
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept override
			{
				if (!open_on_processed) return false;
				return _internal_write_frames(buffer, write_status, 0, open_on_processed);
			}
		};
		class CoreAudioInputDevice : public IAudioInputDevice
		{
			StreamDesc desc;
			AudioQueueRef queue;
			AudioDeviceID device;
			int state; // 0 - stopped, 1 - processing, 2 - paused

			static void Queue_Callback(void * user, AudioQueueRef queue, AudioQueueBufferRef buffer, const AudioTimeStamp *, uint32, const AudioStreamPacketDescription *)
			{
				auto wrapper = reinterpret_cast<CoreAudioBufferWrapper *>(buffer->mUserData);
				wrapper->Finalize(queue);
				wrapper->Release();
			}
			bool _internal_read_frames(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed, Semaphore * open_on_processed)
			{
				if (!buffer) return false;
				auto & buffer_desc = buffer->GetFormatDescriptor();
				if (desc.Format != buffer_desc.Format || desc.ChannelCount != buffer_desc.ChannelCount || desc.FramesPerSecond != buffer_desc.FramesPerSecond) return false;
				AudioQueueBufferRef local;
				OSStatus status = AudioQueueAllocateBuffer(queue, buffer->GetAllocatedSizeInBytes(), &local);
				if (status) return false;
				CoreAudioBufferWrapper * wrapper = new (std::nothrow) CoreAudioBufferWrapper;
				if (!wrapper) { AudioQueueFreeBuffer(queue, local); return false; }
				wrapper->buffer = local;
				local->mUserData = wrapper;
				wrapper->store_at.SetRetain(buffer);
				wrapper->write_status = read_status;
				wrapper->open.SetRetain(open_on_processed);
				wrapper->exec.SetRetain(execute_on_processed);
				status = AudioQueueEnqueueBuffer(queue, local, 0, 0);
				if (status) {
					AudioQueueFreeBuffer(queue, local);
					wrapper->Release();
					return false;
				}
				return true;
			}
		public:
			CoreAudioInputDevice(AudioDeviceID dev_id, CFStringRef dev_uid, AudioStreamBasicDescription & dev_desc) : device(dev_id)
			{
				CoreAudioProduceStreamDescriptor(dev_desc, desc);
				OSStatus status = AudioQueueNewInput(&dev_desc, Queue_Callback, this, 0, 0, 0, &queue);
				if (status) throw Exception();
				if (dev_uid) {
					status = AudioQueueSetProperty(queue, kAudioQueueProperty_CurrentDevice, &dev_uid, sizeof(dev_uid));
					if (status) { AudioQueueDispose(queue, true); throw Exception(); }
				}
				state = 0;
			}
			virtual ~CoreAudioInputDevice(void) override { AudioQueueDispose(queue, false); }
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override { return desc; }
			virtual uint GetChannelLayout(void) const noexcept override { return CoreAudioGetChannelLayout(device, kAudioUnitScope_Input, 0); }
			virtual AudioObjectType GetObjectType(void) const noexcept override { return AudioObjectType::DeviceInput; }
			virtual string GetDeviceIdentifier(void) const override { return string(device); }
			virtual double GetVolume(void) noexcept override
			{
				AudioQueueParameterValue value;
				OSStatus status = AudioQueueGetParameter(queue, kAudioQueueParam_Volume, &value);
				if (status) return -1.0;
				return value;
			}
			virtual void SetVolume(double volume) noexcept override { AudioQueueSetParameter(queue, kAudioQueueParam_Volume, volume); }
			virtual bool StartProcessing(void) noexcept override
			{
				if (state == 0 || state == 2) {
					OSStatus status = AudioQueueStart(queue, 0);
					if (status) return false;
					state = 1;
					return true;
				} else return false;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (state == 1) {
					OSStatus status = AudioQueuePause(queue);
					if (status) return false;
					state = 2;
					return true;
				} else return false;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				if (state == 1 || state == 2) {
					OSStatus status = AudioQueueStop(queue, true);
					if (status) return false;
					state = 0;
					return true;
				} else return false;
			}
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept override
			{
				SafePointer<Semaphore> semaphore = CreateSemaphore(0);
				if (!semaphore) return false;
				bool status;
				if (!ReadFramesAsync(buffer, &status, semaphore)) return false;
				semaphore->Wait();
				return status;
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status) noexcept override
			{
				return _internal_read_frames(buffer, read_status, 0, 0);
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed) noexcept override
			{
				if (!execute_on_processed) return false;
				return _internal_read_frames(buffer, read_status, execute_on_processed, 0);
			}
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, Semaphore * open_on_processed) noexcept override
			{
				if (!open_on_processed) return false;
				return _internal_read_frames(buffer, read_status, 0, open_on_processed);
			}
		};
		class CoreAudioDeviceFactory : public IAudioDeviceFactory
		{
			SafePointer<Semaphore> access_sync;
			Array<IAudioEventCallback *> callbacks;
			Array<AudioDeviceID> devs_out, devs_in;

			static bool _check_device_io(AudioDeviceID dev, AudioObjectPropertyScope scope)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyStreamConfiguration;
				address.mScope = scope;
				address.mElement = 0;
				uint32 size;
				OSStatus status = AudioObjectGetPropertyDataSize(dev, &address, 0, 0, &size);
				if (status) return false;
				AudioBufferList * list = reinterpret_cast<AudioBufferList *>(malloc(size));
				if (!list) return false;
				status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, list);
				if (status) { free(list); return false; }
				bool has_io = false;
				for (int i = 0; i < list->mNumberBuffers; i++) if (list->mBuffers[i].mNumberChannels) { has_io = true; break; }
				free(list);
				return has_io;
			}
			static CFStringRef _get_device_uid(AudioDeviceID dev)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyDeviceUID;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				CFStringRef uid;
				uint32 size = sizeof(uid);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &uid);
				if (status) return 0;
				return uid;
			}
			static string _get_device_name(AudioDeviceID dev)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyDeviceNameCFString;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				CFStringRef name;
				uint32 size = sizeof(name);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &name);
				if (status) throw Exception();
				Array<UniChar> buffer(0x100);
				buffer.SetLength(CFStringGetLength(name));
				CFStringGetCharacters(name, CFRangeMake(0, buffer.Length()), buffer.GetBuffer());
				CFRelease(name);
				return string(buffer.GetBuffer(), buffer.Length(), Encoding::UTF16);
			}
			static AudioDeviceID _get_default_device(AudioObjectPropertySelector selector)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = selector;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				AudioDeviceID result;
				uint32 size = sizeof(result);
				OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, 0, &size, &result);
				if (status) throw Exception();
				return result;
			}
			static void _get_device_stream_format(AudioDeviceID dev, AudioObjectPropertyScope scope, AudioStreamBasicDescription & desc)
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioDevicePropertyStreamFormat;
				address.mScope = scope;
				address.mElement = kAudioObjectPropertyElementMaster;
				uint32 size = sizeof(desc);
				OSStatus status = AudioObjectGetPropertyData(dev, &address, 0, 0, &size, &desc);
				if (status) throw Exception();
			}
			static Array<AudioDeviceID> * _list_audio_devices(void)
			{
				SafePointer< Array<AudioDeviceID> > devs = new Array<AudioDeviceID>(0x10);
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				uint32 size;
				OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, 0, &size);
				if (status) throw Exception();
				devs->SetLength(size / sizeof(AudioDeviceID));
				status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, 0, &size, devs->GetBuffer());
				if (status) throw Exception();
				devs->Retain();
				return devs;
			}

			void _raise_event(AudioDeviceEvent event, AudioObjectType type, const string & dev_id) { for (auto & callback : callbacks) callback->OnAudioDeviceEvent(event, type, dev_id); }
			static OSStatus _device_change_listener(AudioObjectID object, uint32 num_address, const AudioObjectPropertyAddress * addresses, void * user)
			{
				auto self = reinterpret_cast<CoreAudioDeviceFactory *>(user);
				self->access_sync->Wait();
				if (object == kAudioObjectSystemObject) for (int i = 0; i < num_address; i++) {
					if (addresses[i].mScope == kAudioObjectPropertyScopeGlobal && addresses[i].mElement == kAudioObjectPropertyElementMaster) {
						if (addresses[i].mSelector == kAudioHardwarePropertyDevices) {
							try {
								SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
								for (auto & dev : self->devs_out) {
									bool present = false;
									for (auto & dev2 : *devs) if (dev2 == dev) { present = true; break; }
									if (!present) self->_raise_event(AudioDeviceEvent::Inactivated, AudioObjectType::DeviceOutput, string(dev));
								}
								for (auto & dev : self->devs_in) {
									bool present = false;
									for (auto & dev2 : *devs) if (dev2 == dev) { present = true; break; }
									if (!present) self->_raise_event(AudioDeviceEvent::Inactivated, AudioObjectType::DeviceInput, string(dev));
								}
								for (auto & dev : *devs) {
									if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) {
										bool present = false;
										for (auto & dev2 : self->devs_out) if (dev2 == dev) { present = true; break; }
										if (!present) self->_raise_event(AudioDeviceEvent::Activated, AudioObjectType::DeviceOutput, string(dev));
									} else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) {
										bool present = false;
										for (auto & dev2 : self->devs_in) if (dev2 == dev) { present = true; break; }
										if (!present) self->_raise_event(AudioDeviceEvent::Activated, AudioObjectType::DeviceInput, string(dev));
									}
								}
								self->devs_in.Clear();
								self->devs_out.Clear();
								for (auto & dev : *devs) {
									if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) self->devs_out << dev;
									else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) self->devs_in << dev;
								}
							} catch (...) {}
						} else if (addresses[i].mSelector == kAudioHardwarePropertyDefaultOutputDevice) {
							try {
								self->_raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceOutput, string(_get_default_device(addresses[i].mSelector)));
							} catch (...) {}
						} else if (addresses[i].mSelector == kAudioHardwarePropertyDefaultInputDevice) {
							try {
								self->_raise_event(AudioDeviceEvent::DefaultChanged, AudioObjectType::DeviceInput, string(_get_default_device(addresses[i].mSelector)));
							} catch (...) {}
						}
					}
				}
				self->access_sync->Open();
				return 0;
			}
		public:
			CoreAudioDeviceFactory(void) : callbacks(0x10), devs_out(0x10), devs_in(0x10)
			{
				access_sync = CreateSemaphore(1);
				if (!access_sync) throw Exception();
				SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
				for (auto & dev : *devs) {
					if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) devs_out << dev;
					else if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) devs_in << dev;
				}
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) throw Exception();
				address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) {
					address.mSelector = kAudioHardwarePropertyDevices;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					throw Exception();
				}
				address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
				if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this)) {
					address.mSelector = kAudioHardwarePropertyDevices;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
					AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
					throw Exception();
				}
			}
			virtual ~CoreAudioDeviceFactory(void) override
			{
				AudioObjectPropertyAddress address;
				address.mSelector = kAudioHardwarePropertyDevices;
				address.mScope = kAudioObjectPropertyScopeGlobal;
				address.mElement = kAudioObjectPropertyElementMaster;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
				address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
				address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address, _device_change_listener, this);
			}
			virtual Volumes::Dictionary<string, string> * GetAvailableOutputDevices(void) noexcept override
			{
				try {
					SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
					SafePointer< Volumes::Dictionary<string, string> > result = new Volumes::Dictionary<string, string>;
					for (auto & dev : *devs) if (_check_device_io(dev, kAudioDevicePropertyScopeOutput)) result->Append(dev, _get_device_name(dev));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Volumes::Dictionary<string, string> * GetAvailableInputDevices(void) noexcept override
			{
				try {
					SafePointer< Array<AudioDeviceID> > devs = _list_audio_devices();
					SafePointer< Volumes::Dictionary<string, string> > result = new Volumes::Dictionary<string, string>;
					for (auto & dev : *devs) if (_check_device_io(dev, kAudioDevicePropertyScopeInput)) result->Append(dev, _get_device_name(dev));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IAudioOutputDevice * CreateOutputDevice(const string & identifier) noexcept override
			{
				CFStringRef uid = 0;
				try {
					AudioDeviceID dev_id = identifier.ToUInt32();
					if (!_check_device_io(dev_id, kAudioDevicePropertyScopeOutput)) throw Exception();
					uid = _get_device_uid(dev_id);
					if (!uid) throw Exception();
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeOutput, desc);
					IAudioOutputDevice * device = new CoreAudioOutputDevice(dev_id, uid, desc);
					CFRelease(uid);
					return device;
				} catch (...) { if (uid) CFRelease(uid); return 0; }
			}
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept override
			{
				try {
					AudioDeviceID dev_id = _get_default_device(kAudioHardwarePropertyDefaultOutputDevice);
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeOutput, desc);
					return new CoreAudioOutputDevice(dev_id, 0, desc);
				} catch (...) { return 0; }
			}
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept override
			{
				CFStringRef uid = 0;
				try {
					AudioDeviceID dev_id = identifier.ToUInt32();
					if (!_check_device_io(dev_id, kAudioDevicePropertyScopeInput)) throw Exception();
					uid = _get_device_uid(dev_id);
					if (!uid) throw Exception();
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeInput, desc);
					IAudioInputDevice * device = new CoreAudioInputDevice(dev_id, uid, desc);
					CFRelease(uid);
					return device;
				} catch (...) { if (uid) CFRelease(uid); return 0; }
			}
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept override
			{
				try {
					AudioDeviceID dev_id = _get_default_device(kAudioHardwarePropertyDefaultInputDevice);
					AudioStreamBasicDescription desc;
					_get_device_stream_format(dev_id, kAudioDevicePropertyScopeInput, desc);
					return new CoreAudioInputDevice(dev_id, 0, desc);
				} catch (...) { return 0; }
			}
			virtual bool RegisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				access_sync->Wait();
				try {
					for (auto & cb : callbacks) if (cb == callback) { access_sync->Open(); return true; }
					callbacks.Append(callback);
					access_sync->Open();
					return true;
				} catch (...) { access_sync->Open(); return false; }
			}
			virtual bool UnregisterEventCallback(IAudioEventCallback * callback) noexcept override
			{
				access_sync->Wait();
				for (int i = 0; i < callbacks.Length(); i++) if (callbacks[i] == callback) {
					callbacks.Remove(i);
					access_sync->Open();
					return true;
				}
				access_sync->Open();
				return false;
			}
		};

		SafePointer<IAudioCodec> _system_codec;
		IAudioCodec * InitializeSystemCodec(void)
		{
			if (!_system_codec) {
				_system_codec = new CoreAudioCodec;
				RegisterCodec(_system_codec);
			}
			return _system_codec;
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return new CoreAudioDeviceFactory; }
		void SystemBeep(void) { AudioServicesPlayAlertSound(kSystemSoundID_UserPreferredAlert); }
	}
}