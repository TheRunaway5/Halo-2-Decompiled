#include "halo2/physics_body_disposal.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error("Original disposal differs: "+s);}
std::uint32_t word(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte>bytes(const std::string&s) {
    check(s.size()%2==0,"byte extent");std::vector<std::byte>r;r.reserve(s.size()/2);
    for(std::size_t i=0;i<s.size();i+=2)r.push_back(std::byte(word(s.substr(i,2))));
    return r;
}
struct Memory:halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>buffers;
    auto at(std::uint32_t p,std::size_t n)const {
        auto i=buffers.upper_bound(p);
        if(i==buffers.begin())throw std::runtime_error("Required disposal source backing unavailable");
        --i;if(std::uint64_t(p-i->first)+n>i->second.size())throw std::runtime_error("Required disposal source backing extent unavailable");
        return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override {
        const auto i=at(p,b.size());std::copy_n(i->second.begin()+(p-i->first),b.size(),b.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override {
        const auto i=at(p,b.size());std::copy(b.begin(),b.end(),buffers.at(i->first).begin()+(p-i->first));
    }
};
struct Case {
    std::string name;std::uint32_t entry,eax,ecx,result;
    halo2::SourcePhysicsBodyDisposalScratch scratch;
    std::array<std::uint32_t,16>args;Memory before;
    std::map<std::uint32_t,std::vector<std::byte>>after;
    std::vector<halo2::SourcePhysicsContactRecordStore>stores;std::vector<bool>temporary;
    std::vector<std::array<std::uint32_t,5>>free_calls;
};
}
int main(int argc,char**argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply independently measured original disposal fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream input(argv[1]);std::vector<Case>cases;
        for(std::string line;std::getline(input,line);) {
            std::istringstream in(line);std::vector<std::string>v;
            for(std::string x;in>>x;)v.push_back(x);
            if(v.empty())continue;
            if(v[0]=="C") {
                check(v.size()==24,"complete source ABI");Case c{};c.name=v[1];c.entry=word(v[2]);
                c.eax=word(v[3]);c.ecx=word(v[4]);c.result=word(v[5]);c.scratch={word(v[6]),word(v[7])};
                for(unsigned i=0;i<16;++i)c.args[i]=word(v[i+8]);
                cases.push_back(std::move(c));
            }else {
                check(!cases.empty(),"owning call");auto&c=cases.back();
                if(v[0]=="M"||v[0]=="A") {
                    check(v.size()==3,"complete buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
                    check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique backing span");
                }else if(v[0]=="W"||v[0]=="WS") {
                    check(v.size()==5,"literal source store");c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});c.temporary.push_back(v[0]=="WS");
                }else if(v[0]=="F") {
                    check(v.size()==6,"complete allocator call");std::array<std::uint32_t,5>a{};
                    for(unsigned i=0;i<5;++i)a[i]=word(v[i+1]);
                    c.free_calls.push_back(a);
                }else throw std::runtime_error("Unknown source disposal fixture record");
            }
        }
        std::size_t total_bytes=0,total_stores=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
            const auto observe=[&](const auto&s){stores.push_back(s);};
            std::vector<std::array<std::uint32_t,5>>free_calls;
            halo2::SourcePhysicsBodyDisposalServices services{};
            services.observe_free=[&](const auto&a){free_calls.push_back(a);};
            switch(c.entry) {
            case 0x1d3bb0:check(halo2::source_dispose_physics_component_bodies_1_1(m,c.args[0],c.scratch,services,observe)==c.result,c.name+" component result");break;
            case 0x317ad0:check(halo2::source_destroy_physics_rigid_body_1_1(m,c.ecx,c.args[0],services,observe)==c.result,c.name+" body result");break;
            case 0x318900:halo2::source_remove_physics_entity_property_1_1(m,c.ecx,c.args[0],c.args[1],observe);check(c.args[0]==c.result,c.name+" output pointer");break;
            case 0x27f9f0:check(halo2::source_physics_body_reference_is_resident_1_1(m,c.args[0])==((c.result&255u)!=0),c.name+" source AL");break;
            default:throw std::runtime_error("Unmeasured disposal owner");
            }
            check(free_calls==c.free_calls,c.name+" exact allocator args and order");
            auto equal=[&](std::size_t i) {
                const auto&a=stores[i];const auto&b=c.stores[i];
                return a.instruction==b.instruction&&a.address==b.address&&a.written==b.written&&(c.temporary[i]||a.before==b.before);
            };
            std::size_t i=0;while(i<stores.size()&&i<c.stores.size()&&equal(i))++i;
            if(i!=stores.size()||i!=c.stores.size()) {
                std::ostringstream s;s<<c.name<<" store "<<i<<" counts "<<stores.size()<<" / "<<c.stores.size();
                if(i<stores.size())s<<" actual "<<std::hex<<stores[i].instruction<<" at "<<stores[i].address;
                if(i<c.stores.size())s<<" original "<<std::hex<<c.stores[i].instruction<<" at "<<c.stores[i].address;
                throw std::runtime_error(s.str());
            }
            for(const auto&[p,b]:c.after) {
                std::vector<std::byte>actual(b.size());m.read(p,actual);check(actual==b,c.name+" full buffer");total_bytes+=b.size();
            }
            total_stores+=stores.size();
        }
        check(cases.size()==5,"actual PVS disposal, body destruction, two removals and residency owner");
        // Remove required allocator identity backing from the real positive
        // disposal input; source work must stop instead of supplying a free.
        {
            const auto&c=cases.front();auto m=c.before;
            const std::array<std::byte,4>missing{};m.write(0x487638,missing);bool stopped=false;
            try{halo2::source_dispose_physics_component_bodies_1_1(m,c.args[0],c.scratch);}
            catch(const std::runtime_error&){stopped=true;}
            check(stopped,"actual positive disposal requires retained allocator");
        }
        std::cout<<cases.size()<<" original calls; "<<total_bytes<<" full source bytes; "<<total_stores<<" ordered owning writes match\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
