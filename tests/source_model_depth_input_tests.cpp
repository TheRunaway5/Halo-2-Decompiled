#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/source_model_depth_input.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/world_material.hpp"
#include "halo2/pstencil/geometry.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
using Json=nlohmann::json;Json controls=Json::array();
void need(bool value,const char* why){if(!value)throw FormatError(why);}
template<class T>T read(std::ifstream& input){T value;need(bool(input.read(reinterpret_cast<char*>(&value),sizeof(value))),"Truncated actual model/camera fixture");return value;}
SourceRuntimeMemory snapshot(const char* path){std::ifstream input(path,std::ios::binary);need(bool(input),"Actual camera bank absent");SourceRuntimeMemory out;
 while(input.peek()!=std::ifstream::traits_type::eof()){const auto at=read<std::uint32_t>(input),size=read<std::uint32_t>(input);std::vector<std::byte> bytes(size);
  need(bool(input.read(reinterpret_cast<char*>(bytes.data()),size)),"Truncated actual camera segment");out.mount(at,bytes,SourceRuntimeMemory::Access::ReadWrite,"Retained actual camera input");}return out;}
template<class T>T get(const SourceRuntimeMemory& memory,std::uint32_t at){T value;std::memcpy(&value,memory.view(at,sizeof(T)).data(),sizeof(T));return value;}
template<class F>void reject(const char* name,F&& fn){try{fn();}catch(const FormatError& error){controls.push_back({{"name",name},{"reason",error.what()}});return;}
 throw FormatError(std::string("Unowned model-depth control admitted: ")+name);}
