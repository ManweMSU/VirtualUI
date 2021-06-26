#include "MediaRoutines.h"

namespace Engine
{
	namespace Media
	{
		class PlaybackRoutine : public IPlaybackRoutine
		{
			SafePointer<IMediaTrackSource> _audio_in, _video_in, _subtitle_in;
			SafePointer<Object> _audio_obj, _video_obj, _subtitle_obj;
			SafePointer<Audio::AudioDecoderStream> _audio_stream;
			SafePointer<Video::VideoDecoderStream> _video_stream;
			SafePointer<Subtitles::SubtitleDecoderStream> _subtitle_stream;
			IPlaybackRoutineCallback * _callback;
			IAudioDeviceSelector * _audio_out;
			IVideoPresenter * _video_out;
			ISubtitlesPresenter * _subtitle_out;
			SafePointer<Thread> _dispatch;
			SafePointer<Scheduler> _scheduler;
			volatile int _state; // 1 - playing, 2 - end of stream, 4 - never played
			volatile uint _media_time, _error_state;
			volatile uint _streams_active;
			uint _duration, _playback_base;

			void _stream_shutdown(void)
			{
				_streams_active--;
				if (!_streams_active) {
					_state |= 0x02;
					_state &= ~0x01;
					_media_time = 0;
					if (_callback) _callback->OnEndOfStream();
				}
			}
			void _raise_error(uint error_code)
			{
				_error_state = error_code;
				if (_callback && error_code) _callback->OnError(error_code);
			}

			class AudioPlaybackTask : public ISchedulerTask, public Audio::IAudioEventCallback
			{
				PlaybackRoutine * _parent;
				SafePointer<Semaphore> _audio_sync;
				SafePointer<Audio::IAudioDeviceFactory> _factory;
				SafePointer<Audio::IAudioOutputDevice> _device;
				SafePointer<Audio::WaveBuffer> _read, _used_now, _pending;
				Array<double> _matrix;
				uint _pending_ts;
				uint _current_frame;
				uint _pending_packets;
				bool _device_reset;

				bool ResetDevice(bool send_lost = false)
				{
					if (!_device || send_lost) {
						if (send_lost) _parent->_audio_out->OnAudioDeviceLost();
						_device = _parent->_audio_out->ProvideAudioDevice(_factory);
						if (!_device) {
							_parent->_raise_error(PlaybackErrorDeviceStartupFailure | PlaybackErrorDomainAudio);
							return false;
						}
						_device->SetVolume(_volume);
						try {
							_matrix.SetLength(_device->GetFormatDescriptor().ChannelCount * _parent->_audio_stream->GetFormatDescriptor().ChannelCount);
							_parent->_audio_out->ProvideChannelMatrix(_device, _parent->_audio_stream->GetFormatDescriptor(), _parent->_audio_stream->GetChannelLayout(), _matrix.GetBuffer());
							_read = new Audio::WaveBuffer(_parent->_audio_stream->GetFormatDescriptor(), _parent->_audio_stream->GetFormatDescriptor().FramesPerSecond);
							_used_now = new Audio::WaveBuffer(_device->GetFormatDescriptor(), _device->GetFormatDescriptor().FramesPerSecond);
							_pending = new Audio::WaveBuffer(_device->GetFormatDescriptor(), _device->GetFormatDescriptor().FramesPerSecond);
						} catch (...) {
							_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
							_device.SetReference(0);
							return false;
						}
						_device_reset = true;
					}
					return true;
				}
				bool ResampleRead(void)
				{
					try {
						auto & rd = _read->GetFormatDescriptor();
						auto & dd = _device->GetFormatDescriptor();
						if (rd.ChannelCount == dd.ChannelCount && rd.FramesPerSecond == dd.FramesPerSecond && rd.Format == dd.Format) {
							MemoryCopy(_pending->GetData(), _read->GetData(), _read->GetUsedSizeInBytes());
						} else {
							if (rd.ChannelCount == dd.ChannelCount && rd.FramesPerSecond == dd.FramesPerSecond) {
								_read->ConvertFormat(_pending, dd.Format);
							} else if (rd.ChannelCount == dd.ChannelCount && rd.Format == dd.Format) {
								_read->ConvertFrameRate(_pending, dd.FramesPerSecond);
							} else if (rd.FramesPerSecond == dd.FramesPerSecond && rd.Format == dd.Format) {
								_read->RemuxChannels(dd.ChannelCount, _matrix.GetBuffer());
							} else if (rd.ChannelCount == dd.ChannelCount) {
								SafePointer<Audio::WaveBuffer> wave = _read->ConvertFormat(dd.Format);
								wave->ConvertFrameRate(_pending, dd.FramesPerSecond);
							} else if (rd.FramesPerSecond == dd.FramesPerSecond) {
								SafePointer<Audio::WaveBuffer> wave = _read->ConvertFormat(dd.Format);
								wave->RemuxChannels(_pending, dd.ChannelCount, _matrix.GetBuffer());
							} else if (rd.Format == dd.Format) {
								SafePointer<Audio::WaveBuffer> wave = _read->ConvertFrameRate(dd.FramesPerSecond);
								wave->RemuxChannels(_pending, dd.ChannelCount, _matrix.GetBuffer());
							} else {
								SafePointer<Audio::WaveBuffer> wave = _read->ConvertFormat(dd.Format);
								wave = wave->ConvertFrameRate(dd.FramesPerSecond);
								wave->RemuxChannels(_pending, dd.ChannelCount, _matrix.GetBuffer());
							}
						}
						return true;
					} catch (...) {
						_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
						return false;
					}
				}
			
				friend class AudioDeviceResetInterrupt;

				class AudioDeviceResetInterrupt : public ISchedulerTask
				{
					AudioPlaybackTask * _parent;
				public:
					AudioDeviceResetInterrupt(AudioPlaybackTask * parent) : _parent(parent) {}
					virtual ~AudioDeviceResetInterrupt(void) override {}
					virtual void DoTask(Scheduler * scheduler) noexcept override { _parent->ResetDevice(true); }
					virtual void Cancelled(Scheduler * scheduler) noexcept override {}
				};
			public:
				double _volume;

