#include "halo2/source_first_person_packet_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t at) {
    std::array<std::byte,4> b;m.read(at,b);return Bytes(b).u32(0);
}
std::uint32_t body(const SourcePhysicsContactMemory &m,std::uint32_t datum) {
    const auto pool=word(m,0x4ecbd0u);
    if(word(m,pool+0x24u)!=12u)throw FormatError("FP packet object pool stride differs");
    std::array<std::byte,4> b;SourceRawDatumPoolServices services;
    services.read=[&](auto at,std::size_t count)->std::span<const std::byte> {
        if(count>b.size())throw FormatError("FP packet datum operand is oversized");
        m.read(at,std::span(b).first(count));return std::span(b).first(count);
    };
    const auto row=source_raw_datum_lookup(pool,datum,services);
    if(!row)throw FormatError("FP packet requires a live full-salt object");return word(m,row+8u);
}
template<class T> void read_into(const SourcePhysicsContactMemory &m,std::uint32_t at,T &value) {
    m.read(at,std::as_writable_bytes(std::span(&value,1)));
}
bool handle(std::uint32_t d,unsigned limit) {return d!=none && (d&65535u)<limit && (d>>16)!=0u;}
std::uint32_t control_word(const SourceFirstPersonTemporalHandTu15 &h,unsigned at) {return Bytes(h.control).u32(at);}
struct Writer {
    std::vector<std::byte> bytes;
    void u(std::uint64_t v,unsigned n) {for(unsigned k=0;k<n;++k)bytes.push_back(std::byte(v>>(8*k)));}
    void raw(std::span<const std::byte> b) {bytes.insert(bytes.end(),b.begin(),b.end());}
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    void raw(std::span<std::byte> target) {
        if(target.size()>bytes.size()-at)throw FormatError("Truncated FP input packet");
        std::copy_n(bytes.begin()+at,target.size(),target.begin());at+=target.size();
    }
    std::uint64_t u(unsigned n) {
        if(n>bytes.size()-at)throw FormatError("Truncated FP input field");
        std::uint64_t v{};for(unsigned k=0;k<n;++k)v|=std::uint64_t(std::to_integer<unsigned>(bytes[at++]))<<(8*k);return v;
    }
};
void identity(Writer &w,const SourceFirstPersonContextIdentityTu15 &i) {
    w.u(i.player,4);w.u(i.unit,4);w.u(i.unit_definition,4);w.u(std::uint32_t(i.representation),4);
    for(auto d:i.weapons)w.u(d,4);for(auto d:i.weapon_definitions)w.u(d,4);
}
SourceFirstPersonContextIdentityTu15 identity(Reader &r) {
    SourceFirstPersonContextIdentityTu15 i;i.player=r.u(4);i.unit=r.u(4);i.unit_definition=r.u(4);
    i.representation=std::int32_t(r.u(4));for(auto &d:i.weapons)d=r.u(4);for(auto &d:i.weapon_definitions)d=r.u(4);return i;
}
}
SourceFirstPersonPosePacketTu15 source_capture_first_person_packet_tu15(
    const SourceRuntimeResources::Identity &resources,const SourceFirstPersonTemporalStateTu15 &state,
    const SourcePhysicsContactMemory &memory) {
    SourceFirstPersonPosePacketTu15 p;p.resources=resources;p.temporal=state;
    const auto unit=body(memory,state.identity.unit);
    if(word(memory,unit)!=state.identity.unit_definition || word(memory,unit+0x13cu)!=state.identity.player)
        throw FormatError("FP packet actor ownership differs from its completed temporal state");
    auto &a=p.actors;a.game_kind=word(memory,word(memory,0x4f3218u)+8u);
    const auto layout=word(memory,unit+0x114u);
    const auto node_bytes=std::int16_t(layout&65535u),node_offset=std::int16_t(layout>>16);
    if(node_bytes<=0 || node_bytes%52 || node_bytes/52>256 || node_offset<0)
        throw FormatError("FP source unit has no complete BAC30 WORLD input bank");
    a.unit_world_nodes.resize(unsigned(node_bytes)/52u);
    memory.read(unit+unsigned(node_offset),std::as_writable_bytes(std::span(a.unit_world_nodes)));
    const auto extension_layout=word(memory,unit+0x33cu);
    const auto extension_bytes=std::int16_t(extension_layout&65535u),extension_offset=std::int16_t(extension_layout>>16);
    if(extension_bytes<0x83 || extension_offset<0)
        throw FormatError("FP source unit has no complete native single-hand IK transition component");
    memory.read(unit+unsigned(extension_offset)+0x80u,a.single_hand_ik_transition);
    read_into(memory,unit+0x212u,a.current);read_into(memory,unit+0x216u,a.requested);read_into(memory,unit+0x218u,a.inventory);
    for(auto object:a.inventory)if(object!=none && std::none_of(a.weapons.begin(),a.weapons.end(),[&](const auto &w){return w.object==object;})) {
        const auto native=body(memory,object);SourceFirstPersonWeaponInputTu15 w;
        w.object=object;w.definition=word(memory,native);w.owner=word(memory,native+0x154u);
        w.item_flags=word(memory,native+0x12cu);read_into(memory,native+0x188u,w.charge);
        read_into(memory,native+0x224u,w.magazines);a.weapons.push_back(w);
    }
    source_validate_first_person_packet_tu15(p,state.identity);return p;
}
void source_validate_first_person_packet_tu15(const SourceFirstPersonPosePacketTu15 &p,
    const SourceFirstPersonContextIdentityTu15 &expected) {
    const auto &s=p.temporal;const auto &i=s.identity;const auto &a=p.actors;
    if(i!=expected || !handle(i.player,16) || !handle(i.unit,2048) || i.unit_definition==none ||
        (i.representation!=2 && i.representation!=3) || !s.generation)
        throw FormatError("FP packet full owner identity or generation differs");
    if(a.game_kind!=1 && a.game_kind!=2)throw FormatError("FP packet game kind is not supported");
    if(a.unit_world_nodes.empty() || a.unit_world_nodes.size()>256)
        throw FormatError("FP packet source unit WORLD count is invalid");
    for(const auto &node:a.unit_world_nodes) {
        const auto bytes=std::as_bytes(std::span(&node,1));
        for(unsigned n=0;n<13;++n)if(!std::isfinite(Bytes(bytes).f32(n*4u)))
            throw FormatError("FP packet source unit WORLD input is not finite");
    }
    if(a.weapons.size()>4)throw FormatError("FP packet exceeds native inventory capacity");
    std::set<std::uint32_t> inventory,described;
    for(auto d:a.inventory)if(d!=none) {
        if(!handle(d,2048) || !inventory.insert(d).second)throw FormatError("FP packet inventory identity is invalid");
    }
    for(const auto &w:a.weapons)if(!inventory.contains(w.object) || !described.insert(w.object).second ||
        w.owner!=i.unit || w.definition==none || !std::isfinite(w.charge))throw FormatError("FP packet weapon ownership or input is invalid");
    if(inventory!=described)throw FormatError("FP packet inventory has no complete typed weapon inputs");
    for(unsigned n=0;n<2;++n) {
        if(a.current[n]<-1 || a.current[n]>3 || a.requested[n]<-1 || a.requested[n]>3)
            throw FormatError("FP packet inventory selector is invalid");
        const auto selected=a.current[n]<0?none:a.inventory[unsigned(a.current[n])];
        if(selected!=i.weapons[n] || ((selected==none)!=(i.weapon_definitions[n]==none)))
            throw FormatError("FP packet current hand differs from its source binding");
        if(selected!=none) {
            const auto found=std::find_if(a.weapons.begin(),a.weapons.end(),[&](const auto &w){return w.object==selected;});
            if(found==a.weapons.end() || found->definition!=i.weapon_definitions[n])throw FormatError("FP packet weapon definition differs from its source binding");
        }
        const auto &h=s.hands[n];
        if(control_word(h,4)!=selected || control_word(h,0xf4)>64 || control_word(h,0xf8)>64 || h.saved_locals.size()>64)
            throw FormatError("FP packet native hand ownership or map count differs");
        for(unsigned channel=0;channel<3;++channel)if(control_word(h,8+channel*0x20)!=none &&
            (!std::isfinite(std::bit_cast<float>(control_word(h,0x20+channel*0x20))) ||
             !std::isfinite(std::bit_cast<float>(control_word(h,0x24+channel*0x20)))))throw FormatError("FP packet animation channel is not finite");
        for(const auto &local:h.saved_locals)for(unsigned k=0;k<8;++k)
            if(!std::isfinite(Bytes(local).f32(k*4)))throw FormatError("FP packet saved local pose is not finite");
    }
    if(s.camera_node<-1 || s.camera_node>=64)throw FormatError("FP packet camera node is invalid");
}
std::vector<std::byte> source_encode_first_person_packet_tu15(const SourceFirstPersonPosePacketTu15 &p,
    const SourceFirstPersonContextIdentityTu15 &expected) {
    source_validate_first_person_packet_tu15(p,expected);Writer w;w.u(0x50463248,4);w.u(4,1);
    w.raw(p.resources.resident);for(const auto &digest:p.resources.shared) {w.u(bool(digest),1);if(digest)w.raw(*digest);}
    const auto &s=p.temporal;identity(w,s.identity);w.u(s.generation,8);w.u(s.accepted_action_sequence,8);
    w.u(s.source_tick,4);w.u(s.view_flags,4);w.raw(s.clock_controls);w.raw(s.view_feedback);w.u(std::uint32_t(s.camera_node),4);
    for(const auto &h:s.hands) {w.raw(h.control);w.u(h.saved_locals.size(),1);for(const auto &local:h.saved_locals)w.raw(local);}
    const auto &a=p.actors;w.u(a.game_kind,1);w.u(a.unit_world_nodes.size(),2);
    w.raw(std::as_bytes(std::span(a.unit_world_nodes)));
    w.raw(a.single_hand_ik_transition);
    for(auto d:a.inventory)w.u(d,4);for(auto d:a.current)w.u(std::uint8_t(d),1);for(auto d:a.requested)w.u(std::uint8_t(d),1);
    w.u(a.weapons.size(),1);for(const auto &v:a.weapons) {
        w.u(v.object,4);w.u(v.definition,4);w.u(v.owner,4);w.u(v.item_flags,4);w.u(std::bit_cast<std::uint32_t>(v.charge),4);w.raw(v.magazines);
    }
    if(w.bytes.size()>32768)throw FormatError("FP input packet exceeded its bounded codec");return std::move(w.bytes);
}
SourceFirstPersonPacketDecodeTu15 source_decode_first_person_packet_tu15(std::span<const std::byte> input,
    const std::optional<SourceFirstPersonContextIdentityTu15> &expected) {
    Reader r{input};if(r.u(4)!=0x50463248 || r.u(1)!=4)throw FormatError("FP input packet has an unknown version");
    SourceFirstPersonPacketDecodeTu15 result;auto &p=result.packet;r.raw(p.resources.resident);
    for(auto &digest:p.resources.shared) {const auto present=r.u(1);if(present>1)throw FormatError("FP packet digest presence is invalid");if(present){digest.emplace();r.raw(*digest);}}
    auto &s=p.temporal;s.identity=identity(r);s.generation=r.u(8);s.accepted_action_sequence=r.u(8);
    s.source_tick=r.u(4);s.view_flags=r.u(4);r.raw(s.clock_controls);r.raw(s.view_feedback);s.camera_node=std::int32_t(r.u(4));
    for(auto &h:s.hands) {r.raw(h.control);const auto count=r.u(1);if(count>64)throw FormatError("FP saved local count exceeds native capacity");h.saved_locals.resize(count);for(auto &local:h.saved_locals)r.raw(local);}
    auto &a=p.actors;a.game_kind=r.u(1);const auto nodes=r.u(2);
    if(!nodes || nodes>256)throw FormatError("FP source WORLD count exceeds native capacity");
    a.unit_world_nodes.resize(nodes);r.raw(std::as_writable_bytes(std::span(a.unit_world_nodes)));
    r.raw(a.single_hand_ik_transition);
    for(auto &d:a.inventory)d=r.u(4);
    for(auto &d:a.current)d=std::bit_cast<std::int8_t>(std::uint8_t(r.u(1)));for(auto &d:a.requested)d=std::bit_cast<std::int8_t>(std::uint8_t(r.u(1)));
    const auto count=r.u(1);if(count>4)throw FormatError("FP weapon count exceeds native inventory");a.weapons.resize(count);
    for(auto &v:a.weapons) {v.object=r.u(4);v.definition=r.u(4);v.owner=r.u(4);v.item_flags=r.u(4);v.charge=std::bit_cast<float>(std::uint32_t(r.u(4)));r.raw(v.magazines);}
    if(r.at>32768)throw FormatError("FP input packet exceeded its bounded codec");
    source_validate_first_person_packet_tu15(p,expected.value_or(p.temporal.identity));result.consumed=r.at;return result;
}
FirstPersonViewBinding source_first_person_packet_view_tu15(const SourceFirstPersonPosePacketTu15 &p) {
    FirstPersonViewBinding out;out.unit=p.temporal.identity.unit;out.representation=p.temporal.identity.representation;
    for(unsigned n=0;n<2;++n) {auto &d=out.hands[n];const auto &s=p.temporal.hands[n];const Bytes b(s.control);
        d.flags=b.u32(0);d.weapon=b.u32(4);d.graph=b.u32(8);d.animation_identifier=b.u32(0xc);
        d.channel_delay=std::to_integer<std::uint8_t>(s.control[0x18]);d.channel_flags=std::to_integer<std::uint8_t>(s.control[0x19]);d.channel_options=b.u16(0x1a);
        d.events=b.u16(0x1c);d.future_events=b.u16(0x1e);d.rate=b.f32(0x20);d.frame=b.f32(0x24);
        d.activity=b.u32(0x84);d.fire_ticks=std::bit_cast<std::int16_t>(b.u16(0xf2));d.weapon_node_count=b.u32(0xf4);d.arms_node_count=b.u32(0xf8);
        for(unsigned k=0;k<64;++k){d.weapon_nodes[k]=std::bit_cast<std::int32_t>(b.u32(0xfc+k*4));d.arms_nodes[k]=std::bit_cast<std::int32_t>(b.u32(0x1fc+k*4));}
        d.dual_wield=p.temporal.identity.weapons[1]!=none;d.secondary_hand=n==1;
    }return out;
}
namespace {
// The private address domain is an implementation detail of this provider.
// Missing typed body fields fault here instead of borrowing another actor from
// the recipient's asset-only World or supplying an empty dynamic operand.
class ActorInputs final:public SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory &assets;
    std::map<std::uint32_t,std::byte> fields;
    const std::uint32_t pool,rows,bodies,end;
    static std::uint32_t private_domain(const SourceRuntimeMemory &base) {
        const auto gap=base.find_unowned_range(0x70000000u,0x7ffff000u,0x20000u,4096u);
        if(!gap)throw FormatError("FP recipient has no disjoint local typed-input domain");return *gap;
    }
    template<class T> void put(std::uint32_t at,const T &value) {
        auto bytes=std::as_bytes(std::span(&value,1));for(auto b:bytes)fields.emplace(at++,b);
    }
    void put_bytes(std::uint32_t at,std::span<const std::byte> bytes) {for(auto b:bytes)fields.emplace(at++,b);}
    void object(std::uint32_t datum,std::uint32_t at) {
        put(rows+(datum&65535u)*12u,std::uint16_t(datum>>16));put(rows+(datum&65535u)*12u+8u,at);
    }
public:
    ActorInputs(const SourceRuntimeMemory &base,const SourceFirstPersonPosePacketTu15 &p):assets(base),
        pool(private_domain(base)),rows(pool+0x1000u),bodies(pool+0x10000u),end(pool+0x20000u) {
        put(0x4ecbd0u,pool);put(pool+0x20u,std::uint32_t(2048));put(pool+0x24u,std::uint32_t(12));put(pool+0x44u,rows);
        const auto &id=p.temporal.identity;const auto &a=p.actors;
        auto object_high=(id.unit&65535u)+1u;for(const auto &w:a.weapons)object_high=std::max(object_high,(w.object&65535u)+1u);
        put(pool+0x38u,object_high);
        const auto players=pool+0x100u,player_rows=pool+0x8000u;
        put(0x4f54f4u,players);put(players+0x20u,std::uint32_t(16));put(players+0x24u,std::uint32_t(0x21c));
        put(players+0x38u,(id.player&65535u)+1u);
        put(players+0x44u,player_rows);const auto player=player_rows+(id.player&65535u)*0x21cu;
        put(player,std::uint16_t(id.player>>16));put(player+0x2cu,id.unit);
        object(id.unit,bodies);put(bodies,id.unit_definition);
        // BAC30 resolves a local bank offset. No source body address/layout
        // offset crosses the wire; the original node values/order do.
        put(bodies+0x114u,std::uint32_t(a.unit_world_nodes.size()*52u)|(0x1000u<<16));
        put_bytes(bodies+0x1000u,std::as_bytes(std::span(a.unit_world_nodes)));
        //16B558 resolves the unit-relative extension locally. Only its actual
        // three transition inputs are owned; any other body read still faults.
        put(bodies+0x33eu,std::int16_t(0x400));
        put_bytes(bodies+0x480u,a.single_hand_ik_transition);
        put(bodies+0x13cu,id.player);put(bodies+0x212u,a.current);put(bodies+0x216u,a.requested);put(bodies+0x218u,a.inventory);
        unsigned index{};for(const auto &w:a.weapons) {
            const auto at=bodies+0x8000u+(index++)*0x1000u;object(w.object,at);
            put(at,w.definition);put(at+0x12cu,w.item_flags);put(at+0x154u,w.owner);put(at+0x188u,w.charge);put(at+0x224u,w.magazines);
        }
        // Keep the authentic local game/rules owner, overriding only the typed
        // kind consumed by native binding. It must describe this same session.
        const auto game=word(assets,0x4f3218u);
        if(word(assets,game+8u)!=a.game_kind)throw FormatError("FP recipient session game kind differs");
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(out.size()>std::uint64_t(0x100000000ull)-at)throw FormatError("FP actor operand wraps");
        std::size_t n{};while(n<out.size()) {
            const auto address=at+std::uint32_t(n);const auto found=fields.find(address);
            if(found!=fields.end()){out[n++]=found->second;continue;}
            if(address>=pool && address<end) {
                std::ostringstream message;message<<"FP receiver requires uncaptured typed actor input 0x"<<std::hex<<address;
                throw FormatError(message.str());
            }
            const auto start=n++;
            while(n<out.size() && !fields.contains(at+std::uint32_t(n)) &&
                !((at+n)>=pool && (at+n)<end))++n;
            assets.read(at+std::uint32_t(start),out.subspan(start,n-start));
        }
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw FormatError("FP actor inputs are readonly");}
};
}
struct SourceFirstPersonPoseReceiverTu15::Impl {
    const Cache &cache;SourceFirstPersonContextTu15 context;
    Impl(const Cache &c,std::shared_ptr<const SourceRuntimeResources> r):cache(c),context(c,std::move(r)){}
};
SourceFirstPersonPoseReceiverTu15::SourceFirstPersonPoseReceiverTu15(const Cache &c,
    std::shared_ptr<const SourceRuntimeResources> r):impl_(std::make_unique<Impl>(c,std::move(r))){}
