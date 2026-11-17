#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#ifdef HALO2_SUPPORT_PAIR_ZERO_CHILD
#include "physics/runtime/native_tu15_zero_shape_support_frame.hpp"
#endif
#ifdef HALO2_SUPPORT_PAIR_BOX_CHILD
#include "physics/runtime/native_tu15_box_shape_support_frame.hpp"
#endif
#include <iostream>
#include <set>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace support_pair_reference {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)>child;
#include "solver_hull_support_pair_reference.inc"
}
namespace {
struct Result {J abi,packets=J::array(),children=J::array();unsigned callbacks{},observed{};bool stopped{};};
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned observer_fault=0u){
    Result out;WorldState*current{};SourcePhysicsContactRecordObserver observe=[&](const auto&){if(++out.observed==observer_fault)throw Boundary{};};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++out.callbacks;if(support_pair_reference::io_sites.contains(pc))out.packets.push_back(packet(*current,pc));};
    auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&memory.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);current=&state;
    const auto before=[&](auto&s,std::uint32_t pc){out.packets.push_back(packet(s,pc));};
    const auto child=[&](auto&s,std::uint32_t target,std::uint32_t pc){
        bool known=target==0x1f6610u;
#ifdef HALO2_SUPPORT_PAIR_ZERO_CHILD
        known=known||target==0x2e8d20u;
#endif
#ifdef HALO2_SUPPORT_PAIR_BOX_CHILD
        known=known||target==0x306790u;
#endif
        check(known&&(pc==0x3133abu||pc==0x313425u),"Unexpected genuine support owner");
        const auto raw_word=[&](std::uint32_t address){const auto bank=memory.at(address,4);std::uint32_t value{};std::memcpy(&value,bank->second.data()+address-bank->first,4);return value;};
        const auto return_pc=raw_word(s.gp[4]);check(return_pc==pc+3u,"Genuine support return word changed");
        out.children.push_back(J{{"site",pc},{"return_pc",return_pc},{"arguments",J::array({raw_word(s.gp[4]+4u),raw_word(s.gp[4]+8u)})},{"abi",packet(s,target)}});
#ifdef HALO2_SUPPORT_PAIR_ZERO_CHILD
        if(target==0x2e8d20u){native_zero_shape_support_detail::run_private_owner(s);return;}
#endif
#ifdef HALO2_SUPPORT_PAIR_BOX_CHILD
        if(target==0x306790u){native_box_shape_support_detail::run_private_owner(s);return;}
#endif
        native_polygon_support_detail::Noop noop;native_polygon_support_detail::support_retained<true>(s,noop);
    };
    support_pair_reference::child=child;
    try{if(native)native_solver_hull_support_pair_detail::run_private_owner(state,child,before);else support_pair_reference::owner(state);}
    catch(const Boundary&){out.stopped=true;}
    support_pair_reference::child={};out.abi=packet(state,memory.pc);return out;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&label){
    if(x.abi!=y.abi||x.packets!=y.packets||x.children!=y.children||x.stopped!=y.stopped||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes){
        std::cerr<<"abi="<<(x.abi==y.abi)<<" packets="<<x.packets.size()<<"/"<<y.packets.size()<<" children="<<(x.children==y.children)<<" banks="<<(a.banks==b.banks)<<" operations="<<a.operations.size()<<"/"<<b.operations.size()<<" writes="<<a.writes.size()<<"/"<<b.writes.size()<<"\n";
        std::cerr<<label<<" expected="<<x.abi.dump()<<" actual="<<y.abi.dump()<<"\n";
        for(unsigned n=0;n<std::min(x.packets.size(),y.packets.size());++n)if(x.packets[n]!=y.packets[n]){std::cerr<<"packet "<<n<<" expected="<<x.packets[n].dump()<<" actual="<<y.packets[n].dump()<<"\n";break;}
        throw std::runtime_error("Support-pair complete source/child protocol differs "+label);
    }
    check(!y.callbacks,"Native support-pair used parent literal callbacks");
}
void cases(){J rows;std::cin>>rows;unsigned cases{},children{},prefixes{};
    for(const auto&row:rows){Memory seed;for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        Memory a=seed,b=seed;const auto x=run(a,abi,false),y=run(b,abi,true);compare(a,b,x,y,"normal-"+std::to_string(cases));check(!x.stopped,"Support-pair normal stopped");children+=y.children.size();++cases;
        if(row.at("id").get<unsigned>()<2u)for(unsigned category=0;category<3u;++category){const auto limit=category==0u?a.read_count:category==1u?a.write_count:x.observed;for(unsigned n=0;n<limit;++n){Memory c=seed,d=seed;if(category==0u)c.read_fault=d.read_fault=n;if(category==1u)c.write_fault=d.write_fault=n;const auto u=run(c,abi,false,category==2u?n+1u:0u),v=run(d,abi,true,category==2u?n+1u:0u);compare(c,d,u,v,"prefix-"+std::to_string(category)+"-"+std::to_string(n));check(u.stopped,"Support-pair declared prefix did not stop");++prefixes;}}
    }
#ifdef HALO2_SUPPORT_PAIR_ZERO_CHILD
    const char*child_label="genuine_support_child_entries";
#else
    const char*child_label="genuine_polygon_child_entries";
#endif
    std::cout<<J{{"source_cases",cases},{child_label,children},{"observed_read_write_prefixes",prefixes},{"native_parent_callbacks",0}}.dump()<<"\n";
}
}
int main(){try{cases();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
