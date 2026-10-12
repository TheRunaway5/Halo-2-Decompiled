#include "halo2/rumble.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=2)throw FormatError("Supply MIDSHIP_MAP");
    const Cache cache(argv[1]);unsigned tags{},shared_blocks{},shared_curves{};
    for(const auto& tag:cache.select_tags({.group=tag_groups::damage_effect,.minimum_size=0x74})) {
        const auto loaded=load_rumble_definition(cache,tag.datum);++tags;
        const auto owned=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(owned.bytes_at(owned.source_address(),owned.size()));
        const auto count=root.u32(0x6c),address=root.u32(0x70);
        if(!count)continue;
        shared_blocks+=address<owned.source_address() || std::uint64_t(address)+count*0x4c>std::uint64_t(owned.source_address())+owned.size();
        const Bytes rows(cache.read_tag_bytes(address,count*0x4c));
        for(unsigned n=0;n<count;++n)for(unsigned m=0;m<2;++m) {
            const auto at=n*0x4c+0x24+m*12,size=rows.u32(at+4),curve=rows.u32(at+8);
            if(!size)continue;
            shared_curves+=curve<owned.source_address() || std::uint64_t(curve)+size>std::uint64_t(owned.source_address())+owned.size();
            if(loaded.at(n).motors[m].curve.size()!=size)throw FormatError("Authored rumble curve extent changed");
        }
    }
    if(!tags || !shared_blocks || !shared_curves)throw FormatError("Real shared rumble alias controls missing");
    std::cout<<"PASS "<<tags<<" damage tags / "<<shared_blocks<<" shared response blocks / "<<shared_curves<<" shared curves\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
