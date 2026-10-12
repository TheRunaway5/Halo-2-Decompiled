#include "halo2/original_data.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include <algorithm>
#include <bit>
#include <fstream>

namespace halo2 {
std::shared_ptr<const OriginalData> Cache::original_data() const {
  if (!original_data_)
    original_data_ = OriginalData::load_xbe((vista_ && !xbox_donor_root_.empty()?xbox_donor_root_:shared_directory_.parent_path()) / "default.xbe");
  return original_data_;
}
namespace {
// Original source addresses, not replacement tuning values. Import order is
// the first encounter of each unique constant in 164750 / 165B30.
constexpr std::array<std::uint32_t,19> balance_addresses_11{
    0x4652d4, 0x4652d0, 0x451190, 0x4652cc, 0x4652c8,
    0x46502c, 0x4652c4, 0x465104, 0x446548, 0x46509c,
    0x4652c0, 0x4652bc, 0x4652b8, 0x4650f4, 0x4652b4,
    0x46518c, 0x4652b0, 0x46516c, 0x451194};
constexpr std::array<std::uint32_t,19> balance_addresses_15{
    0x46a374, 0x46a370, 0x455a90, 0x46a36c, 0x46a368,
    0x46a0cc, 0x46a364, 0x46a1a4, 0x449fd4, 0x46a13c,
    0x46a360, 0x46a35c, 0x46a358, 0x46a194, 0x46a354,
    0x46a22c, 0x46a350, 0x46a20c, 0x455a94};
// Semantic order: two XYZ targets, squared-distance threshold, X delta.
// Both target Z words intentionally read the same original source operand.
constexpr std::array<std::uint32_t,8> netgame_addresses_11{
    0x465518, 0x465514, 0x446df8, 0x465510, 0x46550c, 0x446df8,
    0x46504c, 0x46509c};
constexpr std::array<std::uint32_t,8> netgame_addresses_15{
    0x46a5c8, 0x46a5cc, 0x44a8a0, 0x46a5c0, 0x46a5c4, 0x44a8a0,
    0x46a0ec, 0x46a13c};
constexpr std::array<std::uint32_t,7> relic_addresses_15{
    0x46a5bc, 0x46a5b8, 0x46a5b4, 0x46a5b0, 0x46a4fc, 0x46a5ac,
    0x46a178};
// Field order is FullScoreboardConstants' twelve binary32 words followed by
// the literal binary64 CRT-power exponent. These are source operand addresses.
constexpr std::array<std::uint32_t,13> scoreboard_addresses_11{
    0x465090, 0x465080, 0x46502c, 0x46507c, 0x465030, 0x465138,
    0x465090, 0x452000, 0x46518c, 0x4651a0, 0x45e208, 0x465764,
    0x4652d8};
constexpr std::array<std::uint32_t,13> scoreboard_addresses_15{
    0x46a130, 0x46a120, 0x46a0cc, 0x46a11c, 0x46a0d0, 0x46a1d8,
    0x46a130, 0x456918, 0x46a22c, 0x46a240, 0x445b20, 0x46a81c,
    0x46a378};
// Exact field order from scope-camera/audit-frozen/constants.json. The first
// five operands belong to executed source constructors, not serialized BSS.
constexpr std::array<std::uint32_t,24> scope_addresses_11{
    0x465ad0,0x4650c4,0x465540,0x4650c4,0x465540,0x46570c,0x465708,
    0x465030,0x47885c,0x4653b8,0x4701c8,0x46502c,0x465044,0x465060,
    0x465068,0x46506c,0x465070,0x465078,0x46507c,0x465080,0x465084,
    0x465218,0x44b020,0x465130};
constexpr std::array<std::uint32_t,24> scope_addresses_15{
    0x46ab88,0x46a164,0x46a5f8,0x46a164,0x46a5f8,0x46a7c4,0x46a7c0,
    0x46a0d0,0x47da8c,0x46a458,0x475288,0x46a0cc,0x46a0e4,0x46a100,
    0x46a108,0x46a10c,0x46a110,0x46a118,0x46a11c,0x46a120,0x46a124,
    0x46a2b8,0x44eac8,0x46a1d0};
constexpr std::array<std::uint32_t,5> animation_addresses_11{
    0x465030, 0x465048, 0x46504c, 0x4650c0, 0x465028};
constexpr std::array<std::uint32_t,5> animation_addresses_15{
    0x46a0d0, 0x46a0e8, 0x46a0ec, 0x46a160, 0x46a0c8};
std::uint64_t fingerprint(std::span<const std::byte> bytes) {
  std::uint64_t hash = 14695981039346656037ull;
  for (auto byte : bytes)
    hash = (hash ^ std::to_integer<unsigned>(byte)) * 1099511628211ull;
  return hash;
}
void require(std::span<const std::byte> data, std::size_t at, std::size_t length) {
  if (at > data.size() || length > data.size() - at)
    throw FormatError("truncated original executable data");
}
std::uint32_t u32(std::span<const std::byte> data, std::size_t at) {
  require(data, at, 4);
  std::uint32_t out = 0;
  for (unsigned i = 0; i < 4; ++i)
    out |= std::to_integer<std::uint32_t>(data[at + i]) << (8 * i);
  return out;
}
std::uint16_t u16(std::span<const std::byte> data, std::size_t at) {
  require(data, at, 2);
  return std::to_integer<std::uint16_t>(data[at]) |
         (std::to_integer<std::uint16_t>(data[at + 1]) << 8);
}
std::span<const std::byte> section_data(std::span<const std::byte> data,
                                      std::uint32_t address, std::size_t length) {
  const auto base = u32(data, 0x104), count = u32(data, 0x11c), table = u32(data, 0x120);
  if (table < base || count > 128)
    throw FormatError("invalid original executable section table");
  const auto offset = std::size_t(table - base);
  require(data, offset, std::size_t(count) * 56);
  for (unsigned i = 0; i < count; ++i) {
    const auto p = offset + i * 56;
    const auto start = u32(data, p + 4), raw = u32(data, p + 12), size = u32(data, p + 16);
    if (address >= start && std::uint64_t(address) - start <= size &&
        length <= size - (std::uint64_t(address) - start)) {
      const auto at = std::uint64_t(raw) + address - start;
      require(data, at, length);
      return data.subspan(at, length);
    }
  }
  throw FormatError("original table is outside executable sections");
}
std::string original_string(std::span<const std::byte> data, std::uint32_t address) {
  std::string result;
  // Bounded owned-image read, not a replacement for an unavailable string.
  for (std::uint32_t offset = 0; offset < 4096; ++offset) {
    const auto byte = section_data(data, address + offset, 1).front();
    if (byte == std::byte{}) return result;
    result.push_back(char(std::to_integer<unsigned char>(byte)));
  }
  throw FormatError("unterminated original executable string");
}
} // namespace

std::shared_ptr<const OriginalData> OriginalData::load_xbe(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream)
    throw FormatError("Original game data requires your extracted default.xbe: " + path.string());
  const auto size = stream.tellg();
  if (size < 0x168 || size > 16 * 1024 * 1024)
    throw FormatError("unsupported original executable size");
  std::vector<std::byte> storage(static_cast<std::size_t>(size));
  stream.seekg(0);
  if (!stream.read(reinterpret_cast<char *>(storage.data()), storage.size()))
    throw FormatError("could not read original executable data");
  const std::span<const std::byte> data(storage);
  if (u32(data, 0) != 0x48454258u)
    throw FormatError("original data source is not an Xbox executable");
  auto result = std::make_shared<OriginalData>();
  result->source = path;
  const auto hash = fingerprint(data);
  std::uint32_t directions, fade, projection;
  std::span<const std::uint32_t> balance_addresses;
  std::span<const std::uint32_t> netgame_addresses;
  std::span<const std::uint32_t> scoreboard_addresses;
  std::span<const std::uint32_t> scope_addresses;
  std::span<const std::uint32_t> animation_addresses;
  if (hash == 0x9c29175742280993ull) {
    directions = 0x4417f0; fade = 0x46d148; projection=0x440b94;
  } else if (hash == 0x598bf549dd8fb828ull) {
    result->image = Image::MapPack11;
    directions = 0x447e40; fade = 0x4745e8; projection=0x4471e4;
    balance_addresses = balance_addresses_11;
    netgame_addresses = netgame_addresses_11;
    scoreboard_addresses = scoreboard_addresses_11;
    scope_addresses = scope_addresses_11;
    animation_addresses = animation_addresses_11;
  } else if (hash == 0x08aeec2701bca325ull) {
    result->image = Image::TitleUpdate15;
    directions = 0x44b8e8; fade = 0x4796a8; projection=0x44ac8c;
    balance_addresses = balance_addresses_15;
    netgame_addresses = netgame_addresses_15;
    scoreboard_addresses = scoreboard_addresses_15;
    scope_addresses = scope_addresses_15;
    animation_addresses = animation_addresses_15;
  } else {
    throw FormatError("unsupported original executable; import the supported Halo 2 disc");
  }
  // Complete DSOUND bodies, encoded consumers and full immutable ranges are
  // paired in research_sound_sdk_images.py. These are per-image addresses,
  // not assumed section offsets or replacement spatial tuning constants.
  constexpr std::array<std::array<std::uint32_t,28>,3> sdk_addresses{{
      {0x45dbd8,0x45dbc0,0x45dbbc,0x417238,0x4171f8,0x4171e8,0x4171ec,
       0x45dfcc,0x45e320,0x45dccc,0x385a48,0x385804,0x385a58,0x385a78,0x3869c0,0x45dc10,
       0x45dbb8,0x4171e4,0x45dde8,0x385a44,0x4171f4,0x4171f0,0x4122dc,0x4172b4,0x4123e8,0x3869e4,0x45e4f0,0x45dcc0},
      {0x465048,0x465030,0x46502c,0x41d878,0x41d838,0x41d828,0x41d82c,
       0x465464,0x4652d0,0x46513c,0x38c088,0x38be44,0x38c098,0x38c0b8,0x38d000,0x465080,
       0x465028,0x41d824,0x465258,0x38c084,0x41d834,0x41d830,0x41891c,0x41d8f4,0x418a28,0x38d024,0x465990,0x465130},
      {0x46a0e8,0x46a0d0,0x46a0cc,0x4212e8,0x4212a8,0x421298,0x42129c,
       0x46a504,0x46a370,0x46a1dc,0x38fb28,0x38f8e4,0x38fb38,0x38fb58,0x390aa0,0x46a120,
       0x46a0c8,0x421294,0x46a2f8,0x38fb24,0x4212a4,0x4212a0,0x41c3bc,0x421364,0x41c4c8,0x390ac4,0x46aa48,0x46a1d0}}};
  const auto &sdk_addresses_selected = sdk_addresses[static_cast<unsigned>(result->image)];
  auto &sdk_data = result->sound_sdk_spatial_data.emplace();
  auto &sdk = sdk_data.constants;
  const std::array<std::uint32_t *,10> sdk_fields{
      &sdk.zero_bits,&sdk.one_bits,&sdk.half_bits,&sdk.position_sentinel_bits,
      &sdk.angle_round_bias_bits,&sdk.half_turn_bits,&sdk.azimuth_scale_bits,
      &sdk.octant_bits,&sdk.quarter_turn_bits,&sdk.volume_scale_bits};
  for (std::size_t i = 0; i < sdk_fields.size(); ++i)
    *sdk_fields[i] = u32(section_data(data,sdk_addresses_selected[i],4),0);
  for (std::size_t i = 0; i < 3; ++i) {
    sdk.mode_one_side[i] = u32(section_data(data,sdk_addresses_selected[10]+i*4,4),0);
    sdk.mode_one_forward[i] = u32(section_data(data,sdk_addresses_selected[11]+i*4,4),0);
  }
  sdk.kernel_table_a = sdk_addresses_selected[12];
  sdk.kernel_table_b = sdk_addresses_selected[13];
  sdk_data.null_kernel_identity = sdk_addresses_selected[14];
  sdk_data.cone_constants = {sdk.octant_bits,sdk.quarter_turn_bits,
      u32(section_data(data,sdk_addresses_selected[15],4),0)};
  auto &sdk_voice = sdk_data.voice_constants;
  sdk_voice.zero_bits=sdk.zero_bits;sdk_voice.one_bits=sdk.one_bits;
  const std::array<std::uint32_t *,11> sdk_voice_fields{
      &sdk_voice.unsigned_correction_bits,&sdk_voice.gain_scale_bits,&sdk_voice.volume_scale_bits,
      &sdk_voice.doppler_max_bits,&sdk_voice.doppler_min_bits,&sdk_voice.doppler_reciprocal_bits,
      &sdk_voice.environment_log_floor_bits,&sdk_voice.sample_rate_bits,&sdk_voice.two_pi_bits,
      &sdk_voice.filter_scale_bits,&sdk_voice.db_exponent_scale_bits};
  for (std::size_t i=0;i<sdk_voice_fields.size();++i)
    *sdk_voice_fields[i]=u32(section_data(data,sdk_addresses_selected[16+i],4),0);
  const auto sdk_voice_double=section_data(data,sdk_addresses_selected[27],8);
  sdk_voice.one_double_bits=std::uint64_t(u32(sdk_voice_double,0))|
      (std::uint64_t(u32(sdk_voice_double,4))<<32);
  const auto sdk_kernels = section_data(data,sdk.kernel_table_a,sdk_data.kernels.size());
  std::copy(sdk_kernels.begin(),sdk_kernels.end(),sdk_data.kernels.begin());
  const auto sdk_null = section_data(data,sdk_data.null_kernel_identity,sdk_data.null_kernel.size());
  std::copy(sdk_null.begin(),sdk_null.end(),sdk_data.null_kernel.begin());
  // Encoded constructor operands and reached AddRef slots in all three
  // complete image pairings; table relocation is not a section-offset guess.
  constexpr std::array<std::array<std::uint32_t,16>,3> sdk_construction_addresses{{
      {0x387198,0x386b08,0x386b04,0x386b0c,0x417120,0x4170e4,0x417140,0x417150,0x417254,0x41727c,0x379d0c,0x37a14f,0x41718c,0x417200,0x3857ec,0x3857b8},
      {0x38d7d8,0x38d148,0x38d144,0x38d14c,0x41d760,0x41d724,0x41d780,0x41d790,0x41d894,0x41d8bc,0x38034c,0x38078f,0x41d7cc,0x41d840,0x38be2c,0x38bdf8},
      {0x391278,0x390be8,0x390be4,0x390bec,0x4211c0,0x42124c,0x4211e0,0x4211f0,0x421304,0x42132c,0x383dec,0x38422f,0x42122c,0x4212b0,0x38f8cc,0x38f898}}};
  const auto &sc=sdk_construction_addresses[static_cast<unsigned>(result->image)];
  auto &construction=sdk_data.construction;
  construction.singleton_address=sc[0];construction.speaker_override_address=sc[1];
  construction.speaker_config_address=sc[2];construction.busy_address=sc[3];
  construction.device_vtable=sc[4];construction.device_data_vtable=sc[5];
  construction.buffer_base_vtable=sc[6];construction.buffer_vtable=sc[7];
  construction.voice_base_vtable=sc[8];construction.voice_vtable=sc[9];
  construction.plain_reference_method=sc[10];construction.synchronized_reference_method=sc[11];
  const std::array<unsigned,8> reference_tables{4,5,6,7,8,9,12,13};
  for(unsigned i=0;i<reference_tables.size();++i){
    const auto table=sc[reference_tables[i]];
    const auto method=u32(section_data(data,table+4u,4),0);
    if(method!=sc[(i==0||i==3)?11:10])
      throw FormatError("Original SDK constructor reference method differs");
    construction.references[i]={table,method};
  }
  for(unsigned i=0;i<construction.listener_defaults.size();++i)
    construction.listener_defaults[i]=u32(section_data(data,sc[14]+i*4u,4),0);
  for(unsigned i=0;i<construction.environment_defaults.size();++i)
    construction.environment_defaults[i]=u32(section_data(data,sc[15]+i*4u,4),0);
  // Complete format/initialization consumers and relocated bin records are
  // paired in sdk-image-pairing-v15. No native channel policy replaces them.
  constexpr std::array<std::array<std::uint32_t,10>,3> sdk_voice_initialization_addresses{{
      {0x385854,0x385828,0x3857a8,0x385798,0x385748,0x38589c,0x38577c,0x38576c,0x45dbb8,0x45dde0},
      {0x38be94,0x38be68,0x38bde8,0x38bdd8,0x38bd88,0x38bedc,0x38bdbc,0x38bdac,0x465028,0x465250},
      {0x38f934,0x38f908,0x38f888,0x38f878,0x38f828,0x38f97c,0x38f85c,0x38f84c,0x46a0c8,0x46a2f0}}};
  const auto &vi=sdk_voice_initialization_addresses[static_cast<unsigned>(result->image)];
  auto &initialization=sdk_data.voice_initialization;
  for(unsigned i=0;i<18;++i)initialization.source_defaults[i]=u32(section_data(data,vi[0]+i*4u,4),0);
  for(unsigned i=0;i<9;++i)initialization.environment_defaults[i]=u32(section_data(data,vi[1]+i*4u,4),0);
  for(unsigned i=0;i<4;++i){
    initialization.pcm_guid[i]=u32(section_data(data,vi[2]+i*4u,4),0);
    initialization.adpcm_guid[i]=u32(section_data(data,vi[3]+i*4u,4),0);
  }
  initialization.default_format_identity=vi[4];initialization.spatial_bins_identity=vi[5];
  initialization.parent_bins_identity=vi[6];initialization.channel_bins_table_identity=vi[7];
  initialization.unsigned_correction_bits=u32(section_data(data,vi[8],4),0);
  initialization.frequency_reciprocal_bits=u32(section_data(data,vi[9],4),0);
  const auto retain_voice_immutable=[&](std::uint32_t identity,std::size_t size){
    if(std::ranges::any_of(initialization.immutable,[&](const auto &r){return r.identity==identity;}))return;
    const auto raw=section_data(data,identity,size);
    initialization.immutable.push_back({identity,{raw.begin(),raw.end()}});
  };
  retain_voice_immutable(vi[7],16);
  const std::array<std::uint32_t,6> bin_headers{vi[5],vi[6],
      u32(section_data(data,vi[7],4),0),u32(section_data(data,vi[7]+4u,4),0),
      u32(section_data(data,vi[7]+8u,4),0),u32(section_data(data,vi[7]+12u,4),0)};
  for(const auto header:bin_headers){
    retain_voice_immutable(header,8);
    const auto count=u32(section_data(data,header,4),0),records=u32(section_data(data,header+4u,4),0);
    if(count>8u)throw FormatError("Original SDK mix-bin header exceeds retained source capacity");
    if(count)retain_voice_immutable(records,count*8u);
  }
  retain_voice_immutable(vi[4],18);
  // Complete381A74 mapping-cell operands paired in sdk-image-pairing-v16.
  // The selected address is a mutable binding; no runtime backing is invented.
  constexpr std::array<std::uint32_t,3> sdk_voice_mapping_cells{0x3872c0,0x38d900,0x3913a0};
  sdk_data.hardware_voice.voice_mapping_pointer_address=sdk_voice_mapping_cells[static_cast<unsigned>(result->image)];
  // Complete38094B/382FE3 bodies and reached constructor vtable+14 slots
  // paired in sdk-image-pairing-v17. Resolve retained original identities.
  constexpr std::array<std::array<std::uint32_t,2>,3> sdk_format_methods{{
      {0x38094b,0x382fe3},{0x386f8b,0x389623},{0x38aa2b,0x38d0c3}}};
  const auto &format_methods=sdk_format_methods[static_cast<unsigned>(result->image)];
  auto &voice_dispatch=sdk_data.voice_dispatch;
  voice_dispatch.base_format_method=format_methods[0];voice_dispatch.reconfigure_method=format_methods[1];
  const std::array<std::uint32_t,2> format_tables{construction.voice_base_vtable,construction.voice_vtable};
  for(unsigned i=0;i<format_tables.size();++i){
    const auto method=u32(section_data(data,format_tables[i]+0x14u,4),0);
    if(method!=format_methods[i])throw FormatError("Original SDK voice format dispatch differs");
    voice_dispatch.format_methods[i]={format_tables[i],method};
  }
  // Complete38025A constructor operands paired in sdk-image-pairing-v20.
  // The available-page cell is mutable runtime state, not an imported count.
  constexpr std::array<SoundSdkSamplePageBindings,3> sdk_sample_page_bindings{{
      {0x417240,0x386af8},{0x41d880,0x38d138},{0x4212f0,0x390bd8}}};
  sdk_data.sample_pages=sdk_sample_page_bindings[static_cast<unsigned>(result->image)];
  // Complete382BE3 mutable stream backing operand paired across all three
  // owned images in sdk-image-pairing-v22; no live pointer is imported here.
  constexpr std::array<std::uint32_t,3> sdk_stream_sample_cells{0x387330,0x38d970,0x391410};
  sdk_data.voice_attachment.stream_samples_pointer_address=sdk_stream_sample_cells[static_cast<unsigned>(result->image)];
  // Exact derived constructor table and complete382AD9/382B08 methods paired
  // in sdk-image-pairing-v24. Only the reached arm/cancel slots are admitted.
  constexpr std::array<std::array<std::uint32_t,2>,3> sdk_notification_methods{{
      {0x382ad9,0x382b08},{0x389119,0x389148},{0x38cbb9,0x38cbe8}}};
  const auto &notification_methods=sdk_notification_methods[static_cast<unsigned>(result->image)];
  const auto arm_method=u32(section_data(data,construction.voice_vtable+0x1cu,4),0);
  const auto cancel_method=u32(section_data(data,construction.voice_vtable+0x20u,4),0);
  if(arm_method!=notification_methods[0]||cancel_method!=notification_methods[1])
    throw FormatError("Original SDK voice notification dispatch differs");
  sdk_data.notifications={construction.voice_vtable,cancel_method,arm_method};
  // Complete gate/unlink/force/status operands and derived status slots are
  // paired across all three owned images in sdk-image-pairing-v25.
  constexpr std::array<std::array<std::uint32_t,4>,3> sdk_retirement_bindings{{
      {0x38699c,0x387110,0x3872b0,0x3829f7},
      {0x38cfdc,0x38d750,0x38d8f0,0x389037},
      {0x390a7c,0x3911f0,0x391390,0x38cad7}}};
  const auto &retirement=sdk_retirement_bindings[static_cast<unsigned>(result->image)];
  const auto status_method=u32(section_data(data,construction.voice_vtable+0x18u,4),0);
  if(status_method!=retirement[3])throw FormatError("Original SDK retirement status dispatch differs");
  sdk_data.voice_retirement={retirement[0],retirement[1],retirement[2],
      sdk_data.hardware_voice.voice_mapping_pointer_address,construction.voice_vtable,status_method,arm_method};

