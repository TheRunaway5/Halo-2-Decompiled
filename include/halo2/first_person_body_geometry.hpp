#pragma once
#include "halo2/model_selection.hpp"
#include "halo2/model.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>

namespace halo2 {
struct FirstPersonBodyGeometry {
    std::uint32_t render_model=0xffffffffu;
    unsigned lod=4;
    std::vector<unsigned> sections;
    bool connected_torso{};
};

// PC presentation extension: the original Chief FP model ends in a solid
// waist cap. Keep168DED's admission, but draw the actual full model's body/legs
// regions using the completed native full-unit WORLD pose. Native4BCC0's FP
// geometry LOD4 and the authored permutation/section mapping choose the bank;
// gameplay's separate anatomy selection retains its uncapped lower torso.
// Arms and helmet remain absent from this view, whose foreground owns its
// separate FP arms.
// Representations without these four independent regions keep their authored
// FP model (Elite's body region also contains its arms).
inline FirstPersonBodyGeometry first_person_body_geometry(const Cache &cache,
    std::uint32_t unit_definition,std::uint32_t authored_first_person_model) {
    FirstPersonBodyGeometry result;result.render_model=authored_first_person_model;
    if(authored_first_person_model==0xffffffffu)return result;
    const auto definition=cache.load_initial_model_definition(unit_definition);
    const auto metadata=cache.load_source_tag_metadata(definition.render_model);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x24));
    const auto count=root.u32(0x1c),pointer=root.u32(0x20);
    if(count>16 || count!=definition.render_sections.size())
        throw FormatError("First-person full body lost its authored render-region mapping");
    const Bytes regions(metadata.bytes_at(pointer,std::size_t(count)*16));
    std::array<unsigned,4> roles{~0u,~0u,~0u,~0u};
    constexpr std::array<std::string_view,4> names{"body","legs","arms","helmet"};
    for(unsigned region=0;region<count;++region) {
        const auto name=cache.string_id(regions.u32(region*16));
        for(unsigned role=0;role<names.size();++role)if(name==names[role]) {
            if(roles[role]!=~0u)throw FormatError("First-person full body has duplicate region roles");
            roles[role]=region;
        }
    }
    if(std::find(roles.begin(),roles.end(),~0u)!=roles.end())return result;
    auto selection=cache.load_initial_model_selection(unit_definition);
    if(!selection.resolved() || selection.permutations.size()!=count)
        throw FormatError("First-person full body has no deterministic authored permutation");
    for(unsigned region=0;region<count;++region)
        if(region!=roles[0] && region!=roles[1])selection.permutations[region]=255;
    result.sections=source_initial_model_sections(definition,selection,result.lod);
    if(result.sections.size()!=2)
        throw FormatError("First-person full body requires both authored torso and leg sections");
    result.render_model=definition.render_model;result.connected_torso=true;
    return result;
}

// PC first-person self-visibility: the full Chief's body region also contains
// neck/head tissue. Region-level helmet omission cannot hide that geometry.
// Keep every authored triangle except those with an active influence from
// the named self-hidden anatomy subtree. No vertex is moved, clipped or newly
// generated. Gameplay explicitly selects spine1, retaining lower abdomen and
// legs while omitting upper-chest/head/arm tissue around the native eye.
inline std::vector<unsigned> first_person_body_vertex_selection(const Cache &cache,
    const RenderModel &model,std::string_view hidden_root) {
    if(model.mesh.vertices.size()%3 || model.skin.size()!=model.mesh.vertices.size())
        throw FormatError("First-person body visibility requires complete authored triangles/skin bindings");
    unsigned neck=~0u;
    for(unsigned node=0;node<model.bones.size();++node)if(cache.string_id(model.bones[node].name)==hidden_root) {
        if(neck!=~0u)throw FormatError("First-person body has multiple authored visibility owners");
        neck=node;
    }
    if(neck==~0u)throw FormatError("Connected first-person body lacks its authored visibility owner");
    std::vector<bool> hidden(model.bones.size()+model.compound_nodes.size());
    for(unsigned node=0;node<model.bones.size();++node) {
        int parent=int(node);unsigned depth{};
        while(parent>=0) {
            if(unsigned(parent)>=model.bones.size() || ++depth>model.bones.size())
                throw FormatError("First-person body has an invalid authored node hierarchy");
            if(unsigned(parent)==neck){hidden[node]=true;break;}
            parent=model.bones[unsigned(parent)].parent;
        }
    }
    for(unsigned node=0;node<model.compound_nodes.size();++node) {
        const auto &compound=model.compound_nodes[node];float sum{};
        for(unsigned influence=0;influence<4;++influence) {
            const auto source=compound.nodes[influence];if(source==255)break;
            if(source>=model.bones.size()+node)
                throw FormatError("First-person body compound node escapes its preceding authored palette");
            const auto weight=influence<3?compound.weights[influence]:1.f-sum;
            if(weight!=0 && hidden[source])hidden[model.bones.size()+node]=true;
            sum=weight+sum;
        }
    }
    std::vector<unsigned> selected;selected.reserve(model.mesh.vertices.size());
    for(unsigned first=0;first<model.skin.size();first+=3) {
        bool omit{};
        for(unsigned vertex=0;vertex<3;++vertex)for(unsigned influence=0;influence<4;++influence) {
            const auto &skin=model.skin[first+vertex];if(skin.weights[influence]==0)continue;
            if(skin.bones[influence]>=hidden.size())throw FormatError("First-person body skin escapes its authored palette");
            omit=omit||hidden[skin.bones[influence]];
        }
        if(!omit)for(unsigned vertex=0;vertex<3;++vertex)selected.push_back(first+vertex);
    }
    if(selected.empty())throw FormatError("First-person anatomical visibility removed the complete body");
    return selected;
}
}
