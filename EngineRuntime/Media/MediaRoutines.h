#pragma once

#include "Audio.h"
#include "Video.h"
#include "Subtitles.h"

namespace Engine
{
	namespace Media
	{
		class IAudioDeviceSelector
		{
		public:
			virtual Object * ObjectAudioInterface(void) noexcept = 0;
			virtual bool ResetOnDefaultDeviceChanged(void) noexcept = 0;
			virtual void OnAudioDeviceLost(void) noexcept = 0;
			virtual Audio::IAudioOutputDevice * ProvideAudioDevice(Audio::IAudioDeviceFactory * factory) noexcept = 0;
			virtual void ProvideChannelMatrix(Audio::IAudioOutputDevice * device, const Audio::StreamDesc & desc, uint channel_layout, double * matrix) noexcept = 0;
		};
		class IVideoPresenter
		{
		public:
			virtual Object * ObjectVideoInterface(void) noexcept = 0;
			virtual IDispatchQueue * ProvideBltContext(void) noexcept = 0;
			virtual Graphics::IDevice * ProvideVideoDevice(void) noexcept = 0;
			virtual Graphics::ITexture * ProvideVideoBuffer(uint width, uint height) noexcept = 0;
			virtual void PresentVideoFrame(uint array_index) noexcept = 0;
		};
		class ISubtitlesPresenter
		{
		public:
			virtual Object * ObjectSubtitlesInterface(void) noexcept = 0;
			virtual void PresentSubtitleText(const string & text, uint flags) noexcept = 0;
		};
		class IPlaybackRoutineCallback
		{
		public:
			virtual void OnEndOfStream(void) noexcept = 0;
			virtual void OnPaused(void) noexcept = 0;
			virtual void OnStarted(void) noexcept = 0;
			virtual void OnSeeked(void) noexcept = 0;
			virtual void OnError(uint error_code) noexcept = 0;
		};
		enum PlaybackErrors : uint {
			PlaybackSuccess = 0x00000000,
			PlaybackErrorCreateDecoderFailure = 0x00000001,
			PlaybackErrorDecodeFailure = 0x00000002,
			PlaybackErrorSynchronizationFailure = 0x00000004,
			PlaybackErrorAllocationFailure = 0x00000008,
			PlaybackErrorDeviceStartupFailure = 0x00000010,
			PlaybackErrorDeviceControlFailure = 0x00000020,
			PlaybackErrorFrameDropped = 0x00000040,
			PlaybackErrorReasonMask = 0x0FFFFFFF,
			PlaybackErrorDomainCommon = 0x10000000,
			PlaybackErrorDomainAudio = 0x20000000,
			PlaybackErrorDomainVideo = 0x30000000,
			PlaybackErrorDomainSubtitles = 0x40000000,
			PlaybackErrorDomainMask = 0xF0000000
		};

		class IPlaybackRoutine : public Object
		{
		public:
			virtual void SetCallback(IPlaybackRoutineCallback * callback) noexcept = 0;
			virtual IPlaybackRoutineCallback * GetCallback(void) noexcept = 0;
			virtual bool AttachInput(Media::IMediaTrackSource * track) noexcept = 0;
			virtual bool AttachAudioOutput(IAudioDeviceSelector * output) noexcept = 0;
			virtual bool AttachVideoOutput(IVideoPresenter * output) noexcept = 0;
			virtual bool AttachSubtitleOutput(ISubtitlesPresenter * output) noexcept = 0;

			virtual bool Play(void) noexcept = 0;
			virtual bool Pause(void) noexcept = 0;
			virtual bool Seek(uint time) noexcept = 0;
			virtual bool SetVolume(double volume) noexcept = 0;
			virtual double GetVolume(void) noexcept = 0;

			virtual uint GetCurrentTime(void) noexcept = 0;
			virtual uint GetDuration(void) noexcept = 0;
			virtual bool IsPlaying(void) noexcept = 0;
			virtual bool IsEndOfStream(void) noexcept = 0;
			virtual uint GetErrorState(void) noexcept = 0;
			virtual bool InvalidateAudioDevice(void) noexcept = 0;
		};
		class ICaptureSink : public Object
		{
		public:
			virtual void OnStarted(void) noexcept = 0;
			virtual void OnStopped(void) noexcept = 0;
			virtual void OnError(uint error_code) noexcept = 0;
			virtual void HandleAudioFormat(int stream_index, const Audio::StreamDesc & desc, uint channel_layout) noexcept = 0;
			virtual void HandleVideoFormat(int stream_index, const Video::VideoObjectDesc & desc) noexcept = 0;
			virtual void HandleAudioInput(int stream_index, Audio::WaveBuffer * wave) noexcept = 0;
			virtual void HandleVideoInput(int stream_index, Video::IVideoFrame * frame) noexcept = 0;
		};
		class IMirrorCaptureSink : public ICaptureSink
		{
		public:
			virtual void SetCallback(IPlaybackRoutineCallback * callback) noexcept = 0;
			virtual IPlaybackRoutineCallback * GetCallback(void) noexcept = 0;
			virtual bool AttachAudioOutput(int stream_index_for, IAudioDeviceSelector * output) noexcept = 0;
			virtual bool AttachVideoOutput(int stream_index_for, IVideoPresenter * output) noexcept = 0;
			virtual bool SetVolume(double volume) noexcept = 0;
			virtual double GetVolume(void) noexcept = 0;
			virtual bool InvalidateAudioDevice(void) noexcept = 0;
		};
		class ICaptureRoutine : public Object
		{
		public:
			virtual bool SetInputAudio(int stream_index, Audio::IAudioInputDevice * device) noexcept = 0;
			virtual bool SetInputVideo(int stream_index, Video::IVideoDevice * device) noexcept = 0;
			virtual bool AddOutputSink(ICaptureSink * sink) noexcept = 0;
			virtual bool RemoveOutputSink(ICaptureSink * sink) noexcept = 0;
			virtual bool SetAudioLatency(uint time) noexcept = 0;
			virtual bool Start(void) noexcept = 0;
			virtual bool Stop(void) noexcept = 0;
			virtual bool IsPlaying(void) noexcept = 0;
		};

		IPlaybackRoutine * CreatePlaybackRoutine(void);
		IMirrorCaptureSink * CreateMirrorCaptureSink(void);
		ICaptureRoutine * CreateCaptureRoutine(void);
		void SetDefaultAudioDeviceSelector(IPlaybackRoutine * routine);
		void SetDefaultAudioDeviceSelector(int index_at, IMirrorCaptureSink * sink);

		uint GetChannelRole(uint layout, uint index);
		bool IsChannelLeft(uint role);
		bool IsChannelRight(uint role);
		bool IsChannelCenter(uint role);
		bool IsChannelSurround(uint role);
		void MakeStandardRemuxMatrix(double * matrix, uint input_channels, uint input_layout, uint output_channels, uint output_layout);
	}
}