				AudioPlaybackTask(PlaybackRoutine * parent) : _parent(parent), _matrix(0x20), _volume(1.0), _pending_packets(0), _device_reset(false)
				{
					_pending_ts = 0;
					_current_frame = 0;
					_audio_sync = CreateSemaphore(0);
					if (!_audio_sync) throw Exception();
					_factory = Audio::CreateAudioDeviceFactory();
					if (!_factory || !_factory->RegisterEventCallback(this)) throw Exception();
				}
				virtual ~AudioPlaybackTask(void) override { _factory->UnregisterEventCallback(this); }
				virtual void OnAudioDeviceEvent(Audio::AudioDeviceEvent event, Audio::AudioObjectType device_type, const string & device_identifier) noexcept override
				{
					if (event == Audio::AudioDeviceEvent::Inactivated && device_type == Audio::AudioObjectType::DeviceOutput) {
						if (!_device || device_identifier == _device->GetDeviceIdentifier()) {
							try {
								SafePointer<AudioDeviceResetInterrupt> reset_int = new AudioDeviceResetInterrupt(this);
								if (!_parent->_scheduler->Interrupt(reset_int)) {
									_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
								}
							} catch (...) {
								_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
							}
						}
					} else if (event == Audio::AudioDeviceEvent::DefaultChanged && device_type == Audio::AudioObjectType::DeviceOutput) {
						if (_parent->_audio_out->ResetOnDefaultDeviceChanged()) {
							try {
								SafePointer<AudioDeviceResetInterrupt> reset_int = new AudioDeviceResetInterrupt(this);
								if (!_parent->_scheduler->Interrupt(reset_int)) {
									_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
								}
							} catch (...) {
								_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
							}
						}
					}
				}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					_parent->_media_time = _pending_ts;
					if (_device_reset) {
						if (!_parent->Seek(_parent->_media_time)) {
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
						}
						return;
					}
					if (_read->FramesUsed()) {
						swap(_used_now, _pending);
						auto fts = _current_frame;
						if (!_parent->_audio_stream->ReadFrames(_read)) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainAudio);
							_parent->_stream_shutdown();
							return;
						}
						_current_frame += _read->FramesUsed();
						auto wait_ts = GetTimerValue();
						_audio_sync->Wait();
						auto block_time = GetTimerValue() - wait_ts;
						if (block_time >= 10) _parent->_playback_base += block_time;
						_pending_packets--;
						if (!ResampleRead()) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_stream_shutdown();
							return;
						}
						if (_read->FramesUsed()) {
							_device->WriteFramesAsync(_pending, 0, _audio_sync);
							_pending_packets++;
						}
						uint end_time = _parent->_playback_base + uint64(fts) * 1000 / _parent->_audio_stream->GetFormatDescriptor().FramesPerSecond;
						_pending_ts = end_time - _parent->_playback_base;
						if (!_parent->_scheduler->Schedule(this, end_time)) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
							_parent->_stream_shutdown();
							return;
						}
					} else {
						_device->StopProcessing();
						while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
						_parent->_stream_shutdown();
					}
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
				void Init(void)
				{
					if (_parent->_audio_in && _parent->_audio_out) {
						try {
							if (!_parent->_audio_stream) _parent->_audio_stream = new Audio::AudioDecoderStream(_parent->_audio_in);
						} catch (...) {
							_parent->_raise_error(PlaybackErrorCreateDecoderFailure | PlaybackErrorDomainAudio);
							return;
						}
						if (!ResetDevice()) _parent->_audio_stream.SetReference(0);
					}
				}
				void Seek(uint time)
				{
					if (_parent->_audio_stream) {
						_device->StopProcessing();
						while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
						if (!_device->StartProcessing()) {
							_parent->_raise_error(PlaybackErrorDeviceControlFailure | PlaybackErrorDomainAudio);
							return;
						}
						_device_reset = false;
						_parent->_streams_active++;
						auto ft_needed = uint64(time) * _parent->_audio_stream->GetFormatDescriptor().FramesPerSecond / 1000;
						auto ft_seeked = _parent->_audio_stream->SetCurrentFrame(ft_needed);
						auto ft_skip = ft_needed - ft_seeked;
						bool report_eos = false;
						while (ft_skip) {
							if (ft_skip < _read->GetSizeInFrames()) {
								try {
									SafePointer<Audio::WaveBuffer> _skip = new Audio::WaveBuffer(_parent->_audio_stream->GetFormatDescriptor(), ft_skip);
									if (!_parent->_audio_stream->ReadFrames(_skip)) {
										_device->StopProcessing();
										_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainAudio);
										_parent->_stream_shutdown();
										return;
									}
									ft_skip = 0;
									if (!_skip->FramesUsed()) { report_eos = true; break; }
								} catch (...) {
									_device->StopProcessing();
									_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
									_parent->_stream_shutdown();
									return;
								}
							} else {
								if (!_parent->_audio_stream->ReadFrames(_read)) {
									_device->StopProcessing();
									_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainAudio);
									_parent->_stream_shutdown();
									return;
								}
								ft_skip -= _read->GetSizeInFrames();
								if (!_read->FramesUsed()) { report_eos = true; break; }
							}
						}
						if (report_eos) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_stream_shutdown();
							return;
						}
						if (!_parent->_audio_stream->ReadFrames(_read)) {
							_device->StopProcessing();
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainAudio);
							_parent->_stream_shutdown();
							return;
						}
						if (!_read->FramesUsed()) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_stream_shutdown();
							return;
						}
						auto frames_first = _read->FramesUsed();
						if (!ResampleRead()) {
							_device->StopProcessing();
							_parent->_stream_shutdown();
							return;
						}
						_device->WriteFramesAsync(_pending, 0, _audio_sync);
						_pending_packets++;
						swap(_used_now, _pending);
						if (!_parent->_audio_stream->ReadFrames(_read)) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainAudio);
							_parent->_stream_shutdown();
							return;
						}
						if (!ResampleRead()) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_stream_shutdown();
							return;
						}
						if (_read->FramesUsed()) {
							_device->WriteFramesAsync(_pending, 0, _audio_sync);
							_pending_packets++;
						}
						uint end_time = _parent->_playback_base + uint64(ft_needed + frames_first) * 1000 / _parent->_audio_stream->GetFormatDescriptor().FramesPerSecond;
						_pending_ts = end_time - _parent->_playback_base;
						_current_frame = ft_needed + frames_first + _read->FramesUsed();
						if (!_parent->_scheduler->Schedule(this, end_time)) {
							_device->StopProcessing();
							while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
							_parent->_stream_shutdown();
							return;
						}
					}
				}
				void Stop(void)
				{
					if (_device) _device->StopProcessing();
					while (_pending_packets) { _audio_sync->Wait(); _pending_packets--; }
				}
				void SetVolume(double volume)
				{
					_volume = volume;
					if (_device) _device->SetVolume(_volume);
				}
				void InvalidateDevice(void)
				{
					try {
						SafePointer<AudioDeviceResetInterrupt> reset_int = new AudioDeviceResetInterrupt(this);
						if (!_parent->_scheduler->Interrupt(reset_int)) {
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainAudio);
						}
					} catch (...) {
						_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainAudio);
					}
				}
			};
			class VideoPlaybackTask : public ISchedulerTask
			{
				PlaybackRoutine * _parent;
				SafePointer<Semaphore> _video_sync;
				SafePointer<Video::IVideoFactory> _factory;
				SafePointer<Video::IVideoFrameBlt> _blt;
				SafePointer<Video::IVideoFrame> _present;
				SafePointer<Graphics::ITexture> _texture;
				int _cyclic_index;

				bool _present_frame(Video::IVideoFrame * frame)
				{
					if (!_blt) {
						_blt = _factory->CreateFrameBlt();
						if (!_blt) {
							_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainVideo);
							return false;
						}
						if (!_blt->SetInputFormat(frame)) {
							_blt.SetReference(0);
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainVideo);
							return false;
						}
						if (!_blt->SetOutputFormat(_texture->GetPixelFormat(), Codec::AlphaMode::Premultiplied, frame->GetObjectDescriptor())) {
							_blt.SetReference(0);
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainVideo);
							return false;
						}
					}
					_cyclic_index = (_cyclic_index + 1) % _texture->GetArraySize();
					auto ctx = _parent->_video_out->ProvideBltContext();
					auto vs = _video_sync;
					auto vf = _present.Inner();
					auto cb = _parent->_video_out;
					auto blt = _blt;
					auto ci = _cyclic_index;
					auto tx = _texture;
					if (!ctx) {
						_parent->_raise_error(PlaybackErrorDeviceStartupFailure | PlaybackErrorDomainVideo);
						return false;
					}
					try {
						ctx->SubmitTask(CreateFunctionalTask([blt, vs, vf, cb, ci, tx]() {
							if (blt->Process(tx, vf, Graphics::SubresourceIndex(0, ci))) {
								vs->Open();
								cb->PresentVideoFrame(ci);
							} else {
								vs->Open();
							}
						}));
					} catch (...) {
						_parent->_raise_error(PlaybackErrorAllocationFailure | PlaybackErrorDomainVideo);
						return false;
					}
					_video_sync->Wait();
					return true;
				}
			public:
				VideoPlaybackTask(PlaybackRoutine * parent) : _parent(parent), _cyclic_index(0)
				{
					_factory = Video::CreateVideoFactory();
					_video_sync = CreateSemaphore(0);
					if (!_video_sync || !_factory) throw Exception();
				}
				virtual ~VideoPlaybackTask(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					_present.SetReference(0);
					if (!_parent->_video_stream->ReadFrame(_present.InnerRef())) {
						_parent->_stream_shutdown();
						_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainVideo);
						return;
					}
					if (!_present) {
						_parent->_stream_shutdown();
						return;
					}
					_parent->_media_time = uint64(_present->GetObjectDescriptor().FramePresentation) * 1000 / _present->GetObjectDescriptor().TimeScale;
					if (!_present_frame(_present)) {
						_parent->_stream_shutdown();
						return;
					}
					uint end_time = _parent->_playback_base +
						uint64(_present->GetObjectDescriptor().FramePresentation + _present->GetObjectDescriptor().FrameDuration) * 1000 /
						_present->GetObjectDescriptor().TimeScale;
					if (!scheduler->Schedule(this, end_time)) {
						_parent->_stream_shutdown();
						_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainVideo);
						return;
					}
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
				void Init(void)
				{
					if (_parent->_video_in && _parent->_video_out) {
						try {
							if (!_parent->_video_stream) _parent->_video_stream = new Video::VideoDecoderStream(_parent->_video_in, _parent->_video_out->ProvideVideoDevice());
						} catch (...) {
							_parent->_raise_error(PlaybackErrorCreateDecoderFailure | PlaybackErrorDomainVideo);
							return;
						}
						auto & desc = _parent->_video_stream->GetObjectDescriptor();
						_texture.SetRetain(_parent->_video_out->ProvideVideoBuffer(desc.Width, desc.Height));
						if (!_texture) {
							_parent->_video_stream.SetReference(0);
							_parent->_raise_error(PlaybackErrorDeviceStartupFailure | PlaybackErrorDomainVideo);
						}
					}
				}
				void Seek(uint time)
				{
					if (_parent->_video_stream) {
						_parent->_streams_active++;
						_blt.SetReference(0);
						_present.SetReference(0);
						auto ft_needed = uint64(time) * _parent->_video_stream->GetObjectDescriptor().TimeScale / 1000;
						auto ft_seeked = _parent->_video_stream->SetCurrentTime(ft_needed);
						bool report_eos = false;
						while (true) {
							_present.SetReference(0);
							if (!_parent->_video_stream->ReadFrame(_present.InnerRef())) {
								_parent->_stream_shutdown();
								_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainVideo);
								return;
							}
							if (_present) {
								if (_present->GetObjectDescriptor().FramePresentation + _present->GetObjectDescriptor().FrameDuration >= ft_needed) break;
							} else { report_eos = true; break; }
						}
						if (report_eos) {
							_parent->_stream_shutdown();
							return;
						}
						if (!_present_frame(_present)) {
							_parent->_stream_shutdown();
							return;
						}
						uint end_time = _parent->_playback_base +
							uint64(_present->GetObjectDescriptor().FramePresentation + _present->GetObjectDescriptor().FrameDuration) * 1000 /
							_present->GetObjectDescriptor().TimeScale;
						if (!_parent->_scheduler->Schedule(this, end_time)) {
							_parent->_stream_shutdown();
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainVideo);
							return;
						}
					}
				}
			};
			class SubtitlePlaybackTask : public ISchedulerTask
			{
				Subtitles::SubtitleSample _current;
				PlaybackRoutine * _parent;
				bool _time_before;
			public:
				SubtitlePlaybackTask(PlaybackRoutine * parent) : _parent(parent), _time_before(true) {}
				virtual ~SubtitlePlaybackTask(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					if (_time_before) {
						_parent->_subtitle_out->PresentSubtitleText(_current.Text, _current.Flags);
						uint end_time = _parent->_playback_base + uint64(_current.TimePresent + _current.Duration) * 1000 / _current.TimeScale;
						auto ftime = _current.TimePresent + _current.Duration;
						if (!_parent->_subtitle_stream->ReadSample(_current)) {
							_parent->_stream_shutdown();
							_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainSubtitles);
							return;
						}
						auto stime = _current.TimePresent;
						_time_before = stime == ftime;
						if (!_parent->_scheduler->Schedule(this, end_time)) {
							_parent->_stream_shutdown();
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainSubtitles);
							return;
						}
					} else if (_current.Text.Length() || _current.Duration) {
						_time_before = true;
						_parent->_subtitle_out->PresentSubtitleText(L"", 0);
						uint end_time = _parent->_playback_base + uint64(_current.TimePresent) * 1000 / _current.TimeScale;
						if (!_parent->_scheduler->Schedule(this, end_time)) {
							_parent->_stream_shutdown();
							_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainSubtitles);
							return;
						}
					} else {
						_parent->_subtitle_out->PresentSubtitleText(L"", 0);
						_parent->_stream_shutdown();
						return;
					}
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
				void Init(void)
				{
					if (_parent->_subtitle_in && _parent->_subtitle_out) {
						try {
							if (!_parent->_subtitle_stream) _parent->_subtitle_stream = new Subtitles::SubtitleDecoderStream(_parent->_subtitle_in);
						} catch (...) {
							_parent->_raise_error(PlaybackErrorCreateDecoderFailure | PlaybackErrorDomainSubtitles);
						}
					}
				}
				void Seek(uint time)
				{
					if (_parent->_subtitle_stream) {
						_parent->_streams_active++;
						auto ft_needed = uint64(time) * _parent->_subtitle_stream->GetObjectDescriptor().TimeScale / 1000;
						auto ft_seeked = _parent->_subtitle_stream->SetCurrentTime(ft_needed);
						bool report_eos = false;
						while (true) {
							if (!_parent->_subtitle_stream->ReadSample(_current)) {
								_parent->_stream_shutdown();
								_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainSubtitles);
								return;
							}
							if (!_current.Text.Length() && !_current.Duration) {
								report_eos = true; break;
							} else {
								if (_current.TimePresent + _current.Duration >= ft_needed) break;
							}
						}
						if (report_eos) {
							_parent->_subtitle_out->PresentSubtitleText(L"", 0);
							_parent->_stream_shutdown();
							return;
						}
						if (ft_needed >= _current.TimePresent) {
							_time_before = false;
							_parent->_subtitle_out->PresentSubtitleText(_current.Text, _current.Flags);
							uint end_time = _parent->_playback_base + uint64(_current.TimePresent + _current.Duration) * 1000 / _current.TimeScale;
							if (!_parent->_subtitle_stream->ReadSample(_current)) {
								_parent->_stream_shutdown();
								_parent->_raise_error(PlaybackErrorDecodeFailure | PlaybackErrorDomainSubtitles);
								return;
							}
							if (!_parent->_scheduler->Schedule(this, end_time)) {
								_parent->_stream_shutdown();
								_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainSubtitles);
								return;
							}
						} else {
							_time_before = true;
							_parent->_subtitle_out->PresentSubtitleText(L"", 0);
							uint end_time = _parent->_playback_base + uint64(_current.TimePresent) * 1000 / _current.TimeScale;
							if (!_parent->_scheduler->Schedule(this, end_time)) {
								_parent->_stream_shutdown();
								_parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainSubtitles);
								return;
							}
						}
					}
				}
			};

			SafePointer<AudioPlaybackTask> _audio_playback;
			SafePointer<VideoPlaybackTask> _video_playback;
			SafePointer<SubtitlePlaybackTask> _subtitle_playback;

			class SeekInterrupt : public ISchedulerTask
			{
				PlaybackRoutine * _parent;
			public:
				uint TimeSeek;

				SeekInterrupt(PlaybackRoutine * parent) : _parent(parent), TimeSeek(0) {}
				virtual ~SeekInterrupt(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					scheduler->CancelAll();
					_parent->_streams_active = 0;
					_parent->_playback_base = GetTimerValue() - TimeSeek;
					_parent->_media_time = TimeSeek;
					_parent->_audio_playback->Seek(TimeSeek);
					_parent->_video_playback->Seek(TimeSeek);
					_parent->_subtitle_playback->Seek(TimeSeek);
					if (_parent->_callback && !(_parent->_state & 0x02)) _parent->_callback->OnSeeked();
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
			};
			class StopInterrupt : public ISchedulerTask
			{
				PlaybackRoutine * _parent;
			public:
				StopInterrupt(PlaybackRoutine * parent) : _parent(parent) {}
				virtual ~StopInterrupt(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					scheduler->CancelAll();
					_parent->_audio_playback->Stop();
					_parent->_state = 0;
					if (_parent->_callback) _parent->_callback->OnPaused();
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
			};

			SafePointer<SeekInterrupt> _seek_int;
			SafePointer<StopInterrupt> _stop_int;

			class InitInterrupt : public ISchedulerTask
			{
				PlaybackRoutine * _parent;
			public:
				InitInterrupt(PlaybackRoutine * parent) : _parent(parent) {}
				virtual ~InitInterrupt(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					_parent->_audio_playback->Init();
					_parent->_video_playback->Init();
					_parent->_subtitle_playback->Init();
					_parent->_state = 0x01;
					if (_parent->_callback) _parent->_callback->OnStarted();
					_parent->_seek_int->TimeSeek = _parent->_media_time;
					if (!scheduler->Interrupt(_parent->_seek_int)) _parent->_raise_error(PlaybackErrorSynchronizationFailure | PlaybackErrorDomainCommon);
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
			};
			class ShutdownInterrupt : public ISchedulerTask
			{
			public:
				SafePointer<AudioPlaybackTask> AudioPlayback;
				SafePointer<Object> Retain1, Retain2;

				ShutdownInterrupt(void) {}
				virtual ~ShutdownInterrupt(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override
				{
					AudioPlayback->Stop();
					scheduler->Break();
				}
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
			};
			class SetVolumeInterrupt : public ISchedulerTask
			{
				PlaybackRoutine * _parent;
			public:
				double VolumeSet;

				SetVolumeInterrupt(PlaybackRoutine * parent) : _parent(parent), VolumeSet(1.0) {}
				virtual ~SetVolumeInterrupt(void) override {}
				virtual void DoTask(Scheduler * scheduler) noexcept override { _parent->_audio_playback->SetVolume(VolumeSet); }
				virtual void Cancelled(Scheduler * scheduler) noexcept override {}
			};

			SafePointer<InitInterrupt> _init_int;
			SafePointer<ShutdownInterrupt> _shutdown_int;
			SafePointer<SetVolumeInterrupt> _vol_int;

			void _evaluate_duration(void) noexcept
			{
				uint ad = 0, vd = 0, sd = 0;
				if (_audio_in) ad = (uint64(_audio_in->GetDuration()) * 1000 + _audio_in->GetTimeScale() - 1) / _audio_in->GetTimeScale();
				if (_video_in) vd = (uint64(_video_in->GetDuration()) * 1000 + _video_in->GetTimeScale() - 1) / _video_in->GetTimeScale();
				if (_subtitle_in) sd = (uint64(_subtitle_in->GetDuration()) * 1000 + _subtitle_in->GetTimeScale() - 1) / _subtitle_in->GetTimeScale();
				_duration = max(max(ad, vd), sd);
			}
		public:
			PlaybackRoutine(void) : _callback(0), _audio_out(0), _video_out(0), _subtitle_out(0), _state(0x04), _media_time(0), _streams_active(0), _duration(0), _playback_base(0)
			{
				_error_state = 0;
				_scheduler = new Scheduler;
				_audio_playback = new AudioPlaybackTask(this);
				_video_playback = new VideoPlaybackTask(this);
				_subtitle_playback = new SubtitlePlaybackTask(this);
				_seek_int = new SeekInterrupt(this);
				_stop_int = new StopInterrupt(this);
				_init_int = new InitInterrupt(this);
				_shutdown_int = new ShutdownInterrupt;
				_vol_int = new SetVolumeInterrupt(this);
			}
			virtual ~PlaybackRoutine(void) override
			{
				if (_dispatch) {
					_shutdown_int->Retain1 = _audio_obj;
					_shutdown_int->Retain2 = _video_obj;
					_shutdown_int->AudioPlayback = _audio_playback;
					_scheduler->CancelAll();
					_scheduler->Interrupt(_shutdown_int);
				}
			}
			virtual void SetCallback(IPlaybackRoutineCallback * callback) noexcept override { if (_state == 0x04) _callback = callback; }
			virtual IPlaybackRoutineCallback * GetCallback(void) noexcept override { return _callback; }
			virtual bool AttachInput(Media::IMediaTrackSource * track) noexcept override
			{
				if (_state != 0x04) return false;
				try {
					if (track->GetTrackClass() == TrackClass::Audio) {
						_audio_in.SetRetain(track);
						_audio_stream.SetRetain(0);
						_evaluate_duration();
						return true;
					} else if (track->GetTrackClass() == TrackClass::Video) {
						_video_in.SetRetain(track);
						_video_stream.SetRetain(0);
						_evaluate_duration();
						return true;
					} else if (track->GetTrackClass() == TrackClass::Subtitles) {
						_subtitle_in.SetRetain(track);
						_subtitle_stream.SetRetain(0);
						_evaluate_duration();
						return true;
					} else return false;
				} catch (...) { return false; }
			}
			virtual bool AttachAudioOutput(IAudioDeviceSelector * output) noexcept override
			{
				if (_state != 0x04) return false;
				_audio_out = output;
				_audio_obj.SetRetain(_audio_out ? _audio_out->ObjectAudioInterface() : 0);
				return true;
			}
			virtual bool AttachVideoOutput(IVideoPresenter * output) noexcept override
			{
				if (_state != 0x04) return false;
				_video_out = output;
				_video_obj.SetRetain(_video_out ? _video_out->ObjectVideoInterface() : 0);
				return true;
			}
			virtual bool AttachSubtitleOutput(ISubtitlesPresenter * output) noexcept override
			{
				if (_state != 0x04) return false;
				_subtitle_out = output;
				_subtitle_obj.SetRetain(_subtitle_out ? _subtitle_out->ObjectSubtitlesInterface() : 0);
				return true;
			}
			virtual bool Play(void) noexcept override
			{
				if (IsPlaying()) return false;
				if (!_dispatch) {
					_state = 0;
					if (!_scheduler->ProcessAsSeparateThread(_dispatch.InnerRef())) return false;
				}
				if (!_scheduler->Interrupt(_init_int)) return false;
				return true;
			}
			virtual bool Pause(void) noexcept override
			{
				if (!IsPlaying()) return false;
				if (!_scheduler->Interrupt(_stop_int)) return false;
				return true;
			}
			virtual bool Seek(uint time) noexcept override
			{
				if (IsPlaying()) {
					_seek_int->TimeSeek = time;
					if (!_scheduler->Interrupt(_seek_int)) return false;
					return true;
				} else {
					_media_time = time;
					return true;
				}
			}
			virtual bool SetVolume(double volume) noexcept override
			{
				if (_scheduler) {
					_vol_int->VolumeSet = volume;
					if (!_scheduler->Interrupt(_vol_int)) return false;
					return true;
				} else {
					_audio_playback->_volume = volume;
					return true;
				}
			}
			virtual double GetVolume(void) noexcept override { return _audio_playback->_volume; }
			virtual uint GetCurrentTime(void) noexcept override
			{
				if (_state & 0x01) return GetTimerValue() - _playback_base;
				else return _media_time;
			}
			virtual uint GetDuration(void) noexcept override { return _duration; }
			virtual bool IsPlaying(void) noexcept override { return _state & 0x01; }
			virtual bool IsEndOfStream(void) noexcept override { return _state & 0x02; }
			virtual uint GetErrorState(void) noexcept override { return _error_state; }
			virtual bool InvalidateAudioDevice(void) noexcept override
			{
				if (_scheduler) _audio_playback->InvalidateDevice();
				return true;
			}
		};
		class DefaultAudioDeviceSelector : public Object, public IAudioDeviceSelector
		{
		public:
			DefaultAudioDeviceSelector(void) {}
			virtual ~DefaultAudioDeviceSelector(void) override {}
			virtual Object * ObjectAudioInterface(void) noexcept override { return this; }
			virtual bool ResetOnDefaultDeviceChanged(void) noexcept override { return true; }
			virtual void OnAudioDeviceLost(void) noexcept override {}
			virtual Audio::IAudioOutputDevice * ProvideAudioDevice(Audio::IAudioDeviceFactory * factory) noexcept override { return factory->CreateDefaultOutputDevice(); }
			virtual void ProvideChannelMatrix(Audio::IAudioOutputDevice * device, const Audio::StreamDesc & desc, uint channel_layout, double * matrix) noexcept override
			{
				MakeStandardRemuxMatrix(matrix, desc.ChannelCount, channel_layout, device->GetFormatDescriptor().ChannelCount, device->GetChannelLayout());
			}
		};
		class MirrorCaptureSink : public IMirrorCaptureSink, public Audio::IAudioEventCallback
		{
			SafePointer<TaskQueue> _queue;
			SafePointer<Object> _audio_obj, _video_obj;
			SafePointer<Semaphore> _video_sync;
			SafePointer<Audio::IAudioOutputDevice> _audio_device;
			SafePointer<Audio::IAudioDeviceFactory> _audio_factory;
			SafePointer<Video::IVideoFrameBlt> _video_frame_blt;
			SafePointer<Video::IVideoFactory> _video_factory;
			SafePointer<Graphics::ITexture> _video_buffer;
			IPlaybackRoutineCallback * _callback;
			IAudioDeviceSelector * _audio_out;
			IVideoPresenter * _video_out;
			int _video_index, _audio_index, _video_frame;
			double _volume;
			Audio::StreamDesc _audio_desc;
			uint _audio_channel_layout;
			Array<double> _matrix;
			bool _processing;
		public:
			MirrorCaptureSink(void) : _callback(0), _audio_out(0), _video_out(0), _volume(1.0), _matrix(0x10), _processing(false), _video_frame(0)
			{
				_queue = new TaskQueue;
				if (!_queue->ProcessAsSeparateThread()) throw Exception();
			}
			virtual ~MirrorCaptureSink(void) override { if (_audio_factory) _audio_factory->UnregisterEventCallback(this); _queue->Break(); }
			void ResetAudioDevice(void) noexcept
			{
				if (_audio_out) {
					_audio_device = _audio_out->ProvideAudioDevice(_audio_factory);
					if (!_audio_device) {
						if (_callback) _callback->OnError(PlaybackErrorAllocationFailure);
						return;
					}
					try {
						_matrix.SetLength(_audio_device->GetFormatDescriptor().ChannelCount * _audio_desc.ChannelCount);
					} catch (...) {
						if (_callback) _callback->OnError(PlaybackErrorAllocationFailure);
						return;
					}
					_audio_out->ProvideChannelMatrix(_audio_device, _audio_desc, _audio_channel_layout, _matrix.GetBuffer());
					_audio_device->SetVolume(_volume);
					if (_processing) _audio_device->StartProcessing();
				}
			}
			virtual void OnAudioDeviceEvent(Audio::AudioDeviceEvent event, Audio::AudioObjectType device_type, const string & device_identifier) noexcept override
			{
				if (event == Audio::AudioDeviceEvent::Inactivated && device_type == Audio::AudioObjectType::DeviceOutput && _audio_device && _audio_device->GetDeviceIdentifier() == device_identifier) {
					if (_audio_out) _audio_out->OnAudioDeviceLost();
					InvalidateAudioDevice();
				} else if (event == Audio::AudioDeviceEvent::DefaultChanged && device_type == Audio::AudioObjectType::DeviceOutput && _audio_device && _audio_out && _audio_out->ResetOnDefaultDeviceChanged()) {
					InvalidateAudioDevice();
				}
			}
			virtual void OnStarted(void) noexcept override
			{
				SafePointer<MirrorCaptureSink> self;
				self.SetRetain(this);
				try {
					_queue->SubmitTask(CreateFunctionalTask([self]() {
						self->_processing = true;
						if (self->_audio_device) {
							if (!self->_audio_device->StartProcessing()) {
								if (self->_callback) self->_callback->OnError(PlaybackErrorDeviceControlFailure);
							}
						}
						if (self->_callback) self->_callback->OnStarted();
					}));
				} catch (...) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); }
			}
			virtual void OnStopped(void) noexcept override
			{
				SafePointer<MirrorCaptureSink> self;
				self.SetRetain(this);
				try {
					_queue->SubmitTask(CreateFunctionalTask([self]() {
						self->_processing = false;
						if (self->_audio_device) self->_audio_device->StopProcessing();
						if (self->_callback) self->_callback->OnPaused();
					}));
				} catch (...) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); }
			}
			virtual void OnError(uint error_code) noexcept override { if (_callback) _callback->OnError(error_code); }
			virtual void HandleAudioFormat(int stream_index, const Audio::StreamDesc & desc, uint channel_layout) noexcept override
			{
				if (stream_index == _audio_index && _audio_out) {
					if (!_audio_factory) _audio_factory = Audio::CreateAudioDeviceFactory();
					if (!_audio_factory) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
					if (!_audio_factory->RegisterEventCallback(this)) { if (_callback) _callback->OnError(PlaybackErrorDeviceStartupFailure); return; }
					_audio_device.SetReference(0);
					_audio_desc = desc;
					_audio_channel_layout = channel_layout;
					ResetAudioDevice();
				}
			}
			virtual void HandleVideoFormat(int stream_index, const Video::VideoObjectDesc & desc) noexcept override
			{
				if (stream_index == _video_index && _video_out) {
					if (!_video_sync) _video_sync = CreateSemaphore(0);
					if (!_video_sync) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
					if (!_video_factory) _video_factory = Video::CreateVideoFactory();
					if (!_video_factory) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
					_video_buffer.SetRetain(_video_out->ProvideVideoBuffer(desc.Width, desc.Height));
					_video_frame = 0;
				}
			}
			virtual void HandleAudioInput(int stream_index, Audio::WaveBuffer * wave) noexcept override
			{
				if (_audio_device && stream_index == _audio_index) {
					SafePointer<MirrorCaptureSink> self;
					self.SetRetain(this);
					SafePointer<Audio::WaveBuffer> copy = new Audio::WaveBuffer(wave);
					try {
						_queue->SubmitTask(CreateFunctionalTask([copy, self]() {
							SafePointer<Audio::WaveBuffer> c = copy;
							auto & in = c->GetFormatDescriptor();
							auto & out = self->_audio_device->GetFormatDescriptor();
							if (in.Format != out.Format) c = c->ConvertFormat(out.Format);
							if (in.FramesPerSecond != out.FramesPerSecond) c = c->ConvertFrameRate(out.FramesPerSecond);
							if (in.ChannelCount != out.ChannelCount || self->_audio_device->GetChannelLayout() != self->_audio_channel_layout) {
								c = c->RemuxChannels(out.ChannelCount, self->_matrix);
							}
							self->_audio_device->WriteFramesAsync(c, 0);
						}));
					} catch (...) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); }
				}
			}
			virtual void HandleVideoInput(int stream_index, Video::IVideoFrame * frame) noexcept override
			{
				if (!_video_frame_blt) {
					_video_frame_blt = _video_factory->CreateFrameBlt();
					if (!_video_frame_blt) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
					if (!_video_frame_blt->SetInputFormat(frame)) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
					if (!_video_frame_blt->SetOutputFormat(_video_buffer->GetPixelFormat(), Codec::AlphaMode::Premultiplied, frame->GetObjectDescriptor())) {
						if (_callback) _callback->OnError(PlaybackErrorAllocationFailure);
						return;
					}
				}
				_video_frame = (_video_frame + 1) % _video_buffer->GetArraySize();
				auto vfi = _video_frame;
				auto vf = frame;
				auto vs = _video_sync;
				auto vo = _video_out;
				auto vt = _video_buffer;
				auto blt = _video_frame_blt;
				try {
					_video_out->ProvideBltContext()->SubmitTask(CreateFunctionalTask([vfi, vf, vs, vo, vt, blt]() {
						if (blt->Process(vt, vf, Graphics::SubresourceIndex(0, vfi))) {
							vs->Open();
							vo->PresentVideoFrame(vfi);
						} else vs->Open();
					}));
				} catch (...) { if (_callback) _callback->OnError(PlaybackErrorAllocationFailure); return; }
				_video_sync->Wait();
			}

			virtual void SetCallback(IPlaybackRoutineCallback * callback) noexcept override { _callback = callback; }
			virtual IPlaybackRoutineCallback * GetCallback(void) noexcept override { return _callback; }
			virtual bool AttachAudioOutput(int stream_index_for, IAudioDeviceSelector * output) noexcept override
			{
				_audio_out = output;
				_audio_index = stream_index_for;
				_audio_obj.SetRetain(output->ObjectAudioInterface());
				return true;
			}
			virtual bool AttachVideoOutput(int stream_index_for, IVideoPresenter * output) noexcept override
			{
				_video_out = output;
				_video_index = stream_index_for;
				_video_obj.SetRetain(output->ObjectVideoInterface());
				return true;
			}
			virtual bool SetVolume(double volume) noexcept override
			{
				_volume = volume;
				SafePointer<MirrorCaptureSink> self;
				self.SetRetain(this);
				try {
					_queue->SubmitTask(CreateFunctionalTask([self]() {
						if (self->_audio_device) self->_audio_device->SetVolume(self->_volume);
					}));
				} catch (...) { return false; }
				return true;
			}
			virtual double GetVolume(void) noexcept override { return _volume; }
			virtual bool InvalidateAudioDevice(void) noexcept override
			{
				SafePointer<MirrorCaptureSink> self;
				self.SetRetain(this);
				try {
					_queue->SubmitTask(CreateFunctionalTask([self]() {
						self->_audio_device.SetReference(0);
						self->ResetAudioDevice();
					}));
				} catch (...) { return false; }
				return true;
			}
		};
		class CaptureRoutine : public ICaptureRoutine
		{
			ObjectArray<Audio::IAudioInputDevice> _aud_in;
			ObjectArray<Video::IVideoDevice> _vid_in;
			ObjectArray<ICaptureSink> _sinks;
			uint _aud_latency;
			bool _running;

			class AudioDeviceHandler : public Task
			{
			public:
				SafePointer<CaptureRoutine> _routine;
				SafePointer<Audio::IAudioInputDevice> _device;
				SafePointer<Audio::WaveBuffer> _swap1, _swap2;
				int _index;
				bool _status;

				AudioDeviceHandler(CaptureRoutine * routine, Audio::IAudioInputDevice * device, int index, uint latency) : _index(index), _status(false)
				{
					_routine.SetRetain(routine);
					_device.SetRetain(device);
					auto num_frames = min(max(latency * device->GetFormatDescriptor().FramesPerSecond / 1000, 1024U), device->GetFormatDescriptor().FramesPerSecond);
					_swap1 = new Audio::WaveBuffer(device->GetFormatDescriptor(), num_frames);
					_swap2 = new Audio::WaveBuffer(device->GetFormatDescriptor(), num_frames);
				}
				virtual ~AudioDeviceHandler(void) override {}
				virtual void DoTask(void) noexcept override
				{
					auto status = _status;
					if (status && _swap1->FramesUsed()) {
						for (auto & sink : _routine->_sinks) sink.HandleAudioInput(_index, _swap1);
						swap(_swap1, _swap2);
						if (!_device->ReadFramesAsync(_swap1, &_status, this)) {
							for (auto & sink : _routine->_sinks) sink.OnError(PlaybackErrorDeviceControlFailure);
						}
					}
				}
				void Start(void)
				{
					if (!_device->ReadFramesAsync(_swap1, &_status, this)) throw Exception();
					if (!_device->ReadFramesAsync(_swap2, &_status, this)) throw Exception();
				}
			};
			class VideoDeviceHandler : public Task
			{
			public:
				SafePointer<CaptureRoutine> _routine;
				SafePointer<Video::IVideoDevice> _device;
				SafePointer<Video::IVideoFrame> _frame;
				int _index;
				bool _status;

				VideoDeviceHandler(CaptureRoutine * routine, Video::IVideoDevice * device, int index) : _index(index), _status(false)
				{
					_routine.SetRetain(routine);
					_device.SetRetain(device);
				}
				virtual ~VideoDeviceHandler(void) override {}
				virtual void DoTask(void) noexcept override
				{
					auto status = _status;
					if (status && _frame) {
						for (auto & sink : _routine->_sinks) sink.HandleVideoInput(_index, _frame);
						_frame.SetReference(0);
						if (!_device->ReadFrameAsync(_frame.InnerRef(), &_status, this)) {
							for (auto & sink : _routine->_sinks) sink.OnError(PlaybackErrorDeviceControlFailure);
						}
					}
				}
				void Start(void)
				{
					_frame.SetReference(0);
					if (!_device->ReadFrameAsync(_frame.InnerRef(), &_status, this)) throw Exception();
				}
			};
		public:
			CaptureRoutine(void) : _aud_in(0x10), _vid_in(0x10), _sinks(0x10), _running(false), _aud_latency(100) {}
			virtual ~CaptureRoutine(void) override { Stop(); }
			virtual bool SetInputAudio(int stream_index, Audio::IAudioInputDevice * device) noexcept override
			{
				try {
					if (_running || stream_index < 0) return false;
					if (_aud_in.Length() < stream_index) return false;
					if (_aud_in.Length() == stream_index) _aud_in.Append(device);
					else _aud_in.SetElement(device, stream_index);
				} catch (...) { return false; }
				for (auto & s : _sinks) s.HandleAudioFormat(stream_index, device->GetFormatDescriptor(), device->GetChannelLayout());
				return true;
			}
			virtual bool SetInputVideo(int stream_index, Video::IVideoDevice * device) noexcept override
			{
				try {
					if (_running || stream_index < 0) return false;
					if (_vid_in.Length() < stream_index) return false;
					if (_vid_in.Length() == stream_index) _vid_in.Append(device);
					else _vid_in.SetElement(device, stream_index);
				} catch (...) { return false; }
				for (auto & s : _sinks) s.HandleVideoFormat(stream_index, device->GetObjectDescriptor());
				return true;
			}
			virtual bool AddOutputSink(ICaptureSink * sink) noexcept override
			{
				if (_running) return false;
				try { _sinks.Append(sink); } catch (...) { return false; }
				for (int i = 0; i < _aud_in.Length(); i++) sink->HandleAudioFormat(i, _aud_in[i].GetFormatDescriptor(), _aud_in[i].GetChannelLayout());
				for (int i = 0; i < _vid_in.Length(); i++) sink->HandleVideoFormat(i, _vid_in[i].GetObjectDescriptor());
				return true;
			}
			virtual bool RemoveOutputSink(ICaptureSink * sink) noexcept override
			{
				if (_running) return false;
				for (int i = 0; i < _sinks.Length(); i++) if (_sinks.ElementAt(i) == sink) {
					_sinks.Remove(i);
					return true;
				}
				return false;
			}
			virtual bool SetAudioLatency(uint time) noexcept override
			{
				if (_running) return false;
				_aud_latency = time;
				return true;
			}
			virtual bool Start(void) noexcept override
			{
				if (_running) return false;
				for (auto & sink : _sinks) sink.OnStarted();
				try {
					for (int i = 0; i < _aud_in.Length(); i++) {
						SafePointer<AudioDeviceHandler> handler = new AudioDeviceHandler(this, &_aud_in[i], i, _aud_latency);
						handler->Start();
					}
				} catch (...) {
					for (auto & sink : _sinks) {
						sink.OnError(PlaybackErrorAllocationFailure);
						sink.OnStopped();
					}
					return false;
				}
				try {
					for (int i = 0; i < _vid_in.Length(); i++) {
						SafePointer<VideoDeviceHandler> handler = new VideoDeviceHandler(this, &_vid_in[i], i);
						handler->Start();
					}
				} catch (...) {
					for (auto & sink : _sinks) {
						sink.OnError(PlaybackErrorAllocationFailure);
						sink.OnStopped();
					}
					return false;
				}
				for (auto & dev : _aud_in) if (!dev.StartProcessing()) {
					for (auto & dev : _aud_in) dev.StopProcessing();
					for (auto & dev : _vid_in) dev.StopProcessing();
					for (auto & sink : _sinks) {
						sink.OnError(PlaybackErrorDeviceStartupFailure);
						sink.OnStopped();
					}
					return false;
				}
				for (auto & dev : _vid_in) if (!dev.StartProcessing()) {
					for (auto & dev : _aud_in) dev.StopProcessing();
					for (auto & dev : _vid_in) dev.StopProcessing();
					for (auto & sink : _sinks) {
						sink.OnError(PlaybackErrorDeviceStartupFailure);
						sink.OnStopped();
					}
					return false;
				}
				_running = true;
				return true;
			}
			virtual bool Stop(void) noexcept override
			{
				if (!_running) return false;
				for (auto & dev : _aud_in) dev.StopProcessing();
				for (auto & dev : _vid_in) dev.StopProcessing();
				for (auto & sink : _sinks) sink.OnStopped();
				_running = false;
				return true;
			}
			virtual bool IsPlaying(void) noexcept override { return _running; }
		};

		IPlaybackRoutine * CreatePlaybackRoutine(void) { return new PlaybackRoutine; }
		IMirrorCaptureSink * CreateMirrorCaptureSink(void) { return new MirrorCaptureSink; }
		ICaptureRoutine * CreateCaptureRoutine(void) { return new CaptureRoutine; }
		void SetDefaultAudioDeviceSelector(IPlaybackRoutine * routine)
		{
			auto result = new DefaultAudioDeviceSelector;
			routine->AttachAudioOutput(result);
			result->Release();
		}
		void SetDefaultAudioDeviceSelector(int index_at, IMirrorCaptureSink * sink)
		{
			auto result = new DefaultAudioDeviceSelector;
			sink->AttachAudioOutput(index_at, result);
			result->Release();
		}
		uint GetChannelRole(uint layout, uint index)
		{
			uint mask = 1;
			while (true) {
				while (!(layout & mask) && mask) mask <<= 1;
				if (!mask) return 0;
				if (index) {
					mask <<= 1;
					index--;
				} else return layout & mask;
			}
		}
		bool IsChannelLeft(uint role)
		{
			return (role & Audio::ChannelLayoutLeft) || (role & Audio::ChannelLayoutBackLeft) || (role & Audio::ChannelLayoutFrontLeft) ||
				(role & Audio::ChannelLayoutSideLeft) || (role & Audio::ChannelLayoutTopFrontLeft) || (role & Audio::ChannelLayoutTopBackLeft);
		}
		bool IsChannelRight(uint role)
		{
			return (role & Audio::ChannelLayoutRight) || (role & Audio::ChannelLayoutBackRight) || (role & Audio::ChannelLayoutFrontRight) ||
				(role & Audio::ChannelLayoutSideRight) || (role & Audio::ChannelLayoutTopFrontRight) || (role & Audio::ChannelLayoutTopBackRight);
		}
		bool IsChannelCenter(uint role)
		{
			return (role & Audio::ChannelLayoutCenter) || (role & Audio::ChannelLayoutBackCenter) || (role & Audio::ChannelLayoutTopCenter) ||
				(role & Audio::ChannelLayoutTopFrontCenter) || (role & Audio::ChannelLayoutTopBackCenter);
		}
		bool IsChannelSurround(uint role)
		{
			return (role & Audio::ChannelLayoutLowFrequency);
		}
		void MakeStandardRemuxMatrix(double * matrix, uint input_channels, uint input_layout, uint output_channels, uint output_layout)
		{
			uint ic = input_channels;
			uint il = input_layout;
			uint oc = output_channels;
			uint ol = output_layout;
			if (ic == oc && il == ol) {
				for (uint j = 0; j < ic; j++) for (uint i = 0; i < ic; i++) matrix[i + j * ic] = (i == j) ? 1.0 : 0.0;
			} else {
				uint in_left = 0, in_right = 0, in_center = 0, in_common = 0;
				for (uint i = 0; i < ic; i++) {
					auto role = GetChannelRole(il, i);
					if (IsChannelLeft(role)) in_left++;
					else if (IsChannelRight(role)) in_right++;
					else if (IsChannelCenter(role)) in_center++;
					else in_common++;
				}
				uint out_left = 0, out_right = 0, out_center = 0, out_common = 0;
				for (uint i = 0; i < oc; i++) {
					auto role = GetChannelRole(ol, i);
					if (IsChannelLeft(role)) out_left++;
					else if (IsChannelRight(role)) out_right++;
					else if (IsChannelCenter(role)) out_center++;
					else out_common++;
				}
				if (out_left + out_right + out_center + out_common == 0 || in_left + in_right + in_center + in_common == 0) {
					for (uint j = 0; j < oc; j++) for (uint i = 0; i < ic; i++) matrix[i + j * ic] = (i == j) ? 1.0 : 0.0;
				} else {
					bool side_as_center = !(out_left + out_right);
					bool center_as_side = !out_center;
					double left_weight = center_as_side ? (1.0 / (in_left + in_center + in_common)) : (1.0 / (in_left + in_common));
					double right_weight = center_as_side ? (1.0 / (in_right + in_center + in_common)) : (1.0 / (in_right + in_common));
					double center_weight = side_as_center ? (1.0 / (in_center + in_left + in_right + in_common)) : (1.0 / (in_center + in_common));
					double common_weight = 1.0 / (in_center + in_left + in_right + in_common);
					for (uint j = 0; j < oc; j++) for (uint i = 0; i < ic; i++) {
						int ir;
						if (IsChannelLeft(GetChannelRole(il, i))) ir = 0;
						else if (IsChannelRight(GetChannelRole(il, i))) ir = 1;
						else if (IsChannelCenter(GetChannelRole(il, i))) ir = 2;
						else ir = 3;
						double w = 0.0;
						if (IsChannelLeft(GetChannelRole(ol, j))) {
							if (ir == 0 || ir == 3 || (ir == 2 && center_as_side)) w = left_weight;
						} else if (IsChannelRight(GetChannelRole(ol, j))) {
							if (ir == 1 || ir == 3 || (ir == 2 && center_as_side)) w = right_weight;
						} else if (IsChannelCenter(GetChannelRole(ol, j))) {
							if (ir == 2 || ir == 3 || ((ir == 0 || ir == 1) && side_as_center)) w = center_weight;
						} else {
							w = common_weight;
						}
						matrix[i + j * ic] = w;
					}
				}
			}
		}
	}
}