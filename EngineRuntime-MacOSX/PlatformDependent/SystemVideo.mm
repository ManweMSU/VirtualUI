#include "../Interfaces/SystemVideo.h"

#include "../Interfaces/Socket.h"
#include "MetalGraphicsAPI.h"
#include "SystemWindowsAPI.h"
#include "CocoaInterop.h"

#include <VideoToolbox/VideoToolbox.h>
#include <AVFoundation/AVFoundation.h>

@interface ERTVideoCaptureDelegate : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
	{
	@public
		Engine::Video::IVideoDevice * device;
	}
	- (void) captureOutput: (AVCaptureOutput *) output didOutputSampleBuffer: (CMSampleBufferRef) sampleBuffer fromConnection: (AVCaptureConnection *) connection;
	- (void) captureOutput: (AVCaptureOutput *) output didDropSampleBuffer: (CMSampleBufferRef) sampleBuffer fromConnection: (AVCaptureConnection *) connection;
@end

namespace Engine
{
	namespace Video
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(h264_header)
				uint8 version;
				uint8 avc_profile;
				uint8 avc_compatibility;
				uint8 avc_level;
				uint8 length_field_size;
				uint8 data[0];
			ENGINE_END_PACKED_STRUCTURE
		}

		class CoreVideoFrame : public IVideoFrame
		{
			CVImageBufferRef _image;
			VideoObjectDesc _desc;
		public:
			CoreVideoFrame(CVImageBufferRef image, const VideoObjectDesc & desc) : _image(image), _desc(desc) { CFRetain(_image); }
			virtual ~CoreVideoFrame(void) override { CFRelease(_image); }
			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override { return _desc; }
			virtual VideoObjectType GetObjectType(void) const noexcept override { return VideoObjectType::Frame; }
			virtual handle GetBufferFormat(void) const noexcept override { return 0; }
			virtual void SetFramePresentation(uint duration) noexcept override { _desc.FramePresentation = duration; }
			virtual void SetFrameDuration(uint duration) noexcept override { _desc.FrameDuration = duration; }
			virtual void SetTimeScale(uint scale) noexcept override { _desc.TimeScale = scale; }
			virtual Codec::Frame * QueryFrame(void) const noexcept override
			{
				CVPixelBufferRef result_wrapper = 0;
				VTPixelTransferSessionRef session = 0;
				try {
					DataBlock data(0x100);
					data.SetLength(4 * _desc.Width * _desc.Height);
					if (CVPixelBufferCreateWithBytes(0, _desc.Width, _desc.Height, kCVPixelFormatType_32BGRA, data.GetBuffer(), _desc.Width * 4, 0, 0, 0, &result_wrapper)) throw Exception();
					if (VTPixelTransferSessionCreate(0, &session)) throw Exception();
					if (VTPixelTransferSessionTransferImage(session, _image, result_wrapper)) throw Exception();
					VTPixelTransferSessionInvalidate(session);
					CFRelease(session);
					session = 0;
					CVAttachmentMode mode;
					CFTypeRef alpha_mode_ref = CVBufferGetAttachment(result_wrapper, kCVImageBufferAlphaChannelModeKey, &mode);
					Codec::AlphaMode alpha_mode = Codec::AlphaMode::Straight;
					if (alpha_mode_ref) {
						if (CFStringCompare(reinterpret_cast<CFStringRef>(alpha_mode_ref), kCVImageBufferAlphaChannelMode_PremultipliedAlpha, 0) == 0) alpha_mode = Codec::AlphaMode::Premultiplied;
						else if (CFStringCompare(reinterpret_cast<CFStringRef>(alpha_mode_ref), kCVImageBufferAlphaChannelMode_StraightAlpha, 0) == 0) alpha_mode = Codec::AlphaMode::Straight;
					}
					CFRelease(result_wrapper);
					result_wrapper = 0;
					SafePointer<Codec::Frame> result = new Codec::Frame(_desc.Width, _desc.Height, _desc.Width * 4, Codec::PixelFormat::B8G8R8A8, alpha_mode, Codec::ScanOrigin::TopDown);
					MemoryCopy(result->GetData(), data.GetBuffer(), result->GetScanLineLength() * result->GetHeight());
					result->Retain();
					return result;
				} catch (...) {
					if (result_wrapper) CFRelease(result_wrapper);
					if (session) {
						VTPixelTransferSessionInvalidate(session);
						CFRelease(session);
					}
					return 0;
				}
			}
			CVImageBufferRef GetSurface(void) const noexcept { return _image; }
		};
		class VideoToolboxTransferer : public IVideoFrameBlt
		{
			DataBlock _internal_blt;
			VTPixelTransferSessionRef _session;
			CVPixelBufferRef _buffer;
			VideoObjectDesc _desc;
			bool _input_set, _output_set;
			uint _stride;

			bool _init_transform(void) noexcept
			{
				try {
					_stride = _desc.Width * 4;
					_internal_blt.SetLength(_stride * _desc.Height);
				} catch (...) { return false; }
				if (!_session) {
					if (VTPixelTransferSessionCreate(0, &_session)) return false;
				}
				if (!_buffer) {
					if (CVPixelBufferCreateWithBytes(0, _desc.Width, _desc.Height, kCVPixelFormatType_32BGRA, _internal_blt.GetBuffer(), _stride, 0, 0, 0, &_buffer)) return false;
				}
				return true;
			}
		public:
			VideoToolboxTransferer(void) : _internal_blt(0x100)
			{
				_session = 0;
				_buffer = 0;
				_desc.Width = 0;
				_desc.Height = 0;
				_desc.TimeScale = 0;
				_desc.FramePresentation = 0;
				_desc.FrameDuration = 0;
				_desc.Device = 0;
				_input_set = false;
				_output_set = false;
			}
			virtual ~VideoToolboxTransferer(void) override
			{
				if (_session) {
					VTPixelTransferSessionInvalidate(_session);
					CFRelease(_session);
				}
				if (_buffer) CFRelease(_buffer);
			}
			virtual bool SetInputFormat(const IVideoObject * format_provider) noexcept override
			{
				if (!format_provider) return false;
				return SetInputFormat(Graphics::PixelFormat::B8G8R8A8_unorm, Codec::AlphaMode::Straight, format_provider->GetObjectDescriptor());
			}
			virtual bool SetInputFormat(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept override
			{
				if (_input_set) return false;
				if (format != Graphics::PixelFormat::B8G8R8A8_unorm) return false;
				if (_output_set) {
					if (_desc.Width != desc.Width || _desc.Height != desc.Height) return false;
				} else {
					_desc.Width = desc.Width;
					_desc.Height = desc.Height;
				}
				_input_set = true;
				if (_input_set && _output_set) return _init_transform(); else return true;
			}
			virtual bool SetOutputFormat(const IVideoObject * format_provider) noexcept override
			{
				if (!format_provider) return false;
				return SetOutputFormat(Graphics::PixelFormat::B8G8R8A8_unorm, Codec::AlphaMode::Straight, format_provider->GetObjectDescriptor());
			}
			virtual bool SetOutputFormat(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept override
			{
				if (_output_set) return false;
				if (format != Graphics::PixelFormat::B8G8R8A8_unorm) return false;
				if (_input_set) {
					if (_desc.Width != desc.Width || _desc.Height != desc.Height) return false;
				} else {
					_desc.Width = desc.Width;
					_desc.Height = desc.Height;
				}
				_output_set = true;
				if (_input_set && _output_set) return _init_transform(); else return true;
			}
			virtual bool Reset(void) noexcept override
			{
				if (_session) {
					VTPixelTransferSessionInvalidate(_session);
					CFRelease(_session);
				}
				if (_buffer) CFRelease(_buffer);
				_internal_blt.SetLength(0);
				_session = 0;
				_buffer = 0;
				_desc.Width = 0;
				_desc.Height = 0;
				_input_set = false;
				_output_set = false;
				return true;
			}
			virtual bool IsInitialized(void) const noexcept override { return _session; }
			virtual bool Process(IVideoFrame * dest, Graphics::ITexture * from, Graphics::SubresourceIndex subres) noexcept override
			{
				if (!IsInitialized()) {
					VideoObjectDesc desc;
					desc.Width = from->GetWidth();
					desc.Height = from->GetHeight();
					desc.TimeScale = desc.FramePresentation = desc.FrameDuration = 0;
					desc.Device = 0;
					if (!_input_set && !SetInputFormat(from->GetPixelFormat(), Codec::AlphaMode::Straight, desc)) return false;
					if (!_output_set && !SetOutputFormat(dest)) return false;
					if (!IsInitialized()) return false;
				}
				if (from->GetTextureType() != Graphics::TextureType::Type2D && from->GetTextureType() != Graphics::TextureType::TypeArray2D) return false;
				auto frame = static_cast<const CoreVideoFrame *>(dest);
				MetalGraphics::QueryMetalTexture(from, _internal_blt.GetBuffer(), _stride, subres);
				if (VTPixelTransferSessionTransferImage(_session, _buffer, frame->GetSurface())) return false;
				return true;
			}
			virtual bool Process(Graphics::ITexture * dest, const IVideoFrame * from, Graphics::SubresourceIndex subres) noexcept override
			{
				if (!IsInitialized()) {
					VideoObjectDesc desc;
					desc.Width = dest->GetWidth();
					desc.Height = dest->GetHeight();
					desc.TimeScale = desc.FramePresentation = desc.FrameDuration = 0;
					desc.Device = 0;
					if (!_input_set && !SetInputFormat(from)) return false;
					if (!_output_set && !SetOutputFormat(dest->GetPixelFormat(), Codec::AlphaMode::Straight, desc)) return false;
					if (!IsInitialized()) return false;
				}
				if (dest->GetTextureType() != Graphics::TextureType::Type2D && dest->GetTextureType() != Graphics::TextureType::TypeArray2D) return false;
				auto frame = static_cast<const CoreVideoFrame *>(from);
				if (VTPixelTransferSessionTransferImage(_session, frame->GetSurface(), _buffer)) return false;
				MetalGraphics::UpdateMetalTexture(dest, _internal_blt.GetBuffer(), _stride, subres);
				return true;
			}
		};

		class VideoToolboxDecoder : public IVideoDecoder
		{
			struct frame_output_rec
			{
				uint64 presentation;
				uint64 duration;
				SafePointer<IVideoFrame> frame;
			};

			SafePointer<IVideoCodec> _codec;
			VTDecompressionSessionRef _session;
			CMFormatDescriptionRef _input_format;
			VideoObjectDesc _desc;
			string _format;
			Array<frame_output_rec> _frames;
			bool _eos;
			int64 _clock;

			void _reset_session(void)
			{
				if (_session) {
					VTDecompressionSessionInvalidate(_session);
					CFRelease(_session);
					_session = 0;
				}
				CFMutableDictionaryRef prop_dict = CFDictionaryCreateMutable(CFAllocatorGetDefault(), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (_desc.Device) CFDictionaryAddValue(prop_dict, kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder, kCFBooleanTrue);
				if (VTDecompressionSessionCreate(0, _input_format, prop_dict, 0, 0, &_session)) {
					CFRelease(prop_dict);
					throw InvalidFormatException();
				}
				CFRelease(prop_dict);
				CFBooleanRef using_hardware_ref;
				if (!VTSessionCopyProperty(_session, kVTDecompressionPropertyKey_UsingHardwareAcceleratedVideoDecoder, CFAllocatorGetDefault(), &using_hardware_ref)) {
					auto using_hardware = CFBooleanGetValue(using_hardware_ref);
					CFRelease(using_hardware_ref);
					if (!using_hardware && _desc.Device) {
						_desc.Device->Release();
						_desc.Device = 0;
					}
				} else if (_desc.Device) {
					_desc.Device->Release();
					_desc.Device = 0;
				}
			}
			bool _inst_time_entry(uint64 time) noexcept
			{
				try {
					for (auto & f : _frames) if (f.presentation == time) return true;
					frame_output_rec rec;
					rec.presentation = time;
					rec.duration = 0;
					if (!_frames.Length() || rec.presentation < _frames.FirstElement().presentation) {
						_frames.Insert(rec, 0);
					} else {
						for (int i = 0; i < _frames.Length(); i++) if (_frames[i].presentation > rec.presentation) {
							_frames.Insert(rec, i);
							return true;
						}
						_frames.Append(rec);
					}
					return true;
				} catch (...) { return false; }
			}
			bool _fill_time_entry(uint64 time, uint64 duration, IVideoFrame * frame) noexcept
			{
				for (auto & f : _frames) if (f.presentation == time) {
					f.duration = duration;
					f.frame.SetRetain(frame);
					return true;
				}
				return false;
			}
			bool _can_read_frame(void) const noexcept
			{
				if (!_frames.Length()) return false;
				if (!_frames.FirstElement().frame) return false;
				auto t1 = _frames.FirstElement().presentation;
				auto t2 = t1 - _frames.FirstElement().duration / 2;
				auto time = min(t1, t2);
				return time <= _clock;
			}
			static void _buffer_release(void * releaseRefCon, const void * baseAddress) { free(releaseRefCon); }
		public:
			VideoToolboxDecoder(IVideoCodec * codec, const Media::TrackFormatDesc & format, Graphics::IDevice * acceleration_device) : _frames(0x10)
			{
				if (!format.GetCodecMagic() && format.GetTrackCodec() == VideoFormatH264) throw InvalidArgumentException();
				_codec.SetRetain(codec);
				_format = format.GetTrackCodec();
				CMFormatDescriptionRef input_format;
				auto & vd = format.As<Media::VideoTrackFormatDesc>();
				_session = 0;
				_desc.Width = vd.GetWidth();
				_desc.Height = vd.GetHeight();
				_desc.TimeScale = vd.GetFrameRateNumerator();
				_desc.FrameDuration = vd.GetFrameRateDenominator();
				_desc.FramePresentation = 0;
				if (_format == VideoFormatH264) {
					auto & magic = *format.GetCodecMagic();
					auto & hdr = *reinterpret_cast<const Format::h264_header *>(magic.GetBuffer());
					auto nalu_length_size = (hdr.length_field_size & 0x03) + 1;
					int offs = 0;
					Array<const uint8 *> ps_ptr(0x10);
					Array<size_t> ps_size(0x10);
					int nsps = hdr.data[offs] & 0x1F;
					offs++;
					for (int i = 0; i < nsps; i++) {
						int fl = (uint32(hdr.data[offs]) << 8) | uint32(hdr.data[offs + 1]);
						offs += 2;
						ps_ptr << &hdr.data[offs];
						ps_size << fl;
						offs += fl;
					}
					int npps = hdr.data[offs];
					offs++;
					for (int i = 0; i < npps; i++) {
						int fl = (uint32(hdr.data[offs]) << 8) | uint32(hdr.data[offs + 1]);
						offs += 2;
						ps_ptr << &hdr.data[offs];
						ps_size << fl;
						offs += fl;
					}
					if (CMVideoFormatDescriptionCreateFromH264ParameterSets(0, nsps + npps, ps_ptr.GetBuffer(), ps_size.GetBuffer(), nalu_length_size, &_input_format)) throw InvalidFormatException();
					_desc.Device = acceleration_device;
					if (_desc.Device) _desc.Device->Retain();
					try {
						_reset_session();
					} catch (...) {
						if (_desc.Device) _desc.Device->Release();
						CFRelease(_input_format);
						throw;
					}
				} else if (_format == VideoFormatRGB) {
					_input_format = 0;
					_session = 0;
					_desc.Device = 0;
				}
				_eos = false;
				_clock = -1;
			}
			virtual ~VideoToolboxDecoder(void) override
			{
				if (_desc.Device) _desc.Device->Release();
				VTDecompressionSessionInvalidate(_session);
				if (_session) CFRelease(_session);
				if (_input_format) CFRelease(_input_format);
			}
			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override { return _desc; }
			virtual VideoObjectType GetObjectType(void) const noexcept override { return VideoObjectType::Decoder; }
			virtual handle GetBufferFormat(void) const noexcept override { return 0; }
			virtual IVideoCodec * GetParentCodec(void) const override { return _codec; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual bool Reset(void) noexcept override
			{
				_frames.Clear();
				_eos = false;
				_clock = -1;
				return true;
			}
			virtual bool IsOutputAvailable(void) const noexcept override
			{
				if (_clock == -1) return false;
				if (_can_read_frame()) return true;
				if (_eos) return true;
				return false;
			}
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept override
			{
				if (IsOutputAvailable()) return false;
				if (packet.PacketDataActuallyUsed) {
					if (_eos) return false;
					if (_session) {
						if (!_inst_time_entry(packet.PacketRenderTime)) return false;
						if (_clock == -1) _clock = packet.PacketDecodeTime;
						CMBlockBufferRef buffer;
						if (CMBlockBufferCreateWithMemoryBlock(0, packet.PacketData->GetBuffer(), packet.PacketDataActuallyUsed, kCFAllocatorNull,
							0, 0, packet.PacketDataActuallyUsed, 0, &buffer)) return false;
						CMSampleBufferRef sample;
						CMSampleTimingInfo timing;
						timing.decodeTimeStamp = CMTimeMake(packet.PacketDecodeTime, 1);
						timing.presentationTimeStamp = CMTimeMake(packet.PacketRenderTime, 1);
						timing.duration = CMTimeMake(packet.PacketRenderDuration, 1);
						size_t size = packet.PacketDataActuallyUsed;
						if (CMSampleBufferCreateReady(kCFAllocatorDefault, buffer, _input_format, 1, 1, &timing, 1, &size, &sample)) {
							CFRelease(buffer);
							return false;
						}
						CFRelease(buffer);
						__block CVImageBufferRef decoded = 0;
						auto status = VTDecompressionSessionDecodeFrameWithOutputHandler(_session, sample, 0, 0,
							^(OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime presentationTimeStamp, CMTime presentationDuration) {
							if (!status && imageBuffer) {
								CFRetain(imageBuffer);
								decoded = imageBuffer;
							}
						});
						CFRelease(sample);
						if (status) return false;
						if (decoded) {
							SafePointer<IVideoFrame> frame;
							try {
								VideoObjectDesc vod;
								vod = _desc;
								vod.Device = 0;
								vod.FramePresentation = packet.PacketRenderTime;
								vod.FrameDuration = packet.PacketRenderDuration;
								frame = new CoreVideoFrame(decoded, vod);
							} catch (...) {
								CFRelease(decoded);
								return false;
							}
							CFRelease(decoded);
							if (!_fill_time_entry(packet.PacketRenderTime, packet.PacketRenderDuration, frame)) return false;
						} else return false;
					} else {
						int src_stride = 3 * _desc.Width;
						if (src_stride & 1) src_stride++;
						int src_size = src_stride * _desc.Height;
						if (packet.PacketDataActuallyUsed < src_size) return false;
						int stride = 4 * _desc.Width;
						int size = stride * _desc.Height;
						uint8 * buffer = reinterpret_cast<uint8 *>(malloc(size));
						if (!buffer) return false;
						for (int y = 0; y < _desc.Height; y++) {
							int line_from = y * src_stride;
							int line_to = y * stride;
							for (int x = 0; x < _desc.Width; x++) {
								buffer[line_to + 0] = packet.PacketData->ElementAt(line_from + 2);
								buffer[line_to + 1] = packet.PacketData->ElementAt(line_from + 1);
								buffer[line_to + 2] = packet.PacketData->ElementAt(line_from + 0);
								buffer[line_to + 3] = 0xFF;
								line_from += 3;
								line_to += 4;
							}
						}
						CVPixelBufferRef buffer_ref;
						if (CVPixelBufferCreateWithBytes(0, _desc.Width, _desc.Height, kCVPixelFormatType_32BGRA, buffer, stride, _buffer_release, buffer, 0, &buffer_ref)) {
							free(buffer);
							return false;
						}
						try {
							VideoObjectDesc vod;
							vod = _desc;
							vod.Device = 0;
							vod.FramePresentation = packet.PacketRenderTime;
							vod.FrameDuration = packet.PacketRenderDuration;
							SafePointer<IVideoFrame> frame = new CoreVideoFrame(buffer_ref, vod);
							frame_output_rec output;
							output.presentation = packet.PacketRenderTime;
							output.duration = packet.PacketRenderDuration;
							output.frame = frame;
							_clock = output.presentation;
							_frames.Append(output);
						} catch (...) { CFRelease(buffer_ref); return false; }
						CFRelease(buffer_ref);
					}
				} else _eos = true;
				return true;
			}
			virtual bool ReadFrame(IVideoFrame ** frame) noexcept override
			{
				if (_can_read_frame()) {
					*frame = _frames.FirstElement().frame;
					(*frame)->Retain();
					_clock += _frames.FirstElement().duration;
					_frames.RemoveFirst();
					return true;
				}
				if (_eos) {
					*frame = 0;
					_eos = false;
					return true;
				}
				return false;
			}
		};
		class VideoToolboxEncoder : public IVideoEncoder
		{
			SafePointer<Media::VideoTrackFormatDesc> _track;
			SafePointer<IVideoCodec> _codec;
			SafePointer<IVideoFactory> _factory;
			VideoObjectDesc _desc;
			string _format;
			VTCompressionSessionRef _session_enc;
			VTPixelTransferSessionRef _session_blt;
			bool _update_magic, _alive;
			Array<Media::PacketBuffer> _packets;
			SafePointer<Semaphore> _sync;
			uint64 _time;

			bool _update_codec_magic(CMFormatDescriptionRef desc) noexcept
			{
				int nalu_length_field;
				size_t nalu_count;
				if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(desc, 0, 0, 0, &nalu_count, &nalu_length_field)) return false;
				try {
					ObjectArray<DataBlock> sps(0x10), pps(0x10);
					for (size_t i = 0; i < nalu_count; i++) {
						size_t nalu_size;
						const uint8 * nalu_ptr;
						if (CMVideoFormatDescriptionGetH264ParameterSetAtIndex(desc, i, &nalu_ptr, &nalu_size, 0, 0)) return false;
						SafePointer<DataBlock> nalu = new DataBlock(nalu_size);
						nalu->SetLength(nalu_size);
						MemoryCopy(nalu->GetBuffer(), nalu_ptr, nalu_size);
						if (nalu->Length()) {
							auto nt = nalu->FirstElement() & 0x1F;
							if (nt == 0x07) sps.Append(nalu);
							else if (nt == 0x08) pps.Append(nalu);
						}
					}
					if (!sps.Length() || !pps.Length()) throw InvalidFormatException();
					SafePointer<DataBlock> codec_data = new DataBlock(0x100);
					codec_data->Append(0x01);
					codec_data->Append(sps.FirstElement()->ElementAt(1));
					codec_data->Append(sps.FirstElement()->ElementAt(2));
					codec_data->Append(sps.FirstElement()->ElementAt(3));
					codec_data->Append(0xFC | (nalu_length_field - 1));
					codec_data->Append(0xE0 | sps.Length());
					for (auto & nalu : sps) {
						codec_data->Append((nalu.Length() & 0xFF00) >> 8);
						codec_data->Append(nalu.Length() & 0xFF);
						codec_data->Append(nalu);
					}
					codec_data->Append(pps.Length());
					for (auto & nalu : pps) {
						codec_data->Append((nalu.Length() & 0xFF00) >> 8);
						codec_data->Append(nalu.Length() & 0xFF);
						codec_data->Append(nalu);
					}
					if (_track) _track->SetCodecMagic(codec_data);
				} catch (...) { return false; }
				return true;
			}
			static void _packet_callback(void * outputCallbackRefCon, void * sourceFrameRefCon, OSStatus status, VTEncodeInfoFlags infoFlags, CMSampleBufferRef sampleBuffer)
			{
				auto self = reinterpret_cast<VideoToolboxEncoder *>(outputCallbackRefCon);
				if (status || !sampleBuffer) {
					self->_alive = false;
					return;
				}
				auto decode = CMSampleBufferGetDecodeTimeStamp(sampleBuffer);
				auto present = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
				auto duration = CMSampleBufferGetDuration(sampleBuffer);
				self->_sync->Wait();
				self->_time += duration.value;
				if (self->_update_magic) {
					auto desc = CMSampleBufferGetFormatDescription(sampleBuffer);
					if (!self->_update_codec_magic(desc)) {
						self->_alive = false;
						self->_sync->Open();
						return;
					}
					self->_update_magic = false;
				}
				SafePointer<DataBlock> packet_block;
				try {
					packet_block = new DataBlock(1);
					auto data = CMSampleBufferGetDataBuffer(sampleBuffer);
					if (!data) throw Exception();
					auto length = CMBlockBufferGetDataLength(data);
					packet_block->SetLength(length);
					if (CMBlockBufferCopyDataBytes(data, 0, length, packet_block->GetBuffer())) throw Exception();
				} catch (...) {
					self->_alive = false;
					self->_sync->Open();
					return;
				}
				int p = -1;
				for (int i = 0; i < self->_packets.Length(); i++) if (self->_packets[i].PacketDecodeTime == decode.value) { p = i; break; }
				if (p < 0) {
					self->_alive = false;
					self->_sync->Open();
					return;
				}
				CFArrayRef prop_array = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, 0);
				if (prop_array) {
					CFDictionaryRef dict = reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(prop_array, 0));
					CFBooleanRef not_sync = dict ? reinterpret_cast<CFBooleanRef>(CFDictionaryGetValue(dict, kCMSampleAttachmentKey_NotSync)) : 0;
					self->_packets[p].PacketIsKey = not_sync ? (!CFBooleanGetValue(not_sync)) : true;
				} else self->_packets[p].PacketIsKey = true;
				self->_packets[p].PacketData = packet_block;
				self->_packets[p].PacketDataActuallyUsed = packet_block->Length();
				self->_packets[p].PacketRenderTime = present.value;
				for (int i = 0; i < p; i++) if (self->_packets[i].PacketDataActuallyUsed < 0) { self->_packets.SwapAt(i, p); break; }
				self->_sync->Open();
			}
		public:
			VideoToolboxEncoder(IVideoCodec * codec, const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options) : _packets(0x20)
			{
				if (format != VideoFormatH264 && format != VideoFormatRGB) throw InvalidFormatException();
				if (!desc.Width || !desc.Height || !desc.TimeScale || !desc.FrameDuration) throw InvalidFormatException();
				_factory = CreateVideoFactory();
				if (!_factory) throw Exception();
				_sync = CreateSemaphore(1);
				if (!_sync) throw Exception();
				_format = format;
				_codec.SetRetain(codec);
				_desc.Width = desc.Width;
				_desc.Height = desc.Height;
				_desc.TimeScale = desc.TimeScale;
				_desc.FrameDuration = desc.FrameDuration;
				_desc.FramePresentation = 0;
				_desc.Device = 0;
				_track = new Media::VideoTrackFormatDesc(_format, _desc.Width, _desc.Height, _desc.TimeScale, _desc.FrameDuration);
				if (VTPixelTransferSessionCreate(0, &_session_blt)) throw Exception();
				if (format == VideoFormatH264) {
					CFMutableDictionaryRef cprop = CFDictionaryCreateMutable(CFAllocatorGetDefault(), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
					if (!cprop) {
						VTPixelTransferSessionInvalidate(_session_blt);
						CFRelease(_session_blt);
						throw Exception();
					}
					if (desc.Device) CFDictionaryAddValue(cprop, kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder, kCFBooleanTrue);
					if (VTCompressionSessionCreate(0, _desc.Width, _desc.Height, kCMVideoCodecType_H264, cprop, 0, 0, _packet_callback, this, &_session_enc)) {
						VTPixelTransferSessionInvalidate(_session_blt);
						CFRelease(_session_blt);
						CFRelease(cprop);
						throw Exception();
					}
					CFRelease(cprop);
					for (uint i = 0; i < num_options; i++) {
						if (options[2 * i] == Media::MediaEncoderSuggestedBytesPerSecond) {
							uint bitrate = options[2 * i + 1] * 8;
							CFNumberRef num = CFNumberCreate(CFAllocatorGetDefault(), kCFNumberSInt32Type, &bitrate);
							if (num) {
								VTSessionSetProperty(_session_enc, kVTCompressionPropertyKey_AverageBitRate, num);
								CFRelease(num);
							}
						} else if (options[2 * i] == Media::MediaEncoderH264Profile) {
							if (options[2 * i + 1] == Media::H264ProfileBase) {
								VTSessionSetProperty(_session_enc, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_AutoLevel);
							} else if (options[2 * i + 1] == Media::H264ProfileMain) {
								VTSessionSetProperty(_session_enc, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Main_AutoLevel);
							} else if (options[2 * i + 1] == Media::H264ProfileHigh) {
								VTSessionSetProperty(_session_enc, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_High_AutoLevel);
							}
						} else if (options[2 * i] == Media::MediaEncoderMaxKeyframePeriod) {
							uint interval = options[2 * i + 1];
							CFNumberRef num = CFNumberCreate(CFAllocatorGetDefault(), kCFNumberSInt32Type, &interval);
							if (num) {
								VTSessionSetProperty(_session_enc, kVTCompressionPropertyKey_MaxKeyFrameInterval, num);
								CFRelease(num);
							}
						}
					}
					_update_magic = true;
				} else if (format == VideoFormatRGB) {
					_session_enc = 0;
					_update_magic = false;
				}
				_alive = true;
				_time = 0;
			}
			virtual ~VideoToolboxEncoder(void) override
			{
				if (_session_enc) {
					VTCompressionSessionInvalidate(_session_enc);
					CFRelease(_session_enc);
				}
				if (_session_blt) {
					VTPixelTransferSessionInvalidate(_session_blt);
					CFRelease(_session_blt);
				}
			}
			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override { return _desc; }
			virtual VideoObjectType GetObjectType(void) const noexcept override { return VideoObjectType::Encoder; }
			virtual handle GetBufferFormat(void) const noexcept override { return 0; }
			virtual IVideoCodec * GetParentCodec(void) const override { return _codec; }
			virtual string GetEncodedFormat(void) const override { return _format; }
			virtual bool Reset(void) noexcept override
			{
				if (_session_enc && VTCompressionSessionCompleteFrames(_session_enc, kCMTimeInvalid)) return false;
				_alive = true;
				_packets.Clear();
				_time = 0;
				return true;
			}
			virtual bool IsOutputAvailable(void) const noexcept override
			{
				bool result;
				_sync->Wait();
				result = _packets.Length() && _packets.FirstElement().PacketDataActuallyUsed != -1;
				_sync->Open();
				return result;
			}
			virtual const Media::VideoTrackFormatDesc & GetEncodedDescriptor(void) const noexcept override { return *_track; }
			virtual const DataBlock * GetCodecMagic(void) noexcept override { return _track->GetCodecMagic(); }
			virtual bool SupplyFrame(const IVideoFrame * frame, bool encode_keyframe) noexcept override
			{
				if (!frame) return false;
				SafePointer<IVideoFrame> copy = _factory->CreateFrame(Graphics::PixelFormat::B8G8R8A8_unorm, Codec::AlphaMode::Straight, _desc);
				if (!copy) return false;
				auto src_frame = static_cast<const CoreVideoFrame *>(frame);
				auto dest_frame = static_cast<const CoreVideoFrame *>(copy.Inner());
				if (VTPixelTransferSessionTransferImage(_session_blt, src_frame->GetSurface(), dest_frame->GetSurface())) return false;
				if (_session_enc) {
					_sync->Wait();
					try {
						Media::PacketBuffer packet;
						packet.PacketDataActuallyUsed = -1;
						packet.PacketIsKey = false;
						packet.PacketDecodeTime = frame->GetObjectDescriptor().FramePresentation;
						packet.PacketRenderTime = 0;
						packet.PacketRenderDuration = frame->GetObjectDescriptor().FrameDuration;
						_packets << packet;
					} catch (...) { _sync->Open(); return false; }
					_sync->Open();
					CFMutableDictionaryRef fprop = CFDictionaryCreateMutable(CFAllocatorGetDefault(), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
					if (!fprop) {
						_sync->Wait();
						_packets.RemoveLast();
						_sync->Open();
						return false;
					}
					if (encode_keyframe) CFDictionaryAddValue(fprop, kVTEncodeFrameOptionKey_ForceKeyFrame, kCFBooleanTrue);
					if (VTCompressionSessionEncodeFrame(_session_enc, dest_frame->GetSurface(),
						CMTimeMake(frame->GetObjectDescriptor().FramePresentation, _desc.TimeScale),
						CMTimeMake(frame->GetObjectDescriptor().FrameDuration, _desc.TimeScale), fprop, 0, 0)) {
						_sync->Wait();
						_packets.RemoveLast();
						_sync->Open();
						CFRelease(fprop);
						return false;
					}
					CFRelease(fprop);
					return _alive;
				} else {
					try {
						auto surface = dest_frame->GetSurface();
						Media::PacketBuffer packet;
						packet.PacketData = new DataBlock(1);
						auto dest_stride = 3 * _desc.Width;
						if (dest_stride & 1) dest_stride++;
						auto dest_size = dest_stride * _desc.Height;
						packet.PacketData->SetLength(dest_size);
						packet.PacketDataActuallyUsed = dest_size;
						if (CVPixelBufferLockBaseAddress(surface, kCVPixelBufferLock_ReadOnly)) throw Exception();
						auto base = CVPixelBufferGetBaseAddress(surface);
						auto stride = CVPixelBufferGetBytesPerRow(surface);
						for (int y = 0; y < _desc.Height; y++) {
							int line_from = stride * y;
							int line_to = dest_stride * y;
							for (int x = 0; x < _desc.Width; x++) {
								packet.PacketData->ElementAt(line_to + 0) = reinterpret_cast<const uint8 *>(base)[line_from + 2];
								packet.PacketData->ElementAt(line_to + 1) = reinterpret_cast<const uint8 *>(base)[line_from + 1];
								packet.PacketData->ElementAt(line_to + 2) = reinterpret_cast<const uint8 *>(base)[line_from + 0];
								line_to += 3;
								line_from += 4;
							}
						}
						CVPixelBufferUnlockBaseAddress(surface, kCVPixelBufferLock_ReadOnly);
						packet.PacketIsKey = true;
						packet.PacketRenderTime = packet.PacketDecodeTime = _time;
						packet.PacketRenderDuration = frame->GetObjectDescriptor().FrameDuration;
						_packets << packet;
						_time += packet.PacketRenderDuration;
					} catch (...) { return false; }
					return true;
				}
			}
			virtual bool SupplyEndOfStream(void) noexcept override
			{
				if (_session_enc && VTCompressionSessionCompleteFrames(_session_enc, kCMTimeInvalid)) return false;
				try {
					Media::PacketBuffer packet;
					packet.PacketDataActuallyUsed = 0;
					packet.PacketIsKey = true;
					packet.PacketDecodeTime = packet.PacketRenderTime = _time;
					packet.PacketRenderDuration = 0;
					_packets << packet;
				} catch (...) { return false; }
				_time = 0;
				return _alive;
			}
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept override
			{
				if (!_alive) return false;
				bool result;
				_sync->Wait();
				result = _packets.Length() && _packets.FirstElement().PacketDataActuallyUsed != -1;
				if (result) {
					packet = _packets.FirstElement();
					_packets.RemoveFirst();
					if (!packet.PacketDataActuallyUsed) _time = 0;
				}
				_sync->Open();
				return result && _alive;
			}
		};
		class VideoToolboxCodec : public IVideoCodec
		{
		public:
			VideoToolboxCodec(void) {}
			virtual ~VideoToolboxCodec(void) override {}
			virtual bool CanEncode(const string & format) const noexcept override { return format == VideoFormatH264 || format == VideoFormatRGB; }
			virtual bool CanDecode(const string & format) const noexcept override { return format == VideoFormatH264 || format == VideoFormatRGB; }
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(2);
				result->Append(VideoFormatH264);
				result->Append(VideoFormatRGB);
				result->Retain();
				return result;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(2);
				result->Append(VideoFormatH264);
				result->Append(VideoFormatRGB);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Video Toolbox Codec"; }
			virtual IVideoDecoder * CreateDecoder(const Media::TrackFormatDesc & format, Graphics::IDevice * acceleration_device) noexcept override
			{
				if (format.GetTrackClass() == Media::TrackClass::Video) {
					if (format.GetTrackCodec() == VideoFormatH264 || format.GetTrackCodec() == VideoFormatRGB) {
						try {
							return new VideoToolboxDecoder(this, format, acceleration_device);
						} catch (...) { return 0; }
					}
				} else return 0;
			}
			virtual IVideoEncoder * CreateEncoder(const string & format, const VideoObjectDesc & desc, uint num_options, const uint * options) noexcept override
			{
				try {
					return new VideoToolboxEncoder(this, format, desc, num_options, options);
				} catch (...) { return 0; }
			}
		};

		class VideoCaptureDevice : public IVideoDevice
		{
			struct _read_task
			{
				_read_task * next;
				bool * status;
				IVideoFrame ** frame;
				SafePointer<Semaphore> open;
				SafePointer<IDispatchTask> task;
			};

			AVCaptureDevice * _physical_device;
			AVCaptureScreenInput * _screen_input;
			AVCaptureInput * _input;
			AVCaptureVideoDataOutput * _output;
			AVCaptureSession * _session;
			ERTVideoCaptureDelegate * _delegate;
			VideoObjectDesc _desc;
			double _display_scale;
			CGDirectDisplayID _display;
			bool _format_set;
			SafePointer<Semaphore> _counter;
			SafePointer<Semaphore> _access;
			_read_task * _first, * _last;
			uint64 _time;
			AVCaptureDeviceFormat * _format_selected;

			void _set_desc(void)
			{
				_time = 0;
				_first = _last = 0;
				_counter = CreateSemaphore(0);
				_access = CreateSemaphore(1);
				_desc.Width = _desc.Height = _desc.TimeScale = _desc.FrameDuration = _desc.FramePresentation = 0;
				_desc.Device = 0;
				if (!_counter || !_access) throw Exception();
			}
			bool _append_queue(bool * status, IVideoFrame ** result, Semaphore * open, IDispatchTask * task)
			{
				auto rec = new (std::nothrow) _read_task;
				if (!rec) return false;
				rec->next = 0;
				rec->status = status;
				rec->frame = result;
				rec->open.SetRetain(open);
				rec->task.SetRetain(task);
				_access->Wait();
				if (_last) {
					_last->next = rec;
					_last = rec;
				} else _first = _last = rec;
				_counter->Open();
				_access->Open();
				return true;
			}
			static void _add_format_unique(Array<VideoObjectDesc> & result, VideoObjectDesc & desc)
			{
				for (auto & f : result) {
					if (f.Width == desc.Width && f.Height == desc.Height && f.TimeScale == desc.TimeScale && f.FrameDuration == desc.FrameDuration) return;
				}
				result.Append(desc);
			}
		public:
			void _input_frame(CMSampleBufferRef buffer) noexcept
			{
				_read_task * task;
				uint64 time;
				if (_counter->TryWait()) {
					_access->Wait();
					task = _first;
					_first = _first->next;
					if (!_first) _last = 0;
					time = _time;
					_time += _desc.FrameDuration;
					_access->Open();
				} else return;
				if (buffer) {
					auto image = CMSampleBufferGetImageBuffer(buffer);
					if (image) {
						auto vd = _desc;
						vd.FramePresentation = time;
						try {
							SafePointer<IVideoFrame> frame = new CoreVideoFrame(image, vd);
							if (task->status) *task->status = true;
							if (task->frame) {
								*task->frame = frame.Inner();
								frame->Retain();
							}
						} catch (...) {
							if (task->status) *task->status = false;
							if (task->frame) *task->frame = 0;
						}
					} else {
						if (task->status) *task->status = false;
						if (task->frame) *task->frame = 0;
					}
				} else {
					if (task->status) *task->status = false;
					if (task->frame) *task->frame = 0;
				}
				if (task->open) task->open->Open();
				if (task->task) task->task->DoTask(0);
				delete task;
			}
		public:
			VideoCaptureDevice(AVCaptureDevice * device) : _display_scale(0.0), _display(0), _format_set(false)
			{
				_set_desc();
				_physical_device = device;
				[_physical_device retain];
				_screen_input = 0;
				@autoreleasepool {
					NSError * error;
					_input = [AVCaptureDeviceInput deviceInputWithDevice: _physical_device error: &error];
					if (error) {
						[_physical_device release];
						[error release];
						throw Exception();
					} else [_input retain];
				}
				_output = [[AVCaptureVideoDataOutput alloc] init];
				_session = 0;
				_delegate = [[ERTVideoCaptureDelegate alloc] init];
				_delegate->device = this;
				[_output setSampleBufferDelegate: _delegate queue: dispatch_get_global_queue(QOS_CLASS_UTILITY, 0)];
			}
			VideoCaptureDevice(CGDirectDisplayID display, double scale) : _display_scale(scale), _format_set(false)
			{
				_display = display;
				_set_desc();
				_physical_device = 0;
				_screen_input = [[AVCaptureScreenInput alloc] initWithDisplayID: display];
				_input = _screen_input;
				[_input retain];
				_output = [[AVCaptureVideoDataOutput alloc] init];
				_session = 0;
				_delegate = [[ERTVideoCaptureDelegate alloc] init];
				_delegate->device = this;
				[_output setSampleBufferDelegate: _delegate queue: dispatch_get_global_queue(QOS_CLASS_UTILITY, 0)];
			}
			virtual ~VideoCaptureDevice(void) override
			{
				StopProcessing();
				[_physical_device release];
				[_screen_input release];
				[_input release];
				[_output release];
				[_session release];
				[_delegate release];
			}
			virtual const VideoObjectDesc & GetObjectDescriptor(void) const noexcept override { return _desc; }
			virtual VideoObjectType GetObjectType(void) const noexcept override { return VideoObjectType::Device; }
			virtual handle GetBufferFormat(void) const noexcept override { return 0; }
			virtual string GetDeviceIdentifier(void) const override { if (_physical_device) return Cocoa::EngineString([_physical_device uniqueID]); else return L""; }
			virtual Array<VideoObjectDesc> * GetSupportedFrameFormats(void) const noexcept override
			{
				try {
					SafePointer< Array<VideoObjectDesc> > result = new Array<VideoObjectDesc>(0x10);
					if (_physical_device) {
						auto formats = [_physical_device formats];
						for (int i = 0; i < [formats count]; i++) {
							auto format = [formats objectAtIndex: i];
							auto desc = [format formatDescription];
							auto dim = CMVideoFormatDescriptionGetDimensions(desc);
							auto ranges = [format videoSupportedFrameRateRanges];
							VideoObjectDesc vd;
							vd.FramePresentation = 0;
							vd.Device = 0;
							vd.Width = dim.width;
							vd.Height = dim.height;
							for (int j = 0; j < [ranges count]; j++) {
								auto range = [ranges objectAtIndex: j];
								auto min_duration = [range minFrameDuration];
								auto max_duration = [range maxFrameDuration];
								vd.FrameDuration = min_duration.value;
								vd.TimeScale = min_duration.timescale;
								if (min_duration.value != max_duration.value || min_duration.timescale != max_duration.timescale) {
									_add_format_unique(*result, vd);
									vd.FrameDuration = max_duration.value;
									vd.TimeScale = max_duration.timescale;
								}
								_add_format_unique(*result, vd);
							}
						}
					} else if (_screen_input) {
						auto bounds = CGDisplayBounds(_display).size;
						VideoObjectDesc vd;
						vd.FramePresentation = 0;
						vd.Device = 0;
						if (_display_scale > 1.0) {
							vd.Width = bounds.width * _display_scale;
							vd.Height = bounds.height * _display_scale;
							vd.TimeScale = 15;
							vd.FrameDuration = 1;
							result->Append(vd);
							vd.TimeScale = 1;
							vd.FrameDuration = 1;
							result->Append(vd);
						}
						vd.Width = bounds.width;
						vd.Height = bounds.height;
						vd.TimeScale = 15;
						vd.FrameDuration = 1;
						result->Append(vd);
						vd.TimeScale = 1;
						vd.FrameDuration = 1;
						result->Append(vd);
					} else return 0;
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual bool SetFrameFormat(const VideoObjectDesc & vd) noexcept override
			{
				if (_session) return false;
				if (_physical_device) {
					auto formats = [_physical_device formats];
					for (int i = 0; i < [formats count]; i++) {
						auto format = [formats objectAtIndex: i];
						auto desc = [format formatDescription];
						auto dim = CMVideoFormatDescriptionGetDimensions(desc);
						auto ranges = [format videoSupportedFrameRateRanges];
						if (dim.width != vd.Width || dim.height != vd.Height) continue;
						for (int j = 0; j < [ranges count]; j++) {
							auto range = [ranges objectAtIndex: j];
							auto min_duration = [range minFrameDuration];
							auto max_duration = [range maxFrameDuration];
							if (uint64(vd.FrameDuration) * min_duration.timescale < uint64(min_duration.value) * vd.TimeScale) continue;
							if (uint64(vd.FrameDuration) * max_duration.timescale > uint64(max_duration.value) * vd.TimeScale) continue;
							_desc.Width = vd.Width;
							_desc.Height = vd.Height;
							_desc.TimeScale = vd.TimeScale;
							_desc.FrameDuration = vd.FrameDuration;
							_format_selected = format;
							_format_set = true;
							return true;
						}
					}
					return false;
				} else if (_screen_input) {
					auto bounds = CGDisplayBounds(_display).size;
					if (vd.FrameDuration * 15 < vd.TimeScale) return false;
					if (vd.Width == uint(bounds.width) && vd.Height == uint(bounds.height)) {
						[_screen_input setScaleFactor: 1.0];
					} else if (vd.Width == uint(bounds.width * _display_scale) && vd.Height == uint(bounds.height * _display_scale)) {
						[_screen_input setScaleFactor: _display_scale];
					} else return false;
					_desc.Width = vd.Width;
					_desc.Height = vd.Height;
					_desc.TimeScale = vd.TimeScale;
					_desc.FrameDuration = vd.FrameDuration;
					_format_set = true;
					return true;
				} else return false;
			}
			virtual bool GetSupportedFrameRateRange(uint * min_rate_numerator, uint * min_rate_denominator, uint * max_rate_numerator, uint * max_rate_denominator) const noexcept override
			{
				if (_format_set) {
					if (_physical_device) {
						if (min_rate_numerator) *min_rate_numerator = _desc.TimeScale;
						if (min_rate_denominator) *min_rate_denominator = _desc.FrameDuration;
						if (max_rate_numerator) *max_rate_numerator = _desc.TimeScale;
						if (max_rate_denominator) *max_rate_denominator = _desc.FrameDuration;
						return true;
					} else if (_screen_input) {
						if (min_rate_numerator) *min_rate_numerator = 1;
						if (min_rate_denominator) *min_rate_denominator = 1;
						if (max_rate_numerator) *max_rate_numerator = 15;
						if (max_rate_denominator) *max_rate_denominator = 1;
						return true;
					} else return false;
				} else return false;
			}
			virtual bool SetFrameRate(uint rate_numerator, uint rate_denominator) noexcept override
			{
				if (_session) return false;
				if (_format_set) {
					if (_physical_device) {
						return rate_numerator == _desc.TimeScale && rate_denominator == _desc.FrameDuration;
					} else if (_screen_input) {
						if (rate_denominator * 15 < rate_numerator) return false;
						_desc.TimeScale = rate_numerator;
						_desc.FrameDuration = rate_denominator;
						return true;
					} else return false;
				} else return false;
			}
			virtual bool Initialize(void) noexcept override
			{
				if (_session || !_format_set) return false;
				if (_physical_device) {
				} else if (_screen_input) {
					[_screen_input setMinFrameDuration: CMTimeMake(_desc.FrameDuration, _desc.TimeScale)];
				} else return false;
				_session = [[AVCaptureSession alloc] init];
				[_session beginConfiguration];
				[_session addInput: _input];
				[_session addOutput: _output];
				[_session commitConfiguration];
				return true;
			}
			virtual bool StartProcessing(void) noexcept override
			{
				if (!_session && !Initialize()) return false;
				if (_physical_device) {
					if (![_physical_device lockForConfiguration: nil]) return false;
					[_physical_device setActiveFormat: _format_selected];
					[_session startRunning];
					[_physical_device unlockForConfiguration];
				} else {
					[_session startRunning];
				}
				return true;
			}
			virtual bool PauseProcessing(void) noexcept override
			{
				if (!_session) return false;
				[_session stopRunning];
				return true;
			}
			virtual bool StopProcessing(void) noexcept override
			{
				if (!_session) return false;
				[_session stopRunning];
				_access->Wait();
				while (_counter->TryWait()) {
					auto task = _first;
					_first = _first->next;
					if (!_first) _last = 0;
					if (task->status) *task->status = false;
					if (task->frame) *task->frame = 0;
					if (task->open) task->open->Open();
					if (task->task) task->task->DoTask(0);
					delete task;
				}
				_time = 0;
				_access->Open();
				return true;
			}
			virtual bool ReadFrame(IVideoFrame ** frame) noexcept override
			{
				try {
					SafePointer<Semaphore> sync = CreateSemaphore(0);
					bool result;
					if (!sync) return false;
					if (!ReadFrameAsync(frame, &result)) return false;
					sync->Wait();
					return result;
				} catch (...) { return false; }
			}
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status) noexcept override { return _append_queue(read_status, frame, 0, 0); }
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status, IDispatchTask * execute_on_processed) noexcept override { return _append_queue(read_status, frame, 0, execute_on_processed); }
			virtual bool ReadFrameAsync(IVideoFrame ** frame, bool * read_status, Semaphore * open_on_processed) noexcept override { return _append_queue(read_status, frame, open_on_processed, 0); }
		};
		class CoreVideoFactory : public IVideoFactory
		{
			static void _frame_release_callback(void * releaseRefCon, const void * baseAddress)
			{
				auto frame = reinterpret_cast<Codec::Frame *>(releaseRefCon);
				frame->Release();
			}
		public:
			CoreVideoFactory(void) {}
			virtual ~CoreVideoFactory(void) override {}
			virtual Volumes::Dictionary<string, string> * GetAvailableDevices(void) noexcept override
			{
				try {
					SafePointer< Volumes::Dictionary<string, string> > result = new Volumes::Dictionary<string, string>;
					@autoreleasepool {
						NSMutableArray<NSString *> * array = [NSMutableArray<NSString *> arrayWithCapacity: 10];
						[array addObject: AVCaptureDeviceTypeBuiltInWideAngleCamera];
						AVCaptureDeviceDiscoverySession * session = [AVCaptureDeviceDiscoverySession
							discoverySessionWithDeviceTypes: array
							mediaType: AVMediaTypeVideo
							position: AVCaptureDevicePositionUnspecified];
						int count = [[session devices] count];
						for (int i = 0; i < count; i++) {
							auto device = [[session devices] objectAtIndex: i];
							auto name = Cocoa::EngineString([device localizedName]);
							auto key = Cocoa::EngineString([device uniqueID]);
							result->Append(key, name);
						}
					}
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IVideoDevice * CreateDevice(const string & identifier, Graphics::IDevice * acceleration_device = 0) noexcept override
			{
				try {
					@autoreleasepool {
						auto ident = Cocoa::CocoaString(identifier);
						[ident autorelease];
						auto device = [AVCaptureDevice deviceWithUniqueID: ident];
						if (!device) return 0;
						return new VideoCaptureDevice(device);
					}
				} catch (...) { return 0; }
			}
			virtual IVideoDevice * CreateDefaultDevice(Graphics::IDevice * acceleration_device = 0) noexcept override
			{
				try {
					@autoreleasepool {
						auto device = [AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeVideo];
						if (!device) return 0;
						return new VideoCaptureDevice(device);
					}
				} catch (...) { return 0; }
			}
			virtual IVideoDevice * CreateScreenCaptureDevice(Windows::IScreen * screen, Graphics::IDevice * acceleration_device = 0) noexcept override
			{
				try {
					if (!screen) return 0;
					return new VideoCaptureDevice(Cocoa::GetDirectDisplayID(screen), screen->GetDpiScale());
				} catch (...) { return 0; }
			}
			virtual IVideoFrame * CreateFrame(Codec::Frame * frame, Graphics::IDevice * acceleration_device = 0) noexcept override
			{
				if (!frame) return 0;
				CVPixelBufferRef buffer = 0;
				try {
					SafePointer<Codec::Frame> copy = frame->ConvertFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
					VideoObjectDesc desc;
					desc.Width = copy->GetWidth();
					desc.Height = copy->GetHeight();
					desc.TimeScale = desc.FramePresentation = desc.FrameDuration = 0;
					desc.Device = 0;
					if (CVPixelBufferCreateWithBytes(0, desc.Width, desc.Height, kCVPixelFormatType_32BGRA, copy->GetData(),
						copy->GetScanLineLength(), _frame_release_callback, copy.Inner(), 0, &buffer)) return 0;
					copy->Retain();
					SafePointer<IVideoFrame> result = new CoreVideoFrame(buffer, desc);
					CFRelease(buffer);
					buffer = 0;
					result->Retain();
					return result;
				} catch (...) { if (buffer) CFRelease(buffer); return 0; }
			}
			virtual IVideoFrame * CreateFrame(Graphics::PixelFormat format, Codec::AlphaMode alpha, const VideoObjectDesc & desc) noexcept override
			{
				if (format != Graphics::PixelFormat::B8G8R8A8_unorm) return 0;
				if (!desc.Width || !desc.Height) return 0;
				CVPixelBufferRef buffer = 0;
				try {
					SafePointer<Codec::Frame> copy = new Codec::Frame(desc.Width, desc.Height, Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
					VideoObjectDesc vd;
					vd.Width = desc.Width;
					vd.Height = desc.Height;
					vd.TimeScale = desc.TimeScale;
					vd.FramePresentation = desc.FramePresentation;
					vd.FrameDuration = desc.FrameDuration;
					vd.Device = 0;
					if (CVPixelBufferCreateWithBytes(0, desc.Width, desc.Height, kCVPixelFormatType_32BGRA, copy->GetData(),
						copy->GetScanLineLength(), _frame_release_callback, copy.Inner(), 0, &buffer)) return 0;
					copy->Retain();
					SafePointer<IVideoFrame> result = new CoreVideoFrame(buffer, desc);
					CFRelease(buffer);
					buffer = 0;
					result->Retain();
					return result;
				} catch (...) { if (buffer) CFRelease(buffer); return 0; }
			}
			virtual IVideoFrameBlt * CreateFrameBlt(void) noexcept override { try { return new VideoToolboxTransferer; } catch (...) { return 0; } }
		};

		SafePointer<IVideoCodec> _system_codec;
		IVideoCodec * InitializeSystemCodec(void)
		{
			if (!_system_codec) {
				_system_codec = new VideoToolboxCodec;
				RegisterCodec(_system_codec);
			}
			return _system_codec;
		}
		IVideoFactory * CreateSystemVideoFactory(void) { return new CoreVideoFactory; }
	}
}

@implementation ERTVideoCaptureDelegate
	- (void) captureOutput: (AVCaptureOutput *) output didOutputSampleBuffer: (CMSampleBufferRef) sampleBuffer fromConnection: (AVCaptureConnection *) connection
	{
		static_cast<Engine::Video::VideoCaptureDevice *>(device)->_input_frame(sampleBuffer);
	}
	- (void) captureOutput: (AVCaptureOutput *) output didDropSampleBuffer: (CMSampleBufferRef) sampleBuffer fromConnection: (AVCaptureConnection *) connection
	{
		static_cast<Engine::Video::VideoCaptureDevice *>(device)->_input_frame(0);
	}
@end