#include "halo2/physics_agent_scheduler.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error("Original agent scheduler differs: "+s);}
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
        if(i==buffers.begin())throw std::runtime_error("Required agent scheduler source backing unavailable");
        --i;if(std::uint64_t(p-i->first)+n>i->second.size())throw std::runtime_error("Required agent scheduler source backing extent unavailable");
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
        if(argc!=2)throw std::runtime_error("Supply independently measured original agent scheduler fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream input(argv[1]);std::vector<Case>cases;
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
                }else throw std::runtime_error("Unknown source agent scheduler fixture record");
            }
        }
        std::size_t total_bytes=0,total_stores=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
            const auto observe=[&](const auto&s){stores.push_back(s);};
            struct Stop {halo2::SourcePhysicsAgentOwnerCall call;};
            halo2::SourcePhysicsAgentSchedulerServices services{};services.floating_point=c.fp;
            services.required_owner=[](auto&,const auto&call,const auto&)->std::uint32_t{throw Stop{call};};
            bool stopped=false;
            try { switch(c.entry) {
            case 0x31d5f0:check(halo2::source_construct_physics_agent_output_bank_1_1(m,c.ecx,c.args[0],services,observe)==c.result,c.name+" output bank");break;
            case 0x31d690:halo2::source_destroy_physics_agent_output_bank_1_1(m,c.ecx,services,observe);break;
            case 0x31f550:{std::array<std::uint32_t,4>a{};std::copy_n(c.args.begin(),4,a.begin());halo2::source_deliver_physics_agent_output_1_1(m,c.ecx,a,c.frame,services,observe);break;}
            case 0x31e9d0:halo2::source_deliver_physics_world_contact_listeners_1_1(m,c.args[0],c.args[1],c.frame,services,observe);break;
            case 0x31c3c0:halo2::source_deliver_physics_entity_contact_listeners_1_1(m,c.args[0],c.args[1],c.frame,services,observe);break;
            case 0x31dee0:halo2::source_process_physics_island_agents_1_1(m,c.ecx,c.args[0],c.frame,c.fp,services,observe);break;
            case 0x31e2c0:halo2::source_schedule_physics_island_agents_1_1(m,c.ecx,c.args[0],c.args[1],c.frame,c.fp,services,observe);break;
            default:throw std::runtime_error("Unmeasured agent scheduler owner");
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
        check(cases.size()==16,"actual output-bank/contact-delivery and genuine scheduler stops");
        std::cout<<cases.size()<<" original complete calls/required stops; "<<total_bytes<<" full source bytes; "<<total_stores<<" ordered owning writes match\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
