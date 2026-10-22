#include "halo2/source_object_spatial_pool.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
void valid_pool(const SourceObjectSpatialDatumPool &p) {
    if((p.record_words!=3 && p.record_words!=8) || p.cursor>p.capacity ||
        p.highwater>p.capacity || p.active_count>p.capacity)
        throw FormatError("invalid source spatial datum-pool state");
}
std::array<std::uint32_t,8>& record(SourceObjectSpatialDatumPool&p,std::uint32_t token) {
    if(token==none || (token&0xffff)>=p.capacity)
        throw FormatError("source spatial link exceeds pool");
    return p.records[token&0xffff];
}
void primitive(std::array<std::uint32_t,8>&r,const ObjectLightSpatialPrimitive&p) {
    static_assert(sizeof(p)==20);
    const auto words=std::bit_cast<std::array<std::uint32_t,5>>(p);
    std::copy(words.begin(),words.end(),r.begin()+3);
}
void valid_cluster(const SourceObjectSpatialPool&p,std::int16_t c) {
    if(c<0 || std::size_t(c)>=p.heads.size())throw FormatError("source spatial cluster exceeds bank");
}
bool registered(const SourceObjectRegistrationOwner&o){return (o.primitive.object_flags&0x100)!=0;}
}
void SourceObjectSpatialDatumPool::activate(std::uint16_t prefix,bool debug) {
    valid_pool(*this);debug_fill=debug;cursor=highwater=active_count=0;
    next_generation=std::uint16_t(prefix|0x8000);occupied.fill(0);
    for(auto&r:records) {
        if(debug)std::fill_n(r.begin(),record_words,0xbabababau);
        r[0]&=0xffff0000u;
    }
}
std::uint32_t SourceObjectSpatialDatumPool::allocate() {
    valid_pool(*this);std::uint32_t i=none;
    for(auto n=cursor;n<highwater;++n)if(!(occupied[n/32]&(1u<<(n&31)))){i=n;break;}
    if(i==none && highwater<capacity)i=highwater;
    if(i==none)return none;
    occupied[i/32]|=1u<<(i&31);++active_count;cursor=i+1;
    if(highwater<=i)highwater=i+1;
    auto&r=records[i];std::fill_n(r.begin(),record_words,0);
    r[0]=next_generation;++next_generation;
    if(next_generation==0xffff)next_generation=0x8000;
    return (r[0]<<16)|i;
}
void SourceObjectSpatialDatumPool::release(std::uint32_t token) {
    valid_pool(*this);auto&r=record(*this,token);const auto i=token&0xffff;
    if(!(occupied[i/32]&(1u<<(i&31))) || !active_count)
        throw FormatError("source spatial release requires an actual allocated record");
    if(debug_fill)std::fill_n(r.begin(),record_words,0xbabababau);
    occupied[i/32]&=~(1u<<(i&31));r[0]&=0xffff0000u;
    if(i<cursor)cursor=i;
    if(i+1==highwater) {
        do{--highwater;}while(highwater && !(records[highwater-1][0]&0xffff));
    }
    --active_count;
}
SourceObjectSpatialPool::SourceObjectSpatialPool(std::size_t clusters,bool collidable)
    :heads(clusters,none),reference_name_prefix_(collidable?0x6f63:0x6f6e) {
    if(clusters>512)throw FormatError("source spatial bank exceeds512heads");
    references.record_words=3;entries.record_words=8;activate();
}
void SourceObjectSpatialPool::activate(bool debug) {
    std::fill(heads.begin(),heads.end(),none);
    entries.activate(0x6c63,debug);references.activate(reference_name_prefix_,debug);
}
void SourceObjectSpatialPool::insert(std::uint32_t owner,std::uint32_t&head,
    const ObjectLightSpatialPrimitive&p,std::span<const std::int16_t> memberships) {
    if(owner==none || memberships.size()>256)throw FormatError("source spatial insertion owner/memberships invalid");
    for(auto c:memberships)valid_cluster(*this,c);
    for(auto c:memberships) {
        const auto ref=references.allocate();
        if(ref!=none) {
            auto&r=record(references,ref);r[1]=std::uint32_t(std::int32_t(c));r[2]=head;head=ref;
        }
        const auto entry=entries.allocate();
        if(entry!=none) {
            auto&r=record(entries,entry);r[1]=owner;r[2]=heads[c];heads[c]=entry;primitive(r,p);
        }
    }
}
void SourceObjectSpatialPool::update(std::uint32_t owner,std::uint32_t head,
    const ObjectLightSpatialPrimitive&p) {
    unsigned refs=0;
    for(;head!=none;head=record(references,head)[2]) {
        if(++refs>references.capacity)throw FormatError("cyclic source spatial owner references");
        const auto c=std::int16_t(record(references,head)[1]);valid_cluster(*this,c);
        auto entry=heads[c];unsigned links=0;
        for(;entry!=none;entry=record(entries,entry)[2]) {
            if(++links>entries.capacity)throw FormatError("cyclic source spatial cluster links");
            auto&r=record(entries,entry);
            if(r[1]==owner){primitive(r,p);break;}
        }
    }
}
void SourceObjectSpatialPool::erase(std::uint32_t owner,std::uint32_t&head) {
    auto ref=head;unsigned refs=0;
    while(ref!=none) {
        if(++refs>references.capacity)throw FormatError("cyclic source spatial owner references");
        const auto&r=record(references,ref);const auto c=std::int16_t(r[1]);const auto next=r[2];
        valid_cluster(*this,c);references.release(ref);
        auto*link=&heads[c];unsigned links=0;
        while(*link!=none) {
            if(++links>entries.capacity)throw FormatError("cyclic source spatial cluster links");
            auto&e=record(entries,*link);
            if(e[1]==owner){const auto tail=e[2];entries.release(*link);*link=tail;break;}
            link=&e[2];
        }
        ref=next;
    }
    head=none;
}
std::vector<ObjectLightSpatialRecord> SourceObjectSpatialPool::cluster(std::int16_t c) const {
    valid_cluster(*this,c);std::vector<ObjectLightSpatialRecord> out;
    auto token=heads[c];unsigned links=0;
    while(token!=none) {
        if((token&0xffff)>=entries.capacity || ++links>entries.capacity)
            throw FormatError("source spatial cluster link exceeds pool");
        const auto&r=entries.records[token&0xffff];std::array<std::uint32_t,5> words;
        std::copy_n(r.begin()+3,5,words.begin());
        out.push_back({r[1],std::bit_cast<ObjectLightSpatialPrimitive>(words)});token=r[2];
    }
    return out;
}
SourceObjectRegistrationJournal::SourceObjectRegistrationJournal(std::size_t clusters)
    :collidable_(clusters,true),noncollidable_(clusters,false){}
