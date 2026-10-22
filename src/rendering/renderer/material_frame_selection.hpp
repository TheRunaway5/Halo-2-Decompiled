#pragma once
#include "halo2/scene.hpp"
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace halo2 {
// Retain every owner of a shared child pass, in original root order. The
// dependency cache owns no Materials: keeping shared_ptr copies would alter
// the source animation producer's copy-on-write decisions.
class MaterialFrameSelection {
    struct Node {const Material *address{};std::vector<unsigned> children;};
    const Material *bank_data_{};
    std::vector<Node> nodes_;
    std::vector<unsigned> root_nodes_,parent_;
    std::vector<std::vector<unsigned>> groups_;
    std::vector<unsigned> candidates_;
    std::vector<std::uint8_t> checked_,selected_;
    static constexpr unsigned absent=~0u;
    unsigned root(unsigned i) {
        while(parent_[i]!=i) {parent_[i]=parent_[parent_[i]];i=parent_[i];}
        return i;
    }
    template<class F> static void children(const Material &material,F &&f) {
        f(material.environment_pass);
        for(const auto &pass:material.world_overlays_before)f(pass);
        for(const auto &pass:material.world_overlays_after)f(pass);
        for(const auto &pass:material.world_bloom_passes)f(pass);
        f(material.planar_fog_pass);
        for(const auto &pass:material.world_planar_passes)f(pass);
    }
    void rebuild(std::span<const Material> materials) {
        bank_data_=materials.data();
        parent_.resize(materials.size());std::iota(parent_.begin(),parent_.end(),0u);
        nodes_.clear();root_nodes_.clear();root_nodes_.reserve(materials.size());
        std::unordered_map<const Material *,std::pair<unsigned,unsigned>> owners;
        owners.reserve(materials.size()*2);
        const auto visit=[&](auto &self,const Material &material,unsigned owner)->unsigned {
            auto [at,inserted]=owners.emplace(&material,std::pair{owner,unsigned(nodes_.size())});
            if(!inserted) {parent_[root(owner)]=root(at->second.first);return at->second.second;}
            const auto index=at->second.second;nodes_.emplace_back();
            Node node;node.address=&material;
            children(material,[&](const auto &pass){node.children.push_back(pass?self(self,*pass,owner):absent);});
            nodes_[index]=std::move(node);return index;
        };
        for(unsigned i=0;i<materials.size();++i)root_nodes_.push_back(visit(visit,materials[i],i));
        groups_.clear();groups_.resize(materials.size());
        for(unsigned i=0;i<materials.size();++i)groups_[root(i)].push_back(i);
        checked_.resize(nodes_.size());selected_.resize(materials.size());
    }
    bool unchanged(unsigned index,const Material &live) {
        // Only dereference a live root or child supplied by its live parent.
        // Cached raw addresses may have expired after a copy-on-write update.
        const auto &node=nodes_[index];
        if(node.address!=&live)return false;
        if(checked_[index])return true;
        checked_[index]=1;
        std::size_t next=0;bool equal=true;
        children(live,[&](const auto &pass) {
            if(!equal)return;
            if(next==node.children.size()) {equal=false;return;}
            const auto child=node.children[next++];
            equal=pass?(child!=absent && unchanged(child,*pass)):child==absent;
        });
        return equal && next==node.children.size();
    }
public:
    explicit MaterialFrameSelection(std::span<const Material> materials) {rebuild(materials);}
    void begin_frame(std::span<const Material> materials) {
        std::fill(checked_.begin(),checked_.end(),0);
        bool equal=materials.size()==root_nodes_.size();
        for(unsigned i=0;equal && i<materials.size();++i)equal=unchanged(root_nodes_[i],materials[i]);
        if(!equal)rebuild(materials);
        std::fill(selected_.begin(),selected_.end(),0);candidates_.clear();
    }
    // For a bank whose runtime producers change values or recursively detach
    // existing child passes by copying them. Detachment can only split sharing
    // groups, so the initial closure remains a conservative superset. Extra
    // owners retain original root order; no newly shared owner can be omitted.
    // Do not use this path when producers add cross-root links or replace a
    // child with an unrelated pass graph. begin_frame() checks general changes.
    void begin_frame_with_detach_only_pass_graph(std::span<const Material> materials) {
        if(materials.data()!=bank_data_ || materials.size()!=root_nodes_.size())
            throw FormatError("detach-only frame material bank changed storage");
        std::fill(selected_.begin(),selected_.end(),0);candidates_.clear();
    }
    void add(unsigned material) {
        if(material>=parent_.size())throw FormatError("frame material outside resident root bank");
        const auto group=root(material);
        if(!selected_[group]) {
            selected_[group]=1;
            candidates_.insert(candidates_.end(),groups_[group].begin(),groups_[group].end());
        }
    }
    void add(std::span<const Vertex> vertices) {
        unsigned previous=absent;
        for(const auto &v:vertices)if(v.material!=previous) {add(v.material);previous=v.material;}
    }
    std::vector<unsigned> roots() {
        std::sort(candidates_.begin(),candidates_.end());return candidates_;
    }
};
}
