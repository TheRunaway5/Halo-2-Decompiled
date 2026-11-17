#include "gameplay/objects/native_tu15_flight_property_query_source_owner.hpp"
#include "gameplay/objects/native_tu15_death_cycle_flight_query_source_owner.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
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
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    explicit Case(const J&row){
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");abi.fs_base=0x12345678u;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(const auto&bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));
    }
};
std::size_t run(const Case&seed,const J&names,const J&keys,bool death,bool bound,bool traced,int stop,
                std::ostream&out,std::ostream&host,std::size_t row){
    auto abi=seed.abi;Provider memory;memory.banks=seed.banks;memory.stop=stop;
    const SourceEffectCompiledSymbolsTu15 lease{memory,bound,[&](std::uint32_t id){return names.at(std::to_string(id)).get<std::string>();}};
    if(bound){
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
    try{
        if(death)source_run_death_cycle_runtime_tu15(memory,0xe0900u,abi,*seed.abi.floating_point,services,observer);
        else source_run_physics_live_support_tu15(memory,0xe0900u,abi,*seed.abi.floating_point,services,observer);
    }
    catch(const Boundary&){stopped=true;}
    check(stopped==(stop>=0),"Actual E0900 public prefix completion differs");
    const auto after=host_fp();
    // Both retained owners leave the same x87 C1 changes after the existing
    // FpScope. Compare the full host state across routes and permit no other
    // per-call change to caller control, status or MXCSR.
    check(before[0]==after[0]&&before[2]==after[2]&&((before[1]^after[1])&~0x200u)==0,
          "E0900 public owner changed caller FP outside x87 C1: "+J(before).dump()+" vs "+J(after).dump());
    host<<J{{"row",row},{"bound",bound},{"traced",traced},{"stop",stop},{"cursor",memory.position},
        {"before",before},{"after",after}}.dump()<<'\n';
    out<<J{{"stopped",stopped},{"abi",abi_packet(abi)},{"cursor",memory.position},{"trace",memory.trace}}.dump()<<'\n';
    for(const auto&[address,bank]:memory.banks){out<<address<<' '<<bank.size()<<'\n';out.write(reinterpret_cast<const char*>(bank.data()),bank.size());}
    return memory.trace.size();
}
} // namespace
int main(int argc,char**argv)try{
    check(argc==5,"Supply authentic flight inputs, owned guest stream, source owner and owned host stream");
    const std::string_view owner=argv[3];check(owner=="primary"||owner=="death-cycle","Unknown E0900 owner");
    const bool death=owner=="death-cycle";
    check(death?source_death_cycle_runtime_tu15_owns_entry(0xe0900u):source_physics_live_support_tu15_owns_entry(0xe0900u),
          "Missing actual E0900 public owner");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    std::ofstream stream(argv[2],std::ios::binary);check(bool(stream),"Open owned public comparison stream");
    std::ofstream host(argv[4]);check(bool(host),"Open owned host floating-point stream");
    unsigned normals{},prefixes{},events{};
    const auto&rows=fixture.at("rows");
    for(std::size_t n=0;n<rows.size();++n){
        const Case seed{rows[n]};
        for(bool bound:{false,true})for(bool traced:{false,true}){
            const auto&names=fixture.at("names");const auto&keys=fixture.at("keys");
            const auto count=run(seed,names,keys,death,bound,traced,-1,stream,host,n);++normals;events+=count;
            if(n<8)for(std::size_t ordinal=0;ordinal<count;++ordinal){events+=run(seed,names,keys,death,bound,traced,int(ordinal),stream,host,n);++prefixes;}
        }
    }
    stream.close();check(bool(stream),"Write complete public comparison stream");
    host.close();check(bool(host),"Write complete host floating-point stream");
    const auto primary=native_flight_property_query_owner_detail::activity();
    const auto death_cycle=native_death_cycle_flight_query_owner_detail::activity();
    std::cout<<J{{"normal",normals},{"prefixes",prefixes},{"events",events},
        {"native_entries",death?death_cycle:primary},{"other_native_entries",death?primary:death_cycle}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
