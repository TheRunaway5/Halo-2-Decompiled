#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_segment_sphere.hpp"
#include "halo2/native_tu15_segment_sphere_value.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
struct InstrumentedState:WorldState {
 using WorldState::WorldState;unsigned literal_calls{};std::function<void(std::uint32_t)>literal_probe;
 void instruction_literal(std::uint32_t pc){++literal_calls;WorldState::instruction_literal(pc);if(literal_probe)literal_probe(pc);}
};
#ifdef HALO2_SEGMENT_SPHERE_SOURCE_COPIES
namespace copies {using WorldState=InstrumentedState;
#include "sphere_bodies.inc"
}
namespace values {
#include "sphere_values.inc"
}
#endif
namespace {
struct Boundary {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto value=std::to_integer<unsigned>(b);out+=digits[value>>4];out+=digits[value&15];}return out;}
void clobber_fp(){
    asm volatile("fninit; fld1; fldz; fdivp; fld1;pxor %%mm0,%%mm0;pxor %%mm1,%%mm1;pxor %%mm2,%%mm2;pxor %%mm3,%%mm3;pxor %%mm4,%%mm4;pxor %%mm5,%%mm5;pxor %%mm6,%%mm6;pxor %%mm7,%%mm7":::"memory","st","st(1)","mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};std::function<void()>before_read,before_write;mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(before_read)before_read();
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
        if(before_write)before_write();
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
struct GuardState:WorldState {using Base=physics_world_runtime_detail::WorldState;using Base::Base;bool selected_observation_pcs{true};};
unsigned refusals(bool reference_only){
 unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x16b810u,0x33b595u};
 for(unsigned kind=0;kind<(reference_only?1u:25u);++kind){
  Memory memory;unsigned instructions{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](auto){++instructions;};auto&cursor=services.instruction_cursor;
  cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  SourcePhysicsContactRecordObserver observe;if(kind==14||kind==15||kind==24)observe=[](const auto&){};
  if(!reference_only&&kind==14)cursor={};
  if(!reference_only&&kind==21)cursor.literal_service_sites=false;
  GuardState state{memory,observe,services,!reference_only&&kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(input);
  if(!reference_only){
   if(kind==1){state.active_mmx=SourcePhysicsMmxAlias{};state.fp.count=1;}
   if(kind==2)state.fp.count=9;if(kind==3)state.fp.top=8;
   if(kind==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}
   if(kind==5)_mm_setcsr(0x1f00);
   if(kind==6)cursor={};if(kind==7)cursor.position=nullptr;
   if(kind==8)cursor.executor={};if(kind==9)cursor.callback_type=&typeid(int);
   if(kind==10)first=true;if(kind==11)second=true;
   if(kind==12)cursor.service_sites=changed;if(kind==13)cursor.literal_service_sites=false;
   if(kind==15)cursor.callback_type=&typeid(int);
   if(kind==16){const auto original=services.instruction;services.instruction=[original](auto pc){original(pc);};}
   if(kind==17)state.selected_observation_pcs=false;
   if(kind==18)state.fp.status=0x80;if(kind==19)state.fp.status=0x8000;
   if(kind==20)state.instruction_cursor=nullptr;
   
   if(kind==22)cursor.service_sites={};
   if(kind==23)services.instruction={};if(kind==24)cursor.observation_only_stores=false;
  }
  const auto hardware=native_raw_x87_detail::capture();const auto gp=state.gp;const auto flags=state.flags.bits();const auto depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);memory.pc=0xdeadbeefu;
  check(!native_segment_sphere_detail::bridge(state),"Unqualified segment sphere view admitted");check(gp==state.gp&&flags==state.flags.bits()&&depth==state.fp.count&&top==state.fp.top&&status==state.fp.status&&!std::memcmp(xmm.data(),state.xmm,128),"Segment sphere refusal changed retained state");check(memory.pc==0xdeadbeefu&&memory.operations.empty()&&!instructions&&native_raw_x87_detail::equivalent(hardware,native_raw_x87_detail::capture()),"Segment sphere refusal changed hardware/protocol");++count;
  asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
 }
 return count;
}}
int main(int argc,char**argv)try{
    if(argc==2&&std::string_view(argv[1])=="--refusals"){std::cout<<J{{"refusals",refusals(false)}}.dump()<<'\n';return 0;}
    if(argc==2&&std::string_view(argv[1])=="--reference-guard"){std::cout<<J{{"reference_refusals",refusals(true)}}.dump()<<'\n';return 0;}
    check(argc==2||argc==3,"Supply actual120FA0 fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong segment sphere original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{},observer_events{},public_checks{},default_calls{},reference_calls{},reference_callbacks{},reference_literals{};
    const bool forced=std::getenv("HALO2_SPHERE_REFERENCE_TEST");const auto guard_checks=refusals(forced);
    for(const auto&row:fixture.at("cases"))for(unsigned route=forced?2u:0u;route<
#ifdef HALO2_SEGMENT_SPHERE_SOURCE_COPIES
6
#else
2
#endif
;++route)for(bool observed:{false,true})for(bool hostile:{false,true}){
        if(forced&&(hostile||row.at("phase")!="normal"))continue;
        const auto phase=row.at("phase").get<std::string>();if(phase!="normal"&&observed!=(phase=="after_observer"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.hostile_fp=hostile;memory.observer_preimages=observed;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");unsigned event_count{};
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{};
        J observed_packets=J::array();std::set<std::uint32_t>seen;InstrumentedState*current{};
        const auto capture=[&](std::uint32_t pc){if(current&&sites.contains(pc)&&seen.insert(pc).second)observed_packets.push_back(packet(*current,pc));};
        bool first{},second{};constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
        if(route){services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;if(forced)capture(pc);};services.instruction_cursor={&memory.pc,&first,&second,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};}
        if(observed)observer=[&](const auto&event){++observer_events;if(phase=="after_observer"&&event_count++==row.at("native_fault_ordinal").get<unsigned>()){if(hostile)clobber_fp();throw Boundary{};}if(hostile)clobber_fp();};
        InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],value.data(),10);}
        if(row.contains("active_mmx")){SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto value=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),value.data(),10);}incoming.active_mmx=alias;}
        state.load(incoming);current=&state;if(forced)state.literal_probe=capture;memory.before_read=[&]{capture(memory.pc);};memory.before_write=[&]{capture(memory.pc);};
        auto before=[&](auto&s,std::uint32_t pc){memory.pc=pc;if(hostile)clobber_fp();};
        bool interrupted{};try{switch(route){
        case 0:native_segment_sphere_detail::run_private(state,before);break;
        case 1:check(native_segment_sphere_detail::bridge(state),"Qualified private segment sphere bridge refused");break;
#ifdef HALO2_SEGMENT_SPHERE_SOURCE_COPIES
        case 2:copies::copy0::bound::run_120fa0(state);break;
        case 3:copies::copy0::prepared::run_120fa0(state);break;
        case 4:copies::copy1::bound::run_120fa0(state);break;
        case 5:copies::copy1::prepared::run_120fa0(state);break;
#endif
        }}catch(const Boundary&){interrupted=true;}check(interrupted==!row.at("returned").get<bool>(),"Segment sphere completion/fault differs");
        const auto name=row.at("name").get<std::string>()+" route="+std::to_string(route);
        if(forced){check(callbacks+state.literal_calls>0,"Reference segment sphere skipped source");if(route>=3&&(route&1u))check(state.literal_calls>0,"Prepared reference skipped literals");++reference_calls;reference_callbacks+=callbacks;reference_literals+=state.literal_calls;}
        else{check(!callbacks&&!state.literal_calls,"Native segment sphere evaluated callbacks/literals");if(route>=2)++default_calls;}
        check(observed_packets==row.at("calls"),name+": segment sphere complete packets differ actual="+observed_packets.dump()+" expected="+row.at("calls").dump());
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};if(row.contains("active_mmx"))expected["active_mmx"]=row.at("boundary_active_mmx");
        check(packet(state,memory.pc)==expected,name+": segment sphere final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads")&&memory.writes==row.at("writes"),name+": segment sphere ordered IO differs");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": segment sphere bank differs");
        ++comparisons;packets+=observed_packets.size();

