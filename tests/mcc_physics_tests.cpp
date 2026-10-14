#include "halo2/mcc_physics.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <iostream>

namespace {
using Buffer=std::vector<std::byte>;
constexpr std::uint32_t original=0x200000,canonical=0x300000;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(Buffer& b,std::size_t at,std::uint32_t v){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(v>>(8*i));}
void block(Buffer& b,unsigned descriptor,unsigned offset,unsigned count=1) {
    word(b,descriptor,count);word(b,descriptor+4,original+offset);
}
template<class F>void rejects(F action) {
    try{action();}catch(const halo2::FormatError&){return;}
    throw std::runtime_error("Unverified MCC physics backing was admitted");
}
std::size_t row(const Buffer& b,unsigned descriptor) {
    return halo2::Bytes(b).u32(descriptor+4)-canonical;
}
void equal_bytes(const Buffer& a,std::size_t from,const Buffer& b,std::size_t to,std::size_t count) {
    check(std::ranges::equal(halo2::Bytes(a).slice(from,count),halo2::Bytes(b).slice(to,count)),
          "Authored physics words changed during layout conversion");
}
}
int main()try {
    const std::array<unsigned,11> descriptors{0x38,0x48,0x50,0x58,0x60,0x68,0x70,0x90,0x98,0xa0,0x108};
    const std::array<unsigned,11> sizes{0xa0,0xa0,0xc0,0x60,0xb0,0x70,0x120,0x68,0x10,0x28,0x40};
    const std::array<unsigned,11> strides{0x90,0x80,0xb0,0x50,0x90,0x60,0x100,0x38,8,0x14,0x20};
    Buffer source(0x1000);std::array<unsigned,11> source_rows{};unsigned next=0x110;
    for(unsigned i=0;i<descriptors.size();++i) {
        source_rows[i]=next;block(source,descriptors[i],next);
        for(unsigned p=0;p<sizes[i];++p)source[next+p]=std::byte((p*37+i*11)&255);
        next=(next+sizes[i]+15)&~15u;
    }
    word(source,source_rows[0]+0x3c,0);
    // Four inline children, including their filters. Runtime pointer pads are
    // zero in serialized selectors, even when the irrelevant vtables are not.
    word(source,source_rows[7]+0x20,4);word(source,source_rows[7]+0x24,0x80000004u);
    for(unsigned i=0;i<4;++i)word(source,source_rows[7]+0x2c+i*16,0);
    word(source,source_rows[8]+4,0);word(source,source_rows[9]+0x1c,0);
    word(source,source_rows[10]+0x1c,0);word(source,source_rows[10]+0x38,1);word(source,source_rows[10]+0x3c,0);
    const auto before=source;
    const auto converted=halo2::normalize_mcc_physics_model(source,original,canonical);
    check(source==before,"Physics conversion changed the donor allocation");
    for(unsigned i=0;i<descriptors.size();++i) {
        check(halo2::Bytes(converted).u32(descriptors[i])==1,"Physics block count changed");
        const auto at=row(converted,descriptors[i]);
        check(at>=0x110 && at%16==0 && strides[i]<=converted.size()-at,"Physics block lost owned aligned backing");
    }
    const auto body=row(converted,0x38);
    equal_bytes(source,source_rows[0],converted,body,0x3c);
    equal_bytes(source,source_rows[0]+0x40,converted,body+0x3c,4);
    equal_bytes(source,source_rows[0]+0x50,converted,body+0x40,0x50);
    for(unsigned i=1;i<=6;++i) {
        const auto at=row(converted,descriptors[i]);
        equal_bytes(source,source_rows[i],converted,at,0x20);
        check(halo2::Bytes(converted).u32(at+0x20)==0 && halo2::Bytes(converted).u32(at+0x28)==0,
              "MCC tool pointer was narrowed into a native physics definition");
        equal_bytes(source,source_rows[i]+0x28,converted,at+0x24,4);
        equal_bytes(source,source_rows[i]+0x38,converted,at+0x2c,4);
    }
    equal_bytes(source,source_rows[1]+0x60,converted,row(converted,0x48)+0x40,0x40);
    equal_bytes(source,source_rows[2]+0x40,converted,row(converted,0x50)+0x30,0x80);
    equal_bytes(source,source_rows[3]+0x40,converted,row(converted,0x58)+0x30,0x20);
    equal_bytes(source,source_rows[4]+0x40,converted,row(converted,0x60)+0x30,0x10);
    equal_bytes(source,source_rows[4]+0x70,converted,row(converted,0x60)+0x50,0x40);
    equal_bytes(source,source_rows[5]+0x40,converted,row(converted,0x68)+0x30,0x30);
    equal_bytes(source,source_rows[6]+0x40,converted,row(converted,0x70)+0x30,0x20);
    equal_bytes(source,source_rows[6]+0x68,converted,row(converted,0x70)+0x54,8);
    equal_bytes(source,source_rows[6]+0x70,converted,row(converted,0x70)+0x60,0x90);
    equal_bytes(source,source_rows[6]+0x100,converted,row(converted,0x70)+0xf0,4);
    equal_bytes(source,source_rows[6]+0x110,converted,row(converted,0x70)+0xf8,8);
    for(unsigned i=0;i<4;++i) {
        equal_bytes(source,source_rows[7]+0x28+i*16,converted,row(converted,0x90)+0x18+i*8,4);
        equal_bytes(source,source_rows[7]+0x30+i*16,converted,row(converted,0x90)+0x1c+i*8,4);
    }
    equal_bytes(source,source_rows[8],converted,row(converted,0x98),4);
    equal_bytes(source,source_rows[8]+8,converted,row(converted,0x98)+4,4);
    equal_bytes(source,source_rows[9]+0x18,converted,row(converted,0xa0)+0xc,4);
    check(halo2::Bytes(converted).u32(row(converted,0x108)+0x1c)==1,"Authored phantom user data was cleared");
    {
        Buffer nested(0x200);block(nested,0xc0,0x110,2);
        word(nested,0x114,1);word(nested,0x118,0x700000);
        word(nested,0x120,1);word(nested,0x124,0x700000);
        Buffer external(0x20);word(external,4,2);word(external,8,0x700010);
        external[0x10]=std::byte{3};external[0x12]=std::byte{9};
        const auto read=[&](std::uint32_t p,std::size_t n) {
            if(p<0x700000 || std::uint64_t(p)-0x700000+n>external.size())throw halo2::FormatError("External test owner missing");
            return std::span<const std::byte>(external).subspan(p-0x700000,n);
        };
        rejects([&]{(void)halo2::normalize_mcc_physics_model(nested,original,canonical);});
        auto owned=halo2::normalize_mcc_physics_model(nested,original,canonical,read);
        const auto regions=row(owned,0xc0),permutations=row(owned,regions+4),indices=row(owned,permutations+4);
        check(halo2::Bytes(owned).u32(regions+8)==halo2::Bytes(owned).u32(regions+20),"Shared physics bank alias changed");
        external.assign(external.size(),std::byte{});
        check(halo2::Bytes(owned).u16(indices)==3 && halo2::Bytes(owned).u16(indices+2)==9,
              "Normalized physics retained an unowned donor alias");
    }
    {
        auto invalid=source;word(invalid,source_rows[0]+0x3c,1);
        rejects([&]{(void)halo2::normalize_mcc_physics_model(invalid,original,canonical);});
        invalid=source;word(invalid,source_rows[10]+0x3c,1);
        rejects([&]{(void)halo2::normalize_mcc_physics_model(invalid,original,canonical);});
        invalid=source;word(invalid,0x3c,original+source.size()-1);
        rejects([&]{(void)halo2::normalize_mcc_physics_model(invalid,original,canonical);});
        invalid=source;word(invalid,0xd0,1);
        rejects([&]{(void)halo2::normalize_mcc_physics_model(invalid,original,canonical);});
        rejects([&]{(void)halo2::normalize_mcc_physics_model(source,original,0xfffffff0u);});
    }
    std::cout<<"MCC phmo authored record packing, owned nested aliases and pointer-width rejection passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
