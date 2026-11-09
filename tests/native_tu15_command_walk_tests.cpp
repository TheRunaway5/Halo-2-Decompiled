#include "halo2/native_tu15_command_walk.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+out.size()<=b.bytes.size()) {
            std::copy_n(b.bytes.begin()+at-b.address,out.size(),out.begin());reads.push_back(J::array({at,out.size()}));return;}
        throw std::runtime_error("Unmapped nativeCommandWalk read at "+std::to_string(at));
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+in.size()<=b.bytes.size()) {
            std::copy(in.begin(),in.end(),b.bytes.begin()+at-b.address);writes.push_back(J::array({at,in.size()}));return;}
        throw std::runtime_error("Unmapped nativeCommandWalk write");
    }
    void put(std::uint32_t at,std::uint32_t value) {
        std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));write(at,b);
    }
};
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
std::uint32_t get(const Memory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);return v;
}
void controls() {
    constexpr auto ctx=0x1000000u,geom=ctx+0x4000u,packet=geom+0x100u,unit=0x4c6958u;
    Memory m;m.banks={{0x4c2000u,std::vector<std::byte>(0x1000)},{0x4c6000u,std::vector<std::byte>(0x3000)},
        {0x4cd000u,std::vector<std::byte>(0x2000)},{0x4d2000u,std::vector<std::byte>(0x4000)},
        {0x4f3000u,std::vector<std::byte>(0x1000)},{0x515000u,std::vector<std::byte>(0x1000)},
        {ctx,std::vector<std::byte>(0x8000)}};
    m.put(0x4cd3a0u,ctx);m.put(0x4cd39cu,0xabcd0001u);m.put(0x4cd398u,0xbeef0002u);
    m.put(unit+32,1);m.put(unit+32+12,(31u<<4)|7u);m.put(unit+32+0x18,1u<<16);
    m.put(0x4cd3a4u,0x9988);m.put(0x4cd6f6u,0xff00);m.put(geom,2);
    NativeTu15PhysicsFrameFp fp{0x27f,0x220,0x1fa0};NativeTu15CommandPartServices s;s.packet=packet;
    std::vector<std::uint32_t> calls;
    s.geometry_449d0=[&](auto u,auto owner,auto&){check(u==1 && owner==0,"44AB0 unit/owner narrowed incorrectly");
        m.put(0x4cd398u,0);m.put(0x4cd3a4u,0);m.put(unit+32+12,0);calls.push_back(0x449d0);return geom;};
    s.prepare_45cd0=[&](auto u,auto p,auto matrix,auto&){check(u==1&&matrix==7,"44AB0 failed to pin matrix");
        calls.push_back(p);m.put(packet,0x12345678);if(p==0)m.put(geom,3);return p!=1;};
    s.emit_41030=[&](const auto& a,auto&){
        check(a.mode==1&&a.coverage==0x99&&a.shader==0x12345678&&!a.grouped&&a.unit==1&&a.mask==0xffffffffu&&!a.add,
            "44AB0 lost pinned inputs");
        calls.push_back(0x100+a.part);if(a.part==2)m.put(geom,0xffffffffu);};
    native_tu15_command_parts_44ab0(m,1,0xffffffffu,31,fp,s);
    check(calls==std::vector<std::uint32_t>{0x449d0,0,0x100,1,2,0x102},"44AB0 lost live signed geometry count or call order");
    // The override is pinned before geometry/part services, even at slot255's
    // boundary. It overrides NONE but source still reads the packet shader.
    m.put(0x4cd39cu,1);m.put(unit,1);m.put(unit+12,31u<<4);m.put(unit+0x18,1u<<16);
    m.put(0x4cd6f6u,254);m.put(0x4cda48u+254*4u,0xaabbccdd);m.put(geom,1);
    s.geometry_449d0=[&](auto,auto,auto&){m.put(0x4cda48u+254*4u,0);return geom;};
    s.prepare_45cd0=[&](auto,auto,auto,auto&){m.put(packet,0xffffffffu);return true;};
    unsigned emitted{};s.emit_41030=[&](const auto& a,auto&){check(a.shader==0xaabbccdd,"44AB0 reloaded pinned override");++emitted;};
    native_tu15_command_parts_44ab0(m,0,0xffffffffu,31,fp,s);check(emitted==1,"44AB0 skipped overridden NONE shader");
    // Special rows bypass all ordinary filtering and retain signed counts.
    m.put(unit,0x400);m.put(0x515f1cu,0xffffffffu);
    native_tu15_command_parts_44ab0(m,0,0,0xffffffffu,fp,{});
    check(get(m,0x4d4f5eu,2)==0&&get(m,0x515f1cu)==0,"44AB0 special main signed count");
    // Frame mode/context stay pinned; variant selection is late and reset
    // stores must precede the genuine object-preparation service.
    m.put(ctx+0x2a78,0);m.put(0x4f3218u,geom);m.put(geom+8,0);m.put(0x4ce26cu,7);
    NativeTu15CommandFrameServices f;calls.clear();
    f.prepare_41c70=[&](auto context,auto mode,auto&){check(context==ctx&&!mode&&!get(m,0x4ce26cu),"40E80 preparation before reset");
        m.put(ctx+0x2a78,1);m.put(geom+8,3);m.put(0x4cd39cu,0);calls.push_back(0x41c70);};
    f.producer_179b40=[&](auto&){calls.push_back(0x179b40);};f.producer_42090=[&](auto&){calls.push_back(0x42090);};
    f.producer_42750=[&](auto mask,auto&){check(mask==0x10,"40E80 selector");calls.push_back(0x42750);};
    f.producer_41630=[&](auto&){calls.push_back(0x41630);};
    native_tu15_command_frame_40e80(m,2,fp,f);
    check(calls==std::vector<std::uint32_t>{0x41c70,0x179b40,0x42090,0x42750,0x41630},"40E80 lost pinned mode or late variant ordering");
    check(fp.status==0x220&&fp.mxcsr==0x1fa0,"Integer command kernels changed FP");
}
J run(const J& row) {
    Memory m;
    for(const auto& bank:row.at("banks")) {
        const auto s=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<s.size();i+=2)bytes.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto& c=row.at("before_fp");NativeTu15PhysicsFrameFp fp{c[0],c[1],c[2]};
    J calls=J::array();bool visible{};
    const auto entry=row.at("entry").get<unsigned>();
    if(entry==0x460c0u)visible=native_tu15_command_part_visible_460c0(m,row.at("matrix"),row.at("gp")[0],row.at("gp")[3]);
    else {
        NativeTu15CommandFrameServices s;
        s.parts.geometry_449d0=[&](auto unit,auto owner,auto&){
            calls.push_back(J{{"entry",0x449d0},{"unit",unit},{"owner",owner}});
            const auto at=0x4c6958u+32u*std::uint32_t(std::int32_t(std::int16_t(unit)));
            const auto model=get(m,at+4),packed=get(m,at+12);
            if(model!=0xffffffffu) {
                const auto definition=get(m,get(m,0x4f0414u)+16u*(model&65535u)+8u);
                return get(m,get(m,definition+0x28u)+92u*((packed>>9)&511u)+0x34u);
            }
            const auto bsp=get(m,0x4ecc18u);
            check(!owner,"Physical empty-service input unexpectedly requests a resource");
            return get(m,get(m,bsp+0xa0u)+176u*((packed>>9)&511u)+0x50u);
        };
        s.prepare_41c70=[&](auto context,auto mode,auto&){calls.push_back(J{{"entry",0x41c70},{"context",context},{"mode",mode}});};
        const auto producer=[&](auto at){calls.push_back(J{{"entry",at}});};
        s.producer_179a40=[&](auto&){producer(0x179a40);};s.producer_179b40=[&](auto&){producer(0x179b40);};
        s.producer_1770c0=[&](auto&){producer(0x1770c0);};s.producer_42090=[&](auto&){producer(0x42090);};
        s.producer_42750=[&](auto mask,auto&){calls.push_back(J{{"entry",0x42750},{"mask",mask}});};s.producer_41630=[&](auto&){producer(0x41630);};
        if(entry==0x44ab0)native_tu15_command_parts_44ab0(m,row.at("matrix"),row.at("source"),row.at("group"),fp,s.parts);
        else native_tu15_command_frame_40e80(m,row.at("gp")[0],fp,s);
    }
    J banks=J::array();const char* digits="0123456789abcdef";
    for(const auto& b:m.banks)if(b.address<0x62000000u || b.address>=0x62020000u) {
        std::string h;for(auto byte:b.bytes){const auto v=std::to_integer<unsigned>(byte);h+=digits[v>>4];h+=digits[v&15];}
        banks.push_back(J{{"address",b.address},{"after",h}});
    }
    return {{"visible",visible},{"calls",calls},{"reads",m.reads},{"writes",m.writes},{"banks",banks},{"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try {
    controls();
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
