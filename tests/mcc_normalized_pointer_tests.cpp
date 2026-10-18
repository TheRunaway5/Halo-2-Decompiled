#include "halo2/cache.hpp"
#include "halo2/mcc_animation.hpp"
#include "halo2/mcc_physics.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
constexpr std::uint32_t donor=0x200000,canonical=0x300000,target=0x80100000;
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void word(Buffer& data,std::size_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)data.at(at+n)=std::byte(value>>(8*n));}
void block(Buffer& data,std::size_t at,std::size_t offset,unsigned count=1){word(data,at,count);word(data,at+4,donor+offset);}
template<class F>void rejects(F action){try{action();}catch(const FormatError&){return;}throw std::runtime_error("Invalid normalized graph admitted");}
template<class Enumerate>void relocated_graph(const Buffer& original,Enumerate enumerate,
    std::uint32_t source=canonical,bool invalid_controls=true) {
    const auto fields=enumerate(original,source);
    std::set<std::uint32_t> words;
    auto relocated=original;
    for(const auto& field:fields) {
        check(words.insert(field.offset).second,"Normalized pointer word was emitted twice");
        check(Bytes(original).u32(field.offset)==field.address,"Pointer field did not name its serialized word");
        check(field.address>=source && std::uint64_t(field.address)-source+field.bytes<=original.size(),"Pointer extent escaped its normalized owner");
        word(relocated,field.offset,target+(field.address-source));
    }
    const auto result=enumerate(relocated,target);
    check(result.size()==fields.size(),"Relocation changed graph topology");
    for(std::size_t n=0;n<fields.size();++n)
        check(result[n].offset==fields[n].offset && result[n].bytes==fields[n].bytes &&
              result[n].address==target+(fields[n].address-source),"Relocated pointer or extent differs");
    for(std::size_t n=0;n<original.size();++n)if(!words.contains(std::uint32_t(n&~std::size_t(3))))
        check(original[n]==relocated[n],"Relocation changed an authored scalar, selector, resource offset or opaque byte");
    if(invalid_controls) {
        for(const auto& field:fields) {
            auto broken=original;word(broken,field.offset,source+std::uint32_t(original.size()));
            rejects([&]{(void)enumerate(broken,source);});
        }
        if(!fields.empty()) {
            auto broken=original;word(broken,fields.front().offset-4,0);
            rejects([&]{(void)enumerate(broken,source);});
        }
        rejects([&]{(void)enumerate(original,0xfffffff0u);});
    }
}
void physics() {
    Buffer input(0x600);
    block(input,0x30,0x110);block(input,0x118,0x130,2);
    block(input,0xc0,0x150,2);
    block(input,0x154,0x180);block(input,0x160,0x180); // one shared permutation
    block(input,0x184,0x1a0,2);input[0x1a0]=std::byte{9};input[0x1a2]=std::byte{3};
    block(input,0xe0,0x1b0);block(input,0x1b8,0x1d0,2);
    block(input,0xa8,0x200,17);
    block(input,0x38,0x220); // 0xa0 donor, 0x90 canonical rigid body
    word(input,0x258,donor+0x200); // a selector which resembles an address
    word(input,0x25c,0);
    for(unsigned n=0;n<17;++n)input[0x200+n]=std::byte(n*13+7);
    const auto normalized=normalize_mcc_physics_model(input,donor,canonical);
    const auto fields=normalized_mcc_physics_pointer_fields(normalized,canonical);
    check(fields.size()==10,"Physics descriptor map missed a root or nested field");
    const auto regions=Bytes(normalized).u32(0xc4)-canonical;
    check(Bytes(normalized).u32(regions+8)==Bytes(normalized).u32(regions+20),"Shared normalized permutation alias changed");
    const auto rigid=Bytes(normalized).u32(0x3c)-canonical;
    check(Bytes(normalized).u32(rigid+0x38)==donor+0x200,"Authored shape selector changed");
    relocated_graph(normalized,normalized_mcc_physics_pointer_fields);
    auto malformed=normalized;word(malformed,0xd0,1);
    rejects([&]{(void)normalized_mcc_physics_pointer_fields(malformed,canonical);});
}
void collision() {
    Buffer input(0x300);
    block(input,0x1c,0x40);block(input,0x44,0x60);
    block(input,0x64,0x80);block(input,0x6c,0x100);
    block(input,0x80+0x2c,0x1c0,2);block(input,0x100+0x98,0x1e0,11);
    for(unsigned n=0;n<11;++n)input[0x1e0+n]=std::byte(n*29+17);
    const auto normalized=normalize_mcc_collision_model(input,donor,canonical);
    const auto fields=normalized_mcc_collision_pointer_fields(normalized,canonical);
    check(fields.size()==6,"Collision descriptor map missed a nested bank");
    const auto region=Bytes(normalized).u32(0x20)-canonical;
    const auto permutation=Bytes(normalized).u32(region+8)-canonical;
    const auto physics=Bytes(normalized).u32(permutation+0x10)-canonical;
    check(std::ranges::any_of(fields,[&](const auto& field){return field.offset==physics+0x68&&field.bytes==11;}),"Collision MOPP descriptor used the donor 0x9c offset");
    relocated_graph(normalized,normalized_mcc_collision_pointer_fields);
}
void animation() {
    Buffer input(0x500);
    block(input,0x2c,0xc0,2);
    input[0xc0+0x16]=std::byte{0xe5};input[0x120+0x16]=std::byte{0x0a};
    block(input,0xc0+0x28,0x240,7);block(input,0x120+0x28,0x250,3);
    block(input,0xc0+0x40,0x280,2);block(input,0x120+0x40,0x280,2);
    block(input,0xc0+0x48,0x290);block(input,0x120+0x58,0x2a0);
    block(input,0x34,0x300);block(input,0x304,0x320);block(input,0x324,0x360);
    block(input,0x364,0x3a0); // action rows nested below mode/weapon/type
    block(input,0x9c,0x400,5);block(input,0xa4,0x420);
    for(unsigned n=0;n<7;++n)input[0x240+n]=std::byte(n*11+3);
    const auto normalized=normalize_mcc_animation_graph(input,donor,canonical,0x80000);
    const auto fields=normalized_mcc_animation_pointer_fields(normalized,canonical);
    check(fields.size()==12,"Canonical animation descriptor map missed shifted clips or the resource table");
    const auto clips=Bytes(normalized).u32(0x30)-canonical;
    for(unsigned n=0;n<2;++n)
        check((std::to_integer<unsigned>(normalized[clips+n*0x6c+0x16])&0x10)!=0,
            "Converted animation selected the null inline resource instead of its owned group");
    check(normalized[clips+0x16]==std::byte{0xf5} && normalized[clips+0x6c+0x16]==std::byte{0x1a},
        "Resource-group conversion changed another authored animation flag");
    const auto groups=Bytes(normalized).u32(0xb0)-canonical;
    for(unsigned n=0;n<2;++n)
        check(Bytes(normalized).u32(groups+n*20+0xc)==0xffffffffu,
            "Converted animation contains an unallocated runtime tracking identity");
    check(std::ranges::any_of(fields,[&](const auto& field){return field.offset==clips+0x50&&field.bytes==8;}),"Canonical event descriptor did not shift from donor offset 0x44");
    check(Bytes(normalized).u32(clips+0x50)==Bytes(normalized).u32(clips+0x6c+0x50),"Canonical clip event alias changed");
    check(std::ranges::any_of(fields,[](const auto& field){return field.offset==0xb0&&field.bytes==40;}),"Added canonical resource-group table was omitted");
    relocated_graph(normalized,normalized_mcc_animation_pointer_fields);
    auto malformed=normalized;word(malformed,0xac,1);
    rejects([&]{(void)normalized_mcc_animation_pointer_fields(malformed,canonical);});
}
}
int main(int argc,char** argv)try{
    physics();collision();animation();
    unsigned actual=0,fields=0;
    if(argc==2) {
        Cache cache(argv[1]);check(cache.is_mcc(),"Actual graph fixture must be an MCC cache");
        for(const auto& tag:cache.tags())if(tag.address&&tag.size&&
            (tag.group=="phmo"||tag.group=="coll"||tag.group=="jmad")) {
            const auto bytes=cache.read_tag_bytes(tag.address,tag.size);
            const Buffer owned(bytes.begin(),bytes.end());
            const auto enumerate=tag.group=="phmo"?normalized_mcc_physics_pointer_fields:
                tag.group=="coll"?normalized_mcc_collision_pointer_fields:normalized_mcc_animation_pointer_fields;
            fields+=unsigned(enumerate(owned,tag.address).size());
            relocated_graph(owned,enumerate,tag.address,false);++actual;
        }
        check(actual&&fields,"Actual MCC fixture had no normalized graphs or references");
    }
    std::cout<<"{\"passed\":true,\"normalized_fixture_graphs\":3,\"actual_MCC_graphs\":"<<actual
        <<",\"actual_MCC_pointer_fields\":"<<fields<<",\"authored_nonpointer_bytes_preserved\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
