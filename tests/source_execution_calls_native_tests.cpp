#include "source_match_rebind_fixture.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "source_lens_draw_replay_fixture.hpp"
#include "halo2/source_projectile_effect_observer.hpp"
#include <set>
#include <future>

namespace {
struct QueryDevice final:SourcePhysicsContactMemory {
    SourceRuntimeMemory storage;
    QueryDevice() {
        const std::array<std::byte,4> driver{std::byte{0},std::byte{0},std::byte{0x10},std::byte{0xf7}};
        storage.mount(0x411568u,driver,SourceRuntimeMemory::Access::ReadWrite,"declared lens driver pointer");
        storage.mount(0xf7100000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared lens driver");
        storage.mount(0xf7140000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared pending query metadata");
        put(0xf71007d4u,0xf7140000u);put(0xf710002cu,1);
    }
    void put(std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes;
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));storage.write(p,bytes);}
    void read(std::uint32_t p,std::span<std::byte> out)const override{storage.read(p,out);}
    void write(std::uint32_t p,std::span<const std::byte> in)override{storage.write(p,in);}
};
void equal(Owner &a,Owner &b){
    const auto x=a.authority->checkpoint(binding),y=b.authority->checkpoint(binding);
    check(encode_source_authority(x)==encode_source_authority(y),"Recorded native calls did not preserve the complete canonical coupled world");
}
void restore(Owner &a,Owner &b){auto p=b.authority->prepare(a.authority->checkpoint(binding),binding);
    check(b.authority->commit(std::move(p)),"Native journal repair failed");equal(a,b);}
}
int main(int argc,char **argv)try {
    if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,4> locals{0,1,2,3};
    Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);restore(host,replica);
    using K=SourceExecutionCallKind;
    // Actual source-world scheduling and automatic replay, complementing the
    // independent original-body state fixture in the parent evidence folder.
    struct FrameCase {unsigned ticks;float seconds;};
    for(const auto frame:std::array{FrameCase{0,0.f},FrameCase{0,1.f/120.f},
            FrameCase{1,1.f/30.f},FrameCase{3,.1f}}) {
        const auto recorded=host.authority->capture_execution_phase([&]{
            for(unsigned n=0;n<frame.ticks;++n)host.step({});
            host.world->advance_effect_frame(frame.seconds);
        });
        std::vector<std::uint32_t> owners;
        unsigned particle_calls{};
        for(const auto &call:recorded.calls) {
            if(call.kind==K::CallActualSource)owners.push_back(Bytes(call.arguments).u32(0));
            if(call.kind==K::EffectParticles) {
                owners.push_back(0x179df0u);
                ++particle_calls;
                check(Bytes(call.arguments).u32(0)==std::bit_cast<std::uint32_t>(frame.seconds),
                    "Actual particle journal changed its accepted interval");
            }
        }
        check(owners==(frame.seconds>0?std::vector<std::uint32_t>{0x17ead0u,0x180520u,0x1378e0u,0x179df0u}:std::vector<std::uint32_t>{0x179df0u}) && particle_calls==1,
            "Zero/one/multi-tick native frame repeated or omitted a retained effect owner");
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(recorded)));equal(host,replica);
    }
    std::cout<<"PASS actual source zero/one/multi-tick effect scheduling and automatic complete-world replay\n";
    auto phase=host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);host.world->publish_native_observer_position(0,{3,4,5});});
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{replica.step({});replica.world->advance_effect_frame(1.f/60.f);});});
    rejects([&]{replica.authority->checkpoint(binding);});restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.step({});host.world->advance_effect_frame(1.f/60.f);host.world->publish_native_observer_position(0,{4,5,6});});
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{replica.step({});replica.world->advance_effect_frame(1.f/60.f);replica.world->publish_native_observer_position(0,{4,5,7});});});
    restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{host.world->publish_native_observer_position(0,{5,6,7});host.world->begin_model_frame();});
    rejects([&]{replica.authority->replay_execution_phase(phase,[&]{replica.world->begin_model_frame();});});restore(host,replica);
    std::set<K> kinds;
    for(unsigned frame=0;frame<2;++frame){
        phase=host.authority->capture_execution_phase([&]{
            std::array<Controls,16> controls{};controls[0].fire=frame==0;
            host.step(controls);host.world->advance_effect_frame(1.f/60.f);host.world->begin_model_frame();
            for(unsigned view=0;view<4;++view){
                const auto position=host.match->unit(view).body.position+Vec3{0,0,1.7f};
                host.world->publish_native_observer_position(view,position);
                const auto camera=source_frustum_camera({position,{1,0,0},{0,0,1},.015625f,1024.f,{-1,1,-1,1}});
                SourcePortalVisibilitySet visibility;visibility.groups.push_back(camera);visibility.frustums.push_back(camera.full_frustum);
                SourceClusterFrustumRanges cluster;cluster.source_cluster=0;cluster.counts[0]=1;cluster.first_indices[0]=0;
                visibility.clusters.push_back(cluster);visibility.sorted_cluster_indices.push_back(0);
                host.world->publish_render_visibility(visibility);host.match->advance_weather_view(1.+frame/30.,std::int16_t(view));
                host.world->render_model_view([&](auto &world){
                    (void)world.build_world_model(host.match->unit(view).source_datum,view,4,false,10.f,1.f);
                    (void)world.prepare_first_person_models(view,position,{1,0,0},{0,0,1});
                });
            }
            host.match->advance_audio(1.f/60.f);(void)host.match->take_sounds();(void)host.match->take_prop_events();(void)host.match->take_grenade_events();
        });
        for(const auto &call:phase.calls)kinds.insert(call.kind);
        check(phase.calls.size()>20,"Native multiview fixture did not record the actual outside-tick calls");
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    }
    check(kinds.size()==14 && kinds.contains(K::CallActualSource),"Native multiview fixture omitted a declared journal operation");
    auto malformed=phase;malformed.calls.back().arguments.push_back(std::byte{1});
    rejects([&]{replica.authority->replay_execution_phase(malformed);});equal(host,replica);
    auto before=host.match->simulation().ticks();
    rejects([&]{host.authority->capture_execution_phase([&]{host.step({});host.world->tu15_world().step_objects();});});
    check(host.match->simulation().ticks()==before+1,"An unsupported call interrupted the actual host");
    // The unsupported direct source tick changes the source/Match pair, so
    // repair the host through the last complete coupled cut before continuing.
    auto p=host.authority->prepare(replica.authority->checkpoint(binding),binding);check(host.authority->commit(std::move(p)),"Unsupported host work could not repair");equal(host,replica);
    rejects([&]{host.authority->capture_execution_phase([&]{host.match->debug_settings({});});});equal(host,replica);
    {
        auto recording=host.authority->start_execution_phase();
        rejects([&]{host.authority->checkpoint(binding);});
        rejects([&]{host.authority->start_execution_phase();});
        auto foreign=std::async(std::launch::async,[&]{rejects([&]{recording.finish();});});foreign.get();
        host.step({});host.world->advance_effect_frame(1.f/60.f);
        // A real frame tail remains in the same owning phase after the tick.
        host.world->publish_native_observer_position(3,{8,9,10});host.world->begin_model_frame();
        phase=recording.finish();rejects([&]{recording.finish();});
    }
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    {
        auto cancelled=host.authority->start_execution_phase();host.world->publish_native_observer_position(1,{11,12,13});
    }
    // Scope cancellation releases binding and does not revoke the real host.
    restore(host,replica);
    const auto limited_before=host.match->simulation().ticks();
    {auto limited=host.authority->start_execution_phase(1);host.step({});rejects([&]{limited.finish();});}
    check(host.match->simulation().ticks()==limited_before+1,"Transcript limit interrupted or repeated real host work");restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{(void)host.match->state();host.step({});(void)host.match->state();});
    check(phase.calls.front().kind==K::MatchState&&phase.calls.back().kind==K::MatchState,
        "Actual native HUD/state queries were omitted from the journal");
    auto bad_state=phase;bad_state.calls.front().result[0]^=std::byte{1};
    rejects([&]{replica.authority->replay_execution_phase(bad_state);});restore(host,replica);
    phase=host.authority->capture_execution_phase([&]{(void)host.match->state();host.step({});(void)host.match->state();});
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    NativeVehicleCamera camera(*host.world,*host.cache.original_data());
    phase=host.authority->capture_execution_phase([&]{for(unsigned view=0;view<4;++view){const auto &p=host.match->player(view+4);
        (void)camera.update_player(view,p.source_datum,p.source_flags,1.f/30.f,view==2);}});
    check(std::any_of(phase.calls.begin(),phase.calls.end(),[](const auto &c){return c.kind==K::PlayerCamera;}),
        "Actual private-view camera scheduler was omitted from the journal");
    auto bad_camera=phase;const auto bad=std::find_if(bad_camera.calls.begin(),bad_camera.calls.end(),[](const auto &c){return c.kind==K::PlayerCamera;});
    bad->result[0]^=std::byte{1};rejects([&]{replica.authority->replay_execution_phase(bad_camera);});restore(host,replica);
    for(unsigned frame=0;frame<3;++frame){
        phase=host.authority->capture_execution_phase([&]{host.step({});for(unsigned view=0;view<4;++view){const auto &p=host.match->player(view+4);
            (void)camera.update_player(view,p.source_datum,p.source_flags,1.f/30.f,frame==1&&view==2);}});
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    }
    auto malformed_camera=phase;auto first=std::find_if(malformed_camera.calls.begin(),malformed_camera.calls.end(),[](const auto &c){return c.kind==K::PlayerCamera;});
    first->arguments.push_back(std::byte{0});rejects([&]{validate_source_execution_calls(malformed_camera.calls);});
    // Mode output can be absent; all actual packet/controller/observer/random
    // outputs still participate in the return seal.
    SourceCameraControllerState controller{};SourceObserverState observer{};std::uint32_t random=12345;
    source_observer_reset(observer,0,*host.cache.original_data()->observer_constants,*host.cache.original_data()->observer_world_constants);
    phase=host.authority->capture_execution_phase([&]{(void)host.world->player_camera_packet(host.match->player(4).source_datum,
        1.f/30.f,controller,observer,random,false,nullptr);});
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    QueryDevice device;
    phase=host.authority->capture_execution_phase([&]{
        auto &world=host.world->tu15_world();auto issue=world.prepare_source_call();issue.gp[0]=0;
        source_execute_lens_query(world,0x402650u,issue,&device);
        const std::array<std::uint32_t,1> args{0x60030200u};auto poll=world.prepare_source_call(args);poll.gp[0]=0;poll.gp[2]=0;
        source_execute_lens_query(world,0x402710u,poll,&device);
        source_finish_lens_query(world,true,{});
        const std::array<SourceLensQueryRollback,1> rollback{{{0,word(host.world->memory(),0x52dfdcu+6u,2),bool(word(host.world->memory(),0x531fc0u)&1u)}}};
        source_finish_lens_query(world,false,rollback);
    });
    check(std::any_of(phase.inputs.begin(),phase.inputs.end(),[](const auto &input){return input.kind==SourceExecutionInputKind::GraphicsRead4;}),
        "Actual original query did not retain declared device operands");
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    auto bad_lens=phase;auto lens=std::find_if(bad_lens.calls.begin(),bad_lens.calls.end(),[](const auto &c){return c.kind==K::LensQuery;});
    lens->result.back()^=std::byte{1};rejects([&]{replica.authority->replay_execution_phase(bad_lens);});restore(host,replica);
    test::SourceLensDrawReplayFixture draw_device(host.world->tu15_world(),host.cache);
    phase=host.authority->capture_execution_phase([&]{auto &world=host.world->tu15_world();
        const std::array<std::uint32_t,2> args{0,draw_device.header};auto abi=world.prepare_source_call(args);abi.gp[0]=draw_device.metadata;
        check(!source_execute_lens_draw(world,0x1301e0u,abi,false,draw_device.domains,draw_device),"Original header constructor invented a world pass");});
    check(Bytes(draw_device.storage.view(draw_device.header+4,4)).u32(0)==(draw_device.alias&0x0fffffffu),"Original native texture header lost its resident alias");
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    auto bad_draw=phase;auto draw=std::find_if(bad_draw.calls.begin(),bad_draw.calls.end(),[](const auto &c){return c.kind==K::LensDraw;});
    draw->result[draw->result.size()-2]^=std::byte{1};rejects([&]{replica.authority->replay_execution_phase(bad_draw);});restore(host,replica);
    const auto old_type=host.world->memory().view(0x491e22u,2);const std::vector<std::byte> old_view(old_type.begin(),old_type.end());
    phase=host.authority->capture_execution_phase([&]{auto &world=host.world->tu15_world();const std::array<std::byte,2> mode{std::byte{2},std::byte{0}};
        world.write_source_call_input(0x491e22u,mode);auto abi=world.prepare_source_call();abi.gp[3]=word(host.world->memory(),0x491e27u,1);
        const auto pass=source_execute_lens_draw(world,0x13660u,abi,true,draw_device.domains,draw_device);
        check(pass&&*pass==std::array<std::uint32_t,6>{1,1,0,0,0,0},"Original parent lost the actual six-argument world-pass boundary");
        world.write_source_call_input(0x491e22u,old_view);});
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    auto bad_pass=phase;auto gate=std::find_if(bad_pass.calls.begin(),bad_pass.calls.end(),[](const auto &c){return c.kind==K::LensDraw;});
    gate->result.back()^=std::byte{1};rejects([&]{replica.authority->replay_execution_phase(bad_pass);});restore(host,replica);
    for(const auto regions:std::array{std::array<SourceLensDeviceRegion,2>{{{0x1000,16},{0x1008,16}}},
        std::array<SourceLensDeviceRegion,2>{{{0x2000,16},{0x1000,16}}},std::array<SourceLensDeviceRegion,2>{{{0x1000,0},{0x2000,16}}},
        std::array<SourceLensDeviceRegion,2>{{{0x1000,16},{0xfffffff0u,32}}}})
        rejects([&]{validate_source_lens_device_regions(regions);});
    unsigned collision_hits{},collision_misses{};
    phase=host.authority->capture_execution_phase([&]{for(unsigned slot=0;slot<8;++slot){
        const auto origin=host.match->unit(slot).body.position+Vec3{0,0,2.f};
        collision_hits+=bool(host.world->particle_collision(0x800005u,origin,{0,0,-100.f}));
        collision_misses+=!host.world->particle_collision(0,origin,{0,0,1.f});
    }});
    check(collision_hits&&collision_misses==8,"Native collision fixture did not exercise actual hits and misses");
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    auto bad_collision=phase;auto hit=std::find_if(bad_collision.calls.begin(),bad_collision.calls.end(),[](const auto &c){return c.kind==K::ParticleCollision&&c.result.size()>1;});
    hit->result.back()^=std::byte{1};rejects([&]{replica.authority->replay_execution_phase(bad_collision);});restore(host,replica);
    const auto reject_fallback=[](const Unit&,const WeaponEffectInput&)->std::uint32_t{throw FormatError("TU1.5 source firing invoked the software presentation fallback");};
    host.match->configure_weapon_effect_owner(reject_fallback);replica.match->configure_weapon_effect_owner(reject_fallback);
    restore(host,replica);const auto loaded_before=host.match->unit(0).weapon.loaded;bool scheduled_effect{};
    for(unsigned frame=0;frame<30;++frame){phase=host.authority->capture_execution_phase([&]{
        std::array<Controls,16> controls{};controls[0].fire=true;host.step(controls);
        const auto state=host.match->state();scheduled_effect|=std::any_of(state.weapon_effects.begin(),state.weapon_effects.end(),[](const auto &e){return e.source_scheduled;});
    });replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));}
    equal(host,replica);check(host.match->unit(0).weapon.loaded<loaded_before&&scheduled_effect,
        "Original firing did not retain real ammunition use and scheduled effect publication");
    host.match->configure_weapon_effect_owner({});replica.match->configure_weapon_effect_owner({});equal(host,replica);
    phase=host.authority->capture_execution_phase([&]{check(host.match->depart(7),"Actual departure was not admitted");host.step({});});
    replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));equal(host,replica);
    check(phase.calls.front().kind==K::DepartPlayer&&host.match->player(7).source_flags&2u,"Actual departure was omitted from the journal");
    check(host.world->has_projectile_effect_observer()&&replica.world->has_projectile_effect_observer(),"Source Match omitted its real projectile journal binding");
    // This metadata rejection performs no game tick; freeze the declared
    // probe-on-read clock so taking the extra source cut cannot advance it.
    host.clock->cycle_stride=replica.clock->cycle_stride=0;
    auto missing_contact=host.authority->checkpoint(binding);missing_contact.match.service_roles^=1u<<10;
    rejects([&]{replica.authority->prepare(missing_contact,binding);});equal(host,replica);
    replica.authority.reset();replica.match.reset();
    check(!replica.world->has_projectile_effect_observer(),"Projectile callback survived its Match target");
    {SourceProjectileEffectObserver retired(replica.world->memory(),[](const SourceProjectileDetonationTu15&){});}
    std::cout<<"PASS actual TU1.5 owning source call journal: omitted/changed/reordered calls rejected, native four-view models/visibility/weather/effects/audio/events replayed without a caller closure, complete canonical worlds, mutation coverage, repair and original departure\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
