#include "halo2/physics_broadphase.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error("Original broadphase differs: "+s);}
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
        if(i==buffers.begin())throw std::runtime_error("Required broadphase source backing unavailable");
        --i;if(std::uint64_t(p-i->first)+n>i->second.size())throw std::runtime_error("Required broadphase source backing extent unavailable");
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
    std::array<std::uint32_t,2> scratch;
    halo2::SourcePhysicsFactoryFloatingPoint fp;
    std::array<std::uint32_t,16>args;Memory before;
    std::map<std::uint32_t,std::vector<std::byte>>after;
    std::vector<halo2::SourcePhysicsContactRecordStore>stores;std::vector<bool>temporary;
};
}
int main(int argc,char**argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply independently measured original broadphase fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream input(argv[1]);std::vector<Case>cases;
        for(std::string line;std::getline(input,line);) {
            std::istringstream in(line);std::vector<std::string>v;
            for(std::string x;in>>x;)v.push_back(x);
            if(v.empty())continue;
            if(v[0]=="C") {
                check(v.size()==26,"complete source ABI");Case c{};c.name=v[1];c.entry=word(v[2]);
                c.eax=word(v[3]);c.ecx=word(v[4]);c.result=word(v[5]);c.scratch={word(v[6]),word(v[7])};
                c.fp={std::uint16_t(word(v[8])),word(v[9])};
                for(unsigned i=0;i<16;++i)c.args[i]=word(v[i+10]);
                cases.push_back(std::move(c));
            }else {
                check(!cases.empty(),"owning call");auto&c=cases.back();
                if(v[0]=="M"||v[0]=="A") {
                    check(v.size()==3,"complete buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
                    check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique backing span");
                }else if(v[0]=="W"||v[0]=="WS") {
                    check(v.size()==5,"literal source store");c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});c.temporary.push_back(v[0]=="WS");
                }else throw std::runtime_error("Unknown source broadphase fixture record");
            }
        }
        std::size_t total_bytes=0,total_stores=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
            const auto observe=[&](const auto&s){stores.push_back(s);};
            switch(c.entry) {
            case 0x313c90:check(halo2::source_construct_physics_pair_bank_1_1(m,c.ecx,c.args[0],{},observe)==c.result,c.name+" pair descriptor");break;
            case 0x313e50:halo2::source_destroy_physics_pair_bank_1_1(m,c.ecx,{},observe);break;
            case 0x2e7da0:check(halo2::source_construct_physics_overlap_bitset_1_1(m,c.ecx,c.args[0],{},observe)==c.result,c.name+" bitset descriptor");break;
            case 0x2e8090:halo2::source_destroy_physics_overlap_bitset_1_1(m,c.ecx,{},observe);break;
            case 0x2e7500: {
                std::array<std::uint32_t,6> a{};std::copy_n(c.args.begin(),6,a.begin());
                check(halo2::source_insert_physics_broadphase_axis_1_1(m,c.ecx,a,observe)==c.result,c.name+" endpoint index");break;
            }
            case 0x2e6240:halo2::source_repair_physics_broadphase_indices_1_1(m,c.args[0],c.args[1],c.args[2],observe);break;
            case 0x2e8a00:halo2::source_insert_physics_broadphase_body_1_1(m,c.ecx,c.args[0],c.args[1],c.args[2],{c.scratch[1]},c.fp,{},observe);break;
            case 0x303e30:halo2::source_physics_convex_bounds_1_1(m,c.ecx,c.args[0],c.args[1],c.args[2],c.fp,observe);break;
            case 0x2e2380:halo2::source_physics_wrapper_bounds_1_1(m,c.ecx,c.args[0],c.args[1],c.args[2],c.fp,observe);break;
            case 0x3144d0: {
                bool stopped=false;
                try{halo2::source_insert_physics_world_broadphase_1_1(m,c.ecx,c.args[0],{c.scratch[0],{c.scratch[1]}},c.fp,{},{},observe);}
                catch(const std::runtime_error&e){const auto msg=std::string(e.what());stopped=msg=="Required original378EB0 world pair manager owner unavailable"||msg=="Required original31D980 island pair manager owner unavailable";}
                check(stopped,c.name+" actual next pair manager boundary");break;
            }
            default:throw std::runtime_error("Unmeasured broadphase owner");
            }
            auto equal=[&](std::size_t i) {
                const auto&a=stores[i];const auto&b=c.stores[i];
                return a.instruction==b.instruction&&a.address==b.address&&a.written==b.written&&(c.temporary[i]||a.before==b.before);
            };
            std::size_t i=0;while(i<stores.size()&&i<c.stores.size()&&equal(i))++i;
            if(i!=stores.size()||i!=c.stores.size()) {
                std::ostringstream s;s<<c.name<<" store "<<i<<" counts "<<stores.size()<<" / "<<c.stores.size();
                if(i<stores.size())s<<" actual "<<std::hex<<stores[i].instruction<<" at "<<stores[i].address;
                if(i<c.stores.size())s<<" original "<<std::hex<<c.stores[i].instruction<<" at "<<c.stores[i].address;
                if(i<stores.size()&&i<c.stores.size()) {
                    s<<" actual bytes ";for(auto b:stores[i].written)s<<std::hex<<unsigned(std::to_integer<unsigned char>(b))<<' ';
                    s<<" original bytes ";for(auto b:c.stores[i].written)s<<std::hex<<unsigned(std::to_integer<unsigned char>(b))<<' ';
                }
                throw std::runtime_error(s.str());
            }
            for(const auto&[p,b]:c.after) {
                std::vector<std::byte>actual(b.size());m.read(p,actual);check(actual==b,c.name+" full buffer");total_bytes+=b.size();
            }
            total_stores+=stores.size();
        }
        check(cases.size()==41,"actual convex/wrapper, four broadphase pairs and exact pair manager stops");
        std::cout<<cases.size()<<" original calls; "<<total_bytes<<" full source bytes; "<<total_stores<<" ordered owning writes match\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
