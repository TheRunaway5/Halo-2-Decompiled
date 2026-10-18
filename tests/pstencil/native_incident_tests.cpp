#include "halo2/pstencil/native_incident.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/pstencil/lightmap.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_bsp_admission.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <unistd.h>
using namespace halo2;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
int main(int argc,char**argv){try {
    require(argc==3,"Supply TU1.5 gameplay image and current Cairo cache");Cache cache(argv[2]);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t){
        return t.group=="bipd" && t.name=="objects\\characters\\masterchief\\masterchief";
    });require(tag!=cache.tags().end(),"Cinematic Chief fixture is absent");
    const auto files=std::filesystem::temp_directory_path()/("pstencil-incident-"+std::to_string(getpid()));
    PhysicsLiveWorldTu15 world(cache,argv[1],files);
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    world.preload_render_model(cache.load_initial_model_definition(tag->datum).render_model);world.initialize_map();
    // Explicit resource fixture, not a visibility result. Production supplies
    // the actual main-camera admission instead of this complete resource set.
    const auto geometry=cache.load_source_bsp_admission(0);SourceBspAdmission resident;
    for(unsigned n=0;n<geometry.clusters.size();++n)resident.clusters.push_back({n});
    for(unsigned n=0;n<geometry.instances.size();++n)resident.instances.push_back({n});
    pstencil::prepare_native_incident_geometry_tu15(world,resident);
    // Same real Cairo floor used by the retained native armor photograph.
    // The campaign spawn is not a substitute for a surface-lit actor position.
    const auto object=world.create_biped(tag->datum,{59.1033f,-12.3787f,5.01502f},{1,0,0});
    require(object!=0xffffffffu,"Native cinematic fixture constructor declined");
    const auto body=world.source_object_body(object);
    // Declared native ABI input: set only the actual cinematic-lighting flag.
    auto flags=Bytes(world.memory().view(body+4,4)).u32(0)|0x01000000u;
    world.write_source_call_input(body+4,std::as_bytes(std::span(&flags,1)));
    world.begin_model_frame();world.begin_model_view();
    (void)world.build_world_model(object,0,1,false,1,1);
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const auto flags_before=word(body+4);
    require(flags_before&0x01000000u,"Native cinematic collector lost the declared cinematic flag");
    const auto token=word(body+0xcc),record=word(word(0x515f14u)+0x44)+(token&65535u)*256u;
    require(!std::to_integer<unsigned>(world.memory().view(record+2,1)[0]),"Cinematic collector unexpectedly supplied a baked-light sample");
    const auto global=word(0x4eabc4u)+0x1cu;
    const auto default_before=std::vector<std::byte>(world.memory().view(global,84).begin(),world.memory().view(global,84).end());
    auto incident=pstencil::native_incident_tu15(world,object,1);
    require(!incident,"New airborne cinematic fixture unexpectedly has an accepted surface sample");
    require(word(body+4)==flags_before,"pStencil incident changed the native cinematic flag");
    require(std::ranges::equal(default_before,world.memory().view(global,84)),"pStencil incident changed visible cinematic lighting");
    // The original updater retains a failed query for its native retry interval.
    // Advance actual object simulation/render frames; do not publish a ready
    // bit, inject a surface result or bypass its timing to make the test pass.
    unsigned accepted_frame=0;
    for(unsigned tick=0;tick<90 && !incident;++tick) {
        world.step_objects();world.begin_model_frame();world.begin_model_view();
        const auto live_flags=word(body+4);
        const auto visible=std::vector<std::byte>(world.memory().view(global,84).begin(),world.memory().view(global,84).end());
        incident=pstencil::native_incident_tu15(world,object,1);
        require(word(body+4)==live_flags,"pStencil incident changed live object flags");
        require(std::ranges::equal(visible,world.memory().view(global,84)),"pStencil incident changed live cinematic lighting");
        if(incident)accepted_frame=word(0x4c6854u);
    }
    require(bool(incident),"Actual cinematic baked-light updater produced no valid incident sample after native simulation/retry");
    require(incident->owner==object && incident->cache==token,"Cinematic query changed native ownership");
    require(incident->sample.incident.z>0,"Known Cairo floor returned an initialized/fallback incident");
    require(word(body+4)&0x01000000u,"Native simulation lost the fixture cinematic flag");
    std::cout<<"Native incident accepted on actual render frame "<<accepted_frame<<'\n';
    const auto origin=pstencil::lightmap_light_origin(incident->sample.incident);
    std::cout<<"PASS native cinematic incident="<<incident->sample.incident.x<<','<<incident->sample.incident.y<<','<<incident->sample.incident.z
        <<" far_origin="<<origin.x<<','<<origin.y<<','<<origin.z<<"; actual updater, retained cinematic flag/default lighting\n";
    const auto spotlight=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& row) {
        if(row.group!="ligh")return false;
        const auto data=cache.load_source_tag_metadata(row.datum);
        return Bytes(data.bytes_at(data.source_address(),6)).u16(4)==2;
    });require(spotlight!=cache.tags().end(),"Current Cairo has no authored frustum light fixture");
    const std::array light_args{spotlight->datum,0xffffffffu,0xffffu,body+0x7cu};
    auto light_call=world.prepare_source_call(light_args);light_call.gp[3]=body+0x64u;light_call.gp[7]=body+0x70u;
    light_call.xmm[0][0]=std::bit_cast<std::uint32_t>(1.f);world.run_actual(0xc2380u,light_call);
    require(light_call.gp[0]!=0xffffffffu,"Original constructor declined the authored frustum fixture");
    const auto shape=pstencil::native_light_shape_tu15(world,light_call.gp[0]);
    require(shape.type()!=0,"Original frustum producer became a sphere");
    for(unsigned byte=0x64;byte<0x70;++byte)require(shape.written[byte],"Original frustum extents were not actually written");
    require(std::isfinite(std::bit_cast<float>(shape.words[25])) && std::bit_cast<float>(shape.words[25])>0,
        "Original frustum has no finite positive depth");
    const auto cameras=pstencil::native_light_cameras_tu15(world,light_call.gp[0]);
    require(cameras.size()==1,"Native frustum became a six-camera sphere query");
    require(cameras.front().far_distance>0,"Native frustum camera lost its authored far plane");
    std::array<ShaderVector,9> previous{};
    for(unsigned row=0;row<previous.size();++row)for(unsigned component=0;component<4;++component)
        previous[row][component]=float(row*4+component)*.03125f;
    const auto constants=pstencil::native_light_constants_tu15(world,light_call.gp[0],{1,1,1,1,1},1,previous);
    for(const auto& row:constants.constants)for(float value:row)require(std::isfinite(value),"Native frustum constant is not finite");
    require(constants.constants[8]==previous[8],"Native frustum changed its retained C39 words");
    const auto light_record=word(word(0x4ecbdcu)+0x44u)+(light_call.gp[0]&65535u)*272u;
    const auto unready_fade=word(light_record+0x100u);
    if(std::bit_cast<float>(unready_fade)==0) {
        require(!pstencil::native_light_render_admission_tu15(world,light_call.gp[0]),
            "Projected admission manufactured a ready per-view fade");
        require(word(light_record+0x100u)==unready_fade,"Projected admission changed its original unready fade");
    }
    std::cout<<"PASS original frustum constants shift="<<constants.color_shift<<" origin="<<constants.constants[0][0]<<','
        <<constants.constants[0][1]<<','<<constants.constants[0][2]<<"; complete49790, preserved C39\n";
    std::cout<<"PASS actual frustum shape type="<<shape.type()<<" depth="<<std::bit_cast<float>(shape.words[25])
        <<" from "<<spotlight->name<<"; complete native parameter/transform producers\n";
    const auto receiver=pstencil::native_light_receiver_tu15(world,light_call.gp[0]);
    require(receiver.view.pass_mode==1 && receiver.view.visibility.groups.size()==1,
        "Original projected receiver changed its pass/camera count");
    require(!std::memcmp(receiver.view.visibility.groups.data(),cameras.data(),sizeof(SourceFrustumCamera)),
        "Original projected query changed native camera bytes");
    require(std::ranges::any_of(receiver.objects.objects,[&](const auto& row){return row.id==object;}),
        "Original projected receiver lost its actual dynamic biped");
    const auto geometry_frame=pstencil::native_light_geometry_tu15(world,receiver);
    require(geometry_frame.retained.state.draw_count>=geometry_frame.retained.state.main_prefix,
        "Original projected collection lost its retained main prefix");
    std::cout<<"PASS whole native projected receiver/collection: mode="<<receiver.view.query_mode
        <<" objects="<<receiver.objects.objects.size()<<" draw_units="<<geometry_frame.retained.state.draw_count<<'\n';
    std::filesystem::remove_all(files);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
