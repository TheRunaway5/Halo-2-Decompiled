#include "halo2/cache.hpp"
#include "halo2/scripts.hpp"
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;
void check(bool yes,const char* message){if(!yes)throw FormatError(message);}
}
int main(int argc,char** argv)try {
    if(argc!=3 || !*argv[1] || !*argv[2] || !std::filesystem::exists(argv[1]) ||
        !std::filesystem::exists(argv[2]))return 77;
    Cache mcc(argv[1]),xbox(argv[2]);
    check(mcc.is_mcc() && !xbox.is_mcc(),"Expected actual MCC and original Xbox Cairo caches");
    const auto* scenario=mcc.find_tag(mcc.scenario_tag());
    check(scenario && scenario->name=="scenarios\\solo\\01b_spacestation\\01b_spacestation",
        "Expected authenticated Cairo scenario");
    const Bytes native_scenario(mcc.read_tag_bytes(scenario->address,0x240));
    check(native_scenario.u32(0x1c0)==51,"Native scenario still declares the added MCC global");
    const auto converted=mcc.load_content(0),original=xbox.load_content(0);
    check(converted.scripts.globals.size()==51 && original.scripts.globals.size()==51,
        "Ordinary content parser global count differs from original Xbox");
    ScriptVm converted_vm(converted.scripts,{}),original_vm(original.scripts,{});
    check(converted_vm.globals().size()==51 && original_vm.globals().size()==51,
        "Ordinary ScriptVm retained the added MCC global");
    for(unsigned i=0;i<51;++i) {
        check(converted.scripts.globals[i].name==original.scripts.globals[i].name &&
            converted.scripts.globals[i].type==original.scripts.globals[i].type,
            "Ordinary content global namespace differs from original Xbox");
        const auto& m=converted_vm.globals()[i];const auto& x=original_vm.globals()[i];
        // String values carry donor-address bits until the genuine native
        // 2AA6D0 fixup. ScriptVm owns their semantic text separately.
        if(!(m.type==x.type && (m.type==9 || m.number()==x.number()) && m.text==x.text))
            throw FormatError("Ordinary ScriptVm global differs at "+std::to_string(i)+" "+
                converted.scripts.globals[i].name+": type "+std::to_string(m.type)+"/"+std::to_string(x.type)+
                ", bits "+std::to_string(m.bits)+"/"+std::to_string(x.bits)+", text "+m.text+"/"+x.text);
    }
    check(converted.scripts.globals[18].name=="seconds" &&
        converted_vm.globals()[18].number()==30,"Original mutable seconds initializer differs");
    std::cout<<nlohmann::json{{"actual_Cache_native_global_count",51},
        {"ordinary_content_global_count",converted.scripts.globals.size()},
        {"ordinary_ScriptVm_global_count",converted_vm.globals().size()},
        {"original_global_names_types_values_equal",51},
        {"MCC_clock_provider_needed",false},{"full_gameplay_execution_claimed",false}}.dump(2)<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
