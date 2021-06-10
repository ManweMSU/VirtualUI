#pragma once

#include "../Streaming.h"
#include "../Miscellaneous/ThreadPool.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Audio
	{
		constexpr const widechar * AudioFormatWaveform = L"WAV";
		constexpr const widechar * AudioFormatEngineRaw = L"ERAU";
		constexpr const widechar * AudioFormatMPEG3 = L"MP3";
		constexpr const widechar * AudioFormatMPEG4AAC = L"AAC";
		constexpr const widechar * AudioFormatAppleLossless = L"ALAC";

		enum class SampleFormat : uint {
			Invalid = 0,
			S8_snorm = 0x00010008, S16_snorm = 0x00010010, S24_snorm = 0x00010018, S32_snorm = 0x00010020,
			S32_float = 0x00020020, S64_float = 0x00020040
		};
		enum class AudioObjectType { Unknown, DeviceOutput, DeviceInput, StreamEncoder, StreamDecoder };
		struct StreamDesc
		{
			SampleFormat Format;
			uint ChannelCount;
			uint FramesPerSecond;

			StreamDesc(void);
			StreamDesc(SampleFormat format, uint num_channels, uint frames_per_second);
			operator string(void) const;
		};

		bool IsFloatingPointFormat(SampleFormat format);
		uint SampleFormatBitSize(SampleFormat format);
		uint SampleFormatByteSize(SampleFormat format);
		uint StreamFrameByteSize(const StreamDesc & format);
		uint64 ConvertFrameCount(uint64 frame_count, uint old_frame_rate, uint new_frame_rate);

		class WaveBuffer : public Object
		{
			StreamDesc desc;
			uint64 frames_allocated;
			uint64 frames_used;
			uint8 * frame_buffer;
		public:
			WaveBuffer(const WaveBuffer & src);
			WaveBuffer(const WaveBuffer * src);
			WaveBuffer(SampleFormat format, uint num_channels, uint frames_per_second, uint size_frames);
			WaveBuffer(const StreamDesc & desc, uint size_frames);
			virtual ~WaveBuffer(void) override;

			const StreamDesc & GetFormatDescriptor(void) const;
			uint64 GetSizeInFrames(void) const;
			uint64 & FramesUsed(void);
			uint64 FramesUsed(void) const;

			uint8 * GetData(void);
			const uint8 * GetData(void) const;
			uint64 GetAllocatedSizeInBytes(void) const;
			uint64 GetUsedSizeInBytes(void) const;
			void ReinterpretFrames(uint frames_per_second);

			int32 ReadSampleInteger(uint64 frame_index, uint num_channel) const;
			double ReadSampleFloat(uint64 frame_index, uint num_channel) const;
			void WriteSample(uint64 frame_index, uint num_channel, int32 value);
			void WriteSample(uint64 frame_index, uint num_channel, double value);

			WaveBuffer * ConvertFormat(SampleFormat format) const;
			WaveBuffer * ConvertFrameRate(uint32 frames_per_second) const;
			WaveBuffer * RemuxChannels(uint num_channels, const double * remux_matrix) const;
			WaveBuffer * ReorderChannels(uint num_channels, const uint * indicies) const;
			WaveBuffer * ReallocateChannels(uint num_channels) const;

			void ConvertFormat(WaveBuffer * buffer, SampleFormat format) const;
			void ConvertFrameRate(WaveBuffer * buffer, uint32 frames_per_second) const;
			void RemuxChannels(WaveBuffer * buffer, uint num_channels, const double * remux_matrix) const;
			void ReorderChannels(WaveBuffer * buffer, uint num_channels, const uint * indicies) const;
			void ReallocateChannels(WaveBuffer * buffer, uint num_channels) const;
		};

		class IAudioObject;
		class IAudioSource;
		class IAudioSink;
		class IAudioCodec;
		class IAudioDecoderStream;
		class IAudioEncoderStream;

		class IAudioObject : public Object
		{
		public:
			virtual const StreamDesc & GetFormatDescriptor(void) const noexcept = 0;
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
			virtual IAudioDecoderStream * TryDecode(Streaming::Stream * source, const StreamDesc * desired_desc) noexcept = 0;
			virtual IAudioEncoderStream * Encode(Streaming::Stream * dest, const string & format, const StreamDesc & desc) noexcept = 0;
		};
		class IAudioStream : public IAudioObject
		{
		public:
			virtual IAudioCodec * GetParentCodec(void) const = 0;
			virtual string GetInternalFormat(void) const = 0;
			virtual const StreamDesc & GetNativeDescriptor(void) const noexcept = 0;
		};
		class IAudioDecoderStream : public IAudioStream
		{
		public:
			virtual bool ReadFrames(WaveBuffer * buffer) noexcept = 0;
			virtual uint64 GetFramesCount(void) const = 0;
			virtual uint64 GetCurrentFrame(void) const = 0;
			virtual bool SetCurrentFrame(uint64 frame_index) = 0;
		};
		class IAudioEncoderStream : public IAudioStream
		{
		public:
			virtual bool WriteFrames(WaveBuffer * buffer) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept = 0;
			virtual bool WriteFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept = 0;
			virtual bool Finalize(void) noexcept = 0;
		};

		class IAudioDevice : public IAudioObject
		{
		public:
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
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status) noexcept = 0;
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, IDispatchTask * execute_on_processed) noexcept = 0;
			virtual bool ReadFramesAsync(WaveBuffer * buffer, bool * write_status, Semaphore * open_on_processed) noexcept = 0;
		};
		class IAudioDeviceFactory : public Object
		{
		public:
			virtual Dictionary::PlainDictionary<uint64, string> * GetAvailableOutputDevices(void) noexcept = 0;
			virtual Dictionary::PlainDictionary<uint64, string> * GetAvailableInputDevices(void) noexcept = 0;
			virtual IAudioOutputDevice * CreateOutputDevice(uint64 identifier) noexcept = 0;
			virtual IAudioOutputDevice * CreateDefaultOutputDevice(void) noexcept = 0;
			virtual IAudioInputDevice * CreateInputDevice(uint64 identifier) noexcept = 0;
			virtual IAudioInputDevice * CreateDefaultInputDevice(void) noexcept = 0;
		};

		void InitializeDefaultCodecs(void);
		void RegisterCodec(IAudioCodec * codec);
		void UnregisterCodec(IAudioCodec * codec);

		IAudioCodec * FindEncoder(const string & format);
		IAudioCodec * FindDecoder(const string & format);
		IAudioDecoderStream * DecodeAudio(Streaming::Stream * stream, const StreamDesc * desired_desc = 0);
		IAudioEncoderStream * EncodeAudio(Streaming::Stream * stream, const string & format, const StreamDesc & desc);

		IAudioDeviceFactory * CreateAudioDeviceFactory(void);
		void Beep(void);
	}
}