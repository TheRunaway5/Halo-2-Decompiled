#include "runtime/native_tu15_diagnostic_stream_admission.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
struct InstrumentedState:physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    bool selected_observation_pcs{true};
};
using State=InstrumentedState;
using J=nlohmann::json;
struct GenuineBoundary{};
struct Fault{};
struct RetainedStop{};
unsigned retained_statements{};bool stop_at_retained{};
void retained_statement(){++retained_statements;if(stop_at_retained)throw RetainedStop{};}
unsigned retained_child_samples{};
void retained_child_statement(){++retained_child_samples;}
namespace retained {
using namespace physics_world_runtime_detail;
using State=physics_world_runtime_detail::WorldState;
inline std::uint32_t canonical_branch(std::uint32_t pc,SourcePhysicsContactImage){return pc;}
#include "diagnostic_formatter_bodies.inc"
}
namespace copies {
using namespace physics_world_runtime_detail;
using State=::State;
using WorldState=::State;
std::function<void(State&,std::uint32_t,std::uint32_t,bool)>source_child;
static void dispatch(State&s,std::uint32_t target,std::uint32_t site,bool tail){source_child(s,target,site,tail);}
#include "diagnostic_source_bodies.inc"
}
namespace {
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J packet(State&s,std::uint32_t pc){
    auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},
        {"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    mutable std::uint32_t pc{};std::set<std::uint32_t>packet_sites,write_sites;
    mutable J reads=J::array(),writes=J::array(),packets=J::array();std::function<J()>capture;
    std::byte*at(std::uint32_t address,std::size_t size)const{
        auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unbacked diagnostic memory address="+std::to_string(address)+" pc="+std::to_string(pc));--it;
        check(std::uint64_t(address-it->first)+size<=it->second.size(),"Diagnostic backing extent exceeded address="+std::to_string(address)+" pc="+std::to_string(pc));
        return const_cast<std::byte*>(it->second.data())+(address-it->first);
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        // This first private seam has no observer/preimage reads. Retained
        // formatter RMW operands remain genuine architectural reads.
        reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
        std::memcpy(out.data(),at(address,out.size()),out.size());
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        const auto before=hex(std::span(at(address,in.size()),in.size()));
        std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});
    }
};
void source_owner(State&s,std::uint32_t entry){
    if(entry==0x2e3080u)copies::run_2e3080(s);
    else if(entry==0x2e30e0u)copies::run_2e30e0(s);
    else if(entry==0x2e2d10u)copies::run_2e2d10(s);
    else copies::run_2e2e00(s);
}
unsigned guards(bool forced){
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x167efbu,0x33b595u};constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x100u};
    for(auto entry:{0x2e3080u,0x2e30e0u,0x2e2d10u,0x2e2e00u})for(unsigned policy=0;policy<(forced?1u:25u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool first{},second{};
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        auto detached=services.instruction_cursor;
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(input);
        if(!forced){
            if(policy==1)state.active_mmx=SourcePhysicsMmxAlias{};if(policy==2)state.fp.count=9;if(policy==3)state.fp.top=8;
            if(policy==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}if(policy==5)_mm_setcsr(0x1f00);
            if(policy==6)services.instruction={};if(policy==7){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
            if(policy==8)services.instruction_cursor.executor={};if(policy==9)services.instruction_cursor.callback_type=nullptr;if(policy==10)services.instruction_cursor.callback_type=&typeid(int);
            if(policy==11)services.instruction_cursor.position=nullptr;if(policy==12)state.instruction_cursor=nullptr;if(policy==13)state.instruction_cursor=&detached;
            if(policy==14)services.instruction_cursor.literal_service_sites=false;if(policy==15)services.instruction_cursor.service_sites={};if(policy==16)services.instruction_cursor.service_sites=changed;if(policy==17)services.instruction_cursor.service_sites=extra;
            if(policy==18)first=true;if(policy==19)second=true;
            if(policy==20){observe=[&](const auto&){++stores;};services.instruction_cursor.observation_only_stores=false;}
            if(policy==21)state.selected_observation_pcs=false;if(policy==22)state.fp.status=0x80;if(policy==23)state.fp.status=0x8000;
            if(policy==24){observe=[&](const auto&){++stores;};services.instruction={};}
        }
        const auto gp=state.gp;const auto flags=state.flags.bits();const auto depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        const auto mmx=state.active_mmx;
        std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto host=native_raw_x87_detail::capture();memory.pc=0xdeadbeefu;
        bool stopped{};retained_statements=0;stop_at_retained=true;
        try{source_owner(state,entry);}catch(const RetainedStop&){stopped=true;}stop_at_retained=false;
        check(stopped&&retained_statements==1,"Unqualified diagnostic policy skipped the untouched first retained statement entry="+std::to_string(entry)+" policy="+std::to_string(policy));
        check(state.gp==gp&&state.flags.bits()==flags&&state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&!std::memcmp(xmm.data(),state.xmm,128),"Diagnostic fallback altered CPU state");
        check(bool(state.active_mmx)==bool(mmx)&&(!mmx||(state.active_mmx->physical==mmx->physical&&state.active_mmx->known_mask==mmx->known_mask)),"Diagnostic fallback altered opaque MMX state");
        check(memory.pc==0xdeadbeefu&&!callbacks&&!stores&&memory.reads.empty()&&memory.writes.empty()&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Diagnostic fallback executed source IO/callbacks or changed host FP");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){
        std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;
    }
    const bool empty_service=argc==3&&(std::string_view(argv[1])=="--source-empty-default"||std::string_view(argv[1])=="--source-empty-reference");
    const bool source_mode=argc==3&&(empty_service||std::string_view(argv[1])=="--source-default"||std::string_view(argv[1])=="--source-reference");
    const bool reference_mode=source_mode&&(std::string_view(argv[1])=="--source-reference"||std::string_view(argv[1])=="--source-empty-reference");
    check(argc==2||source_mode,"Supply genuine diagnostic fixture");std::ifstream file(argv[source_mode?2:1]);const auto fixture=J::parse(file);
    unsigned comparisons{},packets{},formatter_calls{},sink_boundaries{},flush_calls{},source_statements{},own_callbacks{},child_samples{};
    for(const auto&row:fixture.at("cases")){
        if(source_mode&&row.contains("fault"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
        memory.banks.emplace(fixture.at("immutable_text").at("address"),unhex(fixture.at("immutable_text").at("bytes")));
        for(auto pc:fixture.at("packet_sites"))memory.packet_sites.insert(pc.get<std::uint32_t>());
        for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        if(!empty_service)services.instruction=[&](auto pc){memory.pc=pc;if(memory.packet_sites.contains(pc))++own_callbacks;};
        bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        if(source_mode&&!empty_service)services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}state.load(input);
        unsigned occurrence{};
        memory.capture=[&]{return packet(state,memory.pc);};auto before=[&](auto&,auto pc){
            memory.pc=pc;if(memory.packet_sites.contains(pc))memory.packets.push_back(packet(state,pc));
            if(row.contains("fault")&&pc==row.at("fault").at("instruction")&&occurrence++==row.at("fault").at("ordinal"))throw Fault{};
        };
        auto call=[&](auto&s,std::uint32_t target,std::uint32_t,bool){
            memory.pc=target;
            // The original observer stops before its call-tracing hook here.
            // Its complete final ABI still authenticates this genuine entry.
            if(target==0x2e1c80u){++sink_boundaries;throw GenuineBoundary{};}
            memory.packets.push_back(packet(s,target));
            if(target==0x32ba80u){++formatter_calls;retained::run_32ba80(s);return;}
#if HALO2_TEST_GENUINE_BUFFERED_WRITER
            if(target==0x2e3db0u){retained::run_2e3db0(s);return;}
            if(target==0x2e3e70u){retained::run_2e3e70(s);return;}
#endif
            if(target==0x31d400u){++flush_calls;if(s.services.instruction)s.services.instruction(target);s.pop();return;}
            throw std::runtime_error("Unexpected genuine diagnostic child "+std::to_string(target));
        };
        bool boundary{},faulted{};retained_statements=0;retained_child_samples=0;copies::source_child=call;
        try{
            const auto entry=row.at("entry").get<std::uint32_t>();
            if(source_mode)source_owner(state,entry);
            else if(entry==0x2e3080u)native_diagnostic_stream_detail::print(state,call,before);
            else if(entry==0x2e30e0u)native_diagnostic_stream_detail::flush(state,call,before);
            else if(entry==0x2e2d10u)native_diagnostic_stream_detail::format_tail(state,call,before);
            else native_diagnostic_stream_detail::measure(state,before);
        }catch(const GenuineBoundary&){boundary=true;}catch(const Fault&){faulted=true;}
        const auto name=row.at("name").get<std::string>();
        check(boundary==row.contains("stop_before_entries"),name+": genuine boundary ownership changed");
        check(faulted==row.contains("fault"),name+": original before-instruction prefix changed");
        if(source_mode){check(reference_mode?retained_statements>0:retained_statements==0,name+": actual source native/reference body ownership changed");source_statements+=retained_statements;}
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},
            {"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};
        const auto final_packet=packet(state,empty_service?row.at("expected_pc").get<std::uint32_t>():memory.pc);
        check(final_packet==expected,name+": final ABI differs actual="+final_packet.dump()+" expected="+expected.dump());
        auto expected_reads=row.at("reads"),expected_writes=row.at("writes");
        if(empty_service){for(auto*events:{&memory.reads,&memory.writes,&expected_reads,&expected_writes})for(auto&event:*events)event.erase("instruction");}
        if(memory.reads!=expected_reads){
            unsigned index{};while(index<memory.reads.size()&&index<expected_reads.size()&&memory.reads[index]==expected_reads[index])++index;
            throw std::runtime_error(name+": ordered read"+std::to_string(index)+" differs actual="+(index<memory.reads.size()?memory.reads[index].dump():"END")+" expected="+(index<expected_reads.size()?expected_reads[index].dump():"END"));
        }
        check(memory.writes==expected_writes,name+": ordered stores differ");
        J expected_calls=row.at("calls");
        if(source_mode){expected_calls=J::array();for(const auto&event:row.at("calls"))if(std::set<std::uint32_t>{0x32ba80u,0x31d400u,0x2e3db0u,0x2e3e70u}.contains(event.at("entry")))expected_calls.push_back(event);}
        if(memory.packets!=expected_calls){
            unsigned index{};while(index<memory.packets.size()&&index<expected_calls.size()&&memory.packets[index]==expected_calls[index])++index;
            throw std::runtime_error(name+": parent pre-I/O/child-entry packet"+std::to_string(index)+" differs actual="+(index<memory.packets.size()?memory.packets[index].dump():"END")+" expected="+(index<expected_calls.size()?expected_calls[index].dump():"END"));
        }
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": banks differ");
        packets+=memory.packets.size();child_samples+=retained_child_samples;++comparisons;
    }
    if(source_mode&&!reference_mode)check(!own_callbacks,"Compiled native diagnostic source executed parent callbacks");
    check(child_samples>0,"Genuine formatter/stream reference children did not execute measured retained statements");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"parent_pre_io_and_real_child_packets",packets},{"source_retained_statements",source_statements},{"source_own_callbacks",own_callbacks},{"retained_child_statement_samples_lower_bound",child_samples},{"instruction_cursor_observed",!empty_service},
        {"genuine_retained_formatter_returns",formatter_calls},{"unresumed_genuine_sink_entries",sink_boundaries},
        {"genuine_ret0_flush_returns",flush_calls}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
