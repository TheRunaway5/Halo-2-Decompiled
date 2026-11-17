#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_line_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_append_frame.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_frame.hpp"
#include "physics/runtime/native_tu15_box_shape_support_frame.hpp"
#include <iostream>
#include <set>
using namespace halo2;using namespace halo2::mopp_ray_frame_test;
namespace line_reference {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t)>child;
#include "solver_hull_line_reference.inc"
}
namespace {
struct Result{J abi,packets=J::array(),children=J::array(),events=J::array();bool stopped{};unsigned callbacks{};};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&a,bool native,unsigned observer_fault=0u){
    Result out;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){m.pc=pc;++out.callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    unsigned observed{};SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(++observed==observer_fault)throw Boundary{};};
    physics_step_detail::FpScope environment(*a.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    const auto before=[&](WorldState&s,std::uint32_t pc){if(line_reference::io_sites.contains(pc))out.packets.push_back(packet(s,pc));};
    // Both oracles invoke actual qualified child algorithms. Entry ABI/return
    // words are measured from backing; no child outcome is supplied.
    const auto enter=[&](WorldState&s,std::uint32_t target,std::uint32_t pc,unsigned bytes){
        const auto bank=m.at(s.gp[4],12);std::array<std::uint32_t,3>words{};std::memcpy(words.data(),bank->second.data()+s.gp[4]-bank->first,12);
        check(words[0]==pc+bytes,"Hull-line actual child return word differs");
        out.children.push_back(J{{"target",target},{"site",pc},{"return_pc",pc+bytes},{"arguments",target==0x313290u?J::array({words[1]}):J::array({words[1],words[2]})},{"abi",packet(s,target)}});
    };
    const auto leaf=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){
        enter(s,target,pc,3u);
        if(target==0x1f6610u){native_polygon_support_detail::support_retained<true>(s,before);return;}
        if(target==0x2e8d20u){native_zero_shape_support_detail::run_private_owner(s,before);return;}
        if(target==0x306790u){native_box_shape_support_detail::run_private_owner(s,before);return;}
        throw std::runtime_error("Genuine unconverted line support owner");
    };
    const auto pair=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){enter(s,target,pc,5u);if(target==0x313390u)native_solver_hull_support_pair_detail::run_private_owner(s,leaf,before);else if(target==0x313290u)native_solver_hull_append_detail::run_private_owner(s,before);else throw std::runtime_error("Different actual hull-line child");};
    line_reference::child=pair;
    // Reference Before captures the same complete pre-I/O fields, while its
    // literal callback keeps actual pure-statement delivery independently.
    const auto original=services.instruction;
    if(!native)services.instruction=[&](std::uint32_t pc){original(pc);if(line_reference::io_sites.contains(pc))out.packets.push_back(packet(state,pc));};
    try{if(native)native_solver_hull_line_detail::run_private_owner(state,pair,before);else line_reference::owner(state);}catch(const Boundary&){out.stopped=true;}
    out.abi=packet(state,m.pc);line_reference::child={};return out;
}
SourcePhysicsWorldRuntimeAbi input(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}return a;
}
Memory memory(const J&row){Memory m;for(const auto&bank:row.at("banks"))m.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return m;}
}
void compare(const Memory&a,const Memory&b,const Result&want,const Result&got,const std::string&name){
    if(want.abi==got.abi&&want.packets==got.packets&&want.children==got.children&&want.events==got.events&&a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&want.stopped==got.stopped)return;
    std::cerr<<name<<" ABI="<<(want.abi==got.abi)<<" packets="<<(want.packets==got.packets)<<" children="<<(want.children==got.children)<<" stores="<<(a.writes==b.writes)<<"\n";
    for(const auto [expected,actual]:{std::pair{&want.packets,&got.packets},std::pair{&want.children,&got.children}}){
        for(std::size_t n=0;n<std::min(expected->size(),actual->size());++n)if((*expected)[n]!=(*actual)[n]){
            std::cerr<<"first difference at "<<n<<": "<<(*expected)[n]<<" vs "<<(*actual)[n]<<"\n";break;
        }
    }
    throw std::runtime_error("Private hull line phased reference differs");
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;J output=J::array();unsigned cases{};
    if(argc>1&&std::string(argv[1])=="--prefixes"){
        unsigned reads{},writes{},observers{};
        for(const auto&row:rows){if(row.at("id").get<unsigned>()>=12u)continue;const auto seed=memory(row);const auto abi=input(row);auto baseline=seed;const auto complete=run(baseline,abi,false);check(!complete.stopped,"Hull-line prefix seed did not return");
            for(bool writing:{false,true}){unsigned index{};for(const auto&op:baseline.operations){if((op.at("kind")=="write")!=writing)continue;const auto n=index++;const auto pc=op.at("instruction").get<std::uint32_t>();if(!line_reference::io_sites.contains(pc))continue;auto a=seed,b=seed;if(writing)a.write_fault=b.write_fault=n;else a.read_fault=b.read_fault=n;const auto want=run(a,abi,false),got=run(b,abi,true);compare(a,b,want,got,"atomic-provider parent prefix");check(got.stopped,"Hull-line provider prefix did not stop");if(writing)++writes;else ++reads;}}
            for(unsigned n=0;n<complete.events.size();++n){const auto pc=complete.events[n].at("instruction").get<std::uint32_t>();if(!line_reference::io_sites.contains(pc))continue;auto a=seed,b=seed;const auto want=run(a,abi,false,n+1u),got=run(b,abi,true,n+1u);compare(a,b,want,got,"completed-observer parent prefix");check(got.stopped,"Hull-line completed observer did not stop");++observers;}
        }
        std::cout<<J{{"parent_read_prefixes",reads},{"parent_write_prefixes",writes},{"parent_completed_observer_prefixes",observers},{"physical_page_fault_atomicity",false},{"child_fault_domains_qualified",false}}.dump()<<'\n';return 0;
    }
    for(const auto&row:rows){auto a=memory(row);const auto abi=input(row);
        if(argc>1){const auto got=run(a,abi,true);J banks=J::array();for(const auto&[address,data]:a.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});output.push_back(J{{"abi",got.abi},{"packets",got.packets},{"children",got.children},{"stopped",got.stopped},{"banks",banks},{"writes",a.writes}});continue;}
        auto b=a;const auto want=run(a,abi,false),got=run(b,abi,true);
        compare(a,b,want,got,row.at("name"));
        check(got.callbacks==0u,"Private hull-line native path executed a literal callback");++cases;
    }
    if(argc>1)std::cout<<output.dump()<<'\n';else std::cout<<J{{"passed",true},{"canonical_phased_cases",cases},{"supplied_child_results",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
