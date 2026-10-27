#include "gpu_postprocess.hpp"
#include "scalar_combiners.hpp"
#include "helpers/native_world_fragment_addressing_v1.hpp"
#include "halo2/world_material_uniform.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

// Numerical component gate. Unchanged production native FS modules execute on
// declared host images/varyings. A separate CPU scalar combiner and independent
// nearest image/cube addressing supply the expected outputs. No Xbox framebuffer
// or original filtering precision is inferred from this host UNORM comparison.
namespace {
using namespace halo2;
using halo2_test::PostImage;
using Json=nlohmann::json;
namespace fs=std::filesystem;
constexpr unsigned probes_count=32,side=16;
using V=ShaderVector;
using W=std::array<std::uint32_t,60>;
struct Probe {std::array<V,4> coordinates;V color,lighting;};
struct Probes {std::array<std::uint32_t,4> count{probes_count,0,0,0};std::array<Probe,probes_count> values;};
static_assert(sizeof(Probe)==96&&sizeof(Probes)==3088);
void need(bool value,const std::string& why){if(!value)throw FormatError(why);}
std::vector<std::byte> read(const fs::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);need(bool(in),"Missing fragment numerical input: "+path.string());
    const auto size=in.tellg();need(size>=0&&size<=16*1024*1024,"Excessive fragment numerical input");
    std::vector<std::byte> out{std::size_t(size)};in.seekg(0);in.read(reinterpret_cast<char*>(out.data()),out.size());need(bool(in),"Truncated fragment numerical input");return out;
}
std::string sha(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> hash{};unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),hash.data(),&size,EVP_sha256(),nullptr)==1&&size==32,"Cannot hash native fragment numerical input");
    std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto v:hash)out<<std::setw(2)<<unsigned(v);return out.str();
}
std::string key(const W& words) {
    std::array<std::byte,240> bytes{};for(unsigned i=0;i<60;++i)for(unsigned b=0;b<4;++b)bytes[i*4+b]=std::byte((words[i]>>(b*8))&255u);
    return sha(bytes);
}
bool color_word(unsigned i){return (i>=10&&i<26)||i==43||i==44;}
std::vector<PostImage> images(unsigned seed) {
    std::vector<PostImage> out;
    for(unsigned stage=0;stage<4;++stage) {
        PostImage image{stage,side,side};image.filter=VK_FILTER_NEAREST;image.pixels.resize(side*side*4);
        for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)for(unsigned c=0;c<4;++c)
            image.pixels[(y*side+x)*4+c]=std::byte((x*(19+c*8)+y*(43+c*6)+stage*37+seed*53+c*61)%256);
        out.push_back(std::move(image));
    }
    for(unsigned stage=0;stage<4;++stage) {
        PostImage image{5,side,side};image.filter=VK_FILTER_NEAREST;image.view_type=VK_IMAGE_VIEW_TYPE_CUBE;image.array_element=stage;image.pixels.resize(side*side*4*6);
        for(unsigned face=0;face<6;++face)for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)for(unsigned c=0;c<4;++c)
            image.pixels[((face*side+y)*side+x)*4+c]=std::byte((face*31+x*(13+c*7)+y*(23+c*11)+stage*47+seed*29+c*67)%256);
        out.push_back(std::move(image));
    }
    // Retain the complete production descriptor shape, including unused images.
    for(unsigned stage=0;stage<4;++stage) {
        PostImage image{6,1,1};image.filter=VK_FILTER_NEAREST;image.view_type=VK_IMAGE_VIEW_TYPE_3D;image.array_element=stage;
        image.pixels={std::byte{83},std::byte{149},std::byte{211},std::byte{255}};out.push_back(std::move(image));
    }
    return out;
}
V nearest(const PostImage& image,float u,float v,unsigned face=0) {
    need(std::isfinite(u)&&std::isfinite(v),"Nonfinite CPU texture coordinate");
    const auto x=unsigned(std::clamp(int(std::floor(u*image.width)),0,int(image.width)-1));
    const auto y=unsigned(std::clamp(int(std::floor(v*image.height)),0,int(image.height)-1));
    V out{};for(unsigned c=0;c<4;++c)out[c]=float(std::to_integer<unsigned>(image.pixels[((face*image.height+y)*image.width+x)*4+c]))/255.f;return out;
}
V cube(const PostImage& image,V p) {
    const auto x=std::abs(p[0]),y=std::abs(p[1]),z=std::abs(p[2]);float s{},t{},major{};unsigned face{};
    if(x>=y&&x>=z){major=x;t=-p[1];face=p[0]>=0?0:1;s=p[0]>=0?-p[2]:p[2];}
    else if(y>=z){major=y;s=p[0];face=p[1]>=0?2:3;t=p[1]>=0?p[2]:-p[2];}
    else{major=z;t=-p[1];face=p[2]>=0?4:5;s=p[2]>=0?p[0]:-p[0];}
    need(major>0,"Undefined cube direction in CPU fixture");return nearest(image,(s/major+1)*.5f,(t/major+1)*.5f,face);
}
float signed_byte(float value){const auto b=value*255.f;return (b>=128?b-256:b)/127.f;}
std::array<V,4> samples(const W& control,const WorldMaterialUniform& material,const Probe& probe,
    std::span<const PostImage> texture,bool wrong_bump=false) {
    std::array<V,4> out{};
    for(unsigned stage=0;stage<4;++stage) {
        const auto mode=(control[54]>>(stage*5))&31;const auto& coordinate=probe.coordinates[stage];
        if(mode==0)continue;
        if(mode==1) {
            const auto q=(material.alpha_test[3]&(1u<<stage))?1.f:coordinate[3];
            out[stage]=nearest(texture[stage],coordinate[0]/q,coordinate[1]/q);
        } else if(mode==3)out[stage]=cube(texture[4+stage],coordinate);
        else if(mode==6&&stage) {
            const auto previous=stage==1?0:(control[56]>>((stage+2)*4))&3u;
            need(previous<stage,"Independent bump fixture has a future stage input");
            const auto& matrix=material.bump_matrices[stage];
            const auto sign=material.texture_effects[previous][1]>>1;
            const auto ds=(sign&2)?out[previous][2]:signed_byte(out[previous][2]);
            const auto dt=(sign&4)?out[previous][1]:signed_byte(out[previous][1]);
            const auto u=coordinate[0]+(wrong_bump?0.f:matrix[0]*ds+matrix[2]*dt);
            const auto v=coordinate[1]+(wrong_bump?0.f:matrix[1]*ds+matrix[3]*dt);
            out[stage]=nearest(texture[stage],u,v);
        }else throw FormatError("Actual fragment registry gained an unproved texture mode");
    }
    return out;
}
Probes inputs(const W& words,unsigned seed) {
    Probes out;
    for(unsigned i=0;i<probes_count;++i) {
        auto& p=out.values[i];
        for(unsigned stage=0;stage<4;++stage) {
            const auto q=(i+stage+seed)%3==0?2.f:1.f;
            p.coordinates[stage]={float((i*3+stage*5+seed)%13+1)/16.f+.0234375f,
                float((i*7+stage*3+seed*2)%13+1)/16.f+.0390625f,.3125f,q};
        }
        if((words[54]&31)==3) {
            const auto s=float(int((i+seed)%7)-3)*.09375f,t=float(int((i*3+seed)%7)-3)*.0625f;
            switch(i%6) {
                case 0:p.coordinates[0]={1,t,-s,1};break;case 1:p.coordinates[0]={-1,t,s,1};break;
                case 2:p.coordinates[0]={s,1,t,1};break;case 3:p.coordinates[0]={s,-1,-t,1};break;
                case 4:p.coordinates[0]={s,t,1,1};break;case 5:p.coordinates[0]={-s,t,-1,1};break;
            }
        }
        for(unsigned c=0;c<4;++c) {
            p.color[c]=float((i*(31+c*6)+seed*17+c*53)%256)/255.f;
            p.lighting[c]=float((i*(11+c*9)+seed*43+c*71)%256)/255.f;
        }
    }
    return out;
}
std::vector<std::byte> expected(const W& control,const WorldMaterialUniform& material,const Probes& probes,
    std::span<const PostImage> images,bool wrong_bump=false) {
    std::vector<std::byte> out(probes_count*4);
    for(unsigned i=0;i<probes_count;++i) {
        const auto values=samples(control,material,probes.values[i],images,wrong_bump);
        const auto color=halo2_test::scalar_combiners(material.program,values,
            material.flags[2]==3?material.diffuse:probes.values[i].color,probes.values[i].lighting);
        for(unsigned c=0;c<4;++c)out[i*4+c]=std::byte(unsigned(std::lround(std::clamp(color[c],0.f,1.f)*255.f)));
    }
    return out;
}
struct Difference {unsigned maximum{},different{},over_tolerance{};};
Difference difference(std::span<const std::byte> a,std::span<const std::byte> b) {
    need(a.size()==b.size(),"Fragment numerical byte extent differs");Difference out;
    for(unsigned i=0;i<a.size();++i){const auto d=unsigned(std::abs(int(std::to_integer<unsigned>(a[i]))-int(std::to_integer<unsigned>(b[i]))));out.maximum=std::max(out.maximum,d);out.different+=d!=0;out.over_tolerance+=d>1;}return out;
}
}
int main(int argc,char** argv)try {
    need(argc==6,"usage: native_world_fragment_gpu_tests REGISTRY COMPILED_MANIFEST SHADERS PROBE_VS OUTPUT_JSON");
    const auto registry_raw=read(argv[1]),manifest_raw=read(argv[2]);const auto registry=Json::parse(registry_raw),manifest=Json::parse(manifest_raw);
    need(manifest.at("schema")=="HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1"&&manifest.at("registry_sha256")==sha(registry_raw)&&
        manifest.at("runtime_instruction_interpreter")==false,"Fragment cohort is not the actual source-owned native catalog");
    const fs::path shaders=argv[3],vertex=argv[4];std::map<std::string,W> programs;
    for(const auto& row:registry.at("entries")){const auto control=row.at("controls").get<W>();programs.emplace(key(control),control);}
    need(manifest.at("fragment_kernels")==programs.size(),"Native catalog fragment count differs from the complete source registry");
    const std::set<std::string> cohort(halo2_test::native_world_addressing_v1_keys.begin(),halo2_test::native_world_addressing_v1_keys.end());
    Json outside=Json::array();
    // Validate every source-owned compiled module, including those outside the
    // explicitly bounded numerical gate. Never silently filter by texture mode.
    for(const auto& [id,control]:programs) {
        const auto name="native_world/f-"+id+".frag.spv";const auto raw=read(shaders/name);const Json* proof=nullptr;
        for(const auto& stage:manifest.at("stages"))if(stage.at("path")==name){need(!proof,"Ambiguous native fragment catalog module");proof=&stage;}
        need(proof&&proof->at("sha256")==sha(raw)&&proof->at("bytes")==raw.size()&&
            proof->at("contract").at("no_runtime_program_control_reads")==true,
            "Production native fragment bytes differ from their complete compiled contract: "+name);
        if(!cohort.contains(id))outside.push_back({{"key",id},{"path",name},{"numerically_executed_in_this_gate",false},
            {"reason","Outside the fixed source-owned addressing-v1 numerical cohort"}});
    }
    std::vector<std::pair<std::string,W>> ordered;
    for(const auto& id:cohort) {
        need(programs.contains(id),"Source-owned addressing-v1 control identity disappeared: "+id);
        const auto& control=programs.at(id);
        for(unsigned stage=0;stage<4;++stage) {
            const auto mode=(control[54]>>(stage*5))&31;
            need(mode==0||mode==1||mode==3||mode==6,"Pinned addressing-v1 identity has an unproved addressing mode");
        }
        ordered.emplace_back(id,control);
    }
    Json report={{"schema","H2_NATIVE_WORLD_FRAGMENT_GPU_V2"},{"scope","Addressing-v1 bounded component gate: unchanged production native FS modules, declared nearest-filter host images and actual material/varying ABI against separate CPU scalar combiners and modes0/1/3/6 image/cube/bump addressing"},
        {"numerical_cohort","source-owned-world-addressing-v1"},
        {"cohort_original_registry_sha256",halo2_test::native_world_addressing_v1_registry_sha256},
        {"cohort_original_registry_rows",15},{"catalog_fragment_modules",programs.size()},
        {"catalog_module_hash_contracts_verified",programs.size()},
        {"catalog_modules_outside_this_numerical_gate",outside},
        {"complete_catalog_numerical_coverage",outside.empty()},
        {"mandatory_full_map_exact_renderer_gate_remains_required",true},
        {"original_framebuffer_parity",false},{"physical_xbox_precision",false},{"maximum_allowed_unorm_byte_error",1},{"programs",Json::array()}};
    unsigned total{},exact{},maximum{},wrong_modules{},bad_bumps{},poison_invariance{};std::string device;
    for(unsigned program=0;program<ordered.size();++program) {
        const auto& [id,control]=ordered[program];const auto name="native_world/f-"+id+".frag.spv";
        const auto raw=read(shaders/name);const Json* proof=nullptr;for(const auto& stage:manifest.at("stages"))if(stage.at("path")==name){need(!proof,"Ambiguous native fragment catalog module");proof=&stage;}
        need(proof&&proof->at("sha256")==sha(raw)&&proof->at("bytes")==raw.size()&&proof->at("contract").at("no_runtime_program_control_reads")==true,"Production native fragment bytes differ from their compiled contract");
        Json row={{"path",name},{"sha256",sha(raw)},{"cases",Json::array()}};
        for(unsigned seed=0;seed<4;++seed) {
            WorldMaterialUniform material{};material.program=control;material.flags={0,1,1,1};
            for(unsigned i=0;i<60;++i)if(color_word(i))material.program[i]=((i*83+seed*97+47)%256u)<<24|((i*73+seed*47+19)%256u)<<16|((i*29+seed*61+83)%256u)<<8|((i*53+seed*31+149)%256u);
            material.alpha_test[3]=seed&1?15:0;
            for(unsigned stage=1;stage<4;++stage)material.bump_matrices[stage]={.171875f,.109375f,-.140625f,.203125f};
            const auto probes=inputs(control,seed);const auto textures=images(seed);const auto reference=expected(control,material,probes,textures);
            halo2_test::Postprocess renderer;const auto properties=renderer.physical_properties();
            need(properties.deviceType!=VK_PHYSICAL_DEVICE_TYPE_CPU,"Native fragment numerical gate requires a hardware Vulkan device");
            if(device.empty())device=properties.deviceName;
            const auto actual=renderer.render(vertex,shaders/name,4,std::as_bytes(std::span(&material,1)),textures,probes_count,1,
                halo2_test::PostBlend::Disabled,{},std::as_bytes(std::span(&probes,1)),probes_count*6,8);
            const auto delta=difference(actual,reference);maximum=std::max(maximum,delta.maximum);total+=actual.size();exact+=actual.size()-delta.different;
            row["cases"].push_back({{"seed",seed},{"rgba_channels",actual.size()},{"maximum_byte_error",delta.maximum},{"byte_exact_channels",actual.size()-delta.different}});
            need(delta.over_tolerance==0,"Native fragment differs from independent CPU reference: "+name+" seed="+std::to_string(seed)+" maximum="+std::to_string(delta.maximum));
            if(seed==0) {
                // Poison executable control words: production native FS may read
                // only the ordinary animated color words, never these controls.
                auto poison=material;for(unsigned i=0;i<60;++i)if(!color_word(i))poison.program[i]=0xa5f00f5au+i;
                halo2_test::Postprocess poisoned;
                const auto same=poisoned.render(vertex,shaders/name,4,std::as_bytes(std::span(&poison,1)),textures,probes_count,1,
                    halo2_test::PostBlend::Disabled,{},std::as_bytes(std::span(&probes,1)),probes_count*6,8);
                need(actual==same,"Native fragment still consumes uploaded executable control words");++poison_invariance;
                unsigned changed_channels{};
                for(unsigned offset=1;offset<ordered.size()&&!changed_channels;++offset) {
                    const auto wrong_name="native_world/f-"+ordered[(program+offset)%ordered.size()].first+".frag.spv";
                    halo2_test::Postprocess wrong;
                    const auto changed=wrong.render(vertex,shaders/wrong_name,4,std::as_bytes(std::span(&material,1)),textures,probes_count,1,
                        halo2_test::PostBlend::Disabled,{},std::as_bytes(std::span(&probes,1)),probes_count*6,8);
                    const auto bad=difference(changed,reference);changed_channels=bad.over_tolerance;
                    if(changed_channels)row["wrong_actual_module_path"]=wrong_name;
                }
                need(changed_channels>0,"Fragment fixture cannot detect any actual wrong native module");++wrong_modules;
                row["wrong_actual_module_changed_channels_over_tolerance"]=changed_channels;
                if(((control[54]>>5)&31)==6) {
                    const auto bad=difference(actual,expected(control,material,probes,textures,true));
                    need(bad.over_tolerance>0,"Bump fixture cannot detect removed source bump offsets");++bad_bumps;
                    row["wrong_bump_address_changed_channels_over_tolerance"]=bad.over_tolerance;
                }
            }
        }
        report["programs"].push_back(std::move(row));
    }
    need(wrong_modules==14&&poison_invariance==14&&bad_bumps==3,"Native fragment negative-control cohort was incomplete");
    report.update({{"passed",true},{"device",device},{"native_fragment_modules",ordered.size()},{"probe_cases",ordered.size()*4*probes_count},
        {"rgba_channels",total},{"byte_exact_channels",exact},{"maximum_byte_error",maximum},{"wrong_actual_module_controls",wrong_modules},
        {"wrong_bump_address_controls",bad_bumps},{"poisoned_noncolor_control_word_invariance",poison_invariance}});
    std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot save native fragment numerical report");
    std::cout<<"Addressing-v1 component: "<<ordered.size()<<" numerically executed modules of "<<programs.size()
        <<" hash/contract-verified catalog modules; "<<outside.size()<<" outside this numerical gate. "
        <<total<<" RGBA channels, "<<exact<<" byte-exact, max error "<<maximum<<", all independent negative controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
