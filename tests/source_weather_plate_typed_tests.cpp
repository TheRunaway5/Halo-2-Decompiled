#include "halo2/source_weather_plate_caller.hpp"
#include "halo2/source_weather_plate_render.hpp"
#include "source_weather_plate_typed_fixture.hpp"
#include "scalar_combiners.hpp"
#include <bit>
#include <cmath>
#include <functional>
#include <iomanip>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
using namespace halo2_weather_plate_typed_fixture;
using ProgramWords=std::array<std::uint32_t,60>;
struct CallerCase {std::string label;std::array<std::uint32_t,21> input;std::array<std::uint32_t,65> output;};
SourceWeatherPlateCallerInput caller_input(const std::array<std::uint32_t,21> &w) {
    SourceWeatherPlateCallerInput in;std::copy_n(w.begin(),13,in.compositor.words.begin());
    in.depth_start=std::bit_cast<float>(w[13]);in.depth_end=std::bit_cast<float>(w[14]);
    in.selection_near=std::bit_cast<float>(w[15]);in.selection_far=std::bit_cast<float>(w[16]);
    in.projection_near=std::bit_cast<float>(w[17]);in.projection_far=std::bit_cast<float>(w[18]);
    require(w[19]<=65535,"source caller FPCW fixture range");
    in.source_fpcw=static_cast<std::uint16_t>(w[19]);in.source_mxcsr=w[20];return in;
}
std::vector<CallerCase> caller_cases(const std::filesystem::path &root) {
    std::ifstream file(root/"tests/fixtures/retail_source_weather_plate_caller.tsv");require(bool(file),"whole source caller fixture missing");
    std::vector<CallerCase> cases;std::string line,label;
    while(std::getline(file,line)) {
        if(line.empty())continue;
        if(line[0]=='#'){if(line.starts_with("# actual-")||line.starts_with("# controlled-"))label=line.substr(2);continue;}
        std::istringstream row(line);row>>std::hex;CallerCase c;c.label=label;
        for(auto &w:c.input)require(bool(row>>w),"short source caller input");char delimiter;require(bool(row>>delimiter)&&delimiter=='|',"source caller delimiter");
        for(auto &w:c.output)require(bool(row>>w),"short source caller output");std::string extra;require(!(row>>extra),"extra source caller word");
        const auto result=source_weather_plate_caller_profile(caller_input(c.input));
        require(result.program.has_value()==bool(c.output[0])&&unsigned(result.profile)==c.output[1],"native/source caller classification mismatch");
        if(result.program) {
            require(result.depth&&result.projected_zw,"successful caller optional output missing");
            require(std::bit_cast<std::uint32_t>(*result.depth)==c.output[2],"native/source caller depth mismatch");
            for(unsigned i=0;i<2;++i)require(std::bit_cast<std::uint32_t>((*result.projected_zw)[i])==c.output[3+i],"native/source caller quad mismatch");
            require(std::equal(result.program->words.begin(),result.program->words.end(),c.output.begin()+5),"native/source caller full60 words mismatch");
        }else require(!result.depth&&!result.projected_zw,"NoDraw caller retained optional output");
        cases.push_back(c);
    }
    require(cases.size()==14&&cases[0].label=="actual-derived-caller","whole caller fixture scope changed");return cases;
}
std::vector<ProgramWords> source_definitions(const std::filesystem::path &root) {
    std::ifstream file(root/"tests/fixtures/retail_source_weather_plate_program.tsv");require(bool(file),"full source program fixture missing");
    std::set<ProgramWords> unique;std::string line;unsigned rows=0,success=0;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;std::istringstream row(line);row>>std::hex;
        std::array<std::uint32_t,25> w;std::array<std::uint32_t,63> expected;
        for(auto &v:w)require(bool(row>>v),"short source program input");char delimiter;require(bool(row>>delimiter)&&delimiter=='|',"source program delimiter");
        for(auto &v:expected)require(bool(row>>v),"short source program output");std::string extra;require(!(row>>extra),"extra source program word");
        SourceWeatherPlateProgramInput in;std::copy_n(w.begin(),13,in.compositor.words.begin());
        in.depth_start=std::bit_cast<float>(w[13]);in.depth_end=std::bit_cast<float>(w[14]);in.near_clip=std::bit_cast<float>(w[15]);in.far_clip=std::bit_cast<float>(w[16]);
        in.mode=w[17];in.center=w[18];in.anchor_end=w[19];in.anchor_start=w[20];in.skip_state=w[21];
        require(w[22]<=1&&w[23]<=65535,"source program flag/control range");in.depth_requested=w[22];in.source_fpcw=static_cast<std::uint16_t>(w[23]);in.source_mxcsr=w[24];
        const auto result=source_weather_plate_program(in);++rows;
        require(result.program.has_value()==bool(expected[0])&&result.depth.has_value()==bool(expected[1]),"native/source program success mismatch");
        if(result.depth)require(std::bit_cast<std::uint32_t>(*result.depth)==expected[2],"native/source program depth mismatch");
        if(result.program){++success;require(std::equal(result.program->words.begin(),result.program->words.end(),expected.begin()+3),"native/source program full60 mismatch");unique.insert(result.program->words);}
    }
    require(rows==2440&&success==1780&&unique.size()==1651,"full source program corpus scope changed");
    std::cout<<"CPU source evidence: all2440 cases,1780 successful definitions,1651 unique complete60-word programs; whole37A caller14 cases exact\n";
    return {unique.begin(),unique.end()};
}
void set_source_program_and_quad(SourceWeatherPlatePacket &p,const SourceWeatherPlateCallerInput &in) {
    const auto source=source_weather_plate_caller_profile(in);require(source.program&&source.projected_zw,"fixture requires a source quad");
    p.pixel=*source.program;for(auto &v:p.vertices){v.registers[0][2]=(*source.projected_zw)[0];v.registers[0][3]=(*source.projected_zw)[1];}
}
int scene(const std::filesystem::path &root,const std::filesystem::path &output,const std::vector<CallerCase> &cases) {
    const auto inputs=root/"analysis/multiplayer-shading/weather-plate-original-reference/event-215778";
    const auto code=read(root/"analysis/multiplayer-shading/weather-gpu-assets/lockout-global21-class0.vp");
    const auto vertices=decode<SourceWeatherPlateVertex>(read(inputs/"actual-four-vertices.bin"));
    const auto scroll=decode<ShaderVector>(read(inputs/"actual-scroll-c21-c23.bin")),clip=decode<ShaderVector>(read(inputs/"actual-clip-range.bin"));
    const auto regions=decode<std::array<std::int32_t,4>>(read(inputs/"actual-clip-regions.bin"));
    const auto ps=decode<std::uint32_t>(read(inputs/"source-mode3-pixel-definition.bin"));
    require(vertices.size()==4&&scroll.size()==3&&clip.size()==1&&regions.size()==8&&ps.size()==60,"actual captured packet incomplete");
    const auto input21=read(root/"analysis/multiplayer-shading/weather-plate-typed-prepare-review/actual-source-caller-input21.bin");
    require(input21.size()==sizeof(cases[0].input)&&!std::memcmp(input21.data(),cases[0].input.data(),input21.size()),"actual caller input authority mismatch");
    const auto actual=caller_input(cases[0].input);const auto actual_source=source_weather_plate_caller_profile(actual);
    require(actual_source.program&&std::equal(ps.begin(),ps.end(),actual_source.program->words.begin()),"actual source-derived full60 differs from original event");
    for(const auto &v:vertices)for(unsigned i=0;i<2;++i)require(std::bit_cast<std::uint32_t>(v.registers[0][2+i])==cases[0].output[3+i],"actual source-derived Z/W differs from original event");
    const auto before=read(inputs/"before-scene-color.rgba"),original=read(inputs/"after-scene-color.rgba"),low24=read(inputs/"before-depth-d24-low24.bin"),source_depth=read(inputs/"before-scene-depth.raw");
    const auto prior=read(root/"analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba");
    require(before.size()==640*480*4&&original.size()==before.size()&&prior.size()==before.size(),"actual/prior scene byte count mismatch");
    Gpu g;if(!g.initialize())return 77;
    const VkExtent2D extent{640,480};const auto color=g.image(extent,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT),depth=g.image(extent,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    upload_color(g,color,before,VK_IMAGE_LAYOUT_UNDEFINED);upload_depth(g,depth,low24,source_depth);verify_depth(g,depth,low24,source_depth);
    const auto pass=g.scene_pass(false,VK_FORMAT_D24_UNORM_S8_UINT);const auto frame=g.framebuffer(pass,extent,color,depth);
    SourceWeatherPlateRender renderer({g.physical,g.device,pass,HALO2_WEATHER_PLATE_SHADER_DIR,true});SourceWeatherPlatePacket packet;
    packet.vertex_program=code;std::copy(vertices.begin(),vertices.end(),packet.vertices.begin());std::copy(scroll.begin(),scroll.end(),packet.scroll.begin());std::copy(ps.begin(),ps.end(),packet.pixel.words.begin());std::copy(regions.begin(),regions.end(),packet.clip_regions.begin());
    packet.clip_range=clip[0];packet.surface={640,480,0,0};packet.depth_clamp=true;packet.depth_clipping=true;
    for(unsigned i=0;i<4;++i){const VkExtent2D size=i?VkExtent2D{256,256}:extent;const unsigned mips=i?7:1;const auto texture=g.image(size,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,mips);upload_texture(g,texture,inputs,i,size.width);VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};ci.magFilter=ci.minFilter=i?VK_FILTER_LINEAR:VK_FILTER_NEAREST;ci.mipmapMode=i?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;ci.addressModeU=ci.addressModeV=ci.addressModeW=i?VK_SAMPLER_ADDRESS_MODE_REPEAT:VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;ci.minLod=-1000;ci.maxLod=1000;VkSampler sampler;check(vkCreateSampler(g.device,&ci,nullptr,&sampler));g.samplers.push_back(sampler);packet.textures[i]={texture.view,sampler,VK_FORMAT_R8G8B8A8_UNORM,size,mips};}
    auto out=g.buffer(before.size(),VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    const auto record=[&](){g.begin();VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};bi.renderPass=pass;bi.framebuffer=frame;bi.renderArea={{0,0},extent};vkCmdBeginRenderPass(g.command,&bi,VK_SUBPASS_CONTENTS_INLINE);renderer.record(g.command,{0,0,640,480,0,1},{{0,0},extent});vkCmdEndRenderPass(g.command);g.readback(color.handle,extent,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,out);g.end();std::vector<std::byte> result(out.size);std::memcpy(result.data(),out.mapped,out.size);return result;};
    renderer.prepare(actual,packet);const auto native=record();require(native==prior,"typed actual draw changed retained prior native bytes");
    unsigned maximum=0,different=0;std::uint64_t total=0;
    for(unsigned i=0;i<native.size();i+=4){require(native[i+3]==before[i+3],"typed actual draw changed original target alpha");different+=!std::equal(native.begin()+i,native.begin()+i+4,original.begin()+i);for(unsigned c=0;c<3;++c){unsigned d=unsigned(std::abs(int(std::to_integer<unsigned>(native[i+c]))-int(std::to_integer<unsigned>(original[i+c]))));maximum=std::max(maximum,d);total+=d;}}
    require(maximum<=1,"typed actual draw exceeds prior original-host one-byte residual");save(output/"typed-actual-after-scene-color.rgba",native);verify_depth(g,depth,low24,source_depth);
    unsigned rejected=0;std::ofstream controls(output/"typed-retained-controls.tsv");controls<<"control\trejected\tretained_color_equal\tD24_stencil_equal\n";
    const auto reject=[&](const std::string &label,const SourceWeatherPlateCallerInput &in,const SourceWeatherPlatePacket &bad){bool caught=false;try{renderer.prepare(in,bad);}catch(const FormatError&){caught=true;}require(caught,"unsupported typed profile/packet admitted");++rejected;
        // No valid prepare intervenes: record the retained actual descriptor
        // immediately after each rejection, with the original target reset.
        upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(record()==native,"typed rejection mutated retained actual bindings");verify_depth(g,depth,low24,source_depth);controls<<label<<"\t1\t1\t1\n";};
    for(unsigned i:{1u,2u,3u}){const auto in=caller_input(cases[i].input);auto bad=packet;if(i!=3)set_source_program_and_quad(bad,in);reject(cases[i].label,in,bad);}
    auto in=actual;in.selection_near=10;reject("same-PS-and-ZW-near-depth-disabled",in,packet);
    in=actual;in.projection_far=-1024;auto bad=packet;set_source_program_and_quad(bad,in);require(bad.pixel.words==packet.pixel.words&&bad.vertices[0].registers[0][3]==0,"source W0 rejection authority changed");reject("same-PS-projection-clamps-W0",in,bad);
    bad=packet;bad.pixel.words[54]^=1;reject("forged-full60-PS",actual,bad);
    bad=packet;bad.vertices[0].registers[0][2]=std::nextafter(bad.vertices[0].registers[0][2],0.f);reject("forged-quad-Z-bit",actual,bad);
    bad=packet;bad.vertices[3].registers[0][3]=std::nextafter(bad.vertices[3].registers[0][3],0.f);reject("forged-quad-W-bit",actual,bad);
    in=caller_input(cases[4].input);reject("projection-bank-packet-mismatch",in,packet);
    in=caller_input(cases[5].input);reject("selection-bank-program-mismatch",in,packet);
    in=actual;in.compositor.words[0]=(in.compositor.words[0]&~255u)|2u;reject("literal-flag2",in,packet);
    in=actual;in.compositor.words[1]=0x80000000;reject("negative-zero-X-lane",in,packet);
    in=actual;in.compositor.words[12]=0x3f800001;reject("tint-outside-caller-clamp",in,packet);
    in=actual;in.source_mxcsr|=0x8000;reject("unsupported-source-FTZ",in,packet);
    in=actual;in.projection_far=std::numeric_limits<float>::infinity();reject("nonfinite-projection-bank",in,packet);
    bad=packet;bad.textures[0].format=VK_FORMAT_D32_SFLOAT;reject("bad-packed-alias-format",actual,bad);
    bad=packet;bad.surface[0]=640.5f;reject("noninteger-full-surface",actual,bad);
    bad=packet;bad.vertices[0].registers[0][0]=std::numeric_limits<float>::max();reject("derived-screen-overflow",actual,bad);
    bad=packet;bad.vertices[0].registers[2][0]=std::numeric_limits<float>::max();bad.scroll[0][0]=std::numeric_limits<float>::max();reject("derived-scroll-overflow",actual,bad);
    // Padding is real captured stack data and must not be treated as a flag.
    in=actual;in.compositor.words[0]=0xffffff01;renderer.prepare(in,packet);upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(record()==native,"ignored caller padding changed native bindings");
    // All above-near whole-caller controls go through real typed graphics.
    // These are experiments, with no original raster readback claim.
    unsigned broader=0;for(unsigned i=4;i<cases.size();++i){const auto c=caller_input(cases[i].input);bad=packet;set_source_program_and_quad(bad,c);renderer.prepare(c,bad);upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);const auto control=record();for(unsigned j=3;j<control.size();j+=4)require(control[j]==before[j],"typed class control wrote target alpha");verify_depth(g,depth,low24,source_depth);++broader;}
    // Legacy admission remains exact captured60, even after typed prepare.
    bool legacy_caught=false;try{renderer.prepare(bad);}catch(const FormatError&){legacy_caught=true;}require(legacy_caught,"legacy preparation admitted a generalized source definition");
    renderer.prepare(packet);upload_color(g,color,before,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);require(record()==native,"legacy captured packet changed after typed path");
    require(rejected==19&&broader==10&&validation_errors==0,"typed scene controls/validation incomplete");
    std::ofstream report(output/"typed-scene-result.json");report<<"{\n  \"prior_native_bytes_identical\": true,\n  \"original_host_max_RGB_byte_residual\": "<<maximum<<",\n  \"original_host_differing_pixels\": "<<different<<",\n  \"original_host_total_RGB_absolute\": "<<total<<",\n  \"rejected_with_direct_retained_readback\": "<<rejected<<",\n  \"whole_caller_graphics_controls\": "<<broader<<",\n  \"D24_stencil_and_alpha_unchanged\": true,\n  \"validation_errors\": "<<validation_errors<<",\n  \"scope\": \"Only retained actual has original scene readback; ten controls prove typed graphics admission and preservation, not original raster equivalence.\"\n}\n";
    std::cout<<"Typed actual: prior native bytes identical, original-host max RGB residual "<<maximum<<" byte;19 rejected profiles/packets each retain bindings with direct readback; ten whole-caller graphics controls; legacy guard retained\n";return 0;
}
struct alignas(16) Probe {ProgramWords program;std::array<ShaderVector,18> constants;std::array<ShaderVector,4> samples;};
static_assert(sizeof(Probe)==592);
std::array<ShaderVector,4> samples(unsigned index) {
    const unsigned boundaries[]{0,1,127,128,254,255,13,201};std::uint32_t random=0x37a60u+index*0x9e3779b9u;
    std::array<ShaderVector,4> result{};
    for(unsigned stage=0;stage<4;++stage)for(unsigned c=0;c<4;++c) {
        random^=random<<13;random^=random>>17;random^=random<<5;
        const unsigned b=index<8?boundaries[(index+stage*3+c)%8]:random&255u;result[stage][c]=float(b)/255.f;
    }return result;
}
int combiner(const std::filesystem::path &output,const std::vector<ProgramWords> &definitions) {
    std::vector<Probe> input;std::vector<ShaderVector> expected;for(const auto &w:definitions)for(unsigned i=0;i<32;++i){
        Probe p;p.program=w;p.samples=samples(i);
        const auto expand=[](std::uint32_t v){return ShaderVector{float((v>>16)&255)/255.f,float((v>>8)&255)/255.f,float(v&255)/255.f,float(v>>24)/255.f};};
        for(unsigned stage=0;stage<8;++stage){p.constants[stage*2]=expand(w[10+stage]);p.constants[stage*2+1]=expand(w[18+stage]);}
        p.constants[16]=expand(w[43]);p.constants[17]=expand(w[44]);
        input.push_back(p);expected.push_back(halo2_test::scalar_combiners(w,p.samples));
    }
    require(input.size()==52832,"combiner diagnostic corpus scope changed");Gpu g;if(!g.initialize())return 77;
    auto src=g.buffer(input.size()*sizeof(Probe),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),dst=g.buffer(expected.size()*sizeof(ShaderVector),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);std::memcpy(src.mapped,input.data(),src.size);
    VkDescriptorSetLayoutBinding bindings[2]{{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr},{1,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr}};
    VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};dl.bindingCount=2;dl.pBindings=bindings;check(vkCreateDescriptorSetLayout(g.device,&dl,nullptr,&g.descriptor_layout));
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&g.descriptor_layout;check(vkCreatePipelineLayout(g.device,&pl,nullptr,&g.layout));
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=1;dp.pPoolSizes=&size;check(vkCreateDescriptorPool(g.device,&dp,nullptr,&g.descriptors));
    VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};da.descriptorPool=g.descriptors;da.descriptorSetCount=1;da.pSetLayouts=&g.descriptor_layout;VkDescriptorSet descriptor;check(vkAllocateDescriptorSets(g.device,&da,&descriptor));
    VkDescriptorBufferInfo bi[2]{{src.handle,0,src.size},{dst.handle,0,dst.size}};VkWriteDescriptorSet writes[2]{};for(unsigned i=0;i<2;++i){writes[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;writes[i].dstSet=descriptor;writes[i].dstBinding=i;writes[i].descriptorCount=1;writes[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;writes[i].pBufferInfo=&bi[i];}vkUpdateDescriptorSets(g.device,2,writes,0,nullptr);
    const auto bytes=read(std::filesystem::path(HALO2_WEATHER_PLATE_SHADER_DIR)/"source_weather_plate_combiner_probe.comp.spv");const auto code=decode<std::uint32_t>(bytes);VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};sm.codeSize=bytes.size();sm.pCode=code.data();VkShaderModule shader;check(vkCreateShaderModule(g.device,&sm,nullptr,&shader));g.modules.push_back(shader);
    VkComputePipelineCreateInfo pi{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};pi.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_COMPUTE_BIT,shader,"main",nullptr};pi.layout=g.layout;VkPipeline pipeline;check(vkCreateComputePipelines(g.device,0,1,&pi,nullptr,&pipeline));g.pipelines.push_back(pipeline);
    g.begin();VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;host.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&host,0,nullptr,0,nullptr);
    vkCmdBindPipeline(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);vkCmdBindDescriptorSets(g.command,VK_PIPELINE_BIND_POINT_COMPUTE,g.layout,0,1,&descriptor,0,nullptr);vkCmdDispatch(g.command,(input.size()+63)/64,1,1);
    host.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(g.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);g.end();
    const auto *actual=static_cast<const ShaderVector*>(dst.mapped);float maximum=0;unsigned byte_max=0,round_threshold_mismatches=0;std::size_t worst=0;
    for(unsigned i=0;i<expected.size();++i){for(unsigned c=0;c<4;++c){require(std::isfinite(actual[i][c]),"nonfinite native combiner output");float d=std::abs(actual[i][c]-expected[i][c]);if(d>maximum){maximum=d;worst=i;}byte_max=std::max(byte_max,unsigned(std::abs(int(std::round(actual[i][c]*255.f))-int(std::round(expected[i][c]*255.f)))));}
        round_threshold_mismatches+=(std::round(actual[i][3]*255.f)>0)!=(std::round(expected[i][3]*255.f)>0);
    }
    require(maximum<=0.000002f&&byte_max<=1&&round_threshold_mismatches==0&&validation_errors==0,"native combiner/source-definition scalar oracle exceeds diagnostic bound");
    save(output/"combiner-native-output.rgba32f",std::as_bytes(std::span(actual,expected.size())));save(output/"combiner-scalar-output.rgba32f",std::as_bytes(std::span(expected)));save(output/"combiner-source-inputs.bin",std::as_bytes(std::span(input)));
    std::ofstream report(output/"combiner-result.json");report<<std::setprecision(10)<<"{\n  \"source_definitions\": 1651,\n  \"RGBA_byte_probe_sets_per_definition\": 32,\n  \"outputs\": "<<expected.size()<<",\n  \"maximum_float_absolute_error\": "<<maximum<<",\n  \"worst_probe\": "<<worst<<",\n  \"maximum_rounded_byte_error\": "<<byte_max<<",\n  \"rounded_alpha_GT0_disagreements\": "<<round_threshold_mismatches<<",\n  \"validation_errors\": "<<validation_errors<<",\n  \"scope\": \"Test-only native unchanged generic combiner against separate CPU scalar evaluation of complete original source definitions; no public arbitrary-word admission and no original raster equivalence claim.\"\n}\n";
    std::cout<<"Native generic combiner:1651 source definitions x32 byte-sample sets =52832 outputs, max float error "<<maximum<<", max rounded byte error "<<byte_max<<", alpha GT0 disagreements "<<round_threshold_mismatches<<"; test-only coverage, no original raster claim\n";return 0;
}
}
int main(int argc,char **argv){try {
    require(argc<=3,"expected [--cpu|--gpu-scene|--gpu-combiner|--gpu] [private-output-directory]");
    const std::string mode=argc>1?argv[1]:"--cpu";require(mode=="--cpu"||mode=="--gpu-scene"||mode=="--gpu-combiner"||mode=="--gpu","unknown typed plate test mode");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR),output=argc>2?std::filesystem::path(argv[2]):root/"analysis/multiplayer-shading/weather-plate-typed-preparation/test-output";
    const auto callers=caller_cases(root);const auto definitions=source_definitions(root);if(mode=="--cpu")return 0;std::filesystem::create_directories(output);
    if(mode=="--gpu-scene"||mode=="--gpu"){int r=scene(root,output,callers);if(r)return r;}
    if(mode=="--gpu-combiner"||mode=="--gpu")return combiner(output,definitions);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
