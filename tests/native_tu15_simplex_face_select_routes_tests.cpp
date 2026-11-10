#include "physics/runtime/native_tu15_simplex_face_select_source_owner.hpp"
#include "physics/runtime/native_tu15_simplex_face_refine_source_owner.hpp"
#define HALO2_SIMPLEX_FACE_SELECT_TEST_COMPONENT_ONLY 1
#include "native_tu15_simplex_face_select_tests.cpp"
#undef HALO2_SIMPLEX_FACE_SELECT_TEST_COMPONENT_ONLY
struct ActualState:WorldState{using WorldState::WorldState;bool selected_observation_pcs=true;};
namespace route_reference {
#include "simplex_face_select_routes.inc"
}
namespace {
SelectResult normalize(SelectResult value){
    for(auto&e:value.stores)if(e.instruction>=0x30d5d0u&&e.instruction<=0x30d82bu)e.instruction-=0x3af0u;
    if(value.pc>=0x30d5d0u&&value.pc<=0x30d82bu)value.pc-=0x3af0u;return value;
}
template<class State>SelectResult source_run(SelectMemory&m,const SourcePhysicsWorldRuntimeAbi&abi,unsigned variant){
    SelectResult out;SourcePhysicsContactRecordObserver observer=[&](const auto&e){out.stores.push_back(e);};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;++out.callbacks;};auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);State state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);const auto before=route_reference::child_entries;
    if constexpr(requires{state.selected_observation_pcs;})route_reference::run_actual(state,variant);else route_reference::run_canonical(state,variant);
    out.child_entries=route_reference::child_entries-before;out.abi=state.save();return out;
}
void routing(){std::mt19937 rng(0x30d5d0u);unsigned cases{},children{};
    for(unsigned k=0;k<192u;++k){
        SelectMemory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[0]=base+0x2000u;abi.gp[3]=base+0x1000u;abi.gp[4]=stack+k%16u;const auto frame_base=((abi.gp[4]-4u)&~15u)-0x60u;
        if(k%16u==6u)abi.gp[0]=frame_base+0x10u;if(k%16u==10u)abi.gp[3]=frame_base+0x30u;
        for(auto p:{abi.gp[0],abi.gp[3]})for(unsigned row=0;row<8u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        if(k<32u){constexpr std::array<float,16>tetra{0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0};for(unsigned n=0;n<16u;++n)seed.put(abi.gp[0]+n*4u,std::bit_cast<std::uint32_t>(k%8u==5u?0.0f:tetra[n]));for(unsigned lane=0;lane<4u;++lane)seed.put(abi.gp[3]+lane*4u,std::bit_cast<std::uint32_t>(lane==3u?0.0f:k%8u==2u?2.0f:0.25f));}
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,std::bit_cast<std::uint32_t>(k%7u==0u?0.0f:k%7u==1u?-1.0f:1.0f));seed.put(abi.gp[4]+8u,0x23456789u);
        constexpr std::array<unsigned,5>indices{2,0,1,2,0};for(unsigned n=0;n<5u;++n)seed.table[0x760u+n]=std::byte(indices[n]);seed.put(0x46a190u,0x3f8ccccdu);seed.put(0x46a454u,0x358637beu);const std::uint32_t absolute_mask=0x7fffffffu;for(unsigned lane=0;lane<4u;++lane)std::memcpy(seed.mask.data()+0x950u+lane*4u,&absolute_mask,4);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,3u,5u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        for(unsigned variant=0;variant<6u;++variant){const bool actual=variant==0u||variant==3u;SelectMemory a=seed,b=seed;auto expected=select_run(a,abi,false);if(!actual)expected=normalize(expected);const auto got=actual?source_run<ActualState>(b,abi,variant):source_run<WorldState>(b,abi,variant);
            if(!same_abi(expected.abi,got.abi)||expected.pc!=got.pc||expected.child_entries!=got.child_entries||a.bank!=b.bank||a.reads!=b.reads||expected.stores!=got.stores)throw std::runtime_error("Face-select source route differs "+std::to_string(k)+"-"+std::to_string(variant));
            children+=got.child_entries;++cases;
        }
    }
    unsigned literals{};for(auto n:route_reference::literal_entries)literals+=n;auto&p=native_simplex_face_select_owner_detail::activity();auto&c=native_simplex_face_refine_owner_detail::activity();
    if(native_simplex_face_select_owner_detail::reference()?(literals!=cases||p.actual_calls||p.canonical_calls):(literals||p.actual_calls!=384u||p.canonical_calls!=768u))throw std::runtime_error("Face-select parent source activity differs");
    if(native_simplex_face_refine_owner_detail::reference()?(c.actual_calls||route_reference::child_literal_entries!=children):(c.actual_calls!=children||route_reference::child_literal_entries))throw std::runtime_error("Face-select actual child route differs");
    if(!route_reference::actual_child_site_witnesses||!route_reference::canonical_child_site_witnesses||route_reference::actual_child_site_witnesses+route_reference::canonical_child_site_witnesses!=children)throw std::runtime_error("Face-select child dispatch-site witnesses missing");
    std::cerr<<"{\"source_cases\":"<<cases<<",\"source_variants\":6,\"parent_actual_native_calls\":"<<p.actual_calls<<",\"parent_canonical_native_calls\":"<<p.canonical_calls<<",\"parent_literal_body_entries\":"<<literals<<",\"genuine_child_entries\":"<<children<<",\"child_native_calls\":"<<c.actual_calls<<",\"child_literal_body_entries\":"<<route_reference::child_literal_entries<<",\"actual_child_site_witnesses\":"<<route_reference::actual_child_site_witnesses<<",\"canonical_child_site_witnesses\":"<<route_reference::canonical_child_site_witnesses<<"}\n";
}
void refusals(){
    unsigned cases{};
    for(unsigned kind=0;kind<24u;++kind){
        Memory m;SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};SourcePhysicsContactRecordObserver observer=[](const auto&){};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};services.instruction=[](std::uint32_t){};
        std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        if(kind==11u)cursor.literal_service_sites=false;physics_step_detail::FpScope environment(*abi.floating_point);ActualState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);if(kind!=11u&&!native_simplex_face_select_owner_detail::admitted(state))throw std::runtime_error("Face-select guard baseline refused");
        bool trace=true;auto external=cursor;std::array extra{sites[0],sites[1],sites[2],sites[2]};const auto host_mxcsr=_mm_getcsr();
        switch(kind){case 0:state.image=SourcePhysicsContactImage::MapPack11;break;case 1:state.image=SourcePhysicsContactImage(99);break;case 2:state.active_mmx.emplace();break;case 3:state.fp.count=6u;break;case 4:state.fp.top=8u;break;case 5:state.fp.status|=0x8080u;break;case 6:services.instruction={};break;case 7:services.instruction=[unused=17u](std::uint32_t){(void)unused;};break;case 8:cursor.position=nullptr;break;case 9:cursor.executor=std::thread::id{};break;case 10:cursor.literal_service_sites=false;break;case 11:cursor.literal_service_sites=true;break;case 12:state.instruction_cursor=&external;break;case 13:cursor.callback_type=nullptr;break;case 14:sites[2]^=1u;break;case 15:cursor.service_sites=std::span(sites).first(2);break;case 16:cursor.first_trace=&trace;break;case 17:cursor.second_trace=&trace;break;case 18:cursor.observation_only_stores=false;break;case 19:cursor.callback_type=&typeid(int);break;case 20:_mm_setcsr(host_mxcsr&~0x80u);break;case 21:services.instruction={};state.instruction_cursor=nullptr;break;case 22:cursor.service_sites=extra;break;case 23:state.selected_observation_pcs=false;break;}
        const auto gp=state.gp;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);const auto flags=state.flags.bits(),other=state.other_flags;const auto count=state.fp.count,top=state.fp.top;const auto status=state.fp.status;
        const auto accepted=native_simplex_face_select_owner_detail::bridge(state,[](auto&,std::uint32_t,std::uint32_t){});const auto unchanged=state.gp==gp&&std::memcmp(state.xmm,xmm.data(),128)==0&&state.flags.bits()==flags&&state.other_flags==other&&state.fp.count==count&&state.fp.top==top&&state.fp.status==status&&m.read_count==0u&&m.write_count==0u&&pc==0u;_mm_setcsr(host_mxcsr);
        if(accepted||!unchanged)throw std::runtime_error("Face-select live refusal failed "+std::to_string(kind));++cases;
    }
    std::cerr<<"{\"untouched_live_refusals\":"<<cases<<"}\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--refusals")refusals();else routing();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
