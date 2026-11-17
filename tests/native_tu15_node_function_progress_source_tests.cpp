#include "gameplay/animation/native_tu15_node_function_progress_source_frame.hpp"
#include "gameplay/animation/native_tu15_node_progress_query_source_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace genuine {
#include "node_progress_query_reference.inc"
#include "node_function_progress_reference.inc"
}
namespace {
struct Provider final:Memory {
    WorldState* state{};
    mutable J trace=J::array();
    int stop{-1};
    void event(std::string_view kind,std::uint32_t site,J value)const {
        trace.push_back(J{{"kind",kind},{"instruction",site},{"cursor",pc},
                         {"abi",packet(*state,site)},{"value",std::move(value)}});
        if(int(trace.size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override {
        event("read",pc,J::array({address,out.size()}));Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte>value)override {
        event("write",pc,J::array({address,hex(value)}));Memory::write(address,value);
    }
};
struct Result {
    J trace,abi;
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    bool stopped{};
    friend bool operator==(const Result&,const Result&)=default;
};
enum class Route { retained, native_parent, native_composition };
std::set<std::uint32_t> covered_parent,covered_children;
std::array<unsigned,2> genuine_calls{},genuine_returns{};

Result execute(const J&row,bool literal,bool traced,bool feedback,Route route,int stop=-1) {
    Provider memory;memory.stop=stop;
    for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned i=0;i<abi.x87_depth;++i){
        const auto value=bytes(row.at("live_x87")[i]);std::memcpy(&abi.x87[i],value.data(),10);
    }
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {
        memory.event("instruction",pc,J{});memory.pc=pc;
        if(route==Route::retained&&stop<0){
            if(pc>=0xd3950u&&pc<=0xd39c8u)covered_parent.insert(pc);
            else covered_children.insert(pc);
        }
    };
    if(literal){
        static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&memory.pc,&traced,nullptr,sites,std::this_thread::get_id(),
                                     &services.instruction.target_type(),true};
    }
    const SourcePhysicsContactRecordObserver observer=[&](const auto&event) {
        memory.event("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
        if(feedback&&event.instruction==0x1cf0e9u){
            const auto bank=memory.at(event.address,4);const std::uint32_t scratch=0x3f000000u;
            auto&data=memory.banks.at(bank->first);
            std::memcpy(data.data()+event.address-bank->first,&scratch,4);
        }
    };
    services.required_owner=[&](auto&bank,const auto&call,auto&child_abi,const auto&child_observer) {
        check(call.target==0x1cf0e0u&&!call.tail&&
              (call.instruction==0xd3998u||call.instruction==0xd39b3u),"Unexpected D3950 external call");
        const auto index=unsigned(call.instruction==0xd39b3u);
        const physics_step_detail::FpScope scope{*child_abi.floating_point};
        WorldState child{bank,child_observer,services,SourcePhysicsContactImage::TitleUpdate15};child.load(child_abi);
        struct Restore {Provider& memory;WorldState* prior;~Restore(){memory.state=prior;}} restore{memory,memory.state};
        memory.state=&child;
        try{
            memory.event("callee",call.instruction,J::array({call.target,call.tail}));
            if(route==Route::retained&&stop<0)++genuine_calls[index];
            if(literal){
                if(route==Route::native_composition)
                    native_node_progress_query_detail::run_source_frame<true>(child,genuine::literal_child);
                else genuine::literal_parent(child);
            }else{
                if(route==Route::native_composition)
                    native_node_progress_query_detail::run_source_frame<false>(child,genuine::raw_child);
                else genuine::raw_parent(child);
            }
            memory.event("returned",call.instruction,J::array({call.target,call.tail}));
            if(route==Route::retained&&stop<0)++genuine_returns[index];
        }catch(...){child_abi=child.save();throw;}
        child_abi=child.save();
    };
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
    const auto dispatch=[](auto& value,auto target,auto pc,bool tail){value.required(pc,target,tail);};
    bool stopped{};
    try{
        if(literal){
            if(route==Route::retained)genuine::function_progress_literal(state);
            else native_node_function_progress_detail::run_frame<true>(state,dispatch);
        }else{
            if(route==Route::retained)genuine::function_progress_raw(state);
            else native_node_function_progress_detail::run_frame<false>(state,dispatch);
        }
    }catch(const Boundary&){stopped=true;}
    return {std::move(memory.trace),packet(state,memory.pc),std::move(memory.banks),stopped};
}
void compare(const Result&expected,const Result&actual,const std::string&name) {
    if(expected==actual)return;
    std::size_t i{};
    while(i<actual.trace.size()&&i<expected.trace.size()&&actual.trace[i]==expected.trace[i])++i;
    const auto a=i<actual.trace.size()?actual.trace[i].dump():"end";
    const auto e=i<expected.trace.size()?expected.trace[i].dump():"end";
    throw std::runtime_error("D3950 retained comparison differs "+name+" event "+std::to_string(i)+": "+a+" vs "+e);
}
}

int main(int argc,char**argv)try {
    check(argc==2,"Supply D3950's original-backed fixture");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    std::size_t normal{},stops{},packets{};
    for(std::size_t n=0;n<rows.size();++n)for(bool literal:{false,true})for(unsigned mode=0;mode<3;++mode){
        if(!literal&&mode==1)continue;
        const bool quiet=literal&&mode==1,feedback=mode==2;
        const auto name=std::to_string(n)+"/"+std::to_string(literal)+"/"+std::to_string(mode);
        const auto expected=execute(rows[n],literal,!quiet,feedback,Route::retained);
        check(!expected.stopped,"Unexpected D3950 retained stop");
        for(auto route:{Route::native_parent,Route::native_composition})
            compare(expected,execute(rows[n],literal,!quiet,feedback,route),name);
        ++normal;packets+=expected.trace.size();
        if(n<12)for(std::size_t i=0;i<expected.trace.size();++i){
            const auto prefix=execute(rows[n],literal,!quiet,feedback,Route::retained,int(i));
            check(prefix.stopped,"Missing D3950 retained stop");
            for(auto route:{Route::native_parent,Route::native_composition})
                compare(prefix,execute(rows[n],literal,!quiet,feedback,route,int(i)),name+"/stop"+std::to_string(i));
            ++stops;packets+=prefix.trace.size();
        }
    }
    check(covered_parent.size()==37&&covered_children.contains(0x1cf0e0u)&&covered_children.contains(0x1e3150u)&&
          genuine_calls==genuine_returns&&genuine_calls[0]&&genuine_calls[1],"D3950 real child or parent coverage incomplete");
    std::cout<<J{{"passed",true},{"normal",normal},{"failure_prefixes",stops},{"packets_per_native_route",packets},
       {"parent_sites",covered_parent.size()},{"genuine_calls_by_original_pc",genuine_calls},
       {"genuine_returns_by_original_pc",genuine_returns},{"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
