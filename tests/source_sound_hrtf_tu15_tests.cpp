#include "halo2/original_data.hpp"
#include "halo2/source_sound_doppler_tu15.hpp"
#include "halo2/source_sound_hrtf_tu15.hpp"
#include "halo2/source_sound_obstruction_tu15.hpp"
#include "halo2/source_sound_reverb_tu15.hpp"
#include "halo2/sound_i3dl2_tu15.hpp"

#include <algorithm>

#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = 1e-5f) { return std::abs(a - b) <= tolerance; }
double power(const std::array<float, SdlSoundHrtf::taps> &taps) {
  double out = 0;
  for (const auto tap : taps) out += double(tap) * tap;
  return out;
}
// The ear's largest gain over 0..24 kHz in 375 Hz steps.
double peak_gain(const std::array<float, SdlSoundHrtf::taps> &taps) {
  double peak = 0;
  for (unsigned bin = 0; bin <= 64; ++bin) {
    double re = 0, im = 0;
    for (unsigned n = 0; n < taps.size(); ++n) {
      re += taps[n] * std::cos(M_PI * bin / 64 * n);
      im -= taps[n] * std::sin(M_PI * bin / 64 * n);
    }
    peak = std::max(peak, std::sqrt(re * re + im * im));
  }
  return peak;
}
// The ear's response above 4 kHz relative to its whole response: a
// head-shadowed ear keeps less of it.
double treble_share(const std::array<float, SdlSoundHrtf::taps> &taps) {
  double treble = 0, all = 0;
  for (unsigned bin = 1; bin < 64; ++bin) {
    const double frequency = 24000.0 * bin / 64;
    double re = 0, im = 0;
    for (unsigned n = 0; n < taps.size(); ++n) {
      re += taps[n] * std::cos(2 * M_PI * frequency / 48000 * n);
      im -= taps[n] * std::sin(2 * M_PI * frequency / 48000 * n);
    }
    const double magnitude = re * re + im * im;
    all += magnitude;
    if (frequency > 4000) treble += magnitude;
  }
  return treble / all;
}

void coefficients() {
  check(source_sound_hrtf_coefficient_tu15(0x00) == 0 && source_sound_hrtf_coefficient_tu15(0x0f) == 15 &&
            source_sound_hrtf_coefficient_tu15(0x10) == 16 && source_sound_hrtf_coefficient_tu15(0x1f) == 31 &&
            source_sound_hrtf_coefficient_tu15(0x20) == 32 && source_sound_hrtf_coefficient_tu15(0x45) == 168 &&
            source_sound_hrtf_coefficient_tu15(0x7f) == 1984 && source_sound_hrtf_coefficient_tu15(0x85) == -5 &&
            source_sound_hrtf_coefficient_tu15(0xc5) == -168,
        "HRTF coefficient byte decoded wrongly");
}

void head_frame() {
  // Facing +x with z up, -y is to the right.
  const Vec3 listener{1, 2, 3}, forward{2, 0, 0}, up{0, 0, 1};
  const auto right = source_sound_head_position_tu15(listener, forward, up, {1, 1, 3});
  const auto ahead = source_sound_head_position_tu15(listener, forward, up, {3, 2, 3});
  const auto above = source_sound_head_position_tu15(listener, forward, up, {1, 2, 4});
  check(right && near(right->x, 1) && near(right->y, 0) && near(right->z, 0), "Head frame lost its right axis");
  check(ahead && near(ahead->x, 0) && near(ahead->y, 0) && near(ahead->z, 2), "Head frame lost its forward axis");
  check(above && near(above->x, 0) && near(above->y, 1) && near(above->z, 0), "Head frame lost its up axis");
  check(!source_sound_head_position_tu15(listener, forward, forward, {0, 0, 0}),
        "A degenerate head frame placed a voice");
}