  // Complete1389C0/13A130/13B4A0,138EB0/13A620/13B990 and
  //188CD0/18A700/18BAC0 operand mappings verified by oracle_game_over.py.
  const auto game_over_addresses=result->image==Image::Base ?
      std::array<std::uint32_t,3>{0x45dda8,0x45dc0c,0x45dc5c} :
      result->image==Image::MapPack11 ?
      std::array<std::uint32_t,3>{0x465218,0x46507c,0x4650cc} :
      std::array<std::uint32_t,3>{0x46a2b8,0x46a11c,0x46a16c};
  auto& game_over=result->game_over_constants.emplace();
  game_over.seconds=std::bit_cast<float>(u32(section_data(data,game_over_addresses[0],4),0));
  game_over.fade_threshold_seconds=std::bit_cast<float>(u32(section_data(data,game_over_addresses[1],4),0));
  game_over.ambient_linear_gain=std::bit_cast<float>(u32(section_data(data,game_over_addresses[2],4),0));
  game_over.fade_seconds=std::bit_cast<float>(0x3f800000u); // literal PUSH in all four calls
  // Whole1968B0/1983B0/19B2B0 and1967D0/1982D0/19B1D0 operands;
  // original16-byte entries retain their unclassified12-byte tails.
  const auto result_stat_address=result->image==Image::Base ? 0x46ddc8u :
      result->image==Image::MapPack11 ? 0x4752b0u : 0x47a388u;
  const auto result_stat_bytes=section_data(data,result_stat_address,45*16);
  auto& result_stats=result->result_stat_descriptors.emplace();
  for(unsigned i=0;i<result_stats.size();++i) {
    const auto at=i*16;
    result_stats[i].minimum=std::uint16_t(std::to_integer<unsigned>(result_stat_bytes[at])|
        (std::to_integer<unsigned>(result_stat_bytes[at+1])<<8));
    result_stats[i].maximum=std::uint16_t(std::to_integer<unsigned>(result_stat_bytes[at+2])|
        (std::to_integer<unsigned>(result_stat_bytes[at+3])<<8));
    for(unsigned n=0;n<12;++n)result_stats[i].remaining[n]=std::to_integer<std::uint8_t>(result_stat_bytes[at+4+n]);
  }
  const auto capture_addresses=result->image==Image::Base ?
      std::array<std::uint32_t,3>{0x46e098,0x196ff8,0x45dc10} :
      result->image==Image::MapPack11 ?
      std::array<std::uint32_t,3>{0x475580,0x198af8,0x465080} :
      std::array<std::uint32_t,3>{0x47a658,0x19b9f8,0x46a120};
  auto& capture=result->result_capture_constants.emplace();
  const auto weapon_stats=section_data(data,capture_addresses[0],7*16);
  for(unsigned i=0;i<7;++i) {
    const auto at=i*16;
    capture.weapon_descriptors[i].minimum=std::uint16_t(std::to_integer<unsigned>(weapon_stats[at])|
        (std::to_integer<unsigned>(weapon_stats[at+1])<<8));
    capture.weapon_descriptors[i].maximum=std::uint16_t(std::to_integer<unsigned>(weapon_stats[at+2])|
        (std::to_integer<unsigned>(weapon_stats[at+3])<<8));
    for(unsigned n=0;n<12;++n)capture.weapon_descriptors[i].remaining[n]=std::to_integer<std::uint8_t>(weapon_stats[at+4+n]);
  }
  const auto cause_dispatch=section_data(data,capture_addresses[1],41*4);
  for(unsigned cause=0;cause<41;++cause) {
    const auto code=section_data(data,u32(cause_dispatch,cause*4),6);
    if(std::to_integer<unsigned>(code[0])==0xb8 && std::to_integer<unsigned>(code[5])==0xc3) {
      const auto weapon=u32(code,1);
      if(weapon>40)throw FormatError("Original result-stat dispatch exceeds recovered store");
      capture.cause_weapons[cause]=std::uint8_t(weapon);
    } else if(std::to_integer<unsigned>(code[0])!=0x33 || std::to_integer<unsigned>(code[1])!=0xc0 ||
        std::to_integer<unsigned>(code[2])!=0xc3)
      throw FormatError("Original result-stat dispatch leaf changed");
  }
  capture.multikill_seconds=std::bit_cast<float>(u32(section_data(data,capture_addresses[2],4),0));
  const auto direction_bytes = section_data(data, directions, 1026 * 12),
             fade_bytes = section_data(data, fade, 1024);
  if (fingerprint(direction_bytes) != 0x6778693f5db53615ull ||
      fingerprint(fade_bytes) != 0xe2570a57f6e2dc5eull)
    throw FormatError("original gameplay tables do not match the supported images");
  for (unsigned i = 0; i < result->weapon_directions.size(); ++i) {
    result->weapon_directions[i] = {
        std::bit_cast<float>(u32(direction_bytes, i * 12)),
        std::bit_cast<float>(u32(direction_bytes, i * 12 + 4)),
        std::bit_cast<float>(u32(direction_bytes, i * 12 + 8))};
  }
  for (unsigned i = 0; i < result->fade_curve.size(); ++i)
    result->fade_curve[i] = std::to_integer<std::uint8_t>(fade_bytes[i]);
  const auto projection_bytes=section_data(data,projection,36);
  if(fingerprint(projection_bytes)!=0xed2df01a26bd8b1dull)
    throw FormatError("Original collision projection table differs");
  for(unsigned row=0;row<6;++row)
    for(unsigned coordinate=0;coordinate<3;++coordinate) {
      const auto p=row*6+coordinate*2;
      const auto value=std::uint16_t(std::to_integer<unsigned>(projection_bytes[p]) |
          (std::to_integer<unsigned>(projection_bytes[p+1])<<8));
      result->collision_projection[row][coordinate]=std::bit_cast<std::int16_t>(value);
    }
  if (!balance_addresses.empty()) {
    auto &constants = result->multiplayer_balance_constants.emplace();
    for (std::size_t i = 0; i < constants.size(); ++i)
      constants[i] = u32(section_data(data, balance_addresses[i], 4), 0);
  }
  if (!animation_addresses.empty()) {
    auto &constants = result->unit_animation_constants.emplace();
    constants.one_bits = u32(section_data(data, animation_addresses[0], 4), 0);
    constants.zero_bits = u32(section_data(data, animation_addresses[1], 4), 0);
    constants.frame_subtractor_bits =
        u32(section_data(data, animation_addresses[2], 4), 0);
    constants.rng_scale_bits = u32(section_data(data, animation_addresses[3], 4), 0);
    constants.unsigned_conversion_correction_bits =
        u32(section_data(data, animation_addresses[4], 4), 0);
    // Complete DE960/DF880 pairing: source COMISS after the three-axis sum.
    result->unit_locomotion_deadzone_bits = u32(section_data(data,
        result->image == Image::TitleUpdate15 ? 0x46a560 : 0x4654c0, 4), 0);
    const bool later=result->image==Image::TitleUpdate15;
    result->unit_facing_constants=std::array{
      u32(section_data(data,later?0x46a0ec:0x46504c,4),0),
      u32(section_data(data,later?0x46a8fc:0x465844,4),0),
      u32(section_data(data,later?0x46a63c:0x465584,4),0)};
    auto &melee=result->melee_constants.emplace();
    const auto melee_table=section_data(data,later?0x44a8dc:0x446e34,48);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<3;++j)
      melee.candidates[i][j]=u32(melee_table,i*12+j*4);
    melee.frame_seconds_bits=u32(section_data(data,later?0x46a17c:0x4650dc,4),0);
    melee.impact_scale_bits=u32(section_data(data,later?0x46a0c4:0x465024,4),0);
    melee.moving_threshold_bits=u32(section_data(data,later?0x46a194:0x4650f4,4),0);
    melee.special_seconds_bits=u32(section_data(data,later?0x46a11c:0x46507c,4),0);
    melee.release_seconds_bits={u32(section_data(data,later?0x44a8a0:0x446df8,4),0),u32(section_data(data,later?0x44a8d4:0x446e2c,4),0)};
    // Complete paired1EF030/1EF290 operand correspondence, retained in
    // analysis/player-animations-20261012/melee-physics-pairing.json.
    constexpr std::array<std::array<std::uint32_t,3>,22> physics_operands{{
      {0x446548,0x449fd4,4},{0x46502c,0x46a0cc,4},{0x465030,0x46a0d0,4},
      {0x465044,0x46a0e4,4},{0x465048,0x46a0e8,4},{0x465060,0x46a100,4},
      {0x465064,0x46a104,4},{0x46506c,0x46a10c,4},{0x465080,0x46a120,4},
      {0x465084,0x46a124,4},{0x465088,0x46a128,4},{0x465090,0x46a130,4},
      {0x4650cc,0x46a16c,4},{0x4650d4,0x46a174,4},{0x4650e0,0x46a180,4},
      {0x4651a0,0x46a240,4},{0x465278,0x46a318,4},{0x465470,0x46a510,4},
      {0x465670,0x46a728,4},{0x465678,0x46a730,8},{0x465680,0x46a738,8},{0x465688,0x46a740,8}}};
    for(const auto &row:physics_operands){const auto bytes=section_data(data,row[later?1:0],row[2]);
      melee.physics.literals.emplace_back(row[0],std::vector<std::byte>(bytes.begin(),bytes.end()));}
    for(unsigned i=0;i<3;++i){constexpr std::array<std::array<std::uint32_t,2>,3> pointers{{{0x46fc28,0x474ce8},{0x46fc44,0x474d04},{0x46fc48,0x474d08}}};
      const auto ptr=u32(section_data(data,pointers[i][later?1:0],4),0);const auto bytes=section_data(data,ptr,12);
      std::copy(bytes.begin(),bytes.end(),melee.physics.basis[i].begin());}
    result->grenade_constants=NativeGrenadeConstants{
        u32(section_data(data,later?0x456918:0x452000,4),0),
        u32(section_data(data,later?0x46a4c0:0x465420,4),0)};
  }
  if (result->image != Image::Base) {
    auto &constants = result->first_person_sound_constants.emplace();
    const bool later = result->image == Image::TitleUpdate15;
    // Dereference the original immutable basis pointers; no serialized BSS
    // word is used as a live BSP, transform or listener default.
    const auto position = u32(section_data(data, later ? 0x474ce8 : 0x46fc28, 4), 0);
    const auto forward = u32(section_data(data, later ? 0x474d08 : 0x46fc48, 4), 0);
    const auto position_bytes = section_data(data, position, 12);
    const auto forward_bytes = section_data(data, forward, 12);
    std::copy(position_bytes.begin(), position_bytes.end(), constants.marker_position.begin());
    std::copy(forward_bytes.begin(), forward_bytes.end(), constants.marker_forward.begin());
    constants.ordinary_callbacks = later ? 0x44ebf4 : 0x44b14c;
    constants.weapon_callbacks = later ? 0x44ec14 : 0x44b16c;
    // Validate that each identity names a complete resident callback table;
    // callback behavior remains supplied by the native source owners.
    (void)section_data(data, constants.ordinary_callbacks, 32);
    (void)section_data(data, constants.weapon_callbacks, 32);
    auto &object = result->sound_object_request_constants.emplace();
    object.one_bits = u32(section_data(data, later ? 0x46a0d0 : 0x465030, 4), 0);
    object.minus_one_bits = u32(section_data(data, later ? 0x46a0dc : 0x46503c, 4), 0);
    object.z_scale_bits = u32(section_data(data, later ? 0x46a338 : 0x465298, 4), 0);
    object.xy_scale_bits = u32(section_data(data, later ? 0x46a33c : 0x46529c, 4), 0);
    const auto zero = u32(section_data(data, later ? 0x474d04 : 0x46fc44, 4), 0);
    const auto zero_bytes = section_data(data, zero, 12);
    std::copy(zero_bytes.begin(), zero_bytes.end(), object.zero_vector.begin());
    auto &actions = result->unit_action_tables.emplace();
    actions.action_bank = later ? 0x473d28 : 0x46ec68;
    actions.object_type_bank = later ? 0x474b90 : 0x46fad0;
    const auto pointers = section_data(data, actions.action_bank,
                                      actions.descriptors.size() * 4);
    for (std::size_t i = 0; i < actions.descriptors.size(); ++i) {
      actions.descriptor_addresses[i] = u32(pointers, i * 4);
      const auto record = section_data(data, actions.descriptor_addresses[i], 16);
      std::copy(record.begin(), record.end(), actions.descriptors[i].begin());
    }
    constexpr std::array<std::uint32_t,3> verified_types{0, 2, 5};
    for (std::size_t i = 0; i < verified_types.size(); ++i) {
      auto &owned = actions.object_types[i];
      owned.type = verified_types[i];
      owned.source_address = u32(section_data(data,
          actions.object_type_bank + owned.type * 4, 4), 0);
      const auto record = section_data(data, owned.source_address, 16);
      std::copy(record.begin(), record.end(), owned.bytes.begin());
    }
    auto construction = std::make_shared<SourceObjectConstructionTables>();
    construction->bank = actions.object_type_bank;
    construction->base_extents.emplace();
    construction->placement_extents.emplace();
    construction->scenario_block_offsets.emplace();
    const auto retain_construction = [&](std::uint32_t address, std::size_t bytes) {
      const auto original = section_data(data, address, bytes);
      const auto [found, inserted] = construction->ranges.try_emplace(
          address, original.begin(), original.end());
      if (!inserted && (found->second.size() != bytes ||
          !std::equal(original.begin(), original.end(), found->second.begin())))
        throw FormatError("Original object construction field overlap differs");
    };
    retain_construction(construction->bank, construction->type_addresses.size() * 4);
    for (std::size_t type = 0; type < construction->type_addresses.size(); ++type) {
      const auto address = u32(section_data(data, construction->bank + type * 4, 4), 0);
      construction->type_addresses[type] = address;
      retain_construction(address, 8);
      (*construction->base_extents)[type] = u16(section_data(data,address+8,2),0);
      (*construction->placement_extents)[type] = u16(section_data(data,address+0xe,2),0);
      (*construction->scenario_block_offsets)[type] = {
          std::bit_cast<std::int16_t>(u16(section_data(data,address+0xa,2),0)),
          std::bit_cast<std::int16_t>(u16(section_data(data,address+0xc,2),0))};
      // The independently paired selected images have at most four slots,
      // including the actual NULL. Extra fields/types remain unavailable.
      std::size_t slots = 0;
      for (; slots < 4; ++slots) {
        const auto method_owner = u32(section_data(data, address + 0x84 + slots * 4, 4), 0);
        if (!method_owner) break;
        retain_construction(method_owner + 0x28, 8);
      }
      if (slots == 4)
        throw FormatError("Original object construction inheritance terminator unavailable");
      retain_construction(address + 0x84, (slots + 1) * 4);
    }
    result->object_construction_tables = std::move(construction);
    auto &request = result->object_creation_request_operands.emplace();
    request.one_bits = u32(section_data(data,later ? 0x46a0d0 : 0x465030,4),0);
    request.basis_forward = constants.marker_forward;
    const auto up_pointer = u32(section_data(data,later ? 0x474d10 : 0x46fc50,4),0);
    const auto up_bytes = section_data(data,up_pointer,12);
    std::copy(up_bytes.begin(),up_bytes.end(),request.basis_up.begin());
    auto &scenario_request = result->scenario_request_operands.emplace();
    scenario_request.zero_bits = u32(section_data(data,later ? 0x46a0e8 : 0x465048,4),0);
    scenario_request.one_bits = request.one_bits;
    scenario_request.color_scale_bits = u32(section_data(data,later ? 0x46a0c4 : 0x465024,4),0);
    auto &tail = result->unit_constructor_tail_constants.emplace();
    tail.one_bits = result->unit_animation_constants->one_bits;
    tail.zero_bits = result->unit_animation_constants->zero_bits;
    tail.normal_blend_bits = u32(section_data(data, later ? 0x450170 : 0x44c6b4, 4), 0);
    tail.special_blend_bits = u32(section_data(data, later ? 0x46a1e0 : 0x465140, 4), 0);
    const auto pairs = section_data(data, later ? 0x474be4 : 0x46fb24, 32);
    for (std::size_t row = 0; row < tail.activity_pairs.size(); ++row)
      for (std::size_t column = 0; column < 2; ++column)
        tail.activity_pairs[row][column] = u32(pairs, row * 8 + column * 4);
    auto &driver = result->sound_driver_parameter_constants.emplace();
    // Literal operand order from the complete paired driver-body authority.
    // Runtime driver/stream state, format choices and voice indices are absent.
    constexpr std::array<std::uint32_t,16> driver_addresses_11{
        0x4650bc,0x4650b8,0x4650b4,0x46549c,0x45120c,0x4650d8,
        0x446548,0x4652ec,0x4511a4,0x465030,0x46507c,0x451214,
        0x46508c,0x465078,0x46504c,0x451394};
    constexpr std::array<std::uint32_t,16> driver_addresses_15{
        0x46a15c,0x46a158,0x46a154,0x46a53c,0x455b0c,0x46a178,
        0x449fd4,0x46a38c,0x455aa4,0x46a0d0,0x46a11c,0x455b14,
        0x46a12c,0x46a118,0x46a0ec,0x455c94};
    const std::array<std::uint32_t *,16> driver_fields{
        &driver.source_pitch_to_driver_pitch_bits,&driver.driver_pitch_min_bits,
        &driver.driver_pitch_max_bits,&driver.gain_normalized_to_driver_volume_bits,
        &driver.minimum_source_gain_bits,&driver.parameter_change_threshold_bits,
        &driver.driver_pitch_change_threshold_bits,&driver.driver_pitch_octave_scale_bits,
        &driver.relative_rate_ln2_bits,&driver.mono_channel_count_bits,
        &driver.stereo_channel_count_bits,&driver.routing_db_ln10_bits,
        &driver.routing_db_scale_bits,&driver.routing_linear_db_scale_bits,
        &driver.normalized_parameter_range_epsilon_bits,&driver.silent_source_gain_bits};
    const auto &driver_addresses = later ? driver_addresses_15 : driver_addresses_11;
    for (std::size_t i = 0; i < driver_fields.size(); ++i)
      *driver_fields[i] = u32(section_data(data, driver_addresses[i], 4), 0);
    auto &startup = result->sound_startup_constants.emplace();
    startup.ambient_transition_bits = u32(section_data(data, later ? 0x44923c : 0x4457bc, 4), 0);
    const auto policy = section_data(data, later ? 0x455b20 : 0x451220, 16);
    std::copy(policy.begin(), policy.end(), startup.voice_policy.begin());
    auto &cache_startup = result->sound_cache_startup_constants.emplace();
    // Actual PUSH imm32 operands from the complete paired21A6E0 constructor.
    // These source callback identities are never cast to native function pointers.
    const auto deleted = section_data(data, later ? 0x21f036 : 0x21a736, 5);
    const auto busy = section_data(data, later ? 0x21f031 : 0x21a731, 5);
    if (deleted.front() != std::byte{0x68} || busy.front() != std::byte{0x68})
      throw FormatError("Original sound cache callback instructions unavailable");
    cache_startup.delete_callback_identity = u32(deleted, 1);
    cache_startup.busy_callback_identity = u32(busy, 1);
    (void)section_data(data, cache_startup.delete_callback_identity, 1);
    (void)section_data(data, cache_startup.busy_callback_identity, 1);
    auto &map_activation = result->sound_map_activation_constants.emplace();
    map_activation.one_bits = u32(section_data(data, later ? 0x46a0d0 : 0x465030, 4), 0);
    map_activation.ambient_transition_bits = startup.ambient_transition_bits;
    auto &driver_startup = result->sound_driver_startup_operands.emplace();
    driver_startup.one_bits = map_activation.one_bits;
    const auto forward_address = u32(section_data(data, later ? 0x474d08 : 0x46fc48, 4), 0);
    const auto up_address = u32(section_data(data, later ? 0x474d10 : 0x46fc50, 4), 0);
    if (forward_address != (later ? 0x44ac28u : 0x447180u) ||
        up_address != (later ? 0x44ac40u : 0x447198u))
      throw FormatError("Original driver immutable basis forwarding pointers unavailable");
    const auto driver_forward_bytes = section_data(data, forward_address, 12);
    const auto driver_up_bytes = section_data(data, up_address, 12);
    std::copy(driver_forward_bytes.begin(), driver_forward_bytes.end(), driver_startup.basis_forward.begin());
    std::copy(driver_up_bytes.begin(), driver_up_bytes.end(), driver_startup.basis_up.begin());
    auto &listener = result->sound_driver_listener_constants.emplace();
    listener.orientation_epsilon_bits = u32(section_data(data, later ? 0x46a12c : 0x46508c, 4), 0);
    listener.environment_epsilon_bits = u32(section_data(data, later ? 0x46a0ec : 0x46504c, 4), 0);
    const auto listener_default = section_data(data, later ? 0x455cf8 : 0x4513f8, 72);
    std::copy(listener_default.begin(), listener_default.end(), listener.default_environment.begin());
    auto &submit = result->sound_submit_operands.emplace();
    submit.constants.effect_callbacks = later ? 0x455c18 : 0x451318;
    const auto submit_callbacks = section_data(data, submit.constants.effect_callbacks, 32);
    std::copy(submit_callbacks.begin(), submit_callbacks.end(), submit.callback_table.begin());
    auto &promotion = result->sound_promotion_constants.emplace();
    promotion.milliseconds_per_second_bits = u32(section_data(data, later ? 0x46a1dc : 0x46513c, 4), 0);
    promotion.seconds_per_millisecond_bits = u32(section_data(data, later ? 0x46a180 : 0x4650e0, 4), 0);
    promotion.one_bits = u32(section_data(data, later ? 0x46a0d0 : 0x465030, 4), 0);
    auto &create = result->sound_create_constants.emplace();
    const std::array<std::uint32_t,11> create_addresses_11{
        0x447298,0x45120c,0x452044,0x4650c0,0x465028,0x465030,
        0x465048,0x46508c,0x451214,0x465078,0x4650e0};
    const std::array<std::uint32_t,11> create_addresses_15{
        0x44ad40,0x455b0c,0x45695c,0x46a160,0x46a0c8,0x46a0d0,
        0x46a0e8,0x46a12c,0x455b14,0x46a118,0x46a180};
    const std::array<std::uint32_t *,11> create_fields{
        &create.default_extra_gain_bits,&create.minimum_gain_bits,&create.maximum_float_bits,
        &create.rng_scale_bits,&create.unsigned_conversion_correction_bits,&create.one_bits,
        &create.upper_gain_bits,&create.db_exponent_scale_bits,&create.db_exponent_log_bits,
        &create.linear_db_scale_bits,&create.fade_epsilon_bits};
    const auto &create_addresses = later ? create_addresses_15 : create_addresses_11;
    for (std::size_t i = 0; i < create_fields.size(); ++i)
      *create_fields[i] = u32(section_data(data, create_addresses[i], 4), 0);
    auto &admission = result->sound_admission_constants.emplace();
    admission.shared = create;
    admission.occlusion_threshold_bits = u32(section_data(data, later ? 0x46a26c : 0x4651cc, 4), 0);
    auto &construction_operands = result->object_construction_operands.emplace();
    construction_operands.one_bits = create.one_bits;
    construction_operands.sentinel_word = u16(section_data(data, later ? 0x474c24 : 0x46fb64, 2), 0);
    auto &pressure = result->object_pressure_operands.emplace();
    const std::array<std::uint32_t,6> pressure_addresses_11{
        0x4650a4,0x451ff8,0x4650a0,0x46509c,0x465098,0x465094};
    const std::array<std::uint32_t,6> pressure_addresses_15{
        0x46a144,0x456910,0x46a140,0x46a13c,0x46a138,0x46a134};
    const std::array<std::uint32_t *,6> pressure_fields{
        &pressure.main_usage_scale_bits,&pressure.top_limit_bits,&pressure.high_limit_bits,
        &pressure.medium_limit_bits,&pressure.low_limit_bits,&pressure.statistic_usage_scale_bits};
    const auto &pressure_addresses = later ? pressure_addresses_15 : pressure_addresses_11;
    for (std::size_t index = 0; index < pressure_fields.size(); ++index)
      *pressure_fields[index] = u32(section_data(data, pressure_addresses[index], 4), 0);
    auto &component = result->physics_component_construction_constants.emplace();
    component.tick_scale_bits = u32(section_data(data, later ? 0x46a238 : 0x465198, 4), 0);
    auto &rebuild = result->physics_component_rebuild_operands.emplace();
    rebuild.type_remap_address = later ? 0x1d74bc : 0x1d2bfc;
    rebuild.branch_table_address = later ? 0x1d74a4 : 0x1d2be4;
    const auto rebuild_types = section_data(data, rebuild.type_remap_address, 13);
    for (std::size_t index = 0; index < rebuild.type_remap.size(); ++index)
      rebuild.type_remap[index] = std::to_integer<std::uint8_t>(rebuild_types[index]);
    const auto rebuild_targets = section_data(data, rebuild.branch_table_address, 24);
    for (std::size_t index = 0; index < rebuild.branch_targets.size(); ++index)
      rebuild.branch_targets[index] = u32(rebuild_targets, index * 4);
    rebuild.branches = {rebuild.branch_targets[0],rebuild.branch_targets[1],rebuild.branch_targets[2],
        rebuild.branch_targets[3],rebuild.branch_targets[4]};
    auto &looping = result->sound_looping_constants.emplace();
    looping.one_bits = create.one_bits;
    looping.minimum_db_bits = create.minimum_gain_bits;
    looping.game_allocator_method = later ? 0x127210 : 0x125e50;
    (void)section_data(data, looping.game_allocator_method, 1);
    looping.pool_name = original_string(data, later ? 0x45f928 : 0x45ad5c);
    looping.empty_selector = original_string(data, later ? 0x4486a1 : 0x444c21);
    looping.dialogue_selector = original_string(data, later ? 0x45f8d8 : 0x45ad0c);
    const auto class_names = section_data(data, later ? 0x47c7a8 : 0x477578, 216);
    for (std::size_t index = 0; index < looping.class_names.size(); ++index)
      looping.class_names[index] = original_string(data, u32(class_names, index * 4));
    auto &stream = result->sound_driver_stream_constants.emplace();
    auto &master = result->sound_driver_master_gain_constants.emplace();
    master.one_bits = u32(section_data(data, later ? 0x46a0d0 : 0x465030, 4), 0);
    master.logarithm_scale_bits = u32(section_data(data, later ? 0x46a2f8 : 0x465258, 4), 0);
    master.quarter_bits = u32(section_data(data, later ? 0x46a130 : 0x465090, 4), 0);
    const std::array<std::uint32_t,7> stream_addresses_11{
        0x465030,0x46507c,0x2205b5,0x220582,0x45e278,0x45e2b8,0x221665};
    const std::array<std::uint32_t,7> stream_addresses_15{
        0x46a0d0,0x46a11c,0x224eb5,0x224e82,0x4633d8,0x4633b4,0x225f65};
    const std::array<std::uint32_t *,7> stream_fields{
        &stream.one_bits,&stream.stereo_bits,&stream.pcm_queue,&stream.adpcm_queue,
        &stream.pcm_initialize_method,&stream.adpcm_initialize_method,&stream.completion_callback};
    const auto &stream_addresses = later ? stream_addresses_15 : stream_addresses_11;
    for (std::size_t i = 0; i < stream_fields.size(); ++i)
      *stream_fields[i] = u32(section_data(data, stream_addresses[i], 4), 0);
  }
  if (!netgame_addresses.empty()) {
    auto &constants = result->multiplayer_netgame_postprocess_constants.emplace();
    for (std::size_t row = 0; row < 2; ++row)
      for (std::size_t coordinate = 0; coordinate < 3; ++coordinate)
        constants.beaver_creek_targets[row][coordinate] =
            u32(section_data(data, netgame_addresses[row * 3 + coordinate], 4), 0);
    constants.distance_squared_threshold = u32(section_data(data, netgame_addresses[6], 4), 0);
    constants.beaver_creek_x_delta = u32(section_data(data, netgame_addresses[7], 4), 0);
    if (result->image == Image::TitleUpdate15) {
      auto &relic = constants.relic.emplace();
      for (std::size_t row = 0; row < 2; ++row)
        for (std::size_t coordinate = 0; coordinate < 3; ++coordinate)
          relic.targets[row][coordinate] =
              u32(section_data(data, relic_addresses_15[row * 3 + coordinate], 4), 0);
      relic.z_delta = u32(section_data(data, relic_addresses_15[6], 4), 0);
    }
  }
  if (!scoreboard_addresses.empty()) {
    auto &c = result->full_scoreboard_constants.emplace();
    const std::array<std::uint32_t *,12> destinations{
        &c.counter_seconds, &c.fade_scale, &c.stagger_fraction, &c.stagger_scale,
        &c.one, &c.header_rgb, &c.background_alpha, &c.text_scale, &c.text_bias,
        &c.dim_scale, &c.scroll_axis_scale, &c.scroll_pixels};
    for (std::size_t i=0; i<destinations.size(); ++i)
      *destinations[i] = u32(section_data(data, scoreboard_addresses[i], 4), 0);
    const auto exponent = section_data(data, scoreboard_addresses.back(), 8);
    c.fade_exponent = std::uint64_t(u32(exponent,0)) |
                      (std::uint64_t(u32(exponent,4)) << 32);
  }
  if(!scope_addresses.empty()) {
    auto &c=result->unit_scope_camera_constants.emplace();
    const std::array<float *,23> destinations{
      &c.default_fov,&c.packet_minimum,&c.packet_maximum,&c.render_minimum,&c.render_maximum,
      &c.magnified_minimum,&c.magnified_maximum,&c.one,&c.packet_multiplier,
      &c.transition_seconds,&c.frame_dt_multiplier,&c.half,&c.three,&c.five,&c.fifteen,
      &c.six,&c.ten,&c.twenty,&c.two,&c.four,&c.twelve,&c.seven,&c.acceleration_cap};
    for(std::size_t i=0;i<destinations.size();++i)
      *destinations[i]=std::bit_cast<float>(u32(section_data(data,scope_addresses[i],4),0));
    const auto numerator=section_data(data,scope_addresses.back(),8);
    c.reciprocal_numerator=std::bit_cast<double>(std::uint64_t(u32(numerator,0)) |
        (std::uint64_t(u32(numerator,4))<<32));
    auto &death=result->death_camera_constants.emplace();
    const std::array<std::uint32_t,18> death_addresses_11{
      0x465ad0,0x4650c0,0x465080,0x46507c,0x4457bc,0x465640,0x46563c,
      0x465044,0x465068,0x4656a4,0x465030,0x46502c,0x45ac88,0x4650d8,
      0x46504c,0x44aff4,0x44aff8,0x44aff8};
    const std::array<std::uint32_t,18> death_addresses_15{
      0x46ab88,0x46a160,0x46a120,0x46a11c,0x44923c,0x46a6f8,0x46a6f4,
      0x46a0e4,0x46a108,0x46a75c,0x46a0d0,0x46a0cc,0x45f854,0x46a178,
      0x46a0ec,0x44ea9c,0x44eaa0,0x44eaa0};
    const auto &addresses=result->image==Image::TitleUpdate15?death_addresses_15:death_addresses_11;
    const std::array<float *,18> fields{&death.default_fov,&death.random_scale,
      &death.distance_scale,&death.distance_bias,&death.full_turn,&death.pitch_scale,
      &death.pitch_bias,&death.transition_seconds,&death.watch_seconds,&death.attached_seconds,
      &death.one,&death.half,&death.probe_scale,&death.probe_tolerance,&death.epsilon,
      &death.minimum_coordinate,&death.maximum_coordinate,&death.maximum_distance};
    for(std::size_t i=0;i<fields.size();++i)
      *fields[i]=std::bit_cast<float>(u32(section_data(data,addresses[i],4),0));
    auto&observer=result->observer_constants.emplace();observer.scalar=c;
    const bool later=result->image==Image::TitleUpdate15;
    // Complete paired171990/171C70/1720B0 operands, from the frozen scope
    // correspondence. Angular constants are37D540/37D560 initializer inputs.
    for(unsigned i=0;i<6;++i){
      observer.packet_width[i]=std::bit_cast<std::int16_t>(u16(section_data(data,(later?0x475290u:0x4701d0u)+i*2,2),0));
      observer.motion_width[i]=std::bit_cast<std::int16_t>(u16(section_data(data,(later?0x47529cu:0x4701dcu)+i*2,2),0));
      observer.acceleration_caps[i]=std::bit_cast<float>(u32(section_data(data,(later?0x44eab8u:0x44b010u)+i*4,4),0));
    }
    const std::array<std::uint32_t,6> observer_11{0x46504c,0x4650e0,0x465090,0x46524c,0x4457bc,0x465088};
    const std::array<std::uint32_t,6> observer_15{0x46a0ec,0x46a180,0x46a130,0x46a2ec,0x44923c,0x46a128};
    const auto&observer_addresses=later?observer_15:observer_11;
    const std::array<float*,6> observer_fields{&observer.epsilon,&observer.orthogonal_tolerance,
      &observer.quarter,&observer.pi,&observer.full_turn,&observer.velocity_cubic};
    for(unsigned i=0;i<observer_fields.size();++i)
      *observer_fields[i]=std::bit_cast<float>(u32(section_data(data,observer_addresses[i],4),0));
    auto&world=result->observer_world_constants.emplace();
    const auto read_float=[&](std::uint32_t at){return std::bit_cast<float>(u32(section_data(data,at,4),0));};
    // Paired172A50/173EE0 operands and actual37DCC0 initializer input.
    world.maximum_initial_distance=read_float(later?0x45695c:0x452044);
    world.maximum_distance=death.maximum_distance;
    world.aspect=read_float(later?0x46a514:0x465474);
    world.margin=std::bit_cast<float>(std::uint32_t(0x3ca3d70a));
    world.vertical_bias_threshold=read_float(later?0x46a12c:0x46508c);
    world.packet_maximum_seconds=read_float(later?0x44eaac:0x44b004);
    const auto bias=section_data(data,later?0x46aa20:0x465968,8);
    world.vertical_bias_test=std::bit_cast<double>(std::uint64_t(u32(bias,0))|(std::uint64_t(u32(bias,4))<<32));
    const auto read_vector=[&](std::uint32_t pointer){const auto at=u32(section_data(data,pointer,4),0);
      return Vec3{read_float(at),read_float(at+4),read_float(at+8)};};
    world.default_forward=read_vector(later?0x474d08:0x46fc48);
    world.default_up=read_vector(later?0x474d10:0x46fc50);
    world.zero=read_vector(later?0x474d04:0x46fc44);
  }
  if(result->image!=Image::Base) {
    const bool later=result->image==Image::TitleUpdate15;
    const std::uint32_t tokens_address=later?0x455e48:0x451548;
    const std::uint32_t glyphs_address=later?0x47c91c:0x4776ec;
    // Literal complete233CF1 mapping records, owned by the supplied image.
    auto &m=result->full_scoreboard_name_mappings.emplace();
    for(unsigned i=0;i<18;++i) {
      const auto row=section_data(data,tokens_address+i*8,8);
      const auto address=u32(row,0);FullScoreboardNameToken token;
      token.glyph=char16_t(u32(row,4)&0xffffu);
      bool ended=false;
      for(unsigned j=0;j<256;++j) {
        const auto bytes=section_data(data,address+j*2,2);
        const auto c=std::to_integer<unsigned>(bytes[0]) | (std::to_integer<unsigned>(bytes[1])<<8);
        if(!c){ended=true;break;}token.text+=char16_t(c);
      }
      if(!ended)throw FormatError("Unterminated original scoreboard name token");
      m.tokens.push_back(std::move(token));
    }
    for(unsigned i=0;i<131;++i) {
      const auto row=section_data(data,glyphs_address+i*12,12);
      m.glyphs.push_back({u32(row,0),u32(row,4)});
    }
    auto &fallback=result->full_scoreboard_palette_fallback.emplace();
    const auto fallback_pointer=u32(section_data(data,later?0x474c70:0x46fbb0,4),0);
    for(unsigned i=0;i<3;++i)fallback[i]=u32(section_data(data,fallback_pointer+i*4,4),0);
    auto paint=std::make_shared<FullScoreboardPaintConstants>();
    paint->bright_rgb_threshold=u32(section_data(data,later?0x46a438:0x465398,4),0);
    paint->pixel_scale=u32(section_data(data,later?0x46a148:0x4650a8,4),0);
    paint->pulse_phase=u32(section_data(data,later?0x46a2ec:0x46524c,4),0);
    paint->pulse_one=u32(section_data(data,later?0x46a0d0:0x465030,4),0);
    paint->pulse_half=u32(section_data(data,later?0x46a0cc:0x46502c,4),0);
    const auto shadow_pointer=u32(section_data(data,later?0x474c34:0x46fb74,4),0);
    for(unsigned i=0;i<4;++i)paint->shadow[i]=u32(section_data(data,shadow_pointer+i*4,4),0);
    result->full_scoreboard_paint_constants=std::move(paint);
  }
  const std::uint32_t stretch=result->image==OriginalData::Image::Base ? 0x45dc00 :
      result->image==OriginalData::Image::MapPack11 ? 0x465070 : 0x46a110;
  const std::uint32_t mirror=result->image==OriginalData::Image::Base ? 0x45dc0c :
      result->image==OriginalData::Image::MapPack11 ? 0x46507c : 0x46a11c;
  result->hud_bitmap_geometry_constants=HudBitmapGeometryConstants{
      std::bit_cast<float>(u32(section_data(data,stretch,4),0)),
      std::bit_cast<float>(u32(section_data(data,mirror,4),0))};
  return result;
}
} // namespace halo2
