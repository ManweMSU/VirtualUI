#pragma once

#include "AudioBase.h"
#include "Media.h"
#include "../Miscellaneous/ThreadPool.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Audio
	{
		constexpr const widechar * AudioFormatEngineWaveform = L"RAW";
		constexpr const widechar * AudioFormatMicrosoftWaveform = L"WAV";
		constexpr const widechar * AudioFormatMP3 = L"MP3";
		constexpr const widechar * AudioFormatAAC = L"AAC";
		constexpr const widechar * AudioFormatFreeLossless = L"FLAC";
		constexpr const widechar * AudioFormatAppleLossless = L"ALAC";

		enum class AudioObjectType { Unknown, DeviceOutput, DeviceInput, Encoder, Decoder, StreamEncoder, StreamDecoder };
		enum class AudioDeviceEvent { Activated, Inactivated, DefaultChanged };

		enum ChannelLayoutFlags : uint {
			ChannelLayoutLeft = 0x00001, ChannelLayoutRight = 0x00002, ChannelLayoutCenter = 0x00004,
			ChannelLayoutLowFrequency = 0x00008,
			ChannelLayoutBackLeft = 0x00010, ChannelLayoutBackRight = 0x00020,
			ChannelLayoutFrontLeft = 0x00040, ChannelLayoutFrontRight = 0x00080,
			ChannelLayoutBackCenter = 0x00100,
			ChannelLayoutSideLeft = 0x00200, ChannelLayoutSideRight = 0x00400,
			ChannelLayoutTopCenter = 0x00800,
			ChannelLayoutTopFrontLeft = 0x01000, ChannelLayoutTopFrontCenter = 0x02000, ChannelLayoutTopFrontRight = 0x04000,
			ChannelLayoutTopBackLeft = 0x08000, ChannelLayoutTopBackCenter = 0x10000, ChannelLayoutTopBackRight = 0x20000,
		};

		class IAudioObject;
		class IAudioSource;
		class IAudioSink;
		class IAudioCodec;
		class IAudioDecoder;
		class IAudioEncoder;

		class IAudioObject : public Object
		{
		public:
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept = 0;
			virtual uint GetChannelLayout(void) const noexcept = 0;
			virtual AudioObjectType GetObjectType(void) const noexcept = 0;
		};

		class IAudioCodec : public Object
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept = 0;
			virtual bool CanDecode(const string & format) const noexcept = 0;
			virtual Array<string> * GetFormatsCanEncode(void) const = 0;
			virtual Array<string> * GetFormatsCanDecode(void) const = 0;
			virtual string GetCodecName(void) const = 0;
			virtual IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc) noexcept = 0;
			virtual IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options, const uint * options) noexcept = 0;
		};
		class IAudioSession : public IAudioObject
		{
		public:
			virtual IAudioCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept = 0;
			virtual bool Reset(void) noexcept = 0;
			virtual int GetPendingPacketsCount(void) const noexcept = 0;
			virtual int GetPendingFramesCount(void) const noexcept = 0;
		};
		class IAudioDecoder : public IAudioSession
		{
		public:
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept = 0;
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept = 0;
		};
		class IAudioEncoder : public IAudioSession
		{
		public:
			virtual const Media::AudioTrackFormatDesc & GetFullEncodedDescriptor(void) const noexcept = 0;
			virtual const DataBlock * GetCodecMagic(void) noexcept = 0;
			virtual bool SupplyFrames(const WaveBuffer * buffer) noexcept = 0;
			virtual bool SupplyEndOfStream(void) noexcept = 0;
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept = 0;
		};

		class IAudioStream : public IAudioObject
		{
		public:
			virtual IAudioCodec * GetParentCodec(void) const = 0;
			virtual string GetEncodedFormat(void) const = 0;
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept = 0;
		};
		class AudioDecoderStream : public IAudioStream
		{
			SafePointer<Media::IMediaContainerSource> _container;
			SafePointer<Media::IMediaTrackSource> _track;
			SafePointer<IAudioDecoder> _decoder;
			Media::PacketBuffer _packet;
			bool _eos;
		public:
			AudioDecoderStream(Media::IMediaTrackSource * source, const StreamDesc * desired_desc = 0);
			AudioDecoderStream(Media::IMediaContainerSource * source, const StreamDesc * desired_desc = 0);
			AudioDecoderStream(Streaming::Stream * source, const StreamDesc * desired_desc = 0);
			virtual ~AudioDecoderStream(void) override;

			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override;
			virtual AudioObjectType GetObjectType(void) const noexcept override;
			virtual IAudioCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override;
			virtual uint GetChannelLayout(void) const noexcept override;

			virtual Media::IMediaTrackSource * GetSourceTrack(void) noexcept;
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept;
			virtual uint64 GetFramesCount(void) const noexcept;
			virtual uint64 GetCurrentFrame(void) const noexcept;
			virtual uint64 SetCurrentFrame(uint64 frame_index) noexcept;
		};
		class AudioEncoderStream : public IAudioStream
		{
			SafePointer<Media::IMediaContainerSink> _container;
			SafePointer<Media::IMediaTrackSink> _track;
			SafePointer<IAudioEncoder> _encoder;

			void _init(Media::IMediaContainerSink * dest, const string & format, const StreamDesc & desc, uint num_options, const uint * options);
			bool _drain_packets(void) noexcept;
		public:
			AudioEncoderStream(Media::IMediaContainerSink * dest, const string & format, const StreamDesc & desc, uint num_options = 0, const uint * options = 0);
			AudioEncoderStream(Streaming::Stream * dest, const string & media_format, const string & audio_format, const StreamDesc & desc, uint num_options = 0, const uint * options = 0);
			virtual ~AudioEncoderStream(void) override;

			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept override;
			virtual AudioObjectType GetObjectType(void) const noexcept override;
			virtual IAudioCodec * GetParentCodec(void) const override;
			virtual string GetEncodedFormat(void) const override;
			virtual const StreamDesc & GetEncodedDescriptor(void) const noexcept override;
			virtual uint GetChannelLayout(void) const noexcept override;

			virtual Media::IMediaTrackSink * GetDestinationSink(void) noexcept;
			virtual bool WriteFrames(const WaveBuffer * buffer) noexcept;
			virtual bool Finalize(void) noexcept;
		};

		class IAudioEventCallback
		{
		public:
			virtual void OnAudioDeviceEvent(AudioDeviceEvent event, AudioObjectType device_type, const string & device_identifier) noexcept = 0;
		};
		class IAudioDevice : public IAudioObject
		{
		public:
			virtual string GetDeviceIdentifier(void) const = 0;
			virtual double GetVolume(void) noexcept = 0;
			virtual void SetVolume(double volume) noexcept = 0;
			virtual bool StartProcessing(void) noexcept = 0;
			virtual bool PauseProcessing(void) noexcept = 0;
			virtual bool StopProcessing(void) noexcept = 0;
		};
		class IAudioOutputDevice : public IAudioDevice
		{
		public:
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept = 0;
		};
		class IAudioInputDevice : public IAudioDevice
		{
		public:
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept = 0;
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status) noexcept = 0;
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, IDispatchTask * execute_on_processed) noexcept = 0;
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * read_status, Semaphore * open_on_processed) noexcept = 0;
		};
		class IAudioDeviceFactory : public Object
		{
		public:
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableOutputDevices(void) noexcept = 0;
			virtual Dictionary::PlainDictionary<string, string> * GetAvailableInputDevices(void) noexcept = 0;
			virtual IAudioOutputDevice * CreateOutputDevice(const string & identifier) noexcept = 0;
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept = 0;
			virtual IAudioInputDevice * CreateInputDevice(const string & identifier) noexcept = 0;
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept = 0;
			virtual bool RegisterEventCallback(IAudioEventCallback * callback) noexcept = 0;
			virtual bool UnregisterEventCallback(IAudioEventCallback * callback) noexcept = 0;
		};

		void InitializeDefaultCodecs(void);
		void RegisterCodec(IAudioCodec * codec);
		void UnregisterCodec(IAudioCodec * codec);

		Array<string> * GetEncodeFormats(void);
		Array<string> * GetDecodeFormats(void);
		IAudioCodec * FindEncoder(const string & format);
		IAudioCodec * FindDecoder(const string & format);
		IAudioDecoder * CreateDecoder(const Media::TrackFormatDesc & format, const StreamDesc * desired_desc = 0);
		IAudioEncoder * CreateEncoder(const string & format, const StreamDesc & desc, uint num_options = 0, const uint * options = 0);

		IAudioDeviceFactory * CreateAudioDeviceFactory(void);
		void Beep(void);
	}
}