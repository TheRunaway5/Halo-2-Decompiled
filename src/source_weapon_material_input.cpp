#include "halo2/source_weapon_material_input.hpp"
#include "halo2/cache.hpp"
#include <set>
namespace halo2 {
std::optional<FunctionValue> source_weapon_material_input(std::uint32_t name,
    const SourceWeaponShaderState &state,std::span<const SourceWeaponMagazineDefinition> definitions,
    std::span<const SourceWeaponMagazineState> magazines,std::span<const ObjectFunction> functions,
    const ShaderFunctionTables &tables,float seconds,std::optional<FunctionValue> primary_trigger) {
    const ObjectFunctionHandler handler=[&](std::uint32_t input)->std::optional<FunctionValue> {
        if(!input || input==0x030005a6u)return FunctionValue{1.f,true};
        if(input==0x040005a7u)return FunctionValue{};
        if(input==0x09000568u)return source_weapon_turned_on(state);
        if(const auto value=source_weapon_shader_input(input,state))return FunctionValue{*value,*value>0.f};
        if(input==0x1200056au && definitions.size()==magazines.size())
            return source_weapon_primary_ammunition(definitions,magazines);
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
