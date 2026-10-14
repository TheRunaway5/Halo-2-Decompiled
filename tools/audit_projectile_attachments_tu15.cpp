#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=5)throw FormatError("Supply MAP SHARED TU15 OUTPUT_JSON");
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    nlohmann::json rows=nlohmann::json::array();
    for(const auto &tag:cache.select_tags({.group=tag_groups::projectile,.require_address=true})) {
        const auto meta=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(meta.bytes_at(meta.source_address(),0x9c));
        const auto descriptor=read_tag_block_descriptor(root,0x94);
        const auto extent=descriptor.extent(0x18,4095);
        if(!descriptor.count)continue;
        const auto owner=cache.query_tag({.contains=TagAddressRange{descriptor.address,extent},.require_address=true});
        if(!owner)throw FormatError("Projectile attachment bank has no retained owner");
        (void)cache.tag_payload(owner->handle());
        const Bytes data(cache.read_tag_bytes(descriptor.address,extent));
        for(unsigned n=0;n<unsigned(descriptor.count);++n) {
            const auto at=n*0x18;const auto attachment=cache.query_tag({.handle=TagHandle{data.u32(at+4)}});
            rows.push_back({{"projectile",tag.name},{"datum",tag.datum},{"slot",n},
                {"group",attachment?attachment->group:"NONE"},{"attachment",attachment?attachment->name:"NONE"},
                {"marker",cache.string_id(data.u32(at+8))},{"scale_a",cache.string_id(data.u32(at+0x10))},
                {"scale_b",cache.string_id(data.u32(at+0x14))}});
        }
    }
    std::ofstream(argv[4])<<rows.dump(2)<<'\n';
    std::cout<<"Authored projectile attachment rows="<<rows.size()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
