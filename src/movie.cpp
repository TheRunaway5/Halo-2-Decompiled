#include "halo2/movie.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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
constexpr const char *movie_schema = "H2_PORT_BUNDLED_MOVIES_V1";
constexpr const char *source_schema = "halo2-upscaled-movies-v1";
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
std::string metadata_sha(const std::string &text) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned size{};
  if (EVP_Digest(text.data(), text.size(), digest, &size, EVP_sha256(), nullptr) != 1 || size != 32)
    throw FormatError("cannot verify movie metadata SHA-256");
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < size; ++i) {
    result += hex[digest[i] >> 4];
    result += hex[digest[i] & 15];
  }
  return result;
}
void movie_record(const Json &row, bool role) {
  if (!row.is_object() || row.size() != (role ? 3u : 2u) ||
      !row.contains("bytes") || !row["bytes"].is_number_unsigned() ||
      !row["bytes"].get<std::uint64_t>() ||
      row["bytes"].get<std::uint64_t>() >= (std::uint64_t(1) << 63) ||
      !row.contains("sha256") || !digest_string(row["sha256"]))
    throw FormatError("invalid movie size/SHA-256 record");
}
void movie_roles(const Json &roles) {
  if (!roles.is_object() || roles.size() != 2 || !roles.contains("intro") || !roles.contains("credits"))
    throw FormatError("movie roles must be exactly intro and credits");
  for (const std::string role : {"intro", "credits"}) {
    const auto &row = roles[role];
    movie_record(row, true);
    if (row.value("path", std::string{}) != role + "_60.mp4")
      throw FormatError("invalid relative movie role path");
  }
}
std::filesystem::path role_file(const std::filesystem::path &bank,
                                const Json &roles, const std::string &name) {
  // Import/repair authenticates both complete payloads. Startup deliberately
  // checks only this pinned metadata and both file extents; FFmpeg validates
  // the stream on opening. It does not rehash 2.5 GB on every movie query.
  for (const std::string role : {"intro", "credits"}) {
    const auto file = bank / roles[role]["path"].get<std::string>();
    if (!owned(file) || std::filesystem::file_size(file) != roles[role]["bytes"].get<std::uint64_t>())
      throw FormatError("cached movie is missing or has changed extent: " + role);
  }
  return bank / roles[name]["path"].get<std::string>();
}
std::filesystem::path cached_movie(const std::filesystem::path &cache,
                                   const std::string &name) {
  if (!owned(cache / "provenance.json"))
    return {};
  const auto report = metadata(bounded_metadata(cache / "provenance.json", 4 * 1024 * 1024));
  if (!report.contains("port_bundled_movies"))
    return {};
  const auto &entry = report["port_bundled_movies"];
  if (report.value("schema", std::string{}) != "H2_MCC_TU15_PREPARATION_V1" ||
      report.value("external_installation_requirement", std::string{}) != "Halo MCC only" ||
      !entry.is_object() || entry.value("schema", std::string{}) != movie_schema ||
      entry.value("complete", false) != true ||
      entry.value("path", std::string{}) != "presentation/movies/manifest.json" ||
      !entry.contains("sha256") || !digest_string(entry["sha256"]))
    throw FormatError("invalid cached movie provenance");
  const auto bank = cache / "presentation/movies";
  const auto text = bounded_metadata(bank / "manifest.json", 64 * 1024);
  if (metadata_sha(text) != entry["sha256"].get<std::string>())
    throw FormatError("cached movie manifest SHA-256 differs");
  const auto record = metadata(text);
  if (record.value("schema", std::string{}) != movie_schema ||
      record.value("source_schema", std::string{}) != source_schema ||
      !record.value("complete", false) || !record.value("copied_without_transcoding", false) ||
      record.value("attract_assigned", true) || record.value("external_vista_or_xbox_required", true) ||
      !record.contains("source_manifest") || !record["source_manifest"].is_object() ||
      record["source_manifest"].value("path", std::string{}) != "resources/movie-manifest.json" ||
      !record["source_manifest"].contains("sha256") || !digest_string(record["source_manifest"]["sha256"]) ||
      !record.contains("roles") || !entry.contains("roles") || record["roles"] != entry["roles"])
    throw FormatError("invalid cached movie manifest");
  movie_roles(record["roles"]);
  return role_file(bank, record["roles"], name);
}
std::filesystem::path bundled_movie(const std::filesystem::path &bundle,
                                    const std::string &name) {
  if (bundle.empty() || !owned(bundle / "manifest.json") ||
      !owned(bundle.parent_path() / "movie-manifest.json"))
    return {};
  const auto record = metadata(bounded_metadata(bundle / "manifest.json", 64 * 1024));
  if (record != metadata(bounded_metadata(bundle.parent_path() / "movie-manifest.json", 64 * 1024)) ||
      record.value("schema", std::string{}) != source_schema ||
      !record.contains("files") || !record["files"].is_object() || record["files"].size() != 2)
    throw FormatError("bundled movie source manifests disagree");
  Json roles = Json::object();
  for (const std::string role : {"intro", "credits"}) {
    const auto file = role + "_60.mp4";
    if (!record["files"].contains(file))
      throw FormatError("bundled movie role is missing");
    movie_record(record["files"][file], false);
    roles[role] = record["files"][file];
    roles[role]["path"] = file;
  }
  return role_file(bundle, roles, name);
}
} // namespace
struct Movie::Impl {
  AVFormatContext *format{};
  AVCodecContext *decoder{};
  AVFrame *frame = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();
  SwsContext *converter{};
  Texture image;
  int stream{};
  unsigned frames{}, decoded{};
  double rate{}, length{};
  bool eof{}, drained{};
  ~Impl() {
    sws_freeContext(converter);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&decoder);
    avformat_close_input(&format);
  }
  void open(const std::filesystem::path &path) {
    if (!frame || !packet)
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
    if ((!bink && p->codec_id != AV_CODEC_ID_H264 && p->codec_id != AV_CODEC_ID_AV1) ||
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
    const auto *codec = avcodec_find_decoder(p->codec_id);
    if (!codec)
      throw FormatError("FFmpeg lacks required movie video decoder");
    decoder = avcodec_alloc_context3(codec);
    if (!decoder)
      throw FormatError("cannot allocate movie codec");
    check(avcodec_parameters_to_context(decoder, p), "configure movie decoder");
    decoder->thread_count = 4;
    check(avcodec_open2(decoder, codec, nullptr), "open movie decoder");
    image = {unsigned(p->width),
             unsigned(p->height),
             11,
             std::vector<std::byte>(std::size_t(p->width) * p->height * 4),
             {}};
  }
  bool next() {
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
        converter = sws_getCachedContext(
            converter, frame->width, frame->height,
            AVPixelFormat(frame->format), frame->width, frame->height,
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
        if (sws_scale(converter, frame->data, frame->linesize, 0, frame->height,
                      pixels, pitch) != frame->height)
          throw FormatError("incomplete movie pixel conversion");
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
Movie::Movie(const std::filesystem::path &path)
    : impl_(std::make_unique<Impl>()) {
  impl_->open(path);
  impl_->next();
}
Movie::~Movie() = default;
const Texture &Movie::image() const { return impl_->image; }
bool Movie::advance(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0)
    throw FormatError("invalid movie playback time");
  const auto wanted = unsigned(
      std::min(double(impl_->frames), std::floor(seconds * impl_->rate) + 1));
  bool changed = false;
  while (impl_->decoded < wanted && impl_->next())
    changed = true;
  return changed;
}
bool Movie::finished(double seconds) const { return seconds >= impl_->length; }
unsigned Movie::frame_count() const { return impl_->frames; }
unsigned Movie::frame_number() const { return impl_->decoded; }
double Movie::duration() const { return impl_->length; }
double Movie::frame_rate() const { return impl_->rate; }
std::filesystem::path frontend_movie_path(const std::filesystem::path &maps,
                                        const std::filesystem::path &bundle,
                                        const std::string &name) {
  if (name != "intro" && name != "credits" && name != "attract")
    throw FormatError("unknown frontend movie: " + name);
  if (maps.filename() == "h2_maps_win64_dx11" && maps.parent_path().filename() == "halo2") {
    if (name == "attract")
      return {}; // No authenticated bundled/MCC attract role has been assigned.
    try {
      if (const auto cached = cached_movie(maps.parent_path().parent_path(), name); !cached.empty())
        return cached;
      return bundled_movie(bundle, name);
    } catch (const Json::exception &) {
      throw FormatError("invalid movie metadata field type");
    }
  }
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
