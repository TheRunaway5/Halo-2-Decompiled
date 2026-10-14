#include "halo2/prop_sandbox.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>

namespace halo2 {
namespace {
using namespace world_continuation;
void validate_regions(const PropSandboxDefinition &d,const PropSandboxState &state) {
    const auto &s=state.regions.selection;const auto &definition=d.region_definition;
    if(!s.resolved() || s.variant_index!=d.initial_regions.variant_index || s.states.size()!=definition.regions.size() ||
        s.permutations.size()!=s.states.size() || state.regions.looping_effects.size()!=s.states.size())
        throw FormatError("Prop continuation differs from its authored region bank");
    for(unsigned i=0;i<s.states.size();++i) {
        const int choice=s.states[i].variant_permutation;
        const int mapped=s.variant_index<0?-1:definition.variants.at(unsigned(s.variant_index)).regions.at(i);
        if(choice!=-1 && (mapped<0 || std::size_t(choice)>=definition.variants.at(unsigned(s.variant_index)).definitions.at(unsigned(mapped)).permutations.size()))
            throw FormatError("Prop continuation has an invalid region variant choice");
    }
    (void)source_initial_model_sections(definition,s,5);
}
void validate(const PropSandbox::AuthorityCheckpoint &saved,const LevelContent &content,
    const SceneObjects::AuthorityCheckpoint &scene) {
    if(saved.version!=1 || scene.version!=1 || saved.scene_binding!=scene.content_binding || saved.props.size()>2048 ||
        saved.definition_binding!=prop_definitions(content))
        throw FormatError("Prop continuation has incompatible scene or definition bindings");
    for(const auto &[handle,p]:saved.props) {
        const auto &state=p.state;scene_owner(scene,handle,state.placement);
        if(handle!=state.handle || state.placement>=content.prop_sandbox.size() || !content.prop_sandbox[state.placement] ||
            state.placement>=content.objects.size() || content.objects[state.placement].definition!=state.definition)
            throw FormatError("Prop continuation has an invalid authored generation");
        const auto &d=*content.prop_sandbox[state.placement];
        if(state.definition!=d.datum || state.sections.size()!=d.sections.size() || p.nodes.size()!=d.model.bones.size() ||
            !finite(p.root) || !std::ranges::all_of(p.nodes,[](const auto &n){return finite(n);}) ||
            !std::isfinite(state.vitality.body) || !std::isfinite(state.vitality.shield))
            throw FormatError("Prop continuation has invalid vitality, pose or damage-section shape");
        for(std::size_t i=0;i<state.sections.size();++i) {
            const auto count=d.sections[i].responses.size();const auto &s=state.sections[i];
            if((count<16 && (s.destroyed&~((1u<<count)-1u))) || ((s.pending>>3) && (s.pending&7u)>=count))
                throw FormatError("Prop continuation has an invalid pending/destroyed response");
        }
        validate_regions(d,state);
    }
    // Events can survive retirement of their originating prop. Preserve the
    // captured attribution/request rather than reconstructing a live object.
    for(const auto &e:saved.pending_events)
        if(e.handle==null_datum || !(e.handle>>16) || (e.handle&65535u)>=2048 || !finite(e.position) ||
            !finite(e.forward) || !finite(e.effect_position) || !finite(e.effect_forward))
            throw FormatError("Prop continuation has an invalid pending effect/damage request");
}
}
struct PropSandbox::PreparedAuthority::Impl {
    PropSandbox *owner{};bool *staging{};
    std::map<std::uint32_t,Prop> props;
    std::vector<PropSandboxEvent> events;
    bool collision_dirty{};
    ~Impl(){if(staging)*staging=false;}
};
PropSandbox::PreparedAuthority::PreparedAuthority()=default;
PropSandbox::PreparedAuthority::~PreparedAuthority()=default;
PropSandbox::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
PropSandbox::PreparedAuthority &PropSandbox::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool PropSandbox::PreparedAuthority::for_owner(const PropSandbox &owner)const noexcept{return impl_ && impl_->owner==&owner;}
PropSandbox::AuthorityCheckpoint PropSandbox::authority_checkpoint(const SceneObjects::AuthorityCheckpoint &scene)const {
    authority_.capture();if(replica_)throw FormatError("A prop region display replica cannot export authority");
    AuthorityCheckpoint saved;saved.scene_binding=scene.content_binding;saved.definition_binding=prop_definitions(*content_);
    for(const auto &[handle,p]:props_) {
        if(p.state.placement>=content_->prop_sandbox.size() || p.definition!=content_->prop_sandbox[p.state.placement].get())
            throw FormatError("Prop authority is bound to another local content owner");
        saved.props.emplace(handle,AuthorityProp{p.state,p.root,p.nodes,p.active});
    }
    saved.pending_events=events_;saved.collision_dirty=collision_dirty_;validate(saved,*content_,scene);return saved;
}
PropSandbox::PreparedAuthority PropSandbox::prepare_authority(const AuthorityCheckpoint &saved,const SceneObjects::AuthorityCheckpoint &scene) {
    authority_.prepare();if(replica_)throw FormatError("A prop region display replica cannot import authority");
    PreparedAuthority result;result.impl_=std::make_unique<PreparedAuthority::Impl>();
    result.impl_->owner=this;result.impl_->staging=&authority_.staging;authority_.staging=true;
    validate(saved,*content_,scene);
    for(const auto &[handle,p]:saved.props)
        result.impl_->props.emplace(handle,Prop{p.state,content_->prop_sandbox[p.state.placement].get(),p.root,p.nodes,p.active});
    result.impl_->events=saved.pending_events;result.impl_->collision_dirty=saved.collision_dirty;return result;
}
bool PropSandbox::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !authority_.staging || authority_.depth)return false;
    props_.swap(prepared.impl_->props);events_.swap(prepared.impl_->events);collision_dirty_=prepared.impl_->collision_dirty;
    authority_.fault=false;prepared.impl_.reset();return true;
}
}