SourceFirstPersonPoseReceiverTu15::~SourceFirstPersonPoseReceiverTu15()=default;
std::vector<SourceFirstPersonModelTu15> SourceFirstPersonPoseReceiverTu15::sample(
    const SourceRuntimeMemory &assets,const SourcePhysicsWorldRuntimeAbi &caller,
    const SourceFirstPersonPosePacketTu15 &p,const SourceFirstPersonContextIdentityTu15 &expected,
    Vec3 position,Vec3 forward,Vec3 up,const SourcePhysicsWorldRuntimeServices &cpu) {
    source_validate_first_person_packet_tu15(p,expected);
    if(p.resources!=impl_->context.resources_identity())throw FormatError("FP recipient immutable resource identity differs");
    const auto body_model=impl_->cache.load_initial_model_definition(expected.unit_definition).render_model;
    const auto body_metadata=impl_->cache.load_source_tag_metadata(body_model);
    const auto body_nodes=Bytes(body_metadata.bytes_at(body_metadata.source_address(),body_metadata.size())).u32(0x48u);
    if(p.actors.unit_world_nodes.size()!=body_nodes)
        throw FormatError("FP recipient BAC30 input count differs from its actual authored unit model");
    ActorInputs actors(assets,p);std::vector<SourceFirstPersonModelTu15> result;
    impl_->context.with_sampling_inputs(actors,caller,p.temporal,[&](const auto &input) {
        result=source_sample_first_person_pose_tu15(impl_->cache,input,caller,0,position,forward,up,cpu);
    },cpu);return result;
}
}
