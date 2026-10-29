#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
namespace {
using namespace halo2;
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void word(std::span<std::byte> bytes,unsigned at,std::uint32_t value,unsigned width=4){for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8u));}
std::span<const std::byte> text(std::string_view value){return std::as_bytes(std::span(value.data(),value.size()));}
std::vector<std::byte> nodes(){
    std::vector<std::byte> out(60,std::byte{});
    for(unsigned i=0;i<3;++i){word(out,i*20,0x9000u+i,2);word(out,i*20+8,0xffffffffu);}
    word(out,2,329,2);word(out,4,6,2);word(out,6,8,2);word(out,12,0xdeadbeefu);word(out,16,0x90010001u);
    word(out,22,2,2);word(out,24,2,2);word(out,26,9,2);word(out,28,0x90020002u);
    word(out,42,19,2);word(out,44,19,2);word(out,46,9,2);word(out,56,0x40000000u);return out;
}
}
int main()try{
    const auto signature=mcc_script_builtin_signature(329);
    check(signature&&signature->name=="ai_living_fraction"&&signature->declared_return_type==6&&signature->xbox_opcode==327&&signature->xbox_flags==0&&std::ranges::equal(signature->argument_types,std::array<std::uint16_t,1>{19}),"Authenticated MCC329 REAL(AI) signature missing");
    const auto living=mcc_script_builtin_signature(328),strength=mcc_script_builtin_signature(330);
    check(living&&living->xbox_opcode==326&&living->declared_return_type==7&&strength&&strength->xbox_opcode==328&&strength->declared_return_type==6,"Fraction admission changed adjacent signatures");
    const std::string names("ai_living_fraction\0",19);const auto original=nodes();const auto projected=project_mcc_script_layout(original,text(names),{});
    check(projected.native_ready()&&projected.builtin_calls==1&&projected.converted_calls==1&&projected.rewritten_calls==1&&Bytes(projected.nodes).u16(2)==327,"Fraction authored call did not project to original evaluator identity");
    for(unsigned i=0;i<original.size();++i)if(i!=2&&i!=3)check(projected.nodes[i]==original[i],"Fraction projection modified authored context/links/literal/name nodes");
    check(original==nodes(),"Fraction projection mutated donor syntax");
    auto missing=original;word(missing,28,0xffffffffu);const auto bad_arity=project_mcc_script_layout(missing,text(names),{});
    check(!bad_arity.native_ready()&&bad_arity.requirements.size()==1&&bad_arity.requirements[0].need==MccScriptLayoutNeed::ArgumentCount&&bad_arity.nodes==missing,"Fraction admission fabricated missing AI argument");
    const std::string wrong("ai_strength\0",12);const auto bad_name=project_mcc_script_layout(original,text(wrong),{});
    check(!bad_name.native_ready()&&bad_name.requirements.size()==1&&bad_name.requirements[0].need==MccScriptLayoutNeed::BuiltinIdentity&&bad_name.nodes==original,"Fraction mapping accepted opcode without authored name identity");
    auto stale=original;word(stale,16,0x90020001u);bool rejected{};try{(void)project_mcc_script_layout(stale,text(names),{});}catch(const FormatError&){rejected=true;}
    check(rejected,"Fraction mapping accepted stale salted function link");
    std::cout<<"MCC329->TU1.5 327 REAL(AI): exact opcode-only projection and malformed name/arity/salt controls passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
