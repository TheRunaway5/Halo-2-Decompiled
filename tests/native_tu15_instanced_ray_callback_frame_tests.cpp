#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_instanced_ray_callback_frame.hpp"
#include "halo2/native_tu15_instanced_ray_callback_value.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace {
struct Boundary {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto value=std::to_integer<unsigned>(b);out+=digits[value>>4];out+=digits[value&15];}return out;}
void clobber_fp(){
    asm volatile("fninit; fld1; fldz; fdivp; fld1":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
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
int main(int argc,char**argv)try{
    check(argc==2||argc==3,"Supply physical instanced callback fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong callback original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{};
    unsigned value_checks{};
    // Ordinary public DATA views share the same algorithm. Query callbacks
    // validate genuine original entry arguments, then stop without a hit result.
    for(const auto&row:fixture.at("cases")){
        if(row.at("phase")!="normal"||row.at("name").get<std::string>().find("_basic_")==std::string::npos)continue;
        Memory memory,expected;
        for(const auto&bank:row.at("banks")){memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));expected.banks.emplace(bank.at("address"),bytes(bank.at("after")));}
        struct Words {Memory&memory;std::uint32_t word(std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t v{};std::memcpy(&v,b.data(),4);return v;}}words{memory},actual_words{expected};
        auto publish=[&](std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width){memory.pc=pc;std::array<std::byte,4>b{};std::memcpy(b.data(),&value,4);memory.write(p,std::span(b).first(width));};
        const auto sp=row.at("gp")[4].get<std::uint32_t>();
        const auto body=row.at("entry")==0x24def0u?row.at("source").get<std::uint32_t>():row.at("matrix").get<std::uint32_t>();
        const auto collector=row.at("entry")==0x24def0u?words.word(sp+12u):row.at("gp")[1].get<std::uint32_t>();
        auto key=[&]{return words.word(body+4u);};
        auto query=[&](std::uint32_t instance,std::uint32_t mask,std::uint32_t filter,std::uint32_t origin,std::uint32_t displacement,std::uint32_t result)->bool{
            check(!row.at("returned").get<bool>(),"Unexpected value query");
            const auto boundary_sp=row.at("boundary_gp")[4].get<std::uint32_t>();
            check(instance==row.at("boundary_gp")[0]&&result==row.at("boundary_gp")[3]&&mask==actual_words.word(boundary_sp+4u)&&filter==actual_words.word(boundary_sp+8u)&&origin==actual_words.word(boundary_sp+12u)&&displacement==actual_words.word(boundary_sp+16u),"Value query arguments differ from genuine16C000 entry");
            throw Boundary{};
        };
        auto compare=[](std::uint32_t,std::uint32_t)->bool{throw std::runtime_error("Unprovided positive query continuation");};
        native_instanced_ray_callback_detail::CollectorValues values{words,publish,key,query,compare,collector};
        bool interrupted{};
        try{
            if(row.at("entry")==0x24def0u){
                auto invoke=[&](std::uint32_t owner,std::uint32_t actual_collector,std::uint32_t actual_body,std::uint32_t){check(owner==0x24df20u&&actual_collector==collector&&actual_body==body,"Value forward owner differs");native_instanced_ray_callback_detail::Collector data;native_instanced_ray_callback_detail::collect(values,data);};
                const auto packet=((sp-4u)&0xfffffff0u)-0x20u;
                native_instanced_ray_callback_detail::ForwardValues forward{words,publish,invoke,collector,body,packet};native_instanced_ray_callback_detail::Forward data;native_instanced_ray_callback_detail::forward(forward,data);
            }else{native_instanced_ray_callback_detail::Collector data;native_instanced_ray_callback_detail::collect(values,data);}
        }catch(const Boundary&){interrupted=true;}
        check(interrupted==!row.at("returned").get<bool>(),"Value return/query boundary differs");
        for(const auto&bank:row.at("banks"))if(bank.at("address")!=0x62000000u)check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Value persistent DATA differs");
        ++value_checks;
    }
    for(const auto&row:fixture.at("cases"))for(bool hostile:{false,true})for(bool observe:{false,true}){
        const auto phase=row.at("phase").get<std::string>();
        if(phase!="normal"&&observe!=(phase=="after_write"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.observer_preimages=observe;memory.hostile_fp=hostile;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned observed_writes{};
        if(observe)observer=[&](const auto&event){
            const auto&expected=row.at("writes").at(observed_writes);
            check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&
                  hex(event.before)==expected.at("before").template get<std::string>()&&
                  hex(event.written)==expected.at("written").template get<std::string>(),"Callback observer differs");
            const auto ordinal=observed_writes++;if(hostile)clobber_fp();
            if(phase=="after_write"&&ordinal==row.at("native_fault_ordinal").get<unsigned>())throw Boundary{};
        };
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
        incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],v.data(),10);}state.load(incoming);
        J observed=J::array();auto before=[&](auto&s,std::uint32_t pc){
            memory.pc=pc;
            if(sites.contains(pc))observed.push_back(packet(s,pc));
            if(phase=="before_instruction"&&pc==row.at("native_fault_pc"))throw Boundary{};
            if(hostile)clobber_fp();
        };
        bool interrupted{};
        std::function<void(WorldState&,std::uint32_t,std::uint32_t,bool)>dispatch;
        dispatch=[&](auto&s,std::uint32_t target,std::uint32_t,bool){
            if(target==0x24df20u){native_instanced_ray_callback_detail::collect_retained(s,before,dispatch);return;}
            if(target==0x16c000u){memory.pc=target;throw Boundary{};}
            throw std::runtime_error("Unprovided real instanced collector owner");
        };
        try{
            if(row.at("entry")==0x24def0u)native_instanced_ray_callback_detail::forward_retained(state,before,dispatch);
            else native_instanced_ray_callback_detail::collect_retained(state,before,dispatch);
        }catch(const Boundary&){interrupted=true;}
        check(interrupted==!row.value("returned",false),"Callback return/boundary differs");
        const auto name=row.at("name").get<std::string>();
        check(observed==row.at("calls"),name+": callback complete packets differ actual="+observed.dump()+" expected="+row.at("calls").dump());
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        auto actual=packet(state,memory.pc);
        check(actual==expected,name+": callback boundary ABI differs actual="+actual.dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads")&&memory.writes==row.at("writes"),name+": callback ordered IO differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": callback bank differs");
        ++comparisons;packets+=observed.size();
    }
    const auto report=J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"production_binding",false},{"ordinary_shared_value_cases",value_checks},{"scope","Native forward/collector duplicate returns or genuine16C000 entries; positive query returns unqualified"}};std::cout<<report.dump()<<'\n';if(argc==3){std::ofstream out(argv[2]);out<<report.dump()<<'\n';check(bool(out),"Callback native report write failed");}return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
