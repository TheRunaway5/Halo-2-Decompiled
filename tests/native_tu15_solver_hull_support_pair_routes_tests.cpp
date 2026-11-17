#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"
#include "physics/runtime/native_tu15_box_shape_support_source_owner.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include <iostream>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace pair_children {
#include "solver_hull_support_pair_child_routes.inc"
}
namespace pair_routes {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)>owner_dispatch;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t pc,bool tail){check(!tail,"Unexpected pair tail");owner_dispatch(s,target,pc);}
#include "solver_hull_support_pair_routes.inc"
}
namespace {
struct RouteMemory:Memory {
    std::function<void(bool)>feedback;
    void read(std::uint32_t p,std::span<std::byte>b)const override{Memory::read(p,b);if(feedback)feedback(false);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{Memory::write(p,b);if(feedback)feedback(true);}
};
SourcePhysicsWorldRuntimeAbi abi_from(const J&row){SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();return a;}
RouteMemory memory_from(const J&row){RouteMemory m;for(const auto&b:row.at("banks"))m.banks[b.at("address").get<std::uint32_t>()]=bytes(b.at("before"));return m;}
struct Result{J abi,children=J::array(),required=J::array(),events=J::array();std::string stop_reason;bool stopped{},policy_changed{};unsigned callbacks{},post_callbacks{},observed{},mutations{},native_entries{},completed{},resumed{},dispatch_calls{},literal{};};
struct ActualChildState:WorldState{using WorldState::WorldState;bool selected_observation_pcs{true};};
struct Owner {
    RouteMemory&memory;SourcePhysicsWorldRuntimeServices&services;Result&out;WorldState*&current;bool&in_child;const unsigned mode;
    bool trace{};
    void mutate(unsigned kind){
        ++out.mutations;out.policy_changed=true;
        if(kind==1u)services.instruction={};
        else if(kind==2u)services.instruction=[this](std::uint32_t pc){memory.pc=pc;++out.callbacks;++out.post_callbacks;};
        else if(kind==3u){trace=true;services.instruction_cursor.first_trace=&trace;}
        else if(kind==4u)services.required_owner_policy={};
    }
    template<class State>void child(State&s,std::uint32_t target){
        if(target==0x2e8d20u){if(native_zero_shape_support_owner_detail::bridge(s))return;pair_children::zero(s);return;}
        if(target==0x306790u){if(native_box_shape_support_owner_detail::bridge(s))return;pair_children::box(s);return;}
        if(target==0x1f6610u){if(native_polygon_support_detail::bridge(s))return;pair_children::polygon(s);return;}
        throw std::runtime_error("Declared genuine unconverted support entry");
    }
    void compute(const SourcePhysicsContactRuntimeCall&call,SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&observe){
        check(!call.tail&&(call.instruction==0x3133abu||call.instruction==0x313425u),"Pair required call identity differs");
        auto saved=packet(*current,call.target);const auto p=memory.at(a.gp[4],12);std::array<std::uint32_t,3>words{};std::memcpy(words.data(),p->second.data()+a.gp[4]-p->first,12);check(words[0]==call.instruction+3u,"Pair genuine return word differs");
        out.required.push_back(J{{"target",call.target},{"site",call.instruction},{"words",words},{"abi",saved}});
        ActualChildState imported{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};imported.load(a);
        auto*parent=current;const auto previous_child=in_child;current=&imported;in_child=true;
        out.children.push_back(J{{"target",call.target},{"site",call.instruction},{"abi",packet(imported,call.target)}});
        try{child(imported,call.target);a=imported.save();}catch(...){a=imported.save();current=parent;in_child=previous_child;throw;}
        current=parent;in_child=previous_child;
        if(call.instruction==0x3133abu&&(mode==6u||mode==7u))mutate(mode==6u?3u:2u);
    }
};
struct ReadonlyProvider {Owner*owner;void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&c,SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o)const{owner->compute(c,a,o);}};
bool matches(const SourcePhysicsWorldRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{const auto*p=callback.target<ReadonlyProvider>();return p&&p->owner==owner&&p->owner->mode==1u;}
void certify(SourcePhysicsWorldRuntimeServices&s,Owner&o){s.required_owner_policy={&typeid(ReadonlyProvider),std::this_thread::get_id(),&o,matches};}
Result run(RouteMemory&m,const SourcePhysicsWorldRuntimeAbi&a,unsigned variant,unsigned mode,bool reference,bool ordered,unsigned feedback=0u,unsigned observer_fault=0u){
    Result out;WorldState*current{};bool in_child{};SourcePhysicsWorldRuntimeServices services;
    Owner owner{m,services,out,current,in_child,mode==3u||mode==4u||mode==5u?1u:mode};
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(++out.observed==observer_fault)throw Boundary{};};
    services.instruction=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;if(out.policy_changed)++out.post_callbacks;};
    auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    if(mode){services.required_owner=ReadonlyProvider{&owner};if(mode==1u||mode==3u||mode==4u||mode==5u)certify(services,owner);if(mode==2u)services.required_owner_policy={};if(mode==3u){auto original=services.required_owner;services.required_owner=[original](auto&mem,const auto&c,auto&abi,const auto&o){original(mem,c,abi,o);};}if(mode==4u)services.required_owner_policy.owner=&out;if(mode==5u){auto copy=services.required_owner;services.required_owner=std::move(copy);}}
    physics_step_detail::FpScope environment(*a.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);current=&state;
    bool changed{};
    m.feedback=[&](bool writing){
        if(!feedback||changed||!current)return;
        if(feedback==11u||feedback==12u){if(!in_child||!writing)return;changed=true;owner.mutate(feedback==11u?3u:2u);return;}
        if(in_child)return;
        if(feedback>=100u){const auto index=feedback-100u;check(index<pair_routes::frontiers.size(),"Wrong source frontier");if(m.pc!=pair_routes::frontiers[index])return;changed=true;owner.mutate(3u);return;}
        const auto wanted=feedback<=6u?std::array{0x3133aeu,0x3133c3u,0x313418u,0x31342fu,0x31343du,0x313455u}[feedback-1u]:feedback==7u?0x31339au:feedback==8u?0x31341du:feedback==9u?0x3133a4u:0x3133b2u;
        if(m.pc!=wanted||(writing!=(feedback==3u||feedback==8u)))return;changed=true;++out.mutations;
        const auto vector=[&](unsigned reg,std::array<std::uint32_t,4>words){std::memcpy(&state.xmm[reg],words.data(),16);};
        if(feedback==1u){state.gp[3]=0x61002200u;state.gp[6]=0x61001200u;state.gp[7]=0x61003400u;}
        else if(feedback==2u){vector(0,{0x3f800000,0x3f000000,0xbf800000,0});vector(1,{0x80000000,0x80000000,0x80000000,0x80000000});vector(2,{0x40000000,0,0,0});vector(3,{0,0,0x3f800000,0});vector(5,{0,0x3f800000,0,0});}
        else if(feedback==3u)vector(1,{0x3f800000,0x40000000,0x40400000,0});
        else if(feedback==4u){vector(0,{0x3f800000,0x3f000000,0x40000000,0});vector(1,{0x3e800000,0x3f800000,0x3f000000,0});}
        else if(feedback==5u){vector(0,{0x40000000,0x3f800000,0x3f000000,0});vector(1,{0x40000000,0x3f800000,0,0});vector(3,{0x3e800000,0x3f000000,0x3f800000,0});}
        else if(feedback==6u){vector(1,{0x3f800000,0x3f800000,0x3f800000,0});vector(2,{0x3e800000,0x3f000000,0x3f800000,0});vector(3,{0x3f000000,0x3f800000,0x40000000,0});}
        else{--out.mutations;owner.mutate(feedback==7u?1u:feedback==8u?2u:feedback==9u?3u:4u);}
    };
    pair_routes::owner_dispatch=[&](auto&s,std::uint32_t target,std::uint32_t pc){if(target==0x2e8d20u||target==0x306790u){const auto was=in_child;in_child=true;out.children.push_back(J{{"target",target},{"site",pc},{"abi",packet(s,target)}});try{owner.child(s,target);}catch(...){in_child=was;throw;}in_child=was;return;}s.required(pc,target,false);};
    const auto before=native_solver_hull_support_pair_owner_detail::activity();const auto literals=pair_routes::literals;
    try{pair_routes::run(state,variant,reference,ordered);}catch(const Boundary&){out.stopped=true;out.stop_reason="declared provider/observer fault";}catch(const std::runtime_error&e){out.stopped=true;out.stop_reason=e.what();}
    out.abi=packet(state,m.pc);m.feedback={};pair_routes::owner_dispatch={};const auto after=native_solver_hull_support_pair_owner_detail::activity();out.native_entries=after.calls-before.calls;out.completed=after.completed-before.completed;out.resumed=after.continuations-before.continuations;out.dispatch_calls=after.dispatch_children-before.dispatch_children;for(unsigned n=0;n<4;++n)out.literal+=pair_routes::literals[n]-literals[n];return out;
}
void compare_source(const RouteMemory&a,const RouteMemory&b,const Result&x,const Result&y,const std::string&name){
    if(x.abi!=y.abi||x.stopped!=y.stopped||x.stop_reason!=y.stop_reason||x.children!=y.children||x.required!=y.required||x.events!=y.events||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes||x.mutations!=y.mutations||x.post_callbacks!=y.post_callbacks){std::cerr<<name<<" abi="<<(x.abi==y.abi)<<" child="<<(x.children==y.children)<<" required="<<(x.required==y.required)<<" events="<<(x.events==y.events)<<" reads="<<(a.operations==b.operations)<<" post="<<x.post_callbacks<<"/"<<y.post_callbacks<<"\n";throw std::runtime_error("Support-pair complete source route differs");}
}
unsigned refusal_controls(const J&row){
    unsigned count{};
    for(unsigned n=0;n<23u;++n){auto memory=memory_from(row);const auto initial_banks=memory.banks;Result out;WorldState*current{};bool child{},trace=true;SourcePhysicsWorldRuntimeServices services;
        Owner owner{memory,services,out,current,child,1u};services.required_owner=ReadonlyProvider{&owner};certify(services,owner);
        services.instruction=[&](std::uint32_t pc){memory.pc=pc;++out.callbacks;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&memory.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        SourcePhysicsContactRecordObserver observe=[](const auto&){};auto a=abi_from(row);physics_step_detail::FpScope environment(*a.floating_point);if(n==1u)cursor.literal_service_sites=false;WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);current=&state;auto foreign=cursor;if(n==1u)cursor.literal_service_sites=true;
        switch(n){case 0:services.instruction={};break;case 1:break;case 2:state.instruction_cursor=nullptr;break;case 3:state.instruction_cursor=&foreign;break;case 4:cursor.position=nullptr;break;case 5:cursor.callback_type=nullptr;break;case 6:cursor.callback_type=&typeid(ReadonlyProvider);break;case 7:cursor.executor={};break;case 8:cursor.literal_service_sites=false;break;case 9:cursor.service_sites=std::span(sites).first(2);break;case 10:{static constexpr std::array changed{0x21f590u,0x38846eu,0x33b595u};cursor.service_sites=changed;break;}case 11:cursor.first_trace=&trace;break;case 12:cursor.second_trace=&trace;break;case 13:cursor.observation_only_stores=false;break;case 14:services.required_owner_policy={};break;case 15:services.required_owner_policy.callback_type=&typeid(WorldState);break;case 16:services.required_owner_policy.executor={};break;case 17:services.required_owner_policy.owner=&out;break;case 18:services.required_owner_policy.matches=nullptr;break;case 19:{auto original=services.required_owner;services.required_owner=[original](auto&m,const auto&c,auto&v,const auto&o){original(m,c,v,o);};break;}case 20:state.active_mmx=SourcePhysicsMmxAlias{};break;case 21:state.fp.status|=0x8080u;break;case 22:state.image=SourcePhysicsContactImage::MapPack11;break;}
        const auto before=packet(state,memory.pc);const auto calls=native_solver_hull_support_pair_owner_detail::activity().calls;
        const auto accepted=native_solver_hull_support_pair_owner_detail::bridge(state,[&](auto&,auto,auto){throw std::runtime_error("Refused entry invoked dispatcher");});
        check(accepted==0u&&native_solver_hull_support_pair_owner_detail::activity().calls==calls&&out.callbacks==0u&&memory.operations.empty()&&memory.banks==initial_banks&&packet(state,memory.pc)==before,"Malformed/unsupported source admission had side effects "+std::to_string(n));++count;
    }
    return count;
}
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;
    if(argc>1&&std::string(argv[1])=="--refusals"){std::cout<<J{{"untouched_entry_refusals",refusal_controls(rows[0])}}.dump()<<'\n';return 0;}
    if(argc>1&&std::string(argv[1])=="--unknown-owner"){
        auto seed=memory_from(rows[0]);const std::uint32_t table=0x61007000u,target=0x2e7d90u;auto put=[&](std::uint32_t address,std::uint32_t value){const auto bank=seed.at(address,4);std::memcpy(seed.banks.at(bank->first).data()+address-bank->first,&value,4);};put(0x61005000u,table);put(table+0x30u,target);const auto a=abi_from(rows[0]);unsigned stops{};
        for(unsigned variant=0;variant<4u;++variant)for(unsigned mode:{0u,1u,2u}){auto x=seed,y=seed;const auto expected=run(x,a,variant,mode,true,mode!=2u),actual=run(y,a,variant,mode,false,false);compare_source(x,y,expected,actual,"genuine unconverted owner entry");check(actual.stopped&&actual.completed==0u&&actual.resumed==0u,"Unknown owner was assigned a result");if(mode!=2u)check(actual.native_entries==1u,"Unknown owner did not keep genuine native-parent dispatch");++stops;}
        std::cout<<J{{"genuine_unconverted_owner_entry_stops",stops},{"actual_owner",target},{"supplied_child_outcomes",false}}.dump()<<'\n';return 0;
    }
    if(argc>1&&std::string(argv[1])=="--source-publication"){
        unsigned reads{},writes{},observers{};
        for(const auto&row:rows){if(row.at("id").get<unsigned>()>=4u)continue;const auto seed=memory_from(row);const auto a=abi_from(row);
            for(unsigned variant=0;variant<4u;++variant){auto baseline=seed;const auto initial=run(baseline,a,variant,1u,true,true);check(!initial.stopped,"Source fault seed did not return");
                for(bool writing:{false,true}){unsigned index{};for(const auto&op:baseline.operations){if((op.at("kind")=="write")!=writing)continue;const auto n=index++;const auto pc=op.at("instruction").get<std::uint32_t>();if(pc<0x313390u||pc>0x31346bu)continue;auto x=seed,y=seed;if(writing)x.write_fault=y.write_fault=n;else x.read_fault=y.read_fault=n;const auto expected=run(x,a,variant,1u,true,true),actual=run(y,a,variant,1u,false,false);compare_source(x,y,expected,actual,"retained provider prefix");check(actual.stopped&&actual.native_entries==1u,"Source provider fault was not exercised natively");if(writing)++writes;else ++reads;}}
                for(unsigned n=0;n<initial.events.size();++n){const auto pc=initial.events[n].at("instruction").get<std::uint32_t>();if(pc<0x313390u||pc>0x31346bu)continue;auto x=seed,y=seed;const auto expected=run(x,a,variant,1u,true,true,0u,n+1u),actual=run(y,a,variant,1u,false,false,0u,n+1u);compare_source(x,y,expected,actual,"retained completed-observer prefix");check(actual.stopped&&actual.native_entries==1u,"Source observer fault not exercised natively");++observers;}
            }
        }
        std::cout<<J{{"parent_read_prefixes",reads},{"parent_write_prefixes",writes},{"parent_completed_observer_prefixes",observers},{"all_retained_PUSH_and_CALL_return_publication",true},{"child_fault_scope_qualified_here",false}}.dump()<<'\n';return 0;
    }
    const auto mode=argc>1?unsigned(std::stoul(argv[1])):1u;const bool feedback=argc>2&&std::string(argv[2])=="feedback",frontiers=argc>2&&std::string(argv[2])=="frontiers";unsigned cases{},native{},complete{},resumed{},literal{},callbacks{},stops{};
    const bool fallback=mode==2u||mode==3u||mode==4u||mode==6u||mode==7u||native_solver_hull_support_pair_owner_detail::reference();
    for(const auto&row:rows){if((mode!=0u&&mode!=1u&&mode!=5u||feedback||frontiers)&&row.at("id").get<unsigned>()>=16u)continue;auto seed=memory_from(row);const auto a=abi_from(row);
        const auto first=frontiers?100u:feedback?1u:0u,last=frontiers?100u+unsigned(pair_routes::frontiers.size())-1u:feedback?12u:0u;
        for(unsigned variant=0;variant<4u;++variant)for(unsigned f=first;f<=last;++f){auto x=seed,y=seed;const auto expected=run(x,a,variant,mode,true,!fallback,f),actual=run(y,a,variant,mode,false,false,f);compare_source(x,y,expected,actual,row.at("name"));native+=actual.native_entries;complete+=actual.completed;resumed+=actual.resumed;literal+=actual.literal;callbacks+=actual.callbacks;stops+=actual.stopped;++cases;}
    }
    check(cases>0u,"Empty source route corpus");
    if(fallback)check(native==0u&&literal==cases&&resumed==0u,"Whole-entry fallback did not preserve literal route");
    else{check(native==cases&&literal==0u,"Admitted parent did not run natively");if(mode)check(stops==0u&&complete+resumed==cases,"Native parent or retained suffix did not complete");if(!feedback&&!frontiers&&mode)check(complete==cases&&callbacks==0u,"Qualified parent/children executed callbacks");}
    if(frontiers)check(resumed==cases&&complete==0u,"Every source retirement frontier did not resume exactly once");
    std::cout<<J{{"source_cases",cases},{"native_entries",native},{"native_completed",complete},{"retained_continuations",resumed},{"whole_entry_literals",literal},{"callbacks",callbacks},{"stops",stops},{"genuine_children_only",true}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
