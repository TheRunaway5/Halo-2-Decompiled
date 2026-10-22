#include "halo2/source_vehicle_compiled_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include "gameplay/vehicles/native_vehicle_variant_symbols_tu15.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::uint32_t,4> authored{0x05000694u,0x050006a0u,0x0b00078bu,0x0b0007a0u};
    void read(std::uint32_t,std::span<std::byte>)const override {throw std::runtime_error("No authored bank read required");}
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("No authored bank write allowed");}
};
void need(bool value){if(!value)throw std::runtime_error("Vehicle compiled symbol control failed");}
std::string names(std::uint32_t id) {
    if(id==0x050006a0u)return "gauss";
    if(id==0x0b0007a0u)return "multiplayer";
    throw std::runtime_error("Only actual MCC named operands may be resolved");
}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected);}
}
int main()try {
    Memory a,b;const auto before=a.authored;
    need(source_vehicle_compiled_symbol_tu15(a,0x15cbc2u,0x05000694u)==0x05000694u);
    need(detail::compiled_vehicle_variant_tu15(false,names,0x0b00078bu)==0x0b00078bu);
    {
        SourceVehicleCompiledSymbolsTu15 original(a,false,{});
        need(source_vehicle_compiled_symbol_tu15(a,0x15ccb9u,0x0b00078bu)==0x0b00078bu);
    }
    for(const auto invalid:{0x050006a0u,0x0b0007a0u}) {
        rejects([&]{SourceVehicleCompiledSymbolsTu15 wrong(a,true,[&](auto id){return id==invalid?"foreign":names(id);});});
        need(source_vehicle_compiled_symbol_tu15(a,0x15cbc2u,0x05000694u)==0x05000694u);
    }
    rejects([&]{SourceVehicleCompiledSymbolsTu15 missing(a,true,{});});
    {
        SourceVehicleCompiledSymbolsTu15 lease(a,true,names);
        need(source_vehicle_compiled_symbol_tu15(a,0x15cbc2u,0x05000694u)==0x050006a0u);
        need(source_vehicle_compiled_symbol_tu15(a,0x15ccb9u,0x0b00078bu)==0x0b0007a0u);
        need(source_vehicle_compiled_symbol_tu15(a,0x15ccceu,0x0b00078bu)==0x0b0007a0u);
        need(source_vehicle_compiled_symbol_tu15(b,0x15cbc2u,0x05000694u)==0x05000694u);
        rejects([&]{(void)source_vehicle_compiled_symbol_tu15(a,0x15cbc2u,0x050006a0u);});
        rejects([&]{(void)source_vehicle_compiled_symbol_tu15(a,0x15cbc3u,0x05000694u);});
        rejects([&]{SourceVehicleCompiledSymbolsTu15 duplicate(a,true,names);});
        need(detail::compiled_vehicle_variant_tu15(true,names,0x05000694u)==0x050006a0u);
        need(detail::compiled_vehicle_variant_tu15(true,names,0x0b00078bu)==0x0b0007a0u);
        // Already authored MCC values are not converted or looked up as Xbox
        // literals. Their same-number collision remains owned by the Cache.
        need(detail::compiled_vehicle_variant_tu15(true,names,0x050006a0u)==0x050006a0u);
        need(a.authored==before);
    }
    need(source_vehicle_compiled_symbol_tu15(a,0x15cbc2u,0x05000694u)==0x05000694u);
    need(a.authored==before);
    std::cout<<"PASS: authenticated gauss/multiplayer joins, exact literal sites, failed/duplicate admission, bank isolation, lifetime, unchanged authored IDs and presentation agreement\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
