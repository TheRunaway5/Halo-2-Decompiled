#include "halo2/physics_body_state.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool v,const std::string &s){if(!v)throw std::runtime_error("Original body owner differs: "+s);}
std::uint32_t number(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s) {
    check(s.size()%2==0,"literal source byte extent");std::vector<std::byte> r;r.reserve(s.size()/2);
    for(std::size_t n=0;n<s.size();n+=2)r.push_back(std::byte(number(s.substr(n,2))));
    return r;
}
struct Memory:halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> buffers;
    auto at(std::uint32_t a,std::size_t n)const {
        auto p=buffers.upper_bound(a);if(p==buffers.begin())throw std::runtime_error("Required real physics backing unavailable");
        --p;if(std::uint64_t(a-p->first)+n>p->second.size())throw std::runtime_error("Required real physics backing unavailable");return p;
    }
    void read(std::uint32_t a,std::span<std::byte> out)const override {
        auto p=at(a,out.size());std::copy_n(p->second.begin()+(a-p->first),out.size(),out.begin());
    }
    void write(std::uint32_t a,std::span<const std::byte> in)override {
        auto p=at(a,in.size());std::copy(in.begin(),in.end(),buffers.at(p->first).begin()+(a-p->first));
    }
};
struct Delivery {std::uint32_t manager,row;std::array<std::uint32_t,4> args;};
struct Case {
    std::string name;std::uint32_t entry,eax,ecx,result,mxcsr,context,scratch;std::uint16_t cw;
    std::array<std::uint32_t,16> args;Memory before;
    std::map<std::uint32_t,std::vector<std::byte>> after;
    std::vector<halo2::SourcePhysicsContactRecordStore> writes;std::vector<Delivery> delivery;
};
}
int main(int argc,char**argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply immutable original body-owner fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream f(argv[1]);std::vector<Case> cases;
        for(std::string line;std::getline(f,line);) {
            std::istringstream s(line);std::vector<std::string> v;for(std::string x;s>>x;)v.push_back(x);
            if(v.empty())continue;
            if(v[0]=="C") {
                check(v.size()==26,"full source ABI");Case c{};c.name=v[1];c.entry=number(v[2]);
                c.eax=number(v[3]);c.ecx=number(v[4]);c.result=number(v[5]);c.cw=std::uint16_t(number(v[6]));
                c.mxcsr=number(v[7]);c.context=number(v[8]);c.scratch=number(v[9]);
                for(unsigned n=0;n<16;++n)c.args[n]=number(v[n+10]);
                cases.push_back(std::move(c));
            } else {
                check(!cases.empty(),"owning original case");auto&c=cases.back();
                if(v[0]=="M"||v[0]=="A") {
                    check(v.size()==3,"complete captured buffer");auto&b=v[0]=="M"?c.before.buffers:c.after;
                    check(b.emplace(number(v[1]),bytes(v[2])).second,"unique source extent");
                } else if(v[0]=="W") {
                    check(v.size()==5,"source store operands");c.writes.push_back({number(v[1]),number(v[2]),bytes(v[3]),bytes(v[4])});
                } else if(v[0]=="D") {
                    check(v.size()==7,"actual manager/row/four-stack ABI");Delivery d{number(v[1]),number(v[2]),{}};
                    for(unsigned n=0;n<4;++n)d.args[n]=number(v[3+n]);
                    c.delivery.push_back(d);
                } else throw std::runtime_error("Unknown source body fixture record");
            }
        }
        check(cases.size()==50,"original measured case roster");std::size_t stores=0,matched_bytes=0,delivered=0;
        for(const auto&c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore> writes;std::vector<Delivery> delivery;
            const auto observe=[&](const auto&s){writes.push_back(s);};
            switch(c.entry) {
            case 0x317890:check(halo2::source_initialize_physics_body_info_1_1(m,c.ecx,observe)==c.result,c.name+" descriptor return");break;
            case 0x312410:check(halo2::source_construct_physics_entity_1_1(m,c.ecx,c.args[0],observe)==c.result,c.name+" entity return");break;
            case 0x318af0:check(halo2::source_construct_physics_entity_base_1_1(m,c.ecx,c.args[0],observe)==c.result,c.name+" base return");break;
            case 0x318a20:halo2::source_import_physics_entity_properties_1_1(m,c.ecx,c.args[0],observe);break;
            case 0x318a90:halo2::source_add_physics_entity_property_1_1(m,c.ecx,c.args[0],c.args[1],observe);break;
            case 0x2df780:halo2::source_double_physics_body_bank_1_1(m,c.args[0],c.args[1],observe);break;
            case 0x2df360:halo2::source_copy_physics_body_bank_bytes_1_1(m,c.args[0],c.args[1],c.args[2],observe);break;
            case 0x22e950:check(halo2::source_allocate_physics_body_small_block_1_1(m,c.ecx,c.args[0],observe)==c.result,c.name+" allocation backing");break;
            case 0x22eb20:halo2::source_free_physics_body_small_block_1_1(m,c.ecx,c.args[0],c.args[1],observe);break;
            case 0x2e65a0:check(halo2::source_decode_physics_current_bounds_1_1(m,{c.ecx,c.args[0],c.args[1],c.cw,c.mxcsr},observe)==c.result,c.name+" packed bounds return");break;
            case 0x2e7610:check(halo2::source_physics_current_bounds_1_1(m,c.ecx,c.args[0],c.args[1],c.cw,c.mxcsr,observe)==c.result,c.name+" handle bounds return");break;
            case 0x1d4160: {
                halo2::SourcePhysicsContactManagerDelivery d{c.context,{},c.scratch,c.cw,c.mxcsr};
                std::copy_n(c.args.begin(),5,d.stack_arguments.begin());
                halo2::source_deliver_physics_contact_manager_1_1(m,d,observe,{},{},[&](const auto&r){delivery.push_back({r.manager,r.row,r.stack_arguments});});break;
            }
            default:throw std::runtime_error("Unmeasured source body entry");
            }
            if(writes!=c.writes) {
                std::size_t n=0;while(n<writes.size()&&n<c.writes.size()&&writes[n]==c.writes[n])++n;
                std::ostringstream message;message<<c.name<<" ordered store "<<n<<"/"<<writes.size()<<" expected "<<c.writes.size();
                if(n<writes.size())message<<" actual site "<<std::hex<<writes[n].instruction<<" address "<<writes[n].address;
                if(n<c.writes.size())message<<" expected site "<<std::hex<<c.writes[n].instruction<<" address "<<c.writes[n].address;
                throw std::runtime_error(message.str());
            }
            check(m.buffers==c.after,c.name+" full retained outputs and untouched bytes");
            check(delivery.size()==c.delivery.size(),c.name+" manager delivery count");
            for(std::size_t n=0;n<delivery.size();++n)
                check(delivery[n].manager==c.delivery[n].manager&&delivery[n].row==c.delivery[n].row&&delivery[n].args==c.delivery[n].args,c.name+" original EAX/ECX/four stack arguments");
            for(const auto&[a,b]:m.buffers){(void)a;matched_bytes+=b.size();}
            stores+=writes.size();delivered+=delivery.size();
        }
        // An absent required real storage provider cannot manufacture a body,
        // a collider, a callback admission or a successful current-bound query.
        Memory absent;bool failed=false;try{halo2::source_physics_current_bounds_1_1(absent,1,2,3,0x23f,0x1f80);}
        catch(const std::runtime_error&){failed=true;}check(failed,"required backend backing boundary");
        std::cout<<cases.size()<<" original entity/property/bounds/delivery calls matched "<<matched_bytes
            <<" full retained bytes, "<<stores<<" ordered stores and "<<delivered<<" exact manager delivery\n";
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
