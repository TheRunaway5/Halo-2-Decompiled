#include "halo2/source_campaign_water_weather_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_weather_execution.hpp"
#include "halo2/source_weather_rsqrt.hpp"
#include "halo2/source_weather_sprite_profile.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t at) {
    return Bytes(memory.view(at,4)).u32(0);
}
std::int16_t short_at(const Bytes &bytes,unsigned at) {
    return std::bit_cast<std::int16_t>(bytes.u16(at));
}
Vec3 vector_at(const Bytes &bytes,unsigned at) {
    return {bytes.f32(at),bytes.f32(at+4u),bytes.f32(at+8u)};
}
// Synchronous borrow of the genuine source auxiliary owner. No seed or
// persistent copy is introduced, and interleaved source consumers cannot have
// their word overwritten by a late commit. Preserve consumed draws on failure.
template<class F> auto with_auxiliary(PhysicsLiveWorldTu15 &world,F &&operation) {
    auto &memory=world.memory();
    const auto random=word(memory,0x4f3cd8u);
    if(!random || random>std::numeric_limits<std::uint32_t>::max()-8u)
        throw FormatError("Campaign weather lost its complete actual auxiliary random owner");
    const Bytes source(memory.view(random,8));
    const auto before=source.u32(4);auto value=before;
    const auto commit=[&] {
        if(word(memory,0x4f3cd8u)!=random || word(memory,random+4u)!=before)
            throw FormatError("Campaign weather auxiliary owner changed during synchronous consumption");
        if(value!=before)world.write_source_call_input(random+4u,std::as_bytes(std::span(&value,1)));
    };
    auto result=[&] {
        try {return operation(value);}
        catch(...) {commit();throw;}
    }();
    commit();return result;
}
}
SourceCampaignWaterWeatherTu15::SourceCampaignWaterWeatherTu15(const Cache &cache,std::size_t bsp)
    :cache_(cache),bsp_(bsp),water_(cache.load_bsp_water(bsp)),weather_(cache,bsp) {}
