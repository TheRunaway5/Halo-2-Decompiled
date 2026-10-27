#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw FormatError(message);}
class FixtureMemory final:public SourcePhysicsContactMemory {
    struct Row {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Row> rows;
public:
    void mount(std::uint32_t address,std::vector<std::byte> bytes){rows.push_back({address,std::move(bytes)});}
    void read(std::uint32_t address,std::span<std::byte> out) const override {
        for(const auto &row:rows)if(address>=row.address && std::uint64_t(address)-row.address+out.size()<=row.bytes.size()) {
            std::copy_n(row.bytes.begin()+(address-row.address),out.size(),out.begin());return;
        }
        throw FormatError("Texture-camera source fixture owner is unavailable");
    }
    void write(std::uint32_t,std::span<const std::byte>) override {throw FormatError("Read-only camera fixture mutated");}
};
std::vector<std::byte> unhex(const std::string &s){check(!(s.size()%2),"Odd camera fixture extent");std::vector<std::byte> out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
bool equal(const Texture&a,const Texture&b){if(a.width!=b.width||a.height!=b.height||a.format!=b.format||a.layers!=b.layers||a.depth!=b.depth||a.pixels!=b.pixels||a.mips.size()!=b.mips.size())return false;for(unsigned n=0;n<a.mips.size();++n){const auto&x=a.mips[n];const auto&y=b.mips[n];if(x.width!=y.width||x.height!=y.height||x.offset!=y.offset||x.size!=y.size||x.depth!=y.depth)return false;}return true;}
template<class F>void rejected(F f){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}check(stopped,"Missing texture-camera owner did not reject");}
}
int main(int argc,char**argv)try {
 check(argc>=3 && argc<=5,"Supply source initial fixture, disc maps, optional prepared maps and optional source oracle control output");
 std::ifstream stream(argv[1]);const auto fixture=nlohmann::json::parse(stream);
 check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Camera fixture source is not authenticated TU1.5");
 FixtureMemory memory;
 for(const auto& row:fixture.at("rows")){auto raw=unhex(row.at("hex").get<std::string>());memory.mount(row.at("address").get<std::uint32_t>(),std::move(raw));}
 const auto initial=source_texture_camera_inputs_tu15(memory);
 check(initial.camera_words[0]==0xffffffffu && !initial.cinematic_auxiliary && !initial.texture_camera_view && !initial.target18.allocation && !initial.target20.allocation,"Actual initial source camera bank changed");
 const auto lifecycle=source_texture_camera_lifecycle_tu15(initial);
 check(!lifecycle.needed && lifecycle.action==SourceTextureCameraTargetActionTu15::none,"Initial source lifecycle unexpectedly requests a camera target");
 const auto binding=source_texture_camera_binding_tu15(initial);
 check(binding.texture()==SourceTextureCameraTextureTu15::default2d,"Initial source extern18 does not select default2D");
 FixtureMemory missing;rejected([&]{(void)source_texture_camera_inputs_tu15(missing);});
 auto active=initial;active.camera_words[0]=0x80010001;
 const auto allocation=source_texture_camera_lifecycle_tu15(active);check(allocation.action==SourceTextureCameraTargetActionTu15::allocate18 && allocation.width==640 && allocation.height==480,"Source active camera allocation request changed");
 rejected([&]{(void)source_texture_camera_binding_tu15(active);});
 active.target18.allocation=0x81000000;check(source_texture_camera_binding_tu15(active).texture()==SourceTextureCameraTextureTu15::target18,"Source active camera target18 changed");
 active.target20.allocation=0x82000000;check(source_texture_camera_binding_tu15(active).texture()==SourceTextureCameraTextureTu15::target20,"Source target20 priority changed");
 active.target20.retiring=1;check(source_texture_camera_binding_tu15(active).texture()==SourceTextureCameraTextureTu15::target18,"Source retired20 was selected");
 active.texture_camera_view=1;check(source_texture_camera_binding_tu15(active).texture()==SourceTextureCameraTextureTu15::default2d,"Source recursive camera view sampled itself");
 auto retiring=initial;retiring.target18.allocation=0x81000000;check(source_texture_camera_lifecycle_tu15(retiring).action==SourceTextureCameraTargetActionTu15::retire18,"Source inactive camera did not retire target18");rejected([&]{(void)source_texture_camera_binding_tu15(retiring);});
 retiring.target18.retiring=1;check(source_texture_camera_binding_tu15(retiring).texture()==SourceTextureCameraTextureTu15::default2d,"Source retired18 was selected");
 unsigned materials=0,implementations=0;
 for(unsigned directory=2;directory<unsigned(std::min(argc,4));++directory)for(const auto*name:{"06a_sentinelwalls","07b_forerunnership"}) {
  Cache cache(std::filesystem::path(argv[directory])/(std::string(name)+".map"));
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.group=="shad" && t.name=="scenarios\\shaders\\solo\\highcharity\\viewscreen_sanctum";});check(tag!=cache.tags().end(),"Authored texture_camera material owner missing");
  const auto authored=cache.load_material(tag->datum);const auto fallback=cache.load_scope_default_bitmap();
  for(unsigned implementation=0;implementation<4;++implementation){auto material=authored;
   check(prepare_texture_camera_default_material(cache,material,binding,implementation),"Source default camera pass did not prepare");
   const auto &pass=authored.passes.at(material.forward_pass_index);
   check(pass.implementation==implementation && pass.template_pass==0 && material.world_pass && material.forward_pass && !material.static_lightmap_pass && material.forward_vertex==3,"Camera pass owner/class changed");
   check(material.vertex_program==pass.vertex_code && material.forward_program.words==pass.program.words && material.program_constants==pass.constants && material.texture_states[0]==pass.texture_states[0],"Camera source program/constants/sampler changed");
   check(equal(material.diffuse,fallback) && material.rendering_gaps.empty(),"Camera default2D differs from exact matg+64 frame0 image");
   check(!material.forward_blend && material.forward_raster.depth_write && material.forward_raster.color_write==0x01010101,"Camera source raster state changed");
   auto target=initial;target.target20.allocation=0x82000000;auto unbound=authored;
   check(!prepare_texture_camera_default_material(cache,unbound,source_texture_camera_binding_tu15(target),implementation) && !unbound.world_pass,"Unowned active camera target was accepted as default2D");++implementations;
  }
  const auto warning=authored.name+": material uses an unsupported rendering pass";
  auto unbound_mesh=cache.load_bsp(3);
  SourceBspResidentPlan unbound_plan(cache,unbound_mesh,3);
  check(std::find(unbound_mesh.material_warnings.begin(),unbound_mesh.material_warnings.end(),warning)!=unbound_mesh.material_warnings.end(),"Unowned camera warning disappeared");
  check(std::any_of(unbound_mesh.materials.begin(),unbound_mesh.materials.end(),[&](const auto&m){return m.name==authored.name&&!m.world_pass;}),"Default nullptr invented a camera binding");
  auto bound_mesh=cache.load_bsp(3);
  SourceBspResidentPlan bound_plan(cache,bound_mesh,3,SourceBspResidentLighting::static_lightmap,&binding);
  check(!bound_plan.parts().empty() && std::find(bound_mesh.material_warnings.begin(),bound_mesh.material_warnings.end(),warning)==bound_mesh.material_warnings.end(),"Resolved source camera warning survived resident integration");
  check(std::all_of(bound_mesh.materials.begin(),bound_mesh.materials.end(),[&](const auto&m){return m.name!=authored.name || (m.world_pass&&m.forward_pass&&m.rendering_gaps.empty());}),"Resident camera owner did not retain its exact pass");
  ++materials;
 }
 if(argc==5){std::ofstream output(argv[4]);for(unsigned object:{0u,0xffffffffu,1u,0x80010001u})for(unsigned aux:{0u,1u})for(unsigned cinematic:{0u,1u,255u})for(unsigned view:{0u,1u,255u})for(unsigned a18:{0u,0x81000000u})for(unsigned r18:{0u,1u,255u})for(unsigned a20:{0u,0x82000000u})for(unsigned r20:{0u,1u,255u}){
  auto input=initial;input.camera_words[0]=object;input.cinematic_auxiliary=aux?0x2000000u:0;input.cinematic=cinematic;input.texture_camera_view=view;input.target18={a18,std::uint8_t(r18)};input.target20={a20,std::uint8_t(r20)};
  const auto plan=source_texture_camera_lifecycle_tu15(input);int selected=-1;try{selected=int(source_texture_camera_binding_tu15(input).texture());}catch(const FormatError&){}
  const std::array<std::uint32_t,13> row{object,aux,cinematic,view,a18,r18,a20,r20,unsigned(plan.needed),unsigned(plan.action),plan.width,plan.height,std::uint32_t(selected)};
  output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));
 }check(bool(output),"Camera source controls could not be written");}
 std::cout<<materials<<" actual texture_camera materials; "<<implementations<<" source implementation bindings; exact default2D and active-target rejection\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
