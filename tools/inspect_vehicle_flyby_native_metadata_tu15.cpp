#include "halo2/cache.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

using namespace halo2;
namespace {
using Json=nlohmann::json;
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}
  return out;
}
std::string digest(std::span<const std::byte> bytes) {
  std::array<std::byte,32> hash;unsigned size{};
  if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(hash.data()),&size,EVP_sha256(),nullptr)!=1||size!=hash.size())
    throw FormatError("Cannot authenticate the complete projectile owner");
  return hex(hash);
}
}
int main(int argc,char** argv) {
  Json report{{"passed",false},{"runtime_sound_binding_changed",false},{"full_vehicle_parity",false}};
  try {
    if(argc!=4)throw FormatError("Supply an MCC map, its maps directory and a report");
    Cache cache(argv[1],argv[2]);
    if(!cache.is_mcc())throw FormatError("Inspect only the actual prepared MCC namespace");
    std::optional<MccSoundRuntimePlan> sound;
    const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u,
        [&](MccSoundRuntimePlan&& plan){sound.emplace(std::move(plan));});
    if(!sound)throw FormatError("Canonical delivery lost its actual sound plan");
    const auto read=[&](std::uint32_t address,std::size_t bytes){
      if(address<sound->address || std::uint64_t(address)-sound->address>sound->bytes.size() ||
         bytes>sound->bytes.size()-(std::uint64_t(address)-sound->address))
        throw FormatError("Native metadata read escapes its real sound allocation");
      return std::span<const std::byte>(sound->bytes).subspan(address-sound->address,bytes);
    };
    const auto row=[&](unsigned block,unsigned stride,unsigned index){
      const auto b=sound->blocks.at(block);
      if(b.count<0 || index>=unsigned(b.count))throw FormatError("Native metadata ordinal escapes its bank");
      return read(b.address+index*stride,stride);
    };
    const auto patch=[&](TagHandle handle,std::size_t bytes)->const MccSoundRuntimePatch&{
      const auto p=std::ranges::find_if(sound->patches,[&](const auto& p){return p.owner==handle&&p.after.size()==bytes;});
      if(p==sound->patches.end())throw FormatError("Native metadata lost its complete projected root");return *p;
    };
    const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
    if(!globals)throw FormatError("Missing actual globals owner");
    const Bytes g(cache.read_tag_bytes(globals->address,0xc8));
    if(g.u32(0xc0)!=1)throw FormatError("Sound globals are not singular");
    const auto sg=cache.read_tag_bytes(g.u32(0xc4),0x2c);
    const auto& cls=patch({Bytes(sg).u32(12)},8);const Bytes class_root(cls.after);
    report["sounds"]=Json::array();
    for(const auto& t:cache.tags()) {
      if(t.group!="snd!" || !t.address ||
         (t.name!="sound\\vehicles\\phantom\\phantom_turr_flyby" &&
          t.name!="sound_remastered\\vehicles\\phantom\\phantom_turr_flyby"))continue;
      const auto& root=patch(t.handle(),20);const Bytes s(root.after);
      const auto c=std::to_integer<unsigned>(root.after[2]);
      if(c>=class_root.u32(0))throw FormatError("Native sound class escapes its complete owner");
      Json value{{"name",t.name},{"datum",t.datum},{"native_sound_hex",hex(root.after)},
          {"flags",s.u16(0)},{"sound_class",c},
          {"codec",{std::to_integer<unsigned>(root.after[3]),std::to_integer<unsigned>(root.after[4]),std::to_integer<unsigned>(root.after[5])}},
          {"maximum_play_time",s.u32(16)},
          {"playback_hex",hex(row(0,56,s.u16(6)))},{"scale_hex",hex(row(1,20,std::to_integer<unsigned>(root.after[11])))},
          {"sound_class_hex",hex(read(class_root.u32(4)+c*92u,92))},{"pitch_ranges",Json::array()}};
      for(unsigned n=0;n<std::to_integer<unsigned>(root.after[10]);++n) {
        const auto raw=row(4,12,s.u16(8)+n);const Bytes r(raw);
        Json range{{"name",cache.string_id(Bytes(row(2,4,r.u16(0))).u32(0))},
                   {"native_range_hex",hex(raw)},{"interval_hex",hex(row(3,10,r.u16(2)))},{"permutations",Json::array()}};
        for(unsigned p=0;p<r.u16(10);++p) {
          const auto bytes=row(5,16,r.u16(8)+p);const Bytes permutation(bytes);
          range["permutations"].push_back({{"ordinal",p},
              {"name",cache.string_id(Bytes(row(2,4,permutation.u16(0))).u32(0))},
              {"chance_and_copy_hex",hex(bytes.subspan(2,6))},{"native_encoded_bytes",permutation.u32(8)},
              {"native_permutation_hex",hex(bytes)}});
        }
        value["pitch_ranges"].push_back(std::move(range));
      }
      report["sounds"].push_back(std::move(value));
    }
    if(report["sounds"].empty())throw FormatError("No actual Phantom flyby metadata was inspected");
    // Inspect the actual typed reference, including other users of the same
    // donor. Replacing a shared sound globally would also alter those users.
    report["projectile_references"]=Json::array();
    unsigned ap_owners{};
    for(const auto& t:cache.tags()) {
      if(t.group!="proj"||!t.address||t.size<320)continue;
      const auto bytes=cache.read_tag_bytes(t.address,t.size);const Bytes p(bytes);
      const auto target=TagHandle{p.u32(316)};
      const bool ap=t.name=="objects\\vehicles\\c_turret_ap\\weapon\\plasma_turret_bolt";
      const bool inspected=std::ranges::any_of(report["sounds"],[&](const auto& s){return s.at("datum")==target.value;});
      if(!ap&&!inspected)continue;
      const auto* sound_tag=cache.find_tag(target,tag_groups::sound);
      if(p.u32(312)!=tag_groups::sound.value||!sound_tag||!sound_tag->address)
        throw FormatError("Projectile flyby lost its exact full-salt sound owner");
      report["projectile_references"].push_back({{"owner",t.name},{"datum",t.datum},
          {"source_root",t.address},{"source_bytes",t.size},{"source_sha256",digest(bytes)},
          {"reference_offset",312},{"reference_hex",hex(bytes.subspan(312,8))},
          {"target",sound_tag->name},{"target_datum",target.value},{"authored_ap_owner",ap}});
      ap_owners+=ap;
    }
    if(ap_owners!=1)throw FormatError("Expected one complete authored AP projectile owner");
    constexpr std::uint32_t diagnostic=0x90c605c8u;
    report["coax_failure_address"]={{"address",diagnostic},{"nearby_tags",Json::array()},
        {"regions",Json::array()},{"relocations",Json::array()}};
    const auto& directory=delivery.index();const Bytes index(directory.region.bytes);
    for(unsigned n=0;n<directory.count;++n) {
      const auto at=directory.table-directory.region.address+n*16u;
      const auto address=index.u32(at+8),bytes=index.u32(at+12);
      const auto handle=TagHandle{index.u32(at+4)};
      if(address&&std::int64_t(diagnostic)-address>=-256&&std::int64_t(diagnostic)-address<std::int64_t(bytes)+256) {
        const auto* t=cache.find_tag(handle);
        report["coax_failure_address"]["nearby_tags"].push_back({{"address",address},{"bytes",bytes},
            {"datum",handle.value},{"name",t?t->name:""},{"group",t?t->group:""}});
      }
    }
    for(const auto& region:delivery.tag_image().regions)
      report["coax_failure_address"]["regions"].push_back({{"address",region.address},{"bytes",region.bytes.size()},
          {"contains",diagnostic>=region.address&&std::uint64_t(diagnostic)-region.address<region.bytes.size()}});
    for(const auto& r:delivery.relocations())
      if(std::int64_t(diagnostic)-r.target>=-256&&std::int64_t(diagnostic)-r.target<std::int64_t(r.bytes)+256)
        report["coax_failure_address"]["relocations"].push_back({{"source",r.source},{"target",r.target},{"bytes",r.bytes}});
    if(sound->authored_flags)for(const auto& owner:sound->authored_flags->owners)
      report["metadata_owners"].push_back({{"gestalt",owner.gestalt.value},
          {"authenticated",owner.status==MccSoundVanillaOwnerStatus::Authenticated},{"reason",owner.reason}});
    report["passed"]=true;
  } catch(const std::exception& error){report["error"]=error.what();}
  if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}
  std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
