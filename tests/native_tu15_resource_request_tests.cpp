#include "halo2/source_geometry_resource_request.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/cache.hpp"
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
        throw std::runtime_error("Unmapped nativeResourceRequest read at "+std::to_string(at));
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+in.size()<=b.bytes.size()) {
            std::copy(in.begin(),in.end(),b.bytes.begin()+at-b.address);writes.push_back(J::array({at,in.size()}));return;}
        throw std::runtime_error("Unmapped nativeResourceRequest write");
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
SourceGeometryResourceMemory adapter(Memory& m) {
    SourceGeometryResourceMemory io;
    io.read=[&](auto at,auto size)->std::span<const std::byte>{m.reads.push_back(J::array({at,size}));return m.view(at,size);};
    io.write=[&](auto at,auto size){m.writes.push_back(J::array({at,size}));return m.view(at,size);};return io;
}
void controls() {
    constexpr auto data=0x1000000u,h=data+0x100u,frame=data+0x1000u,meta=data+0x2000u,records=data+0x3000u,
        flight=data+0x4000u,rows=data+0x5000u,tags=data+0x6000u,other=data+0x7000u,third=data+0x8000u;
    Memory m;m.banks={{data,std::vector<std::byte>(0x10000)},{0x4f0000,std::vector<std::byte>(4096)},
        {0x4f2000,std::vector<std::byte>(4096)},{0x475000,std::vector<std::byte>(4096)}};
    auto io=adapter(m);const auto g=SourceGeometryResourceGlobals::title_update_1_5();
    m.put(g.tags,tags);m.put(tags,0x73627370);m.put(g.physical,frame);m.put(frame+0x64,meta);m.put(meta+0x44,records);
    m.put(frame+0x38,0xabcd);m.put(g.inflight,flight);m.put(flight+0x44,rows);m.put(h+8,16);m.put(h+0x20,0xffffffff);
    m.put(g.policy,1);m.put(g.policy_filter,1);m.put(rows+12+3,1);std::vector<std::uint32_t> calls;
    SourceGeometryResourceRequestServices s;
    s.acquire=[&](auto header,bool blocking,bool selective,auto priority){
        check(header==h&&blocking&&selective&&priority==2,"130980 policy-selected acquisition inputs");
        calls.push_back(1);m.put(h+0x20,1);};
    s.promote=[&](auto job,auto priority){check(job==0&&priority==7,"130980 literal promotion priority");
        calls.push_back(2);m.put(g.physical,other);};
    s.yield=[&]{calls.push_back(3);};
    s.pump=[&]{calls.push_back(4);const std::array ready{std::byte{1}};m.write(rows+14,ready);
        m.put(h+0x1c,1u<<16);m.put(g.physical,third);}; // service links header itself
    check(source_geometry_resource_request(h,2,g,io,s),"130980 lost genuine service publication");
    check(calls==std::vector<std::uint32_t>{1,2,3,4},"130980 service order");
    unsigned physical_reads{};for(const auto& r:m.reads)if(r[0]==g.physical)++physical_reads;
    check(physical_reads==3,"130980 failed to reread physical owner after promotion/completion");
    check(get(m,records+24+0x14)==0xabcd&&get(m,rows+12+3,1)==2,"130980 lost pre-service stamp or post-promotion priority");
    // The special classification is pinned across acquire even if its tag
    // changes, and pending failure occurs only after the actual token reread.
    m.put(h+0x1c,0);m.put(h+0x20,0xffffffff);m.put(g.policy,0);m.put(g.missing_special,7);m.put(g.pending_failure,0);
    s.acquire=[&](auto,bool,bool,auto){m.put(tags,0);};
    check(!source_geometry_resource_request(h,2,g,io,s),"130980 accepted missing token");
    check(get(m,g.missing_special)==8&&get(m,g.pending_failure,1)==1,"130980 lost pinned special/pending-failure writes");
}
struct Boundary {std::uint32_t entry;};
J run(const J& row) {
    Memory m;for(const auto& bank:row.at("banks")) {
        const auto h=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<h.size();i+=2)bytes.push_back(std::byte(std::stoul(h.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    auto io=adapter(m);J calls=J::array();bool result{},stopped{};
    const auto stop=[&](unsigned entry,J args){calls.push_back(J{{"entry",entry},{"arguments",std::move(args)}});throw Boundary{entry};};
    SourceGeometryResourceRequestServices s;
    s.acquire=[&](auto header,bool blocking,bool selective,auto priority){
        const auto args=J::array({header,std::uint32_t(blocking),std::uint32_t(selective),priority});
        if(row.value("genuine_acquire_failure",false)) {
            calls.push_back(J{{"entry",0x130eb0},{"arguments",args}});
            if(row.value("limit_hint",false))m.put(row.at("physical").get<std::uint32_t>()+0x48u,3);
            return; // declared unavailable service; no token/outcome supplied to original
        }
        stop(0x130eb0,args);
    };
    s.promote=[&](auto job,auto priority){stop(0x1236a0,J::array({job,priority}));};
    s.yield=[&]{stop(0x2dbf74,J::array());};s.pump=[&]{stop(0x128870,J::array());};
    try {
        if(row.at("entry")==0x22a680)source_geometry_resource_publish(row.at("gp")[7],row.at("matrix"),io);
        else result=source_geometry_resource_request(row.at("gp")[1],row.at("gp")[0],SourceGeometryResourceGlobals::title_update_1_5(),io,s);
    }catch(const Boundary& b){const auto entries=row.at("stop_before_entries").get<std::vector<std::uint32_t>>();
        check(std::find(entries.begin(),entries.end(),b.entry)!=entries.end(),"Native130980 reached an undeclared service boundary");stopped=true;}
    J banks=J::array();const char* digits="0123456789abcdef";
    for(const auto& b:m.banks)if(b.address<0x62000000u || b.address>=0x62020000u) {
        std::string h;for(auto byte:b.bytes){const auto v=std::to_integer<unsigned>(byte);h+=digits[v>>4];h+=digits[v&15];}
        banks.push_back(J{{"address",b.address},{"after",h}});
    }
    return {{"result",result},{"stopped",stopped},{"calls",calls},{"reads",m.reads},{"writes",m.writes},{"banks",banks},{"fp",row.at("before_fp")}};
}
}
int main()try {
    controls();for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
