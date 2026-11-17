#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/native_campaign_script_world_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
void source_object_function_set_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,
    std::uint32_t name,float value,float frames) {
    auto bank=native_campaign_script_bank(w,true);
    native_campaign_script_tu15::object_set_function_variable(bank,object,name,
        std::bit_cast<std::uint32_t>(value),std::bit_cast<std::uint32_t>(frames)); // native 10FC30
}
void source_object_function_clear_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,std::uint32_t name) {
    auto bank=native_campaign_script_bank(w);
    native_campaign_script_tu15::object_clear_function_variable(bank,object,name); // native 10FD50
}
void source_object_set_permutation_tu15(PhysicsLiveWorldTu15 &w,std::uint32_t object,
    std::uint32_t region,std::uint32_t permutation) {
    //2AC3FB..2AC40E skips NONE, otherwise passes all three DWORD arguments
    //unchanged to BC440. That owner performs authored selection and callbacks.
    if(object==null_datum)return;
    auto a=w.prepare_source_call(std::array{object,region,permutation});
    w.run_actual(0xbc440u,a);
}
std::optional<float> source_object_function_value_tu15(PhysicsLiveWorldTu15 &w,
    std::uint32_t object,std::uint32_t name) {
    const auto bank=native_campaign_script_bank(w,true);
    const auto found=native_campaign_script_tu15::object_function_value(bank,object,name); // native 10FB70
    if(!found)return {};
    return std::bit_cast<float>(found->value);
}
std::map<std::pair<std::uint32_t,std::uint32_t>,float> source_object_function_samples_tu15(PhysicsLiveWorldTu15 &w) {
    auto &m=w.memory();const auto p=Bytes(m.view(0x51daf4u,4)).u32(0);
    if(!p)throw FormatError("Source shader functions lack their original slot-bank constructor");
    const Bytes bank(m.view(p,0x300u));std::map<std::pair<std::uint32_t,std::uint32_t>,float> out;
    for(unsigned slot=0;slot<32;++slot) {
        const auto object=bank.u32(0x280u+slot*4u);if(object==0xffffffffu)continue;
        const auto name=bank.u32(slot*0x14u);
        const auto value=source_object_function_value_tu15(w,object,name);
        if(!value)throw FormatError("Original occupied shader-function slot did not resolve");
        out.emplace(std::pair{object,name},*value);
    }
    return out;
}
}
