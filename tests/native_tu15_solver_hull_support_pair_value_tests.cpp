#include "halo2/native_tu15_solver_hull_support.hpp"
#include "reference/native_tu15_solver_hull_support_pair_value_before.inc"
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
    auto at(std::uint32_t address,std::size_t size)const{auto p=banks.upper_bound(address);check(p!=banks.begin(),"Missing public pair bank");--p;check(std::uint64_t(address-p->first)+size<=p->second.size(),"Public pair bank extent");return p;}
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
struct Result{J operations,writes,events,children,fp,stats;std::map<std::uint32_t,std::vector<std::byte>>banks;bool stopped{};unsigned reads{},stores{},observed{};std::string error;};
Result run(const J&row,bool old,bool observed,int read_fault=-1,int write_fault=-1,int observer_fault=-1,bool missing=false){
    Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    constexpr std::uint32_t scratch=0x62000000u;memory.banks[scratch]=std::vector<std::byte>(0x1000,std::byte{0xa5});memory.read_fault=read_fault;memory.write_fault=write_fault;
    J events=J::array(),children=J::array();unsigned observations{};SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)},{"host_fp",host_fp()}});if(int(observations++)==observer_fault)throw Fault{};};
    NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};NativeTu15SolverHullSupportStats stats{11,12,13,14,15,16,17,18};NativeTu15SolverHullSupportServices services;
    if(!missing)services.support=[&](auto shape,auto owner,auto direction,auto output,auto&state){
        children.push_back(J{{"shape",shape.identity},{"owner",owner.identity},{"direction",direction},{"output",output},{"fp",{state.control_word,state.status,state.mxcsr}},{"host_fp",host_fp()}});
        native_tu15_contact_shape_support(memory,shape,owner,direction,output,{scratch+0x100u,scratch+0x110u,nullptr},state,0u,observer);
    };
    bool stopped{};std::string error;
    try{
        const auto function=old?solver_hull_support_pair_value_reference::native_tu15_solver_hull_support_pair:native_tu15_solver_hull_support_pair;
        function(memory,row.at("gp")[1],row.at("matrix"),row.at("source"),scratch,fp,0u,services,observer,&stats);
    }catch(const Fault&){stopped=true;}catch(const std::runtime_error&e){stopped=true;error=e.what();}
    return{memory.operations,memory.writes,events,children,J::array({fp.control_word,fp.status,fp.mxcsr}),J::array({stats.append_calls,stats.append_completed,stats.refusals,stats.pair_calls,stats.pair_completed,stats.shape_support_calls,stats.initialize_calls,stats.initialize_completed}),memory.banks,stopped,memory.read_count,memory.write_count,observations,error};
}
void equal(const Result&a,const Result&b,const std::string&name){check(a.operations==b.operations&&a.writes==b.writes&&a.events==b.events&&a.children==b.children&&a.fp==b.fp&&a.stats==b.stats&&a.banks==b.banks&&a.stopped==b.stopped&&a.error==b.error,name+": complete public pair DATA/FP/provider phases differ");}
}
int main()try{
    J rows;std::cin>>rows;unsigned normal{},prefixes{},missing{},children{};
    for(const auto&row:rows)for(bool observed:{false,true}){
        const auto name=row.at("name").get<std::string>();const auto expected=run(row,true,observed),actual=run(row,false,observed);equal(actual,expected,name);check(!actual.stopped,"Normal public pair stopped");++normal;children+=actual.children.size();
        if(row.at("id").get<unsigned>()>=2u)continue;
        for(unsigned n=0;n<expected.reads;++n){equal(run(row,false,observed,int(n)),run(row,true,observed,int(n)),name+" read throw");++prefixes;}
        for(unsigned n=0;n<expected.stores;++n){equal(run(row,false,observed,-1,int(n)),run(row,true,observed,-1,int(n)),name+" write throw");++prefixes;}
        for(unsigned n=0;n<expected.observed;++n){equal(run(row,false,observed,-1,-1,int(n)),run(row,true,observed,-1,-1,int(n)),name+" observer throw");++prefixes;}
        equal(run(row,false,observed,-1,-1,-1,true),run(row,true,observed,-1,-1,-1,true),name+" absent service");++missing;
    }
    std::cout<<J{{"passed",true},{"normal",normal},{"actual_computed_public_shape_calls",children},{"read_write_observer_throw_prefixes",prefixes},{"missing_service_stops",missing},{"provider_and_observer_host_fp_packets_compared",true},{"public_vector_protocol_bytes",16},{"raw_abi_qualified_here",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
