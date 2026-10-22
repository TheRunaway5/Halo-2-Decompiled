#include "halo2/source_weapon_material_input.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <set>
namespace halo2 {
SourceWeaponMaterialNames SourceWeaponMaterialNames::from_cache(const Cache &cache) {
    SourceWeaponMaterialNames result;
    if(!cache.is_mcc())return result;
    struct Named {std::uint32_t id;std::string_view name;};
    // Exact paired original/MCC symbol identities; never apply an index delta
    // to arbitrary authored aliases, gates or scale-function references.
    constexpr std::array<Named,13> mcc{{
        {0x030005b2u,"one"},{0x040005b3u,"zero"},{0x09000574u,"turned_on"},
        {0x04000573u,"heat"},{0x03000583u,"age"},{0x12000576u,"primary_ammunition"},
        {0x17000577u,"primary_ammunition_ones"},{0x17000578u,"primary_ammunition_tens"},
        {0x0f000580u,"primary_charged"},{0x0f0005b6u,"random_constant"},{0x05000567u,"alive"},
        {0x0e00057eu,"primary_firing"},{0x1000057fu,"secondary_firing"}}};
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
    const SourceWeaponMaterialNames &names,std::optional<std::uint32_t> object,
    std::optional<std::uint16_t> object_damage_flags,
    std::optional<SourceWeaponFiringMaterialState> firing) {
    const ObjectFunctionHandler handler=[&](std::uint32_t input)->std::optional<FunctionValue> {
        if(!input)return FunctionValue{1.f,true};
        const auto builtin=names.builtin(input);
        if(!builtin)return {};
        input=*builtin;
        if(input==0x030005a6u)return FunctionValue{1.f,true};
        if(input==0x040005a7u)return FunctionValue{};
        if(input==0x0e000572u || input==0x10000573u) {
            if(!object || !firing)return {};
            return source_weapon_firing_material(*firing,input==0x10000573u?1u:0u);
        }
        if(input==0x0500055bu) {
            if(!object || !object_damage_flags)return {};
            // Complete base BEC50 selector: the weapon object's actual
            // +10A damage flags own alive, independently of its parent.
            const bool alive=(*object_damage_flags&4u)==0;
            return FunctionValue{alive?1.f:0.f,alive};
        }
        if(input==0x0f0005aau) {
            if(!object)return {};
            // Original10DE40's base BEC50 callback, BEE4E..BEE79: the
            // complete salted object datum owns this stable scalar. The
            // original46A160 multiplier is rounded only at the float store.
            const auto sample=(*object*0x19660du+0x3c6ef35fu)>>16;
            const float value=float(sample)*std::bit_cast<float>(0x37800080u);
            return FunctionValue{value,value>0.f};
        }
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
    // This handler owns the cache namespace. An MCC-authored alias whose ID
    // equals an Xbox one/zero token must still execute its own definition.
    return evaluate_object_function(functions,tables,name,seconds,{},handler,false);
}
} // namespace halo2