SourceCampaignWaterMaterialBindingsTu15 SourceCampaignWaterWeatherTu15::append_water_materials(Mesh &mesh)const {
    std::vector<unsigned> bases;
    for(unsigned d=0;d<water_.definitions.size();++d) {
        const auto &definition=water_.definitions[d];
        if(const auto tag=cache_.query_tag({.handle=TagHandle{definition.shader}}))
            for(auto &material:mesh.materials)
                if(material.water_pass && material.name==tag->name)material.water_definition=int(d);
        bases.push_back(unsigned(mesh.materials.size()));
        for(auto material:definition.geometry.materials) {
            material.water_definition=int(d);mesh.materials.push_back(std::move(material));
        }
    }
    return {this,std::move(bases),mesh.materials.size()};
}
void SourceCampaignWaterWeatherTu15::bind_water_pools(std::span<Material> materials)const {
    bind_water_pool_materials(water_,materials);
}
void SourceCampaignWaterWeatherTu15::require_bsp(PhysicsLiveWorldTu15 &world)const {
    const auto &memory=world.memory();const auto bsp=word(memory,0x4ecc18u);
    if(world.source_bsp_index()!=bsp_)
        throw FormatError("Campaign water/weather owner differs from the actual native selected BSP ordinal");
    const auto &source=weather_.bsp();
    if(!bsp || word(memory,bsp+0x84u)!=source.palette.size() ||
        word(memory,bsp+0x9cu)!=source.cluster_palette.size() ||
        word(memory,bsp+0x214u)!=water_.definitions.size())
        throw FormatError("Campaign water/weather resources differ from the actual current BSP");
    const auto palette=word(memory,bsp+0x88u),clusters=word(memory,bsp+0xa0u);
    for(unsigned i=0;i<source.palette.size();++i)
        if(word(memory,palette+i*0x88u+0x24u)!=source.palette[i].datum)
            throw FormatError("Campaign weather palette lost its actual typed source owner");
    for(unsigned i=0;i<source.cluster_palette.size();++i)
        if(short_at(Bytes(memory.view(clusters+i*176u+0x76u,2)),0)!=source.cluster_palette[i])
            throw FormatError("Campaign weather cluster assignment differs from its actual source BSP");
    const auto water=word(memory,bsp+0x218u);
    for(unsigned i=0;i<water_.definitions.size();++i) {
        const auto row=water+i*0xacu;
        if(word(memory,row+4u)!=water_.definitions[i].shader)
            throw FormatError("Campaign water definition lost its actual shader identity");
        const auto raw=memory.view(row+52u,120u);
        if(std::memcmp(raw.data(),water_.definitions[i].parameters.data()+13,120u))
            throw FormatError("Campaign water definition differs from its complete actual authored real bank");
    }
}
void SourceCampaignWaterWeatherTu15::initialize(PhysicsLiveWorldTu15 &world) {
    if(memory_owner_ || weather_.initialized())throw FormatError("Campaign weather BSP constructors were repeated");
    require_bsp(world);SourceWeatherExecution arithmetic;
    with_auxiliary(world,[&](auto &random){weather_.initialize(random);return true;});
    memory_owner_=&world.memory();
}
void SourceCampaignWaterWeatherTu15::activate_after_bsp_switch(PhysicsLiveWorldTu15 &world,
    SourceCampaignWaterWeatherStateTu15 state) {
    if(memory_owner_ && memory_owner_!=&world.memory())
        throw FormatError("Campaign weather activation cannot move retained resources between source Worlds");
    require_bsp(world);SourceWeatherExecution arithmetic;
    with_auxiliary(world,[&](auto &random){weather_.initialize(random);return true;});
    // Exact36AA0 order: constructors precede its global stores. It does not
    // clear the separate elapsed scalar or checkpointed command record.
    state.state=SourceWeatherState{};state.clock.previous=0.f;
    memory_owner_=&world.memory();frame_.reset();
}
void SourceCampaignWaterWeatherTu15::restore_binding(PhysicsLiveWorldTu15 &world)const {
    if(memory_owner_!=&world.memory() || !weather_.initialized())
        throw FormatError("Campaign weather restore lacks the previously initialized same-World BSP resources");
    require_bsp(world);
}
void SourceCampaignWaterWeatherTu15::begin_frame(PhysicsLiveWorldTu15 &world,std::uint64_t frame) {
    if(memory_owner_!=&world.memory() || !weather_.initialized())
        throw FormatError("Campaign water/weather frame lacks its actual initialized source World");
    if(frame_ && frame<=*frame_)throw FormatError("Campaign water/weather frame was repeated or reversed");
    require_bsp(world);weather_.begin_frame();frame_=frame;
}
SourceCampaignWaterWeatherFrameTu15 SourceCampaignWaterWeatherTu15::collect_view(
    PhysicsLiveWorldTu15 &world,SourceCampaignWaterWeatherStateTu15 state,const SourceCampaignWaterWeatherViewTu15 &input,
    const SourceCampaignWaterMaterialBindingsTu15 &materials) {
    if(!frame_ || memory_owner_!=&world.memory())
        throw FormatError("Campaign water/weather view lacks its same completed source World frame");
    if(materials.owner_!=this || materials.bases_.size()!=water_.definitions.size())
        throw FormatError("Campaign water view lost its same-BSP scene material registration");
    require_bsp(world);auto &memory=world.memory();
    if(!std::ranges::equal(memory.view(0x491e20u,0x298),input.submitting_packet))
        throw FormatError("Campaign water/weather view differs from its actual132B0 publication");
    const Bytes packet(input.submitting_packet);
    if(std::memcmp(input.submitting_packet.data()+0x8cu,&input.submitting_rendering,sizeof(input.submitting_rendering)) ||
        std::memcmp(memory.view(0x4c6634u,sizeof(input.collection_rendering)).data(),
            &input.collection_rendering,sizeof(input.collection_rendering)))
        throw FormatError("Campaign water/weather lost its separate actual camera1/camera2 projection banks");
    const auto slot=short_at(packet,0);
    const auto source_views=std::bit_cast<std::int32_t>(word(memory,0x4c686cu));
    const auto views=std::int16_t(std::clamp(source_views,1,4));
    if(slot<0 || slot>=views)throw FormatError("Campaign weather has no actual owned viewport partition");
    const std::array<std::int16_t,4> rectangle{short_at(packet,0x48u),short_at(packet,0x4au),
        short_at(packet,0x4cu),short_at(packet,0x4eu)};
    const auto width=int(rectangle[3])-int(rectangle[1]),height=int(rectangle[2])-int(rectangle[0]);
    if(width<=0 || height<=0)throw FormatError("Campaign water/weather source rectangle is empty or reversed");
    const auto eye=vector_at(packet,0x18u);
    //3CA80 reads the camera2 eye but camera1's globally published forward.
    // Weather simulation and391E0 consume camera2 from the raw packet below.
    const auto water_forward=vector_at(Bytes(memory.view(0x4c65ccu,12)),0);
    const float angle=packet.f32(0x40u),near=packet.f32(0x58u),far=packet.f32(0x5cu);
    const Bytes configured(memory.view(0x4922f4u,8));const Vec2 projection{configured.f32(0),configured.f32(4)};
    if(!std::isfinite(angle) || !std::isfinite(near) || !std::isfinite(far) ||
       !std::isfinite(projection.x) || !std::isfinite(projection.y))
        throw FormatError("Campaign water/weather camera contains exceptional source operands");
    SourceCampaignWaterWeatherFrameTu15 frame;frame.source_view=slot;
    frame.source_user=std::bit_cast<std::int32_t>(word(memory,0x4c66f8u));
    std::memcpy(&frame.source_realtime,memory.view(0x4920c0u,8).data(),8);
    if(!std::isfinite(frame.source_realtime))throw FormatError("Campaign weather lost its admitted source render clock");
    // Original36B40 reads this same published clock for every submitting
    // viewport. No game tick or second independent elapsed accumulator runs.
    source_weather_view_tick(state.state,state.clock,frame.source_realtime,slot,&state.record);
    frame.source_elapsed=state.clock.elapsed;
    bool wide=memory.view(0x4922e2u,1)[0]!=std::byte{};
    if(const auto context=word(memory,0x4f3218u)) {
        const Bytes current(memory.view(context,12));
        if(current.slice(1,1)[0]!=std::byte{} && short_at(current,2)!=-1 && current.u32(8)==3u)wide=false;
    }
    frame.water.reserve(water_.definitions.size());
    for(unsigned d=0;d<water_.definitions.size();++d) {
        const auto &definition=water_.definitions[d];
        frame.water.push_back(water_frame(definition,eye,water_forward,angle,float(width)/float(height),wide));
        if(!definition.geometry.materials.empty())for(auto vertex:definition.geometry.vertices) {
            vertex.position=water_world_position(frame.water.back(),vertex.position);
            vertex.material+=materials.bases_.at(d);
            if(vertex.material>=materials.material_count_)
                throw FormatError("Campaign water vertex escapes its registered scene material table");
            frame.water_vertices.push_back(vertex);
        }
    }
    if(!water_.definitions.empty()) {
        frame.reflection_eye=eye;frame.reflection_eye->z=2.f*water_.definitions.front().parameters[33]-eye.z;
    }
    SourceWeatherExecution arithmetic;
    weather_.admit_view(input.submitting_packet.first<0xf4>(),input.atmosphere,input.fog.cluster,
        input.visibility,state.state);
    const auto depth=menu_world_depth_clip_planes({projection.x,projection.y,MenuWorldDepthBoundary::none,0,true});
    const ShaderVector clip_range{0,16777215.f,depth.minimum_w,depth.maximum_w};
    const auto mode=std::bit_cast<std::int32_t>(word(memory,0x4920d8u));
    if(mode<0)throw FormatError("Campaign weather has a negative source raster mode");
    SourceWeatherWorldDrawServices draws;
    draws.plate=[&](std::uint32_t datum,std::size_t index,std::span<const std::byte> plate,
        std::span<const std::byte>,unsigned submitting_slot) {
        auto submission=source_weather_plate_submission({plate,std::int16_t(submitting_slot),rectangle,
            {near,far},{projection.x,projection.y}});
        if(submission.result.profile==SourceWeatherPlateCallerProfile::NoDraw)return true;
        const auto &owner=weather_.definition(datum).definition;
        auto &draw=frame.weather_plates.emplace_back();draw.vertex_program=owner.vertex_programs[0];
        draw.submission=std::move(submission);draw.clip_range=clip_range;draw.rectangle=rectangle;
        for(unsigned layer=0;layer<3;++layer) {
            const auto bitmap=owner.plates.at(index).textures[layer];
            if(!layers_.contains(bitmap)) {
                const auto texture=cache_.load_bitmap(bitmap,0);
                if(texture.format!=16 || texture.layers!=1 || texture.depth)
                    throw FormatError("Campaign weather plate lacks its authored2D DXT45 resource");
                layers_.emplace(bitmap,prepare_xemu_bc3_texture(texture));
            }
            draw.layers[layer]=&layers_.at(bitmap);
        }
        return true;
    };
    draws.particle=[&](std::uint32_t datum,std::size_t index,SourceWeatherParticleResource &resource,
        float elapsed,std::int16_t view_count,std::int16_t submitting_slot) {
        const auto &owner=weather_.definition(datum).definition;
        const auto immediate=resource.submission(elapsed,view_count,submitting_slot);
        const auto camera=[&](auto &draw) {
            draw.vertex_program=owner.vertex_programs[1];
            draw.camera=source_view_shader_bank(input.submitting_rendering,far,eye,
                {float(rectangle[1]),float(rectangle[0]),float(width),float(height),1,false,false,unsigned(mode)},false);
            const auto viewport=source_weather_viewport_constants(rectangle[1],rectangle[0],rectangle[3],rectangle[2],mode);
            std::copy(viewport.begin(),viewport.end(),draw.camera.begin()+10);draw.clip_range=clip_range;
        };
        if(immediate.lines) {
            auto &draw=frame.weather_lines.emplace_back();camera(draw);
            draw.pixel=source_weather_pixel_program(true);draw.line_width=immediate.line_width;
            for(const auto &vertex:immediate.vertices)draw.vertices.push_back({vertex.position,vertex.color});
        } else {
            const auto bitmap=owner.particles.at(index).sprites;
            if(!sprites_.contains(bitmap)) {
                auto texture=cache_.load_bitmap(bitmap,0);
                if(!source_weather_sprite_resource_profile(texture.width,texture.height,unsigned(texture.mips.size()),
                    texture.format,texture.layers,texture.depth))
                    throw FormatError("Campaign weather point lacks its admitted complete authored sprite profile");
                sprites_.emplace(bitmap,std::move(texture));
            }
            auto &draw=frame.weather_points.emplace_back();camera(draw);
            draw.pixel=source_weather_pixel_program(false);draw.sprite=&sprites_.at(bitmap);
            for(const auto &point:immediate.vertices)draw.points.push_back({point.position,point.color});
        }
        return true;
    };
    frame.weather_step=with_auxiliary(world,[&](auto &random) {
        frame.auxiliary_before=random;
        const auto result=weather_.visit_view(unsigned(slot),state.state,
            {views,frame.source_elapsed,0x023f,0x1f80,[](std::size_t,std::uint32_t speed) {
                return source_weather_rsqrt_xemu_0_8_136(speed);
            }},random,draws);
        frame.auxiliary_after=random;return result;
    });
    return frame;
}
}