void original_kernels() {
  const auto data = OriginalData::native_tu15();
  check(data && data->sound_sdk_spatial_data, "TU1.5 original data lacks its SDK HRTF table");
  const SourceSoundHrtfTu15 hrtf(*data->sound_sdk_spatial_data);

  const auto ahead = hrtf.filter({0, 0, 10});
  check(near(hrtf.azimuth({0, 0, 10}), 0) && ahead.left == ahead.right && !ahead.left_delay && !ahead.right_delay,
        "Straight ahead is not one undelayed kernel for both ears");
  // The voice processor's scale (coefficient / 2048): straight ahead is
  // about 12 dB down in power, and no kernel's gain reaches full scale.
  check(std::abs(10 * std::log10(power(ahead.left)) + 11.94) < 0.05, "Straight-ahead kernel left the voice processor's scale");
  double loudest = 0;
  for (int degrees = -180; degrees < 180; degrees += 3) {
    const auto placed = hrtf.filter({std::sin(float(degrees * M_PI / 180)), 0, std::cos(float(degrees * M_PI / 180))});
    loudest = std::max({loudest, peak_gain(placed.left), peak_gain(placed.right)});
  }
  check(20 * std::log10(loudest) < -3.5 && 20 * std::log10(loudest) > -4.5, "The kernels' loudest gain left -3.8 dB");
  const SourceSoundHrtfTu15 unity(*data->sound_sdk_spatial_data, SourceSoundHrtfScaleTu15::UnityPower);
  check(std::abs(power(unity.filter({0, 0, 10}).left) - 1) < 1e-5, "Straight-ahead kernel is not unity power");

  const auto right = hrtf.filter({10, 0, 0}), left = hrtf.filter({-10, 0, 0});
  check(near(hrtf.azimuth({10, 0, 0}), 90) && near(hrtf.azimuth({-10, 0, 0}), -90), "Side azimuths differ from the SDK's");
  check(right.left_delay == 30 && !right.right_delay && left.right_delay == 30 && !left.left_delay,
        "The far ear lost its original interaural delay");
  check(right.left == left.right && right.right == left.left, "Left and right placements are not mirrored");
  check(treble_share(right.left) < treble_share(right.right) && power(right.right) > power(right.left),
        "The far ear is not head-shadowed");

  // Height does not move a voice (380168 projects onto the horizontal plane).
  check(hrtf.filter({10, 5, 0}) == right, "Height changed a voice's HRTF");
  // Stereo keeps the rear kernels: behind differs from ahead.
  const auto behind = hrtf.filter({0, 0, -10});
  check(behind.left == behind.right && behind.left != ahead.left && !behind.left_delay,
        "Behind did not select the rear kernel");
  // 30 degrees right (by the SDK's linear arctangent) follows the
  // spherical-head delay.
  const auto quarter = hrtf.filter({5, 0, 8.660254f});
  check(quarter.left_delay >= 9 && quarter.left_delay <= 13 && !quarter.right_delay,
        "A 30-degree voice lost its interaural delay");
  // Stereo leaves the rear off (37DFE1's disabled call: 0 and -10000).
  check(ahead.front == 0 && ahead.rear == -10000 && behind.front == 0 && behind.rear == -10000,
        "Stereo opened the back pair");
  // With rear speakers a rear azimuth folds to its mirrored front kernel and
  // DirectSound splits the voice: ahead plays in front, behind at the back,
  // and a source at the listener spreads toward both.
  const auto surround_ahead = hrtf.filter({0, 0, 10}, true), surround_behind = hrtf.filter({0, 0, -10}, true);
  check(surround_ahead.left == ahead.left && surround_behind.left == ahead.left,
        "Surround did not fold the rear kernels");
  check(surround_ahead.front == 0 && surround_ahead.rear <= -6400 && surround_behind.rear == 0 &&
        surround_behind.front <= -6400, "Surround did not split front and rear");
  const auto near = hrtf.filter({0, 0, -0.01f}, true);
  check(near.front > -1000 && near.rear > -1000, "A source at the listener did not spread to both pairs");
}
// 12A9D0: the listener's and source's speeds along the line between them.
void doppler() {
  const double c = 111.548553;
  const auto toward = source_sound_doppler_cents_tu15({0, 0, 0}, {10, 0, 0}, {5, 0, 0}, {0, 0, 0});
  const auto away = source_sound_doppler_cents_tu15({0, 0, 0}, {0, 0, 0}, {5, 0, 0}, {10, 0, 0});
  check(near(toward, float(std::log((c + 10) / c) * 1731.234), 0.01f) && toward > 0,
        "An approaching listener did not raise the pitch");
  check(near(away, float(std::log(c / (c + 10)) * 1731.234), 0.01f) && away < 0,
        "A receding source did not lower the pitch");
  check(source_sound_doppler_cents_tu15({0, 0, 0}, {0, 10, 0}, {5, 0, 0}, {0, 0, 0}) == 0.f,
        "Motion across the line of sight shifted the pitch");
  check(near(source_sound_doppler_cents_tu15({0, 0, 0}, {0, 0, 0}, {5, 0, 0}, {10000, 0, 0}),
             float(std::log(0.125) * 1731.234), 0.01f),
        "The Doppler ratio escaped its original 1/8 bound");
  check(source_sound_doppler_cents_tu15({1, 1, 1}, {10, 0, 0}, {1, 1, 1}, {0, 0, 0}) == 0.f,
        "A source at the listener shifted the pitch");
}
// 21ECE0, 21EE80 and 227BF0 on Cairo's authored environment record.
void reverb_conversions() {
  SourceSoundReverbTu15 cairo{-12, -3, 0, 1.49f, 1.1f, -78, 0.007f, 4.41f, 0.011f, 1, 1, 5000};
  const auto p = source_sound_reverb_i3dl2_tu15(cairo);
  check(p.room == -1200 && p.room_hf == -300 && p.reflections == -6400 && std::abs(p.reverb - 441) <= 1 &&
            p.diffusion == 100.f && p.density == 100.f && p.hf_reference == 5000.f,
        "Environment reverb converted differently from 21ECE0");
  SourceSoundListenerEnvironmentTu15 all{cairo, {0.f, 6.2831855f, 1.f, 0.f}, 1.f};
  for (const auto gain : source_sound_reverb_speaker_gains_tu15(all)) check(near(gain, 1.f), "A full span left a speaker dry");
  all.fade = 0.5f;
  for (const auto gain : source_sound_reverb_speaker_gains_tu15(all)) check(near(gain, 0.5f), "The fade did not scale the reverb");
  // A doorway spanning ahead to the left (0..90 degrees counterclockwise).
  const SourceSoundListenerEnvironmentTu15 door{cairo, {0.f, 1.5707964f, 0.f, 1.f}, 1.f};
  const auto gains = source_sound_reverb_speaker_gains_tu15(door);
  check(near(gains[0], 0.5f) && near(gains[1], 0.25f) && near(gains[2], 0.25f) && gains[3] == 0.f,
        "A portal span reached the wrong speakers");
  check(source_sound_room_send_tu15(0) == 0 && source_sound_room_send_tu15(-6) == -600 &&
            source_sound_room_send_tu15(-80) == -6400,
        "Room send differs from 227BF0");
}

