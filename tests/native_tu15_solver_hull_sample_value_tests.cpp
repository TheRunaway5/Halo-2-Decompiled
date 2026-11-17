#include "halo2/native_tu15_solver_hull_polytope.hpp"
#include "reference/native_tu15_solver_hull_sample_value_before.inc"
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Fault {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char chars[]="0123456789abcdef";std::string out;for(auto b:data){const auto v=std::to_integer<unsigned>(b);out+=chars[v>>4];out+=chars[v&15];}return out;}
J host_fp(){std::uint16_t cw{},sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));return J::array({cw,sw,_mm_getcsr()});}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};
    auto at(std::uint32_t address,std::size_t size)const{auto p=banks.upper_bound(address);check(p!=banks.begin(),"Missing public hull sample bank");--p;check(std::uint64_t(address-p->first)+size<=p->second.size(),"Public hull sample bank extent");return p;}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        operations.push_back(J{{"kind","read"},{"address",address},{"bytes",out.size()},{"host_fp",host_fp()}});
        if(int(read_count++)==read_fault)throw Fault{};auto p=at(address,out.size());std::copy_n(p->second.begin()+address-p->first,out.size(),out.begin());
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        operations.push_back(J{{"kind","write"},{"address",address},{"bytes",data.size()},{"host_fp",host_fp()}});
        if(int(write_count++)==write_fault)throw Fault{};auto p=at(address,data.size());auto&bank=banks.at(p->first);
        writes.push_back(J{{"address",address},{"before",hex(std::span(bank).subspan(address-p->first,data.size()))},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),bank.begin()+address-p->first);
    }
};
struct Result{J operations,writes,events,fp,stats;std::map<std::uint32_t,std::vector<std::byte>>banks;bool stopped{};unsigned reads{},stores{},observed{};std::string error;std::uint32_t returned{};};
Memory seed(const J&row){
    Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return memory;
}
Result run(const J&row,const Memory&seed,bool old,bool observed,int read_fault=-1,int write_fault=-1,int observer_fault=-1,unsigned depth=0u){
    auto memory=seed;memory.read_fault=read_fault;memory.write_fault=write_fault;
    J events=J::array();unsigned observations{};SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)},{"host_fp",host_fp()}});if(int(observations++)==observer_fault)throw Fault{};};
    NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
    NativeTu15SolverHullPolytopeStats stats{1,2,3,4,5,6};
    const auto sp=row.at("gp").at(4).get<std::uint32_t>();const auto bank=seed.at(sp,20u);
    std::array<std::uint32_t,5>words{};std::memcpy(words.data(),bank->second.data()+sp-bank->first,20u);
    const auto output=row.at("id").get<unsigned>()%6u?words[4]:0x41d780u;
    bool stopped{};std::string error;std::uint32_t returned=0xdeadbeefu;
    try{
        const auto function=old?solver_hull_sample_value_reference::native_tu15_sample_solver_hull_direction:native_tu15_sample_solver_hull_direction;
        returned=function(memory,words[1],words[2],words[3],output,fp,depth,observer,&stats);
    }catch(const Fault&){stopped=true;}catch(const std::runtime_error&e){stopped=true;error=e.what();}
    return{memory.operations,memory.writes,events,J::array({fp.control_word,fp.status,fp.mxcsr}),
        J::array({stats.minimum_calls,stats.minimum_completed,stats.facet_candidates,stats.sample_calls,stats.sample_completed,stats.refusals}),
        memory.banks,stopped,memory.read_count,memory.write_count,observations,error,returned};
}
void equal(const Result&a,const Result&b,const std::string&name){
    check(a.operations==b.operations&&a.writes==b.writes&&a.events==b.events&&a.fp==b.fp&&a.stats==b.stats&&a.banks==b.banks&&a.stopped==b.stopped&&a.error==b.error&&a.returned==b.returned,
        name+": complete public hull sample DATA/FP/provider phases differ");
}
}
int main()try{
    J rows;std::cin>>rows;unsigned normal{},prefixes{},depth_refusals{},scale_aliases{};
    for(const auto&row:rows){const auto saved=seed(row);for(bool observed:{false,true}){
        const auto name=row.at("name").get<std::string>();
        const auto expected=run(row,saved,true,observed),actual=run(row,saved,false,observed);
        equal(actual,expected,name);check(!actual.stopped,"Normal public hull sample stopped");
        ++normal;scale_aliases+=row.at("id").get<unsigned>()%6u==0u;
        if(row.at("id").get<unsigned>()>=6u)continue;
        for(unsigned n=0;n<expected.reads;++n){equal(run(row,saved,false,observed,int(n)),run(row,saved,true,observed,int(n)),name+" read throw");++prefixes;}
        for(unsigned n=0;n<expected.stores;++n){equal(run(row,saved,false,observed,-1,int(n)),run(row,saved,true,observed,-1,int(n)),name+" write throw");++prefixes;}
        for(unsigned n=0;n<expected.observed;++n){equal(run(row,saved,false,observed,-1,-1,int(n)),run(row,saved,true,observed,-1,-1,int(n)),name+" observer throw");++prefixes;}
        const auto refused=run(row,saved,false,observed,-1,-1,-1,1u);
        equal(refused,run(row,saved,true,observed,-1,-1,-1,1u),name+" nonzero depth");
        check(refused.stopped&&refused.operations.empty(),"Public sample depth rejection touched DATA");++depth_refusals;
    }}
    std::cout<<J{{"passed",true},{"normal",normal},{"constant_alias_cases",scale_aliases},{"read_write_observer_throw_prefixes",prefixes},{"nonzero_depth_stops",depth_refusals},{"provider_and_observer_host_fp_packets_compared",true},{"raw_abi_qualified_here",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
