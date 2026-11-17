#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_box_shape_support_frame.hpp"
#include <iostream>
#include <set>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace box_support_reference {
#include "box_shape_support_reference.inc"
}
namespace {
struct Result{J abi,packets=J::array();bool stopped{};unsigned callbacks{},observed{};};
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned observer_fault=0u,bool hostile=false,unsigned feedback=0u){
    Result out;WorldState*current{};SourcePhysicsContactRecordObserver observe=[&](const auto&event){
        if((feedback==1u&&event.instruction==0x306790u)||(feedback==2u&&event.instruction==0x3067bfu)){
            const auto address=abi.gp[4]+8u;const auto bank=memory.at(address,4);const std::uint32_t alternate=0x61004000u;std::memcpy(memory.banks.at(bank->first).data()+address-bank->first,&alternate,4);
        }
        if(hostile)poison();if(++out.observed==observer_fault)throw Boundary{};
    };SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++out.callbacks;if(box_support_reference::io_sites.contains(pc))out.packets.push_back(packet(*current,pc));};auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&memory.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);current=&state;const auto before=[&](auto&s,std::uint32_t pc){out.packets.push_back(packet(s,pc));if(hostile)poison();};
    try{if(native)native_box_shape_support_detail::run_private_owner(state,before);else box_support_reference::owner(state);}catch(const Boundary&){out.stopped=true;}out.abi=packet(state,memory.pc);return out;
}
SourcePhysicsWorldRuntimeAbi abi_from(const J&row){SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();return abi;}
Memory memory_from(const J&row){Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return memory;}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y){if(x.abi!=y.abi||x.packets!=y.packets||x.stopped!=y.stopped||a.banks!=b.banks||a.operations!=b.operations||a.writes!=b.writes){std::cerr<<"expected="<<x.abi.dump()<<" actual="<<y.abi.dump()<<"\n";throw std::runtime_error("Box support full retained protocol differs");}check(!y.callbacks,"Box support used literal callbacks");}
void cases(const J&rows){unsigned count{},prefixes{},hostile{},feedback_count{};for(const auto&row:rows){const auto abi=abi_from(row);const auto seed=memory_from(row);Memory a=seed,b=seed;const auto x=run(a,abi,false),y=run(b,abi,true);compare(a,b,x,y);check(!x.stopped,"Normal box support stopped");++count;
    if(row.at("id").get<unsigned>()<16u)for(unsigned category=0;category<3u;++category){const auto limit=category==0u?a.read_count:category==1u?a.write_count:x.observed;for(unsigned n=0;n<limit;++n){Memory c=seed,d=seed;if(category==0u)c.read_fault=d.read_fault=n;if(category==1u)c.write_fault=d.write_fault=n;const auto u=run(c,abi,false,category==2u?n+1u:0u),v=run(d,abi,true,category==2u?n+1u:0u);compare(c,d,u,v);check(u.stopped,"Box support prefix did not stop");++prefixes;}}
    if(row.at("id").get<unsigned>()<16u){Memory c=seed,d=seed;d.hostile=true;const auto u=run(c,abi,false),v=run(d,abi,true,0u,true);compare(c,d,u,v);++hostile;}
    if(row.at("id").get<unsigned>()<8u)for(unsigned feedback=1;feedback<=2u;++feedback){Memory c=seed,d=seed;const auto u=run(c,abi,false,0u,false,feedback),v=run(d,abi,true,0u,false,feedback);compare(c,d,u,v);++feedback_count;}
    }std::cout<<J{{"source_cases",count},{"source_read_write_observer_prefixes",prefixes},{"hostile_fp_cases",hostile},{"backed_caller_argument_feedback_cases",feedback_count},{"native_literal_callbacks",0}}.dump()<<"\n";
}
}
int main(int argc,char**){try{J rows;std::cin>>rows;if(argc==1){cases(rows);return 0;}J outputs=J::array();for(const auto&row:rows){auto memory=memory_from(row);const auto got=run(memory,abi_from(row),true);J banks=J::array();for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});outputs.push_back(J{{"abi",got.abi},{"packets",got.packets},{"stopped",got.stopped},{"banks",banks},{"writes",memory.writes}});}std::cout<<outputs.dump()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
