#include "halo2/source_contrail_snapshot.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>
using namespace halo2;
namespace {
void check(bool b,const char *why){if(!b)throw FormatError(why);}
template<class F>void rejects(F f){bool rejected{};try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Invalid contrail input was accepted");}
void live_math_policy_snapshot() {
 constexpr std::uint32_t policy=0x46ecd0u,error_policy=0x48d4f4u;
 for(const auto value:{0u,0x2694u,0xffffffffu})for(const auto error_value:{0u,1u,0xffffffffu}) {
  SourceRuntimeMemory live;
  const auto put=[&](unsigned p,unsigned v){live.write(p,std::as_bytes(std::span(&v,1)));};
  const auto word=[&](unsigned p,unsigned v){live.mount(p,std::as_bytes(std::span(&v,1)),
   SourceRuntimeMemory::Access::ReadWrite,"Explicit contrail snapshot caller input");};
  word(policy,value);
  word(error_policy,error_value);
  live.mount(0x4f7210u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,
   "Explicit adjacent source point/contrail pool bindings");
  for(const auto [cell,pool,stride]:std::array{
   std::array{0x4f7214u,0x60000000u,72u},std::array{0x4f7210u,0x60001000u,56u},
   std::array{0x4ecbd0u,0x60002000u,12u}}) {
   if(cell==0x4ecbd0u)word(cell,pool);else put(cell,pool);
   live.mount(pool,std::array<std::byte,0x4c>{},SourceRuntimeMemory::Access::ReadWrite,
    "Explicit empty source pool header");
   put(pool+0x20,1);put(pool+0x24,stride);put(pool+0x48,pool+0x80);word(pool+0x80,0);
  }
  word(0x4f0414u,0x60003000u);
  for(const auto [cell,bank]:std::array{std::array{0x474c70u,0x60004000u},std::array{0x474d10u,0x60005000u}}) {
   word(cell,bank);live.mount(bank,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,
    "Explicit contrail color/vector input");
  }
  const auto before=live.checkpoint();const auto captured=capture_source_contrail_snapshot(live);
  check(live.checkpoint()==before,"Contrail capture mutated its actual source inputs");
  put(policy,value^0xffffffffu);
  put(error_policy,error_value^0xffffffffu);
  SourceContrailSnapshotMemory replica(captured,live);std::array<std::byte,4> bytes{};
  replica.read(policy,bytes);
  check(Bytes(bytes).u32(0)==value,
   "Contrail replica must read the captured live CRT policy, not later source or loader state");
  replica.read(error_policy,bytes);
  check(Bytes(bytes).u32(0)==error_value,
   "Contrail replica lost its actual live x87 error-dispatch policy");
  const auto row=std::ranges::find_if(captured.slices,[](const auto& r){return r.address==policy;});
  check(row!=captured.slices.end() && row->bytes.size()==4,
   "Contrail policy capture must exclude adjacent mutable CRT state");
  const auto error_row=std::ranges::find_if(captured.slices,[](const auto& r){return r.address==error_policy;});
  check(error_row!=captured.slices.end() && error_row->bytes.size()==4,
   "Contrail error-policy capture must exclude adjacent mutable CRT state");
  auto missing=captured;
  missing.slices.erase(std::remove_if(missing.slices.begin(),missing.slices.end(),
   [](const auto& r){return r.address==policy || r.address==error_policy;}),missing.slices.end());
  SourceContrailSnapshotMemory omitted(missing,live);
  rejects([&]{omitted.read(policy,bytes);});
  rejects([&]{omitted.read(error_policy,bytes);});
 }
}
}
int main(int argc,char **argv)try {
 if(argc!=2 || !std::filesystem::exists(argv[1]))return 77;
 live_math_policy_snapshot();
 auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);
 SourceContrailSnapshot snapshot{true,{{0x70000100u,{std::byte{0x42},std::byte{0x19},std::byte{0x31},std::byte{0x88}}}}};
 SourceContrailSnapshotMemory memory(snapshot,original);std::array<std::byte,4> bytes{};
 memory.read(0x70000100u,bytes);check(std::ranges::equal(bytes,snapshot.slices.front().bytes),"Actual supplied render-input bytes changed");
 std::array<std::byte,4> pinned{};original.read(0x46a0d0u,pinned);memory.read(0x46a0d0u,bytes);check(bytes==pinned,"Immutable original data fallback changed");
 rejects([&]{memory.read(0x70000101u,bytes);});
 rejects([&]{memory.read(0x474c70u,bytes);});
 rejects([&]{memory.write(0x70000100u,bytes);});
 const auto invalid=[&](auto change){auto value=snapshot;change(value);rejects([&]{validate_source_contrail_snapshot(value);});};
 invalid([](auto&s){s.authoritative=false;});
 invalid([](auto&s){s.slices.front().address=0;});
 invalid([](auto&s){s.slices.front().address=0xfffffffeu;});
 invalid([](auto&s){s.slices.front().bytes.clear();});
 invalid([](auto&s){s.slices.push_back(s.slices.front());});
 invalid([](auto&s){s.slices.front().bytes.resize(2u*1024u*1024u+1u);});
 invalid([](auto&s){s.slices.resize(32769);});
 check(std::ranges::equal(bytes,pinned),"Rejected source input unexpectedly changed the read buffer");
 std::cout<<"PASS contrail snapshot: exact source bytes, pinned immutable data, missing mutable inputs, read-only policy and malformed bounds\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
