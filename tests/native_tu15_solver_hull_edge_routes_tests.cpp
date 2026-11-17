#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"
#include "physics/runtime/native_tu15_box_shape_support_source_owner.hpp"
#include <iostream>
#include <set>
using namespace halo2;using namespace halo2::mopp_ray_frame_test;
namespace edge_actual_children {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)>owner_dispatch;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){check(!tail,"Unexpected support tail");owner_dispatch(s,target,site);}
#include "solver_hull_edge_children.inc"
}
namespace edge_routes {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)>child;
static void run_313390(WorldState&s){child(s,0x313390u,0u);}
#include "solver_hull_edge_routes.inc"
}
namespace {
struct RouteMemory:Memory {
    std::function<void(bool)>feedback;
    void read(std::uint32_t p,std::span<std::byte>b)const override{Memory::read(p,b);if(feedback)feedback(false);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{Memory::write(p,b);if(feedback)feedback(true);}
};
SourcePhysicsWorldRuntimeAbi input(const J&row){SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}return a;}
RouteMemory memory(const J&row){RouteMemory m;for(const auto&b:row.at("banks"))m.banks[b.at("address").get<std::uint32_t>()]=bytes(b.at("before"));return m;}
struct Result{J abi,children=J::array(),events=J::array(),statements=J::array(),parent_operations=J::array();unsigned parent_io{};bool stopped{};unsigned callbacks{},post{},observed{},native{},complete{},resumed{},literal{},mutations{};std::string error;};
struct Owner{SourcePhysicsWorldRuntimeServices&services;void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver&)const{throw std::runtime_error("Genuine unconverted support owner");}};
// This provider is a named exact readonly leaf boundary; genuine qualified
// pair/shape computations below are direct, while unknown targets stay open.
struct Provider{Owner*owner;void operator()(SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeCall&c,SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsContactRecordObserver&o)const{(*owner)(m,c,a,o);}};
bool certified(const SourcePhysicsWorldRuntimeServices::RequiredOwner&f,const void*p)noexcept{const auto*v=f.target<Provider>();return v&&v->owner==p;}
Result run(RouteMemory&m,const SourcePhysicsWorldRuntimeAbi&a,unsigned variant,bool reference,bool ordered,unsigned frontier=0u,unsigned observer_fault=0u,unsigned policy_mode=0u,unsigned observer_mode=0u,bool callback_free=false,unsigned install_access=0u){
    Result out;SourcePhysicsWorldRuntimeServices services;bool changed{},revoked{},in_child{},trace{};WorldState*current{};
    const auto callback=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;out.statements.push_back(pc);if(revoked)++out.post;};
    services.instruction=callback;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    Owner owner{services};services.required_owner=Provider{&owner};services.required_owner_policy={&typeid(Provider),std::this_thread::get_id(),&owner,certified};
    if(policy_mode==1u)services.required_owner_policy={};
    if(policy_mode==2u){auto actual=services.required_owner;services.required_owner=[actual](auto&m,const auto&c,auto&a,const auto&o){actual(m,c,a,o);};}
    if(policy_mode==3u)services.required_owner_policy.owner=&out;
    SourcePhysicsContactRecordObserver observe;
    const auto make_observer=[&](unsigned generation){return [&,generation](const SourcePhysicsContactRecordStore&e){out.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)},{"generation",generation}});if(++out.observed==observer_fault)throw Boundary{};};};
    if(observer_mode!=1u&&observer_mode!=4u)observe=make_observer(0u);
    if(callback_free){services.instruction={};cursor={};observe={};}
    physics_step_detail::FpScope environment(*a.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);current=&state;
    bool observer_changed{};
    m.feedback=[&](bool writing){
        if(!in_child){++out.parent_io;out.parent_operations.push_back(J{{"writing",writing},{"index",writing?m.write_count-1u:m.read_count-1u}});if(install_access&&out.parent_io==install_access){changed=true;revoked=true;++out.mutations;services.instruction=callback;return;}}
        if(observer_mode&&!observer_changed&&!in_child){
            const auto wanted=observer_mode<=3u?0x313470u:observer_mode<=5u?0x313494u:0x313499u;
            const bool after_write=observer_mode==1u||observer_mode==4u||observer_mode==6u;
            if(m.pc==wanted&&writing==after_write){observer_changed=true;++out.mutations;if(observer_mode==2u||observer_mode==5u)observe={};else observe=make_observer(1u);return;}
        }
        if(changed||!frontier||in_child)return;
        if(frontier<100u){if(m.pc!=edge_routes::frontiers.at(frontier-1u))return;changed=true;revoked=true;++out.mutations;trace=true;cursor.first_trace=&trace;return;}
        const auto kind=frontier-100u;check(kind>=1u&&kind<=7u,"Unknown edge feedback");
        const auto wanted=std::array{0x3134beu,0x3134dfu,0x3134e7u,0x3134f0u,0x3134f8u,0x313535u,0x313539u}[kind-1u];
        if(m.pc!=wanted||writing!=(kind==4u||kind==7u))return;changed=true;++out.mutations;
        const auto vector=[&](unsigned reg,std::array<std::uint32_t,4>words){std::memcpy(&state.xmm[reg],words.data(),16);};
        if(kind==1u){vector(1,{0x3f800000,0x3f000000,0xbf800000,0});vector(2,{0x40000000,0,0,0});vector(3,{0x3e800000,0x3f800000,0x40000000,0});}
        else if(kind==2u){vector(4,{0x3f800000,0x40000000,0x40400000,0});vector(5,{0x40000000,0x40400000,0x40800000,0});}
        else if(kind==3u)vector(5,{0x3e800000,0x3f800000,0x3f000000,0});
        else if(kind==4u||kind==5u){vector(0,{0x3f800000,0x3f000000,0x40000000,0});vector(1,{0x3e800000,0x3f800000,0x3f000000,0});vector(4,{0x3f800000,0x40000000,0x3f800000,0});}
        else state.gp[6]=kind==6u?0x61003400u:state.gp[6]+1u;
    };
    const auto enter=[&](WorldState&s,std::uint32_t target,std::uint32_t pc,unsigned width){const auto b=m.at(s.gp[4],12);std::array<std::uint32_t,3>words{};std::memcpy(words.data(),b->second.data()+s.gp[4]-b->first,12);check(words[0]==pc+width,"Genuine edge child return differs");out.children.push_back(J{{"target",target},{"site",pc},{"words",words},{"abi",packet(s,target)}});};
    const auto leaf=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){enter(s,target,pc,3u);if(target==0x1f6610u){if(native_polygon_support_detail::admitted(s)){auto noop=[](auto&,std::uint32_t){};native_polygon_support_detail::support_retained<true>(s,noop);}else edge_actual_children::polygon_source(s);return;}if(target==0x2e8d20u){edge_actual_children::zero(s,variant);return;}if(target==0x306790u){edge_actual_children::box(s,variant);return;}s.required(pc,target,false);};
    edge_actual_children::owner_dispatch=leaf;
    edge_routes::child=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){check(target==0x313390u,"Different actual edge pair child");const auto bank=m.at(s.gp[4],4);std::uint32_t returned{};std::memcpy(&returned,bank->second.data()+s.gp[4]-bank->first,4);const auto actual_site=returned-5u;check((actual_site==0x313499u||actual_site==0x3134a9u)&&(!pc||pc==actual_site),"Actual pair callsite differs from genuine return word");enter(s,target,actual_site,5u);const auto was=in_child;in_child=true;try{edge_actual_children::pair(s,variant);}catch(...){in_child=was;throw;}in_child=was;};
    const auto before=native_solver_hull_edge_owner_detail::activity();const auto literals=edge_routes::literals;
    try{edge_routes::run(state,variant,reference,ordered);}catch(const Boundary&){out.stopped=true;}catch(const std::runtime_error&e){out.stopped=true;out.error=e.what();}
    out.abi=packet(state,m.pc);m.feedback={};edge_routes::child={};edge_actual_children::owner_dispatch={};const auto after=native_solver_hull_edge_owner_detail::activity();out.native=after.calls-before.calls;out.complete=after.completed-before.completed;out.resumed=after.continuations-before.continuations;for(unsigned n=0;n<4;++n)out.literal+=edge_routes::literals[n]-literals[n];return out;
}
void compare(const RouteMemory&a,const RouteMemory&b,const Result&x,const Result&y,const std::string&name){if(x.abi!=y.abi||x.stopped!=y.stopped||x.error!=y.error||x.children!=y.children||x.events!=y.events||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes||x.mutations!=y.mutations||x.post!=y.post||x.statements!=y.statements||x.parent_operations!=y.parent_operations){std::cerr<<name<<" abi="<<(x.abi==y.abi)<<" children="<<(x.children==y.children)<<" events="<<(x.events==y.events)<<" reads="<<(a.operations==b.operations)<<" post="<<x.post<<"/"<<y.post<<'\n';if(x.abi!=y.abi)std::cerr<<x.abi<<'\n'<<y.abi<<'\n';throw std::runtime_error("Hull-edge complete source route differs");}}
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;const std::string mode=argc>1?argv[1]:"baseline";unsigned cases{},native{},complete{},resumed{},callbacks{},literals{},reads{},writes{},observers{};std::set<unsigned>visited;
    if(mode=="refusals"){
        unsigned refused{};
        for(unsigned n=0;n<24u;++n){auto m=memory(rows[0]);const auto initial_banks=m.banks;Result out;SourcePhysicsWorldRuntimeServices services;Owner owner{services};
            services.required_owner=Provider{&owner};services.required_owner_policy={&typeid(Provider),std::this_thread::get_id(),&owner,certified};
            services.instruction=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
            SourcePhysicsContactRecordObserver observe=[](const auto&){};auto a=input(rows[0]);physics_step_detail::FpScope environment(*a.floating_point);if(n==1u)cursor.literal_service_sites=false;
            WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);auto foreign=cursor;bool trace=true;if(n==1u)cursor.literal_service_sites=true;
            switch(n){case 0:services.instruction={};break;case 1:break;case 2:state.instruction_cursor=nullptr;break;case 3:state.instruction_cursor=&foreign;break;case 4:cursor.position=nullptr;break;case 5:cursor.callback_type=nullptr;break;case 6:cursor.callback_type=&typeid(Provider);break;case 7:cursor.executor={};break;case 8:cursor.literal_service_sites=false;break;case 9:cursor.service_sites=std::span(sites).first(2);break;case 10:{static constexpr std::array changed{0x21f590u,0x38846eu,0x33b595u};cursor.service_sites=changed;break;}case 11:cursor.first_trace=&trace;break;case 12:cursor.second_trace=&trace;break;case 13:cursor.observation_only_stores=false;break;case 14:services.required_owner_policy={};break;case 15:services.required_owner_policy.callback_type=&typeid(WorldState);break;case 16:services.required_owner_policy.executor={};break;case 17:services.required_owner_policy.owner=&out;break;case 18:services.required_owner_policy.matches=nullptr;break;case 19:{auto original=services.required_owner;services.required_owner=[original](auto&m,const auto&c,auto&a,const auto&o){original(m,c,a,o);};break;}case 20:state.active_mmx=SourcePhysicsMmxAlias{};break;case 21:state.fp.status|=0x8080u;break;case 22:state.image=SourcePhysicsContactImage::MapPack11;break;case 23:state.fp.count=8u;break;}
            const auto before=packet(state,m.pc);const auto calls=native_solver_hull_edge_owner_detail::activity().calls;
            const auto accepted=native_solver_hull_edge_owner_detail::bridge(state,[](auto&,auto,auto){throw std::runtime_error("Refused entry invoked child");});
            check(accepted==0u&&native_solver_hull_edge_owner_detail::activity().calls==calls&&out.callbacks==0u&&m.operations.empty()&&m.banks==initial_banks&&packet(state,m.pc)==before,"Unsupported hull-edge admission changed State or provider");++refused;
        }
        std::cout<<J{{"untouched_entry_refusals",refused}}.dump()<<'\n';return 0;
    }
    for(const auto&row:rows){const auto id=row.at("id").get<unsigned>();if(mode!="baseline"&&mode!="raw-free"&&id>=12u)continue;auto seed=memory(row);const auto a=input(row);
        for(unsigned v=0;v<4u;++v){
            if(mode=="raw-free"||mode=="raw-install"){
                const unsigned last=mode=="raw-free"?0u:[&]{auto baseline=seed;const auto sample=run(baseline,a,v,true,true,0u,0u,0u,0u,true);check(!sample.stopped,"Raw free source seed stopped");return sample.parent_io;}();
                for(unsigned at=mode=="raw-free"?0u:1u;at<=last;++at){auto x=seed,y=seed;const auto want=run(x,a,v,true,true,0u,0u,0u,0u,true,at),got=run(y,a,v,false,false,0u,0u,0u,0u,true,at);compare(x,y,want,got,"actual raw callback-free/installation stream");check(!got.stopped&&got.native==1u&&got.literal==0u,"Callback-free tagged source did not enter native");if(at)check(got.mutations==1u&&((got.resumed==1u&&got.post>0u)||(at==last&&got.complete==1u&&got.post==0u)),"Callback installation did not cause a retired-I/O source handoff or complete final RET");else check(got.complete==1u&&got.callbacks==0u,"Callback-free source ran callbacks");++cases;native+=got.native;complete+=got.complete;resumed+=got.resumed;callbacks+=got.callbacks;}
                continue;
            }
            const bool raw=v<2u;
            if(mode=="own-reference"){
                auto x=seed,y=seed;const auto want=run(x,a,v,true,false),got=run(y,a,v,false,false);compare(x,y,want,got,"forced own reference");check(!got.stopped&&got.native==0u&&got.literal==1u,"Own reference did not take exact whole-entry fallback");++cases;literals+=got.literal;callbacks+=got.callbacks;continue;
            }
            if(mode=="source-publication"||mode=="raw-publication"){
                const bool free=mode=="raw-publication";const unsigned expected_native=free||!raw;
                auto baseline=seed;const auto base=run(baseline,a,v,true,free||!raw,0u,0u,0u,0u,free);check(!base.stopped,"Source prefix seed stopped");
                for(bool writing:{false,true}){for(const auto&op:base.parent_operations){if(op.at("writing").get<bool>()!=writing)continue;const auto n=op.at("index").get<unsigned>();auto x=seed,y=seed;if(writing)x.write_fault=y.write_fault=n;else x.read_fault=y.read_fault=n;const auto want=run(x,a,v,true,free||!raw,0u,0u,0u,0u,free),got=run(y,a,v,false,false,0u,0u,0u,0u,free);compare(x,y,want,got,"retained provider prefix");check(got.stopped&&got.native==expected_native,"Source provider fault missed its tagged route");if(writing)++writes;else ++reads;++cases;native+=got.native;literals+=got.literal;callbacks+=got.callbacks;}}
                for(unsigned n=0;n<base.events.size();++n){const auto pc=base.events[n].at("instruction").get<std::uint32_t>();if(pc<0x313470u||pc>0x31355du)continue;auto x=seed,y=seed;const auto want=run(x,a,v,true,free||!raw,0u,n+1u,0u,0u,free),got=run(y,a,v,false,false,0u,n+1u,0u,0u,free);compare(x,y,want,got,"retained completed observer prefix");check(got.stopped&&got.native==expected_native,"Source observer fault missed its tagged route");++observers;++cases;native+=got.native;literals+=got.literal;callbacks+=got.callbacks;}continue;
            }
            if(mode=="observer-objects"){
                for(unsigned object=1;object<=6u;++object){auto x=seed,y=seed;const auto want=run(x,a,v,true,!raw,0u,0u,0u,object),got=run(y,a,v,false,false,0u,0u,0u,object);compare(x,y,want,got,"observer object change");check(!got.stopped&&got.native==unsigned(!raw)&&got.complete==unsigned(!raw)&&got.literal==unsigned(raw)&&got.mutations==1u,"Observer object case missed its exact tagged route");++cases;native+=got.native;complete+=got.complete;literals+=got.literal;callbacks+=got.callbacks;}continue;
            }
            if(mode=="provider-controls"){
                for(unsigned policy=1;policy<=3u;++policy){auto x=seed,y=seed;const auto want=run(x,a,v,true,false,0u,0u,policy),got=run(y,a,v,false,false,0u,0u,policy);compare(x,y,want,got,"uncertified or wrapped required owner");check(!got.stopped&&got.native==0u&&got.literal==1u,"Uncertified provider did not take whole-entry fallback");++cases;literals+=got.literal;callbacks+=got.callbacks;}continue;
            }
            const unsigned first=mode=="frontiers"?1u:mode=="feedback"?101u:0u,last=mode=="frontiers"?edge_routes::frontiers.size():mode=="feedback"?107u:0u;
            for(unsigned f=first;f<=last;++f){auto x=seed,y=seed;const auto want=run(x,a,v,true,!raw,f),got=run(y,a,v,false,false,f);compare(x,y,want,got,row.at("name"));if(f&&got.mutations==0u)continue;check(!got.stopped,"Source edge stopped");check(got.native==unsigned(!raw)&&got.literal==unsigned(raw),"Source format did not select its exact entry route");if(f<100u&&f&&!raw)check(got.resumed==1u,"Revoked edge did not resume retained body");else if(!f&&!raw)check(got.complete==1u&&got.callbacks==0u,"Prepared edge executed literal statements");if(f)visited.insert(f);++cases;native+=got.native;complete+=got.complete;resumed+=got.resumed;callbacks+=got.callbacks;literals+=got.literal;}
        }
    }
    if(mode=="source-publication"||mode=="raw-publication")check(reads>0u&&writes>0u,"Empty retained publication matrix");
    if(mode=="raw-publication")check(native==cases&&literals==0u&&callbacks==0u,"Raw fault matrix did not use callback-free native entries exclusively");
    if(mode=="frontiers")check(visited.size()==43u,"Not all43 source frontiers were exercised");
    if(mode=="feedback")check(visited.size()==7u,"Not all7 source feedback cases were exercised");
    std::cout<<J{{"exercised_feedback_or_frontiers",visited.size()},{"whole_entry_literals",literals},{"source_cases",cases},{"native_entries",native},{"native_completed",complete},{"retained_continuations",resumed},{"callbacks",callbacks},{"parent_read_prefixes",reads},{"parent_write_prefixes",writes},{"parent_completed_observer_prefixes",observers},{"genuine_computed_children_only",true},{"child_internal_policy_frontiers_qualified",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
