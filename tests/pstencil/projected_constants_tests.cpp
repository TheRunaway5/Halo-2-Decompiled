#include "halo2/effect_light_draw.hpp"
#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/projected_light_material.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <unistd.h>
using namespace halo2;
void require(bool b,const char*s){if(!b)throw FormatError(s);}
int main(int argc,char**argv){try{
 require(argc==3,"Supply native TU1.5 image and opening cache");Cache cache(argv[2]);
 const auto files=std::filesystem::temp_directory_path()/("pstencil-frustum-"+std::to_string(getpid()));
 PhysicsLiveWorldTu15 world(cache,argv[1],files);world.configure_session(source_game_variant_default(0),1,30,0x12345678u);world.initialize_map();
 const auto word=[&](unsigned p){return Bytes(world.memory().view(p,4)).u32(0);};
 const auto put=[&](unsigned p,const auto&v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};
 const auto geometry=cache.load_source_bsp_admission(0);require(!geometry.clusters.empty(),"Opening fixture lost its first BSP");
 const auto b=geometry.clusters.front().bounds;const Vec3 center{(b[0]+b[1])*.5f,(b[2]+b[3])*.5f,(b[4]+b[5])*.5f},forward{1,0,0},up{0,0,1};
 std::vector<Material> authored;
 for(const auto&t:cache.tags())if(t.group=="shad" && (t.name=="objects\\characters\\masterchief\\shaders\\masterchief" ||
   t.name=="objects\\characters\\elite\\shaders\\arms" || t.name.ends_with("\\panel_mech_gunbarrel") || t.name=="objects\\characters\\dervish\\shaders\\dervish_armor"))authored.push_back(cache.load_material(t.datum));
 require(authored.size()>=3,"Opening fixture lost its actual armor/cinematic materials");
 unsigned lights=0,bindings=0;
 for(const auto&t:cache.tags())if(t.group=="ligh" && t.name.find("\\x01_light_")!=std::string::npos){
  const auto raw=cache.load_source_tag_metadata(t.datum);const Bytes bytes(raw.bytes_at(raw.source_address(),228));EffectLightDefinition definition;
  std::copy(bytes.slice(0,228).begin(),bytes.slice(0,228).end(),definition.bytes.begin());definition.flags=bytes.u32(0);
  auto call=world.prepare_source_call();const auto input=call.gp[4]-0x200u;put(input,center);put(input+16u,forward);put(input+32u,up);
  call=world.prepare_source_call(std::array{t.datum,0xffffffffu,0xffffu,input+32u});call.gp[3]=input;call.gp[7]=input+16u;
  call.xmm[0][0]=std::bit_cast<unsigned>(1.f);world.run_actual(0xc2380u,call);const auto id=call.gp[0];require(id!=0xffffffffu,"Original light constructor declined");
  const auto shape=pstencil::native_light_shape_tu15(world,id);const auto cameras=pstencil::native_light_cameras_tu15(world,id);
  const auto constants=pstencil::native_light_constants_tu15(world,id,{1,1,1,1,1},1,{});
  const auto receiver=pstencil::native_light_receiver_tu15(world,id);
  require(shape.type()==2 && cameras.size()==1 && receiver.view.visibility.groups.size()==1,"Authored opening frustum changed shape/camera");
  require(!std::memcmp(cameras.data(),receiver.view.visibility.groups.data(),sizeof(SourceFrustumCamera)),"Original query changed native camera bytes");
  const auto pool=word(0x4ecbdcu),record=word(pool+0x44)+(id&65535u)*272u;
  std::cout<<t.name<<" radius="<<std::bit_cast<float>(word(record+0x24))<<" query="<<receiver.view.query_mode
   <<" clusters="<<receiver.view.visibility.clusters.size()<<" frustums="<<receiver.view.visibility.frustums.size()
   <<" shift="<<constants.color_shift<<" virtual_origin="<<constants.constants[0][0]<<','<<constants.constants[0][1]<<','<<constants.constants[0][2]<<'\n';
  for(auto m:authored)for(unsigned lod=0;lod<m.lods.size();++lod){m.selected_shader_lod=lod;
   for(const unsigned mask:{0u,2u,4u,6u}){
    const auto impl=pstencil::projected_light_implementation(definition.flags,mask,m.shader_switches[4]!=0);
    const auto prepared=pstencil::prepare_projected_light_material(cache,m,definition,constants,impl,mask);
    const auto fast=pstencil::complete_projected_light_constants(cache,m,definition,constants,impl,mask);
    require(fast.size()==prepared.size(),"Resident constant completion changed the pass count");
    for(unsigned i=0;i<fast.size();++i) {
     const auto&a=prepared[i];const auto&b=fast[i];
     require(a.forward_program.words==b.forward_program.words && a.forward_program.fragment_offsets==b.forward_program.fragment_offsets,"Image-free completion changed the completed program");
     require(!std::memcmp(a.program_constants.data(),b.program_constants.data(),sizeof(a.program_constants)),"Image-free completion changed the vertex/attribute bank");
     require(a.forward_blend==b.forward_blend && a.forward_raster==b.forward_raster && a.texture_states==b.texture_states,"Image-free completion changed fixed/texture state");
     require(a.forward_pass_index==b.forward_pass_index && a.source_shader_datum==b.source_shader_datum && a.selected_shader_lod==b.selected_shader_lod,"Image-free completion changed the selected owner/pass");
     const auto ap=pack_vertex_program_bank(*a.vertex_bank),bp=pack_vertex_program_bank(*b.vertex_bank);
     require(!std::memcmp(&ap,&bp,sizeof(ap)),"Image-free completion changed the extended vertex bank");
     for(const auto*t:{&b.diffuse,&b.detail,&b.bump,&b.auxiliary})require(t->pixels.empty() && t->mips.empty(),"Constant completion loaded image bytes");
     (void)source_effect_light_draw(0,b,{});
    }

    if(!(m.lods[lod].available_layers&(1u<<12))){require(prepared.empty(),"Unlit LOD acquired a projected pass");continue;}
    require(!prepared.empty(),"Authored projected layer lost its compiled pass");
    for(const auto&out:prepared){
     (void)source_effect_light_draw(0,out,{});require(out.forward_pass && out.vertex_bank,"Projected material lost its compiled program/bank");
     const auto&p=out.passes.at(out.forward_pass_index);require(p.implementation==impl,"Projected material changed original implementation");
     require(std::equal(constants.constants.begin(),constants.constants.begin()+3,out.program_constants[1].begin()+13),"Projected material changed nativeC31..33");
     require(std::equal(constants.constants.begin()+3,constants.constants.end(),out.vertex_bank->constants.begin()),"Projected material changed nativeC34..39");++bindings;
    }
   }
  }++lights;
 }
 require(lights==5,"Opening fixture did not exercise all five authored cinematic lights");
 std::cout<<"PASS "<<lights<<" real opening light definitions; "<<bindings<<" actual shader/LOD/projected material bindings exactly match full resource binding; no image loads; whole native shapes, cameras, constants and receiver queries\n";
 std::filesystem::remove_all(files);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
