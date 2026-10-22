#include "../src/assets/tag_owner_index.hpp"
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
using namespace halo2;
int main() {
    std::mt19937 random(0x20261020);
    std::vector<Tag> tags;
    for(unsigned i=0;i<2048;++i)
        tags.push_back({i%2?"bitm":"weap",i%3?"tag":"alias",i,
            i<1024?i*32:std::uint32_t(random()),i%17?std::uint32_t(random()%512):0});
    tags.push_back({"bitm","edge",2048,0xffffffffu,0});
    tags.push_back({"bitm","edge",2049,0xfffffffeu,2});
    const detail::TagOwnerIndex index(tags);
    for(unsigned i=0;i<16384;++i) {
        TagQuery query{.contains=TagAddressRange{i<4096?i*17:i%3?std::uint32_t(random()):0xffffffffu,
            i%7?random()%128:std::numeric_limits<std::size_t>::max()},
            .require_address=bool(i%2),.minimum_size=i%4?0u:16u};
        if(i%11==0)query.group=tag_groups::bitmap;
        if(i%13==0)query.name="alias";
        if(i%19==0)query.handle=TagHandle{std::uint32_t(random()%tags.size())};
        if(i%23==0)query.address=tags[i%tags.size()].address;
        if(i%29==0)query.contains.reset();
        std::vector<const Tag*> expected;
        for(const auto &tag:tags)if(query.matches(tag))expected.push_back(&tag);
        if(index.select(query)!=expected)throw std::runtime_error("Indexed ownership differs from the complete predicate or original order");
    }
    std::cout<<"16384 indexed queries preserve complete predicates, overlaps, empty ranges, boundaries and directory order\n";
}
