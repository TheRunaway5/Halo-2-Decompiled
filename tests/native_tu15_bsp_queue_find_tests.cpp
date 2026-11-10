#include "rendering/native_tu15_bsp_queue_find.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using J=nlohmann::json;using State=physics_world_runtime_detail::WorldState;
struct InstrumentedState:State {using Base=physics_world_runtime_detail::WorldState;using Base::Base;bool selected_observation_pcs{true};unsigned literal_calls{};void instruction_literal(std::uint32_t pc){++literal_calls;Base::instruction_literal(pc);}};
struct RetainedStop{};unsigned retained_statements{};bool stop_at_retained{};
void retained_statement(){++retained_statements;if(stop_at_retained)throw RetainedStop{};}
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "bsp_queue_find_bodies.inc"
}
namespace {
struct Fault{};
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
J packet(State&s,std::uint32_t pc){const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;}
native_raw_x87_detail::Image host_image(const J&row,unsigned mode){
 auto image=native_raw_x87_detail::capture();const auto top=std::array{5u,3u,7u,2u,0u,6u,1u}[mode];
 image.word(4,std::uint16_t((row.at("before_fp")[1].get<unsigned>()&0x3fu)|0x4500u|(top<<11u)));image.word(8,0xffffu);
 const auto count=std::array{0u,1u,4u,8u,8u,0u,1u}[mode];std::uint16_t tags=0xffffu;
 for(unsigned n=0;n<8;++n){
  const auto raw=unhex(row.at("active_mmx").at("physical")[n]);std::memcpy(image.x87.data()+28+n*10,raw.data(),10);
  if(n<count)tags=std::uint16_t(tags&~(3u<<(((top+n)&7u)*2u)));
 }
 if(mode==5)tags=0xcf3fu;image.word(8,tags);return image;
}
void hostile(){const std::uint16_t unmasked=0x37e;asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(unmasked):"memory","st","st(1)");_mm_setcsr(0x8065);}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;mutable std::uint32_t pc{};mutable J reads=J::array(),writes=J::array(),packets=J::array();
    std::set<std::uint32_t>write_sites;std::function<J()>capture;std::optional<J>fault;bool noisy{};mutable unsigned read_count{},write_count{};
    std::byte*at(std::uint32_t address,std::size_t size)const{auto it=banks.upper_bound(address);check(it!=banks.begin(),"Unmapped queue lookup data");--it;check(std::uint64_t(address-it->first)+size<=it->second.size(),"Queue lookup data escaped backing");return const_cast<std::byte*>(it->second.data())+(address-it->first);}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        const bool preimage=write_sites.contains(pc);if(!preimage){packets.push_back(capture());if(fault&&fault->at("phase")=="before_read"&&read_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}++read_count;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});}
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{
        packets.push_back(capture());if(fault&&fault->at("phase")=="before_write"&&write_count==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        const auto before=hex(std::span(at(address,in.size()),in.size()));std::memcpy(at(address,in.size()),in.data(),in.size());
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",before},{"written",hex(in)}});++write_count;if(noisy)hostile();
    }
};
unsigned guards(bool forced){
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x167efbu,0x33b595u};constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x100u};
    for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<(forced?1u:24u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observer;if(policy==18||policy==19)observer=[&](const auto&){++stores;};
        InstrumentedState s{memory,observer,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(input);
        if(!forced){
            if(policy==1){s.active_mmx=SourcePhysicsMmxAlias{};s.fp.count=1;}if(policy==2)s.fp.count=9;if(policy==3)s.fp.top=8;
            if(policy==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}if(policy==5)_mm_setcsr(0x1f00);
            if(policy==6||policy==19)services.instruction={};
            if(policy==7){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
            if(policy==8)services.instruction_cursor.executor={};if(policy==9)services.instruction_cursor.callback_type=nullptr;
            if(policy==10)services.instruction_cursor.callback_type=&typeid(int);if(policy==11)services.instruction_cursor.position=nullptr;
            if(policy==12)s.instruction_cursor=nullptr;if(policy==13)services.instruction_cursor.literal_service_sites=false;
            if(policy==14)services.instruction_cursor.service_sites={};if(policy==15)services.instruction_cursor.service_sites=changed;
            if(policy==16)first=true;if(policy==17)second=true;if(policy==18)services.instruction_cursor.literal_service_sites=false;
            if(policy==20)s.selected_observation_pcs=false;if(policy==21)s.fp.status=0x80;if(policy==22)s.fp.status=0x8000;
            if(policy==23)services.instruction_cursor.service_sites=extra;
        }
        const auto gp=s.gp;const auto flags=s.flags.bits();const auto depth=s.fp.count,top=s.fp.top;const auto status=s.fp.status;const auto host=native_raw_x87_detail::capture();std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),s.xmm,128);memory.pc=0xdeadbeefu;
        retained_statements=0;stop_at_retained=true;bool stopped{};try{if(route)copies::prepared::run_166c00(s);else copies::bound::run_166c00(s);}catch(const RetainedStop&){stopped=true;}stop_at_retained=false;
        check(stopped&&retained_statements==1,"Unqualified policy skipped untouched first retained statement");
        check(s.gp==gp&&s.flags.bits()==flags&&s.fp.count==depth&&s.fp.top==top&&s.fp.status==status&&!std::memcmp(xmm.data(),s.xmm,128),"Admission fallback changed CPU state");
        check(memory.pc==0xdeadbeefu&&!callbacks&&!stores&&memory.reads.empty()&&memory.writes.empty()&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Admission fallback executed source I/O/callbacks");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){std::cout<<J{{"untouched_retained_fallbacks",guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;}
    check(argc==2,"Supply physical queue lookup aggregation fixture");std::ifstream file(argv[1]);const auto fixture=J::parse(file);unsigned comparisons{},packets{},calls{},reference_calls{};
    const bool forced=std::getenv("HALO2_BSP_QUEUE_FIND_REFERENCE_TEST");unsigned default_calls{},literal_statements{},native_statements{};
    for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?4u:0u;mode<6;++mode){
        if(forced&&row.contains("fault"))continue;
        
        const bool mmx=row.contains("active_mmx");const bool after_write=row.contains("fault")&&row.at("fault").at("phase")=="after_write";
        if(after_write&&mode!=2&&mode<4)continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
        memory.noisy=mode==1;if(row.contains("fault"))memory.fault=row.at("fault");SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned callbacks{};
        bool trace{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(mode==2||after_write)observer=[&](const auto&){if(memory.fault&&memory.fault->at("phase")=="after_write"&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};hostile();};
        InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];input.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<input.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}if(mmx){SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto raw=unhex(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),raw.data(),10);}input.active_mmx=alias;}state.load(input);retained_statements=0;
        auto caller=native_raw_x87_detail::capture();if(mmx&&!forced&&mode!=3){caller=host_image(row,mode);native_raw_x87_detail::restore(caller);caller=native_raw_x87_detail::capture();}
        memory.capture=[&]{return packet(state,memory.pc);};auto before=[&](auto&,auto pc){memory.pc=pc;if(mode==2)hostile();};bool faulted{};
        try{
            if(mode==4){copies::bound::run_166c00(state);++default_calls;}
            else if(mode==5){copies::prepared::run_166c00(state);++default_calls;}
            else if(mode==3){copies::retained::run_166c00(state);++reference_calls;}
            else{check(native_bsp_queue_find_detail::bridge(state,before),"Private queue lookup aggregation refused");++calls;}
        }catch(const Fault&){faulted=true;}
        const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
        check(faulted==row.contains("fault"),name+": fault completion changed");if(mode==3||forced){check(retained_statements>0,"Reference route skipped original body");literal_statements+=retained_statements;}else{native_statements+=retained_statements;check(!callbacks&&!state.literal_calls&&!retained_statements,name+": native evaluated retained statements/callbacks");}
        auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};if(mmx)expected["active_mmx"]=row.at("boundary_active_mmx");
        check(packet(state,memory.pc)==expected,name+": final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.packets.size()==row.at("calls").size(),name+": pre-I/O packet count changed");
        for(unsigned index=0;index<memory.packets.size();++index){
            const auto&a=memory.packets[index];const auto&e=row.at("calls")[index];
            const bool equal=a==e;
            check(equal,name+": pre-I/O packet"+std::to_string(index)+" differs actual="+a.dump()+" expected="+e.dump());
        }
        check(memory.reads==row.at("reads"),name+": ordered reads differ");check(memory.writes==row.at("writes"),name+": ordered stores differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backed bytes differ");++comparisons;packets+=memory.packets.size();
        if(mmx){if(!forced&&mode!=3){auto wanted=caller;wanted.word(0,row.at("after_fp")[0]);wanted.word(4,std::uint16_t((row.at("after_fp")[1].get<unsigned>()&~0x3800u)|(caller.word(4)&0x3800u)));wanted.mxcsr=row.at("after_fp")[2];const auto actual=native_raw_x87_detail::capture();check(actual.word(0)==wanted.word(0)&&actual.word(4)==wanted.word(4)&&actual.word(8)==wanted.word(8)&&actual.mxcsr==wanted.mxcsr&&!std::memcmp(actual.x87.data()+28,wanted.x87.data()+28,80),name+": opaque caller FP changed");}asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);}

    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"native_calls",calls},{"positive_reference_calls",reference_calls},{"installed_source_calls",default_calls},{"retained_statements",literal_statements},{"native_retained_statements",native_statements}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
