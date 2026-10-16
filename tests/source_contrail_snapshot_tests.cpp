#include "halo2/source_contrail_snapshot.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/cache.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool b,const char *why){if(!b)throw FormatError(why);}
template<class F>void rejects(F f){bool rejected{};try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Invalid contrail input was accepted");}
}
int main(int argc,char **argv)try {
 if(argc!=2 || !std::filesystem::exists(argv[1]))return 77;
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
