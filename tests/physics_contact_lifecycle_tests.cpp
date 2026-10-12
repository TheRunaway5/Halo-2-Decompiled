#include "halo2/physics_contact_lifecycle.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const std::string &message) {
    if(!value)throw std::runtime_error("Original contact lifecycle differs: "+message);
}
std::uint32_t number(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s) {
    check(s.size()%2==0,"literal hex extent");std::vector<std::byte> r;r.reserve(s.size()/2);
    for(std::size_t n=0;n<s.size();n+=2)r.push_back(std::byte(number(s.substr(n,2))));
    return r;
}
struct Memory:halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> buffers;
    auto at(std::uint32_t a,std::size_t n)const {
        auto p=buffers.upper_bound(a);
        if(p==buffers.begin())throw std::runtime_error("Actual source backing unavailable");
        --p;
        if(std::uint64_t(a-p->first)+n>p->second.size())throw std::runtime_error("Actual source backing unavailable");
        return p;
    }
    void read(std::uint32_t a,std::span<std::byte> out)const override {
        auto p=at(a,out.size());std::copy_n(p->second.begin()+(a-p->first),out.size(),out.begin());
    }
    void write(std::uint32_t a,std::span<const std::byte> in)override {
        auto p=at(a,in.size());std::copy(in.begin(),in.end(),buffers.at(p->first).begin()+(a-p->first));
    }
};
struct Case {
    std::string name;std::uint32_t entry,eax,ecx,result,mxcsr;std::uint16_t cw;
    std::array<std::uint32_t,16> args;
    Memory before;std::map<std::uint32_t,std::vector<std::byte>> after;
    std::vector<halo2::SourcePhysicsContactRecordStore> writes;
    std::vector<halo2::SourcePhysicsContactDispatch> calls;
    std::vector<halo2::SourcePhysicsContactMath> math;
};
bool same(const halo2::SourcePhysicsContactDispatch &a,const halo2::SourcePhysicsContactDispatch &b) {
    return a.instruction==b.instruction&&a.target==b.target&&a.this_pointer==b.this_pointer&&a.stack_arguments==b.stack_arguments;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("Supply immutable original contact-lifecycle fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        std::ifstream f(argv[1]);std::vector<Case> cases;
        for(std::string line;std::getline(f,line);) {
            std::istringstream s(line);std::vector<std::string> r;
            for(std::string v;s>>v;)r.push_back(v);
            if(r.empty())continue;
            if(r[0]=="C") {
                check(r.size()==24,"full actual ABI words");Case c{};c.name=r[1];c.entry=number(r[2]);
                c.eax=number(r[3]);c.ecx=number(r[4]);c.result=number(r[5]);c.cw=std::uint16_t(number(r[6]));c.mxcsr=number(r[7]);
                for(unsigned n=0;n<16;++n)c.args[n]=number(r[n+8]);
                cases.push_back(std::move(c));
            }else {
                check(!cases.empty(),"owning input before records");auto &c=cases.back();
                if(r[0]=="M" || r[0]=="A") {
                    check(r.size()==3,"literal complete buffer");auto &b=r[0]=="M"?c.before.buffers:c.after;
                    check(b.emplace(number(r[1]),bytes(r[2])).second,"unique nonoverlapping source extent");
                }else if(r[0]=="W") {
                    check(r.size()==5,"literal owning store");c.writes.push_back({number(r[1]),number(r[2]),bytes(r[3]),bytes(r[4])});
                }else if(r[0]=="V") {
                    check(r.size()==4 || r.size()==5,"actual source virtual dispatch");
                    halo2::SourcePhysicsContactDispatch v{number(r[1]),number(r[2]),number(r[3]),{}};
                    if(r.size()==5)v.stack_arguments.push_back(number(r[4]));
                    c.calls.push_back(v);
                }else if(r[0]=="F") {
                    check(r.size()==3 || r.size()==4,"source arithmetic snapshot");
                    halo2::SourcePhysicsContactMath v{number(r[1]),{}};
                    for(std::size_t n=2;n<r.size();++n)v.words.push_back(number(r[n]));
                    c.math.push_back(v);
                }else throw std::runtime_error("Unknown original contact-lifecycle fixture record");
            }
        }
        check(cases.size()==11,"original measured owner roster");std::size_t byte_count=0,store_count=0,dispatch_count=0,math_count=0;
        for(const auto &c:cases) {
            auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore> writes;
            std::vector<halo2::SourcePhysicsContactDispatch> calls;
            std::vector<halo2::SourcePhysicsContactMath> math;
            auto observe=[&](const auto &v){writes.push_back(v);};
            auto dispatch=[&](const auto &v){calls.push_back(v);};
            auto arithmetic=[&](const auto &v){math.push_back(v);};
            switch(c.entry) {
            case 0x22e950:
                check(halo2::source_physics_contact_fast_allocate_1_1(m,c.ecx,c.args[0],observe)==c.result,c.name+" source allocation pointer");break;
            case 0x2df710:
                halo2::source_grow_physics_contact_bank_1_1(m,c.args[0],c.args[1],c.args[2],observe);break;
            case 0x1d3df0: {
                halo2::SourcePhysicsContactAppend a{};std::copy_n(c.args.begin(),13,a.stack_arguments.begin());
                check(halo2::source_append_physics_contact_1_1(m,a,observe)==c.result,c.name+" original retained row index");break;
            }
            case 0x1d2f10: {
                halo2::SourcePhysicsContactRefresh a{c.eax,c.ecx,{},c.cw,c.mxcsr};
                std::copy_n(c.args.begin(),4,a.stack_arguments.begin());
                halo2::source_refresh_physics_contact_1_1(m,a,observe,dispatch,arithmetic);break;
            }
            case 0x1d3fc0:
                halo2::source_remove_physics_contact_1_1(m,c.eax,c.ecx,c.args[0],c.args[1],observe);break;
            case 0x1d40b0:
                halo2::source_remove_physics_contact_row_1_1(m,c.args[0],c.args[1],observe);break;
            case 0x31f4b0:
                check(halo2::source_physics_contact_point_1_1(m,c.ecx,c.args[0])==c.result,c.name+" actual point pointer");break;
            case 0x31f4d0:
                check(halo2::source_physics_contact_scalar_1_1(m,c.ecx,c.args[0])==c.result,c.name+" actual scalar pointer");break;
            default:throw std::runtime_error("Unmeasured original entry");
            }
            check(writes==c.writes,c.name+" all ordered stores and before/written bytes");
            check(m.buffers==c.after,c.name+" full current-state outputs/untouched bytes");
            check(calls.size()==c.calls.size()&&std::equal(calls.begin(),calls.end(),c.calls.begin(),same),c.name+" actual manager/motion virtual ABI/order");
            check(math.size()==c.math.size()&&std::equal(math.begin(),math.end(),c.math.begin(),[](const auto &a,const auto &b) {
                return a.instruction==b.instruction&&a.words==b.words;
            }),c.name+" original reciprocal/scalar/spill/x87 integer arithmetic");
            for(const auto &[address,raw]:m.buffers){(void)address;byte_count+=raw.size();}
            store_count+=writes.size();dispatch_count+=calls.size();
            math_count+=math.size();
            // Contract checks only; no changed data is presented as another
            // original admitted contact or measured positive branch.
            if(c.entry==0x1d2f10) {
                auto missing=c.before;const auto p=missing.at(c.eax,4);missing.buffers.erase(p->first);
                bool rejected=false;try {halo2::SourcePhysicsContactRefresh a{c.eax,c.ecx,{c.args[0],c.args[1],c.args[2],c.args[3]},c.cw,c.mxcsr};
                    halo2::source_refresh_physics_contact_1_1(missing,a);}
                catch(const std::runtime_error &){rejected=true;}check(rejected,"missing genuine manager cannot produce success");
            }
        }
        std::cout<<cases.size()<<" original contact lifecycle calls matched "<<byte_count<<" full buffer bytes, "
                 <<store_count<<" ordered stores, "<<dispatch_count<<" actual virtual deliveries and "<<math_count<<" arithmetic snapshots\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
