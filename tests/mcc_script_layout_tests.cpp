#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void word(std::span<std::byte> out,unsigned at,std::uint32_t value,unsigned bytes=4) {
    for(unsigned n=0;n<bytes;++n)out[at+n]=std::byte(value>>(n*8));
}
void row(std::span<std::byte> out,unsigned index,unsigned salt,unsigned opcode,unsigned type,
    unsigned flags,unsigned next,unsigned offset,unsigned value) {
    const auto at=index*20;word(out,at,salt,2);word(out,at+2,opcode,2);word(out,at+4,type,2);
    word(out,at+6,flags,2);word(out,at+8,next);word(out,at+12,offset);word(out,at+16,value);
}
std::span<const std::byte> bytes(std::string_view text){return std::as_bytes(std::span(text.data(),text.size()));}
template<class F>void rejected(F&& action) {
    bool failed=false;try{action();}catch(const FormatError&){failed=true;}
    require(failed,"Malformed/unowned script data admitted");
}
void synthetic() {
    std::vector<std::byte> nodes(4*20,std::byte{0x5a});
    row(nodes,0,0x9000,0x1c3,4,8,0xffffffff,0xdeadbeef,0x90010001);
    row(nodes,1,0x9001,2,2,9,0x90020002,0,0);
    row(nodes,2,0x9002,5,5,9,0xffffffff,0,1);
    // A free pool row retains arbitrary fields and must remain untouched.
    word(nodes,60,0,2);
    const std::string names("camera_control\0",15);const auto donor=nodes;
    auto converted=project_mcc_script_layout(nodes,bytes(names),{});
    require(converted.native_ready() && converted.converted_calls==1 && converted.rewritten_calls==1,
        "Compatible camera_control signature did not convert");
    require(Bytes(converted.nodes).u16(2)==0x1c1,"MCC camera_control still selects the wrong Xbox builtin");
    for(unsigned n=0;n<nodes.size();++n)if(n!=2 && n!=3)
        require(converted.nodes[n]==nodes[n],"Conversion changed non-opcode AST bytes");
    require(nodes==donor,"Conversion modified donor syntax storage");
    // Same-name command-script overloads have distinct signatures/opcodes.
    const auto one=mcc_script_builtin_signature(380),two=mcc_script_builtin_signature(381);
    require(one && two && one->name=="cs_fly_to" && two->name==one->name &&
        one->argument_types.size()==1 && two->argument_types.size()==2 &&
        one->xbox_opcode==378 && two->xbox_opcode==379,"Overloads collapsed by name");
    // The sole verified renamed builtin still retains its donor function name.
    const auto vibration=mcc_script_builtin_signature(0x295);
    require(vibration && vibration->declared_return_type==4 && vibration->xbox_opcode==0x28d &&
        std::ranges::equal(vibration->argument_types,std::array<std::uint16_t,2>{6,6}),
        "Authenticated vibration/rumble prototype did not project");
    require(mcc_script_projected_builtin_name(0x28d,"player_effect_set_max_vibration")==
        std::optional<std::string_view>("player_effect_set_max_rumble") &&
        !mcc_script_projected_builtin_name(0x295,"player_effect_set_max_vibration") &&
        !mcc_script_projected_builtin_name(0x28d,"unrelated"),
        "Semantic alias used name alone or changed unrelated builtins");
    // Static script ordinal0 must never undergo builtin opcode conversion.
    std::vector<std::byte> scripts(0x28);std::copy(bytes(names).begin(),bytes(names).end(),scripts.begin());
    word(nodes,2,0,2);word(nodes,6,10,2);
    converted=project_mcc_script_layout(nodes,bytes(names),scripts);
    require(converted.nodes==nodes && converted.static_calls==1 && !converted.builtin_calls,
        "Static script ordinal changed as though it were a builtin");
    // Context SHORT is a legitimate coercion of the MCC getter's declared
    // LONG. The same-name Xbox void(long) setter must never be selected.
    const std::string getter("game_tick_rate\0",15);
    row(nodes,0,0x9000,0x38f,7,8,0xffffffff,0xdeadbeef,0x90010001);
    word(nodes,28,0xffffffff);
    converted=project_mcc_script_layout(nodes,bytes(getter),{});
    require(converted.nodes==nodes && converted.requirements.size()==1 &&
        converted.requirements[0].need==MccScriptLayoutNeed::NativeClockGetter,
        "Clock getter silently changed to same-name Xbox setter");
    rejected([&]{converted.require_native_ready();});
    const auto signature=mcc_script_builtin_signature(0x38f);
    require(signature && signature->declared_return_type==8 && signature->argument_types.empty() &&
        !signature->xbox_opcode,"Getter signature confused with context return type");
    // Known opcode with a different function-name identity is explicit too.
    converted=project_mcc_script_layout(nodes,bytes(names),{});
    require(converted.requirements[0].need==MccScriptLayoutNeed::BuiltinIdentity && converted.nodes==nodes,
        "Changed function name admitted using opcode alone");
    word(nodes,16,0x12340001);rejected([&]{project_mcc_script_layout(nodes,bytes(getter),{});});
    word(nodes,16,0x90010001);word(nodes,28,0x90020002);word(nodes,48,0x90020002);
    rejected([&]{project_mcc_script_layout(nodes,bytes(getter),{});});

    nodes.assign(60,std::byte{});const std::string globals("ai_current_actor\0",17);
    row(nodes,0,0x9100,19,31,5,0xffffffff,0,0xffff8314);
    // An internal global with identical name/index bits remains authored.
    row(nodes,1,0x9101,19,19,5,0xffffffff,0,0x12340314);
    row(nodes,2,0x9102,19,19,1,0xffffffff,0,0xffff8314);
    const auto global_donor=nodes;
    converted=project_mcc_script_layout(nodes,bytes(globals),{});
    require(converted.nodes==nodes && converted.requirements.size()==1 &&
        converted.requirements[0].need==MccScriptLayoutNeed::ExternalGlobalsDisabled,
        "External-global conversion was implicit");
    converted=project_mcc_script_layout(nodes,bytes(globals),{},{true});
    require(converted.native_ready() && converted.converted_external_globals==1 &&
        Bytes(converted.nodes).u32(16)==0xffff831e,"External global index/signed upper bits differ");
    for(unsigned n=0;n<nodes.size();++n)if(n!=16 && n!=17)
        require(converted.nodes[n]==nodes[n],"External-global conversion touched other AST fields");
    require(nodes==global_donor,"External-global conversion changed borrowed cache data");
    word(nodes,2,31,2);converted=project_mcc_script_layout(nodes,bytes(globals),{},{true});
    require(converted.native_ready() && Bytes(converted.nodes).u16(2)==31,
        "Global requested object_list coercion was confused with its declaration type");
    const std::string wrong_global("ai_current_squad\0",17);
    converted=project_mcc_script_layout(nodes,bytes(wrong_global),{},{true});
    require(converted.requirements[0].need==MccScriptLayoutNeed::ExternalGlobalIdentity,
        "Incorrect global declaration identity admitted");
}
}
int main(int argc,char** argv)try {
    synthetic();
    nlohmann::json report={{"synthetic_pass",true}};
    if(argc>1) {
        Cache cache(argv[1]);const auto* scenario=cache.find_tag(cache.scenario_tag());
        const Bytes root(cache.read_tag_bytes(scenario->address,0x240));
        const auto read_bank=[&](unsigned count_offset,unsigned stride) {
            const auto count=root.u32(count_offset);
            return count?cache.read_tag_bytes(root.u32(count_offset+4),std::size_t(count)*stride):
                std::span<const std::byte>{};
        };
        const auto nodes=read_bank(0x238,20);
        const std::vector<std::byte> donor(nodes.begin(),nodes.end());
        const auto strings=read_bank(0x1b0,1);
        const auto scripts=read_bank(0x1b8,0x28);
        const auto converted=project_mcc_script_layout(nodes,strings,scripts,{true});
        for(std::size_t n=0;n<nodes.size()/20;++n)for(unsigned byte=0;byte<20;++byte) {
            const Bytes row(nodes.subspan(n*20,20));const auto flags=row.u16(6);
            const bool allowed=(row.u16(0) && flags!=0xbaba && !(flags&3) && (byte==2 || byte==3)) ||
                (row.u16(0) && flags!=0xbaba && (flags&5)==5 && (row.u32(16)&0x8000) && (byte==16 || byte==17));
            require(allowed || converted.nodes[n*20+byte]==nodes[n*20+byte],"Actual cache AST changed outside allowed fields");
        }
        require(std::ranges::equal(donor,read_bank(0x238,20)),"Actual donor cache changed");
        std::map<std::string,unsigned> needs;for(const auto& need:converted.requirements)++needs[need.name];
        report["map"]=cache.name();report["nodes"]=nodes.size()/20;
        report["builtin_calls"]=converted.builtin_calls;report["converted_calls"]=converted.converted_calls;
        report["rewritten_calls"]=converted.rewritten_calls;report["static_calls"]=converted.static_calls;
        report["external_globals"]=converted.external_globals;report["converted_external_globals"]=converted.converted_external_globals;
        report["native_ready"]=converted.native_ready();report["requirements"]=needs;
    }
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
