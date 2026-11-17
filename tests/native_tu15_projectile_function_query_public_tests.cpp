#include "gameplay/objects/native_tu15_projectile_function_query_source_owner.hpp"
#include "gameplay/objects/native_tu15_object_definition_mode_query_source_owner.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace {
struct Provider final:mopp_ray_frame_test::Memory {
    std::uint32_t position{};int stop{-1};mutable J trace=J::array();
    void event(std::string_view name,std::uint32_t site,J values)const{
        trace.push_back(J::array({name,site,position,std::move(values)}));
        if(int(trace.size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        event("read",position,J::array({address,out.size()}));mopp_ray_frame_test::Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        event("write",position,J::array({address,hex(data)}));mopp_ray_frame_test::Memory::write(address,data);
    }
};
J abi_packet(const SourcePhysicsContactRuntimeAbi&abi){
    J stack=J::array();
    for(unsigned n=0;n<abi.x87_depth;++n)stack.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
    const auto&fp=*abi.floating_point;
    return J{{"gp",abi.gp},{"xmm",abi.xmm},{"flags",abi.eflags},{"fs",abi.fs_base},
        {"fp",J::array({fp.x87_control_word,abi.x87_status,fp.mxcsr})},{"x87",stack}};
}
std::array<unsigned,3> host_fp(){
    std::uint16_t control{},status{};asm volatile("fnstcw %0;fnstsw %1":"=m"(control),"=m"(status));
    return {control,status,_mm_getcsr()};
}
struct Case {
    SourcePhysicsContactRuntimeAbi abi{};
    std::uint32_t entry{};
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    explicit Case(const J&row){
        entry=row.at("entry");
        check((entry==0x100e70u||entry==0x165590u)&&source_combat_action_tu15_owns_entry(entry),
              "Missing exact public CombatAction query owner");
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");abi.fs_base=0x12345678u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(const auto&bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));
    }
};
std::size_t run(const Case&seed,const J&names,const J&keys,bool bound,bool traced,int stop,std::ostream&out){
    auto abi=seed.abi;Provider memory;memory.banks=seed.banks;memory.stop=stop;
    const SourceEffectCompiledSymbolsTu15 lease{memory,bound,[&](std::uint32_t id){return names.at(std::to_string(id)).get<std::string>();}};
    if(bound&&seed.entry==0x100e70u){
        const auto address=abi.gp[4]+8u;const auto bank=memory.at(address,4);
        auto&data=memory.banks.at(bank->first);std::uint32_t key{};std::memcpy(&key,data.data()+address-bank->first,4);
        key=keys.value(std::to_string(key),key);std::memcpy(data.data()+address-bank->first,&key,4);
    }
    SourcePhysicsContactRuntimeServices services;
    services.instruction=[&](std::uint32_t site){memory.event("instruction",site,J{});memory.position=site;};
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    services.instruction_cursor={&memory.position,&traced,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true};
    const SourcePhysicsContactRecordObserver observer=[&](const auto&event){
        memory.event("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
    };
    bool stopped{};const auto before=host_fp();
    try{source_run_combat_action_tu15(memory,seed.entry,abi,{0x37f,0x1f80},services,observer);}
    catch(const Boundary&){stopped=true;}
    check(stopped==(stop>=0),"Actual CombatAction public prefix completion differs");
    check(host_fp()==before,"CombatAction public owner failed to restore caller FP");
    out<<J{{"stopped",stopped},{"abi",abi_packet(abi)},{"cursor",memory.position},{"trace",memory.trace}}.dump()<<'\n';
    for(const auto&[address,bank]:memory.banks){out<<address<<' '<<bank.size()<<'\n';out.write(reinterpret_cast<const char*>(bank.data()),bank.size());}
    return memory.trace.size();
}
} // namespace
int main(int argc,char**argv)try{
    check(argc==3,"Supply authentic CombatAction query inputs and owned public stream path");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    std::ofstream stream(argv[2],std::ios::binary);check(bool(stream),"Open owned public comparison stream");
    unsigned normals{},prefixes{},events{};
    const bool definition_mode=fixture.value("definition_mode",false);
    const auto&rows=fixture.at("rows");
    for(std::size_t n=0;n<rows.size();++n){
        const Case seed{rows[n]};
        check((seed.entry==0x165590u)==definition_mode,"Mixed public CombatAction query fixtures");
        for(bool bound:{false,true})for(bool traced:{false,true}){
            const auto&names=fixture.at("names");const auto&keys=fixture.at("keys");
            const auto count=run(seed,names,keys,bound,traced,-1,stream);++normals;events+=count;
            if(n<8)for(std::size_t ordinal=0;ordinal<count;++ordinal){events+=run(seed,names,keys,bound,traced,int(ordinal),stream);++prefixes;}
        }
    }
    stream.close();check(bool(stream),"Write complete public comparison stream");
    const auto projectile=native_projectile_function_query_owner_detail::activity();
    const auto definition=native_object_definition_mode_query_owner_detail::activity();
    std::cout<<J{{"normal",normals},{"prefixes",prefixes},{"events",events},
        {"native_entries",definition_mode?definition:projectile},
        {"other_native_entries",definition_mode?projectile:definition}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
