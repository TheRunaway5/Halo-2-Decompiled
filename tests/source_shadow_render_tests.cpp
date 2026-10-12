#include "source_shadow_gpu_fixture.hpp"
#include "scalar_combiners.hpp"
using namespace halo2;
using namespace halo2_shadow_fixture;
int main(){try {
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);if(!std::filesystem::exists(root/"assets/disc/maps/zanzibar.map"))return 77;
    Cache cache(root/"assets/disc/maps/zanzibar.map");auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.name=="objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod"&&t.group=="shad";});require(tag!=cache.tags().end(),"source shadow shader missing");auto m=cache.load_material(tag->datum);auto generation=prepare_shadow_material(cache,m,SourceShadowPhase::Generate,true,false,.75f);auto application=prepare_shadow_material(cache,m,SourceShadowPhase::Apply,true,false,.75f);require(generation.size()==1&&application.size()==1,"source shadow passes differ");
    Gpu gpu;if(!gpu.initialize()){std::cout<<"Shadow GPU test requires validation and custom border color\n";return 77;}
    auto opaque=gpu.image({1,1},VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT),transparent=gpu.image({1,1},VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);gpu.upload(opaque,false);gpu.upload(transparent,true);auto bitmap_sampler=gpu.sampler(false),border_sampler=gpu.sampler(true);
    unsigned checks=0;
    for(const VkExtent2D extent:std::array{VkExtent2D{320,240},VkExtent2D{853,479}}) {
        auto color=gpu.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);auto depth=gpu.image(extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);auto initial=gpu.scene_pass(true),continuation=gpu.scene_pass(false);auto frame=gpu.framebuffer(initial,extent,color,depth);
        SourceShadowRender render({gpu.device,gpu.physical,extent,continuation});auto caster=gpu.pipeline(render.generation_pass(),false),receiver=gpu.pipeline(continuation,true);
        Uniform gen{generation[0].material.forward_program.words,{},{},{1,1,0,0},{4,0,0,0}};
        Uniform apply{application[0].material.forward_program.words,{.5f,.5f,.5f,.5f},{},{128,128,0,0},{0,1,0,0}};
        auto solid=gpu.descriptor(opaque,opaque,bitmap_sampler,bitmap_sampler,gen);auto empty=gpu.descriptor(transparent,opaque,bitmap_sampler,bitmap_sampler,gen);
        Image target{render.target_image(),{},render.target_view()};auto read=gpu.descriptor(target,opaque,border_sampler,bitmap_sampler,apply);apply.coordinates={256,256,-64,-64};auto border=gpu.descriptor(target,opaque,border_sampler,bitmap_sampler,apply);
        const auto scene_pixels=gpu.buffer(VkDeviceSize(extent.width)*extent.height*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT),target_pixels=gpu.buffer(128*128*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        const auto draw=[&](VkPipeline pipeline,VkDescriptorSet set,Position position){return [&,pipeline,set,position](VkCommandBuffer cmd,const SourceShadowUniforms&){vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,gpu.layout,0,1,&set,0,nullptr);vkCmdPushConstants(cmd,gpu.layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(position),&position);vkCmdDraw(cmd,6,1,0,0);};};
        for(unsigned mode=0;mode<6;++mode) {
            SourceShadowDraw generate{&generation[0],draw(caster,mode==1?empty:solid,{{-.5f,-.5f,.5f,.5f},{.5f,0,0,0}})};
            SourceShadowDraw receive{&application[0],draw(receiver,mode==3?border:read,{{-1,-1,1,1},{mode==2?.75f:.5f,0,0,0}})};
            SourceShadowSubmission submission;submission.casters={&generate,1};submission.receivers={&receive,1};
            SourceShadowDraw second_generate{&generation[0],draw(caster,mode==4?empty:solid,{{-.5f,-.5f,.5f,.5f},{.5f,0,0,0}})};
            SourceShadowDraw second_receive{&application[0],draw(receiver,read,{{-1,-1,1,1},{.5f,0,0,0}})};
            SourceShadowSubmission second;second.casters={&second_generate,1};second.receivers={&second_receive,1};
            std::array<SourceShadowSubmission,2> roots{submission,second};
            gpu.begin();VkClearValue clears[2]{};clears[0].color={{.8f,.6f,.4f,.25f}};clears[1].depthStencil={.5f,0};VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=initial;begin.framebuffer=frame;begin.renderArea={{0,0},extent};begin.clearValueCount=2;begin.pClearValues=clears;vkCmdBeginRenderPass(gpu.command,&begin,VK_SUBPASS_CONTENTS_INLINE);
            render.record(gpu.command,{frame,{0,0,float(extent.width),float(extent.height),0,1},{{0,0},extent}},std::span(roots).first(mode>=4?2:1));vkCmdEndRenderPass(gpu.command);
            gpu.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,scene_pixels);gpu.readback(render.target_image(),{128,128},VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,target_pixels);gpu.end();
            const auto *target_data=static_cast<const unsigned char*>(target_pixels.mapped),*scene_data=static_cast<const unsigned char*>(scene_pixels.mapped);
            for(unsigned y=8;y<128;y+=16)for(unsigned x=8;x<128;x+=16){const bool cast=mode!=1&&mode!=4&&x>=32&&x<96&&y>=32&&y<96;for(unsigned c=0;c<4;++c){const unsigned expected=c==3?(cast?255:0):(cast?0:255);if(target_data[(y*128+x)*4+c]!=expected)throw std::runtime_error("shadow target mode"+std::to_string(mode)+" x"+std::to_string(x)+" y"+std::to_string(y)+" c"+std::to_string(c)+" expected"+std::to_string(expected)+" got"+std::to_string(target_data[(y*128+x)*4+c]));++checks;}}
            for(unsigned y=9;y+9<extent.height;y+=23)for(unsigned x=9;x+9<extent.width;x+=29) {
                const float u=(x+.5f)/extent.width,v=(y+.5f)/extent.height;const float su=mode==3?u*2-.5f:u,sv=mode==3?v*2-.5f:v;
                // Sample well away from the original linear filter boundary.
                if(std::abs(su-.25f)<.03f||std::abs(su-.75f)<.03f||std::abs(sv-.25f)<.03f||std::abs(sv-.75f)<.03f)continue;
                const bool cast=mode!=1&&su>=.25f&&su<.75f&&sv>=.25f&&sv<.75f;
                const std::array<ShaderVector,4> samples{{{cast?0.f:1.f,cast?0.f:1.f,cast?0.f:1.f,cast?1.f:0.f},{1,1,1,1},{},{}}};
                auto factor=halo2_test::scalar_combiners(application[0].material.forward_program.words,samples,{.5f,.5f,.5f,.5f},{});
                for(unsigned c=0;c<4;++c){const unsigned base=std::array{204u,153u,102u,64u}[c];const float expected=c==3||mode==2?float(base):base*factor[c]*(mode==5?factor[c]:1.f);require(std::abs(scene_data[(std::size_t(y)*extent.width+x)*4+c]-expected)<=1.1f,"shadow receiver multiply/world depth/alpha preservation differs");++checks;}
            }
        }
    }
    require(validation_errors==0,"shadow GPU validation errors");std::cout<<checks<<" fixed-target/clear/alpha-kill/receiver/border/depth/resize GPU checks passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
