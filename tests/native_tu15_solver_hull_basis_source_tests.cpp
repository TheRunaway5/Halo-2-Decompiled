#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_basis_source_owner.hpp"
#include "physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_source_owner.hpp"
#include "physics/runtime/native_tu15_solver_hull_append_source_owner.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"
#include "physics/runtime/native_tu15_box_shape_support_source_owner.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace basis_children {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)> child;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){check(!tail,"Unexpected hull basis tail");child(s,target,site);}
static void run_313390(WorldState&s){child(s,0x313390u,0u);}
#include "solver_hull_basis_children.inc"
}
namespace basis_source {
static std::function<void(WorldState&,std::uint32_t)> child;
static void run_313470(WorldState&s){child(s,0x313470u);}
static void run_313290(WorldState&s){child(s,0x313290u);}
#include "solver_hull_basis_source.inc"
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
    unsigned estimate_fault{};
    std::uint32_t child_policy_site{};
    bool ordered_reference{};
};
struct Result {
    J abi,events=J::array(),statements=J::array(),witnesses=J::array(),estimates=J::array(),children=J::array();
    bool stopped{};
    unsigned io{},callbacks{},observed{},native{},completed{},continued{},literal{},mutations{};
    std::uint32_t suffix{};
    std::string error;
    unsigned estimate_count{},child_depth{};
    std::vector<std::pair<bool,unsigned>> parent_provider_operations;
};
std::uint32_t estimate_bits(std::uint32_t bits){
    const auto input=_mm_set_ss(std::bit_cast<float>(bits));
    return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(input)));
}
struct EstimateJournal {
    SourceRuntimeMemory bank;
    SourceExecutionInputBinding binding;
    unsigned computed{};
    explicit EstimateJournal(SourceExecutionInputs&inputs):binding(bank,inputs){}
    std::uint32_t sample(std::uint32_t operand){
        return static_cast<std::uint32_t>(source_execution_input(bank,SourceExecutionInputKind::ReciprocalSquareRoot,operand,[&]{
            ++computed;return estimate_bits(operand);
        }));
    }
};
struct NamedProvider {
    const void*owner;
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
                    SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver&)const{
        throw std::runtime_error("Unqualified required owner was invoked");
    }
};
bool matches_provider(const SourcePhysicsWorldRuntimeServices::RequiredOwner&f,const void*owner)noexcept{
    const auto*named=f.target<NamedProvider>();return named&&named->owner==owner;
}
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
Result run(RouteMemory&m,const SourcePhysicsWorldRuntimeAbi&a,unsigned variant,bool reference,const Control&control={},EstimateJournal*journal=nullptr){
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
    bool installed{};
    unsigned parent_io{};std::uint32_t direct_child{};
    const auto retire_policy=[&](std::uint32_t pc){
        if(control.revoke_pc!=pc||tracing||!pc)return;
        tracing=true;cursor.first_trace=&tracing;++result.mutations;
    };
    m.feedback=[&](bool writing){
        ++result.io;
        if(result.child_depth){
            if(!writing&&!tracing&&control.child_policy_site&&direct_child==control.child_policy_site&&
               (m.pc==0x31354du||m.pc==0x31355du||m.pc==0x3132f2u)){
                tracing=true;cursor.first_trace=&tracing;++result.mutations;
            }
            return;
        }
        result.parent_provider_operations.emplace_back(writing,writing?m.write_count-1u:m.read_count-1u);
        if(control.install_callback_at==++parent_io&&!installed){
            services.instruction=callback;installed=true;++result.mutations;return;
        }
        retire_policy(m.pc);
        if(!control.feedback||result.mutations)return;
        if(control.feedback==1u&&writing&&m.pc==0x313870u){
            state.gp[2]+=0x10u;++result.mutations;
        }else if(control.feedback==2u&&writing&&m.pc==0x3138a1u){
            const auto address=state.gp[4]+0xcu;const auto bank=m.at(address,4);
            const std::uint32_t value=0x3f400000u;
            std::memcpy(m.banks.at(bank->first).data()+address-bank->first,&value,4);
            ++result.mutations;
        }
    };
    services.reciprocal_sqrt=[&](std::uint32_t bits){
        const auto nth=++result.estimate_count;
        if(nth==control.estimate_fault)throw std::runtime_error("Actual reciprocal estimate unavailable");
        const auto output=journal?journal->sample(bits):estimate_bits(bits);
        result.estimates.push_back(J{{"ordinal",nth},{"input",bits},{"output",output}});
        retire_policy(nth==1u?0x313875u:0x3138edu);
        if(control.feedback==3u&&nth==1u){
            ++result.mutations;
            const std::array lanes{0x3f000000u,0x40400000u,0x40800000u,0x40a00000u};
            std::memcpy(&state.xmm[3],lanes.data(),16);
        }
        if(control.feedback==4u&&nth==2u){
            ++result.mutations;
            throw native_solver_hull_basis_owner_detail::Continuation{0xdeadbeefu,&m};
        }
        return output;
    };
    const auto call_words=[&](WorldState&s,unsigned arity){
        const auto bank=m.at(s.gp[4],(arity+1u)*4u);
        std::array<std::uint32_t,4>words{};
        std::memcpy(words.data(),bank->second.data()+s.gp[4]-bank->first,(arity+1u)*4u);
        return words;
    };
    const auto measured=[&](WorldState&s,std::uint32_t target,std::uint32_t pc,unsigned width,unsigned arity,const auto&call){
        const auto words=call_words(s,arity);check(words[0]==pc+width,"Actual hull-basis source child RET changed");
        J args=J::array();for(unsigned n=0;n<arity;++n)args.push_back(words[n+1]);
        const auto index=result.children.size();
        result.children.push_back(J{{"target",target},{"site",pc},{"return",words[0]},{"arguments",args},{"entry",packet(s,target)}});
        ++result.child_depth;
        try{call();}catch(...){--result.child_depth;throw;}
        --result.child_depth;result.children[index]["return_state"]=packet(s,words[0]);
    };
    basis_children::child=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){
        if(target==0x313390u){
            const auto site=call_words(s,2u)[0]-5u;
            check(!pc&&(site==0x313499u||site==0x3134a9u),"Unexpected genuine basis-edge pair call site");
            measured(s,target,site,5u,2u,[&]{basis_children::pair(s,variant);});return;
        }
        check(pc==0x3133abu||pc==0x313425u,"Unexpected genuine basis shape call site");
        measured(s,target,pc,3u,2u,[&]{
            if(target==0x2e8d20u)basis_children::zero(s,variant);
            else if(target==0x306790u)basis_children::box(s,variant);
            else if(target==0x1f6610u){
                if(native_polygon_support_detail::admitted(s)){
                    auto noop=[](auto&,std::uint32_t){};
                    native_polygon_support_detail::support_retained<true>(s,noop);
                }else basis_children::polygon_source(s);
            }else throw std::runtime_error("Unqualified actual hull basis shape");
        });
    };
    basis_source::child=[&](WorldState&s,std::uint32_t target){
        const auto arity=target==0x313470u?3u:1u;
        const auto pc=call_words(s,arity)[0]-5u;
        check(target==0x313470u?(pc==0x31392cu||pc==0x313944u):
            target==0x313290u&&(pc==0x313962u||pc==0x313977u),"Unexpected actual hull basis direct child site");
        direct_child=pc;
        try{
            measured(s,target,pc,5u,arity,[&]{
                if(target==0x313470u)basis_children::edge(s,variant);
                else basis_children::append(s,variant);
            });
        }catch(...){direct_child=0u;throw;}
        direct_child=0u;
    };
    const auto before=native_solver_hull_basis_owner_detail::activity();
    const auto literal=basis_source::literals[variant];basis_source::suffixes[variant]=0u;
    try{basis_source::run(state,variant,reference,control.ordered_reference);}
    catch(const Boundary&){result.stopped=true;}
    catch(const native_solver_hull_basis_owner_detail::Continuation&resume){
        check(resume.owner==&m,"Foreign basis token owner changed");
        result.stopped=true;result.error="foreign continuation "+std::to_string(resume.pc);
    }catch(const std::runtime_error&e){result.stopped=true;result.error=e.what();}
    result.abi=packet(state,m.pc);m.feedback={};basis_source::child={};basis_children::child={};
    const auto after=native_solver_hull_basis_owner_detail::activity();
    result.native=after.calls-before.calls;result.completed=after.completed-before.completed;
    result.continued=after.continuations-before.continuations;result.suffix=basis_source::suffixes[variant];
    result.literal=basis_source::literals[variant]-literal;
    return result;
}
void compare(const RouteMemory&a,const RouteMemory&b,const Result&x,const Result&y,const std::string&name){
    if(x.abi==y.abi&&x.events==y.events&&x.statements==y.statements&&x.witnesses==y.witnesses&&
       x.estimates==y.estimates&&x.children==y.children&&x.stopped==y.stopped&&x.error==y.error&&x.mutations==y.mutations&&x.io==y.io&&
       a.operations==b.operations&&a.writes==b.writes&&a.banks==b.banks)return;
    std::cerr<<name<<" ABI="<<(x.abi==y.abi)<<" read/write="<<(a.operations==b.operations)
             <<" events="<<(x.events==y.events)<<" callbacks="<<(x.statements==y.statements)
             <<" estimates="<<(x.estimates==y.estimates)<<" children="<<(x.children==y.children)<<" exceptions="<<x.error<<"/"<<y.error<<'\n';
    if(x.estimates!=y.estimates)std::cerr<<x.estimates<<'\n'<<y.estimates<<'\n';
    for(std::size_t i=0;i<std::min(x.children.size(),y.children.size());++i)if(x.children[i]!=y.children[i]){
        std::cerr<<"first differing real child "<<i<<": "<<x.children[i]<<'\n'<<y.children[i]<<'\n';break;
    }
    if(x.abi!=y.abi)std::cerr<<x.abi<<'\n'<<y.abi<<'\n';
    throw std::runtime_error("Actual hull basis retained source differs");
}
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;
    const std::string mode=argc>1?argv[1]:"baseline";
    if(mode=="refusals"){
        unsigned refused{};
        for(unsigned n=0;n<22u;++n){
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
            case 19:services.required_owner=NamedProvider{&memory};break;
            case 20:
                services.required_owner=NamedProvider{&memory};
                services.required_owner_policy={&typeid(NamedProvider),std::this_thread::get_id(),&callbacks,matches_provider};break;
            case 21:{
                auto exact=NamedProvider{&memory};
                services.required_owner=[exact](auto&m,const auto&c,auto&a,const auto&o){exact(m,c,a,o);};
                services.required_owner_policy={&typeid(NamedProvider),std::this_thread::get_id(),&memory,matches_provider};break;
            }
            }
            const auto before=packet(state,memory.pc);
            const auto calls=native_solver_hull_basis_owner_detail::activity().calls;
            const auto refuse_estimate=[](auto&,auto)->std::uint32_t{throw std::runtime_error("Refused basis entered estimate");};
            const auto refuse_child=[](auto&,auto,auto){throw std::runtime_error("Refused basis entered child");};
            const auto result=n==18u?
                native_solver_hull_basis_owner_detail::bridge<false,false>(state,native_solver_hull_basis_source_detail::LegacySpill{},refuse_estimate,refuse_child):
                native_solver_hull_basis_owner_detail::bridge<true,false>(state,native_solver_hull_basis_source_detail::LegacySpill{},refuse_estimate,refuse_child);
            check(result==0u&&calls==native_solver_hull_basis_owner_detail::activity().calls&&callbacks==0u&&
                memory.operations.empty()&&memory.banks==original&&packet(state,memory.pc)==before,
                "Unqualified basis source touched entry state or DATA");
            ++refused;
        }
        std::cout<<J{{"untouched_refusals",refused}}.dump()<<'\n';return 0;
    }
    if(mode=="journal"){
        unsigned cases{},native_requests{},replay_requests{},replay_refusals{};
        for(const auto&row:rows){
            if(row.at("id").get<unsigned>()>=8u)break;
            const auto a=abi(row);const auto saved=seed(row);
            for(unsigned v=0;v<4u;++v){
                auto expected_memory=saved;SourceExecutionInputs capture;
                Result expected;
                {EstimateJournal journal(capture);expected=run(expected_memory,a,v,true,{},&journal);
                    check(!expected.stopped&&journal.computed==2u,"Real retained estimate recording did not complete two requests");native_requests+=journal.computed;}
                const auto entries=capture.finish();
                check(entries.size()==2u&&std::all_of(entries.begin(),entries.end(),[](const auto&entry){return entry.kind==SourceExecutionInputKind::ReciprocalSquareRoot;}),"Wrong real estimate journal shape");
                auto actual_memory=saved;SourceExecutionInputs replay(entries);Result actual;
                {EstimateJournal journal(replay);actual=run(actual_memory,a,v,false,{},&journal);
                    check(journal.computed==0u,"Estimate replay called the host producer");}
                check(replay.finish().size()==2u&&actual.estimates.size()==2u,"Estimate replay failed to consume both recorded inputs");
                compare(expected_memory,actual_memory,expected,actual,"actual two-operand estimate capture/replay");
                ++cases;replay_requests+=2u;
                for(unsigned n=0;n<2u;++n){
                    auto malformed=std::vector(entries.begin(),entries.end());malformed[n].operand^=1u;
                    auto left=saved,right=saved;SourceExecutionInputs l(malformed),r(malformed);Result x,y;
                    {EstimateJournal journal(l);x=run(left,a,v,true,{},&journal);check(journal.computed==0u,"Rejected replay called producer");}
                    {EstimateJournal journal(r);y=run(right,a,v,false,{},&journal);check(journal.computed==0u,"Rejected native replay called producer");}
                    compare(left,right,x,y,"actual wrong estimate operand stops before next statement");
                    check(x.stopped&&x.estimates.size()==n,"Wrong estimate replay operand was not rejected at the exact position");++replay_refusals;
                }
            }
        }
        std::cout<<J{{"capture_replay_pairs",cases},{"recorded_host_requests",native_requests},
                     {"consumed_replay_requests",replay_requests},{"rejected_operand_changes",replay_refusals}}.dump()<<'\n';return 0;
    }
    if(mode=="paired-nan"){
        const auto&row=rows.at(49);check(row.at("name")=="basis-49","Missing original paired-NaN discriminator");
        const auto input=abi(row);const auto saved=seed(row);
        J witness=J::array();
        for(unsigned v:{2u,3u}){
            auto legacy_memory=saved,phased_memory=saved,native_memory=saved;
            const auto legacy=run(legacy_memory,input,v,true);
            const auto phased=run(phased_memory,input,v,true,Control{.ordered_reference=true});
            const auto got=run(native_memory,input,v,false);
            compare(phased_memory,native_memory,phased,got,"original paired-NaN first child ABI");
            const auto lane=[](const Result&result){return result.children.at(0).at("entry").at("xmm").at(0).at(3).get<std::uint32_t>();};
            check(!legacy.stopped&&!phased.stopped&&!got.stopped&&got.native==1u&&got.completed==1u&&lane(got)==0xffc00000u,
                  "Native paired-NaN packet differs from unchanged i386");
            witness.push_back(J{{"variant",v},{"unmodified_retained_xmm0_w",lane(legacy)},
                                {"ordered_reference_xmm0_w",lane(phased)},{"native_xmm0_w",lane(got)},
                                {"unchanged_i386_xmm0_w",0xffc00000u}});
        }
        std::cout<<J{{"paired_nan_witnesses",witness}}.dump()<<'\n';return 0;
    }
    unsigned cases{},native{},complete{},retained{},continuations{},prefixes{},read_stops{},write_stops{},observer_stops{},fallback_callbacks{},estimate_calls{},child_entries{};
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
                continuations+=got.continued;fallback_callbacks+=got.callbacks;estimate_calls+=got.estimates.size();child_entries+=got.children.size();
                return got;
            };
            if(mode=="baseline"||mode=="own-reference"||mode=="raw-free"){
                const bool should_enter=mode=="raw-free"||(mode=="baseline"&&!raw);
                const auto got=one(Control{.callback_free=mode=="raw-free",.ordered_reference=should_enter});
                if(got.stopped||got.native!=unsigned(should_enter)||got.completed!=unsigned(should_enter)||
                   got.literal!=unsigned(!should_enter)||got.estimates.size()!=2u||got.children.size()!=15u){
                    std::cerr<<row.at("name")<<" variant="<<v<<" expected-native="<<should_enter<<" native="<<got.native
                             <<" completed="<<got.completed<<" continued="<<got.continued<<" suffix="<<std::hex<<got.suffix
                             <<std::dec<<" literals="<<got.literal<<" estimates="<<got.estimates.size()<<" children="<<got.children.size()
                             <<" exception="<<got.error<<'\n';
                    throw std::runtime_error("Basis selected the wrong source format or reference route");
                }
            }else if(mode=="frontiers"){
                for(const auto [pc,next]:basis_source::frontiers){
                    const auto got=one(Control{.revoke_pc=pc});
                    if(!got.mutations){check(!got.stopped&&!got.continued,"Unvisited basis frontier executed a suffix");continue;}
                    check(!got.stopped&&got.mutations==1u&&got.continued==unsigned(!raw)&&got.suffix==(raw?0u:next),
                          "Basis did not resume at its exact retired-I/O boundary");
                    frontiers.insert(pc);
                }
            }else if(mode=="feedback"){
                for(unsigned n=1;n<=4u;++n){
                    const auto got=one(Control{.feedback=n});
                    if(!got.mutations){check(!got.stopped,"Basis stopped without feedback");continue;}
                    check(got.mutations==1u&&got.stopped==(n==4u),"Missing actual basis feedback/foreign stop");
                    if(n==4u)check(got.error=="foreign continuation 3735928559","Wrong actual basis foreign token");
                }
                for(unsigned n=1;n<=2u;++n){
                    const auto got=one(Control{.estimate_fault=n});
                    check(got.stopped&&got.estimates.size()==n-1u&&got.error=="Actual reciprocal estimate unavailable","Wrong real estimate failure prefix");
                }
            }else if(mode=="child-return"){
                for(const auto site:{0x31392cu,0x313944u,0x313962u,0x313977u}){
                    const auto got=one(Control{.child_policy_site=site});
                    if(!got.mutations){check(!got.stopped&&!got.continued,"Unreached basis child returned to a suffix");continue;}
                    const auto next=site+5u;
                    check(!got.stopped&&got.mutations==1u&&got.continued==unsigned(!raw)&&got.suffix==(raw?0u:next),
                          "Basis did not adopt the exact genuine returned child continuation");
                    frontiers.insert(site);
                }
            }else if(mode=="prefixes"){
                auto baseline=saved;const auto before=run(baseline,a,v,true);
                for(const auto&[writing,n]:before.parent_provider_operations){
                    check((writing?one({},-1,int(n)):one({},int(n))).stopped,"Expected stopped basis provider");
                    if(writing)++write_stops;else ++read_stops;
                    ++prefixes;
                }
                for(unsigned n=1;n<=before.observed;++n){check(one(Control{.observer_fault=n}).stopped,"Expected stopped basis observer");++observer_stops;++prefixes;}
            }else if(mode=="raw-install"){
                auto baseline=saved;const auto before=run(baseline,a,v,true,Control{.callback_free=true});
                for(unsigned at=1;at<=before.parent_provider_operations.size();++at){
                    const auto got=one(Control{.install_callback_at=at,.callback_free=true});
                    check(!got.stopped&&got.native==1u&&got.mutations==1u&&
                          (got.continued==1u||(at==before.parent_provider_operations.size()&&got.completed==1u)),
                          "Raw basis callback installation skipped the required retained suffix");
                }
            }else throw std::runtime_error("Unknown basis route mode");
        }
    }
    if(mode=="frontiers")check(frontiers.size()==56u,"Basis test did not execute all 56 actual frontiers");
    if(mode=="child-return")check(frontiers.size()==4u,"Basis test did not exercise all four genuine child returns");
    std::cout<<J{{"source_cases",cases},{"native_entries",native},{"native_completed",complete},
                 {"retained_continuations",continuations},{"whole_entry_literals",retained},{"callbacks",fallback_callbacks},
                 {"exercised_frontiers",frontiers.size()},{"provider_and_observer_prefixes",prefixes},{"estimate_calls",estimate_calls},{"genuine_child_entries",child_entries},
                 {"read_prefixes",read_stops},{"write_prefixes",write_stops},{"observer_prefixes",observer_stops}}.dump()<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
