#include "physics/runtime/native_tu15_simplex_paired_source_owner.hpp"
#include "physics/runtime/native_tu15_simplex_triangle_source_owner.hpp"
#include "physics/runtime/native_tu15_closest_segments_source_owner.hpp"
#include "physics/runtime/native_tu15_simplex_helpers_handoff_internal.hpp"
#define HALO2_SIMPLEX_PAIRED_TEST_COMPONENT_ONLY 1
#include "native_tu15_simplex_paired_tests.cpp"
struct ActualPairedState:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
namespace paired_routes {
#include "simplex_paired_routes.inc"
}
namespace {
// The installed source dispatch owns both children locally; this provider
// therefore remains uninvoked in these source-route cases. Its exact body is
// nevertheless a genuine same-bank child computation if called, not an outcome
// supplier. Actual returned-provider mutation is exercised by Boundary's real
// external-owner route, separately from this whole-entry guard coverage.
struct PairedProviderOwner {
    PairedMemory&memory;SourcePhysicsWorldRuntimeServices&services;unsigned mode;
    unsigned calls{};bool trace{};
    void compute(const SourcePhysicsContactRuntimeCall&call,SourcePhysicsWorldRuntimeAbi&abi,
                 const SourcePhysicsContactRecordObserver&observe){
        ++calls;WorldState imported{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};imported.load(abi);
        try{paired_routes::known_child(imported,call.target);abi=imported.save();}
        catch(...){abi=imported.save();throw;}
        if(mode==5u){trace=true;services.instruction_cursor.first_trace=&trace;}
    }
};
struct PairedReadonlyProvider {
    PairedProviderOwner*owner;
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&call,
                    SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsContactRecordObserver&observe)const{owner->compute(call,abi,observe);}
};
bool paired_provider_matches(const SourcePhysicsWorldRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
    const auto*actual=callback.target<PairedReadonlyProvider>();return actual&&actual->owner==owner&&actual->owner->mode==1u;
}
void certify_paired_provider(SourcePhysicsWorldRuntimeServices&services,PairedProviderOwner&owner){
    services.required_owner_policy={&typeid(PairedReadonlyProvider),std::this_thread::get_id(),&owner,paired_provider_matches};
}
bool native_provider(unsigned provider){return provider==0u||provider==1u||provider==6u;}
PairedResult normalized(PairedResult value){
    const auto canonical=[](std::uint32_t pc){return (pc>=0x30d830u&&pc<=0x30dc05u)||(pc>=0x30d3f0u&&pc<=0x30d5c2u)||(pc>=0x312620u&&pc<=0x3128b1u)?pc-0x3af0u:pc;};
    value.pc=canonical(value.pc);for(auto&e:value.stores)e.instruction=canonical(e.instruction);return value;
}
struct RouteResult:PairedResult{std::uint64_t parent_actual{},parent_canonical{},triangle_native{},closest_native{},child_literals{},parent_literals{},site_witnesses{},provider_calls{};};
template<class State>RouteResult paired_source_run(PairedMemory&m,const SourcePhysicsWorldRuntimeAbi&abi,unsigned variant,unsigned provider,bool gold=false){
    RouteResult out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.stores.push_back(e);};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;++out.callbacks;};auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    PairedProviderOwner owner{m,services,provider==3u||provider==4u||provider==6u?1u:provider};
    if(provider){services.required_owner=PairedReadonlyProvider{&owner};
        if(provider==1u||provider==3u||provider==4u||provider==6u)certify_paired_provider(services,owner);
        if(provider==3u){auto old=services.required_owner;services.required_owner=[old](auto&m,const auto&c,auto&a,const auto&o){old(m,c,a,o);};}
        if(provider==4u)services.required_owner_policy.owner=&out;
        if(provider==6u){auto copy=services.required_owner;services.required_owner=std::move(copy);}
    }
    physics_step_detail::FpScope environment(*abi.floating_point);State state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto parent_before=native_simplex_paired_owner_detail::activity();const auto triangle_before=native_simplex_triangle_detail::activity();const auto closest_before=native_closest_segments_owner_detail::activity();const auto literal_before=paired_routes::literal_entries;const auto child_literal_before=paired_routes::child_literals;const auto site_before=paired_routes::site_witnesses;
    paired_routes::child_packets.clear();
    if(gold){
        if(native_simplex_paired_owner_detail::reference()||!native_provider(provider)){
            if constexpr(requires{state.selected_observation_pcs;})paired_routes::run_actual(state,variant,true);else paired_routes::run_canonical(state,variant,true);
        }else native_simplex_paired_detail::run_private_owner(state,[](auto&s,auto target,auto site){paired_routes::dispatch(s,target,site,false);});
    }else if constexpr(requires{state.selected_observation_pcs;})paired_routes::run_actual(state,variant);else paired_routes::run_canonical(state,variant);
    out.provider_calls=owner.calls;out.abi=state.save();out.children=paired_routes::child_packets;const auto&p=native_simplex_paired_owner_detail::activity();const auto&t=native_simplex_triangle_detail::activity();const auto&c=native_closest_segments_owner_detail::activity();out.parent_actual=p.actual_calls-parent_before.actual_calls;out.parent_canonical=p.canonical_calls-parent_before.canonical_calls;out.triangle_native=t.actual_calls+t.canonical_calls-triangle_before.actual_calls-triangle_before.canonical_calls;out.closest_native=c.actual_calls+c.canonical_calls-closest_before.actual_calls-closest_before.canonical_calls;out.child_literals=paired_routes::child_literals-child_literal_before;out.site_witnesses=paired_routes::site_witnesses-site_before;for(unsigned n=0;n<6u;++n)out.parent_literals+=paired_routes::literal_entries[n]-literal_before[n];return out;
}
void routing(unsigned provider){unsigned cases{},parent_actual{},parent_canonical{},parent_literals{},triangle_native{},closest_native{},child_literals{},child_entries{},witnesses{},legacy_differences{};
    for(unsigned count=input_word(),k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10);std::memcpy(&abi.x87[n],b.data(),10);}for(auto&v:abi.xmm)for(auto&w:v)w=input_word();PairedMemory seed;for(auto b:{std::span<std::byte>(seed.bank),std::span<std::byte>(seed.table),std::span<std::byte>(seed.constant),std::span<std::byte>(seed.ones),std::span<std::byte>(seed.sign)})std::cin.read(reinterpret_cast<char*>(b.data()),b.size());
        for(unsigned variant=0;variant<6u;++variant){PairedMemory a=seed,b=seed;const bool actual=variant==0u||variant==3u;const auto expected=(native_simplex_paired_owner_detail::reference()||!native_provider(provider))?(actual?paired_source_run<ActualPairedState>(a,abi,variant,provider,true):paired_source_run<WorldState>(a,abi,variant,provider,true)):paired_source_run<ActualPairedState>(a,abi,0u,provider,true);const auto got=actual?paired_source_run<ActualPairedState>(b,abi,variant,provider):paired_source_run<WorldState>(b,abi,variant,provider);const auto x=normalized(expected),y=normalized(got);
            if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.callbacks!=y.callbacks||a.bank!=b.bank||a.ones!=b.ones||a.sign!=b.sign||a.table!=b.table||a.constant!=b.constant||a.reads!=b.reads||x.stores!=y.stores||x.children.size()!=y.children.size())throw std::runtime_error("Paired complete source route differs "+std::to_string(k)+"-"+std::to_string(variant));
            for(unsigned n=0;n<x.children.size();++n)if(x.children[n].first!=y.children[n].first||!same_abi(x.children[n].second,y.children[n].second))throw std::runtime_error("Paired source callee-entry packet differs");
            if(got.provider_calls)throw std::runtime_error("Static Paired child dispatch unexpectedly called required provider");
            parent_actual+=got.parent_actual;parent_canonical+=got.parent_canonical;parent_literals+=got.parent_literals;triangle_native+=got.triangle_native;closest_native+=got.closest_native;child_literals+=got.child_literals;child_entries+=got.children.size();witnesses+=got.site_witnesses;
            if(!native_simplex_paired_owner_detail::reference()&&native_provider(provider)&&got.callbacks&&!native_simplex_triangle_detail::reference()&&!native_closest_segments_owner_detail::reference())throw std::runtime_error("Paired native closure emitted literal callbacks");
            if(native_simplex_paired_owner_detail::reference()){PairedMemory gold=seed;const auto hardware_policy=paired_run(gold,abi,true);legacy_differences+=!same_abi(got.abi,hardware_policy.abi);}++cases;
        }
    }
    const bool reference=native_simplex_paired_owner_detail::reference()||!native_provider(provider);if(reference?(parent_literals!=cases||parent_actual||parent_canonical):(parent_literals||parent_actual!=cases/3u||parent_canonical!=cases*2u/3u))throw std::runtime_error("Paired parent source activity differs");
    if(!native_simplex_triangle_detail::reference()&&!native_closest_segments_owner_detail::reference()?(triangle_native+closest_native!=child_entries||child_literals):(triangle_native+closest_native+child_literals!=child_entries))throw std::runtime_error("Paired child activity differs");
    if(!reference&&witnesses!=child_entries)throw std::runtime_error("Paired internal callsite/observation witnesses missing");
    std::cerr<<"{\"source_cases\":"<<cases<<",\"source_variants\":6,\"parent_actual_native_calls\":"<<parent_actual<<",\"parent_canonical_native_calls\":"<<parent_canonical<<",\"parent_literal_entries\":"<<parent_literals<<",\"triangle_native_calls\":"<<triangle_native<<",\"closest_native_calls\":"<<closest_native<<",\"child_literal_entries\":"<<child_literals<<",\"genuine_child_entries\":"<<child_entries<<",\"dispatch_observation_witnesses\":"<<witnesses<<",\"actual_required_provider_calls\":0,\"forced_legacy_abi_differences\":"<<legacy_differences<<"}\n";
}
void refusals(){unsigned cases{};for(unsigned kind=0;kind<32u;++kind){
    PairedMemory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[](std::uint32_t){};std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    PairedProviderOwner provider_owner{memory,services,1u};if(kind>=24u){services.required_owner=PairedReadonlyProvider{&provider_owner};certify_paired_provider(services,provider_owner);}
    if(kind==11u)cursor.literal_service_sites=false;physics_step_detail::FpScope environment(*abi.floating_point);ActualPairedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);if(kind!=11u&&!native_simplex_paired_owner_detail::admitted(state))throw std::runtime_error("Paired baseline admission refused");bool trace=true;auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto mxcsr=_mm_getcsr();
    switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.active_mmx.emplace();break;case 3:state.fp.count=7u;break;case 4:state.fp.top=8u;break;case 5:state.fp.status|=0x8080u;break;case 6:services.instruction={};break;case 7:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;case 10:cursor.literal_service_sites=false;break;case 11:cursor.literal_service_sites=true;break;case 12:state.instruction_cursor=&external;break;case 13:cursor.callback_type=nullptr;break;case 14:sites[2]^=1u;break;case 15:cursor.service_sites=std::span(sites).first(2);break;case 16:cursor.first_trace=&trace;break;case 17:cursor.second_trace=&trace;break;case 18:cursor.observation_only_stores=false;break;case 19:cursor.callback_type=&typeid(int);break;case 20:_mm_setcsr(mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:cursor.service_sites=extra;break;case 23:state.selected_observation_pcs=false;break;}
    if(kind>=24u)switch(kind){case 24:services.required_owner_policy={};break;case 25:services.required_owner_policy.callback_type=&typeid(int);break;case 26:services.required_owner_policy.owner=&memory;break;case 27:services.required_owner_policy.executor=std::thread::id{};break;case 28:services.required_owner_policy.matches=nullptr;break;case 29:{auto old=services.required_owner;services.required_owner=[old](auto&m,const auto&c,auto&a,const auto&o){old(m,c,a,o);};break;}case 30:provider_owner.mode=5u;break;case 31:services.required_owner=[](auto&,const auto&,auto&,const auto&){};break;}
    const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags;const auto count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;const auto accepted=native_simplex_paired_owner_detail::bridge(state,[](auto&,std::uint32_t,std::uint32_t){});const bool unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&memory.read_count==0u&&memory.write_count==0u&&pc==0u;_mm_setcsr(mxcsr);if(accepted||!unchanged)throw std::runtime_error("Paired live refusal failed "+std::to_string(kind));++cases;
    }std::cerr<<"{\"untouched_live_refusals\":"<<cases<<"}\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--refusals")refusals();else routing(argc==2?unsigned(std::stoul(argv[1])):0u);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
