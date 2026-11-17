#include "native_tu15_marker_owner_test_support.hpp"
#include "gameplay/weapons/native_tu15_marker_owner_admission.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "physics/vehicles/physics_vehicle_motion_internal.hpp"
#include <algorithm>
unsigned marker_statements{};bool marker_stop{},marker_required_active{};SourcePhysicsWorldRuntimeAbi*marker_live_abi{};
void marker_owner_statement(){++marker_statements;if(marker_stop)throw RetainedStop{};}
void marker_owner_enter(SourcePhysicsWorldRuntimeAbi&abi){marker_live_abi=&abi;}
void marker_child_enter(SourcePhysicsWorldRuntimeAbi&abi){
    check(marker_required_active,"FC510 bypassed its actual required-owner boundary");
    marker_live_abi=&abi;
}
#include "marker_owner_source0.inc"
#include "marker_owner_source1.inc"
#include "marker_owner_source2.inc"
#include "marker_owner_source3.inc"
namespace{
struct RequiredStop{};
struct ReadonlyRequiredContext{
    OwnerMemory*memory{};const SourcePhysicsWorldRuntimeServices*services{};
    const SourcePhysicsContactRecordObserver*observe{};const J*row{};
    SourcePhysicsWorldRuntimeAbi*effect_abi{};unsigned mode{};bool canonical{},entry_stop{};
    unsigned*owner_calls{},*child_entries{},*effect_aliases{};bool hostile_fp{};
};
// This exact fixture owner invokes only the genuine qualified FC510 child.
// Its counters/ABI transport are observable, but its body and child do not
// replace the caller's instruction/cursor/trace policy.
struct ReadonlyRequired{
    const ReadonlyRequiredContext*context{};
    void operator()(SourcePhysicsContactMemory&bank,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&callee,const SourcePhysicsContactRecordObserver&observer)const{
        const auto&c=*context;
        check(&bank==c.memory&&call.instruction==(c.canonical?0xb9bf9u:0xbad39u)&&call.target==0xfc510u&&!call.tail,"Inherited-marker required-owner descriptor differs");
        check(&observer==c.observe,"Inherited-marker required-owner observer identity differs");
        if(c.mode==1||c.mode==2){check(&callee==c.effect_abi,"Effect child lost caller ABI reference identity");++*c.effect_aliases;}
        const auto found=std::find_if(c.row->at("calls").begin(),c.row->at("calls").end(),[](const auto&entry){return entry.at("entry")==0xfc510u;});
        check(found!=c.row->at("calls").end()&&packet(callee,0xfc510u)==*found,"Complete actual required-owner entry differs");
        ++*c.owner_calls;++*c.child_entries;auto*previous=marker_live_abi;marker_live_abi=&callee;
        if(c.entry_stop){marker_live_abi=previous;throw RequiredStop{};}
        const bool previous_active=marker_required_active;marker_required_active=true;
        try{native_weapon_effect_dispatch_tu15_run(bank,call.target,callee,*c.services,observer);}
        catch(...){marker_live_abi=previous;marker_required_active=previous_active;if(c.hostile_fp)hostile();throw;}
        marker_live_abi=previous;marker_required_active=previous_active;if(c.hostile_fp)hostile();
    }
    static bool matches(const SourcePhysicsContactRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
        const auto*actual=callback.target<ReadonlyRequired>();return actual&&actual->context==owner;
    }
};
void certify(SourcePhysicsWorldRuntimeServices&services,const ReadonlyRequiredContext&context){
    services.required_owner=ReadonlyRequired{&context};
    services.required_owner_policy={&typeid(ReadonlyRequired),std::this_thread::get_id(),&context,&ReadonlyRequired::matches};
}
struct ChangedInstruction{
    std::uint32_t*pc{};unsigned*callbacks{};J*log{};
    void operator()(std::uint32_t value)const{*pc=value;++*callbacks;log->push_back(value);}
};
struct MutatingRequired{
    ReadonlyRequired child;SourcePhysicsWorldRuntimeServices*services{};
    bool*trace{};std::uint32_t*pc{};unsigned*callbacks{};J*log{};unsigned mutation{};
    void operator()(SourcePhysicsContactMemory&bank,const SourcePhysicsContactRuntimeCall&call,
        SourcePhysicsWorldRuntimeAbi&callee,const SourcePhysicsContactRecordObserver&observer)const{
        child(bank,call,callee,observer);
        switch(mutation){
        case 0:services->instruction={};break;
        case 1:services->instruction=ChangedInstruction{pc,callbacks,log};break;
        case 2:*trace=true;break;
        case 3:services->instruction_cursor.position=nullptr;break;
        case 4:services->instruction_cursor.literal_service_sites=false;break;
        }
    }
};
unsigned owner_guards(bool forced){
    unsigned checks{};constexpr std::array good{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0xbacd0u,0x33b595u};
    for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<(forced?1u:26u);++policy){
        if((policy==16&&!route)||(policy==17&&route))continue;
        OwnerMemory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;bool trace{};unsigned callbacks{};
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,good,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        if(policy==9)observe=[](const auto&){};
        State state{memory,observe,services,policy==17?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000200,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
        ReadonlyRequiredContext context{},other_context{};
        if(policy>=18)certify(services,context);
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
        case 13:state.active_mmx=SourcePhysicsMmxAlias{};abi.active_mmx=SourcePhysicsMmxAlias{};break;
        case 14:if(route)abi.floating_point->x87_control_word=0x37e;else{const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw):"memory");}break;
        case 15:if(route)abi.floating_point->mxcsr=0x1f00;else _mm_setcsr(0x1f00);break;
        case 16:abi.floating_point.reset();break;
        case 18:services.required_owner_policy={};break;
        case 19:services.required_owner_policy.callback_type=&typeid(int);break;
        case 20:services.required_owner_policy.executor={};break;
        case 21:services.required_owner_policy.owner=&other_context;break;
        case 22:services.required_owner_policy.matches=nullptr;break;
        case 23:{auto original=services.required_owner;services.required_owner=[original](auto&m,const auto&c,auto&a,const auto&o){original(m,c,a,o);};break;}
        case 24:services.required_owner=ReadonlyRequired{&other_context};break;
        case 25:services.required_owner=[](auto&,const auto&,auto&,const auto&){};break;
        }
        const auto gp=state.gp;const auto flags=state.other_flags|state.flags.bits();const auto before=abi;
        const auto count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        memory.pc=0xdeadbeefu;const auto host=native_raw_x87_detail::capture();
        const bool accepted=route?native_marker_owner_detail::effect(memory,abi,services,observe):native_marker_owner_detail::bridge(state);
        check(!accepted&&state.gp==gp&&(state.other_flags|state.flags.bits())==flags&&abi.gp==before.gp&&abi.eflags==before.eflags,"Inherited-marker refusal changed GP/flags");
        check(state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&abi.x87_depth==before.x87_depth&&abi.x87_status==before.x87_status&&abi.xmm==before.xmm,"Inherited-marker refusal changed FP/SIMD metadata");
        check(!callbacks&&memory.reads.empty()&&memory.writes.empty()&&memory.pc==0xdeadbeefu&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Inherited-marker refusal performed I/O or changed host FP");++checks;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return checks;
}
void copied(State&state,unsigned mode){
    switch(mode){
    case 3:marker_source0::raw::run_bacd0(state);return;
    case 4:marker_source0::prepared::run_bacd0(state);return;
    case 5:marker_source1::raw::run_bacd0(state);return;
    case 6:marker_source1::prepared::run_bacd0(state);return;
    case 7:marker_source2::raw::run_bacd0(state);return;
    case 8:marker_source2::prepared::run_bacd0(state);return;
    case 9:marker_source3::raw::run_b9b90(static_cast<physics_vehicle_motion_detail::MotionState&>(state));return;
    case 10:marker_source3::prepared::run_b9b90(static_cast<physics_vehicle_motion_detail::MotionState&>(state));return;
    }
    throw std::runtime_error("Invalid inherited-marker source copy");
}
unsigned source_guards(){
    unsigned checks{};constexpr std::array good{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846eu,0xbacd0u,0x33b595u};
    for(unsigned route=0;route<9;++route)for(unsigned policy=0;policy<10;++policy){
        OwnerMemory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;bool trace{};unsigned callbacks{};
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,good,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State world{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        physics_vehicle_motion_detail::MotionState motion{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        State&state=route>=7?static_cast<State&>(motion):world;
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,0x62000200,6,7,8};abi.eflags=0x246;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(abi);
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
        const auto host=native_raw_x87_detail::capture();memory.pc=0xdeadbeefu;marker_stop=true;marker_statements=0;marker_live_abi=nullptr;bool stopped{};
        try{if(route==0)native_weapon_effect_dispatch_tu15_run(memory,0xbacd0u,abi,services,observe);else copied(state,route+2u);}
        catch(const RetainedStop&){stopped=true;}
        marker_stop=false;marker_live_abi=nullptr;
        check(stopped&&marker_statements==1&&!callbacks&&memory.reads.empty()&&memory.writes.empty()&&memory.pc==0xdeadbeefu,"Actual inherited-marker source did not reach untouched fallback");
        check(state.gp==abi.gp&&(state.other_flags|state.flags.bits())==abi.eflags&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Actual inherited-marker fallback changed entry state");++checks;
    }
    return checks;
}
}
int main(int argc,char**argv)try{
    if(argc==2&&(std::string_view(argv[1])=="--guards"||std::string_view(argv[1])=="--reference-guards")){
        std::cout<<J{{"passed",true},{"untouched_refusals",owner_guards(std::string_view(argv[1])=="--reference-guards")}}.dump()<<'\n';return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--source-guards"){std::cout<<J{{"passed",true},{"untouched_source_fallbacks",source_guards()}}.dump()<<'\n';return 0;}
    const bool entry_stop=argc==3&&std::string_view(argv[1])=="--required-entry-stop";
    const bool empty_services=argc==3&&std::string_view(argv[1])=="--empty-services";
    const bool policy_mutations=argc==3&&std::string_view(argv[1])=="--provider-policy-mutations";
    const bool hostile_required=argc==3&&std::string_view(argv[1])=="--hostile-required";
    check(argc==2||entry_stop||empty_services||policy_mutations||hostile_required,"Supply inherited-marker source fixture");std::ifstream file(argv[argc==3?2:1]);const auto fixture=J::parse(file);
    const bool forced=std::getenv("HALO2_MARKER_OWNER_SOURCE_REFERENCE_TEST");
    unsigned comparisons{},packets{},child_entries{},reference_statements{},effect_aliases{},policy_rows{};J policy_results=J::array();
    for(const auto&original:fixture.at("cases"))for(unsigned mutation=0;mutation<(policy_mutations?5u:1u);++mutation)for(unsigned mode=forced||policy_mutations?2u:0u;mode<11;++mode){
        const bool canonical=mode>=9;
        auto row=canonical?marker_canonical_projection(original):original;
        if(canonical)for(auto&site:row["write_sites"]){const auto pc=site.get<std::uint32_t>();if(pc>=0xbacd0u&&pc<=0xbad51u)site=pc-0x1140u;}
        if(forced&&row.contains("fault"))continue;
        const auto child_entry=std::find_if(row.at("calls").begin(),row.at("calls").end(),[](const auto&entry){return entry.at("entry")==0xfc510u;});
        if(policy_mutations&&(row.contains("fault")||child_entry==row.at("calls").end()||original.at("id").get<unsigned>()>=32u))continue;
        if(entry_stop&&(row.contains("fault")||child_entry==row.at("calls").end()))continue;
        OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.noisy=!forced&&!policy_mutations;memory.write_sites=row.at("write_sites").get<std::set<std::uint32_t>>();
        if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
        bool trace{};unsigned callbacks{};J instruction_log=J::array();constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        if(!empty_services){
            services.instruction=[&](auto pc){++callbacks;memory.pc=pc;instruction_log.push_back(pc);};
            services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
            observe=[&](const auto&event){
            check(event.instruction==memory.pc,"Inherited-marker source observer PC differs");
            if(!forced&&!policy_mutations)hostile();
            if(memory.fault&&memory.fault->at("phase")=="after_write"&&memory.write_count-1==memory.fault->at("ordinal"))throw Fault{};
            };
        }else if(memory.fault&&memory.fault->at("phase")=="after_write")continue;
        State world{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        physics_vehicle_motion_detail::MotionState motion{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        State&state=canonical?static_cast<State&>(motion):world;
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        state.load(abi);auto effect_abi=abi;marker_live_abi=mode==1||mode==2?&effect_abi:nullptr;marker_statements=0;
        memory.capture=[&]{return marker_live_abi?packet(*marker_live_abi,memory.pc):packet(state,memory.pc);};
        unsigned owner_calls{};
        const ReadonlyRequiredContext required{&memory,&services,&observe,&row,&effect_abi,mode,canonical,entry_stop,&owner_calls,&child_entries,&effect_aliases,hostile_required};
        certify(services,required);
        if(policy_mutations)services.required_owner=MutatingRequired{ReadonlyRequired{&required},&services,&trace,&memory.pc,&callbacks,&instruction_log,mutation};
        else if(original.at("id").get<unsigned>()&1u){auto immutable_copy=services.required_owner;services.required_owner=std::move(immutable_copy);}
        bool stopped{};std::string policy_exception;
        try{
            if(mode==0)check(native_marker_owner_detail::bridge(state),"Inherited-marker bridge refused valid source");
            else if(mode==1)check(native_marker_owner_detail::effect(memory,effect_abi,services,observe),"Inherited-marker Effect API refused valid source");
            else if(mode==2)native_weapon_effect_dispatch_tu15_run(memory,0xbacd0u,effect_abi,services,observe);
            else copied(state,mode);
        }catch(const Fault&){stopped=true;}catch(const RequiredStop&){check(entry_stop,"Unexpected required stop");stopped=true;}
        catch(const std::exception&error){if(!policy_mutations)throw;stopped=true;policy_exception=std::string(typeid(error).name())+":"+error.what();}
        marker_live_abi=nullptr;if(mode==1||mode==2)state.load(effect_abi);
        const auto context=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
        if(!policy_mutations)check(stopped==(row.contains("fault")||entry_stop),"Inherited-marker source completion differs "+context);
        const bool expected_owner=std::any_of(row.at("calls").begin(),row.at("calls").end(),[](const auto&entry){return entry.at("entry")==0xfc510u;});
        check(owner_calls==unsigned(expected_owner),"Inherited-marker source bypassed/repeated actual required owner");
        if(policy_mutations){
            check(callbacks&&marker_statements,"Uncertified provider did not retain whole source parent");
            auto actual=packet(state,row.at("expected").at("entry"));
            if(!stopped)check(actual==row.at("expected"),"Uncertified provider changed original numerical ABI");
            J banks=J::array();for(const auto&bank:row.at("banks"))banks.push_back(hex(memory.banks.at(bank.at("address"))));
            policy_results.push_back(J{{"mode",mode},{"mutation",mutation},{"id",row.at("id")},{"abi",packet(state,memory.pc)},{"reads",memory.reads},{"writes",memory.writes},{"packets",memory.packets},{"banks",banks},{"callbacks",instruction_log},{"retained_statements",marker_statements},{"required_entries",owner_calls},{"exception",policy_exception}});
            ++policy_rows;continue;
        }
        auto expected=row.at("expected"),reads=row.at("reads"),writes=row.at("writes"),calls=row.at("calls");
        if(entry_stop){
            expected=*child_entry;
            while(!reads.empty()&&reads.back().at("instruction")!=0xfc510u)reads.erase(reads.end()-1);
            check(!reads.empty(),"Original child-entry read missing");reads.erase(reads.end()-1);
            while(!writes.empty()&&writes.back().at("instruction")!=(canonical?0xb9bf9u:0xbad39u))writes.erase(writes.end()-1);
            calls.erase(calls.begin()+std::distance(row.at("calls").begin(),child_entry),calls.end());
        }
        if(empty_services){expected["entry"]=0u;for(auto&read:reads)read["instruction"]=0u;for(auto&call:calls)call["entry"]=0u;for(auto&write:writes)write["instruction"]=0u;}
        check(packet(state,entry_stop?0xfc510u:memory.pc)==expected,"Inherited-marker source final ABI differs "+context+" actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.reads==reads&&memory.writes==writes,"Inherited-marker source access protocol differs "+context);
        if(!forced){check(!callbacks&&!marker_statements,"Native inherited-marker source executed retained statements");check(memory.packets==calls,"Inherited-marker source complete pre-I/O packets differ "+context);}
        else{check(callbacks&&marker_statements,"Inherited-marker positive source reference did not execute");reference_statements+=marker_statements;}
        if(entry_stop){
            OwnerMemory original;for(const auto&bank:row.at("banks"))original.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            for(const auto&write:writes){const auto data=bytes(write.at("written"));std::memcpy(const_cast<std::byte*>(original.at(write.at("address"),data.size())),data.data(),data.size());}
            check(memory.banks==original.banks,"Inherited-marker required-entry original bank prefix differs");
        }else for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Inherited-marker source bank differs");
        packets+=memory.packets.size();++comparisons;
    }
    if(policy_mutations){check(policy_rows,"Provider policy corpus did not enter a genuine child");std::cout<<J{{"passed",true},{"provider_policy_rows",policy_rows},{"results",policy_results}}.dump()<<'\n';return 0;}
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"genuine_required_child_entries",child_entries},{"effect_original_abi_aliases",effect_aliases},{"positive_reference_statements",reference_statements},{"hostile_required_provider",hostile_required},{"reference_full_pre_io_packet_parity",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
