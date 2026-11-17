#include "gameplay/weapons/native_tu15_object_parent_mode_admission.hpp"
#include "halo2/native_tu15_object_parent_mode_value.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "halo2/native_campaign_object_motion_refs_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
unsigned parent_statements{};bool parent_stop{};SourcePhysicsWorldRuntimeAbi*parent_current_abi{};
struct RetainedStop{};
void parent_mode_enter(SourcePhysicsWorldRuntimeAbi&a){parent_current_abi=&a;}
void parent_mode_statement(){++parent_statements;if(parent_stop)throw RetainedStop{};}
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
#include "parent_mode_sources.inc"
namespace frozen_parent_mode {bool native_campaign_object_pickup_parent_tu15(const SourcePhysicsContactMemory&,std::uint32_t);}
#endif
namespace {
struct Fault{};
void check(bool condition,const std::string&why){if(!condition)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){
    std::vector<std::byte>out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte>data){
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:data){const auto n=std::to_integer<unsigned>(value);out+=digits[n>>4u];out+=digits[n&15u];}
    return out;
}
void hostile(){
    const std::uint16_t cw=0x37e;
    asm volatile("fninit;fldz;fldz;fdivp;fldcw %0"::"m"(cw):"memory","st","st(1)");_mm_setcsr(0x8065);
}
J packet(const SourcePhysicsWorldRuntimeAbi&a,std::uint32_t pc){
    J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    J out{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},
        {"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
    if(a.active_mmx){
        unsigned tags{};J physical=J::array();
        for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tags|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}
        out["active_mmx"]=J{{"tag_word",tags},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};
    }
    return out;
}
J packet(const State&state,std::uint32_t pc){return packet(state.save(),pc);}
struct Memory:SourcePhysicsContactMemory{
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::function<J()>capture;std::optional<J>fault;bool noisy{};
    mutable unsigned ordinal{};mutable std::uint32_t pc{};
    mutable J reads=J::array(),packets=J::array();
    const std::byte*at(std::uint32_t address,std::size_t width)const{
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Unbacked parent-mode read");--found;
        check(std::uint64_t(address-found->first)+width<=found->second.size(),"Parent-mode bank extent");
        return found->second.data()+address-found->first;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        packets.push_back(capture());
        if(fault&&ordinal==fault->at("ordinal")){if(noisy)hostile();throw Fault{};}
        ++ordinal;reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",out.size()}});
        std::memcpy(out.data(),at(address,out.size()),out.size());if(noisy)hostile();
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Unexpected parent-mode write");}
};
unsigned guards(bool forced){
    unsigned checks{};constexpr std::array good{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0xfc510u,0x33b595u};
    for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<(forced?1u:17u);++policy){
        if((policy==9&&route)||(policy==16&&!route))continue;
        Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;unsigned callbacks{};bool trace{};
        services.instruction=[&](auto){++callbacks;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,good,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(policy==9)observe=[](const auto&){};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={1,2,3,4,0x62000100,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
        if(!forced)switch(policy){
        case 0:services.instruction={};break;
        case 1:services.instruction=[](std::uint32_t){};break;
        case 2:services.instruction_cursor.position=nullptr;break;
        case 3:services.instruction_cursor.callback_type=nullptr;break;
        case 4:services.instruction_cursor.executor={};break;
        case 5:services.instruction_cursor.literal_service_sites=false;break;
        case 6:services.instruction_cursor.service_sites=wrong;break;
        case 7:trace=true;break;
        case 8:services.instruction_cursor.callback_type=&typeid(int);break;
        case 9:services.instruction_cursor.observation_only_stores=false;break;
        case 10:state.fp.count=9;abi.x87_depth=9;break;
        case 11:state.fp.status|=0x80;abi.x87_status|=0x80;break;
        case 12:state.fp.status|=0x8000;abi.x87_status|=0x8000;break;
        case 13:state.active_mmx=SourcePhysicsMmxAlias{};state.fp.count=1;abi.active_mmx=SourcePhysicsMmxAlias{};abi.x87_depth=1;break;
        case 14:if(route)abi.floating_point->x87_control_word=0x37e;else{const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw):"memory");}break;
        case 15:if(route)abi.floating_point->mxcsr=0x1f00;else _mm_setcsr(0x1f00);break;
        case 16:abi.floating_point.reset();break;
        }
        const auto gp=state.gp;const auto flags=state.other_flags|state.flags.bits();const auto before=abi;
        const auto depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        memory.pc=0xdeadbeefu;const auto host=native_raw_x87_detail::capture();
        const bool accepted=route?native_object_parent_mode_detail::effect(memory,abi,services):native_object_parent_mode_detail::bridge(state);
        check(!accepted&&state.gp==gp&&(state.other_flags|state.flags.bits())==flags&&abi.gp==before.gp&&abi.eflags==before.eflags,"Parent-mode refusal changed GP/flags");
        check(state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&abi.x87_depth==before.x87_depth&&abi.x87_status==before.x87_status&&abi.xmm==before.xmm,"Parent-mode refusal changed FP/SIMD metadata");
        check(!callbacks&&memory.reads.empty()&&memory.pc==0xdeadbeefu&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Parent-mode refusal performed I/O or changed host FP");
        ++checks;asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return checks;
}
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
unsigned source_guards(){
    unsigned checks{};constexpr std::array good{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0xfc510u,0x33b595u};
    for(unsigned route=0;route<3;++route)for(unsigned policy=0;policy<10;++policy){
        Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;bool trace{};unsigned callbacks{};
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,good,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000100,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
        switch(policy){
        case 0:services.instruction={};break;
        case 1:services.instruction=[](std::uint32_t){};break;
        case 2:services.instruction_cursor.position=nullptr;break;
        case 3:services.instruction_cursor.callback_type=&typeid(int);break;
        case 4:services.instruction_cursor.executor={};break;
        case 5:services.instruction_cursor.literal_service_sites=false;break;
        case 6:services.instruction_cursor.service_sites=wrong;break;
        case 7:trace=true;break;
        case 8:services.instruction_cursor.callback_type=nullptr;break;
        case 9:services.instruction_cursor.second_trace=&trace;trace=true;break;
        }
        memory.pc=0xdeadbeefu;parent_stop=true;parent_statements=0;parent_current_abi=nullptr;bool stopped{};
        const auto host=native_raw_x87_detail::capture();
        try{if(route==0)parent_source::raw::run_fc510(state);else if(route==1)parent_source::prepared::run_fc510(state);else native_weapon_effect_dispatch_tu15_run(memory,0xfc510u,abi,services,observe);}
        catch(const RetainedStop&){stopped=true;}
        parent_stop=false;parent_current_abi=nullptr;
        check(stopped&&parent_statements==1&&!callbacks&&memory.reads.empty()&&memory.pc==0xdeadbeefu,"Actual parent source did not reach untouched retained fallback");
        check(state.gp==abi.gp&&(state.other_flags|state.flags.bits())==abi.eflags&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Actual parent source fallback changed entry state");++checks;
    }
    return checks;
}
#endif
}
int main(int argc,char**argv)try{
    if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guard")){
        std::cout<<J{{"passed",true},{"untouched_refusals",guards(std::string_view(argv[1])=="--reference-guard")}}.dump()<<'\n';return 0;
    }
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
    if(argc==2&&std::string_view(argv[1])=="--source-guards"){std::cout<<J{{"passed",true},{"actual_source_fallbacks",source_guards()}}.dump()<<'\n';return 0;}
#endif
    const bool empty_services=argc==3&&std::string_view(argv[1])=="--empty-services";
    check(argc==2||empty_services,"Supply parent-mode original fixture");std::ifstream input_file(argv[empty_services?2:1]);const auto fixture=J::parse(input_file);
    unsigned comparisons{},packets{},value_cases{},source_calls{},reference_statements{};
    const bool forced=std::getenv("HALO2_OBJECT_PARENT_REFERENCE_TEST");
    const auto modes=
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
        6u;
#else
        3u;
#endif
    for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?3u:empty_services?2u:0u;mode<modes;++mode){
        if(forced&&row.contains("fault"))continue;
        if(mode>=2&&row.at("image")!="TitleUpdate15")continue;
        const bool noisy=!forced&&mode!=0;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault");memory.noisy=noisy;
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;unsigned callbacks{};
        bool trace{};constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
        if(!empty_services){
            services.instruction=[&](auto pc){++callbacks;memory.pc=pc;};
            services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        }
        const auto image=row.at("image")=="TitleUpdate15"?SourcePhysicsContactImage::TitleUpdate15:SourcePhysicsContactImage::MapPack11;
        State state{memory,observe,services,image};SourcePhysicsWorldRuntimeAbi input{};
        input.gp=row.at("gp");input.xmm=row.at("xmm");input.eflags=row.at("eflags");input.fs_base=0x5a000000u|(input.gp[1]&65535u);
        input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_status=row.at("before_fp")[1];
        input.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<input.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&input.x87[n],raw.data(),10);}
        if(row.contains("active_mmx")){
            SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),raw.data(),10);}input.active_mmx=alias;
        }
        state.load(input);auto effect_abi=input;parent_current_abi=nullptr;parent_statements=0;
        memory.capture=[&]{return parent_current_abi?packet(*parent_current_abi,memory.pc):mode==2||mode==3?packet(effect_abi,memory.pc):packet(state,memory.pc);};
        bool stopped{};
        try{
            if(mode==2)check(native_object_parent_mode_detail::effect(memory,effect_abi,services),"Actual parent-mode ABI candidate refused");
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
            else if(mode==3)native_weapon_effect_dispatch_tu15_run(memory,0xfc510u,effect_abi,services,observe);
            else if(mode==4)parent_source::raw::run_fc510(state);
            else if(mode==5)parent_source::prepared::run_fc510(state);
#endif
            else native_object_parent_mode_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;if(noisy)hostile();},
                image==SourcePhysicsContactImage::TitleUpdate15?native_object_parent_mode_detail::Observation::actual:native_object_parent_mode_detail::Observation::canonical);
        }
        catch(const Fault&){stopped=true;}
        parent_current_abi=nullptr;if(mode==2||mode==3)state.load(effect_abi);
        const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
        check(stopped==row.contains("fault"),name+": completion differs");
        check(state.save().fs_base==input.fs_base,name+": invariant FS base changed");
        auto expected=row.at("expected"),reads=row.at("reads"),calls=row.at("calls");
        // With no instruction service, neither owner publishes an external PC.
        // Keep the independently observed GP/flags/FP/read packets unchanged.
        if(empty_services){expected["entry"]=0u;for(auto&read:reads)read["instruction"]=0u;for(auto&call:calls)call["entry"]=0u;}
        check(packet(state,memory.pc)==expected,name+": final ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.reads==reads,name+": ordered reads differ");
        if(!forced){check(!callbacks&&!parent_statements,name+": native ran retained statements");check(memory.packets==calls,name+": complete pre-I/O packets differ");}
        else{check(callbacks&&parent_statements,name+": actual source reference did not execute");reference_statements+=parent_statements;}
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": backed bytes changed");
        if(!forced)packets+=memory.packets.size();++comparisons;if(mode>=3)++source_calls;
        if(!stopped&&mode==0){
            unsigned index{};J reads=J::array();
            const auto result=native_object_parent_mode_detail::value([&](std::uint32_t address,unsigned width){
                const auto&read=row.at("reads")[index++];check(read.at("address")==address&&read.at("bytes")==width,"Value DATA read order differs");
                std::uint32_t value{};std::memcpy(&value,memory.at(address,width),width);return value;
            },input.gp[1],image==SourcePhysicsContactImage::TitleUpdate15?0x4ecbd0u:0x4e78d0u);
            check(index==4&&result==bool(row.at("expected").at("gp")[0].get<std::uint32_t>()&255u),"Typed parent-mode value differs");++value_cases;
#ifdef HALO2_OBJECT_PARENT_SOURCE_COPIES
            if(image==SourcePhysicsContactImage::TitleUpdate15){
                for(bool old:{false,true}){
                    Memory public_memory;public_memory.banks=memory.banks;public_memory.capture=[]{return J{};};
                    const auto actual=old?frozen_parent_mode::native_campaign_object_pickup_parent_tu15(public_memory,input.gp[1]):
                        native_campaign_object_pickup_parent_tu15(public_memory,input.gp[1]);
                    check(actual==result&&public_memory.reads.size()==4,"Real public predecessor/shared parent DATA differs");
                    for(unsigned n=0;n<4;++n)check(public_memory.reads[n]["address"]==row.at("reads")[n]["address"]&&public_memory.reads[n]["bytes"]==row.at("reads")[n]["bytes"],"Real public parent DATA read order differs");
                    ++value_cases;
                }
            }
#endif
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"typed_value_cases",value_cases},{"actual_source_calls",source_calls},{"positive_reference_statements",reference_statements},{"reference_full_pre_io_packet_parity",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
