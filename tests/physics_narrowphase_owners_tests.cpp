#include "halo2/physics_narrowphase_owners.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string&s){if(!v)throw std::runtime_error("Original narrowphase owners differs: "+s);}
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
        if(i==buffers.begin())throw std::runtime_error("Required narrowphase owners source backing unavailable");
        --i;if(std::uint64_t(p-i->first)+n>i->second.size()){std::ostringstream s;s<<"Required narrowphase owners source backing extent unavailable at "<<std::hex<<p<<" bytes "<<n;throw std::runtime_error(s.str());}
        return i;
    }
    void read(std::uint32_t p,std::span<std::byte>b)const override {
        const auto i=at(p,b.size());std::copy_n(i->second.begin()+(p-i->first),b.size(),b.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override {
        const auto i=at(p,b.size());std::copy(b.begin(),b.end(),buffers.at(i->first).begin()+(p-i->first));
    }
};
struct NeedOwner {};
struct Case {
    std::string name;std::uint32_t entry,eax,ecx,result;
    std::uint32_t frame,edx,esi,edi;halo2::SourcePhysicsFactoryFloatingPoint fp;
    std::optional<halo2::SourcePhysicsAgentOwnerCall> boundary;
    std::array<std::uint32_t,16>args;Memory before;
    std::map<std::uint32_t,std::vector<std::byte>>after;
    std::vector<halo2::SourcePhysicsContactRecordStore>stores;std::vector<bool>temporary;
};
}
int main(int argc,char**argv) {
    try {
        if(argc<2||argc>3)throw std::runtime_error("Supply independently measured original narrowphase owners fixture");
        std::vector<Case>cases;
        for(int file=1;file<argc;++file) {
        if(!std::filesystem::exists(argv[file]))return 77;
        std::ifstream input(argv[file]);
        for(std::string line;std::getline(input,line);) {
            std::istringstream in(line);std::vector<std::string>v;
            for(std::string x;in>>x;)v.push_back(x);
            if(v.empty())continue;
            if(v[0]=="C") {
                check(v.size()==28,"complete source ABI");Case c{};c.name=v[1];c.entry=word(v[2]);
                c.eax=word(v[3]);c.ecx=word(v[4]);c.result=word(v[5]);c.frame=word(v[6]);c.fp={std::uint16_t(word(v[7])),word(v[8])};c.edx=word(v[9]);c.esi=word(v[10]);c.edi=word(v[11]);
                for(unsigned i=0;i<16;++i)c.args[i]=word(v[i+12]);
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
                }else throw std::runtime_error("Unknown source narrowphase owners fixture record");
            }
        }
        }
        std::size_t total_bytes=0,total_stores=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
            const auto observe=[&](const auto&s){stores.push_back(s);};
            bool stopped=false;
            halo2::SourcePhysicsNarrowphaseServices services;
            services.required_owner=[&](auto&,const auto&q,const auto&)->std::uint32_t {
                check(c.boundary.has_value(),c.name+" unexpected required source owner");const auto&b=*c.boundary;
                check(q.instruction==b.instruction&&q.entry==b.entry&&q.ecx==b.receiver&&q.entry_esp==b.entry_esp&&q.arguments==b.arguments,c.name+" exact required owner ABI");
                throw NeedOwner{};
            };
            try { switch(c.entry) {
            case 0x2e1430:{std::array<std::uint32_t,4>a{};std::copy_n(c.args.begin(),4,a.begin());check(halo2::source_admit_physics_narrowphase_1_1(m,a,c.frame,services,observe)==c.result,c.name+" original admission pointer");break;}
            case 0x2e14e0:{std::array<std::uint32_t,5>a{};std::copy_n(c.args.begin(),5,a.begin());check(halo2::source_update_physics_motion_narrowphase_1_1(m,c.ecx,a,c.frame,c.fp,services,observe)==c.result,c.name+" original wrapper output pointer");break;}
            case 0x2e17ed:{std::array<std::uint32_t,5>a{};std::copy_n(c.args.begin(),5,a.begin());check(halo2::source_finish_physics_motion_narrowphase_1_1(m,c.ecx,a,c.frame,observe)==c.result,c.name+" actual original child continuation pointer");break;}
            case 0x2f4bd0:{std::array<std::uint32_t,5>a{};std::copy_n(c.args.begin(),5,a.begin());check(halo2::source_update_physics_separation_1_1(m,c.ecx,a,c.frame,c.fp,services,observe)==c.result,c.name+" original separation output pointer");break;}
            case 0x2f4d0a:{std::array<std::uint32_t,5>a{};std::copy_n(c.args.begin(),5,a.begin());check(halo2::source_finish_physics_separation_1_1(m,c.ecx,a,c.frame,c.eax,c.fp,observe)==c.result,c.name+" actual original GJK continuation pointer");break;}
            case 0x309860:{std::array<std::uint32_t,3>a{};std::copy_n(c.args.begin(),3,a.begin());check(halo2::source_emit_physics_narrowphase_result_1_1(m,c.ecx,a,c.frame,c.fp,observe)==c.result,c.name+" actual original result packet");break;}
            case 0x2e0c70:halo2::source_rotate_physics_contact_vector_1_1(m,c.ecx,c.args[0],c.args[1],c.fp,observe);break;
            default:throw std::runtime_error("Unmeasured narrowphase owner owner");
            }
            }catch(const NeedOwner&){stopped=true;}catch(const std::exception&e){
                std::ostringstream failure;failure<<c.name<<": "<<e.what();
                std::size_t k=0;while(k<stores.size()&&k<c.stores.size()&&stores[k].instruction==c.stores[k].instruction&&stores[k].address==c.stores[k].address&&stores[k].written==c.stores[k].written)++k;
                failure<<" prefix "<<k<<" actual "<<stores.size()<<" original "<<c.stores.size();
                if(k<stores.size()){failure<<" actual "<<std::hex<<stores[k].instruction<<" address "<<stores[k].address<<" bytes ";for(auto b:stores[k].written)failure<<unsigned(std::to_integer<unsigned char>(b))<<' ';}
                if(k<c.stores.size()){failure<<" original "<<std::hex<<c.stores[k].instruction<<" address "<<c.stores[k].address<<" bytes ";for(auto b:c.stores[k].written)failure<<unsigned(std::to_integer<unsigned char>(b))<<' ';}
                throw std::runtime_error(failure.str());
            }
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
        check(!cases.empty(),"actual narrowphase owner calls");
        std::cout<<cases.size()<<" original complete calls/required stops; "<<total_bytes<<" full source bytes; "<<total_stores<<" ordered owning writes match\n";return 0;
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
