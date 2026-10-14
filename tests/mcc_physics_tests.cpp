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
        Buffer data(0x150);block(data,0xa8,0x110,0x31);
        for(unsigned i=0;i<0x31;++i)data[0x110+i]=std::byte((i*19+0x43)&255);
        const auto owned=halo2::normalize_mcc_physics_model(data,original,canonical);
        equal_bytes(data,0x110,owned,row(owned,0xa8),0x31);
        check(halo2::Bytes(owned).u32(0xa8)==0x31,"MOPP bytecode extent changed");
        Buffer empty(0x110);
        rejects([&]{(void)halo2::normalize_mcc_physics_model(empty,original,0xfffffff0u);});
    }
    {
        Buffer lists(0x1c0);block(lists,0x90,0x110);block(lists,0x98,0x180,2);
        word(lists,0x130,2);word(lists,0x134,0x80000010u);
        // External lists bind the separately owned child bank. Stale inline
        // storage is not a serialized pointer or an active authored child.
        std::fill(lists.begin()+0x138,lists.begin()+0x178,std::byte{0xcc});
        word(lists,0x180,0x12340002u);word(lists,0x188,0x55u);
        word(lists,0x190,0x56780003u);word(lists,0x198,0x77u);
        const auto owned=halo2::normalize_mcc_physics_model(lists,original,canonical);
        const auto list=row(owned,0x90),children=row(owned,0x98);
        check(halo2::Bytes(owned).u32(list+0x10)==2 && halo2::Bytes(owned).u32(list+0x14)==0x80000010u,
              "External list count/capacity changed");
        check(halo2::Bytes(owned).u32(children)==0x12340002u && halo2::Bytes(owned).u32(children+12)==0x77u,
              "External list authored selector/filter changed");
        check(std::ranges::all_of(halo2::Bytes(owned).slice(list+0x18,0x20),[](auto b){return b==std::byte{};}),
              "External list's inactive inline storage was admitted");
    }
    {
        Buffer collision(0x200);block(collision,0x1c,0x40);
        block(collision,0x44,0x50);block(collision,0x54,0x70);block(collision,0x5c,0xc0);
        block(collision,0x70+0x2c,0x1a0);
        for(unsigned at:{0xc0u,0xd0u,0x110u,0x120u,0x130u,0x140u}) {
            word(collision,at,0x12345678u);word(collision,at+4,0xabcdef01u);
        }
        word(collision,0xc0+8,0x80000000u);word(collision,0xc0+0x58,0x80000000u);
        word(collision,0xc0+0x78,0x80000000u);
        for(unsigned i=0;i<0x30;++i)collision[0xc0+0x20+i]=std::byte((i*27+19)&255);
        block(collision,0xc0+0x98,0x180,6);
        for(unsigned i=0;i<6;++i)collision[0x180+i]=std::byte(i*13+7);
        for(unsigned i=0;i<8;++i)collision[0x1a0+i]=std::byte(i*17+3);
        const auto owned=halo2::normalize_mcc_collision_model(collision,original,canonical);
        const auto region=row(owned,0x1c),permutation=row(owned,region+4);
        const auto bsp=row(owned,permutation+4),physics=row(owned,permutation+0xc);
        equal_bytes(collision,0xe0,owned,physics+0x10,0x30);
        for(unsigned offset:{0u,8u,0xcu,0x40u,0x48u,0x4cu,0x50u,0x58u,0x5cu,0x60u,0x6cu})
            check(halo2::Bytes(owned).u32(physics+offset)==0,"Collision retained an unbound MCC tool pointer/padding");
        equal_bytes(collision,0x180,owned,row(owned,physics+0x64),6);
        equal_bytes(collision,0x1a0,owned,row(owned,bsp+0x2c),8);
        auto invalid=collision;word(invalid,0x5c,2);
        rejects([&]{(void)halo2::normalize_mcc_collision_model(invalid,original,canonical);});
        invalid=collision;word(invalid,0,1);
        rejects([&]{(void)halo2::normalize_mcc_collision_model(invalid,original,canonical);});
    }
    {
        Buffer bsp(0x600);block(bsp,0x138,0x240,2);
        for(unsigned definition:{0x240u,0x308u}) {
            block(bsp,definition+0xb0,0x400);
            block(bsp,definition+0x98,0x520);
        }
        word(bsp,0x400,0x12345678u);word(bsp,0x404,0xabcdef01u);
        word(bsp,0x420,0x3fa00000u);block(bsp,0x498,0x500,6);
        for(unsigned i=0;i<6;++i)bsp[0x500+i]=std::byte(i*13+7);
        for(unsigned i=0;i<8;++i)bsp[0x520+i]=std::byte(i*17+3);
        const auto before=bsp;
        const auto owned=halo2::normalize_mcc_structure_bsp_physics(bsp,original);
        check(bsp==before && owned.size()==bsp.size(),"Instanced physics moved or changed its donor allocation");
        equal_bytes(bsp,0,owned,0,0x400);equal_bytes(bsp,0x470,owned,0x470,bsp.size()-0x470);
        check(halo2::Bytes(owned).u32(0x400)==0 && halo2::Bytes(owned).u32(0x410)==0x3fa00000u &&
              halo2::Bytes(owned).u32(0x464)==6 && halo2::Bytes(owned).u32(0x468)==original+0x500,
              "Instanced physics changed authored bounds or its retained MOPP owner");
        auto invalid=bsp;word(invalid,0x240+0xb0,2);
        rejects([&]{(void)halo2::normalize_mcc_structure_bsp_physics(invalid,original);});
        invalid=bsp;word(invalid,0x240+0xb0,0);
        rejects([&]{(void)halo2::normalize_mcc_structure_bsp_physics(invalid,original);});
        invalid=bsp;word(invalid,0x49c,original+0x400);
        rejects([&]{(void)halo2::normalize_mcc_structure_bsp_physics(invalid,original);});
        invalid=bsp;word(invalid,0x240+0xb4,original+0x5ff);
        rejects([&]{(void)halo2::normalize_mcc_structure_bsp_physics(invalid,original);});
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
    std::cout<<"MCC phmo/coll authored record packing, owned geometry/MOPP, nested aliases and pointer-width rejection passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
