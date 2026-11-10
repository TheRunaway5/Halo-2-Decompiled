#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_mopp_ray_primitive_key_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_leaf_frame.hpp"
#include "physics/runtime/native_tu15_instanced_ray_child_frame.hpp"
#include "physics/runtime/native_tu15_instanced_ray_callback_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_controller.hpp"
#include "physics/runtime/native_tu15_mopp_ray_slab_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp"
#include "physics/runtime/native_tu15_affine_inverse.hpp"
#ifdef HALO2_CALLBACK_SOURCE_ROUTES
#include "physics/runtime/native_tu15_instanced_ray_callback.hpp"
#endif
#include "physics/runtime/native_tu15_camera_instance_query_frame.hpp"
#include "physics/runtime/native_tu15_camera_instance_admission.hpp"
#include "physics/runtime/native_tu15_camera_instance_bitmap.hpp"
#include "physics/runtime/native_tu15_segment_sphere.hpp"
#include "physics/runtime/native_tu15_camera_collision_publication.hpp"
#include "physics/runtime/native_tu15_bsp_segment_setup.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace retained {
#include "camera_query_children.inc"
}
struct InstrumentedState:WorldState {
    using WorldState::WorldState;unsigned callback_literals{};
    void instruction_literal(std::uint32_t pc){
        callback_literals+=pc>=0x24def0u&&pc<=0x24dfabu;WorldState::instruction_literal(pc);
    }
};
#ifdef HALO2_CALLBACK_SOURCE_ROUTES
namespace callback_copies {
using WorldState=InstrumentedState;
inline std::function<void(WorldState&,std::uint32_t)> current_dispatch;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){current_dispatch(s,target);}
static void run_16c000(WorldState&s){current_dispatch(s,0x16c000u);}
using retained::sign_extend;
#include "instanced_ray_callback_bodies.inc"
}
#endif
namespace {
struct Boundary {};
void check(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>data){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:data){const auto value=std::to_integer<unsigned>(b);out+=digits[value>>4];out+=digits[value&15];}return out;}
void clobber_fp(){
    asm volatile("fninit; fld1; fldz; fdivp; fld1":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);
}
J packet(const WorldState&s,std::uint32_t pc){
    const auto a=s.save();J live=J::array();
    for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};bool observer_preimages{},hostile_fp{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray caller bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray caller bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(int(read_count++)==read_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        operations.push_back(J{{"kind","read"},{"instruction",pc},{"address",address},{"bytes",out.size()}});
        auto i=at(address,out.size());std::copy_n(i->second.begin()+address-i->first,out.size(),out.begin());
        if(hostile_fp)clobber_fp();
    }
    // This fixture owns committed ordinary banks. The independent CPU oracle
    // records one sixteen-byte vector operand; device adapters retain their
    // separate four-scalar policy and need their own provider proof.
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        if(int(write_count++)==write_fault){if(hostile_fp)clobber_fp();throw Boundary{};}
        auto i=at(address,data.size());auto&bank=banks.at(i->first);
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",hex(std::span(bank).subspan(address-i->first,data.size()))},{"written",hex(data)}});
        operations.push_back(J{{"kind","write"},{"instruction",pc},{"address",address},{"bytes",data.size()}});
        std::copy(data.begin(),data.end(),bank.begin()+address-i->first);
        if(hostile_fp)clobber_fp();
    }
    J architectural_reads()const{
        J out=J::array();
        for(std::size_t n=0;n<operations.size();++n){
            auto event=operations[n];if(event.at("kind")!="read")continue;
            if(observer_preimages&&n+1<operations.size()){
                const auto&next=operations[n+1];
                if(next.at("kind")=="write"&&event.at("instruction")==next.at("instruction")&&
                   event.at("address")==next.at("address")&&event.at("bytes")==next.at("bytes"))continue;
            }
            event.erase("kind");out.push_back(event);
        }
        return out;
    }
};
}
int main(int argc,char**argv)try{
    check(argc==2||argc==3,"Supply physical primitive-key fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong primitive-key original");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{},private_packets{},queries{},hits{},duplicates{},any_hits{},replaced{},kept{},source_calls{},source_callbacks{},source_literals{};
    const bool forced=std::getenv("HALO2_INSTANCED_RAY_CALLBACK_REFERENCE_TEST");
#ifdef HALO2_CALLBACK_SOURCE_ROUTES
    const unsigned routes=5;
#else
    const unsigned routes=1;check(!forced,"Reference controls require real callback source copies");
#endif
    for(const auto&row:fixture.at("cases"))for(unsigned route=forced?1u:0u;route<routes;++route)for(bool hostile:{false,true})for(bool observe:{false,true}){
        if(hostile&&(!fixture.value("hostile_providers",false)||!row.value("native_inverse",false)||forced))continue;
        const auto phase=row.at("phase").get<std::string>();
        if((forced&&phase!="normal")||(route&&phase=="before_instruction"))continue;
        if(phase!="normal"&&observe!=(phase=="after_write"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.observer_preimages=observe;memory.hostile_fp=hostile;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        J observed=J::array();WorldState*current{};unsigned callback_events{};
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){callback_events+=pc>=0x24def0u&&pc<=0x24dfabu;memory.pc=pc;if(sites.contains(pc))observed.push_back(packet(*current,pc));};
        constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};bool trace{};
        services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observer;unsigned observed_writes{};
        if(observe)observer=[&](const auto&event){
            const auto&expected=row.at("writes").at(observed_writes);
            check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&
                  hex(event.before)==expected.at("before").template get<std::string>()&&
                  hex(event.written)==expected.at("written").template get<std::string>(),"Primitive-key observer differs");
            const auto ordinal=observed_writes++;if(hostile)clobber_fp();
            if(phase=="after_write"&&ordinal==row.at("native_fault_ordinal").get<unsigned>())throw Boundary{};
        };
        InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
        incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],v.data(),10);}state.load(incoming);current=&state;
        auto before=[&](auto&s,std::uint32_t pc){
            memory.pc=pc;if(pc==0x63000000u)throw Boundary{};
            if(sites.contains(pc))observed.push_back(packet(s,pc));if(hostile)clobber_fp();
            if(phase=="before_instruction"&&pc==row.at("expected_pc")&&
               memory.write_count==row.at("native_fault_ordinal").get<unsigned>())throw Boundary{};
        };
        bool interrupted{};
        try{
            unsigned row_queries{},row_hits{};
            std::function<void(InstrumentedState&,std::uint32_t,std::uint32_t)> dispatch;
            auto forwarded=[&](auto&s,auto target,auto site,bool){dispatch(s,target,site);};
            dispatch=[&](auto&s,std::uint32_t owner,std::uint32_t){
                if(owner==0x24dec0u)native_instanced_ray_child_detail::resolve_retained(s,before);
                else if(owner==0x24def0u||owner==0x24df20u){
#ifdef HALO2_CALLBACK_SOURCE_ROUTES
                    if(route){
                        const bool forward=owner==0x24def0u;
                        if(route==1){if(forward)callback_copies::primary_bound::run_24def0(s);else callback_copies::primary_bound::run_24df20(s);}
                        else if(route==2){if(forward)callback_copies::primary_prepared::run_24def0(s);else callback_copies::primary_prepared::run_24df20(s);}
                        else if(route==3){if(forward)callback_copies::ai_bound::run_24def0(s);else callback_copies::ai_bound::run_24df20(s);}
                        else {if(forward)callback_copies::ai_prepared::run_24def0(s);else callback_copies::ai_prepared::run_24df20(s);}
                    }else
#endif
                    if(owner==0x24def0u)native_instanced_ray_callback_detail::forward_retained(s,before,forwarded);
                    else native_instanced_ray_callback_detail::collect_retained(s,before,forwarded);
                }
                else if(owner==0x16c000u){++row_queries;native_camera_instance_query_detail::run_private(s,before,dispatch);row_hits+=(s.gp[0]&255u)!=0;}
                else if(owner==0x16b810u)native_camera_instance_admission_detail::run_private(s,before);
                else if(owner==0x186e30u)native_camera_instance_bitmap_detail::run_private(s,before);
                else if(owner==0x120fa0u)native_segment_sphere_detail::run_private(s,before);
                else if(owner==0x16b9a0u)native_camera_collision_publication_detail::run_private(s,before);
                else if(owner==0x144090u){
                    if(row.value("native_inverse",false))native_affine_inverse_detail::run_private(s,before);
                    else retained::run_144090(s);
                }
                else if(owner==0x1e68e0u){
                    auto missing=[&](auto&state,auto target,auto site,bool){throw std::runtime_error("MOPP query escaped native BSP owners "+J{{"case",row.at("name")},{"target",target},{"site",site},{"hostile",hostile},{"state",packet(state,memory.pc)}}.dump());};
                    check(native_bsp_segment_setup_detail::bridge(s,missing,before),"MOPP query native BSP refused actual input");
                }else throw std::runtime_error("Unprovided real MOPP leaf child "+std::to_string(owner));
            };
#ifdef HALO2_CALLBACK_SOURCE_ROUTES
            callback_copies::current_dispatch=[&](auto&s,auto owner){dispatch(s,owner,0);};
#endif
            if(row.value("complete_controller",false)){
                auto slab=[&](auto&s,auto&before,auto&scope,auto opcode,auto&program){return native_mopp_ray_caller_detail::slab(s,before,scope,opcode,program);};
                auto diagnostic=[&](auto&s,auto&before,auto&scope,auto&dispatch){native_mopp_ray_caller_detail::DiagnosticFrame frame{s,before,scope,dispatch};frame.invoke();};
                native_mopp_ray_caller_detail::traverse(state,before,dispatch,slab,diagnostic);
            }else{
                native_raw_x87_detail::Scope scope{state};native_mopp_ray_caller_detail::PrimitiveKeyFrame frame{state,before,scope};
                const auto entry=frame.begin();check(entry.valid_program_class,"Invalid primitive key opcode");
                frame.publish_program_class(native_mopp_aabb_abi_detail::source_class(entry.opcode));
                std::uint32_t key{};check(native_mopp_primitive_key_detail::decode(frame,entry.program,entry.opcode,key),"Wrong primitive-key program");
                native_mopp_ray_caller_detail::LeafFrame leaf{state,before,scope,dispatch};
                native_mopp_ray_leaf_detail::Progress progress;
                const auto result=native_mopp_ray_leaf_detail::visit(leaf,key,progress);leaf.finish(result);
            }
            queries+=row_queries;hits+=row_hits;duplicates+=!row_queries;before(state,0x63000000u);
        }catch(const Boundary&){interrupted=true;}
        catch(const std::exception&error){throw std::runtime_error(std::string(error.what())+" "+J{{"case",row.at("name")},{"phase",phase},{"hostile",hostile},{"observed",observe},{"state",packet(state,memory.pc)}}.dump());}
        check(interrupted,"Primitive key did not reach declared boundary");
        const auto name=row.at("name").get<std::string>();
        if(!route&&observed!=row.at("calls")){
            const auto&expected_calls=row.at("calls");std::size_t n{};
            while(n<std::min(observed.size(),expected_calls.size())&&observed[n]==expected_calls[n])++n;
            check(false,name+": primitive-key complete packets differ at "+std::to_string(n)+" actual="+
                (n<observed.size()?observed[n].dump():"missing")+" expected="+(n<expected_calls.size()?expected_calls[n].dump():"missing"));
        }
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        check(packet(state,memory.pc)==expected,name+": primitive-key boundary ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads"),name+": ordered reads differ actual="+memory.architectural_reads().dump()+" expected="+row.at("reads").dump());
        check(memory.writes==row.at("writes"),name+": ordered writes differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": primitive-key bank differs");
        if(forced)check(callback_events||state.callback_literals,name+": forced callback reference executed no retained statements");
        else check(!callback_events&&!state.callback_literals,name+": native callback executed retained statements");
        if(route)++source_calls;source_callbacks+=callback_events;source_literals+=state.callback_literals;
        for(const auto&write:memory.writes){const auto pc=write.at("instruction").get<std::uint32_t>();any_hits+=pc==0x24df6cu;replaced+=pc==0x24df8du;kept+=pc==0x24dfa0u;}
        ++comparisons;packets+=observed.size();if(!route)private_packets+=observed.size();
    }
    const auto report=J{{"passed",true},{"comparisons",comparisons},{"observed_pre_io_packets",packets},{"strict_private_pre_io_packets",private_packets},{"real_queries",queries},{"real_query_hits",hits},{"duplicate_instance_returns",duplicates},{"any_hit_stores",any_hits},{"replacement_stores",replaced},{"keep_stores",kept},{"actual_source_calls",source_calls},{"callback_retained_events",source_callbacks},{"callback_literal_statements",source_literals},{"forced_reference",forced},{"production_binding",false},{"scope","Whole native MOPP collector leaf with genuine provider/forwarder/collector/camera query and returned parent ABI"}};if(argc==3){std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';}std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
