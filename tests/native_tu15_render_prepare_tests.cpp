#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/source_geometry_resource_request.hpp"
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
        throw std::runtime_error("Unmapped nativeRenderPrepare read at "+std::to_string(at));
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+in.size()<=b.bytes.size()) {
            std::copy(in.begin(),in.end(),b.bytes.begin()+at-b.address);writes.push_back(J::array({at,in.size()}));return;}
        throw std::runtime_error("Unmapped nativeRenderPrepare write");
    }
    void put(std::uint32_t at,std::uint32_t value) {
        std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));write(at,b);
    }
    std::span<std::byte> view(std::uint32_t at,std::size_t size) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())
            return std::span(b.bytes).subspan(at-b.address,size);
        throw std::runtime_error("Unmapped declared resource span");
    }
};
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
std::uint32_t get(const Memory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(bytes[i])<<(8*i);return v;
}
void controls() {
    constexpr auto data=0x1000000u,ctx=data,pool=data+0x1000u,rows=data+0x2000u,
        other_pool=data+0x3000u,other_rows=data+0x4000u,body=data+0x5000u,queue=data+0x6000u;
    Memory m;m.banks={{data,std::vector<std::byte>(0x10000)},{0x4ec000,std::vector<std::byte>(4096)}};
    m.put(0x4ecbd0,pool);m.put(pool+0x44,rows);m.put(other_pool+0x44,other_rows);
    for(unsigned i=0;i<4;++i) {
        m.put(rows+12*i+8,body+0x100*i);m.put(other_rows+12*i+8,body+0x100*i);
        m.put(body+0x100*i+0xc,0xffffffffu);m.put(body+0x100*i+0x10,0xffffffffu);
    }
    m.put(other_rows+8,body+0x400); // root's already pinned body must not change
    m.put(ctx+0x14,queue);m.put(queue+4,1);m.put(queue+8,queue+0x100);m.put(queue+12,queue+0x200);
    m.put(queue+0x100,0x8001);m.put(queue+0x200,0xabcd0000);
    m.put(queue+0x304,2);m.put(queue+0x308,queue+0x400);m.put(queue+0x30c,queue+0x500);
    m.put(queue+0x402,0xfffe);m.put(queue+0x504,0xfedc0003);
    std::vector<std::array<std::uint32_t,3>> calls;
    NativeTu15PhysicsFrameFp fp{0x27f,0x220,0x1fa0};NativeTu15RenderPrepareServices s;
    s.update_d7230=[&](auto object,auto mode,auto flags,auto&){
        check(mode==0x10001u,"41C70 narrowed pinned mode");calls.push_back({object,mode,flags});
        if(object==0xabcd0000u) {
            m.put(0x4ecbd0,other_pool);m.put(body+0x10,0xb0000001);m.put(body+0xc,0xdead0003);
            m.put(ctx+0x14,queue+0x300);
        }
        if(object==0xb0000001u)m.put(body+0xc,0xc0000002);
    };
    s.attached_41970=[&](auto mode,auto object,auto&){check(mode==0x10001u,"41C10 lost mode");calls.push_back({object,0,0});};
    native_tu15_render_prepare_41c70(m,ctx,0x10001u,fp,s);
    check(calls==std::vector<std::array<std::uint32_t,3>>{
        {0xabcd0000u,0x10001u,0xffff8001u},{0xabcd0000u,0,0},
        {0xb0000001u,0x10001u,0xffff8001u},{0xb0000001u,0,0},
        {0xc0000002u,0x10001u,0xffff8001u},{0xc0000002u,0,0},
        {0xfedc0003u,0x10001u,0xfffffffeu},{0xfedc0003u,0,0}},
        "41C70/41C10 lost live queue, pinned body, late child/sibling, datum or signed flags");
    check(fp.status==0x220&&fp.mxcsr==0x1fa0,"Integer preparation changed FP");
    // Geometry remains tied to the original section across resource callbacks.
    m.banks.push_back({0x4c6000,std::vector<std::byte>(0x2000)});m.banks.push_back({0x4f0000,std::vector<std::byte>(4096)});
    m.put(0x4c695c,0x12340000);m.put(0x4f0414,rows);m.put(rows+8,body);m.put(body+0x28,queue);m.put(queue+0x34,0);
    const auto value=native_tu15_render_geometry_449d0(m,0,1,0,fp,[&](auto header,auto&){
        check(header==queue+0x38,"449D0 requested wrong model header");m.put(body+0x28,0);m.put(queue+0x34,0x76543210);return true;});
    check(value==0x76543210,"449D0 did not reload its pinned section after resource request");
    m.put(body+0x28,queue);m.reads.clear();
    check(native_tu15_render_geometry_449d0(m,0,1,0,fp,[](auto,auto&){return false;})==0,"449D0 accepted a refused resource");
    for(const auto& r:m.reads)check(r[0]!=queue+0x34,"449D0 read geometry after refusal");
}
J run(const J& row) {
    Memory m;for(const auto& bank:row.at("banks")) {
        const auto h=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<h.size();i+=2)bytes.push_back(std::byte(std::stoul(h.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto& c=row.at("before_fp");NativeTu15PhysicsFrameFp fp{c[0],c[1],c[2]};J calls=J::array();std::uint32_t result{};
    if(row.at("entry")==0x449d0) {
        result=native_tu15_render_geometry_449d0(m,row.at("gp")[0],std::uint8_t(row.at("gp")[2].get<unsigned>()),
            std::uint8_t(row.at("matrix").get<unsigned>()),fp,[&](auto header,auto&){
                calls.push_back(J{{"entry",0x130980},{"header",header},{"flags",3}});
                // Reuse the existing typed resource kernel for declared paths
                // needing no acquisition/promotion/completion service. Genuine
                // physical130980/22A680 supply all gold outcomes unmodified.
                SourceGeometryResourceMemory memory;
                memory.read=[&](auto at,auto size)->std::span<const std::byte>{m.reads.push_back(J::array({at,size}));return m.view(at,size);};
                memory.write=[&](auto at,auto size){m.writes.push_back(J::array({at,size}));return m.view(at,size);};
                constexpr auto globals=SourceGeometryResourceGlobals::title_update_1_5();
                return source_geometry_resource_request(header,3,globals,memory,{});
            });
    } else {
        NativeTu15RenderPrepareServices s;
        s.update_d7230=[&](auto object,auto mode,auto flags,auto&){calls.push_back(J{{"entry",0xd7230},{"object",object},{"mode",mode},{"flags",flags}});};
        s.attached_41970=[&](auto mode,auto object,auto&){calls.push_back(J{{"entry",0x41970},{"mode",mode},{"object",object}});};
        if(row.at("entry")==0x41c70)native_tu15_render_prepare_41c70(m,row.at("gp")[7],row.at("gp")[3],fp,s);
        else native_tu15_render_object_prepare_41c10(m,row.at("matrix"),row.at("source"),row.at("flags"),fp,s);
    }
    J banks=J::array();const char* digits="0123456789abcdef";
    for(const auto& b:m.banks)if(b.address<0x62000000u || b.address>=0x62020000u) {
        std::string h;for(auto byte:b.bytes){const auto v=std::to_integer<unsigned>(byte);h+=digits[v>>4];h+=digits[v&15];}
        banks.push_back(J{{"address",b.address},{"after",h}});
    }
    return {{"result",result},{"calls",calls},{"reads",m.reads},{"writes",m.writes},{"banks",banks},{"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try {
    controls();for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
