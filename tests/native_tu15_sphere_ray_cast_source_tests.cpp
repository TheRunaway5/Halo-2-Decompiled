#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_sphere_ray_cast.hpp"
#include <fstream>
#include <iostream>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using namespace halo2::mopp_ray_frame_test;
struct LiteralState:physics_world_runtime_detail::WorldState {
    using physics_world_runtime_detail::WorldState::WorldState;
    unsigned literal_statements{};
    void instruction_literal(std::uint32_t pc){++literal_statements;physics_world_runtime_detail::WorldState::instruction_literal(pc);}
};
namespace source_copies {
using WorldState=LiteralState;
#include "sphere_source_bodies.inc"
}
struct CursorMemory:Memory {
    std::uint32_t*position{};
    void sync()const{const_cast<CursorMemory*>(this)->pc=*position;}
    void read(std::uint32_t address,std::span<std::byte>out)const override{sync();Memory::read(address,out);}
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{sync();Memory::read_vector(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>data)override{sync();Memory::write(address,data);}
};
struct Result {J abi,reads,writes;std::map<std::uint32_t,std::vector<std::byte>>banks;bool interrupted{};unsigned callbacks{},literal_statements{};};
Result run(const J&row,unsigned variant,bool expected,bool reference,bool bare=false){
    const auto phase=row.at("phase").get<std::string>();CursorMemory memory;
    for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    memory.observer_preimages=!bare&&(phase=="normal"||phase=="after_write");
    // The hardware-qualified native scope owns hostile provider isolation;
    // retained-reference arithmetic is compared in its own original host mode.
    memory.hostile=!reference;
    if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
    if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
    Result out;unsigned stores{};SourcePhysicsContactRecordObserver observer;
    if(memory.observer_preimages)observer=[&](const auto&){if(phase=="after_write"&&stores++==row.at("native_fault_ordinal"))throw Boundary{};};
    std::uint32_t position{};memory.position=&position;SourcePhysicsWorldRuntimeServices services;
    if(!bare)services.instruction=[&](std::uint32_t pc){position=pc;++out.callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(!bare)services.instruction_cursor={&position,nullptr,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    LiteralState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
    incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
    incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<incoming.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],raw.data(),10);}
    physics_step_detail::FpScope environment(*incoming.floating_point);state.load(incoming);
    try{
        if(expected&&!reference){auto before=[](auto&,auto){};native_sphere_ray_cast_detail::run_private(state,before);}
        else if(variant==0){if(expected)source_copies::bound::retained_2e8dd0(state);else source_copies::bound::run_2e8dd0(state);}
        else if(variant==1){if(expected)source_copies::prepared::retained_2e8dd0(state);else source_copies::prepared::run_2e8dd0(state);}
        else if(variant==2){if(expected)source_copies::fresh_bound::retained_2e8dd0(state);else source_copies::fresh_bound::run_2e8dd0(state);}
        else{if(expected)source_copies::fresh_prepared::retained_2e8dd0(state);else source_copies::fresh_prepared::run_2e8dd0(state);}
    }catch(const Boundary&){out.interrupted=true;}
    out.literal_statements=state.literal_statements;out.abi=packet(state,0);out.reads=memory.architectural_reads();out.writes=memory.writes;out.banks=std::move(memory.banks);return out;
}
int main(int argc,char**argv)try{
    check(argc==2||argc==3,"Supply original sphere source cases");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto*p=std::getenv("HALO2_REFERENCE_SPHERE_RAY_CAST");const bool reference=p&&*p;
    check(native_sphere_ray_cast_binding_detail::reference()==reference,"Sphere source reference control differs from requested mode");
    unsigned comparisons{},legacy_differences{},statements{},bare_calls{};
    for(const auto&row:fixture.at("cases"))for(unsigned variant=0;variant<4;++variant){
        const auto expected=run(row,variant,true,reference),actual=run(row,variant,false,reference);
        check(expected.abi==actual.abi&&expected.reads==actual.reads&&expected.writes==actual.writes&&expected.banks==actual.banks&&expected.interrupted==actual.interrupted,
              row.at("name").get<std::string>()+": sphere actual source route differs variant="+std::to_string(variant));
        const auto events=actual.callbacks+actual.literal_statements;
        check(reference?events>0:events==0,"Sphere source literal instruction routing changed");statements+=events;
        if(reference&&row.at("phase")=="normal"){const auto native=run(row,variant,true,false);legacy_differences+=native.abi!=actual.abi;}
        ++comparisons;
        if(row.at("phase")=="normal"){
            const auto empty_expected=run(row,variant,true,reference,true),empty_actual=run(row,variant,false,reference,true);
            check(empty_expected.abi==empty_actual.abi&&empty_expected.reads==empty_actual.reads&&empty_expected.writes==empty_actual.writes&&empty_expected.banks==empty_actual.banks&&!empty_actual.interrupted,
                  row.at("name").get<std::string>()+": sphere empty-service default differs");
            check(reference||(!empty_actual.callbacks&&!empty_actual.literal_statements),"Sphere empty-service native default executed literals");
            statements+=empty_actual.callbacks+empty_actual.literal_statements;++bare_calls;++comparisons;
        }
    }
    const auto native=native_sphere_ray_cast_binding_detail::calls();
    const auto literal=source_copies::bound::literal_entries+source_copies::prepared::literal_entries+source_copies::fresh_bound::literal_entries+source_copies::fresh_prepared::literal_entries;
    check(reference?(native==0&&literal==comparisons):(native==comparisons&&literal==0),"Sphere source default/reference entry accounting changed");
    const J result{{"passed",true},{"source_comparisons",comparisons},{"source_variants",4},{"empty_service_calls",bare_calls},{"native_default_calls",native},{"retained_body_entries",literal},{"retained_statement_events",statements},{"forced_reference",reference},{"known_retained_abi_differences",legacy_differences},{"production_binding",false}};
    if(argc==3){std::ofstream output(argv[2]);output<<result.dump(2)<<'\n';}std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
