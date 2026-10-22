#include "halo2/scene.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <type_traits>

namespace halo2 {
namespace {
// Native continuation admission only; this is neither an Xbox save record nor
// a network codec. Explicit scalar encoding avoids padding and process pointers.
struct Binding {
    std::vector<unsigned char> bytes;
    void integer(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<unsigned char>(value>>(8*i)));
    }
    void scalar(float value){integer(std::bit_cast<std::uint32_t>(value));}
    void vec(Vec3 value){scalar(value.x);scalar(value.y);scalar(value.z);}
    void string(const std::string &value){integer(value.size());bytes.insert(bytes.end(),value.begin(),value.end());}
};
std::array<std::byte,32> content_binding(const LevelContent &content,
    const std::optional<SourceSceneGameContext> &context,bool factory,bool resolver,bool child_factory=false,bool trigger_query=false) {
    Binding h;h.string("halo2-native-scene-authority-v1");h.integer(content.bsp_count);
    h.integer(context.has_value());if(context){h.integer(context->game_type);h.integer(context->engine);}
    h.integer(factory);h.integer(resolver);
    if(child_factory)h.string("original-model-child-parent-generation-v1");
    if(trigger_query)h.string("original-tu15-trigger-world-node-query-v1");
    h.integer(content.objects.size());
    for(const auto &p:content.objects) {
        h.integer(p.netgame_row);h.integer(p.model_attachment.has_value());
        if(p.model_attachment){h.integer(p.model_attachment->parent);h.integer(p.model_attachment->parent_marker);h.integer(p.model_attachment->child_marker);}
        h.integer(p.type);h.integer(p.placement);h.integer(p.name);h.integer(p.origin_bsp);h.integer(p.bsp_policy);
        h.integer(p.definition);h.integer(p.flags);h.integer(p.variant);h.integer(p.manual_bsps);
        h.vec(p.position);h.vec(p.rotation);h.vec(p.bounding_offset);h.scalar(p.scale);
        h.integer(p.source);h.integer(p.damageable);h.integer(p.unique_id);h.integer(p.lightmapping_policy);
        h.integer(p.valid_multiplayer_games);h.integer(p.pathfinding_references.size());
        for(const auto &[a,b]:p.pathfinding_references){h.integer(a);h.integer(b);}
        h.integer(p.weapon.has_value());if(p.weapon){h.integer(p.weapon->rounds_total);h.integer(p.weapon->rounds_loaded);h.integer(p.weapon->flags);}
        h.scalar(p.bounding_radius);
    }
    h.integer(content.object_names.size());
    for(const auto &n:content.object_names){h.string(n.name);h.integer(n.type);h.integer(n.placement);}
    h.integer(content.bsp_transitions.size());
    for(const auto &t:content.bsp_transitions){h.integer(t.pair);h.integer(t.volume);}
    h.integer(content.trigger_volumes.size());
    for(const auto &v:content.trigger_volumes){h.string(v.name);h.integer(v.object_name);h.integer(v.node);
        h.vec(v.forward);h.vec(v.up);h.vec(v.position);h.vec(v.extents);h.scalar(v.query_radius);}
    std::array<std::byte,32> result{};
    if(!SHA256(h.bytes.data(),h.bytes.size(),reinterpret_cast<unsigned char *>(result.data())))
        throw FormatError("Cannot bind scene continuation content");
    return result;
}
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
bool finite(const Transform &t){return std::isfinite(t.scale)&&finite(t.position)&&
    std::all_of(t.basis.begin(),t.basis.end(),[](Vec3 v){return finite(v);});}
void validate_values(const SceneObjects::AuthorityCheckpoint &saved,const LevelContent &content) {
    if(saved.objects.size()>2048 || saved.names.size()!=content.object_names.size())
        throw FormatError("Scene continuation exceeds capacity or changes name bindings");
    std::map<std::uint32_t,std::size_t> slots;
    std::vector<std::uint32_t> names(saved.names.size(),null_datum);
    for(std::size_t i=0;i<saved.objects.size();++i) {
        const auto &o=saved.objects[i];if(o.handle==null_datum)continue;
        if(!(o.handle>>16) || (o.handle&65535u)>=2048 || o.placement>=content.objects.size() ||
            !finite(o.position) || !finite(o.rotation) || !std::isfinite(o.scale) ||
            (o.solved_transform && !finite(*o.solved_transform)) ||
            (o.solved_nodes && !std::all_of(o.solved_nodes->begin(),o.solved_nodes->end(),[](const auto &t){return finite(t);})) ||
            !slots.emplace(o.handle,i).second)
            throw FormatError("Scene continuation has an invalid live generation or pose");
        const auto &p=content.objects[o.placement];
        if(p.name>=0) {
            if(std::size_t(p.name)>=names.size() || names[p.name]!=null_datum)
                throw FormatError("Scene continuation has duplicate or invalid authored names");
            names[p.name]=o.handle;
        }
    }
    if(slots!=saved.slots || names!=saved.names)
        throw FormatError("Scene continuation lookup bindings differ from its objects");
    for(const auto &[key,f]:saved.functions)
        if(!slots.contains(key.first) || !key.second || key.second==null_datum ||
            !std::isfinite(f.from) || !std::isfinite(f.to) || f.duration<0)
            throw FormatError("Scene continuation has an invalid shader interpolation");
}
template<class Lookup>void validate_identities(const SceneObjects::AuthorityCheckpoint &saved,
    const LevelContent &content,Lookup lookup) {
    for(const auto &o:saved.objects)if(o.handle!=null_datum) {
        const auto &native=lookup(o.handle);const auto &p=content.objects[o.placement];
        if(native.handle!=o.handle || !native.published || native.type!=p.type || native.definition!=p.definition)
            throw FormatError("Scene continuation differs from its constructed full identity");
    }
}
void validate_owned(const SceneObjects::AuthorityCheckpoint &saved,const LevelContent &content,
    const NativeObjectLifetimes &incoming,const NativeObjectLifetimes &current) {
    if(incoming.pool().source_memory() || incoming.pool().capacity()!=current.pool().capacity() ||
        incoming.pool().stride()!=current.pool().stride())
        throw FormatError("Scene continuation changes its owned allocator shape");
    validate_identities(saved,content,[&](auto h)->const auto &{return incoming.owner(h);});
    for(const auto &o:saved.objects)if(o.handle!=null_datum) {
        const auto header=incoming.header(o.handle);
        if(header.size()!=12 || header[3]!=incoming.owner(o.handle).type)
            throw FormatError("Scene continuation differs from its owned object header type");
    }
}
bool same_owned(const NativeObjectLifetimes &a,const NativeObjectLifetimes &b) {
    const auto &x=a.pool(),&y=b.pool();
    return !x.source_memory() && !y.source_memory() && x.capacity()==y.capacity() && x.stride()==y.stride() &&
        x.cursor()==y.cursor() && x.high_water()==y.high_water() && x.count()==y.count() && x.next_salt()==y.next_salt() &&
        std::ranges::equal(x.elements(),y.elements()) && std::ranges::equal(x.bitmap(),y.bitmap()) &&
        std::ranges::equal(a.events(),b.events());
}
}
struct SceneObjects::PreparedAuthority::Impl {
    SceneObjects *owner{};
    bool *staging{};
    AuthorityCheckpoint saved;
    std::optional<NativeObjectLifetimes> shared_owned;
    std::optional<NativeObjectLifetimes::SourceAuthorityCheckpoint> shared_source;
    SourceRuntimeMemory *bank{};
    NativeRuntimeAddress native;
    ~Impl(){if(staging)*staging=false;}
};
SceneObjects::PreparedAuthority::PreparedAuthority()=default;
SceneObjects::PreparedAuthority::~PreparedAuthority()=default;
SceneObjects::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
SceneObjects::PreparedAuthority &SceneObjects::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool SceneObjects::PreparedAuthority::for_owner(const SceneObjects &owner)const noexcept{return impl_ && impl_->owner==&owner;}
SceneObjects::AuthorityCheckpoint SceneObjects::authority_checkpoint() const {
    if(source_replica_ || authority_calls_.depth || authority_calls_.staging || authority_calls_.fault)
        throw FormatError("Scene continuation requires a completed unfaulted authority boundary");
    AuthorityCheckpoint saved;
    saved.source_factory=bool(source_owner_.create);saved.render_attribution_resolver=bool(render_attribution_definition_);
    saved.source_child_factory=bool(source_owner_.adopt_child);
    saved.source_trigger_query=bool(source_owner_.contains_trigger);
    saved.content_binding=content_binding(content_,game_context_,saved.source_factory,saved.render_attribution_resolver,saved.source_child_factory,saved.source_trigger_query);
    saved.bsp=bsp_;saved.objects=objects_;saved.slots=slots_;saved.names=names_;saved.functions=functions_;
    saved.pool=shared_lifetimes_?(shared_lifetimes_->pool().source_memory()?AuthorityPool::SharedSource:AuthorityPool::SharedOwned):AuthorityPool::LocalOwned;
    if(!shared_lifetimes_)saved.local_lifetimes=local_lifetimes_;
    if(saved.bsp>=content_.bsp_count)throw FormatError("Scene continuation has an invalid current BSP");
    validate_values(saved,content_);
    validate_identities(saved,content_,[&](auto h)->const auto &{return object_lifetimes().owner(h);});
    return saved;
}
SceneObjects::PreparedAuthority SceneObjects::prepare_authority_values(const AuthorityCheckpoint &saved) {
    const auto pool=shared_lifetimes_?(shared_lifetimes_->pool().source_memory()?AuthorityPool::SharedSource:AuthorityPool::SharedOwned):AuthorityPool::LocalOwned;
    if(source_replica_ || authority_calls_.depth || authority_calls_.staging || saved.version!=1 || saved.bsp>=content_.bsp_count ||
        saved.pool!=pool || saved.source_factory!=bool(source_owner_.create) ||
        saved.source_child_factory!=bool(source_owner_.adopt_child) ||
        saved.source_trigger_query!=bool(source_owner_.contains_trigger) ||
        saved.render_attribution_resolver!=bool(render_attribution_definition_) ||
        saved.content_binding!=content_binding(content_,game_context_,bool(source_owner_.create),bool(render_attribution_definition_),bool(source_owner_.adopt_child),bool(source_owner_.contains_trigger)) ||
        saved.local_lifetimes.has_value()!=(pool==AuthorityPool::LocalOwned))
        throw FormatError("Scene continuation is incompatible with its local content or owner");
    PreparedAuthority result;result.impl_=std::make_unique<PreparedAuthority::Impl>();
    result.impl_->owner=this;result.impl_->staging=&authority_calls_.staging;authority_calls_.staging=true;
    validate_values(saved,content_);result.impl_->saved=saved;return result;
}
SceneObjects::PreparedAuthority SceneObjects::prepare_authority(const AuthorityCheckpoint &saved) {
    auto result=prepare_authority_values(saved);
    if(saved.pool!=AuthorityPool::LocalOwned || saved.local_lifetimes->pool().source_memory())
        throw FormatError("Standalone scene continuation requires its owned lifetime bank");
    validate_owned(saved,content_,*saved.local_lifetimes,object_lifetimes());return result;
}
SceneObjects::PreparedAuthority SceneObjects::prepare_authority(const AuthorityCheckpoint &saved,const NativeObjectLifetimes &incoming) {
    auto result=prepare_authority_values(saved);
    if(saved.pool!=AuthorityPool::SharedOwned || incoming.pool().source_memory())
        throw FormatError("Owned shared scene continuation requires its separate owned bank");
    validate_owned(saved,content_,incoming,object_lifetimes());
    result.impl_->shared_owned.emplace(incoming);return result;
}
SceneObjects::PreparedAuthority SceneObjects::prepare_authority(const AuthorityCheckpoint &saved,
    const NativeObjectLifetimes::SourceAuthorityCheckpoint &incoming,const SourceRuntimeMemory &replacement) {
    auto result=prepare_authority_values(saved);
    if(saved.pool!=AuthorityPool::SharedSource)
        throw FormatError("Source scene continuation requires its separate source identity bank");
    // This also admits the complete raw pool, sidecars and ordered native
    // history against the exact staged allocation. Cancellation is read-only.
    auto admitted=shared_lifetimes_->prepare_source_authority(incoming,replacement);
    validate_identities(saved,content_,[&](auto h)->const auto &{return incoming.owners.at(h);});
    result.impl_->shared_source=incoming;result.impl_->bank=shared_lifetimes_->pool().source_memory();
    result.impl_->native=replacement.native_address(incoming.pool.pool,0x4c);return result;
}
bool SceneObjects::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !authority_calls_.staging || authority_calls_.depth)return false;
    auto &p=*prepared.impl_;
    try {
        if(p.shared_owned && !same_owned(*shared_lifetimes_,*p.shared_owned))return false;
        if(p.shared_source) {
            const auto &saved=*p.shared_source;const auto &current=shared_lifetimes_->pool();
            if(current.source_memory()!=p.bank || current.address()!=p.native || current.source_binding()!=saved.pool ||
                !std::ranges::equal(shared_lifetimes_->events(),saved.events))return false;
            for(const auto &[handle,owner]:saved.owners)if(shared_lifetimes_->owner(handle)!=owner)return false;
        }
    }catch(...){return false;}
    static_assert(std::is_nothrow_swappable_v<std::optional<NativeObjectLifetimes>>);
    objects_.swap(p.saved.objects);slots_.swap(p.saved.slots);names_.swap(p.saved.names);functions_.swap(p.saved.functions);
    if(!shared_lifetimes_)local_lifetimes_.swap(p.saved.local_lifetimes);
    bsp_=p.saved.bsp;authority_calls_.fault=false;prepared.impl_.reset();return true;
}
} // namespace halo2