// 3838A4 for DirectSound's Generic listener reproduces the TU1.5 DSPImage's
// installed parameter words of record 8 (built with that preset), except
// where the image's tool differed: it designed the room HF filter at
// 24 kHz where this DirectSound uses 48 kHz, rounded the short reverb delay
// one 4-word step lower (moving the last reflection tap with it), and
// rounded the cascade damping gain one unit higher.
void reverb_words() {
  const auto words = sound_i3dl2_reverb_words_tu15(SoundI3dl2Parameters{});
  const auto image = sound_i3dl2_reverb_installed_tu15(8);
  std::array<std::uint32_t, 66> installed{};
  for (unsigned n = 0; n < 19; ++n) installed[n] = image.delays[n];
  for (unsigned n = 0; n < 8; ++n) installed[19 + n] = image.reflection_gains[n];
  installed[27] = image.cross_gain;
  installed[28] = image.mono_gain;
  for (unsigned n = 0; n < 6; ++n) installed[29 + n] = image.target_gains[n];
  installed[35] = image.output_mode;
  for (unsigned n = 0; n < 10; ++n) {
    installed[36 + 3 * n] = image.filters[n].previous;
    installed[37 + 3 * n] = image.filters[n].feedback;
    installed[38 + 3 * n] = image.filters[n].feedforward;
  }
  const std::array<std::pair<unsigned, std::uint32_t>, 7> tool{{{4, 0x3a0}, {5, 0x6c0}, {37, 0x26e06d}, {38, 0x591f92},
                                                               {41, 0x1fe6dd}, {44, 0x1fe6dd}, {47, 0x1fe6dd}}};
  for (unsigned n = 0; n < 66; ++n) {
    const auto differs = std::find_if(tool.begin(), tool.end(), [&](const auto &entry) { return entry.first == n; });
    if (differs == tool.end()) check(words[n] == installed[n], "I3DL2 words differ from the DSP image's Generic preset");
    else check(words[n] == differs->second, "I3DL2 words changed where the image's tool differed");
  }
  // The first gain glides from the installed words to the committed ones.
  auto state = sound_i3dl2_reverb_installed_tu15(9);
  SoundI3dl2Parameters cairo;
  cairo.room = -1200;
  sound_i3dl2_reverb_commit_tu15(state, sound_i3dl2_reverb_words_tu15(cairo));
  check((state.flags & 4u) && state.target_gains[4] != image.target_gains[4], "A commit did not request the record's update");
}

