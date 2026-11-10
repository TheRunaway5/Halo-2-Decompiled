#include "physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp"
#define main refinement_core_main
#include "native_tu15_simplex_face_refine_tests.cpp"
#undef main
namespace route_reference {
#include "simplex_face_refine_routes.inc"
}
namespace {
void routes(){
    std::mt19937 rng(0x30d1a1u);unsigned cases{},literal_entries{};
    for(unsigned k=0;k<96u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[7]=base+0x2000u;abi.gp[4]=stack+k%16u;
        auto point=base+0x1000u,scores=base+0x3000u;const auto frame_base=((abi.gp[4]-4u)&~15u)-0x20u;
        if(k%16u==6u)abi.gp[7]=frame_base-0x30u;if(k%16u==10u)scores=frame_base+0x10u;
        for(auto p:{point,base+0x1100u,abi.gp[7]})for(unsigned row=0;row<12u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        for(unsigned n=0;n<3u;++n)seed.put(scores+n*4u,std::bit_cast<std::uint32_t>(float(int((k>>(n*2u))%4u)-1)));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,scores);constexpr std::array<unsigned,5>indices{2,0,1,2,0};for(unsigned n=0;n<indices.size();++n)seed.table[0x760u+n]=std::byte(indices[n]);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,4u,6u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        for(unsigned variant=0;variant<8u;++variant){
            Memory a=seed,b=seed;const auto expected=run(a,abi,false);Result got;SourcePhysicsContactRecordObserver observer=[&](const auto&e){got.stores.push_back(e);};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){got.pc=pc;++got.callbacks;};auto&cursor=services.instruction_cursor;
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&got.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
            physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{b,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);route_reference::run(state,variant);got.abi=state.save();
            if(!same_abi(expected.abi,got.abi)||expected.pc!=got.pc||a.bank!=b.bank||a.table!=b.table||a.constant!=b.constant||a.reads!=b.reads||expected.stores!=got.stores)throw std::runtime_error("Source face-refine route differs "+std::to_string(k)+"-"+std::to_string(variant));
            if(!native_simplex_face_refine_owner_detail::reference()&&got.callbacks)throw std::runtime_error("Native source face-refine route used callbacks");++cases;
        }
    }
    for(auto v:route_reference::literal_entries)literal_entries+=v;auto&activity=native_simplex_face_refine_owner_detail::activity();
    if(native_simplex_face_refine_owner_detail::reference()?(activity.actual_calls||literal_entries!=cases):(activity.actual_calls!=cases||literal_entries))throw std::runtime_error("Source face-refine routing activity differs");
    std::cerr<<"{\"bound_source_cases\":"<<cases<<",\"source_variants\":8,\"native_calls\":"<<activity.actual_calls<<",\"literal_body_entries\":"<<literal_entries<<"}\n";
}
void refusals(){
    unsigned cases{};
    for(unsigned kind=0;kind<23u;++kind){
        Memory m;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[](std::uint32_t){};
        std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        if(kind==11u)cursor.literal_service_sites=false;physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);if(kind!=11u&&!native_simplex_face_refine_owner_detail::admitted(state))throw std::runtime_error("Face-refine guard baseline refused");
        bool trace=true;auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto host_mxcsr=_mm_getcsr();
        switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.active_mmx.emplace();break;case 3:state.fp.count=7u;break;case 4:state.fp.top=8u;break;case 5:state.fp.status|=0x8080u;break;case 6:services.instruction={};break;case 7:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;case 10:cursor.literal_service_sites=false;break;case 11:cursor.literal_service_sites=true;break;case 12:state.instruction_cursor=&external;break;case 13:cursor.callback_type=nullptr;break;case 14:sites[2]^=1u;break;case 15:cursor.service_sites=std::span(sites).first(2);break;case 16:cursor.first_trace=&trace;break;case 17:cursor.second_trace=&trace;break;case 18:cursor.observation_only_stores=false;break;case 19:cursor.callback_type=&typeid(int);break;case 20:_mm_setcsr(host_mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:cursor.service_sites=extra;break;}
        const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags;const auto count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        const auto accepted=native_simplex_face_refine_owner_detail::bridge(state);const auto unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&m.read_count==0u&&m.write_count==0u&&pc==0u;_mm_setcsr(host_mxcsr);
        if(accepted||!unchanged)throw std::runtime_error("Face-refine live refusal failed "+std::to_string(kind));++cases;
    }
    std::cerr<<"{\"untouched_live_refusals\":"<<cases<<"}\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--refusals")refusals();else routes();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
