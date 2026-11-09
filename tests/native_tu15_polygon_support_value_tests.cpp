#include "halo2/native_tu15_contact_shape.hpp"
#include "reference/native_tu15_contact_shape_support_before.inc"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;using J=nlohmann::json;
namespace {
struct Boundary {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto value=std::to_integer<unsigned>(b);out+=digits[value>>4];out+=digits[value&15];}return out;}
void clobber_fp(){
    asm volatile("fninit; fld1; fldz; fdivp; fld1":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing polygon support bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Polygon support bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(int(read_count++)==read_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        operations.push_back(J{{"kind","read"},{"instruction",pc},{"address",address},{"bytes",out.size()}});
        auto i=at(address,out.size());std::copy_n(i->second.begin()+address-i->first,out.size(),out.begin());
        if(hostile_fp)clobber_fp();
    }
    // This fixture owns committed ordinary banks. The independent CPU oracle
    // records one sixteen-byte vector operand; device adapters retain their
    // separate four-scalar policy and need their own provider proof.
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        if(int(write_count++)==write_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        auto i=at(address,data.size());auto&bank=banks.at(i->first);
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",hex(std::span(bank).subspan(address-i->first,data.size()))},{"written",hex(data)}});
        operations.push_back(J{{"kind","write"},{"instruction",pc},{"address",address},{"bytes",data.size()}});
        std::copy(data.begin(),data.end(),bank.begin()+address-i->first);
        if(hostile_fp)clobber_fp();
    }
    J architectural_reads()const{
        J out=J::array();
        for(std::size_t n=0;n<operations.size();++n){
            auto event=operations[n];if(event.at("kind")!="read")continue;
            if(observer_preimages&&n+1<operations.size()){
                const auto&next=operations[n+1];
                if(next.at("kind")=="write"&&event.at("instruction")==next.at("instruction")&&
                   event.at("address")==next.at("address")&&event.at("bytes")==next.at("bytes"))continue;
            }
            event.erase("kind");out.push_back(event);
        }
        return out;
    }
};
}
struct Result {J operations,writes,events;std::map<std::uint32_t,std::vector<std::byte>>banks;NativeTu15PhysicsFrameFp fp;bool failed{};};
Result execute(const J&row,bool reference,bool hostile,bool observe,int read_fault=-1,int write_fault=-1,int observer_fault=-1){
    Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    memory.read_fault=read_fault;memory.write_fault=write_fault;memory.hostile_fp=hostile;
    J events=J::array();unsigned observations{};SourcePhysicsContactRecordObserver observer;
    if(observe)observer=[&](const auto&event){events.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});if(hostile)clobber_fp();if(int(observations++)==observer_fault)throw Boundary{};};
    const auto&g=row.at("gp");const auto sp=g.at(4).get<std::uint32_t>();const auto local=((sp-4u)&0xfffffff0u)-16u;
    NativeTu15ContactShapeScratch scratch{local+12u,0,nullptr};NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
    bool failed{};
    try{
        if(reference)polygon_support_reference::reference_contact_shape_support(memory,{g.at(1)},{0x1f6610u},row.at("matrix"),row.at("source"),scratch,fp,row.at("live_x87").size(),observer);
        else native_tu15_contact_shape_support(memory,{g.at(1)},{0x1f6610u},row.at("matrix"),row.at("source"),scratch,fp,row.at("live_x87").size(),observer);
    }catch(const Boundary&){failed=true;}catch(const std::runtime_error&){failed=true;}
    return {memory.operations,memory.writes,events,memory.banks,fp,failed};
}
void equal(const Result&a,const Result&b,const std::string&name){
    check(a.failed==b.failed&&a.operations==b.operations&&a.writes==b.writes&&a.events==b.events&&a.banks==b.banks&&
        a.fp.control_word==b.fp.control_word&&a.fp.status==b.fp.status&&a.fp.mxcsr==b.fp.mxcsr,name+": public support DATA differs");
}
int main(int argc,char**argv)try{
    check(argc==2||argc==3,"Supply polygon physical fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);unsigned normal{},hostile{},prefixes{};
    for(const auto&row:fixture.at("cases")){
        const auto phase=row.at("phase").get<std::string>();if(phase!="support_boundary"&&phase!="alignment_fault")continue;
        for(bool observe:{false,true}){
            const auto old=execute(row,true,false,observe),current=execute(row,false,false,observe);
            const auto name=row.at("name").get<std::string>();equal(current,old,name);++normal;
            equal(execute(row,false,true,observe),current,name+" hostile");++hostile;
            if(row.at("id").get<unsigned>()%16u>1u)continue;
            unsigned reads{},writes{};for(const auto&op:current.operations){if(op.at("kind")=="read")++reads;else ++writes;}
            for(unsigned n=0;n<reads;++n){const auto actual=execute(row,false,false,observe,int(n));equal(actual,execute(row,true,false,observe,int(n)),name+" read prefix");equal(execute(row,false,true,observe,int(n)),actual,name+" hostile read prefix");++prefixes;}
            for(unsigned n=0;n<writes;++n){const auto actual=execute(row,false,false,observe,-1,int(n));equal(actual,execute(row,true,false,observe,-1,int(n)),name+" write prefix");equal(execute(row,false,true,observe,-1,int(n)),actual,name+" hostile write prefix");++prefixes;
                if(observe){const auto complete=execute(row,false,false,observe,-1,-1,int(n));equal(complete,execute(row,true,false,observe,-1,-1,int(n)),name+" observer prefix");equal(execute(row,false,true,observe,-1,-1,int(n)),complete,name+" hostile observer prefix");++prefixes;}}
        }
    }
    const auto report=J{{"passed",true},{"normal",normal},{"hostile",hostile},{"prefixes",prefixes}};if(argc==3){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';}
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
