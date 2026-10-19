#include "halo2/source_weapon_material_input.hpp"
#include "halo2/cache.hpp"
#include <set>
namespace halo2 {
SourceWeaponMaterialNames SourceWeaponMaterialNames::from_cache(const Cache &cache) {
    SourceWeaponMaterialNames result;
    if(!cache.is_mcc())return result;
    struct Named {std::uint32_t id;std::string_view name;};
    // Exact paired original/MCC symbol identities; never apply an index delta
    // to arbitrary authored aliases, gates or scale-function references.
    constexpr std::array<Named,9> mcc{{
        {0x030005b2u,"one"},{0x040005b3u,"zero"},{0x09000574u,"turned_on"},
        {0x04000573u,"heat"},{0x03000583u,"age"},{0x12000576u,"primary_ammunition"},
        {0x17000577u,"primary_ammunition_ones"},{0x17000578u,"primary_ammunition_tens"},
        {0x0f000580u,"primary_charged"}}};
    for(unsigned i=0;i<mcc.size();++i) {
        if(cache.string_id(mcc[i].id)!=mcc[i].name)
            throw FormatError("MCC weapon material builtin differs from its authored symbol identity");
        result.authored_[i]=mcc[i].id;
    }
    return result;
}
std::optional<std::uint32_t> SourceWeaponMaterialNames::builtin(std::uint32_t authored) const {
    for(unsigned i=0;i<authored_.size();++i)if(authored_[i]==authored)return original_[i];
    return {};
}
std::optional<FunctionValue> source_weapon_material_input(std::uint32_t name,
    const SourceWeaponShaderState &state,std::span<const SourceWeaponMagazineDefinition> definitions,
    std::span<const SourceWeaponMagazineState> magazines,std::span<const ObjectFunction> functions,
    const ShaderFunctionTables &tables,float seconds,std::optional<FunctionValue> primary_trigger,
    const SourceWeaponMaterialNames &names) {
    const ObjectFunctionHandler handler=[&](std::uint32_t input)->std::optional<FunctionValue> {
        if(!input)return FunctionValue{1.f,true};
        const auto builtin=names.builtin(input);
        if(!builtin)return {};
        input=*builtin;
        if(input==0x030005a6u)return FunctionValue{1.f,true};
        if(input==0x040005a7u)return FunctionValue{};
        if(input==0x09000568u)return source_weapon_turned_on(state);
        if(const auto value=source_weapon_shader_input(input,state))return FunctionValue{*value,*value>0.f};
        if(definitions.size()==magazines.size())
            if(const auto value=source_weapon_magazine_input(input,definitions,magazines))return value;
        if(input==0x0f000574u)return primary_trigger;
        return {};
    };
    if(const auto direct=handler(name))return direct;
    std::set<std::uint32_t> pending;
    const auto admitted=[&](auto&& self,std::uint32_t id)->bool {
        if(handler(id))return true;
        if(!pending.insert(id).second)throw FormatError("cyclic weapon material function");
        bool found=false;
        for(const auto &function:functions)if(function.output==id) {
            found=true;
            // Original built-in inputs/enable gates are not authored function
            // overrides. Never manufacture an unavailable producer or phase.
            if(!handler(function.input) || (function.enable && !handler(function.enable)) ||
               (!function.curve.empty() && function.curve[0]==std::byte{3} && (function.flags&8u)) ||
               (function.scale && !self(self,function.scale))) {
                pending.erase(id);return false;
            }
        }
        pending.erase(id);return found;
    };
    if(!admitted(admitted,name))return {};
    return evaluate_object_function(functions,tables,name,seconds,{},handler);
}
} // namespace halo2
