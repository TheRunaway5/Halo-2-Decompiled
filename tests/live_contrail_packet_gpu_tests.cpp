// Re-render read-only packets actually submitted by a visible LAN session.
#include "halo2/source_contrail_renderer.hpp"
#include "halo2/source_contrail_state.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/original_data.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/source_runtime_image.hpp"
#include "source_weather_point_gpu_fixture.hpp"
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;using halo2_weather_point_fixture::require;
namespace {
Vec3 xyz(const Json &j){return {j.at(0),j.at(1),j.at(2)};}
}
int main(int argc,char **argv)try {
 require(argc==6,"Supply map, shared maps, original XBE, actual GPU packets and output directory");
 const auto original=OriginalData::load_xbe(argv[3]);require(original->image==OriginalData::Image::TitleUpdate15,"Actual GPU packets require TU1.5");
 Cache cache(argv[1],argv[2],original);Json packets;std::ifstream(argv[4])>>packets;
 const std::filesystem::path output=argv[5];std::filesystem::create_directories(output);
 halo2_weather_point_fixture::Gpu gpu;if(!gpu.initialize(true))return 77;
 const VkExtent2D extent{960,540};const auto pass=gpu.scene_pass(true);
 SourceContrailRenderer renderer(gpu.physical,gpu.device,gpu.queue,gpu.family,pass,HALO2_SHADER_DIR);
 const auto color=gpu.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
 const auto depth=gpu.image(extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
 const auto frame=gpu.framebuffer(pass,extent,color,depth);const auto buffer=gpu.buffer(960*540*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
 Json cases=Json::array();std::array<unsigned,2> maximum_pixels{};unsigned ordinal{},tested{};
 std::map<std::uint32_t,Texture> textures;
 for(const auto &p:packets){if(!p.at("colored_triangle_area").get<bool>())continue;
  const auto definition=p.at("definition").get<unsigned>();const auto tag=cache.load_source_tag_metadata(definition);const Bytes d(tag.bytes_at(tag.source_address(),tag.size()));
  const auto bitmap=d.u32(0x34);require(cache.bitmap_count(bitmap)==1,"GPU capture requires an explicit multi-image selector");
  auto [texture,created]=textures.try_emplace(bitmap);if(created)texture->second=cache.load_bitmap(bitmap,0);
  SourceContrailDraw draw;draw.datum=p.at("trail");draw.definition=definition;draw.texture=&texture->second;
  draw.blend=d.u16(0x66);draw.pixel.words=p.at("pixel").get<decltype(draw.pixel.words)>();
  require(draw.pixel.words==source_contrail_pixel_program(draw.blend,std::uint8_t(d.u16(0x64))).words,"GPU packet combiner differs from original authored selection");
  draw.sampler=std::bit_cast<ShaderTextureState>(p.at("sampler").get<std::array<std::uint32_t,14>>());
  draw.constants=p.at("constants").get<decltype(draw.constants)>();draw.viewport=p.at("viewport").get<ShaderVector>();draw.clip_range=p.at("clip").get<ShaderVector>();
  for(const auto &v:p.at("vertices"))draw.vertices.push_back({xyz(v.at("position")),{v.at("uv").at(0),v.at("uv").at(1)},v.at("color")});
  // Original indexed blend operands are immutable source data. Authenticate
  // their exact cells through the XBE-backed runtime image.
  auto image=load_source_runtime_image(argv[3],SourcePhysicsContactImage::TitleUpdate15);
  if(draw.blend!=10)for(unsigned n=0;n<3;++n)draw.blend_operands[n]=Bytes(image.view(std::array{0x45c438u,0x45c408u,0x45c3d8u}[n]+draw.blend*4u,4)).u32(0);
  renderer.prepare(std::span(&draw,1));gpu.begin();VkClearValue clear[2]{};clear[0].color={{8.f/255,16.f/255,24.f/255,1}};clear[1].depthStencil={1,0};
  VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=pass;begin.framebuffer=frame;begin.renderArea={{0,0},extent};begin.clearValueCount=2;begin.pClearValues=clear;
  vkCmdBeginRenderPass(gpu.command,&begin,VK_SUBPASS_CONTENTS_INLINE);
  const auto &v=draw.viewport;renderer.record(gpu.command,{v[0],v[1],v[2],v[3],0,1},{{std::int32_t(v[0]),std::int32_t(v[1])},{std::uint32_t(v[2]),std::uint32_t(v[3])}},0,1);
  vkCmdEndRenderPass(gpu.command);gpu.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,buffer);gpu.end();
  const auto *pixels=static_cast<const unsigned char*>(buffer.mapped);unsigned changed{};
  for(unsigned i=0;i<960*540;++i)changed+=pixels[i*4]!=8 || pixels[i*4+1]!=16 || pixels[i*4+2]!=24;
  const auto view=p.at("view").get<unsigned>();require(view<2,"Actual GPU packet has an invalid view");
  if(changed>maximum_pixels[view]){maximum_pixels[view]=changed;std::ofstream ppm(output/("view-"+std::to_string(view)+".ppm"),std::ios::binary);ppm<<"P6\n960 540\n255\n";for(unsigned i=0;i<960*540;++i)ppm.write(reinterpret_cast<const char*>(pixels+i*4),3);require(bool(ppm),"Cannot retain actual packet pixels");}
  cases.push_back({{"view",view},{"authority_tick",p.at("authority_tick")},{"frame",p.at("completed_frames_before_submission")},{"changed_pixels",changed}});++tested;++ordinal;
 }
 require(halo2_weather_point_fixture::validation_errors==0,"Actual contrail packet caused Vulkan validation errors");
 const bool passed=maximum_pixels[0]>0 && maximum_pixels[1]>0;
 std::ofstream(output/"report.json")<<Json{{"passed",passed},{"reference","English TU1.5 actual live LAN GPU packets"},{"tested_colored_packets",tested},{"maximum_changed_pixels_by_view",maximum_pixels},{"validation_errors",halo2_weather_point_fixture::validation_errors},{"cases",cases},{"boundary","Actual submitted vertices, constants, clip, sampler and combiner with original single-image bitmap and immutable blend operands. Clear background and unoccluded depth are diagnostic inputs; this is not an Xbox framebuffer or full-scene pixel comparison."}}.dump(2)<<'\n';
 require(passed,"Actual colored contrail packets produce no pixels in one or both views");std::cout<<"PASS actual live contrail GPU packets: "<<maximum_pixels[0]<<", "<<maximum_pixels[1]<<" visible pixels\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
