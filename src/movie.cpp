#include "halo2/movie.hpp"
#include <algorithm>
#include <cmath>
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
          "open retail movie");
    check(avformat_find_stream_info(format, nullptr), "read movie streams");
    stream =
        av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    check(stream, "find movie video");
    const auto *track = format->streams[stream];
    const auto *p = track->codecpar;
    if (p->codec_id != AV_CODEC_ID_BINKVIDEO || p->width < 1 || p->height < 1 ||
        p->width > 1920 || p->height > 1080)
      throw FormatError("unsupported retail Bink movie format");
    rate = av_q2d(track->r_frame_rate);
    length = track->duration * av_q2d(track->time_base);
    if (!std::isfinite(rate) || rate < 1 || rate > 120 ||
        !std::isfinite(length) || length <= 0 || length > 600)
      throw FormatError("invalid movie frame rate/duration");
    frames = unsigned(std::llround(length * rate));
    const auto *codec = avcodec_find_decoder(p->codec_id);
    if (!codec)
      throw FormatError("FFmpeg lacks Bink video decoder");
    decoder = avcodec_alloc_context3(codec);
    if (!decoder)
      throw FormatError("cannot allocate Bink codec");
    check(avcodec_parameters_to_context(decoder, p), "configure Bink decoder");
    check(avcodec_open2(decoder, codec, nullptr), "open Bink decoder");
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
        check(result, "decode Bink frame");
        if (frame->width != int(image.width) ||
            frame->height != int(image.height))
          throw FormatError("movie changes dimensions");
        converter = sws_getCachedContext(
            converter, frame->width, frame->height,
            AVPixelFormat(frame->format), frame->width, frame->height,
            AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!converter)
          throw FormatError("cannot convert movie pixels");
        // Bink 1 YUV is limited-range BT.601, matching the original 640x480
        // decode. Explicit settings keep host defaults from choosing BT.709.
        const auto *coefficients = sws_getCoefficients(SWS_CS_ITU601);
        check(sws_setColorspaceDetails(converter, coefficients, 0, coefficients,
                                       1, 0, 1 << 16, 1 << 16),
              "configure Bink color conversion");
        unsigned char *pixels[]{
            reinterpret_cast<unsigned char *>(image.pixels.data())};
        int pitch[]{int(image.width * 4)};
        if (sws_scale(converter, frame->data, frame->linesize, 0, frame->height,
                      pixels, pitch) != frame->height)
          throw FormatError("incomplete Bink pixel conversion");
        av_frame_unref(frame);
        if (++decoded > frames)
          throw FormatError("movie exceeds its declared frame count");
        return true;
      }
      if (eof)
        throw FormatError("Bink decoder failed to drain");
      int read;
      do {
        av_packet_unref(packet);
        read = av_read_frame(format, packet);
      } while (read >= 0 && packet->stream_index != stream);
      if (read == AVERROR_EOF) {
        eof = true;
        check(avcodec_send_packet(decoder, nullptr), "flush Bink video");
      } else {
        check(read, "read Bink video");
        check(avcodec_send_packet(decoder, packet), "submit Bink video");
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
} // namespace halo2
