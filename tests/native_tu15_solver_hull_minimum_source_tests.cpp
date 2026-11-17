#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_minimum_source_owner.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace minimum_source {
#include "solver_hull_minimum_source.inc"
}

namespace {
struct RouteMemory:Memory{
    std::function<void(bool)>feedback;
    void read(std::uint32_t p,std::span<std::byte>bytes)const override{Memory::read(p,bytes);if(feedback)feedback(false);}
    void write(std::uint32_t p,std::span<const std::byte>bytes)override{Memory::write(p,bytes);if(feedback)feedback(true);}
};
struct Control {
    std::uint32_t revoke_pc{};
    unsigned install_callback_at{},observer_fault{},feedback{};
    bool callback_free{};
};
struct Result {
    J abi,events=J::array(),statements=J::array(),witnesses=J::array();
    bool stopped{};
    unsigned io{},callbacks{},observed{},native{},completed{},continued{},literal{},mutations{};
    std::uint32_t suffix{};
    std::string error;
};
SourcePhysicsWorldRuntimeAbi abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
RouteMemory seed(const J&row){
    RouteMemory m;for(const auto&bank:row.at("banks"))m.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    return m;
}
Result run(RouteMemory&m,const SourcePhysicsWorldRuntimeAbi&a,unsigned variant,bool reference,const Control&control={}){
    Result result;SourcePhysicsWorldRuntimeServices services;
    const auto callback=[&](std::uint32_t pc){m.pc=pc;++result.callbacks;result.statements.push_back(pc);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;bool tracing{};
    if(!control.callback_free){
        services.instruction=callback;cursor.position=&m.pc;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
        cursor.observation_only_stores=true;
    }
    SourcePhysicsContactRecordObserver observe;
    if(!control.callback_free)observe=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(++result.observed==control.observer_fault)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    const auto feedback_word=[&](std::uint32_t address,std::uint32_t value){
        const auto bank=m.at(address,4u);std::memcpy(m.banks.at(bank->first).data()+address-bank->first,&value,4u);
    };
    m.feedback=[&](bool writing){
        ++result.io;
        if(control.install_callback_at==result.io){services.instruction=callback;++result.mutations;return;}
        if(control.revoke_pc==m.pc&&!tracing&&control.revoke_pc){
            tracing=true;cursor.first_trace=&tracing;++result.mutations;return;
        }
        // A later, better candidate may overwrite SI before return. Keep the
        // actual reload's pre-read GP after changing the candidate base, so a
        // stale winner cannot be hidden by that later promotion.
        if(control.feedback==2u&&!writing&&result.mutations&&
           (m.pc==0x3131f5u||m.pc==0x31327cu))result.witnesses.push_back(packet(state,m.pc));
        const bool candidate_store=m.pc==0x3131e3u||m.pc==0x31326bu;
        if(control.feedback&&writing&&candidate_store&&!result.mutations){
            ++result.mutations;
            if(control.feedback==1u)
                feedback_word(state.gp[4]+(m.pc==0x3131e3u?8u:4u),0x3f000000u);
            else if(control.feedback==2u)
                state.gp[2]+=0x50u;
            else if(control.feedback==3u)
                state.fp.st(0)=-0x1.000003p0L;
            else
                throw native_solver_hull_minimum_owner_detail::Continuation{0xdeadbeefu,&m};
        }
    };
    const auto before=native_solver_hull_minimum_owner_detail::activity();
    const auto literal=minimum_source::literals[variant];minimum_source::suffixes[variant]=0u;
    try{minimum_source::run(state,variant,reference);}
    catch(const Boundary&){result.stopped=true;}
    catch(const native_solver_hull_minimum_owner_detail::Continuation&resume){
        check(resume.owner==&m,"Foreign minimum token owner changed");
        result.stopped=true;result.error="foreign continuation "+std::to_string(resume.pc);
    }catch(const std::runtime_error&e){result.stopped=true;result.error=e.what();}
    result.abi=packet(state,m.pc);m.feedback={};
    const auto after=native_solver_hull_minimum_owner_detail::activity();
    result.native=after.calls-before.calls;result.completed=after.completed-before.completed;
    result.continued=after.continuations-before.continuations;result.suffix=minimum_source::suffixes[variant];
    result.literal=minimum_source::literals[variant]-literal;
    return result;
}
void compare(const RouteMemory&a,const RouteMemory&b,const Result&x,const Result&y,const std::string&name){
    if(x.abi==y.abi&&x.events==y.events&&x.statements==y.statements&&x.witnesses==y.witnesses&&
       x.stopped==y.stopped&&x.error==y.error&&x.mutations==y.mutations&&x.io==y.io&&
       a.operations==b.operations&&a.writes==b.writes&&a.banks==b.banks)return;
    std::cerr<<name<<" ABI="<<(x.abi==y.abi)<<" read/write="<<(a.operations==b.operations)
             <<" events="<<(x.events==y.events)<<" callbacks="<<(x.statements==y.statements)
             <<" exceptions="<<x.error<<"/"<<y.error<<'\n';
    if(x.abi!=y.abi)std::cerr<<x.abi<<'\n'<<y.abi<<'\n';
    throw std::runtime_error("Actual hull minimum retained source differs");
}
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;
    const std::string mode=argc>1?argv[1]:"baseline";
    if(mode=="refusals"){
        unsigned refused{};
        for(unsigned n=0;n<19u;++n){
            auto memory=seed(rows.at(0));const auto original=memory.banks;
            SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
            services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            auto&cursor=services.instruction_cursor;
            cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
            cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;
            cursor.literal_service_sites=n!=1u;cursor.observation_only_stores=true;
            SourcePhysicsContactRecordObserver observe=[](const auto&){};
            const auto a=abi(rows.at(0));physics_step_detail::FpScope environment(*a.floating_point);
            WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
            auto foreign=cursor;bool trace=true;
            switch(n){
            case 0:services.instruction={};break;
            case 1:cursor.literal_service_sites=true;break;
            case 2:state.instruction_cursor=nullptr;break;
            case 3:state.instruction_cursor=&foreign;break;
            case 4:cursor.position=nullptr;break;
            case 5:cursor.callback_type=nullptr;break;
            case 6:cursor.callback_type=&typeid(WorldState);break;
            case 7:cursor.executor={};break;
            case 8:cursor.literal_service_sites=false;break;
            case 9:cursor.service_sites=std::span(sites).first(2);break;
            case 10:{static constexpr std::array changed{0x21f590u,0x38846eu,0x33b595u};cursor.service_sites=changed;break;}
            case 11:cursor.first_trace=&trace;break;
            case 12:cursor.second_trace=&trace;break;
            case 13:cursor.observation_only_stores=false;break;
            case 14:state.active_mmx=SourcePhysicsMmxAlias{};break;
            case 15:state.fp.status|=0x8080u;break;
            case 16:state.image=SourcePhysicsContactImage::MapPack11;break;
            case 17:state.fp.count=7u;break;
            case 18:break; // Valid cursor, but raw source requires its callable.
            }
            const auto before=packet(state,memory.pc);
            const auto calls=native_solver_hull_minimum_owner_detail::activity().calls;
            const auto result=n==18u?
                native_solver_hull_minimum_owner_detail::bridge<false,false>(state,native_solver_hull_minimum_source_detail::LegacySpill{}):
                native_solver_hull_minimum_owner_detail::bridge<true,false>(state,native_solver_hull_minimum_source_detail::LegacySpill{});
            check(result==0u&&calls==native_solver_hull_minimum_owner_detail::activity().calls&&callbacks==0u&&
                memory.operations.empty()&&memory.banks==original&&packet(state,memory.pc)==before,
                "Unqualified minimum source touched entry state or DATA");
            ++refused;
        }
        std::cout<<J{{"untouched_refusals",refused}}.dump()<<'\n';return 0;
    }
    unsigned cases{},native{},complete{},retained{},continuations{},prefixes{},read_stops{},write_stops{},observer_stops{},fallback_callbacks{},winner_reload_witnesses{};
    std::set<std::uint32_t>frontiers;
    for(const auto&row:rows){
        const auto id=row.at("id").get<unsigned>();
        if(mode!="baseline"&&id>=16u)continue;
        const auto saved=seed(row);const auto a=abi(row);
        for(unsigned v=0;v<4u;++v){
            const bool raw=v<2u;
            auto one=[&](Control c={},int read_fault=-1,int write_fault=-1){
                auto x=saved,y=saved;x.read_fault=y.read_fault=read_fault;x.write_fault=y.write_fault=write_fault;
                const auto want=run(x,a,v,true,c),got=run(y,a,v,false,c);
                compare(x,y,want,got,row.at("name"));
                ++cases;native+=got.native;complete+=got.completed;retained+=got.literal;
                continuations+=got.continued;fallback_callbacks+=got.callbacks;
                return got;
            };
            if(mode=="baseline"||mode=="own-reference"||mode=="raw-free"){
                const auto got=one(Control{.callback_free=mode=="raw-free"});
                const bool should_enter=mode=="raw-free"||(mode=="baseline"&&!raw);
                check(!got.stopped&&got.native==unsigned(should_enter)&&got.completed==unsigned(should_enter)&&
                      got.literal==unsigned(!should_enter),"Minimum selected the wrong source format or reference route");
            }else if(mode=="frontiers"){
                for(const auto [pc,next]:minimum_source::frontiers){
                    const auto got=one(Control{.revoke_pc=pc});
                    if(!got.mutations){check(!got.stopped&&!got.continued,"Unvisited minimum frontier executed a suffix");continue;}
                    check(!got.stopped&&got.mutations==1u&&got.continued==unsigned(!raw)&&got.suffix==(raw?0u:next),
                          "Minimum did not resume at its exact retired-I/O boundary");
                    frontiers.insert(pc);
                }
            }else if(mode=="feedback"){
                for(unsigned n=1;n<=4u;++n){
                    const auto got=one(Control{.feedback=n});
                    if(!got.mutations){check(!got.stopped,"Minimum stopped without feedback");continue;}
                    check(got.mutations==1u&&got.stopped==(n==4u),"Missing actual minimum feedback/foreign stop");
                    winner_reload_witnesses+=got.witnesses.size();
                    if(n==4u)check(got.error=="foreign continuation 3735928559","Wrong actual minimum foreign token");
                }
            }else if(mode=="prefixes"){
                auto baseline=saved;const auto before=run(baseline,a,v,true);
                for(unsigned n=0;n<baseline.read_count;++n){check(one({},int(n)).stopped,"Expected stopped minimum read");++read_stops;++prefixes;}
                for(unsigned n=0;n<baseline.write_count;++n){check(one({},-1,int(n)).stopped,"Expected stopped minimum write");++write_stops;++prefixes;}
                for(unsigned n=1;n<=before.observed;++n){check(one(Control{.observer_fault=n}).stopped,"Expected stopped minimum observer");++observer_stops;++prefixes;}
            }else if(mode=="raw-install"){
                auto baseline=saved;const auto before=run(baseline,a,v,true,Control{.callback_free=true});
                for(unsigned at=1;at<=before.io;++at){
                    const auto got=one(Control{.install_callback_at=at,.callback_free=true});
                    check(!got.stopped&&got.native==1u&&got.mutations==1u&&
                          (got.continued==1u||(at==before.io&&got.completed==1u)),
                          "Raw minimum callback installation skipped the required retained suffix");
                }
            }else throw std::runtime_error("Unknown minimum route mode");
        }
    }
    if(mode=="frontiers")check(frontiers.size()==24u,"Minimum test did not execute all 24 actual frontiers");
    if(mode=="feedback")check(winner_reload_witnesses>0u,"Minimum feedback did not reach an actual changed winner reload");
    std::cout<<J{{"source_cases",cases},{"native_entries",native},{"native_completed",complete},
                 {"retained_continuations",continuations},{"whole_entry_literals",retained},{"callbacks",fallback_callbacks},
                 {"exercised_frontiers",frontiers.size()},{"provider_and_observer_prefixes",prefixes},{"winner_reload_witnesses",winner_reload_witnesses},
                 {"read_prefixes",read_stops},{"write_prefixes",write_stops},{"observer_prefixes",observer_stops}}.dump()<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
