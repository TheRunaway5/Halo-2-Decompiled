#include "halo2/movie.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <condition_variable>
#include <deque>
#include <exception>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <mutex>
#include <string_view>
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}
namespace halo2 {
namespace {
void check(int code, const char *operation) {
  if (code >= 0)
    return;
  char message[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(code, message, sizeof(message));
  throw FormatError(std::string(operation) + ": " + message);
}

using Json = nlohmann::json;
bool digest_string(const Json &value) {
  return value.is_string() && value.get_ref<const std::string &>().size() == 64 &&
         std::all_of(value.get_ref<const std::string &>().begin(),
                     value.get_ref<const std::string &>().end(), [](char c) {
                       return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                     });
}
bool owned(const std::filesystem::path &path, bool directory = false) {
  std::filesystem::path component;
  for (const auto &part : std::filesystem::absolute(path).lexically_normal()) {
    component /= part;
    std::error_code error;
    const auto state = std::filesystem::symlink_status(component, error);
    if (error == std::errc::no_such_file_or_directory ||
        state.type() == std::filesystem::file_type::not_found)
      return false;
    if (error || std::filesystem::is_symlink(state))
      throw FormatError("movie path has no private owner: " + component.string());
    if (component != std::filesystem::absolute(path).lexically_normal() &&
        !std::filesystem::is_directory(state))
      throw FormatError("invalid movie parent: " + component.string());
  }
  return directory ? std::filesystem::is_directory(path)
                   : std::filesystem::is_regular_file(path);
}
std::string bounded_metadata(const std::filesystem::path &path,
                             std::uintmax_t maximum) {
  if (!owned(path))
    throw FormatError("missing movie metadata: " + path.string());
  const auto size = std::filesystem::file_size(path);
  if (!size || size > maximum)
    throw FormatError("invalid movie metadata extent");
  std::ifstream input(path, std::ios::binary);
  std::string text(std::size_t(size), '\0');
  input.read(text.data(), std::streamsize(size));
  if (!input || input.peek() != std::char_traits<char>::eof())
    throw FormatError("movie metadata changed while reading");
  return text;
}
Json metadata(const std::string &text) {
  try {
    const auto value = Json::parse(text);
    if (!value.is_object())
      throw FormatError("invalid movie metadata object");
    return value;
  } catch (const Json::exception &) {
    throw FormatError("invalid movie metadata JSON");
  }
}
void movie_record(const Json &row, bool role) {
  if (!row.is_object() || row.size() != (role ? 3u : 2u) ||
      !row.contains("bytes") || !row["bytes"].is_number_unsigned() ||
      !row["bytes"].get<std::uint64_t>() ||
      row["bytes"].get<std::uint64_t>() >= (std::uint64_t(1) << 63) ||
      !row.contains("sha256") || !digest_string(row["sha256"]))
    throw FormatError("invalid movie size/SHA-256 record");
}
std::string file_sha(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw FormatError("cannot read lossless movie identity");
  auto *context = EVP_MD_CTX_new();
  if (!context)
    throw FormatError("cannot allocate movie SHA-256");
  const auto cleanup = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>(context, EVP_MD_CTX_free);
  if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1)
    throw FormatError("cannot initialize movie SHA-256");
  std::array<char, 64 * 1024> buffer;
  while (input) {
    input.read(buffer.data(), buffer.size());
    if (input.gcount() && EVP_DigestUpdate(context, buffer.data(), std::size_t(input.gcount())) != 1)
      throw FormatError("cannot hash movie payload");
  }
  if (!input.eof())
    throw FormatError("cannot finish movie identity read");
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned size{};
  if (EVP_DigestFinal_ex(context, digest, &size) != 1 || size != 32)
    throw FormatError("cannot finish movie SHA-256");
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < size; ++i) {
    result += hex[digest[i] >> 4];
    result += hex[digest[i] & 15];
  }
  return result;
}
struct LosslessPlayback {
  std::filesystem::path path;
  double duration{}, rate{};
  unsigned width{}, height{}, frames{};
};
std::optional<LosslessPlayback> lossless_playback(const std::filesystem::path &source) {
  if (source.filename() != "intro_60.mp4")
    return {};
  const auto base = std::unique_ptr<char, decltype(&SDL_free)>(SDL_GetBasePath(), SDL_free);
  if (!base)
    return {};
  const auto bank = std::filesystem::path(base.get()) / "movie-playback";
  try {
    if (!owned(bank / "manifest.json"))
      return {};
    const auto record = metadata(bounded_metadata(bank / "manifest.json", 64 * 1024));
    if (record.value("schema", std::string{}) != "H2_LOSSLESS_MOVIE_PLAYBACK_V1" ||
        record.value("role", std::string{}) != "intro" ||
        record.value("file", std::string{}) != "intro.hevc.mp4" ||
        !record.value("all_yuv_frames_identical", false) ||
        !record.value("copied_audio_packets", false) ||
        record.value("pixel_format", std::string{}) != "yuv420p" ||
        record.value("frame_rate", std::string{}) != "60000/1001" ||
        !record.contains("source") || !record.contains("sha256") || !digest_string(record["sha256"]))
      throw FormatError("invalid lossless movie receipt");
    movie_record(record["source"], false);
    const auto file = bank / "intro.hevc.mp4";
    // This optional derived asset is admitted only for the exact source and
    // complete verified payload. Original movies and their import metadata
    // remain untouched; a stale or modified cache falls back to the source.
    if (!owned(source) || !owned(file) ||
        std::filesystem::file_size(source) != record["source"]["bytes"].get<std::uint64_t>() ||
        std::filesystem::file_size(file) != record.at("bytes").get<std::uint64_t>() ||
        file_sha(source) != record["source"]["sha256"].get<std::string>() ||
        file_sha(file) != record["sha256"].get<std::string>())
      throw FormatError("lossless movie source/payload identity differs");
    LosslessPlayback result{file, record.at("source_duration").get<double>(), 60000.0 / 1001,
                            record.at("width").get<unsigned>(), record.at("height").get<unsigned>(),
                            record.at("frames").get<unsigned>()};
    if (!std::isfinite(result.duration) || result.duration <= 0 || result.duration > 600 ||
        !result.width || result.width > 4096 || !result.height || result.height > 2160 ||
        !result.frames || result.frames > 72000)
      throw FormatError("invalid lossless movie extent");
    return result;
  } catch (const std::exception &error) {
    std::clog << "Lossless movie cache ignored: " << error.what() << '\n';
    return {};
  }
}
} // namespace
struct Movie::Impl {
  AVFormatContext *format{};
  AVCodecContext *decoder{};
  AVFrame *frame = av_frame_alloc();
  AVFrame *download = av_frame_alloc();
  AVFrame *planar = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();
  SwsContext *converter{};
  Texture image;
  Texture displayed;
  struct ReadyFrame { unsigned number; Texture image; };
  std::deque<ReadyFrame> ready;
  std::deque<Texture> reusable;
  std::mutex mutex;
  std::condition_variable condition;
  std::thread worker;
  std::exception_ptr failure;
  unsigned presented{}, requested{};
  bool stopping{}, complete{}, initialized{};
  int stream{};
  unsigned frames{}, decoded{};
  double rate{}, length{};
  bool eof{}, drained{};
  AVPixelFormat hardware_format = AV_PIX_FMT_NONE;
  AVHWDeviceType hardware_device = AV_HWDEVICE_TYPE_NONE;
  bool used_hardware{};
  std::filesystem::path path, source_path;
  std::ofstream timings;
  ~Impl() {
    {
      std::lock_guard lock(mutex);
      stopping = true;
      condition.notify_all();
    }
    if (worker.joinable())
      worker.join();
    sws_freeContext(converter);
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&download);
    av_frame_free(&planar);
    avcodec_free_context(&decoder);
    avformat_close_input(&format);
  }
  void start(const std::filesystem::path &source, MovieDecodeMode mode) {
    worker = std::thread([this, source, mode] {
      try {
        source_path = source;
        open(source, mode);
        next();
        if (const auto *output = std::getenv("HALO2_MOVIE_TIMINGS"); output && *output) {
          timings.open(output);
          timings << "clock_seconds\tframe\tadvance_ms\tdevice\n";
        }
        {
          std::lock_guard lock(mutex);
          presented = requested = decoded;
          displayed = std::move(image);
          image.pixels = std::vector<std::byte>(std::size_t(image.width) * image.height * 4);
          initialized = true;
          condition.notify_all();
        }
        for (;;) {
          std::unique_lock lock(mutex);
          condition.wait(lock, [this] {
            return stopping || decoded < std::max(requested, presented + 2);
          });
          if (stopping)
            return;
          if (decoded == frames) {
            complete = true;
            condition.notify_all();
            return;
          }
          const bool display = decoded + 1 >= requested;
          lock.unlock();
          const bool available = next(display);
          lock.lock();
          if (!available) {
            complete = true;
            condition.notify_all();
            return;
          }
          if (display) {
            ready.push_back({decoded, std::move(image)});
            if (!reusable.empty()) {
              image = std::move(reusable.front());
              reusable.pop_front();
            } else {
              image.pixels = std::vector<std::byte>(std::size_t(image.width) * image.height * 4);
            }
          }
          condition.notify_all();
        }
      } catch (...) {
        std::lock_guard lock(mutex);
        failure = std::current_exception();
        condition.notify_all();
      }
    });
    std::unique_lock lock(mutex);
    condition.wait(lock, [this] { return initialized || failure; });
    if (failure)
      std::rethrow_exception(failure);
  }
  bool advance(unsigned wanted) {
    std::unique_lock lock(mutex);
    if (wanted <= presented)
      return false;
    requested = wanted;
    condition.notify_all();
    for (;;) {
      while (!ready.empty()) {
        auto frame = std::move(ready.front());
        ready.pop_front();
        if (frame.number < wanted) {
          reusable.push_back(std::move(frame.image));
          continue;
        }
        reusable.push_back(std::move(displayed));
        displayed = std::move(frame.image);
        presented = frame.number;
        condition.notify_all();
        return true;
      }
      if (failure)
        std::rethrow_exception(failure);
      if (complete)
        throw FormatError("movie ended before requested frame");
      condition.wait(lock);
    }
  }
  static AVPixelFormat hardware_pixel_format(AVCodecContext *context,
                                            const AVPixelFormat *formats) {
    const auto wanted = static_cast<Impl *>(context->opaque)->hardware_format;
    for (; *formats != AV_PIX_FMT_NONE; ++formats)
      if (*formats == wanted)
        return wanted;
    return AV_PIX_FMT_NONE;
  }
  const AVCodec *hardware_codec(const AVCodecParameters *parameters,
                                const AVCodec *software) {
    // Keep unsupported pixel layouts on the reference software path. The
    // bundled eight-bit movies can use NVDEC/VAAPI/etc without changing YUV.
    if (parameters->format != AV_PIX_FMT_YUV420P)
      return software;
    const auto *native = parameters->codec_id == AV_CODEC_ID_AV1
                             ? avcodec_find_decoder_by_name("av1") : software;
    if (!native)
      return software;
    const auto *backend = std::getenv("HALO2_MOVIE_HWACCEL");
    const auto *device_name = std::getenv("HALO2_MOVIE_HW_DEVICE");
    for (const auto device : {AV_HWDEVICE_TYPE_CUDA, AV_HWDEVICE_TYPE_D3D11VA,
                              AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_HWDEVICE_TYPE_VAAPI}) {
      if (backend && *backend && std::string_view(backend) != "auto" &&
          std::string_view(backend) != av_hwdevice_get_type_name(device))
        continue;
      for (int index = 0; const auto *config = avcodec_get_hw_config(native, index); ++index) {
        if (config->device_type != device ||
            !(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
          continue;
        AVBufferRef *owner{};
        if (av_hwdevice_ctx_create(&owner, device,
                                  device_name && *device_name ? device_name : nullptr,
                                  nullptr, 0) < 0) {
          // VAAPI's default render node can belong to another GPU. Probe
          // available DRM nodes so AMD and Intel work on multi-GPU Linux hosts
          // without requiring a vendor-specific environment setting.
          if (device != AV_HWDEVICE_TYPE_VAAPI || (device_name && *device_name))
            continue;
          for (unsigned node = 128; node < 192 && !owner; ++node) {
            const auto name = "/dev/dri/renderD" + std::to_string(node);
            std::error_code error;
            if (!std::filesystem::exists(name, error))
              continue;
            av_hwdevice_ctx_create(&owner, device, name.c_str(), nullptr, 0);
          }
          if (!owner)
            continue;
        }
        decoder->hw_device_ctx = owner;
        hardware_format = config->pix_fmt;
        hardware_device = device;
        decoder->opaque = this;
        decoder->get_format = hardware_pixel_format;
        return native;
      }
    }
    return software;
  }
  void open(const std::filesystem::path &source, MovieDecodeMode mode) {
    const auto playback = mode == MovieDecodeMode::Automatic ? lossless_playback(source) : std::nullopt;
    path = playback ? playback->path : source;
    if (!frame || !download || !planar || !packet)
      throw FormatError("cannot allocate movie decoder");
    check(avformat_open_input(&format, path.string().c_str(), nullptr, nullptr),
          "open movie");
    check(avformat_find_stream_info(format, nullptr), "read movie streams");
    stream =
        av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    check(stream, "find movie video");
    const auto *track = format->streams[stream];
    const auto *p = track->codecpar;
    const bool bink = p->codec_id == AV_CODEC_ID_BINKVIDEO;
    if ((!bink && p->codec_id != AV_CODEC_ID_H264 && p->codec_id != AV_CODEC_ID_AV1 && p->codec_id != AV_CODEC_ID_HEVC) ||
        p->width < 1 || p->height < 1 || p->width > 4096 || p->height > 2160)
      throw FormatError("unsupported movie video format");
    rate = av_q2d(av_guess_frame_rate(format, format->streams[stream], nullptr));
    length = track->duration * av_q2d(track->time_base);
    if (!std::isfinite(rate) || rate < 1 || rate > 120 ||
        !std::isfinite(length) || length <= 0 || length > 600)
      throw FormatError("invalid movie frame rate/duration");
    const auto count = track->nb_frames > 0 ? track->nb_frames : std::llround(length * rate);
    if (count < 1 || count > 72000)
      throw FormatError("invalid movie frame count");
    frames = unsigned(count);
    // Keep the final video frame through the slightly longer AAC audio tail.
    if (!bink && format->duration != AV_NOPTS_VALUE) {
      const double container_length = double(format->duration) / AV_TIME_BASE;
      if (container_length > length && container_length <= 600)
        length = container_length;
    }
    if (playback) {
      if (p->codec_id != AV_CODEC_ID_HEVC || unsigned(p->width) != playback->width ||
          unsigned(p->height) != playback->height || frames != playback->frames ||
          rate != playback->rate)
        throw FormatError("lossless movie stream differs from its verified receipt");
      length = playback->duration;
      std::clog << "Lossless movie playback: " << path << '\n';
    }
    const auto *codec = avcodec_find_decoder(p->codec_id);
    if (!codec)
      throw FormatError("FFmpeg lacks required movie video decoder");
    decoder = avcodec_alloc_context3(nullptr);
    if (!decoder)
      throw FormatError("cannot allocate movie codec");
    check(avcodec_parameters_to_context(decoder, p), "configure movie decoder");
    if (!bink && mode == MovieDecodeMode::Automatic)
      codec = hardware_codec(p, codec);
    if (playback && hardware_device == AV_HWDEVICE_TYPE_NONE) {
      // HEVC is a GPU compatibility copy. The original H.264 is faster in
      // software on the measured host, so CPU fallback keeps the source codec.
      avcodec_free_context(&decoder);
      avformat_close_input(&format);
      open(source, MovieDecodeMode::Software);
      return;
    }
    // Let software codecs use the available AMD/Intel/other CPU cores. The
    // lossless 4K intro can exceed a fixed four-thread budget. Retain a small
    // hardware frame-thread pool so upload, decode and download can overlap.
    decoder->thread_count = hardware_device == AV_HWDEVICE_TYPE_NONE ? 0 : 4;
    check(avcodec_open2(decoder, codec, nullptr), "open movie decoder");
    image = {unsigned(p->width),
             unsigned(p->height),
             11,
             std::vector<std::byte>(std::size_t(p->width) * p->height * 4),
             {}};
  }
  // NVDEC downloads eight-bit frames as NV12. Deinterleave without resampling
  // before the existing planar-YUV conversion: swscale's direct NV12-to-RGBA
  // kernel rounds differently from its YUV420P kernel.
  AVFrame *software_frame() {
    if (frame->format != hardware_format || hardware_format == AV_PIX_FMT_NONE)
      return frame;
    av_frame_unref(download);
    check(av_hwframe_transfer_data(download, frame, 0), "download movie frame");
    check(av_frame_copy_props(download, frame), "copy movie frame properties");
    if (!used_hardware)
      used_hardware = true;
    if (download->format == AV_PIX_FMT_YUV420P)
      return download;
    if (download->format != AV_PIX_FMT_NV12)
      throw FormatError("unsupported hardware movie pixel layout");
    if (!planar->data[0]) {
      planar->format = AV_PIX_FMT_YUV420P;
      planar->width = frame->width;
      planar->height = frame->height;
      check(av_frame_get_buffer(planar, 32), "allocate planar movie frame");
    }
    check(av_frame_make_writable(planar), "reuse planar movie frame");
    for (int y = 0; y < frame->height; ++y)
      std::memcpy(planar->data[0] + y * planar->linesize[0],
                  download->data[0] + y * download->linesize[0], frame->width);
    for (int y = 0; y < (frame->height + 1) / 2; ++y) {
      const auto *uv = download->data[1] + y * download->linesize[1];
      auto *u = planar->data[1] + y * planar->linesize[1];
      auto *v = planar->data[2] + y * planar->linesize[2];
      for (int x = 0; x < (frame->width + 1) / 2; ++x) {
        u[x] = uv[2 * x];
        v[x] = uv[2 * x + 1];
      }
    }
    check(av_frame_copy_props(planar, frame), "copy planar movie properties");
    return planar;
  }
  bool next(bool display = true) {
    if (drained)
      return false;
    for (;;) {
      const int result = avcodec_receive_frame(decoder, frame);
      if (result == AVERROR_EOF) {
        drained = true;
        if (decoded != frames)
          throw FormatError("movie ended before its declared frame count");
        return false;
      }
      if (result != AVERROR(EAGAIN)) {
        check(result, "decode movie frame");
        if (frame->width != int(image.width) ||
            frame->height != int(image.height))
          throw FormatError("movie changes dimensions");
        // VAAPI drivers can recycle an unsynchronized discarded output before
        // later AV1 reference/show-existing frames have finished using it.
        // Synchronize downloads even during catch-up; omit only RGBA work.
        const auto *pixels_frame = (display || hardware_device == AV_HWDEVICE_TYPE_VAAPI)
                                     ? software_frame() : nullptr;
        if (display) {
          converter = sws_getCachedContext(
              converter, pixels_frame->width, pixels_frame->height,
              AVPixelFormat(pixels_frame->format), pixels_frame->width, pixels_frame->height,
              AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
          if (!converter)
            throw FormatError("cannot convert movie pixels");
          // Retain Bink's limited BT.601 conversion. HD MP4s use their declared
          // matrix/range, with BT.709 for these untagged upscaled sources.
          const bool bink = decoder->codec_id == AV_CODEC_ID_BINKVIDEO;
          int matrix = SWS_CS_ITU709;
          if (bink || frame->colorspace == AVCOL_SPC_SMPTE170M ||
              frame->colorspace == AVCOL_SPC_BT470BG)
            matrix = SWS_CS_ITU601;
          else if (frame->colorspace == AVCOL_SPC_BT2020_NCL)
            matrix = SWS_CS_BT2020;
          const auto *coefficients = sws_getCoefficients(matrix);
          const int full_range = !bink && frame->color_range == AVCOL_RANGE_JPEG;
          check(sws_setColorspaceDetails(converter, coefficients, full_range, coefficients,
                                         1, 0, 1 << 16, 1 << 16),
                "configure movie color conversion");
          unsigned char *pixels[]{
              reinterpret_cast<unsigned char *>(image.pixels.data())};
          int pitch[]{int(image.width * 4)};
          if (sws_scale(converter, pixels_frame->data, pixels_frame->linesize, 0, frame->height,
                        pixels, pitch) != frame->height)
            throw FormatError("incomplete movie pixel conversion");
        }
        av_frame_unref(frame);
        if (++decoded > frames)
          throw FormatError("movie exceeds its declared frame count");
        return true;
      }
      if (eof)
        throw FormatError("movie decoder failed to drain");
      int read;
      do {
        av_packet_unref(packet);
        read = av_read_frame(format, packet);
      } while (read >= 0 && packet->stream_index != stream);
      if (read == AVERROR_EOF) {
        eof = true;
        check(avcodec_send_packet(decoder, nullptr), "flush movie video");
      } else {
        check(read, "read movie video");
        check(avcodec_send_packet(decoder, packet), "submit movie video");
      }
    }
  }
};
Movie::Movie(const std::filesystem::path &path) : Movie(path, MovieDecodeMode::Automatic) {}
Movie::Movie(const std::filesystem::path &path, MovieDecodeMode mode)
    : impl_(std::make_unique<Impl>()) {
  if (const auto *reference = std::getenv("HALO2_REFERENCE_SOFTWARE_MOVIES");
      reference && *reference)
    mode = MovieDecodeMode::Software;
  try {
    impl_->start(path, mode);
  } catch (const FormatError &error) {
    const bool derived = impl_->path != path;
    if (impl_->hardware_device == AV_HWDEVICE_TYPE_NONE && !derived)
      throw;
    std::clog << "Movie decoder fallback: " << error.what() << '\n';
    // Reopen the original for either device failure or a malformed optional
    // stream. The lossless copy is specifically a GPU compatibility asset.
    const bool device_failure = impl_->hardware_device != AV_HWDEVICE_TYPE_NONE;
    const auto fallback_path = path;
    const auto authored_length = impl_->length;
    impl_ = std::make_unique<Impl>();
    impl_->start(fallback_path, MovieDecodeMode::Software);
    if (device_failure && authored_length > 0)
      impl_->length = authored_length;
  }
}
Movie::~Movie() = default;
const Texture &Movie::image() const { return impl_->displayed; }
bool Movie::advance(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError("invalid movie playback time");
  const auto wanted = unsigned(
      std::min(double(impl_->frames), std::floor(seconds * impl_->rate) + 1));
  const auto begin = std::chrono::steady_clock::now();
  bool changed;
  try {
    changed = impl_->advance(wanted);
  } catch (const FormatError &) {
    if (impl_->hardware_device == AV_HWDEVICE_TYPE_NONE)
      throw;
    // Recover an unavailable/lost decode device at the same authored frame.
    // If the stream itself is invalid, the software decoder still rejects it.
    const auto path = impl_->source_path;
    const auto authored_length = impl_->length;
    impl_ = std::make_unique<Impl>();
    impl_->start(path, MovieDecodeMode::Software);
    impl_->length = authored_length;
    impl_->advance(wanted);
    changed = true;
  }
  if (impl_->timings.is_open()) {
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    impl_->timings << seconds << '\t' << impl_->presented << '\t' << elapsed << '\t' << decode_device() << '\n';
    impl_->timings.flush();
  }
  return changed;
}
bool Movie::finished(double seconds) const { return seconds >= impl_->length; }
unsigned Movie::frame_count() const { return impl_->frames; }
unsigned Movie::frame_number() const { return impl_->presented; }
double Movie::duration() const { return impl_->length; }
double Movie::frame_rate() const { return impl_->rate; }
const char *Movie::decode_device() const {
  return impl_->used_hardware ? av_hwdevice_get_type_name(impl_->hardware_device) : "software";
}
std::filesystem::path frontend_movie_path(const std::filesystem::path &maps,
                                        const std::filesystem::path &bundle,
                                        const std::string &name) {
  if (name != "intro" && name != "credits" && name != "attract")
    throw FormatError("unknown frontend movie: " + name);
  // No authenticated MCC source has been assigned to these frontend roles.
  // Retained supplied banks and arbitrary collection .bk2 files do not establish
  // that ownership; leave the menu active without opening their metadata.
  if (maps.filename() == "h2_maps_win64_dx11" && maps.parent_path().filename() == "halo2")
    return {};
  if (name != "attract") {
    const auto replacement = bundle / (name + "_60.mp4");
    if (!bundle.empty() && std::filesystem::is_regular_file(replacement))
      return replacement;
  }
  return maps.parent_path() / "bink" / (name + "_60.bik");
}
std::array<float, 4> movie_canvas_rectangle(unsigned width, unsigned height,
                                          unsigned viewport_width,
                                          unsigned viewport_height) {
  if (!width || !height || !viewport_width || !viewport_height)
    throw FormatError("empty movie canvas");
  const float scale = std::min(float(viewport_width) / width,
                               float(viewport_height) / height);
  const float w = width * scale * 640 / viewport_width;
  const float h = height * scale * 480 / viewport_height;
  return {(640 - w) * .5f, (480 - h) * .5f, w, h};
}
} // namespace halo2
