#pragma once
#include "halo2/source_live_transparent_groups_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include <map>
#include <memory>
#include <tuple>

namespace halo2 {
// Indices refer to the same owned frame's group command/base-unit banks.
// Immutable pass metadata is shared so moving/copying that frame cannot leave
// pointers into another frame or a temporary resolver. No pose is selected.
struct SourceTransparentGroupPart {
    unsigned base_unit{},command_ordinal{},sorted_ordinal{};
    std::shared_ptr<const MenuWorldSourcePass> pass;
};
struct SourceTransparentGroupLayerParts {
    unsigned layer{};
    std::vector<SourceTransparentGroupPart> parts;
};
struct SourceTransparentGroupParts {
    std::vector<SourceTransparentGroupLayerParts> ordered_layers;
    // Full salted identities of the bodies that this group, rather than the
    // earlier opaque submission, owns. Attached models retain their own IDs.
    std::vector<std::uint32_t> owners;
};
// Resolve actual3BAA0 descriptors against copied main units and literal tag
// metadata, preserving every descriptor and empty admitted layer. The same
// resolver retains immutable pass/geometry metadata across frames. This is a
// command join, not resource/vertex binding, graphics readiness or GPU output.
class SourceTransparentGroupPartBindings {
public:
    explicit SourceTransparentGroupPartBindings(const Cache &cache):cache_(cache){}
    SourceTransparentGroupParts resolve(const SourceLiveTransparentGroupTu15 &,
        std::span<const SourceLiveModelBaseDrawTu15>);
private:
    const Cache &cache_;
    using Key=std::tuple<std::uint32_t,unsigned,unsigned,unsigned,unsigned>;
    std::map<Key,std::shared_ptr<const MenuWorldSourcePass>> passes_;
    std::map<std::tuple<std::uint32_t,unsigned,std::uint32_t,bool>,unsigned> classes_;
};
}
