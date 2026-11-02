#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_tu15_shadow_basis_32340.hpp"
#include "halo2/native_tu15_shadow_view_320a0.hpp"
#include <bit>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace halo2 {
namespace {
struct ShadowViewDiagnostics {
    bool enabled=[] {const auto* v=std::getenv("HALO2_NATIVE_SHADOW_VIEW_STATS");return v&&*v&&*v!='0';}();
    std::atomic<std::uint64_t> calls{},admitted{};
    ~ShadowViewDiagnostics(){if(enabled)std::fprintf(stderr,"Native320A0 stats: calls=%llu admitted=%llu\n",
        static_cast<unsigned long long>(calls.load()),static_cast<unsigned long long>(admitted.load()));}
};
void completed_shadow_view(bool admitted) {
    static ShadowViewDiagnostics stats;if(stats.enabled){++stats.calls;if(admitted)++stats.admitted;}
}
}
std::vector<SourceLiveShadowRootTu15> source_live_shadow_root_frames_tu15(
    PhysicsLiveWorldTu15& world,std::span<const SourceShadowCandidate> selected,
    const SourceShadowAdmissionObserverTu15& observe,const SourceShadowRootCompletedTu15& completed_root,
    const SourceShadowCommandsTu15& commands) {
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto real=[&](std::uint32_t p){return std::bit_cast<float>(word(p));};
    const auto put=[&](std::uint32_t p,const auto& v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};
    const auto body=[&](std::uint32_t id){(void)world.object_body(id);return word(word(word(0x4ecbd0u)+0x44u)+(id&65535u)*12u+8u);};
    const auto vector=[&](std::uint32_t p){Vec3 v;std::memcpy(&v,memory.view(p,sizeof(v)).data(),sizeof(v));return v;};
    const auto raw_vector=[&](std::uint32_t p){return std::array{word(p),word(p+4),word(p+8)};};
    const auto value_vector=[](const std::array<std::uint32_t,3>& v){return Vec3{std::bit_cast<float>(v[0]),std::bit_cast<float>(v[1]),std::bit_cast<float>(v[2])};};
    //32340's two authored basis candidates. Reject an unsupported changed
    // global rather than selecting a different direction implicitly.
    const auto first_basis=vector(word(0x474d0cu)),second_basis=vector(word(0x474d14u));
    if(first_basis.x!=0 || first_basis.y!=1 || first_basis.z!=0 ||
        second_basis.x!=-1 || second_basis.y!=0 || second_basis.z!=0)
        throw FormatError("TU1.5 fixed shadow basis globals differ from the admitted projection tail");
    std::vector<SourceLiveShadowRootTu15> result;
    for(const auto& candidate:selected) {
        auto& root=result.emplace_back();root.owner=std::uint32_t(candidate.owner);root.history_fade=candidate.requested_fade;
        const auto object=body(root.owner);
        auto a=world.prepare_source_call();const auto return_address=word(a.gp[4]);
        const auto scratch=a.gp[4]-0x100u;
        put(scratch,0.f);put(scratch+4u,std::uint8_t(0));
        a.gp[4]=scratch-16u;put(a.gp[4],return_address);
        put(a.gp[4]+4u,root.owner);put(a.gp[4]+8u,scratch);put(a.gp[4]+12u,scratch+4u);
        world.run_actual(0x3d7e0u,a);
        root.owner_fade=real(scratch);
        if(!(a.gp[0]&255u))continue;
        if(word(object+0x14u)!=0xffffffffu){root.route=SourceLiveShadowRootRouteTu15::parent;continue;}
        if(memory.view(0x4c6840u,1)[0]==std::byte{}){root.route=SourceLiveShadowRootRouteTu15::disabled;continue;}
        const auto local_light=memory.view(scratch+4u,1)[0]!=std::byte{};
        a=world.prepare_source_call();put(scratch+8u,std::uint8_t(0));a.gp[6]=root.owner;a.gp[7]=scratch+8u;
        world.run_actual(0x3d7b0u,a);root.first_person=memory.view(scratch+8u,1)[0]!=std::byte{};
        if(!(a.gp[0]&255u)){root.route=SourceLiveShadowRootRouteTu15::blocked;continue;}
        if(local_light){root.route=SourceLiveShadowRootRouteTu15::local_light;continue;}
        root.lighting_address=world.call_actual(0x3dc80u,std::array{root.owner});
        if(!root.lighting_address){root.route=SourceLiveShadowRootRouteTu15::no_lighting;continue;}
        root.cached_opacity=real(root.lighting_address+0x1cu);
        const auto& policy=a.floating_point.value();
        NativeTu15PhysicsFrameFp arithmetic{policy.x87_control_word,a.x87_status,policy.mxcsr};
        const auto gates=native_tu15_shadow_gates_32340(memory,
            {std::bit_cast<std::uint32_t>(root.owner_fade),std::bit_cast<std::uint32_t>(root.history_fade),
             word(root.lighting_address+0x1cu),word(object+0x3cu)},arithmetic);
        if(!gates.admitted) {
            root.route=SourceLiveShadowRootRouteTu15::opacity;continue;
        }
        a=world.prepare_source_call();const auto stop=word(a.gp[4]);
        const auto extents=a.gp[4]-0x100u;put(extents,gates.minimum_radius);put(extents+4u,gates.minimum_radius);
        a.gp[4]=extents-24u;put(a.gp[4],stop);
        put(a.gp[4]+4u,root.owner);put(a.gp[4]+8u,object+0x30u);
        put(a.gp[4]+12u,root.lighting_address+0xcu);put(a.gp[4]+16u,extents);put(a.gp[4]+20u,extents+4u);
        world.run_actual(0x31f40u,a);root.extents={real(extents),real(extents+4u)};
        //32459 reads the centre after31F40. The typed prefix publishes the
        // original raw/normalized words in order, through the source owner.
        const NativeTu15ShadowBasisInput basis_input{raw_vector(object+0x30u),raw_vector(root.lighting_address+0xcu),word(extents),word(extents+4)};
        const auto basis=native_tu15_shadow_basis_32340(memory,basis_input,arithmetic,
            [&](auto,auto address,auto bits){put(address,bits);});
        auto& completed=root.frame.emplace();completed.center=value_vector(basis_input.center);
        completed.direction=value_vector(basis.direction);completed.up=value_vector(basis.up);
        completed.transverse_radius=root.extents.x;completed.axial_radius=root.extents.y;
        completed.width=std::bit_cast<float>(basis.width);completed.near_offset=std::bit_cast<float>(basis.near_offset);
        completed.far_offset=std::bit_cast<float>(basis.far_offset);completed.opacity=std::bit_cast<float>(gates.opacity);completed.cone_angle=real(0x48c5e0u);
        a=world.prepare_source_call();const auto projection=a.gp[4]-0x100u;
        a.gp[0]=0x48c5b8u;a.gp[2]=projection;world.run_actual(0xc3960u,a);
        a=world.prepare_source_call(std::array{0x48c5b8u});
        a.gp[1]=0x48c530u;a.gp[2]=projection;world.run_actual(0xc3ab0u,a);
        root.projection.emplace();std::memcpy(root.projection->words.data(),memory.view(projection,124).data(),124);
        const auto tail=native_tu15_shadow_projection_tail_32340(memory,basis_input.center,basis,root.projection->words,arithmetic);
        completed.left=value_vector(tail.left);completed.volume_center=value_vector(tail.volume_center);
        completed.volume_half_extent=value_vector(tail.half_extent);completed.virtual_eye=value_vector(tail.virtual_eye);
        completed.virtual_distance=std::bit_cast<float>(tail.virtual_distance);
        //320A0 consumes the actual first main camera at context+4. The
        // native portal owner already published that completed camera bank.
        SourceFrustumCamera camera;
        std::memcpy(&camera,memory.view(word(0x556e18u)+4u,sizeof(camera)).data(),sizeof(camera));
        const auto& frame=*root.frame;
        const SourceShadowViewBox box{{1.f,{frame.direction,frame.left,frame.up},frame.volume_center},frame.volume_half_extent};
        // Explicit finite-frame comparison control; ordinary rendering always
        // uses the original-differential native value kernel below.
        static const bool reference_view=[] {const auto* v=std::getenv("HALO2_REFERENCE_SHADOW_VIEW");return v&&*v&&*v!='0';}();
        if(reference_view)root.view_volume=source_shadow_view_volume(camera,box);
        else {
            root.view_volume=native_tu15_shadow_view_volume_320a0(memory,camera,box,arithmetic);
            completed_shadow_view(root.view_volume->admitted);
        }
        if(root.view_volume->admitted) {
            // Original327E0 rejects the volume before the secondary camera.
            // The typed nonzero-mode C3DC0 tail consumes its complete native
            // C3AB0 projection; no virtual eye/rectangle is reconstructed.
            put(0x48c5a4u,frame.opacity);put(0x4920dcu,std::uint8_t(0));
            root.secondary_camera=source_shadow_secondary_camera(*root.projection,frame.direction,frame.up);
            // Complete134C00 uses the actual source secondary context and
            // prior main queues;168900 then classifies the retained rows.
            root.query_radius=std::bit_cast<float>(tail.query_radius);
            a=world.prepare_source_call();const auto return_address=word(a.gp[4]);
            const auto inputs=a.gp[4]-0x300u,center=inputs+448u;
            world.write_source_call_input(inputs,std::as_bytes(std::span(&root.secondary_camera->camera,1)));
            put(center,frame.volume_center);
            const auto cluster=std::int32_t(std::bit_cast<std::int16_t>(Bytes(memory.view(object+0x2cu,2)).u16(0)));
            a.gp[4]=inputs-32u;put(a.gp[4],return_address);
            const std::array args{0u,inputs,center,std::bit_cast<std::uint32_t>(root.query_radius),0x556e18u,0u,std::uint32_t(cluster)};
            for(unsigned n=0;n<args.size();++n)put(a.gp[4]+4u+n*4u,args[n]);
            a.gp[0]=2;a.gp[1]=1;a.gp[2]=inputs&0xffffff00u;
            if(observe)observe(true,0x134c00u,root.owner,a,memory);
            world.run_actual(0x134c00u,a);
            if(observe)observe(false,0x134c00u,root.owner,a,memory);
            a=world.prepare_source_call(std::array{root.owner,0u,0u});a.gp[7]=0x5598f8u;
            if(observe)observe(true,0x168900u,root.owner,a,memory);
            world.run_actual(0x168900u,a);
            if(observe)observe(false,0x168900u,root.owner,a,memory);
            auto& queues=root.secondary_queues.emplace();queues.context=0x5598f8u;queues.visibility_bank=word(queues.context);
            queues.pass_mode=word(queues.context+0x2a78u);queues.query_mode=word(queues.context+0x2a7cu);
            const auto rows=[&](std::uint32_t offset,auto& output) {
                const auto header=word(0x5598f8u+offset);
                const auto count=std::bit_cast<std::int16_t>(Bytes(memory.view(header+4u,2)).u16(0));
                if(count<0 || count>Bytes(memory.view(header,2)).u16(0))
                    throw FormatError("TU1.5 secondary shadow queue exceeds its original capacity");
                for(int n=0;n<count;++n)output.push_back({word(word(header+12u)+4u*n),
                    Bytes(memory.view(word(header+8u)+2u*n,2)).u16(0),
                    Bytes(memory.view(word(header+20u)+2u*n,2)).u16(0),word(word(header+16u)+4u*n)});
            };
            rows(12u,queues.clusters);rows(20u,queues.objects);rows(24u,queues.instances);
            const auto append_objects=std::uint32_t(std::to_integer<unsigned>(memory.view(object+0xaau,1)[0])!=1);
            a=world.prepare_source_call(std::array{append_objects});
            if(observe)observe(true,0x2cba0u,root.owner,a,memory);
            NativeTu15PhysicsFrameFp append_fp{a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr};
            const auto append_context=world.collect_render_bsp_units_tu15(append_fp);
            auto dynamic=world.prepare_source_call(std::array{append_objects});
            const auto caller_ebx=a.gp[3],caller_sp=a.gp[4];
            dynamic.gp[0]=append_context;dynamic.gp[3]=append_context;
            for(unsigned reg:{5u,6u,7u})dynamic.gp[reg]=a.gp[reg];
            dynamic.x87_status=append_fp.status;
            dynamic.floating_point=SourcePhysicsFactoryFloatingPoint{append_fp.control_word,append_fp.mxcsr};
            world.run_actual(0x136230u,dynamic);
            a=dynamic;a.gp[3]=caller_ebx;a.gp[4]=caller_sp+8u;
            if(observe)observe(false,0x2cba0u,root.owner,a,memory);
            const auto append_status=a.x87_status;
            const auto append_control=a.floating_point;
            a=world.prepare_source_call();
            a.x87_status=append_status;a.floating_point=append_control;
            if(observe)observe(true,0x44710u,root.owner,a,memory);
            world.run_actual(0x44710u,a);
            if(observe)observe(false,0x44710u,root.owner,a,memory);
            auto& collected=root.collected.emplace();auto& state=collected.state;
            state.main_prefix=word(0x4cd39cu);state.draw_count=word(0x4cd398u);
            state.override_count=word(0x4cdd48u);state.section_highwater=word(0x4ce1ccu);
            if(state.draw_count>850 || state.main_prefix>state.draw_count || state.override_count>192 || state.section_highwater>32)
                throw FormatError("TU1.5 native shadow collector escaped its original draw/override/section banks");
            const auto copy=[&](std::uint32_t address,auto& output,std::size_t count) {
                if(count)std::memcpy(output.data(),memory.view(address,count*sizeof(output[0])).data(),count*sizeof(output[0]));
            };
            copy(0x4c6958u,state.units,state.draw_count);copy(0x4cd3a4u,state.coverage,state.draw_count);
            copy(0x4cd6f6u,state.override_slots,state.draw_count);copy(0x4cda48u,state.overrides,state.override_count);
            copy(0x4cdd4cu,state.sections,state.sections.size());
            copy(collected.main_arena_address,collected.main_arena.words,512);
            copy(collected.secondary_arena_address,state.arena.words,512);
            collected.main_arena.used=Bytes(memory.view(0x556e18u+0xa5cu,2)).u16(0);
            state.arena.used=Bytes(memory.view(0x5598f8u+0xa5cu,2)).u16(0);
            if(!queues.clusters.empty()) {
                // Original328E2/328F5 finish uniforms before3290A builds this
                // root's commands. Preserve that ordering and the actual
                // pre-command feature bit consumed by48ED0; do not infer it
                // from the new command masks. The60 consumed words match
                // unchanged TU1.5 prefixes (shadow-uniform-prefix-original-v2).
                root.shadow_depth_feature=(word(0x4ce2a0u)&(1u<<10))!=0;
                root.uniforms=source_shadow_uniforms(frame,root.shadow_depth_feature);
                //32340's completed CPU caller context for whole40E80. The
                // GPU consumer is still separate; retain and restore the
                // caller overlays without supplying a GPU completion flag.
                const auto saved_mask=word(0x4c6834u),saved_definition=word(0x4d1828u),saved_opacity=word(0x492250u),saved_light_definition=word(0x4920e4u);
                const auto saved_active=memory.view(0x4c6838u,1)[0];
                const auto restore=[&] {
                    put(0x4c6834u,saved_mask);put(0x4c6838u,saved_active);
                    put(0x4d1828u,saved_definition);put(0x492250u,saved_opacity);put(0x4920e4u,saved_light_definition);
                };
                put(0x4c6838u,std::uint8_t(1));put(0x4c6834u,std::uint32_t(0x14));
                put(0x4d1828u,std::uint32_t(0x48c5b8));put(0x492250u,frame.opacity);
                //4A20E retains32340's actual fixed-definition argument.
                //30CF0 consumes this caller binding while selecting Apply.
                put(0x4920e4u,std::uint32_t(0x48c5b8));
                try {
                    a=world.prepare_source_call(); //EAX0 selects all shadow layers.
                    if(observe)observe(true,0x40e80u,root.owner,a,memory);
                    const auto translated=[&]{world.run_actual(0x40e80u,a);};
                    if(commands)commands(a,translated);else translated();
                    if(observe)observe(false,0x40e80u,root.owner,a,memory);
                    auto& banks=root.commands.emplace();
                    for(unsigned n=0;n<banks.size();++n) {
                        auto& bank=banks[n];bank.address=0x4ce280u+n*24u;
                        copy(bank.address,bank.header,bank.header.size());
                        const auto count=bank.header[1];
                        if(bank.header[0]!=std::array{1024u,512u}[n] || count>bank.header[0] || !bank.header[5])
                            throw FormatError("TU1.5 shadow command producer escaped its actual command banks");
                        bank.commands.resize(count);copy(bank.header[5],bank.commands,count);
                    }
                    //32917/3292F require both authored phase masks before
                    //the original per-layer selection/sort and GPU corridor.
                    if((banks[1].header[2]&(1u<<10)) && (banks[0].header[2]&(1u<<11))) {
                        auto& ordered=root.ordered_draws.emplace();
                        ordered[0]=source_live_shadow_ordered_draws_tu15(world,SourceShadowPhase::Generate,
                            0x4ce298u,2u,10u,root.owner,observe);
                        ordered[1]=source_live_shadow_ordered_draws_tu15(world,SourceShadowPhase::Apply,
                            0x4ce280u,1u,11u,root.owner,observe);
                    }
                } catch(...) {restore();throw;}
                restore();
            }

        }
        root.route=SourceLiveShadowRootRouteTu15::projected;
        if(completed_root)completed_root(world,root);
    }
    return result;
}
}
