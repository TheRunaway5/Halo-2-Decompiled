#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/source_geometry_resource_request.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    std::span<const std::byte> view(std::uint32_t at,std::size_t size)const {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())
            return std::span(b.bytes).subspan(at-b.address,size);
        throw std::runtime_error("Unmapped NativeRenderLighting read at "+std::to_string(at));
    }
    std::span<std::byte> view(std::uint32_t at,std::size_t size) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())
            return std::span(b.bytes).subspan(at-b.address,size);
        throw std::runtime_error("Unmapped NativeRenderLighting write at "+std::to_string(at));
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        const auto from=view(at,bytes.size());std::copy(from.begin(),from.end(),bytes.begin());
        reads.push_back(J::array({at,bytes.size()}));
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        auto into=view(at,bytes.size());std::copy(bytes.begin(),bytes.end(),into.begin());
        writes.push_back(J::array({at,bytes.size()}));
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        write(at,std::span(bytes).first(size));
    }
};
std::uint32_t get(const Memory& m,std::uint32_t at) {
    std::array<std::byte,4> bytes{};m.read(at,bytes);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
void controls() {
    constexpr auto data=0x1000000u,lm=data+0x1000u,group=data+0x2000u,bsp=data+0x3000u,
        other=data+0x4000u,instances=data+0x5000u,refs=data+0x6000u,buckets=data+0x7000u;
    Memory m;m.banks={{0x4ec000u,std::vector<std::byte>(4096)},{data,std::vector<std::byte>(0x10000)}};
    m.put(0x4ecc14,lm);m.put(lm+0x80,1);m.put(lm+0x84,group);m.put(0x4ecc18,bsp);
    m.put(group+4,0xabcdef);m.put(bsp+8,0xabcdef);m.put(group+0x54,refs);m.put(group+0x44,buckets);
    m.put(bsp+0x144,instances);m.put(other+0x144,instances+0x100);m.put(instances+0x1ae,1,2);
    unsigned requests{};NativeTu15PhysicsFrameFp fp{0x27f,0x220,0x1fa0};
    NativeTu15RenderLightingServices services;
    services.request=[&](auto header,auto& arithmetic) {
        check(header==buckets+0xc,"2CFE0 resource identity changed");++requests;
        m.put(0x4ecc18,other);m.put(lm+0x84,0);arithmetic.status=0x1234;return true;
    };
    check(native_tu15_render_lighting_2cfe0(m,0xffffffffu,0xffffffffu,1,fp,services)==1,
        "2CFE0 pinned its BSP across the resource request");
    check(requests==1&&fp.status==0x1234,"2CFE0 lost callback count or FP state");
    m.put(lm+0x84,group);m.put(0x4ecc18,bsp);
    check(native_tu15_render_lighting_2cfe0(m,0,0xffffffffu,0x7ff,fp,{})==0,
        "2CFE0 sentinel BSP instance requested a bucket");
    check(native_tu15_render_lighting_2cfe0(m,0,0x80010001u,0x7ff,fp,{})==2,
        "2CFE0 sentinel model instance lost class2");
    services.request=[&](auto,auto&){return false;};
    services.object=[&](auto,auto&)->std::uint32_t{throw std::runtime_error("2CFE0 incorrectly fell back after request refusal");};
    check(native_tu15_render_lighting_2cfe0(m,0,0xffffffffu,1,fp,services)==3,
        "2CFE0 refused bucket did not return3");
}
struct Boundary {std::uint32_t entry;};
J run(const J& row) {
    Memory m;for(const auto& bank:row.at("banks")) {
        const auto h=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<h.size();i+=2)bytes.push_back(std::byte(std::stoul(h.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto before=row.at("before_fp").get<std::array<std::uint32_t,3>>();
    NativeTu15PhysicsFrameFp fp{std::uint16_t(before[0]),std::uint16_t(before[1]),before[2]};
    J calls=J::array();std::uint32_t result{};bool stopped{};
    const auto call=[&](auto entry,J arguments){calls.push_back(J{{"entry",entry},{"arguments",std::move(arguments)}});};
    SourceGeometryResourceMemory io;
    io.read=[&](auto at,auto size)->std::span<const std::byte>{m.reads.push_back(J::array({at,size}));return m.view(at,size);};
    io.write=[&](auto at,auto size){m.writes.push_back(J::array({at,size}));return m.view(at,size);};
    SourceGeometryResourceRequestServices resources;
    resources.acquire=[&](auto header,bool blocking,bool selective,std::int32_t priority) {
        call(0x130eb0,J::array({header,std::uint32_t(blocking),std::uint32_t(selective),std::bit_cast<std::uint32_t>(priority)}));
        throw Boundary{0x130eb0};
    };
    resources.promote=[&](auto job,auto priority){call(0x1236a0,J::array({job,priority}));throw Boundary{0x1236a0};};
    resources.yield=[&]{call(0x2dbf74,J::array());throw Boundary{0x2dbf74};};
    resources.pump=[&]{call(0x128870,J::array());throw Boundary{0x128870};};
    NativeTu15RenderLightingServices services;
    services.request=[&](auto header,auto&) {
        call(0x130980,J::array({3,header}));
        return source_geometry_resource_request(header,3,SourceGeometryResourceGlobals::title_update_1_5(),io,resources);
    };
    services.object=[&](auto owner,auto&) {
        call(0xbcdf0,J::array({owner,0xffffffffu}));return native_tu15_object_body_bcdf0(m,owner,0xffffffffu);
    };
    const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    try {
        const auto entry=row.at("entry").get<std::uint32_t>();
        if(entry==0xbcdf0)result=native_tu15_object_body_bcdf0(m,gp[0],row.at("matrix"));
        else if(entry==0x44930)result=native_tu15_render_section_44930(m,gp[0],std::uint8_t(row.at("matrix").get<std::uint32_t>()));
        else result=native_tu15_render_lighting_2cfe0(m,gp[0],gp[2],gp[1],fp,services);
    }catch(const Boundary& b) {
        const auto entries=row.at("stop_before_entries").get<std::vector<std::uint32_t>>();
        check(std::find(entries.begin(),entries.end(),b.entry)!=entries.end(),"2CFE0 reached an undeclared resource service");stopped=true;
    }
    J banks=J::array();const char* digits="0123456789abcdef";
    for(const auto& b:m.banks)if(b.address!=0x62010000u) {
        std::string h;for(auto byte:b.bytes){const auto v=std::to_integer<unsigned>(byte);h+=digits[v>>4];h+=digits[v&15];}
        banks.push_back(J{{"address",b.address},{"after",h}});
    }
    return {{"result",result},{"stopped",stopped},{"calls",calls},{"reads",m.reads},{"writes",m.writes},
        {"banks",banks},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})}};
}
}
int main()try {
    controls();for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
