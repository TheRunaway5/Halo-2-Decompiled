#include "halo2/source_live_effect_light_commands_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
constexpr std::uint32_t main_context=0x556e18u,secondary=0x5598f8u,
    main_arena=main_context+0x25cu,light_arena=secondary+0x25cu;
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
std::uint16_t half(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,2)).u16(0);}
template<class T>void copy(const SourceRuntimeMemory& m,std::uint32_t p,T& value) {
    std::memcpy(&value,m.view(p,sizeof(value)).data(),sizeof(value));
}
template<class T>bool equal(const SourceRuntimeMemory& m,std::uint32_t p,const T& value) {
    return !std::memcmp(m.view(p,sizeof(value)).data(),&value,sizeof(value));
}
void object(const SourceRuntimeMemory& m,std::uint32_t id) {
    SourceRawDatumPoolServices storage;storage.read=[&](auto p,auto n){return m.view(p,n);};
    const auto row=source_raw_datum_lookup(word(m,0x4ecbd0u),id,storage);
    if(!row || !word(m,row+8u))throw FormatError("TU1.5 light command input lost a live full object identity");
}
void validate_state(const SourceDrawUnitState& s) {
    if(s.main_prefix>s.draw_count || s.draw_count>s.units.size() || s.override_count>s.overrides.size() ||
       s.section_highwater>s.sections.size() || s.arena.used>=s.arena.words.size())
        throw FormatError("TU1.5 light draw state exceeds its original retained banks");
}
SourceLiveEffectLightQueueTu15 queue(const SourceRuntimeMemory& m,std::uint32_t offset) {
    SourceLiveEffectLightQueueTu15 q;q.header=word(m,secondary+offset);
    if(!q.header)throw FormatError("TU1.5 light query has no original queue allocation");
    q.capacity=half(m,q.header);
    if(!q.capacity || q.capacity>32767u || half(m,q.header+4u)>q.capacity)
        throw FormatError("TU1.5 light query queue capacity/count is invalid");
    q.header_native=m.native_address(q.header,24);
    for(unsigned n=0;n<4;++n) {
        q.streams[n]=word(m,q.header+8u+n*4u);
        if(!q.streams[n])throw FormatError("TU1.5 light query queue lacks an original column");
        q.streams_native[n]=m.native_address(q.streams[n],q.capacity*(n==0 || n==3?2u:4u));
    }
    return q;
}
void retained(const SourceRuntimeMemory& m,const SourceLiveEffectLightStateTu15& s) {
    if(word(m,0x4ecc18u)!=s.bsp || word(m,0x4cd3a0u)!=s.current_context ||
       m.native_address(s.bsp,0x160)!=s.bsp_native ||
       m.native_address(0x4c6958u,sizeof(s.state.units))!=s.draws_native ||
       m.native_address(secondary,0x2a80)!=s.secondary_native ||
       word(m,0x4cd39cu)!=s.state.main_prefix || word(m,0x4cd398u)!=s.state.draw_count ||
       word(m,0x4cdd48u)!=s.state.override_count || word(m,0x4ce1ccu)!=s.state.section_highwater ||
       !equal(m,0x4c6958u,s.state.units) || !equal(m,0x4cd3a4u,s.state.coverage) ||
       !equal(m,0x4cd6f6u,s.state.override_slots) || !equal(m,0x4cda48u,s.state.overrides) ||
       !equal(m,0x4cdd4cu,s.state.sections) || !equal(m,main_arena,s.main_arena.words) ||
       half(m,main_context+0xa5cu)!=s.main_arena.used || !equal(m,light_arena,s.state.arena.words) ||
       half(m,secondary+0xa5cu)!=s.state.arena.used)
        throw FormatError("TU1.5 light collection no longer owns its retained native draw/query banks");
    for(unsigned n=0;n<3;++n) {
        const auto now=queue(m,std::array{12u,20u,24u}[n]);const auto& old=s.queues[n];
        if(now.header!=old.header || now.capacity!=old.capacity || now.streams!=old.streams ||
           now.header_native!=old.header_native || now.streams_native!=old.streams_native)
            throw FormatError("TU1.5 light query replaced an original queue allocation");
    }
}
}
SourceLiveEffectLightStateTu15 source_live_effect_light_state_tu15(const SourceRuntimeMemory& m) {
    SourceLiveEffectLightStateTu15 out;out.bsp=word(m,0x4ecc18u);out.current_context=word(m,0x4cd3a0u);
    if(!out.bsp || (out.current_context!=main_context && out.current_context!=secondary))
        throw FormatError("TU1.5 light collection lacks its retained source BSP/context");
    auto& s=out.state;s.main_prefix=word(m,0x4cd39cu);s.draw_count=word(m,0x4cd398u);
    s.override_count=word(m,0x4cdd48u);s.section_highwater=word(m,0x4ce1ccu);
    copy(m,0x4c6958u,s.units);copy(m,0x4cd3a4u,s.coverage);copy(m,0x4cd6f6u,s.override_slots);
    copy(m,0x4cda48u,s.overrides);copy(m,0x4cdd4cu,s.sections);copy(m,light_arena,s.arena.words);
    s.arena.used=half(m,secondary+0xa5cu);copy(m,main_arena,out.main_arena.words);
    out.main_arena.used=half(m,main_context+0xa5cu);validate_state(s);
    if(out.main_arena.used>=out.main_arena.words.size())throw FormatError("TU1.5 main mask arena cursor is invalid");
    for(unsigned n=0;n<3;++n)out.queues[n]=queue(m,std::array{12u,20u,24u}[n]);
    out.bsp_native=m.native_address(out.bsp,0x160);out.draws_native=m.native_address(0x4c6958u,sizeof(s.units));
    out.secondary_native=m.native_address(secondary,0x2a80);
    return out;
}
SourceLiveEffectLightHandoffTu15 source_live_effect_light_handoff_tu15(
    const SourceRuntimeMemory& m,const SourceLiveEffectLightStateTu15& before,
    const SourceDrawUnitState& s,const SourceBspAdmission& bsp,const SourceObjectAdmission& objects,
    const SourceLiveEffectLightBindingsTu15& bindings,const SourceLiveEffectLightInputWriterTu15& write) {
    if(!write || !bindings.light_arena_token ||
       std::uint64_t(bindings.light_arena_token)+sizeof(s.arena.words)>0x100000000ull)
        throw FormatError("TU1.5 light handoff lacks its explicit caller arena/input writer");
    retained(m,before);validate_state(s);
    const auto prefix=s.main_prefix;
    if(prefix!=before.state.main_prefix || s.override_count<before.state.override_count ||
       s.section_highwater<before.state.section_highwater || bsp.associations!=objects.associations ||
       objects.associations.size()>512u ||
       std::memcmp(s.units.data(),before.state.units.data(),prefix*sizeof(s.units[0])) ||
       std::memcmp(s.coverage.data(),before.state.coverage.data(),prefix) ||
       std::memcmp(s.override_slots.data(),before.state.override_slots.data(),prefix) ||
       std::memcmp(s.overrides.data(),before.state.overrides.data(),before.state.override_count*4u))
        throw FormatError("TU1.5 light collector replaced its retained main prefix or prior overrides");
    const std::array<std::span<const SourceBspAdmissionRecord>,3> rows{bsp.clusters,objects.objects,bsp.instances};
    for(unsigned n=0;n<rows.size();++n)if(rows[n].size()>before.queues[n].capacity)
        throw FormatError("TU1.5 completed light queue exceeds its actual source allocation");
    for(const auto& row:objects.objects)object(m,row.id);
    const auto native_instances=word(m,before.bsp+0x144u),instances=word(m,before.bsp+0x140u);
    if(instances>2048u || (instances && !native_instances))
        throw FormatError("TU1.5 light BSP instance allocation exceeds its source unit index");
    if(instances)(void)m.view(native_instances,std::size_t(instances)*88u);
    for(const auto& row:bsp.clusters)if(row.id>=word(m,before.bsp+0x9cu))
        throw FormatError("TU1.5 light cluster queue escapes its actual source BSP");
    for(const auto& row:bsp.instances)if(row.id>=instances)
        throw FormatError("TU1.5 light instance queue escapes its actual source BSP");
    if(bindings.instance_token && instances!=bindings.instance_transforms.size())
        throw FormatError("TU1.5 light instance binding changed its authored BSP extent");
    auto units=s.units;SourceLiveEffectLightHandoffTu15 out;
    out.main_prefix=prefix;out.draw_count=s.draw_count;
    for(unsigned n=prefix;n<s.draw_count;++n) {
        auto& u=units[n].words;
        if(s.override_slots[n]!=255 && s.override_slots[n]>=s.override_count)
            throw FormatError("TU1.5 light unit consumes an absent override slot");
        if((u[0]&0x40u) && u[5]) {
            if(u[5]>=bindings.light_arena_token && std::uint64_t(u[5])<std::uint64_t(bindings.light_arena_token)+sizeof(s.arena.words)) {
                if((u[5]-bindings.light_arena_token)&3u)throw FormatError("TU1.5 light mask pointer is unaligned");
                u[5]=light_arena+(u[5]-bindings.light_arena_token);++out.mask_relocations;
            } else if(u[5]<light_arena || u[5]>=light_arena+sizeof(s.arena.words) || ((u[5]-light_arena)&3u))
                throw FormatError("TU1.5 light mask does not belong to its declared secondary arena");
        }
        if(u[1]!=0xffffffffu && (u[0]&0x1fffffu)) {
            if(!u[4])throw FormatError("TU1.5 light model unit lacks its actual owner context");
            object(m,word(m,u[4]));
            const auto row=word(m,0x4f0414u)+(u[1]&65535u)*16u;
            if(word(m,row)!=0x6d6f6465u || word(m,row+4u)!=u[1])
                throw FormatError("TU1.5 light model unit lost its full authored render-model identity");
        } else if(u[1]==0xffffffffu && (u[0]&0x1000u)) {
            const auto instance=(u[3]>>18)&2047u;
            if(instance>=instances)throw FormatError("TU1.5 light instance ordinal escapes its actual source BSP");
            const auto native=native_instances+instance*88u;
            if(bindings.instance_token) {
                const auto token=std::uint64_t(bindings.instance_token)+instance*88u;
                if(token>0xffffffffu || u[7]!=token || std::memcmp(m.view(native,sizeof(Transform)).data(),
                    &bindings.instance_transforms[instance],sizeof(Transform)))
                    throw FormatError("TU1.5 light instance pointer/transform differs from its authored binding");
                u[7]=native;++out.instance_relocations;
            } else if(u[7]!=native)throw FormatError("TU1.5 light instance has no admitted native transform pointer");
        }
    }
    // Validate every destination before the first source input publication.
    const auto admit=[&](std::uint32_t p,std::size_t n){if(n)m.validate_mutable_view(p,n);};
    admit(0x4c6958u+prefix*32u,(850u-prefix)*32u);admit(0x4cd3a4u+prefix,850u-prefix);
    admit(0x4cd6f6u+prefix,850u-prefix);admit(0x4cda48u,sizeof(s.overrides));admit(0x4cdd4cu,sizeof(s.sections));
    for(auto p:{0x4cd398u,0x4cdd48u,0x4ce1ccu,0x4cd3a0u,secondary+0x2a78u,secondary+0x2a7cu})admit(p,4);
    admit(secondary+0x1cu,sizeof(bsp.cluster_entries));admit(secondary+0x21cu,sizeof(bsp.marked_clusters));
    admit(light_arena,sizeof(s.arena.words));admit(secondary+0xa5cu,4);admit(secondary+0xa60u,objects.associations.size()*16u);
    for(unsigned n=0;n<3;++n){admit(before.queues[n].header+4u,2);for(unsigned c=0;c<4;++c)admit(before.queues[n].streams[c],rows[n].size()*(c==0 || c==3?2u:4u));}
    const auto bytes=[&](std::uint32_t p,std::span<const std::byte> raw){if(!raw.empty())write(p,raw);};
    const auto put=[&](std::uint32_t p,const auto& value){bytes(p,std::as_bytes(std::span(&value,1)));};
    bytes(0x4c6958u+prefix*32u,std::as_bytes(std::span(units).subspan(prefix)));
    bytes(0x4cd3a4u+prefix,std::as_bytes(std::span(s.coverage).subspan(prefix)));
    bytes(0x4cd6f6u+prefix,std::as_bytes(std::span(s.override_slots).subspan(prefix)));
    put(0x4cd398u,s.draw_count);put(0x4cdd48u,s.override_count);put(0x4ce1ccu,s.section_highwater);
    put(0x4cda48u,s.overrides);put(0x4cdd4cu,s.sections);
    put(secondary+0x1cu,bsp.cluster_entries);put(secondary+0x21cu,bsp.marked_clusters);
    put(light_arena,s.arena.words);put(secondary+0xa5cu,s.arena.used);
    put(secondary+0xa5eu,std::uint16_t(objects.associations.size()));bytes(secondary+0xa60u,std::as_bytes(std::span(objects.associations)));
    for(unsigned n=0;n<3;++n) {
        const auto& q=before.queues[n];out.queue_counts[n]=unsigned(rows[n].size());
        for(unsigned r=0;r<rows[n].size();++r){const auto& row=rows[n][r];put(q.streams[0]+r*2u,row.flags);put(q.streams[1]+r*4u,row.id);put(q.streams[2]+r*4u,row.other);put(q.streams[3]+r*2u,row.association);}
        put(q.header+4u,std::int16_t(rows[n].size()));
    }
    put(secondary+0x2a78u,1u);put(secondary+0x2a7cu,bindings.query_mode);put(0x4cd3a0u,secondary);
    return out;
}
SourceLiveEffectLightCommandsTu15 source_live_effect_light_private_commands_tu15(const SourceRuntimeMemory& m) {
    SourceLiveEffectLightCommandsTu15 out;
    for(unsigned n=0;n<3;++n) {
        auto& b=out.legacy.banks[n];copy(m,0x4ce268u+n*24u,b.header);
        if(b.header[0]!=std::array{2560u,1024u,512u}[n] || b.header[1]>b.header[0] || !b.header[5])
            throw FormatError("TU1.5 light producer escaped its actual command allocation");
        b.storage.resize(b.header[0]);std::memcpy(b.storage.data(),m.view(b.header[5],b.storage.size()*24u).data(),b.storage.size()*24u);
        for(unsigned r=0;r<b.header[1];++r){auto& command=b.storage[r].words;if(command[0]==0x4cbe0u)command[0]=0x4cbf0u;if(command[1]==0x4d0a0u)command[1]=0x4d0b0u;}
    }
    const auto special=std::uint64_t(0x4d2b18u)+std::uint64_t(word(m,0x515f20u))*0x408u;
    if(special>0xffffffffu)throw FormatError("TU1.5 light special-unit bank address wraps");
    out.special_bank=std::uint32_t(special);copy(m,out.special_bank,out.special_source);
    std::memcpy(out.legacy.special_units.data(),out.special_source.data(),0x400);
    std::memcpy(&out.legacy.special_count,out.special_source.data()+0x404,2);
    if(out.legacy.special_count<0 || out.legacy.special_count>32)throw FormatError("TU1.5 light special-unit count escapes its selected source bank");
    out.legacy.main_object_counts={word(m,0x4c2aa0u),word(m,0x4c2aa4u)};
    return out;
}
SourceLiveEffectLightCommandsTu15 source_live_effect_light_commands_tu15(
    PhysicsLiveWorldTu15& world,const SourceLiveEffectLightStateTu15& before,
    const SourceDrawUnitState& s,const SourceBspAdmission& bsp,const SourceObjectAdmission& objects,
    const SourceLiveEffectLightBindingsTu15& bindings,const SourceLiveEffectLightCommandObserverTu15& observe) {
    const auto handoff=source_live_effect_light_handoff_tu15(world.memory(),before,s,bsp,objects,bindings,
        [&](auto p,auto raw){world.write_source_call_input(p,raw);});
    auto a=world.prepare_source_call();a.gp[0]=0;
    if(observe)observe(true,a,world.memory());
    world.run_actual(0x40e80u,a);
    if(observe)observe(false,a,world.memory());
    auto result=source_live_effect_light_private_commands_tu15(world.memory());result.handoff=handoff;return result;
}
}