int run(int argc,char** argv){
 need(argc==6,"Supply original Lockout, actual camera bank, native command fixture, shaders and report");
 for(unsigned n=1;n<=3;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
 Cache cache(argv[1]);auto memory=snapshot(argv[2]);auto mesh=cache.load_bsp(0);
 SourceBspResidentPlan resident(cache,mesh,0,SourceBspResidentLighting::classification3);
 const auto geometry=cache.load_source_bsp_admission(0);SourceBspAdmission admission;
 for(unsigned n=0;n<geometry.clusters.size();++n)admission.clusters.push_back({n});
 for(unsigned n=0;n<geometry.instances.size();++n)admission.instances.push_back({n});
 const auto camera=get<SourceViewProjectionBank>(memory,0x4c6634u);
 auto selected=resident.select({camera,admission,0,480,1,false,{}});
 // The retained record supplies the real original model section/class and
 // current c50 palette. Apply pass choice below is an explicit component
 // service, not a claim that this record came from a live Apply collector.
 std::ifstream fixture(argv[3],std::ios::binary);const auto header=read<std::array<std::uint32_t,7>>(fixture);
 need(header[0]==0x53524853 && header[1]==1,"Actual command fixture schema changed");
 SourceShadowUniforms uniforms;uniforms.projection=read<std::array<ShaderVector,6>>(fixture);uniforms.constants=read<std::array<ShaderVector,9>>(fixture);uniforms.opacity=std::bit_cast<float>(header[6]);
 fixture.seekg(header[4]*4,std::ios::cur);const auto info=read<std::array<std::uint32_t,5>>(fixture);
 SourceShadowRawCommandTu15 command;command.descriptor=read<std::array<std::uint32_t,6>>(fixture);command.command=read<std::array<std::uint32_t,6>>(fixture);command.unit=read<std::array<std::uint32_t,8>>(fixture);
 std::vector<std::array<float,12>> palette(info[4]);need(bool(fixture.read(reinterpret_cast<char*>(palette.data()),palette.size()*48)),"Actual native palette absent");
 const auto model_id=command.unit[1],section_id=(command.unit[3]>>9)&511u,part_id=command.command[3]&255u;
 auto section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,model_id,section_id));
 std::uint32_t definition=~0u;
 for(const auto& tag:cache.tags())if(tag.group=="weap"||tag.group=="bipd"||tag.group=="vehi"){
  try{if(cache.load_initial_model_definition(tag.datum).render_model==model_id){definition=tag.datum;break;}}catch(const FormatError&){}
 }
 need(definition!=~0u,"Actual native model has no matching Cache object definition");
 const std::array<unsigned,1> sections{section_id};auto model=cache.load_model_sections(model_id,true,0,5,sections);
 // Only the captured part submits geometry. The model also retains other
 // authored shader slots and an unused NONE slot; those own no fixture draw.
 std::vector<bool> used_materials(model.mesh.materials.size());
 for(unsigned n=0;n<model.mesh.vertices.size();++n)
  if(model.vertex_origins[n].section==section_id&&model.vertex_origins[n].part==part_id)
   used_materials.at(model.mesh.vertices[n].material)=true;
 for(unsigned slot=0;slot<model.mesh.materials.size();++slot)if(used_materials[slot])
  need(prepare_dynamic_material(cache,model.mesh.materials[slot],{},true),"Actual model ordinary material unavailable");
 const auto base=unsigned(mesh.materials.size());mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
 std::vector<Vertex> vertices;std::vector<std::uint32_t> ordinals;
 for(unsigned n=0;n<model.mesh.vertices.size();++n)if(model.vertex_origins[n].section==section_id&&model.vertex_origins[n].part==part_id){
  vertices.push_back(model.mesh.vertices[n]);vertices.back().material+=base;ordinals.push_back(n);}
 need(!vertices.empty(),"Actual model part has no original expanded ordinals");
 SDL_SetMainReady();need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
 auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Native model depth owners",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);need(bool(window),SDL_GetError());
 // Renderer/postprocessing retain the actual atmosphere through attachment
 // recreation; this owner must outlive both the renderer and its scaled use.
 const auto atmosphere=cache.load_bsp_atmosphere(0);
 Renderer renderer(window.get(),mesh,true,{},nullptr,{},nullptr,argv[4]);renderer.scene_bsp_atmosphere(atmosphere);
 auto targets=renderer.target_resources();auto use=targets.acquire();auto world=source_world_depth_input_tu15(memory,use,cache,mesh,resident,selected);const auto& raster=world.raster(use);
 auto authored=cache.load_material(command.command[2]);authored.selected_shader_lod=command.command[5]>>8&255u;
 auto materials=prepare_shadow_material(cache,authored,SourceShadowPhase::Apply,true,false,uniforms.opacity);need(!materials.empty(),"Actual model has no authored Apply pass");auto& material=materials.front();
 const auto pass=load_menu_world_source_pass(cache,command.command[2],{authored.selected_shader_lod,11,0,4});
 command.phase=SourceShadowPhase::Apply;command.root_owner=header[5];command.bank=0x4ce280;command.category=1;command.layer=11;command.sorted_ordinal=0;command.unit_ordinal=info[2];
 command.command[4]|=1u<<11;command.command[5]&=~255u;command.descriptor[1]=(command.descriptor[1]&~255u)|4u;command.descriptor[4]=pass.implementation_address();
 command.geometry_owner={MenuWorldSourceOwnerKind::dynamic_object,model_id,info[3]&65535u,info[3]};
 MenuWorldSourceConstants bank{};std::copy(raster.camera.begin(),raster.camera.end(),bank.begin());bank=source_shadow_raw_constants(bank,material,uniforms);
 const auto topology=(command.unit[0]&0x200u) && !(section->parts().at(part_id).flags&8u)?MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
 auto binding=menu_world_source_binding_class(cache,command.geometry_owner,menu_world_source_draw(section,part_id,topology,palette),pass.vertex_shader(),command.descriptor[2]&65535u,bank);
 MenuWorldRasterPacket packet{};packet.surface_scale=raster.surface_scale;packet.regions.fill({0,0,640,480});
 auto raw=source_shadow_raw_draw(0,material,pass,std::move(binding),command,uniforms,{0x900,true,raster.surface_clip},packet,raster.clip_range,{1,1},raster.surface_size);
 const std::array<const SourceShadowRawDraw*,1> receivers{&raw};SourceModelDepthSelection selection{info[3],definition,model_id,5,&model,0,ordinals};
 const auto make=[&](const auto& choice,const auto& input){return source_model_depth_input_tu15(world,use,input,std::span(&choice,1),receivers,mesh.materials);};
 auto depth=make(selection,std::span<const Vertex>(vertices));need(depth.model_vertices()==vertices.size(),"Native model depth lost selected source vertices");depth.validate_vertices(use,depth.vertices());
 const auto positions=pstencil::complete_world_positions(raw.source);unsigned native_order_triangles=0;std::size_t expanded=0;
 for(std::size_t n=0;n<raw.source.draw().triangle_count();++n) {
  const auto triangle=raw.source.draw().triangle(n);
  if(triangle[0]==triangle[1]||triangle[0]==triangle[2]||triangle[1]==triangle[2])continue;
  need(expanded+3<=vertices.size(),"Native model depth lost an authored source triangle");bool reordered=false;
  for(unsigned lane=0;lane<3;++lane) {
   unsigned original=0;while(original<3&&model.vertex_origins[ordinals[expanded+original]].vertex!=triangle[lane])++original;
   need(original<3,"Native model depth original primitive occurrence differs from authored raw order");
   auto expected=vertices[expanded+original];expected.position=positions[triangle[lane]];
   need(!std::memcmp(&depth.vertices()[expanded+lane],&expected,sizeof(Vertex)),"Native model depth changed exact raw triangle order/attributes or fixed prefix output");reordered|=original!=lane;
  }
  native_order_triangles+=reordered;expanded+=3;
 }
 need(expanded==vertices.size()&&native_order_triangles>0,"Native model depth fixture must exercise cyclic strip expansion with exact raw order");
 SourceShadowRawPlanRoot root{command.root_owner,uniforms,{1,1},{},std::span(&raw,1)};SourceShadowRawPlan plan{raster.camera,std::span(&root,1)};depth.validate_receivers(plan);
 reject("missing raw Apply root",[&]{auto empty=plan;empty.roots={};depth.validate_receivers(empty);});
 reject("changed actual descriptor",[&]{auto saved=raw.command.descriptor;raw.command.descriptor[4]^=4;try{depth.validate_receivers(plan);}catch(...){raw.command.descriptor=saved;throw;}raw.command.descriptor=saved;});
 reject("foreign full salted object",[&]{auto bad=selection;bad.object^=65536u;(void)make(bad,std::span<const Vertex>(vertices));});
 reject("foreign definition",[&]{auto bad=selection;bad.definition=~0u;(void)make(bad,std::span<const Vertex>(vertices));});
 reject("foreign LOD",[&]{auto bad=selection;bad.lod=6;(void)make(bad,std::span<const Vertex>(vertices));});
 reject("escaped original ordinal",[&]{auto ids=ordinals;ids[0]=model.mesh.vertices.size();auto bad=selection;bad.source_vertices=ids;(void)make(bad,std::span<const Vertex>(vertices));});
 reject("reversed original triangle",[&]{auto ids=ordinals;std::swap(ids[0],ids[1]);auto bad=selection;bad.source_vertices=ids;(void)make(bad,std::span<const Vertex>(vertices));});
 reject("changed submitted WORLD position",[&]{auto bad=std::vector<Vertex>(depth.vertices().begin(),depth.vertices().end());bad.front().position.x=std::nextafter(bad.front().position.x,INFINITY);depth.validate_vertices(use,bad);});
 reject("changed native palette",[&]{auto changed=palette;changed.front()[3]+=1;auto saved=raw.source;raw.source=menu_world_source_binding_class(cache,command.geometry_owner,menu_world_source_draw(section,part_id,topology,changed),pass.vertex_shader(),command.descriptor[2]&65535u,bank);try{depth.validate_vertices(use,depth.vertices());}catch(...){raw.source=std::move(saved);throw;}raw.source=std::move(saved);});
 reject("changed ordinary shader",[&]{auto& m=mesh.materials.at(vertices.front().material);const auto saved=m.source_shader_datum;m.source_shader_datum^=1;try{depth.validate_vertices(use,depth.vertices());}catch(...){m.source_shader_datum=saved;throw;}m.source_shader_datum=saved;});
 // Reuse this actual device so the fixture never depends on two Vulkan
 // instance/swapchain teardown order. Physical320x240 remains distinct from
 // the unchanged authenticated640x480 source viewport and raw Apply bank.
 auto display=renderer.display_settings();display.internal_width=320;display.internal_height=240;
 renderer.display_settings(display);need(renderer.prepare_world_targets(),"Actual scaled target creation failed");
 auto scaled_targets=renderer.target_resources();auto scaled_use=scaled_targets.acquire();
 need(scaled_use.bank().depth_stencil.width==320&&scaled_use.bank().depth_stencil.height==240,"Scaled native fixture did not own its actual physical attachment");
 reject("recreated actual attachment lease",[&]{depth.validate_vertices(use,depth.vertices());});
 reject("wrong declared logical source extent",[&]{(void)source_world_depth_input_tu15(memory,scaled_use,cache,mesh,resident,selected,{320,240});});
 auto scaled_world=source_world_depth_input_tu15(memory,scaled_use,cache,mesh,resident,selected,{640,480});
 auto scaled=source_model_depth_input_tu15(scaled_world,scaled_use,vertices,std::span(&selection,1),receivers,mesh.materials);
 scaled.validate_vertices(scaled_use,scaled.vertices());scaled.validate_receivers(plan);
 need(scaled.model_vertices()==depth.model_vertices()&&!std::memcmp(scaled.vertices().data(),depth.vertices().data(),scaled.vertices().size_bytes()),"Logical/physical scaling changed authentic native WORLD positions");
 reject("raw source surface replaced by physical extent",[&]{const auto saved=raw.surface_size;raw.surface_size={320,240};try{(void)source_model_depth_input_tu15(scaled_world,scaled_use,vertices,std::span(&selection,1),receivers,mesh.materials);}catch(...){raw.surface_size=saved;throw;}raw.surface_size=saved;});
 reject("mutated actual native viewport",[&]{const auto saved=get<std::array<std::int16_t,4>>(memory,0x4c65f0u);auto changed=saved;changed[3]-=2;memory.write(0x4c65f0u,std::as_bytes(std::span(changed)));try{scaled.validate_vertices(scaled_use,scaled.vertices());}catch(...){memory.write(0x4c65f0u,std::as_bytes(std::span(saved)));throw;}memory.write(0x4c65f0u,std::as_bytes(std::span(saved)));});
 renderer.release_presentation();reject("retired actual attachment lease",[&]{scaled.validate_vertices(scaled_use,scaled.vertices());});
 Json report{{"passed",true},{"model",model_id},{"object",info[3]},{"definition",definition},{"class",command.descriptor[2]&65535u},{"vertices",depth.model_vertices()},{"native_order_triangles",native_order_triangles},{"negative_controls",controls},
  {"logical_surface",{640,480}},{"physical_scaled_surface",{320,240}},
  {"scope","Actual original model section, captured native palette/geometry ordinal, retained actual main camera and genuine Vulkan leases including logical640x480/physical320x240; explicit Apply caller transition. No live Apply collection or physical Xbox framebuffer claim."}};
 std::ofstream output(argv[5]);output<<report.dump(2)<<'\n';need(bool(output),"Could not retain model depth owner report");std::cout<<"PASS native model-depth owner: "<<depth.model_vertices()<<" vertices; "<<controls.size()<<" strict controls\n";return 0;
}
}
int main(int argc,char** argv)try{return run(argc,argv);}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