#ifdef HALO2_SEGMENT_SPHERE_SOURCE_COPIES
        if(!forced&&route==0&&phase=="normal"&&!hostile&&!observed&&!row.contains("active_mmx")){
            std::vector<J>reads,writes;std::vector<std::map<std::uint32_t,std::vector<std::byte>>>banks;std::vector<unsigned>mxcsr;std::vector<bool>results;
            for(bool shared:{false,true}){
                Memory plain;for(const auto&bank:row.at("banks"))plain.banks.emplace(bank.at("address"),bytes(bank.at("before")));
                NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
                const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();const auto radius=row.at("matrix").get<std::uint32_t>();
                results.push_back(shared?values::public_shared::native_tu15_segment_near_sphere(plain,gp[0],gp[1],gp[2],radius,fp):values::public_reference::native_tu15_segment_near_sphere(plain,gp[0],gp[1],gp[2],radius,fp));
                reads.push_back(plain.architectural_reads());writes.push_back(plain.writes);banks.push_back(plain.banks);mxcsr.push_back(fp.mxcsr);
            }
            check(reads[0]==reads[1]&&writes[0]==writes[1]&&banks[0]==banks[1]&&mxcsr[0]==mxcsr[1]&&results[0]==results[1],name+": old-public/shared DATA regression");++public_checks;
        }
#endif

    }
    const auto report=J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"observer_events",observer_events},{"public_value_regressions",public_checks},{"guard_checks",guard_checks},{"default_calls",default_calls},{"reference_calls",reference_calls},{"reference_callbacks",reference_callbacks},{"reference_literals",reference_literals}};
    std::cout<<report.dump()<<'\n';if(argc==3){std::ofstream out(argv[2]);out<<report.dump()<<'\n';check(bool(out),"Segment sphere report failed");}return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