void SourceObjectRegistrationJournal::event(std::uint64_t n,SourceObjectRegistrationEventKind kind,std::uint32_t owner) {
    if(!events_.empty() && n<=events_.back().ordinal)throw FormatError("source registration events are not actual monotonic chronology");
    events_.push_back({n,kind,owner});
}
SourceObjectRegistrationOwner& SourceObjectRegistrationJournal::mutable_owner(std::uint32_t h) {
    const auto p=owners_.find(h);if(p==owners_.end())throw FormatError("source registration owner lifetime absent");return p->second;
}
const SourceObjectRegistrationOwner& SourceObjectRegistrationJournal::owner(std::uint32_t h) const {
    const auto p=owners_.find(h);if(p==owners_.end())throw FormatError("source registration owner lifetime absent");return p->second;
}
void SourceObjectRegistrationJournal::birth(std::uint64_t n,const SourceObjectRegistrationOwner&o) {
    if(o.full_handle==none || o.pool_rank>=2048 || owners_.size()>=2048 || registered(o) ||
        (o.primitive.header_flags&0x40) || o.reference_head!=none)
        throw FormatError("source registration birth requires actual unregistered owner");
    for(const auto &[h,live]:owners_)if(h==o.full_handle || live.pool_rank==o.pool_rank || (h&0xffff)==(o.full_handle&0xffff))
        throw FormatError("source registration birth has ambiguous native pool identity");
    event(n,SourceObjectRegistrationEventKind::birth,o.full_handle);owners_.emplace(o.full_handle,o);
}
SourceObjectRegistrationEffects SourceObjectRegistrationJournal::link(SourceObjectRegistrationOwner&o,
    const SourceObjectLocation&location,const ObjectLightSpatialBsp&bsp) {
    if(registered(o) || (o.primitive.header_flags&0x40))throw FormatError("source owner must unregister before registration");
    if(bsp.clusters.size()!=collidable_.heads.size())throw FormatError("source registration BSP does not match live spatial bank");
    const auto previous=o.header_cluster;
    if(location.cluster==-1)o.primitive.object_flags|=0x40000;
    else {o.location=location;o.header_cluster=location.cluster;o.primitive.object_flags&=~0x40000u;}
    std::vector<std::uint32_t> forced;
    if(o.primitive.object_flags&0x200000u)forced.assign((bsp.clusters.size()+31)/32,none);
    const auto memberships=source_object_light_spatial_membership(bsp,o.location.cluster,
        o.primitive.primitive_sphere.center,o.primitive.primitive_sphere.radius,forced);
    auto&pool=(o.primitive.object_flags&0x200)?collidable_:noncollidable_;
    pool.insert(o.full_handle,o.reference_head,source_object_registration_primitive(o.primitive),memberships.clusters);
    o.primitive.object_flags|=0x100;o.primitive.header_flags|=0x40;
    return {true,false,previous==-1 && o.header_cluster!=-1,true};
}
SourceObjectRegistrationEffects SourceObjectRegistrationJournal::register_owner(std::uint64_t n,
    std::uint32_t h,const SourceObjectLocation&s,const ObjectLightSpatialBsp&bsp) {
    auto&o=mutable_owner(h);event(n,SourceObjectRegistrationEventKind::register_supplied,h);return link(o,s,bsp);
}
SourceObjectRegistrationEffects SourceObjectRegistrationJournal::register_default(std::uint64_t n,
    std::uint32_t h,const BspAtmosphere&b,std::int16_t selected,const ObjectLightSpatialBsp&bsp) {
    auto&o=mutable_owner(h);const auto location=source_object_default_registration_location(b,selected,o.primitive.primitive_sphere.center,o.root_position);
    event(n,SourceObjectRegistrationEventKind::register_default,h);return link(o,location,bsp);
}
void SourceObjectRegistrationJournal::unlink(SourceObjectRegistrationOwner&o) {
    if(!registered(o))throw FormatError("source unregister requires actual registered owner");
    auto&pool=(o.primitive.object_flags&0x200)?collidable_:noncollidable_;
    pool.erase(o.full_handle,o.reference_head);o.primitive.object_flags&=~0x100u;o.primitive.header_flags&=~0x40u;
}
void SourceObjectRegistrationJournal::unregister(std::uint64_t n,std::uint32_t h) {
    auto&o=mutable_owner(h);event(n,SourceObjectRegistrationEventKind::unregister,h);unlink(o);
}
void SourceObjectRegistrationJournal::attach_link(std::uint64_t n,std::uint32_t parent,
    std::uint32_t child,std::int16_t node,std::int16_t active_bsp) {
    auto&c=mutable_owner(child);auto&p=mutable_owner(parent);
    if(c.parent!=none || c.sibling!=none || registered(c) ||
        (c.primitive.header_flags&0x40) || c.reference_head!=none)
        throw FormatError("source attach link requires admitted unparented/unregistered child");
    unsigned depth=0;
    for(auto ancestor=parent;ancestor!=none;ancestor=owner(ancestor).parent) {
        if(ancestor==child || ++depth>2048)
            throw FormatError("source attach link would create or retain a hierarchy cycle");
    }
    event(n,SourceObjectRegistrationEventKind::attach_link,child);
    c.parent=parent;c.parent_node=std::uint8_t(node);
    c.sibling=p.child_head;p.child_head=child;c.primitive.header_flags|=0x80;
    c.location={none,-1,active_bsp};
}
bool SourceObjectRegistrationJournal::detach_link(std::uint64_t n,std::uint32_t child) {
    auto&c=mutable_owner(child);auto&p=mutable_owner(c.parent);
    auto*link=&p.child_head;unsigned visited=0;
    while(*link!=none && *link!=child) {
        if(++visited>2048)throw FormatError("source detach link contains a cyclic sibling list");
        link=&mutable_owner(*link).sibling;
    }
    const bool found=*link==child;
    event(n,SourceObjectRegistrationEventKind::detach_link,child);
    if(found){*link=c.sibling;c.sibling=none;}
    c.primitive.header_flags&=0x7f;c.parent=none;c.parent_node=0xff;
    return found;
}
void SourceObjectRegistrationJournal::primitive_update(std::uint64_t n,std::uint32_t h,
    const SourceObjectPrimitiveInput&p) {
    auto&o=mutable_owner(h);
    if(registered(o)!=bool(p.object_flags&0x100) || ((o.primitive.object_flags^p.object_flags)&0x200) ||
        ((o.primitive.header_flags^p.header_flags)&0x40) ||
        o.primitive.type!=p.type || o.primitive.primitive_sphere.center.x!=p.primitive_sphere.center.x ||
        o.primitive.primitive_sphere.center.y!=p.primitive_sphere.center.y ||
        o.primitive.primitive_sphere.center.z!=p.primitive_sphere.center.z ||
        o.primitive.primitive_sphere.radius!=p.primitive_sphere.radius)
        throw FormatError("source primitive update cannot replace registration/bounds event");
    event(n,SourceObjectRegistrationEventKind::primitive_update,h);o.primitive=p;
    if((p.header_flags&0x40) && (p.object_flags&0x200))collidable_.update(h,o.reference_head,source_object_registration_primitive(p));
}
SourceObjectRegistrationEffects SourceObjectRegistrationJournal::bound_refresh(std::uint64_t n,
    std::uint32_t h,ObjectLightSpatialSphere rendering,ObjectLightSpatialSphere next,Vec3 root,
    const BspAtmosphere&b,std::int16_t selected,const ObjectLightSpatialBsp&bsp) {
    auto&o=mutable_owner(h);const bool was=registered(o);
    event(n,SourceObjectRegistrationEventKind::bound_refresh,h);o.rendering_sphere=rendering;o.root_position=root;
    if(!source_object_light_spatial_bounds_changed(o.primitive.primitive_sphere,next,was))return {};
    if(was)unlink(o);
    o.primitive.primitive_sphere=next;
    if(!was)return {};
    const auto location=source_object_default_registration_location(b,selected,next.center,root);
    auto result=link(o,location,bsp);result.relinked=true;return result;
}
void SourceObjectRegistrationJournal::free(std::uint64_t n,std::uint32_t h) {
    const auto&o=owner(h);
    if(registered(o) || (o.primitive.header_flags&0xc0) || o.reference_head!=none ||
        o.child_head!=none || o.parent!=none || o.sibling!=none)
        throw FormatError("source free requires actual preceding unregister/child callbacks");
    event(n,SourceObjectRegistrationEventKind::free,h);owners_.erase(h);
}
std::vector<SourceObjectRegistrationOwner> SourceObjectRegistrationJournal::actual_pool_order() const {
    std::vector<SourceObjectRegistrationOwner> out;
    for(const auto &[h,o]:owners_)out.push_back(o);
    std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.pool_rank<b.pool_rank;});return out;
}
}
