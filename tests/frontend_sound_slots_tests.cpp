#include "halo2/audio.hpp"
#include "halo2/frontend.hpp"
#include "halo2/original_matchmaking_audio.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void check(bool value, const char *reason) {
  if (!value) throw FormatError(reason);
}
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  check(bool(stream), "cannot open sound authority input");
  const auto size = stream.tellg();
  check(size > 0 && size <= std::streamoff(512 * 1024 * 1024), "sound authority size");
  std::vector<std::byte> result(static_cast<std::size_t>(size));
  stream.seekg(0);
  stream.read(reinterpret_cast<char *>(result.data()), std::streamsize(result.size()));
  check(bool(stream), "truncated sound authority input");
  return result;
}
struct SoundSlots {
  std::array<std::uint32_t, 13> datums;
  std::array<std::string, 13> names;
  std::array<EncodedSound, 13> vista_encoded;
};
// Independent literal cache header/tag-table reader. It does not use
// Cache::load_frontend, FrontendUiSound, or production UI field offsets.
SoundSlots raw_slots(const std::filesystem::path &path, bool vista) {
  const auto storage = read(path);
  const Bytes b(storage);
  check(b.u32(0) == 0x68656164 && b.u32(4) == 8 && b.u32(2044) == 0x666f6f74,
        "sound fixture is not a version8 map");
  check(b.u32(8) == b.size(), "sound fixture map extent");
  check(b.string(vista ? 0x12c : 0x120, 32) ==
            (vista ? "11081.07.04.30.0934.main" : "02.09.27.09809"),
        "sound fixture build family");
  const auto index = b.u32(16);
  auto pointer = [&](std::uint32_t address, std::int64_t magic, std::size_t size) {
    const auto offset = std::int64_t(address) - magic;
    check(offset >= 0, "sound fixture negative pointer");
    b.require(std::size_t(offset), size);
    return std::size_t(offset);
  };
  const auto header_magic = vista ? -std::int64_t(index) :
      std::int64_t(b.u32(index)) - index - 32;
  const auto table = pointer(b.u32(index + 8), header_magic, 16);
  const auto magic = vista ? -std::int64_t(index) :
      std::int64_t(b.u32(table + 8)) - index - b.u32(20);
  const auto count = b.u32(index + 24);
  check(count > 0 && count <= 65536, "sound fixture tag count");
  b.require(table, std::size_t(count) * 16);
  const auto names = b.u32(vista ? 0x2d0 : 0x2c4);
  const auto names_size = b.u32(vista ? 0x2d4 : 0x2c8);
  const auto name_offsets = b.u32(vista ? 0x2d8 : 0x2cc);
  b.require(names, names_size);
  b.require(name_offsets, std::size_t(count) * 4);
  std::map<std::uint32_t, std::string> sound_names;
  std::map<std::uint32_t, std::size_t> sound_offsets;
  std::size_t globals = 0, gestalt = 0;
  for (unsigned i = 0; i < count; ++i) {
    const auto t = table + i * 16;
    if (b.u32(t) == 0xffffffffu) continue;
    const auto name = b.u32(name_offsets + i * 4);
    check(name < names_size, "sound fixture tag name bounds");
    const auto text = b.string(names + name, names_size - name);
    if (b.u32(t) == 0x736e6421) {
      sound_names.emplace(b.u32(t + 4), text);
      if (vista) sound_offsets.emplace(b.u32(t + 4), pointer(b.u32(t + 8), magic, 20));
    }
    if (vista && b.u32(t) == 0x75676821) {
      check(gestalt == 0, "duplicate Vista sound gestalt");
      gestalt = pointer(b.u32(t + 8), magic, 72);
    }
    if (b.u32(t) == 0x7769676c && text == "ui\\ui_shared_globals") {
      check(globals == 0 && b.u32(t + 12) >= 0xf8, "sound fixture wigl identity");
      globals = pointer(b.u32(t + 8), magic, 0xf8);
    }
  }
  check(globals != 0, "sound fixture has no shared UI globals");
  SoundSlots result;
  for (unsigned i = 0; i < 13; ++i) {
    const auto id = b.u32(globals + 0x94 + 8 * i);
    result.datums[i] = id;
    if (id != 0xffffffffu) {
      const auto found = sound_names.find(id);
      check(found != sound_names.end(), "UI sound slot does not resolve to snd!");
      result.names[i] = found->second;
      if (vista) {
        check(gestalt != 0, "Vista UI sound gestalt missing");
        const auto s = sound_offsets.at(id);
        auto block = [&](std::size_t header, unsigned stride) {
          const auto count = b.u32(header);
          check(count > 0 && count <= 1000000, "Vista UI sound block extent");
          return std::pair{pointer(b.u32(header + 4), magic, std::size_t(count) * stride), count};
        };
        const auto [ranges, range_count] = block(gestalt + 32, 12);
        check(b.slice(s + 10, 1)[0] == std::byte{1} && b.u16(s + 8) < range_count,
              "Vista UI sound pitch family differs");
        const auto r = ranges + b.u16(s + 8) * 12;
        const auto [perms, perm_count] = block(gestalt + 40, 16);
        check(b.u16(r + 10) == 1 && b.u16(r + 8) < perm_count,
              "Vista UI sound permutation family differs");
        const auto p = perms + b.u16(r + 8) * 16;
        const auto [chunks, chunk_count] = block(gestalt + 64, 12);
        const auto first = b.u16(p + 12), count = b.u16(p + 14);
        check(count > 0 && std::size_t(first) + count <= chunk_count,
              "Vista UI sound chunk range");
        auto &encoded = result.vista_encoded[i];
        encoded.name = found->second;
        encoded.sound_class = std::to_integer<unsigned>(b.slice(s + 2, 1)[0]);
        const auto rate = std::to_integer<unsigned>(b.slice(s + 3, 1)[0]);
        const auto encoding = std::to_integer<unsigned>(b.slice(s + 4, 1)[0]);
        check(rate <= 2 && encoding <= 1, "Vista UI ADPCM family differs");
        constexpr unsigned rates[]{22050, 44100, 32000};
        encoded.sample_rate = rates[rate];
        encoded.channels = encoding + 1;
        encoded.codec = std::to_integer<unsigned>(b.slice(s + 5, 1)[0]);
        encoded.samples = b.u32(p + 8);
        for (unsigned n = 0; n < count; ++n) {
          const auto c = chunks + (first + n) * 12;
          check((b.u32(c) >> 30) == 0, "Vista UI sound has external resource owner");
          const auto raw = b.slice(b.u32(c) & 0x3fffffffu, b.u32(c + 4) & 0x3fffffffu);
          encoded.data.insert(encoded.data.end(), raw.begin(), raw.end());
        }
      }
    }
  }
  return result;
}
void original_pc_instructions(const std::filesystem::path &path) {
  const auto storage = read(path);
  const Bytes b(storage);
  check(b.u16(0) == 0x5a4d, "Vista sound fixture is not PE");
  const auto pe = b.u32(0x3c);
  check(b.u32(pe) == 0x4550 && b.u16(pe + 24) == 0x10b &&
        b.u32(pe + 24 + 28) == 0x400000, "Vista sound fixture PE family");
  const auto sections = pe + 24 + b.u16(pe + 20);
  auto file_offset = [&](std::uint32_t va, std::size_t count) {
    const auto rva = va - 0x400000u;
    for (unsigned i = 0; i < b.u16(pe + 6); ++i) {
      const auto s = sections + i * 40;
      const auto base = b.u32(s + 12), size = b.u32(s + 16);
      if (rva >= base && std::uint64_t(rva - base) + count <= size) {
        const auto result = std::size_t(b.u32(s + 20)) + rva - base;
        b.require(result, count);
        return result;
      }
    }
    throw FormatError("Vista sound instruction address outside raw sections");
  };
  auto exact = [&](std::uint32_t va, std::initializer_list<unsigned> expected) {
    const auto bytes = b.slice(file_offset(va, expected.size()), expected.size());
    unsigned i = 0;
    for (const auto value : expected)
      check(bytes[i++] == std::byte(value), "Vista literal UI sound instructions differ");
  };
  // These are actual61DD04 instruction operands, not a native-generated fixture.
  exact(0x61dd18, {0x83, 0xf9, 0x0d}); // CMP slot,13
  exact(0x61dd21, {0x8b, 0x84, 0xc8, 0x94, 0, 0, 0}); // [globals+slot*8+94]
  exact(0x61dd28, {0x83, 0xf8, 0xff}); // NONE skips playback
  exact(0x61ddea, {0xd9, 0xe8, 0x83, 0xc4, 0x04, 0xd9, 0x1c, 0x24}); // language FLD1
  exact(0x61de1a, {0xd9, 0xe8, 0x51, 0xd9, 0x1c, 0x24, 0x50}); // ordinary FLD1
  const auto call = file_offset(0x61de21, 5);
  check(b.slice(call, 1)[0] == std::byte{0xe8} &&
        std::uint32_t(0x61de26u + b.u32(call + 1)) == 0x48836c,
        "Vista ordinary sound request target differs");
  exact(0x48836f, {0xf3, 0x0f, 0x10, 0x44, 0x24, 0x60}); // consumes second float argument
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 4) throw FormatError("sound slots need Xbox maps, Vista mainmenu.map and Vista halo2.exe");
    const auto xbox = std::filesystem::path(argv[1]) / "mainmenu.map";
    if (!std::filesystem::exists(xbox) || !std::filesystem::exists(argv[2]) ||
        !std::filesystem::exists(argv[3])) return 77;
    const auto original = raw_slots(xbox, false), vista = raw_slots(argv[2], true);
    original_pc_instructions(argv[3]);
    constexpr std::array<std::uint32_t, 13> xbox_datums{
      0xe4ce02ae,0xe4cf02af,0xe48b026b,0xe4d002b0,0xe4d102b1,0xe4d202b2,
      0xe4ce02ae,0xe4d302b3,0xe4d402b4,0xe4cf02af,0xe4d502b5,0xffffffff,0xe4d602b6};
    constexpr std::array<std::uint32_t, 13> vista_datums{
      0xe4d802b6,0xe4d902b7,0xe4950273,0xe4da02b8,0xe4db02b9,0xe4dc02ba,
      0xe4d802b6,0xe4dd02bb,0xe4de02bc,0xe4d902b7,0xe4df02bd,0xffffffff,0xe4e002be};
    check(original.datums == xbox_datums && vista.datums == vista_datums,
          "original sound-slot authority changed");
    constexpr std::array<const char *, 13> expected_names{
      "sound\\ui\\cursor1","sound\\ui\\forward1","sound\\ui\\flag_fail","sound\\ui\\advance",
      "sound\\ui\\back1","sound\\ui\\transport","sound\\ui\\cursor1","sound\\ui\\virtual_keyboard_click",
      "sound\\ui\\receive_message","sound\\ui\\forward1","sound\\game_sfx\\multiplayer\\countdown_for_respawn",
      "","sound\\game_sfx\\multiplayer\\pickup_health"};
    for (unsigned i = 0; i < 13; ++i)
      check(original.names[i] == expected_names[i] && vista.names[i] == expected_names[i],
            "original Xbox/Vista sound slot identity differs");
    check(frontend_ui_sound_slot_count == 13 &&
          unsigned(FrontendUiSound::Cursor) == 0 && unsigned(FrontendUiSound::Forward) == 1 &&
          unsigned(FrontendUiSound::Error) == 2 && unsigned(FrontendUiSound::Back) == 4 &&
          unsigned(FrontendUiSound::PregameCountdown) == 10 &&
          std::bit_cast<std::uint32_t>(frontend_ui_sound_request_scale) == 0x3f800000u,
          "native sound request differs from literal source slots/scale");
    Cache cache(xbox);
    const auto content = cache.load_frontend();
    check(content.sounds == original.datums, "native UI sound slots shifted or truncated");
    std::set<std::uint32_t> decoded;
    for (const auto id : content.sounds) if (id != 0xffffffffu && decoded.insert(id).second) {
      const auto encoded = cache.load_sound(id);
      const auto slot = std::size_t(std::find(content.sounds.begin(), content.sounds.end(), id) - content.sounds.begin());
      const auto &pc = vista.vista_encoded[slot];
      check(pc.name == encoded.name && pc.sample_rate == encoded.sample_rate &&
            pc.channels == encoded.channels && pc.codec == encoded.codec &&
            pc.samples == encoded.samples && pc.sound_class == encoded.sound_class &&
            pc.data == encoded.data,
            "native Xbox UI payload/metadata differs from actual Vista asset");
      const auto pcm = decode_sound(encoded);
      check(pcm.frames() > 0 && pcm.sound_class == encoded.sound_class,
            "required Xbox UI sound is not preloadable with its source class");
      if(original_matchmaking_audio_slot(unsigned(slot))) {
        const auto private_sound=load_original_matchmaking_audio(unsigned(slot));
        check(private_sound.name==encoded.name && private_sound.sample_rate==encoded.sample_rate &&
              private_sound.channels==encoded.channels && private_sound.codec==encoded.codec &&
              private_sound.samples==encoded.samples && private_sound.sound_class==encoded.sound_class &&
              private_sound.data==encoded.data && private_sound.packet_chunks.empty(),
              "Private countdown sound differs from the actual original DVD encoded owner");
        const auto private_pcm=decode_sound(private_sound);
        check(private_pcm.stereo==pcm.stereo && private_pcm.rate==pcm.rate && private_pcm.sound_class==pcm.sound_class,
              "Private countdown PCM differs from decoding the actual original DVD asset");
        SDL_setenv("SDL_AUDIODRIVER","dummy",1);
        AudioEngine mixer(false);
        const auto voice=mixer.play(std::make_shared<PcmSound>(private_pcm),frontend_ui_sound_request_scale);
        check(voice && mixer.active_voices()==1,"Original countdown did not create an actual native voice");
        const auto rendered=mixer.render(pcm.frames()+512);
        check(std::equal(pcm.stereo.begin(),pcm.stereo.end(),rendered.begin()) &&
              std::all_of(rendered.begin()+pcm.stereo.size(),rendered.end(),[](float v){return v==0;}) &&
              mixer.active_voices()==0,"Original countdown output or natural voice completion differs");
      }
    }
    check(decoded.size() == 10, "source sound alias/NONE handling differs");
    for(unsigned slot=0;slot<13;++slot)
      check(original_matchmaking_audio_slot(slot)==(slot==10 || slot==12),"Private source audio leaked into another UI role");
    std::cout << "13 original Xbox/Vista slots; 10 unique native UI sounds have byte-identical Vista payloads and metadata, including countdown; literal PC request scale; no mixer/playback or trigger-timing claim\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
