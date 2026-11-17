#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"
#include <iostream>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace zero_routes {
#include "zero_shape_support_routes.inc"
}
namespace {
struct Result{J abi;unsigned callbacks{},provider_calls{},native{},literal{},observed{};bool stopped{};};
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&abi,unsigned variant,unsigned mode,bool gold,bool force_native=false,unsigned observer_fault=0u,bool source_reference=false){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&){if(++out.observed==observer_fault)throw Boundary{};};
    if(mode==4u)observe={};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t pc){memory.pc=pc;++out.callbacks;};
    auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};bool trace=mode==1u;
    cursor.position=&memory.pc;cursor.first_trace=&trace;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    // This pure leaf never invokes a required owner; no certificate or computed
    // child outcome is asserted by this presence control.
    if(mode==3u)services.required_owner=[&](auto&,const auto&,auto&,const auto&){++out.provider_calls;throw std::runtime_error("Pure zero support called a provider");};
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    if(mode==2u)services.instruction=[&,changed=1u](std::uint32_t pc){memory.pc=pc;++out.callbacks;(void)changed;};
    const auto before=native_zero_shape_support_owner_detail::activity();const auto literal=zero_routes::literals;
    try{if(source_reference)zero_routes::run(state,variant,true);
    else if(force_native||(gold&&!native_zero_shape_support_owner_detail::reference()&&mode!=1u&&mode!=2u))native_zero_shape_support_detail::run_private_owner(state);
    else zero_routes::run(state,variant,gold);}catch(const Boundary&){out.stopped=true;}
    out.abi=packet(state,memory.pc);out.native=native_zero_shape_support_owner_detail::activity()-before;for(unsigned n=0;n<4;++n)out.literal+=zero_routes::literals[n]-literal[n];return out;
}
void routing(unsigned mode){J inputs;std::cin>>inputs;unsigned cases{},native{},literal{},callbacks{},legacy_differences{};
    for(const auto&row:inputs){Memory seed;for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        for(unsigned variant=0;variant<4u;++variant){Memory a=seed,b=seed;const auto expected=run(a,abi,variant,mode,true),got=run(b,abi,variant,mode,false);
            check(expected.abi==got.abi&&expected.callbacks==got.callbacks&&a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes,"Complete zero support source route differs "+std::to_string(row.at("id").get<unsigned>())+"-"+std::to_string(variant));
            check(!got.provider_calls,"Pure zero support invoked a required provider");native+=got.native;literal+=got.literal;callbacks+=got.callbacks;++cases;
            if(native_zero_shape_support_owner_detail::reference()){Memory c=seed;const auto original_policy=run(c,abi,variant,0u,true,true);legacy_differences+=original_policy.abi!=got.abi;}
        }
    }
    const bool reference=native_zero_shape_support_owner_detail::reference()||mode==1u||mode==2u;
    check(reference?(!native&&literal==cases&&callbacks>0u):(native==cases&&!literal&&!callbacks),"Zero support source native/reference accounting differs");
    std::cout<<J{{"source_cases",cases},{"source_variants",4},{"parent_native_calls",native},{"parent_literal_entries",literal},{"literal_callbacks",callbacks},{"required_provider_calls",0},{"old_source_abi_differences",legacy_differences}}.dump()<<"\n";
}
void source_publication(){J inputs;std::cin>>inputs;unsigned comparisons{},physical_differences{};
    const auto equal=[](const Memory&a,const Memory&b,const Result&x,const Result&y){
        check(x.abi==y.abi&&x.stopped==y.stopped&&a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes,"Retained source publication differs");
        check(!x.native&&!y.callbacks,"Retained/native source publication routing differs");
    };
    for(const auto&row:inputs){if(row.at("id").get<unsigned>()>=16u)continue;Memory seed;for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        for(unsigned variant=0;variant<4u;++variant){Memory a=seed,b=seed;const auto normal=run(a,abi,variant,0u,true,false,0u,true),got=run(b,abi,variant,0u,false);equal(a,b,normal,got);check(!got.stopped,"Normal source publication stopped");if(variant<2u)check(normal.callbacks>0u,"Raw retained body did not execute literal callbacks");++comparisons;
            for(unsigned kind=0;kind<3u;++kind){const auto limit=kind==0u?a.read_count:kind==1u?a.write_count:normal.observed;
                for(unsigned n=0;n<limit;++n){Memory c=seed,d=seed;if(kind==0u)c.read_fault=d.read_fault=n;if(kind==1u)c.write_fault=d.write_fault=n;
                    const auto x=run(c,abi,variant,0u,true,false,kind==2u?n+1u:0u,true),y=run(d,abi,variant,0u,false,false,kind==2u?n+1u:0u);equal(c,d,x,y);check(x.stopped&&y.stopped,"Source publication prefix did not stop");++comparisons;
                    if(n==0u&&kind<2u){Memory physical=seed;physical.read_fault=c.read_fault;physical.write_fault=c.write_fault;const auto raw=run(physical,abi,variant,0u,true,true);
                        check(raw.stopped&&raw.abi.at("gp").at(4)==abi.gp[4]&&x.abi.at("gp").at(4)==abi.gp[4]-4u,"Physical/source PUSH policies were conflated");++physical_differences;}
                }
            }
        }
    }
    std::cout<<J{{"retained_source_normal_and_throw_comparisons",comparisons},{"genuine_physical_vs_retained_push_differences",physical_differences},{"source_reference_uses_actual_State_push",true},{"source_reference_push_rewritten",false}}.dump()<<"\n";
}
void refusals(){unsigned count{};for(unsigned kind=0;kind<24u;++kind){Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer=[](const auto&){};services.instruction=[](std::uint32_t){};std::uint32_t pc{};auto&cursor=services.instruction_cursor;std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    if(kind==11u)cursor.literal_service_sites=false;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);if(kind!=11u)check(native_zero_shape_support_owner_detail::admitted(state),"Zero support baseline admission refused");bool trace=true;auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto mxcsr=_mm_getcsr();std::uint16_t cw{};asm volatile("fnstcw %0":"=m"(cw));
    switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.active_mmx.emplace();break;case 3:state.fp.count=9u;break;case 4:state.fp.top=8u;break;case 5:state.fp.status|=0x8080u;break;case 6:services.instruction={};break;case 7:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;case 10:cursor.literal_service_sites=false;break;case 11:cursor.literal_service_sites=true;break;case 12:state.instruction_cursor=&external;break;case 13:cursor.callback_type=nullptr;break;case 14:sites[2]^=1u;break;case 15:cursor.service_sites=std::span(sites).first(2);break;case 16:cursor.first_trace=&trace;break;case 17:cursor.second_trace=&trace;break;case 18:cursor.observation_only_stores=false;break;case 19:cursor.callback_type=&typeid(int);break;case 20:_mm_setcsr(mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:cursor.service_sites=extra;break;case 23:{const auto changed=std::uint16_t(cw&~1u);asm volatile("fldcw %0"::"m"(changed));break;}}
    const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags,depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;const auto accepted=native_zero_shape_support_owner_detail::bridge(state);const bool unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==depth&&state.fp.top==top&&state.fp.status==status&&memory.read_count==0u&&memory.write_count==0u&&pc==0u;_mm_setcsr(mxcsr);asm volatile("fldcw %0"::"m"(cw));check(!accepted&&unchanged,"Zero support untouched live refusal failed "+std::to_string(kind));++count;
    }std::cout<<J{{"untouched_live_refusals",count}}.dump()<<"\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--refusals")refusals();else if(argc==2&&std::string(argv[1])=="--source-publication")source_publication();else routing(argc==2?unsigned(std::stoul(argv[1])):0u);return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
