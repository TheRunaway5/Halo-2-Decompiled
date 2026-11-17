#include "physics/runtime/native_tu15_simplex_boundary_source_owner.hpp"
#define HALO2_SIMPLEX_BOUNDARY_TEST_COMPONENT_ONLY 1
#include "native_tu15_simplex_boundary_tests.cpp"
namespace boundary_routes {
#include "simplex_boundary_routes.inc"
}
namespace {
struct RequiredPacket {
    SourcePhysicsContactRuntimeCall call;
    SourcePhysicsWorldRuntimeAbi abi;
};
struct BoundaryRouteResult:PairedResult {
    std::uint64_t parent_native{},direct_triangle{},direct_closest{},dispatch_children{};
    std::uint64_t parent_literals{},child_literals{},entry_return_witnesses{},policy_mutations{},post_mutation_callbacks{};
    std::vector<RequiredPacket> required;
};
// This named test owner computes genuine authenticated children. Its certificate
// is deliberately limited to the immutable readonly mode; wrappers and the
// separately declared mutation modes never gain this contract.
struct BoundaryProviderOwner {
    PairedMemory&memory;SourcePhysicsWorldRuntimeServices&services;
    BoundaryRouteResult&out;WorldState*&current;unsigned mode;
    std::uint32_t entry_seen{};bool trace{};
    void capture(std::uint32_t target){
        if(!current||entry_seen==target)return;
        const auto packet=current->save();
        std::uint32_t ret{};std::memcpy(&ret,memory.view(packet.gp[4],4).data(),4);
        if((target==0x30d3f0u&&(ret!=0x30dc41u&&ret!=0x30dc76u))||
           (target==0x312620u&&ret!=0x30dd36u))
            throw std::runtime_error("Boundary real child return word differs");
        out.children.emplace_back(target,packet);++out.entry_return_witnesses;entry_seen=target;
    }
    void compute(const SourcePhysicsContactRuntimeCall&call,SourcePhysicsWorldRuntimeAbi&retained,
                 const SourcePhysicsContactRecordObserver&observe){
        if(call.tail||!((call.target==0x30d3f0u&&(call.instruction==0x30dc3cu||call.instruction==0x30dc71u))||
                        (call.target==0x312620u&&call.instruction==0x30dd31u)))
            throw std::runtime_error("Boundary required-owner packet identity differs");
        out.required.push_back({call,retained});capture(call.target);
        if(mode==2u)throw std::runtime_error("Declared observing provider stop");
        WorldState imported{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
        imported.load(retained);auto*parent=current;current=&imported;
        try{boundary_routes::known_child(imported,call.target);retained=imported.save();}
        catch(...){retained=imported.save();current=parent;throw;}
        current=parent;
        if(mode==6u){++out.policy_mutations;trace=true;services.instruction_cursor.first_trace=&trace;}
        if(mode==7u){++out.policy_mutations;services.instruction=[this](std::uint32_t pc){out.pc=pc;++out.callbacks;++out.post_mutation_callbacks;};}
    }
};
struct BoundaryReadonlyProvider {
    BoundaryProviderOwner*owner;
    void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&call,
                    SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsContactRecordObserver&observe)const{
        owner->compute(call,abi,observe);
    }
};
bool boundary_provider_matches(const SourcePhysicsWorldRuntimeServices::RequiredOwner&callback,const void*owner)noexcept{
    const auto*actual=callback.target<BoundaryReadonlyProvider>();
    return actual&&actual->owner==owner&&actual->owner->mode==1u;
}
void certify_boundary_provider(SourcePhysicsWorldRuntimeServices&services,BoundaryProviderOwner&owner){
    services.required_owner_policy={&typeid(BoundaryReadonlyProvider),std::this_thread::get_id(),&owner,boundary_provider_matches};
}
bool native_provider(unsigned provider){return provider==0u||provider==1u||provider==8u;}
BoundaryRouteResult boundary_source_run(PairedMemory&m,const SourcePhysicsWorldRuntimeAbi&abi,
                                      unsigned variant,unsigned provider,bool gold=false) {
    BoundaryRouteResult out;
    WorldState*current{};
    SourcePhysicsContactRecordObserver observer=[&](const auto&e){out.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    BoundaryProviderOwner owner{m,services,out,current,provider==8u||provider==4u||provider==5u?1u:provider};
    const auto capture=[&](std::uint32_t target){owner.capture(target);};
    services.instruction=[&](std::uint32_t pc){
        out.pc=pc;++out.callbacks;if(out.policy_mutations)++out.post_mutation_callbacks;
        if(pc==0x30d3f0u||pc==0x312620u)capture(pc);else owner.entry_seen=0u;
    };
    auto&cursor=services.instruction_cursor;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    cursor.position=&out.pc;cursor.service_sites=sites;
    cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    // Entry PUSH's real preimage read provides a native entry witness without
    // adding provider reads. Atomic PUSH has not committed SP at this point.
    m.on_read=[&](std::uint32_t){
        if(out.pc==0x30d3f0u||out.pc==0x312620u)capture(out.pc);else owner.entry_seen=0u;
    };
    if(provider){
        services.required_owner=BoundaryReadonlyProvider{&owner};
        if(provider==1u||provider==4u||provider==5u||provider==8u)certify_boundary_provider(services,owner);
        if(provider==4u){auto original=services.required_owner;services.required_owner=[original](auto&m,const auto&c,auto&a,const auto&o){original(m,c,a,o);};}
        if(provider==5u)services.required_owner_policy.owner=&out;
        if(provider==8u){auto copy=services.required_owner;services.required_owner=std::move(copy);}
    }
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);current=&state;boundary_routes::context=variant;
    const auto activity_before=native_simplex_boundary_owner_detail::activity();
    const auto literal_before=boundary_routes::literal_entries;
    const auto child_before=boundary_routes::child_literals;
    const auto call=[&](auto&s,std::uint32_t target,std::uint32_t site){
        if(!s.services.required_owner){
            if(target==0x30d3f0u&&native_simplex_triangle_detail::bridge_actual(s))return;
            if(target==0x312620u&&native_closest_segments_owner_detail::bridge_actual(s))return;
        }
        boundary_routes::dispatch(s,target,site,false);
    };
    try {
        if(gold&&!native_simplex_boundary_owner_detail::reference()&&native_provider(provider))
            native_simplex_boundary_detail::run_private_owner(state,call);
        else boundary_routes::run(state,variant,gold);
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=state.save();m.on_read={};
    const auto after=native_simplex_boundary_owner_detail::activity();
    out.parent_native=after.calls-activity_before.calls;
    out.direct_triangle=after.direct_triangle_calls-activity_before.direct_triangle_calls;
    out.direct_closest=after.direct_closest_calls-activity_before.direct_closest_calls;
    out.dispatch_children=after.dispatch_child_calls-activity_before.dispatch_child_calls;
    out.child_literals=boundary_routes::child_literals-child_before;
    for(unsigned n=0;n<6u;++n)out.parent_literals+=boundary_routes::literal_entries[n]-literal_before[n];
    return out;
}
void compare_route(const PairedMemory&a,const PairedMemory&b,const BoundaryRouteResult&x,
                   const BoundaryRouteResult&y,const std::string&label){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.callbacks!=y.callbacks||x.policy_mutations!=y.policy_mutations||x.post_mutation_callbacks!=y.post_mutation_callbacks||x.stopped!=y.stopped||a.bank!=b.bank||
       a.table!=b.table||a.constant!=b.constant||a.ones!=b.ones||a.sign!=b.sign||
       a.reads!=b.reads||x.stores!=y.stores||x.children.size()!=y.children.size()||
       x.required.size()!=y.required.size()){
        std::cerr<<label<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec
                 <<" child "<<x.children.size()<<"/"<<y.children.size()<<" required "<<x.required.size()<<"/"<<y.required.size()
                 <<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        throw std::runtime_error("Boundary complete source route differs "+label);
    }
    for(unsigned n=0;n<x.children.size();++n)
        if(x.children[n].first!=y.children[n].first||!same_abi(x.children[n].second,y.children[n].second))
            throw std::runtime_error("Boundary complete callee-entry ABI differs "+label+"-"+std::to_string(n));
    for(unsigned n=0;n<x.required.size();++n){const auto&u=x.required[n];const auto&v=y.required[n];
        if(u.call.instruction!=v.call.instruction||u.call.target!=v.call.target||u.call.tail!=v.call.tail||!same_abi(u.abi,v.abi))
            throw std::runtime_error("Boundary actual required-owner entry ABI differs "+label);
    }
}
void boundary_routing(unsigned provider){
    std::uint64_t cases{},parent_native{},parent_literals{},child_literals{},entries{},required{},stops{},
                  direct_triangle{},direct_closest{},dispatch{},return_witnesses{},legacy_differences{},mutations{},post_mutation_callbacks{};
    for(unsigned count=input_word(),k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>bytes{};std::cin.read(reinterpret_cast<char*>(bytes.data()),10);std::memcpy(&abi.x87[n],bytes.data(),10);}
        for(auto&v:abi.xmm)for(auto&w:v)w=input_word();PairedMemory seed;
        for(auto bytes:{std::span<std::byte>(seed.bank),std::span<std::byte>(seed.table),std::span<std::byte>(seed.constant),std::span<std::byte>(seed.ones),std::span<std::byte>(seed.sign)})
            std::cin.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        for(unsigned variant=0;variant<6u;++variant){
            PairedMemory a=seed,b=seed;
            const auto expected=boundary_source_run(a,abi,variant,provider,true);
            const auto got=boundary_source_run(b,abi,variant,provider);
            compare_route(a,b,expected,got,std::to_string(k)+"-"+std::to_string(variant));
            parent_native+=got.parent_native;parent_literals+=got.parent_literals;
            direct_triangle+=got.direct_triangle;direct_closest+=got.direct_closest;
            dispatch+=got.dispatch_children;child_literals+=got.child_literals;
            entries+=got.children.size();required+=got.required.size();stops+=got.stopped;
            return_witnesses+=got.entry_return_witnesses;mutations+=got.policy_mutations;post_mutation_callbacks+=got.post_mutation_callbacks;++cases;
            if(!native_simplex_boundary_owner_detail::reference()&&!native_simplex_triangle_detail::reference()&&
               !native_closest_segments_owner_detail::reference()&&native_provider(provider)&&(got.callbacks||got.parent_literals||got.child_literals))
                throw std::runtime_error("Admitted Boundary native route entered translated statements");
            if(provider==2u&&variant%3u!=1u&&!got.stopped)
                throw std::runtime_error("Observing provider did not retain real Boundary stop");
            if(native_simplex_boundary_owner_detail::reference()&&provider==1u){
                PairedMemory c=seed;const auto original_policy=boundary_run(c,abi,true);
                legacy_differences+=!same_abi(got.abi,original_policy.abi);
            }
        }
    }
    const bool reference=native_simplex_boundary_owner_detail::reference()||!native_provider(provider);
    if(reference?(parent_native||parent_literals!=cases):(parent_literals||parent_native!=cases))
        throw std::runtime_error("Boundary parent default/reference activity differs");
    if(provider>=3u&&provider<=7u&&!parent_literals)throw std::runtime_error("Uncertified provider skipped whole-entry fallback");
    if((provider==6u||provider==7u)&&(!required||!parent_literals||!mutations||!post_mutation_callbacks))throw std::runtime_error("Mutating provider did not execute genuine external child path");
    if(entries!=return_witnesses||(!provider&&!reference&&!native_simplex_triangle_detail::reference()&&
       !native_closest_segments_owner_detail::reference()&&direct_triangle+direct_closest!=entries)||
       (provider&&(direct_triangle||direct_closest)))
        throw std::runtime_error("Boundary direct/provider entry accounting differs");
    std::cerr<<"{\"source_cases\":"<<cases<<",\"source_variants\":6,\"parent_native_calls\":"<<parent_native
             <<",\"parent_literal_entries\":"<<parent_literals<<",\"direct_triangle_calls\":"<<direct_triangle
             <<",\"direct_closest_calls\":"<<direct_closest<<",\"original_dispatch_child_calls\":"<<dispatch
             <<",\"child_literal_entries\":"<<child_literals<<",\"complete_child_entry_packets\":"<<entries
             <<",\"actual_required_owner_packets\":"<<required<<",\"real_provider_or_absent_owner_stops\":"<<stops
             <<",\"actual_return_word_witnesses\":"<<return_witnesses<<",\"policy_mutations\":"<<mutations<<",\"callbacks_after_mutation\":"<<post_mutation_callbacks<<",\"forced_old_parent_abi_differences\":"<<legacy_differences<<"}\n";
}
void boundary_refusals(){unsigned cases{};for(unsigned kind=0;kind<32u;++kind){
    PairedMemory memory;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
    SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;
    std::uint32_t pc{};services.instruction=[](std::uint32_t){};
    std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    BoundaryRouteResult provider_out;WorldState*current{};BoundaryProviderOwner provider_owner{memory,services,provider_out,current,1u};
    if(kind>=24u){services.required_owner=BoundaryReadonlyProvider{&provider_owner};certify_boundary_provider(services,provider_owner);}
    if(kind==11u)cursor.literal_service_sites=false;
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    if(kind!=11u&&!native_simplex_boundary_owner_detail::admitted(state))throw std::runtime_error("Boundary baseline admission refused");
    bool trace=true;auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto mxcsr=_mm_getcsr();std::uint16_t cw{};asm volatile("fnstcw %0":"=m"(cw));
    switch(kind){
    case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;
    case 2:state.active_mmx.emplace();break;case 3:state.fp.count=7u;break;case 4:state.fp.top=8u;break;
    case 5:state.fp.status|=0x8080u;break;case 6:services.instruction={};break;
    case 7:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;
    case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;
    case 10:cursor.literal_service_sites=false;break;case 11:cursor.literal_service_sites=true;break;
    case 12:state.instruction_cursor=&external;break;case 13:cursor.callback_type=nullptr;break;
    case 14:sites[2]^=1u;break;case 15:cursor.service_sites=std::span(sites).first(2);break;
    case 16:cursor.first_trace=&trace;break;case 17:cursor.second_trace=&trace;break;
    case 18:cursor.observation_only_stores=false;break;case 19:cursor.callback_type=&typeid(int);break;
    case 20:_mm_setcsr(mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;
    case 22:cursor.service_sites=extra;break;case 23:{const auto changed=std::uint16_t(cw&~1u);asm volatile("fldcw %0"::"m"(changed));break;}
    }
    if(kind>=24u)switch(kind){
    case 24:services.required_owner_policy={};break;
    case 25:services.required_owner_policy.callback_type=&typeid(int);break;
    case 26:services.required_owner_policy.owner=&provider_out;break;
    case 27:services.required_owner_policy.executor=std::thread::id{};break;
    case 28:services.required_owner_policy.matches=nullptr;break;
    case 29:{auto old=services.required_owner;services.required_owner=[old](auto&m,const auto&c,auto&a,const auto&o){old(m,c,a,o);};break;}
    case 30:provider_owner.mode=6u;break;
    case 31:services.required_owner=[](auto&,const auto&,auto&,const auto&){};break;
    }
    const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);
    const auto flags=state.flags.bits(),other=state.other_flags,count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
    const auto accepted=native_simplex_boundary_owner_detail::bridge(state,[](auto&,std::uint32_t,std::uint32_t){});
    const bool unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&
        state.other_flags==other&&state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&
        memory.read_count==0u&&memory.write_count==0u&&pc==0u;
    _mm_setcsr(mxcsr);asm volatile("fldcw %0"::"m"(cw));if(accepted||!unchanged)throw std::runtime_error("Boundary untouched live refusal failed "+std::to_string(kind));++cases;
    }std::cerr<<"{\"untouched_live_refusals\":"<<cases<<"}\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--refusals")boundary_refusals();else boundary_routing(argc==2?unsigned(std::stoul(argv[1])):0u);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
