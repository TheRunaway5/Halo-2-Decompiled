#include "halo2/menu_music.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
constexpr float scale = std::bit_cast<float>(0x37800080u); // original 45DC50
std::uint16_t advance(std::uint32_t &seed, std::uint32_t instruction,
                      std::vector<SoundRngStore> *stores) {
  const auto before = seed;
  seed = seed * 0x19660du + 0x3c6ef35fu;
  if (stores) stores->push_back({instruction, before, seed});
  return std::uint16_t(seed >> 16);
}
}
std::int16_t source_looping_sound_permutation(
    std::span<const std::uint16_t> chances, LoopingSoundSelection &state,
    std::uint32_t &seed, bool mirror, bool *all_used,
    std::vector<SoundRngStore> *stores) {
  if (chances.empty() || chances.size() > 31)
    throw FormatError("Unrecovered looping sound permutation count");
  const auto count = unsigned(chances.size()), mask = (1u << count) - 1;
  int previous = state.previous;
  unsigned tries = 0, selected;
  if (all_used) *all_used = false;
  for (;;) {
    selected = (unsigned(advance(seed, 0x219171, stores)) * count) >> 16;
    if (!(~state.used & mask)) {
      state.used = 0;
      if (count > 1 && previous != -1)
        state.used = 1u << (unsigned(previous) & 31);
      previous = -1;
    }
    const auto bit = 1u << selected;
    if (state.used & bit) continue;
    state.used |= bit;
    if (all_used && !(~state.used & mask)) *all_used = true;
    float chance = float(chances[selected]) * scale;
    if (mirror && chance >= .5f) chance = 1.f - chance;
    // Original multiplies the accepted draw on x87 after storing chance as
    // single precision. Keep that wider product rather than rounding to float.
    const double draw = double(advance(seed, 0x219239, stores)) * double(scale);
    if (draw >= double(chance) || ++tries == count * 2) break;
  }
  state.previous = std::int16_t(selected);
  return std::int16_t(selected);
}
std::uint32_t source_secondary_random_seed(std::uint32_t seconds,
                                          std::uint32_t ticks,
                                          std::uint16_t crt_draw) {
  return seconds ^ ticks ^ crt_draw;
}
std::uint16_t source_crt_random(std::uint32_t &state) {
  state = state * 0x343fdu + 0x269ec3u;
  return std::uint16_t((state >> 16) & 0x7fff);
}

struct MenuMusic::State {
  struct Bank {
    std::vector<std::shared_ptr<const PcmSound>> pcm;
    std::vector<std::uint16_t> chances;
    LoopingSoundSelection selection;
    std::shared_ptr<const PcmSound> choose(std::uint32_t &seed) {
      if (pcm.empty()) return {};
      return pcm[unsigned(source_looping_sound_permutation(chances, selection, seed))];
    }
  };
  struct Track { Bank start, loop, end; float gain{}, fade_out{}; std::uint16_t fade_curve{}; };
  std::uint32_t seed;
  std::vector<Track> tracks;
  explicit State(std::uint32_t supplied) : seed(supplied) {}
};
MenuMusic::MenuMusic(const Cache &cache, std::uint32_t datum, std::uint32_t seed)
    : state_(std::make_shared<State>(seed)) {
  const auto definition = cache.load_looping_sound(datum);
  // The retail title uses one normal track, an intro, four loop permutations,
  // no outro/alternates, and no controller. Other looping modes need their
  // own scheduler before this frontend path can claim to support them.
  if (definition.flags != 0 || definition.tracks.size() != 1)
    throw FormatError("Unrecovered menu music looping mode");
  auto load = [&](std::uint32_t sound) {
    State::Bank bank;
    if (sound == null_datum) return bank;
    const auto metadata = cache.load_sound_variation(sound);
    if (std::to_integer<unsigned>(metadata.sound[10]) != 1 ||
        (std::to_integer<unsigned>(metadata.sound_class[10]) & 0x20))
      throw FormatError("Unrecovered menu music range persistence");
    const auto imported = imported_sound_range_state(metadata);
    bank.selection = {imported.used, imported.last};
    for (unsigned permutation = 0; permutation < metadata.permutations.size(); ++permutation) {
      bank.chances.push_back(Bytes(metadata.permutations[permutation]).u16(2));
      auto pcm = std::make_shared<PcmSound>(decode_sound(cache.load_sound(sound, 0, permutation)));
      if (pcm->rate != 48000 || !pcm->frames())
        throw FormatError("Invalid decoded menu music permutation");
      bank.pcm.push_back(std::move(pcm));
    }
    return bank;
  };
  for (const auto &track : definition.tracks) {
    if (track.flags != 2 || track.loop == null_datum || track.end != null_datum ||
        track.alternate != null_datum || track.alternate_end != null_datum)
      throw FormatError("Unrecovered menu music track transitions");
    state_->tracks.push_back({load(track.start), load(track.loop), load(track.end),
                              std::pow(10.f, track.gain_db / 20.f), track.fade_out,
                              std::uint16_t(!(Bytes(cache.load_sound_variation(track.loop).sound_class).u16(10)&0x400))});
  }
}
std::vector<AudioEngine::VoiceId> MenuMusic::play(AudioEngine &audio) {
  std::vector<AudioEngine::VoiceId> voices;
  for (unsigned i = 0; i < state_->tracks.size(); ++i) {
    auto &track = state_->tracks[i];
    // 21B940 draws its fixed pitch before the initial sound selection. A
    // start-to-loop transition (21C190) and subsequent final chunks (21C400)
    // select through 219110 without drawing another creation pitch.
    advance(state_->seed, 0, nullptr);
    auto start = track.start.choose(state_->seed);
    voices.push_back(audio.play_sequence(start, nullptr, nullptr, track.gain, 0,
        [state = state_, i] { return state->tracks[i].loop.choose(state->seed); }));
  }
  return voices;
}
void MenuMusic::stop(AudioEngine &audio, std::span<const AudioEngine::VoiceId> voices) {
  if(voices.size()!=state_->tracks.size())throw FormatError("Menu music voice/track mismatch");
  for(unsigned i=0;i<voices.size();++i)
    audio.fade_out(voices[i],state_->tracks[i].fade_out,state_->tracks[i].fade_curve);
}
const std::vector<std::shared_ptr<const PcmSound>> &MenuMusic::loops(unsigned track) const {
  return state_->tracks.at(track).loop.pcm;
}
std::uint32_t MenuMusic::seed() const { return state_->seed; }
} // namespace halo2
