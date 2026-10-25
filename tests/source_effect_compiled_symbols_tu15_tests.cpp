#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,12> authored{};
    void read(std::uint32_t,std::span<std::byte>)const override {throw std::runtime_error("No bank read required");}
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("No bank write permitted");}
};
void need(bool passed){if(!passed)throw std::runtime_error("Compiled effect symbol control failed");}
std::string names(std::uint32_t id) {
    struct Name {std::uint32_t original_index,mcc;const char *text;};
    static constexpr Name joined[]{
#include "../src/ui/mcc_frontend_string_ids.inc"
    };
    const auto at=std::find_if(std::begin(joined),std::end(joined),[&](const auto &row){return row.mcc==id;});
    if(at!=std::end(joined))return at->text;
    throw std::runtime_error("Unknown authored namespace query");
}
template<class F>void rejects(F &&fn){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}need(rejected);}
}
int main()try {
    Memory a,b;const auto before=a.authored;
    constexpr std::array<std::uint32_t,5> unknown{0u,0xffffffffu,0x0400055bu,0x0700055au,0x09000100u};
    need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400054f);
    need(source_projectile_explosion_location_symbol_tu15(a,0x101cad,0x0700054c)==0x0700054c);
    {
        SourceEffectCompiledSymbolsTu15 original(a,false,{});
        need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400054f);
    }
    for(const auto bad:{0x030000d9u,0x0700055au,0x0400055bu,0x07000558u,0x0800055cu,
        0x1a000729u,0x110005a7u,0x0600056cu}) {
        rejects([&]{SourceEffectCompiledSymbolsTu15 partial(a,true,[&](auto id){return id==bad?"foreign":names(id);});});
        need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400054f);
    }
    rejects([&]{SourceEffectCompiledSymbolsTu15 missing(a,true,{});});
    rejects([&]{SourceEffectCompiledSymbolsTu15 missing(a,true,[](auto)->std::string{throw std::runtime_error("Missing owned table");});});
    {
        SourceEffectCompiledSymbolsTu15 first(a,true,names);
        // Folded arithmetic operands are not StringIDs. Their target names
        // validate independently; only the exact instruction/operand pair
        // may acquire the adapted distance.
        for(const auto [pc,original,adapted]:std::array<std::array<std::uint32_t,3>,3>{{
            {0xbef88u,0x0200011eu,0x02000121u},
            {0xc91b9u,0x513u,0x51fu},
            {0x10be5cu,0x06000560u,0x0600056cu}}}) {
            need(source_object_function_compiled_symbol_tu15(a,pc,original)==adapted);
            need(source_object_function_compiled_symbol_tu15(b,pc,original)==original);
            need(source_object_function_compiled_symbol_tu15(a,pc+1,original)==original);
            need(source_object_function_compiled_symbol_tu15(a,pc,adapted)==adapted);
        }
        need(source_effect_compiled_symbol_tu15(a,0x030000d9)==0x030000d9);
        need(source_effect_compiled_symbol_tu15(a,0x0700054e)==0x0700055a);
        need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400055b);
        need(source_projectile_explosion_location_symbol_tu15(a,0x101cad,0x0700054c)==0x07000558);
        need(source_projectile_explosion_location_symbol_tu15(a,0x101cc5,0x08000550)==0x0800055c);
        need(source_projectile_explosion_location_symbol_tu15(b,0x101cad,0x0700054c)==0x0700054c);
        constexpr std::array sites{0x100ea4u,0x100eb0u,0x100eb7u,0x100f04u,0x100f0bu};
        constexpr std::array original{0x0f000563u,0x06000564u,0x0e000562u,0x11000566u,0x12000565u};
        constexpr std::array authored{0x0f00056fu,0x06000570u,0x0e00056eu,0x11000572u,0x12000571u};
        for(unsigned i=0;i<sites.size();++i) {
            need(source_projectile_function_symbol_tu15(a,sites[i],original[i])==authored[i]);
            need(source_projectile_function_symbol_tu15(b,sites[i],original[i])==original[i]);
            need(source_projectile_function_symbol_tu15(a,sites[i]+1u,original[i])==original[i]);
            need(source_projectile_function_symbol_tu15(a,sites[i],authored[i])==authored[i]);
            need(source_effect_compiled_symbol_tu15(a,original[i])==original[i]);
        }
        // Other stores and the original comparison-only namespace cannot
        // acquire either seed-name alias, even with an active Cache lease.
        need(source_effect_compiled_symbol_tu15(a,0x0700054c)==0x0700054c);
        need(source_projectile_explosion_location_symbol_tu15(a,0x101cad,0x08000550)==0x08000550);
        for(const auto pc:{0u,0x101cacu,0x101caeu,0x101cc4u,0x101cc6u})
            need(source_projectile_explosion_location_symbol_tu15(a,pc,0x0700054c)==0x0700054c);
        for(const auto value:{0x070000c0u,0x020000cau,0xffffffffu,0x07000558u})
            need(source_projectile_explosion_location_symbol_tu15(a,0x101cad,value)==value);
        need(source_effect_compiled_symbol_tu15(b,0x0400054f)==0x0400054f);
        for(auto value:unknown)need(source_effect_compiled_symbol_tu15(a,value)==value);
        // The authored MCC value at old root's numeric ID is never queried or
        // globally renamed. Literal adaptation cannot overwrite that symbol.
        rejects([&]{SourceEffectCompiledSymbolsTu15 duplicate(a,true,names);});
        need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400055b);
        std::vector<std::thread> readers;
        for(unsigned n=0;n<4;++n)readers.emplace_back([&]{for(unsigned i=0;i<1000;++i)
            need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400055b);});
        for(auto &reader:readers)reader.join();
        SourceEffectCompiledSymbolsTu15 second(b,true,names);
        need(source_effect_compiled_symbol_tu15(b,0x0700054e)==0x0700055a);
    }
    need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400054f);
    need(source_effect_compiled_symbol_tu15(b,0x0400054f)==0x0400054f);
    need(source_projectile_explosion_location_symbol_tu15(a,0x101cad,0x0700054c)==0x0700054c);
    {
        SourceEffectCompiledSymbolsTu15 reused(a,true,names);
        need(source_effect_compiled_symbol_tu15(a,0x0400054f)==0x0400055b);
    }
    need(a.authored==before);
    std::cout<<"PASS: exact any/replace/root joins, original/unknown/collision controls, bank isolation, failed and duplicate admission, concurrent reads and lifetime\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
