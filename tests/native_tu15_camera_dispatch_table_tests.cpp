#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char**argv)try{
 if(argc>2)throw FormatError("usage: camera-mode-table-test [ORIGINAL_XBE|--dump]");auto m=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=m.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>bytes;for(auto b:r.bytes)bytes.push_back(std::to_integer<unsigned>(b));j.push_back({{"address",r.address},{"bytes",bytes},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 constexpr std::array<unsigned,5>targets{0x15807du,0x15807du,0x1580d0u,0x158089u,0x1580dfu};const auto owner=std::ranges::find(cold.regions,0x158220u,&SourceRuntimeMemory::CheckpointRegion::address);if(owner==cold.regions.end()||owner->bytes.size()!=20||owner->access!=SourceRuntimeMemory::Access::ReadOnly||owner->owner!="native TU1.5 authored five camera mode dispatch targets"||owner->page_bytes||!owner->committed.empty())throw FormatError("camera mode dispatch lost exact20-byte readonly provenance");
 for(unsigned i=0;i<5;++i)if(Bytes(m.view(0x158220,20)).u32(i*4)!=targets[i])throw FormatError("camera mode dispatch target differs from genuine original branch");if(m.native_address(0x158220,20).value<=UINT32_MAX)throw FormatError("camera mode dispatch backing narrowed");
 unsigned negatives{};for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,4>{{{0x15821f,1},{0x158220,21},{0x158234,4},{0x158230,8}}}){bool failed{};try{(void)m.view(p,n);}catch(const std::out_of_range&){failed=true;}if(!failed)throw FormatError("camera dispatch borrowed unsupported mode or adjacent byte");++negatives;}bool immutable{};try{m.validate_mutable_view(0x158220,20);}catch(const std::exception&){immutable=true;}if(!immutable||m.checkpoint()!=cold)throw FormatError("camera dispatch mutation admitted");++negatives;
 if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;const auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);if(!std::ranges::equal(m.view(0x158220,20),original.view(0x158220,20)))throw FormatError("camera dispatch data differs from authenticated TU1.5 image");}
 std::cout<<"PASS exact5camera targets/20readonly bytes, native64backing,"<<negatives<<" owner/immutable negatives; no branch body replacement or completed-camera/GPU claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