// Records 8/9 with DirectSound's words: the long reverb decays at its decay
// time and the room level scales the whole effect.
void reverb_decay() {
  const auto tail = [](std::int32_t room, unsigned skip, unsigned frames) {
    auto state = sound_i3dl2_reverb_installed_tu15(8);
    tu15_sound_ring_effect::Workspace workspace{};
    SoundI3dl2Parameters p;
    p.room = room;
    p.room_hf = 0;
    p.reflections = -10000;
    p.reverb = 0;
    p.decay_time = 1.f;
    p.decay_hf_ratio = 1.f;
    const auto block = [&](std::uint32_t first) {
      std::array<std::uint32_t, 32> input{};
      input[0] = first;
      std::array<std::array<std::uint32_t, 32>, 4> out{};
      tu15_sound_ring_effect::process_admitted(state, workspace, input, {out[0], out[1], out[2], out[3]});
      double sum = 0;
      for (const auto &channel : out)
        for (const auto word : channel) {
          const double value = double(std::int32_t(word << 8) >> 8) / 8388608.0;
          sum += value * value;
        }
      return sum;
    };
    (void)block(0);
    sound_i3dl2_reverb_commit_tu15(state, sound_i3dl2_reverb_words_tu15(p));
    // Let the gain glide and the tap crossfades settle (about 0.22 s).
    for (unsigned n = 0; n < 400; ++n) (void)block(0);
    (void)block(0x400000);
    for (unsigned n = 0; n < skip; ++n) (void)block(0);
    double sum = 0;
    for (unsigned n = 0; n < frames; ++n) sum += block(0);
    return sum;
  };
  // 150 blocks is 0.1 s.
  const double early = tail(0, 150, 150), late = tail(0, 750, 150);
  const double drop = 10 * std::log10(early / late);
  // 0.4 s apart at a 1 s decay time: about 24 dB.
  check(early > 0 && drop > 20 && drop < 28, "Original reverb decay differs from its decay time");
  const double quieter = 10 * std::log10(tail(-1000, 150, 150) / early);
  check(quieter < -9.5 && quieter > -10.5, "The room level did not scale the reverb by 10 dB per 1000 mB");
}

