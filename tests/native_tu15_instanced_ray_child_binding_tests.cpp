#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_instanced_ray_child.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
#include "instanced_ray_child_sources.inc"
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
    std::function<void()>capture;
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(capture)capture();
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
        if(capture)capture();
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
unsigned admission_guards(){
    for(unsigned kind=0;kind<22;++kind){
        Memory memory;std::uint32_t pc{};bool first=kind==4,second=kind==5;SourcePhysicsWorldRuntimeServices services;
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};constexpr std::array extra_sites{0x38846eu,0x21f590u,0x33b595u,0x24decbu};
        if(kind!=21)services.instruction=[&](auto value){pc=value;};auto&cursor=services.instruction_cursor;
        cursor={&pc,&first,&second,sites,std::this_thread::get_id(),services.instruction?&services.instruction.target_type():nullptr,true,true};
        if(kind==1)cursor.position=nullptr;if(kind==2)cursor.executor={};if(kind==3)cursor.callback_type=&typeid(int);
        if(kind==6)cursor.observation_only_stores=false;if(kind==7)cursor.service_sites={};if(kind==20)cursor.literal_service_sites=false;
        SourcePhysicsContactRecordObserver observe;if(kind!=15&&kind!=21)observe=[](const auto&){};
        struct Guarded:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
        Guarded state{memory,observe,services,kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi incoming{};incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(incoming);
        if(kind==8||kind==15)services.instruction={};if(kind==9)services.instruction=[&,changed=1u](auto value){pc=value+changed;};
        if(kind==10)cursor.executor={};if(kind==11)cursor.literal_service_sites=false;
        if(kind==12)state.fp.count=9;if(kind==13)state.fp.top=8;if(kind==14)state.active_mmx=SourcePhysicsMmxAlias{};
        auto detached=cursor;if(kind==16)state.instruction_cursor=&detached;if(kind==17)cursor.service_sites=extra_sites;
        if(kind==18)state.selected_observation_pcs=false;
        if(kind==21)observe=[](const auto&){};
        const auto gp=state.gp;const auto flags=state.flags.bits();const auto count=state.fp.count;const auto top=state.fp.top;const auto status=state.fp.status;const auto mxcsr=_mm_getcsr();
        if(kind==19)_mm_setcsr(0x1f00u);
        unsigned before{};const auto accepted=native_instanced_ray_child_binding_detail::bridge(state,[&](auto&,auto){++before;});
        if(kind==19)_mm_setcsr(mxcsr);
        check(!accepted&&before==0&&memory.operations.empty()&&pc==0&&state.gp==gp&&state.flags.bits()==flags&&
              state.fp.count==count&&state.fp.top==top&&state.fp.status==status,"Unsafe instanced-child contract changed source state");
    }
    return 22;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&std::string_view(argv[1])=="--guards"){std::cout<<J{{"passed",true},{"refusals",admission_guards()}}.dump()<<'\n';return 0;}
    check(argc==3,"Supply physical child fixture and route");const std::string route=argv[2];
    const auto*reference_value=std::getenv("HALO2_REFERENCE_INSTANCED_RAY_CHILD");const bool reference=reference_value&&*reference_value;
    std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong primitive-key original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(bool hostile:{false,true})for(bool observe:{false,true}){
        if(reference&&hostile)continue;
        const auto phase=row.at("phase").get<std::string>();
        if(phase!="child_boundary"&&observe!=(phase=="after_write"))continue;
        instanced_child_primary::literal_body_statements=0;instanced_child_ai::literal_body_statements=0;
        instanced_child_primary_prepared::literal_body_statements=0;instanced_child_ai_prepared::literal_body_statements=0;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.observer_preimages=observe;memory.hostile_fp=hostile;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned observed_writes{},callbacks{};
        constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};
        services.instruction_cursor={&memory.pc,nullptr,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(observe)observer=[&](const auto&event){
            const auto&expected=row.at("writes").at(observed_writes);
            check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&
                  hex(event.before)==expected.at("before").template get<std::string>()&&
                  hex(event.written)==expected.at("written").template get<std::string>(),"Primitive-key observer differs");
            const auto ordinal=observed_writes++;if(hostile)clobber_fp();
            if(phase=="after_write"&&ordinal==row.at("native_fault_ordinal").get<unsigned>())throw Boundary{};
        };
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
        incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],v.data(),10);}state.load(incoming);
        J observed=J::array();std::set<std::uint32_t>seen;
        memory.capture=[&]{if(sites.contains(memory.pc)&&seen.insert(memory.pc).second)observed.push_back(packet(state,memory.pc));};
        auto before=[&](auto&s,std::uint32_t pc){
            memory.pc=pc;if(pc==0x63000000u)throw Boundary{};
            if(hostile)clobber_fp();
        };
        bool interrupted{};
        try{
            if(route=="raw")native_instanced_ray_child_detail::resolve_retained(state,before);
            else if(route=="bridge")check(native_instanced_ray_child_binding_detail::bridge(state,before),"Qualified child bridge refused");
            else if(route=="primary")instanced_child_primary::run_24dec0(state);
            else if(route=="ai")instanced_child_ai::run_24dec0(state);
            else if(route=="primary_prepared")instanced_child_primary_prepared::run_24dec0(state);
            else if(route=="ai_prepared")instanced_child_ai_prepared::run_24dec0(state);
            else throw std::runtime_error("Unknown child source route");
            before(state,0x63000000u);
        }catch(const Boundary&){interrupted=true;}
        check(interrupted,"Primitive key did not reach declared boundary");
        const auto literal_statements=instanced_child_primary::literal_body_statements+instanced_child_ai::literal_body_statements+
            instanced_child_primary_prepared::literal_body_statements+instanced_child_ai_prepared::literal_body_statements;
        check(reference?literal_statements>0:literal_statements==0,"Child source default/reference retained body attribution differs");
        check(reference&&!route.ends_with("_prepared")?callbacks>0:callbacks==0,"Child source default/reference callback route differs");
        const auto name=row.at("name").get<std::string>();
        check(observed==row.at("calls"),name+": primitive-key complete packets differ actual="+observed.dump()+" expected="+row.at("calls").dump());
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        check(packet(state,memory.pc)==expected,name+": primitive-key boundary ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads")&&memory.writes==row.at("writes"),name+": primitive-key ordered IO differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": primitive-key bank differs");
        ++comparisons;packets+=observed.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"production_binding",false},{"scope","Genuine native instanced child provider; complete returns and retained I/O boundaries"}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
