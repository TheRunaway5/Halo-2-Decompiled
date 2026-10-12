#include "halo2/physics_current_broadphase.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error("Original current broadphase differs: "+s);}
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
        if(i==buffers.begin())throw std::runtime_error("Required current broadphase source backing unavailable");
        --i;if(std::uint64_t(p-i->first)+n>i->second.size())throw std::runtime_error("Required current broadphase source backing extent unavailable");
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
    std::uint32_t frame;halo2::SourcePhysicsFactoryFloatingPoint fp;
    std::optional<halo2::SourcePhysicsAgentOwnerCall> boundary;
    std::array<std::uint32_t,16>args;Memory before;
    std::map<std::uint32_t,std::vector<std::byte>>after;
    std::vector<halo2::SourcePhysicsContactRecordStore>stores;std::vector<bool>temporary;
};
}
int main(int argc,char**argv) {
    try {
        if(argc<2||argc>3)throw std::runtime_error("Supply independently measured original current broadphase fixture");
        std::vector<Case>cases;
        for(int file=1;file<argc;++file) {
        if(!std::filesystem::exists(argv[file]))return 77;
        std::ifstream input(argv[file]);
        for(std::string line;std::getline(input,line);) {
            std::istringstream in(line);std::vector<std::string>v;
            for(std::string x;in>>x;)v.push_back(x);
            if(v.empty())continue;
            if(v[0]=="C") {
                check(v.size()==25,"complete source ABI");Case c{};c.name=v[1];c.entry=word(v[2]);
                c.eax=word(v[3]);c.ecx=word(v[4]);c.result=word(v[5]);c.frame=word(v[6]);c.fp={std::uint16_t(word(v[7])),word(v[8])};
                for(unsigned i=0;i<16;++i)c.args[i]=word(v[i+9]);
                cases.push_back(std::move(c));
            }else {
                check(!cases.empty(),"owning call");auto&c=cases.back();
                if(v[0]=="M"||v[0]=="A") {
                    check(v.size()==3,"complete buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
                    check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique backing span");
                }else if(v[0]=="B") {
                    check(v.size()>=7,"complete unavailable-owner ABI");
                    halo2::SourcePhysicsAgentOwnerCall b{word(v[1]),word(v[2]),word(v[3]),word(v[4]),{},v[5]=="1"};
                    const auto n=std::stoul(v[6]);check(v.size()==7+n,"complete unavailable-owner args");
                    for(std::size_t k=0;k<n;++k)b.arguments.push_back(word(v[7+k]));
                    c.boundary=std::move(b);
                }else if(v[0]=="W"||v[0]=="WS") {
                    check(v.size()==5,"literal source store");c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});c.temporary.push_back(v[0]=="WS");
                }else throw std::runtime_error("Unknown source current broadphase fixture record");
            }
        }
        }
        std::size_t total_bytes=0,total_stores=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
            const auto observe=[&](const auto&s){stores.push_back(s);};
            struct Stop {halo2::SourcePhysicsAgentOwnerCall call;};
            halo2::SourcePhysicsCurrentBroadphaseServices services{};
            services.required_owner=[](auto&,const auto&call,const auto&)->std::uint32_t{throw Stop{call};};
            bool stopped=false;
            try { switch(c.entry) {
            case 0x2e68d0:{std::array<std::uint32_t,5>a{};std::copy_n(c.args.begin(),5,a.begin());halo2::source_update_physics_broadphase_bodies_1_1(m,c.ecx,a,c.fp,observe);break;}
            case 0x2e4670:halo2::source_physics_capsule_bounds_1_1(m,c.ecx,c.args[0],c.args[1],c.args[2],c.fp,observe);break;
            case 0x2e2380:halo2::source_physics_current_wrapper_bounds_1_1(m,c.ecx,c.args[0],c.args[1],c.args[2],c.fp,observe);break;
            case 0x31db70:halo2::source_update_physics_island_broadphase_1_1(m,c.ecx,c.args[0],c.args[1],c.frame,c.fp,services,observe);break;
            case 0x31e2c0:{
                halo2::SourcePhysicsAgentSchedulerServices outer{};outer.floating_point=c.fp;
                outer.required_owner=[&](auto&memory,const auto&call,const auto&observer)->std::uint32_t {
                    if(call.entry!=0x31db70)throw Stop{call};
                    check(call.arguments.size()==2,c.name+" current owner ABI");
                    halo2::source_update_physics_island_broadphase_1_1(memory,call.receiver,
                        call.arguments[0],call.arguments[1],call.entry_esp,c.fp,services,observer);
                    //31DB70 is void; the existing generic callback's value is
                    //discarded by31E2C0. This is not a success/admission result.
                    return {};
                };
                halo2::source_schedule_physics_island_agents_1_1(m,c.ecx,c.args[0],c.args[1],c.frame,c.fp,outer,observe);break;
            }
            default:throw std::runtime_error("Unmeasured current broadphase owner");
            }
            }catch(const Stop&stop) {
                stopped=true;check(c.boundary.has_value(),c.name+" unexpected required owner");
                const auto&a=stop.call;const auto&b=*c.boundary;
                check(a.instruction==b.instruction&&a.entry==b.entry&&a.receiver==b.receiver&&a.entry_esp==b.entry_esp&&a.arguments==b.arguments&&a.cdecl==b.cdecl,c.name+" actual unavailable-owner ABI");
            }catch(const std::exception&e){throw std::runtime_error(c.name+": "+e.what());}
            check(stopped==c.boundary.has_value(),c.name+" complete versus required-stop distinction");
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
        check(cases.size()==(argc==3?40u:38u),"actual current broadphase and capsule/wrapper calls");
        std::cout<<cases.size()<<" original complete calls/required stops; "<<total_bytes<<" full source bytes; "<<total_stores<<" ordered owning writes match\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
