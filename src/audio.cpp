#include "halo2/audio.hpp"
#include "halo2/vista_audio_framing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
namespace halo2 {
namespace {
void check(int result, const char *context) {
  if (result < 0) {
    char text[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(result, text, sizeof(text));
    throw FormatError(std::string(context) + ": " + text);
  }
}
PcmSound decode_audio_container(const EncodedSound &input,
                                const std::filesystem::path &movie = {}) {
  struct Source {
    std::span<const std::byte> bytes;
    std::int64_t cursor{};
  } source{input.data};
  struct Resources {
    AVFormatContext *format{};
    AVIOContext *io{};
    AVCodecContext *decoder{};
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    SwrContext *resampler{};
    ~Resources() {
      swr_free(&resampler);
      av_packet_free(&packet);
      av_frame_free(&frame);
      avcodec_free_context(&decoder);
      avformat_close_input(&format);
      if (io) {
        av_freep(&io->buffer);
        avio_context_free(&io);
      }
    }
  } r;
  r.format = avformat_alloc_context();
  if (!r.format || !r.frame || !r.packet) {
    throw FormatError("cannot allocate WMA decoder");
  }
  if (movie.empty()) {
    auto *buffer = static_cast<unsigned char *>(av_malloc(32768));
    if (!buffer)
      throw FormatError("cannot allocate audio input buffer");
    r.io = avio_alloc_context(
        buffer, 32768, 0, &source,
        [](void *opaque, unsigned char *destination, int size) -> int {
          auto &s = *static_cast<Source *>(opaque);
          const auto count = std::min<std::int64_t>(
              size, std::int64_t(s.bytes.size()) - s.cursor);
          if (count <= 0)
            return AVERROR_EOF;
          std::memcpy(destination, s.bytes.data() + s.cursor,
                      std::size_t(count));
          s.cursor += count;
          return int(count);
        },
        nullptr,
        [](void *opaque, std::int64_t offset, int whence) -> std::int64_t {
          auto &s = *static_cast<Source *>(opaque);
          if (whence == AVSEEK_SIZE)
            return std::int64_t(s.bytes.size());
          whence &= ~AVSEEK_FORCE;
          const auto base = whence == SEEK_SET   ? 0
                            : whence == SEEK_CUR ? s.cursor
                            : whence == SEEK_END ? std::int64_t(s.bytes.size())
                                                 : -1;
          if (base < 0 || offset < -base ||
              offset > std::int64_t(s.bytes.size()) - base)
            return AVERROR(EINVAL);
          s.cursor = base + offset;
          return s.cursor;
        });
    if (!r.io) {
      av_free(buffer);
      throw FormatError("cannot allocate WMA input stream");
    }
    r.format->pb = r.io;
    r.format->flags |= AVFMT_FLAG_CUSTOM_IO;
  }
  const auto filename = movie.string();
  check(avformat_open_input(&r.format,
                            movie.empty() ? nullptr : filename.c_str(), nullptr,
                            nullptr),
        "open retail audio container");
  check(avformat_find_stream_info(r.format, nullptr),
        "read WMA stream information");
  const auto stream =
      av_find_best_stream(r.format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  check(stream, "find WMA audio stream");
  const auto *parameters = r.format->streams[stream]->codecpar;
  const bool supported =
      movie.empty() ? (parameters->codec_id == AV_CODEC_ID_WMAV1 ||
                       parameters->codec_id == AV_CODEC_ID_WMAV2)
                    : (parameters->codec_id == AV_CODEC_ID_BINKAUDIO_DCT ||
                       parameters->codec_id == AV_CODEC_ID_BINKAUDIO_RDFT);
  if (!supported || parameters->ch_layout.nb_channels < 1 ||
      parameters->ch_layout.nb_channels > 2 || parameters->sample_rate < 8000 ||
      parameters->sample_rate > 96000)
    throw FormatError("unsupported retail WMA stream format");
  const auto *codec = avcodec_find_decoder(parameters->codec_id);
  if (!codec)
    throw FormatError("FFmpeg lacks WMA decoder");
  r.decoder = avcodec_alloc_context3(codec);
  if (!r.decoder)
    throw FormatError("cannot allocate WMA codec");
  check(avcodec_parameters_to_context(r.decoder, parameters),
        "configure WMA decoder");
  check(avcodec_open2(r.decoder, codec, nullptr), "open WMA decoder");
  PcmSound output;
  output.name = input.name;
  output.sound_class = input.sound_class;
  auto receive = [&] {
    while (true) {
      const auto result = avcodec_receive_frame(r.decoder, r.frame);
      if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        break;
      check(result, "decode WMA frame");
      if (!r.resampler) {
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        check(swr_alloc_set_opts2(&r.resampler, &stereo, AV_SAMPLE_FMT_FLT,
                                  48000, &r.frame->ch_layout,
                                  AVSampleFormat(r.frame->format),
                                  r.decoder->sample_rate, 0, nullptr),
              "configure WMA resampler");
        check(swr_init(r.resampler), "initialize WMA resampler");
      }
      const int capacity = int(
          av_rescale_rnd(swr_get_delay(r.resampler, r.decoder->sample_rate) +
                             r.frame->nb_samples,
                         48000, r.decoder->sample_rate, AV_ROUND_UP));
      const auto offset = output.stereo.size();
      if (offset + std::size_t(capacity) * 2 > 48000u * 600u * 2u)
        throw FormatError("decoded WMA duration limit");
      output.stereo.resize(offset + std::size_t(capacity) * 2);
      auto *dst =
          reinterpret_cast<unsigned char *>(output.stereo.data() + offset);
      const auto converted = swr_convert(
          r.resampler, &dst, capacity,
          const_cast<const unsigned char **>(r.frame->extended_data),
          r.frame->nb_samples);
      check(converted, "resample WMA frame");
      output.stereo.resize(offset + std::size_t(converted) * 2);
      av_frame_unref(r.frame);
    }
  };
  int result;
  while ((result = av_read_frame(r.format, r.packet)) >= 0) {
    if (r.packet->stream_index == stream) {
      check(avcodec_send_packet(r.decoder, r.packet), "submit WMA packet");
      receive();
    }
    av_packet_unref(r.packet);
  }
  if (result != AVERROR_EOF)
    check(result, "read WMA packets");
  check(avcodec_send_packet(r.decoder, nullptr), "flush WMA decoder");
  receive();
  if (r.resampler)
    for (;;) {
      const int capacity = swr_get_out_samples(r.resampler, 0);
      if (capacity <= 0)
        break;
      const auto offset = output.stereo.size();
      output.stereo.resize(offset + std::size_t(capacity) * 2);
      auto *dest =
          reinterpret_cast<unsigned char *>(output.stereo.data() + offset);
      const auto converted =
          swr_convert(r.resampler, &dest, capacity, nullptr, 0);
      check(converted, "flush WMA resampler");
      output.stereo.resize(offset + std::size_t(converted) * 2);
      if (!converted)
        break;
    }
  for (auto &sample : output.stereo) {
    if (!std::isfinite(sample))
      throw FormatError("nonfinite WMA sample");
    sample = std::clamp(sample, -1.f, 1.f);
  }
  if (output.stereo.empty())
    throw FormatError("empty decoded WMA stream");
  return output;
}
} // namespace
PcmSound decode_movie_audio(const std::filesystem::path &path) {
  EncodedSound input;
  input.name = path.filename().string();
  return decode_audio_container(input, path);
}
PcmSound decode_sound(const EncodedSound &input) {
  if (!input.channels || input.channels > 2 || !input.sample_rate ||
      input.data.empty())
    throw FormatError("invalid encoded sound");
  if (input.codec == 4) {
    if (is_vista_serialized_audio(input.data))
      return decode_vista_serialized_sound(input);
    return decode_audio_container(input);
  }
  AVCodecID id;
  if (input.codec == 0)
    id = AV_CODEC_ID_PCM_S16BE;
  else if (input.codec == 1)
    id = AV_CODEC_ID_ADPCM_IMA_XBOX;
  else if (input.codec == 2)
    id = AV_CODEC_ID_ADPCM_IMA_WAV;
  else if (input.codec == 3)
    id = AV_CODEC_ID_PCM_S16LE;
  else
    throw FormatError("unsupported sound codec " + std::to_string(input.codec) +
                      " (WMA/container support pending)");
  const auto *codec = avcodec_find_decoder(id);
  if (!codec)
    throw FormatError("FFmpeg lacks required sound decoder");
  struct Resources {
    AVCodecContext *decoder{};
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    SwrContext *resampler{};
    ~Resources() {
      swr_free(&resampler);
      av_packet_free(&packet);
      av_frame_free(&frame);
      avcodec_free_context(&decoder);
    }
  } r;
  r.decoder = avcodec_alloc_context3(codec);
  if (!r.decoder || !r.frame || !r.packet)
    throw FormatError("cannot allocate sound decoder");
  r.decoder->sample_rate = int(input.sample_rate);
  av_channel_layout_default(&r.decoder->ch_layout, int(input.channels));
  r.decoder->block_align = (input.codec == 1 || input.codec == 2)
                               ? 36 * int(input.channels)
                               : 2 * int(input.channels);
  r.decoder->bits_per_coded_sample =
      (input.codec == 1 || input.codec == 2) ? 4 : 16;
  check(avcodec_open2(r.decoder, codec, nullptr), "open sound decoder");
  PcmSound output;
  output.name = input.name;
  output.sound_class = input.sound_class;
  auto receive = [&]() {
    for (;;) {
      const int status = avcodec_receive_frame(r.decoder, r.frame);
      if (status == AVERROR(EAGAIN) || status == AVERROR_EOF)
        break;
      check(status, "decode sound frame");
      if (!r.resampler) {
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        check(swr_alloc_set_opts2(&r.resampler, &stereo, AV_SAMPLE_FMT_FLT,
                                  48000, &r.frame->ch_layout,
                                  AVSampleFormat(r.frame->format),
                                  int(input.sample_rate), 0, nullptr),
              "configure sound resampler");
        check(swr_init(r.resampler), "initialize sound resampler");
      }
      const int capacity = int(av_rescale_rnd(
          swr_get_delay(r.resampler, input.sample_rate) + r.frame->nb_samples,
          48000, input.sample_rate, AV_ROUND_UP));
      const auto offset = output.stereo.size();
      output.stereo.resize(offset + std::size_t(capacity) * 2);
      auto *destination =
          reinterpret_cast<std::uint8_t *>(output.stereo.data() + offset);
      const auto frames =
          swr_convert(r.resampler, &destination, capacity,
                      const_cast<const std::uint8_t **>(r.frame->extended_data),
                      r.frame->nb_samples);
      check(frames, "resample sound frame");
      output.stereo.resize(offset + std::size_t(frames) * 2);
      av_frame_unref(r.frame);
    }
  };
  const auto block = std::size_t(r.decoder->block_align);
  if (input.data.size() % block)
    throw FormatError("truncated sound codec block");
  // Xbox ADPCM has 64 samples per 36-byte channel block. Decode complete
  // blocks; chunks have already been concatenated by the gestalt reader.
  const auto packet_size =
      (input.codec == 1 || input.codec == 2) ? block : block * 1024;
  for (std::size_t at = 0; at < input.data.size();) {
    const auto size = std::min(packet_size, input.data.size() - at);
    check(av_new_packet(r.packet, int(size)), "allocate sound packet");
    std::memcpy(r.packet->data, input.data.data() + at, size);
    check(avcodec_send_packet(r.decoder, r.packet), "submit sound packet");
    av_packet_unref(r.packet);
    receive();
    at += size;
    if (output.frames() > 48000ull * 600)
      throw FormatError("decoded sound exceeds ten minute limit");
  }
  check(avcodec_send_packet(r.decoder, nullptr), "flush sound decoder");
  receive();
  if (r.resampler) {
    for (;;) {
      const int capacity = swr_get_out_samples(r.resampler, 0);
      if (capacity <= 0)
        break;
      const auto offset = output.stereo.size();
      output.stereo.resize(offset + std::size_t(capacity) * 2);
      auto *dest =
          reinterpret_cast<std::uint8_t *>(output.stereo.data() + offset);
      const int frames = swr_convert(r.resampler, &dest, capacity, nullptr, 0);
      check(frames, "flush resampler");
      output.stereo.resize(offset + std::size_t(frames) * 2);
      if (!frames)
        break;
    }
  }
  for (auto &sample : output.stereo) {
    if (!std::isfinite(sample))
      throw FormatError("nonfinite decoded sound sample");
    sample = std::clamp(sample, -1.f, 1.f);
  }
  if (!output.frames())
    throw FormatError("sound decoded to no samples");
  return output;
}
void write_wav(const PcmSound &sound, const std::filesystem::path &path) {
  if (sound.stereo.size() > UINT32_MAX / 2 - 36)
    throw FormatError("WAV exceeds RIFF limit");
  std::ofstream f(path, std::ios::binary);
  if (!f)
    throw FormatError("cannot create WAV " + path.string());
  auto u16 = [&](unsigned v) {
    char b[]{char(v), char(v >> 8)};
    f.write(b, 2);
  };
  auto u32 = [&](unsigned v) {
    char b[]{char(v), char(v >> 8), char(v >> 16), char(v >> 24)};
    f.write(b, 4);
  };
  const auto bytes = std::uint32_t(sound.stereo.size() * 2);
  f.write("RIFF", 4);
  u32(bytes + 36);
  f.write("WAVEfmt ", 8);
  u32(16);
  u16(1);
  u16(2);
  u32(sound.rate);
  u32(sound.rate * 4);
  u16(4);
  u16(16);
  f.write("data", 4);
  u32(bytes);
  for (float sample : sound.stereo)
    u16(unsigned(std::int16_t(std::clamp(sample, -1.f, 1.f) * 32767.f)));
  if (!f)
    throw FormatError("failed writing WAV");
}
AudioEngine::AudioEngine(bool realtime) : realtime_(realtime) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    throw FormatError(SDL_GetError());
  SDL_AudioSpec spec{};
  spec.freq = 48000;
  spec.format = AUDIO_F32SYS;
  spec.channels = 2;
  spec.samples = 512;
  spec.callback = callback;
  spec.userdata = this;
  device_ = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
  if (!device_)
    throw FormatError(SDL_GetError());
  voices_.reserve(32);
  SDL_PauseAudioDevice(device_, realtime ? 0 : 1);
}
AudioEngine::~AudioEngine() {
  if (device_)
    SDL_CloseAudioDevice(device_);
}
namespace {
void validate_pcm(const std::shared_ptr<const PcmSound> &sound) {
  if (sound && (sound->rate != 48000 || sound->stereo.empty() ||
                sound->stereo.size() % 2))
    throw FormatError("invalid mixer PCM format");
  if (sound) {
    std::size_t previous=0;
    for (const auto &span:sound->class_spans) {
      if (span.end_frame<=previous || span.end_frame>sound->frames() ||
          span.sound_class>sound_class_count)
        throw FormatError("invalid sound phase class boundaries");
      previous=span.end_frame;
    }
  }
}
std::pair<float, float> levels(float gain, float pan) {
  if (!std::isfinite(gain) || !std::isfinite(pan))
    throw FormatError("invalid mixer levels");
  pan = std::clamp(pan, -1.f, 1.f);
  gain = std::clamp(gain, 0.f, 2.f);
  return {gain * (pan > 0 ? 1 - pan : 1), gain * (pan < 0 ? 1 + pan : 1)};
}
} // namespace
AudioEngine::VoiceId AudioEngine::play(std::shared_ptr<const PcmSound> sound,
                                       float gain, float pan, bool loop) {
  if (!sound)
    throw FormatError("missing mixer sound");
  return play_sequence(sound, loop ? sound : nullptr, nullptr, gain, pan);
}
AudioEngine::VoiceId AudioEngine::play_sequence(
    std::shared_ptr<const PcmSound> start, std::shared_ptr<const PcmSound> loop,
    std::shared_ptr<const PcmSound> end, float gain, float pan,
    LoopNext choose_loop) {
  validate_pcm(start);
  validate_pcm(loop);
  validate_pcm(end);
  if (!start && !loop && choose_loop) start = choose_loop();
  validate_pcm(start);
  if (!start && !loop)
    throw FormatError("empty audio sequence");
  levels(gain,pan);
  const auto [left, right] = levels(1, pan);
  SDL_LockAudioDevice(device_);
  if (voices_.size() == 32) {
    const auto expendable =
        std::find_if(voices_.begin(), voices_.end(),
                     [](const auto &v) { return !v.looping; });
    if (expendable == voices_.end()) {
      SDL_UnlockAudioDevice(device_);
      throw FormatError("looping audio voice capacity exhausted");
    }
    voices_.erase(expendable);
  }
  const auto id = next_id_++;
  voices_.push_back({start ? std::move(start) : loop, loop, std::move(end), id,
                     0, left, right, std::clamp(gain,0.f,2.f), bool(loop) || bool(choose_loop),
                     std::move(choose_loop), {}});
  SDL_UnlockAudioDevice(device_);
  return id;
}
void AudioEngine::stop(VoiceId id, bool immediate) {
  SDL_LockAudioDevice(device_);
  for (auto &v : voices_)
    if (v.id == id) {
      v.next.reset();
      v.choose_loop = {};
      v.fade.reset();
      v.looping = false;
      if (!immediate && v.end) {
        v.sound = std::move(v.end);
        v.cursor = 0;
      } else
        v.cursor = v.sound->frames();
    }
  SDL_UnlockAudioDevice(device_);
}
void AudioEngine::fade_out(VoiceId id, float seconds, std::uint16_t curve) {
  if (!std::isfinite(seconds) || seconds < 0 || seconds > 600 || curve > 1)
    throw FormatError("Invalid source sound fade");
  SDL_LockAudioDevice(device_);
  for (auto &v : voices_) if (v.id == id) {
    const auto time = std::bit_cast<std::int32_t>(std::uint32_t(mixed_frames_.load()/48));
    const float gain = v.fade ? source_sound_playback_fade(*v.fade,time) : 0.f;
    v.fade = SoundPlaybackFade{curve,gain,std::int32_t(seconds*1000.f),-1,true};
  }
  SDL_UnlockAudioDevice(device_);
}
void AudioEngine::parameters(VoiceId id, float gain, float pan) {
  levels(gain,pan);
  const auto [left, right] = levels(1, pan);
  SDL_LockAudioDevice(device_);
  for (auto &v : voices_)
    if (v.id == id) {
      v.left = left;
      v.right = right;
      v.gain = std::clamp(gain,0.f,2.f);
    }
  SDL_UnlockAudioDevice(device_);
}
void AudioEngine::next_loop(VoiceId id, std::shared_ptr<const PcmSound> sound) {
  validate_pcm(sound);
  if (!sound)
    throw FormatError("empty replacement loop");
  SDL_LockAudioDevice(device_);
  for (auto &v : voices_)
    if (v.id == id && v.looping)
      v.next = sound;
  SDL_UnlockAudioDevice(device_);
}
std::size_t AudioEngine::active_voices() const {
  SDL_LockAudioDevice(device_);
  const auto count = voices_.size();
  SDL_UnlockAudioDevice(device_);
  return count;
}
void AudioEngine::stop() {
  SDL_LockAudioDevice(device_);
  voices_.clear();
  SDL_UnlockAudioDevice(device_);
}
unsigned PcmSound::class_at(std::size_t frame) const {
  const auto span=std::upper_bound(class_spans.begin(),class_spans.end(),frame,
      [](std::size_t frame,const ClassSpan &span){return frame<span.end_frame;});
  return span == class_spans.end() ? sound_class : span->sound_class;
}
std::string AudioEngine::speaker_layout(unsigned layout) {
  if(layout>4)return "Invalid speaker configuration";
  if(layout==speaker_layout_)return {};
  unsigned channels=layout==2?4:layout==3?6:2;
#if SDL_VERSION_ATLEAST(2,24,0)
  if(layout==4) {SDL_AudioSpec actual{};if(SDL_GetDefaultAudioInfo(nullptr,&actual,0)==0 && actual.channels>=2 && actual.channels<=8)channels=actual.channels;}
#endif
  if(channels==channels_) {speaker_layout_=layout;return {};}
  if(!realtime_) {speaker_layout_=layout;channels_=channels;return {};}
  SDL_AudioSpec spec{};spec.freq=48000;spec.format=AUDIO_F32SYS;spec.channels=channels;
  spec.samples=1024;spec.callback=callback;spec.userdata=this;
  const auto replacement=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
  if(!replacement)return std::string("Cannot open selected speaker configuration: ")+SDL_GetError();
  SDL_PauseAudioDevice(device_,1);SDL_CloseAudioDevice(device_);
  device_=replacement;channels_=channels;speaker_layout_=layout;
  SDL_PauseAudioDevice(device_,0);return {};
}
void AudioEngine::master_volume(float gain) {
  if(!std::isfinite(gain) || gain<0 || gain>1)throw FormatError("Invalid native master volume");
  master_volume_.store(gain);
}
void AudioEngine::class_gains(const std::array<float,sound_class_count> &gains) {
  for (auto gain:gains) if (!std::isfinite(gain) || gain < -64 || gain > 0)
    throw FormatError("invalid mixer class gain");
  SDL_LockAudioDevice(device_);
  class_gains_=gains;
  SDL_UnlockAudioDevice(device_);
}
std::vector<float> AudioEngine::render(std::size_t frames) {
  if (realtime_ || frames > std::size_t(INT_MAX)/(channels_*sizeof(float)))
    throw FormatError("offline render requires a paused mixer and bounded frames");
  std::vector<float> output(frames*channels_);
  SDL_LockAudioDevice(device_);
  callback(this,reinterpret_cast<Uint8*>(output.data()),int(output.size()*sizeof(float)));
  SDL_UnlockAudioDevice(device_);
  return output;
}
void AudioEngine::callback(void *ptr, Uint8 *stream, int bytes) {
  auto &engine = *static_cast<AudioEngine *>(ptr);
  auto *output = reinterpret_cast<float *>(stream);
  const auto samples = std::size_t(bytes) / sizeof(float);
  std::fill(output, output + samples, 0.f);
  if(engine.paused_.load()) {engine.peak_.store(0);return;}
  const auto channels=engine.channels_;
  for (auto &voice : engine.voices_) {
    std::size_t written = 0;
    while (written < samples / channels) {
      if (voice.cursor == voice.sound->frames()) {
        if (!voice.looping)
          break;
        auto next = voice.next;
        if (voice.choose_loop) {
          try { next = voice.choose_loop(); }
          catch (...) { voice.looping = false; break; }
        }
        if (!next || next->rate != 48000 || next->stereo.empty() || next->stereo.size() % 2) {
          voice.looping = false;
          break;
        }
        voice.sound = std::move(next);
        voice.cursor = 0;
      }
      const auto n =
          std::min(samples / channels - written, voice.sound->frames() - voice.cursor);
      unsigned last_class=UINT_MAX;
      float gain=voice.gain;
      const auto caller_db=sound_linear_to_db(voice.gain);
      auto last_fade_time=INT32_MIN;
      float fade_db=0;
      for (std::size_t i = 0; i < n; ++i) {
        const auto sound_class=voice.sound->class_at(voice.cursor+i);
        bool fade_changed=false;
        if (voice.fade) {
          const auto time=std::bit_cast<std::int32_t>(std::uint32_t(
              (engine.mixed_frames_.load()+written+i)/48));
          if(time!=last_fade_time) {
            fade_db=source_sound_playback_fade(*voice.fade,time);
            last_fade_time=time;fade_changed=true;
          }
          if (fade_db+caller_db<=-64.f && voice.fade->start>voice.fade->end) {
            voice.looping=false;voice.next.reset();voice.choose_loop={};
            voice.cursor=voice.sound->frames();
            break;
          }
        }
        if (sound_class != last_class || fade_changed) {
          last_class=sound_class;
          gain=sound_class < sound_class_count || voice.fade ?
              sound_db_to_linear(caller_db+fade_db+(sound_class<sound_class_count ?
                  engine.class_gains_[sound_class] : 0.f)) : voice.gain;
        }
        output[(written + i) * channels] +=
            voice.sound->stereo[(voice.cursor + i) * 2] * voice.left * gain;
        output[(written + i) * channels + 1] +=
            voice.sound->stereo[(voice.cursor + i) * 2 + 1] * voice.right * gain;
      }
      if (!voice.looping && voice.cursor==voice.sound->frames()) break;
      voice.cursor += n;
      written += n;
    }
  }
  std::erase_if(engine.voices_, [](const Voice &v) {
    return !v.looping && v.cursor == v.sound->frames();
  });
  float peak = 0;
  const float master=engine.master_volume_.load();
  for (std::size_t i = 0; i < samples; ++i) {
    output[i] = std::clamp(output[i]*master, -1.f, 1.f);
    peak = std::max(peak, std::abs(output[i]));
  }
  if (peak > engine.peak_.load())
    engine.peak_.store(peak);
  engine.mixed_frames_.fetch_add(samples / channels);
}
} // namespace halo2
