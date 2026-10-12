#pragma once
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_object_view.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
struct ObjectLightingBinding;
struct SourceNetgameWeaponSceneStats {
    unsigned rows{},resident_rows{},ground_unported{},controller_unported{},widget_unported{},selector_unported{};
    unsigned materials{};
};
struct SourceWorldWeaponShaderInput {
    SourceWeaponShaderState weapon;
    std::span<const SourceWeaponMagazineDefinition> definitions;
    std::span<const SourceWeaponMagazineState> magazines;
    std::span<const ObjectFunction> functions;
    const ShaderFunctionTables *tables{};
    std::optional<float> value(std::uint32_t name)const {
        if(const auto value=source_weapon_shader_input(name,weapon))return value;
        if(!tables)throw FormatError("source world weapon shader function tables are unavailable");
        if(const auto value=source_weapon_magazine_shader_input(name,definitions,magazines,functions,*tables))
            return value->value;
        return {};
    }
};
// Exact authored stock rows and all six geometry/shader LOD banks are loaded
// before Renderer allocation. Only the recovered stationary levitated branch
// submits geometry; general settling/controller/widget work remains explicit.
class SourceNetgameWeaponScene {
public:
    SourceNetgameWeaponScene(const Cache &,std::span<const NetgameEquipment>,std::size_t bsp,
                            Mesh &,const CollisionWorld &,int source_engine = 2);
    ~SourceNetgameWeaponScene();
    std::span<const SourceNetgameWeaponRegistration> registrations()const;
    const SourceNetgameWeaponSceneStats &stats()const;
    std::optional<SourceWorldWeaponShaderInput> shader_state(unsigned material,
        std::span<const SourceNativeWorldWeapon>)const;
    void append_view(std::vector<Vertex> &,std::vector<ObjectLightingBinding> &,
        std::span<const SourceNativeWorldWeapon>,std::int32_t render_frame,const SourceObjectViewLod &);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