// 38368B/3834F3: obstruction (LF ratio 0) and occlusion (0.25) split into
// levels and one-pole poles whose low-pass loses the HF part at 5000 Hz.
void i3dl2_voice() {
  SoundI3dl2SourceTu15 source;
  source.room = -600;
  source.room_hf = -600;
  source.obstruction = -3000;
  source.occlusion = -2000;
  const auto voice = sound_i3dl2_voice_tu15(source);
  check(voice.direct == -500 && voice.room == -1100, "I3DL2 levels differ from 38368B");
  const auto loss = [](std::uint16_t pole) {
    const double b = pole / 65536.0, w = 2 * 3.14159265358979 * 5000 / 48000;
    const double re = 1 - b * std::cos(w), im = b * std::sin(w);
    return 1000 * std::log10((1 - b) * (1 - b) / (re * re + im * im));
  };
  // Direct HF: 0.75 of the occlusion plus all of the obstruction.
  check(std::abs(loss(voice.direct_pole) - (-4500)) < 10, "The direct filter misses its HF loss");
  // Room HF: DirectSound reads lRoom, not lRoomHF, with occlusion's HF part.
  check(std::abs(loss(voice.room_pole) - (-2100)) < 10, "The room filter misses its HF loss");
  SoundI3dl2SourceTu15 open;
  const auto clear = sound_i3dl2_voice_tu15(open);
  check(clear.direct == 0 && clear.room == 0 && clear.direct_pole == 0 && clear.room_pole == 0,
        "An unobstructed voice was filtered");
}
// 221A70/221E30/222150/249D60 on a small resident structure: three
// clusters, one door between {0,1} and {2} at half closed, an environment
// portal in cluster 2 and authored pair distances.
void obstruction() {
  constexpr std::uint32_t heap = 0x10000000u;
  std::vector<std::byte> globals(0x20000), memory(0x4000);
  const auto put = [&](std::uint32_t address, auto value) {
    auto &bank = address >= heap ? memory : globals;
    const auto offset = address >= heap ? address - heap : address - 0x4e0000u;
    std::memcpy(bank.data() + offset, &value, sizeof(value));
  };
  const std::uint32_t S = heap, C = heap + 0x400, planes = heap + 0x500, environments = heap + 0x600,
                      clusters = heap + 0x800, sounds = heap + 0xc00, A = heap + 0xd00, G = heap + 0x2000;
  put(0x4ecc18u, S);
  put(0x4f9b58u, G);
  put(S + 0x18, C);
  put(C + 0xc, planes);
  put(planes, 1.f);
  put(planes + 4, 0.f);
  put(planes + 8, 0.f);
  put(S + 0x60, environments);
  put(environments + 4, std::int32_t(0));
  put(environments + 8, 10.f);
  put(environments + 0x14, 2.f);
  put(S + 0x9c, std::int32_t(3));
  put(S + 0xa0, clusters);
  for (std::uint32_t c = 0; c < 3; ++c) put(clusters + c * 0xb0 + 0x7a, std::int16_t(c == 2));
  put(clusters + 2 * 0xb0 + 0x8c, std::int32_t(1));
  put(clusters + 2 * 0xb0 + 0x90, sounds);
  put(sounds, std::int16_t(0));
  put(S + 0x224, std::int32_t(1));
  put(S + 0x228, A);
  put(A, std::int32_t(1));
  put(A + 4, 0.f);
  put(A + 8, 100.f);
  put(A + 0x10, heap + 0xe00);
  put(heap + 0xe00, std::uint32_t(0b011));
  put(heap + 0xe04, std::uint32_t(0b100));
  put(A + 0x18, heap + 0xe80);
  for (std::uint32_t c = 0; c < 3; ++c) put(heap + 0xe80 + c * 4, std::uint32_t(1));
  put(A + 0x28, heap + 0xf00);
  put(heap + 0xf01, std::uint8_t(128));
  put(A + 0x30, heap + 0xf80);
  put(heap + 0xf80, std::int8_t(5));
  put(G + 0x28 + 5 * 2, std::uint16_t(0x8000));
  float ahead = 0.4f, behind = 0.5f;
  const SourceSoundObstructionWorldTu15 world{
      [&](std::uint32_t address, std::size_t size) -> std::span<const std::byte> {
        if (address >= heap) return std::span(memory).subspan(address - heap, size);
        return std::span(globals).subspan(address - 0x4e0000u, size);
      },
      [&](Vec3 from, Vec3) -> std::optional<float> { return from.x < 1.f ? ahead : behind; }};
  const auto doors = source_sound_door_values_tu15(world, 0);
  check(doors.size() == 3 && doors[1] == 0.f && near(doors[2], 32768.f / 65535.f), "Door values differ from 221A70");
  const SourceSoundObstructionPointTu15 listener{{0, 0, 0}, 0, 10}, source{{20, 0, 0}, 2, 20};
  const auto r = source_sound_obstruction_tu15(world, doors, listener, source, false);
  check(near(r.obstruction, 0.1f) && near(r.gain, 1.f - 1.f / 11.f) && near(r.occlusion, doors[2]),
        "Obstruction differs from 221E30/222150");
  const auto same = source_sound_obstruction_tu15(world, doors, listener, {{20, 0, 0}, 2, 10}, false);
  check(same.obstruction == 0.f && same.gain == 0.f && same.occlusion == 0.f, "A source in the listener's leaf was obstructed");
  const float travel = 127.f / 253.f * 100.f;
  check(near(source_sound_cluster_distance_tu15(world, 2, 0), travel), "Cluster distance differs from 249D60");
  const auto estimate = source_sound_obstruction_tu15(world, doors, listener, source, true);
  check(near(estimate.obstruction, 1.f - 20.f / (20.f + travel)), "Portal estimate differs from 221E30");
  // 129100's request for a centered 0x1000 sound: its cluster, at the
  // origin with no leaf. Its occlusion is that cluster's door value.
  const auto centered = source_sound_obstruction_tu15(world, doors, listener, {{0, 0, 0}, 2, -1}, true);
  check(near(centered.occlusion, doors[2]), "A centered cluster sound lost its door occlusion");
}
} // namespace

int main() {
  try {
    coefficients();
    head_frame();
    original_kernels();
    doppler();
    reverb_conversions();
    reverb_words();
    reverb_decay();
    i3dl2_voice();
    obstruction();
    std::cout << "hrtfCoefficients=signExponentMantissa/2048 headFrame=rightUpForward originalKernels=61 "
                 "interauralDelay=original rearKernels=stereo doppler=12A9D0 reverb=21ECE0/21EE80/227BF0/3838A4/records8-9 i3dl2Voice=38368B obstruction=221A70/221E30\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
