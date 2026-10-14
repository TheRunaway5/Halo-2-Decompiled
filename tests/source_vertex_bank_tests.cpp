#include "halo2/vertex_program.hpp"
#include "halo2/render_view.hpp"
#include "halo2/source_vertex_declaration.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
constexpr std::array<unsigned,7> outputs{0,3,4,9,10,11,12};
struct CameraData {
    std::array<ShaderVector,4> projection{};
    ShaderVector forward{},position{},viewport{};
};
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,6> fog{};
    std::array<ShaderVector,16> material{};
    PackedVertexProgramBank bank;
    CameraData camera;
    std::array<std::uint32_t,4> control{};
    ShaderVector decoded_position{},decoded_uv{},decoded_normal{};
    PackedSourceVertexDeclaration declaration=legacy_source_vertex_declaration;
};
static_assert(sizeof(Fixture)==5536 && offsetof(Fixture,bank)==2800 && offsetof(Fixture,camera)==5344);
struct Reference {
    unsigned event{},vertex{};
    ShaderVector conversion{};
    std::array<ShaderVector,192> constants{};
    std::array<ShaderVector,7> values{};
};
struct Sample {Fixture fixture;Reference reference;};
constexpr std::array<std::array<unsigned,2>,4> host_extents{{{128,128},{320,240},{1920,1080},{3840,2160}}};
void check(bool v,const std::string&m){if(!v)throw std::runtime_error(m);}
float word(std::istream&s){std::string t;check(bool(s>>t)&&t.size()==8,"invalid source bank fixture word");
    std::size_t n;const auto b=std::stoul(t,&n,16);check(n==8&&b<=0xfffffffful,"invalid source bank fixture bits");return std::bit_cast<float>(std::uint32_t(b));}
bool exact(const ShaderVector&a,const ShaderVector&b){return std::bit_cast<std::array<std::uint32_t,4>>(a)==std::bit_cast<std::array<std::uint32_t,4>>(b);}
void require_validation_environment(){
    const char*layers=std::getenv("VK_INSTANCE_LAYERS");const char*sync=std::getenv("VK_LAYER_VALIDATE_SYNC");
    const char*enables=std::getenv("VK_LAYER_ENABLES");
    check(layers&&std::string_view(layers).find("VK_LAYER_KHRONOS_validation")!=std::string_view::npos,"source bank GPU test requires Vulkan validation layer");
    check((sync&&std::string_view(sync)=="1")||(enables&&std::string_view(enables).find("SYNCHRONIZATION_VALIDATION")!=std::string_view::npos),"source bank GPU test requires synchronization validation");
    unsigned count=0;halo2_test::vkcheck(vkEnumerateInstanceLayerProperties(&count,nullptr));std::vector<VkLayerProperties> available(count);
    halo2_test::vkcheck(vkEnumerateInstanceLayerProperties(&count,available.data()));
    check(std::any_of(available.begin(),available.end(),[](const auto&l){return std::string_view(l.layerName)=="VK_LAYER_KHRONOS_validation";}),"required Vulkan validation layer unavailable");
}
CameraData host_camera(unsigned width,unsigned height){
    CameraData c;c.projection={ShaderVector{2,0,0,0},ShaderVector{0,4,0,0},ShaderVector{0,0,4,0},ShaderVector{17,23,31,1}};
    c.forward={0,1,0,1};c.position={101,202,303,2};c.viewport={7,11,float(width),float(height)};return c;
}
ShaderVector ordinary_camera_constant(unsigned index,const CameraData&c){
    if(index<4){ShaderVector v{};for(unsigned axis=0;axis<4;++axis)v[axis]=c.projection[axis][index]*(index==1?-1.f:1.f);return v;}
    if(index==4)return {1,0,0,1};if(index==5)return {-0.f,-1,-0.f,.5f};
    if(index==6)return {-c.forward[0],-c.forward[1],-c.forward[2],2};
    if(index==7)return {c.position[0],c.position[1],c.position[2],767.8125f};
    return viewport_constants({c.viewport[0],c.viewport[1],c.viewport[2],c.viewport[3]})[index-8];
}
void packer_controls(){
    VertexProgramBank bank;bank.first_constant=0;bank.camera_inputs=true;bank.constants.resize(49);
    const auto packed=pack_vertex_program_bank(bank);
    check(packed.range==std::array<std::uint32_t,4>{0,49,0,1},"actual camera flag/range was not packed");
    for(unsigned i=49;i<158;++i)check(exact(packed.constants[i],{}),"unused packed constants were not zeroed");
    std::vector<VertexProgramBank> invalid;
    auto add=[&](unsigned first,unsigned count,bool camera){VertexProgramBank b;b.first_constant=first;b.constants.resize(count);b.camera_inputs=camera;invalid.push_back(b);};
    add(0,0,true);add(0,11,true);add(1,12,true);add(34,12,true);add(0,159,true);add(193,0,false);add(190,3,false);add(33,1,false);add(0,12,false);
    for(const auto&b:invalid){bool rejected=false;try{(void)pack_vertex_program_bank(b);}catch(const std::exception&){rejected=true;}
        check(rejected,"production packer accepted malformed/incomplete source camera bank");}
    VertexProgramBank empty;check(pack_vertex_program_bank(empty).range==std::array<std::uint32_t,4>{34,0,0,0},"ordinary empty bank rejected");
}
std::vector<Sample> receiver_samples(const std::filesystem::path&root){
    std::ifstream file(root/"tests/fixtures/retail_shadow_vertex.tsv");check(bool(file),"original receiver fixture missing");
    std::map<unsigned,Sample> banks;std::map<unsigned,unsigned> counts;std::set<unsigned> admitted;std::vector<Sample> samples;std::string line;
    while(std::getline(file,line)){
        if(line.empty()||line[0]=='#')continue;std::istringstream f(line);char kind;unsigned event,index;check(bool(f>>kind>>event>>index),"truncated original bank record");
        if(kind=='B'){
            unsigned ordinal;check(bool(f>>ordinal),"missing source receiver class");
            ShaderVector conversion{};for(unsigned i=0;i<3;++i)conversion[i]=word(f);
            if(index){check(ordinal==0,"source receiver is not world class0");admitted.insert(event);auto&b=banks[event];b.reference.event=event;b.reference.conversion=conversion;
                std::ifstream code(root/"tests/fixtures/retail_shadow_apply_class0.vp",std::ios::binary);check(bool(code),"source receiver program missing");
                const std::string bytes((std::istreambuf_iterator<char>(code)),{});const auto p=decode_vertex_program({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});
                b.fixture.instructions=p.instructions;b.fixture.flags[3]=p.count;
            }
        }else if(kind=='C'){
            ShaderVector value;for(auto&v:value)v=word(f);
            if(admitted.contains(event)){check(index==counts[event]++&&index<192,"source receiver constants incomplete");banks[event].reference.constants[index]=value;}
        }else if(kind=='V'){
            std::array<ShaderVector,16> attributes;std::array<ShaderVector,7> values;
            for(auto&a:attributes)for(auto&v:a)v=word(f);for(auto&o:values)for(auto&v:o)v=word(f);
            if(admitted.contains(event)){check(counts[event]==192,"source receiver bank lacks192 registers");auto s=banks.at(event);s.reference.vertex=index;s.reference.values=values;s.fixture.attributes=attributes;samples.push_back(s);}
        }else throw std::runtime_error("unknown source bank record");
        std::string extra;check(!(f>>extra),"trailing original bank input");
    }
    check(admitted==std::set<unsigned>{7988,7993,7998,8003}&&samples.size()==10,"actual source receiver coverage changed");return samples;
}
void bind_camera(Sample&s){
    VertexProgramBank bank;bank.first_constant=0;bank.camera_inputs=true;bank.constants.resize(49);
    for(unsigned i=0;i<12;++i)bank.constants[i]=s.reference.constants[i];
    for(unsigned i=12;i<34;++i)bank.constants[i]={-901.f-float(i),-902,-903,-904};
    for(unsigned i=34;i<49;++i)bank.constants[i]=s.reference.constants[i];
    s.fixture.bank=pack_vertex_program_bank(bank);
    for(unsigned i=0;i<6;++i)s.fixture.fog[i]=s.reference.constants[i+12];
    for(unsigned i=0;i<16;++i)s.fixture.material[i]=s.reference.constants[i+18];
    s.fixture.control[0]=1;
}
ShaderVector bound_constant(const Fixture&f,unsigned i){
    if(i<12&&f.bank.range[3])return f.bank.constants[i];
    if(i<7&&f.control[0])return f.camera.position;
    if(i<12)return ordinary_camera_constant(i,f.camera);
    if(i<18&&f.camera.forward[3]!=0)return f.fog[i-12];
    if(i>=18&&i<34)return f.material[i-18];
    if(i==172)return {1,1,0,0};
    if(i>=f.bank.range[0]&&i-f.bank.range[0]<f.bank.range[1])return f.bank.constants[i-f.bank.range[0]];
    return {};
}
void compare(std::span<const ShaderVector>result,const std::vector<Sample>&samples,bool gpu){
    check(result.size()==samples.size()*16,"receiver output count differs");std::array<unsigned,7> maximum{},differing{};
    for(unsigned n=0;n<samples.size();++n)for(unsigned o=0;o<7;++o){
        auto value=result[n*16+outputs[o]];const auto&reference=samples[n].reference;
        if(o==0){for(unsigned axis=0;axis<2;++axis)value[axis]=((std::trunc(value[axis]*16.f)/16.f)*2.f-reference.conversion[axis])/reference.conversion[axis];
            value[2]/=reference.conversion[2];for(unsigned axis=0;axis<3;++axis)value[axis]*=value[3];}
        for(unsigned axis=0;axis<4;++axis){const auto expected=reference.values[o][axis];check(std::isfinite(value[axis])&&std::isfinite(expected),"nonfinite source receiver output");
            const auto a=std::bit_cast<std::uint32_t>(value[axis]),b=std::bit_cast<std::uint32_t>(expected);
            const auto ulp=(a>>31)==(b>>31)?(a>b?a-b:b-a):0xffffffffu;maximum[o]=std::max(maximum[o],ulp);differing[o]+=a!=b;
            check(a==b||(gpu&&o==0&&ulp<=4),std::string(gpu?"GPU":"CPU")+" actual bank receiver mismatch event"+std::to_string(reference.event)+" vertex"+std::to_string(reference.vertex)+" output"+std::to_string(outputs[o])+" axis"+std::to_string(axis)+" ULP"+std::to_string(ulp));
        }
    }
    for(unsigned o=0;o<7;++o)std::cout<<(gpu?"GPU":"CPU")<<" source-bank o"<<outputs[o]<<" differing "<<differing[o]<<" maxULP "<<maximum[o]<<'\n';
}
std::array<ShaderVector,82> original_prt_constants(const std::filesystem::path&root){
    std::ifstream f(root/"tests/fixtures/retail_prt_rigid_gpu.tsv");check(bool(f),"original PRT bank fixture missing");
    std::array<ShaderVector,82> result{};std::string line;unsigned first=0,count=0;
    while(std::getline(f,line)){if(line.empty()||line[0]=='#')continue;std::istringstream s(line);char kind;unsigned event,index;check(bool(s>>kind>>event>>index),"bad original PRT bank record");
        if(kind!='C')continue;if(!first)first=event;if(event!=first)break;
        ShaderVector v;for(auto&c:v)c=word(s);if(index>=68&&index<150){result[index-68]=v;++count;}}
    check(count==82,"actual PRT coefficient controls incomplete");
    check(std::any_of(result.begin(),result.end(),[](const auto&v){return !exact(v,{});}),"actual PRT control bank is empty");return result;
}
VertexProgram program(const Fixture&f){VertexProgram p;p.instructions=f.instructions;p.count=f.flags[3];return p;}
VertexProgramState source_state(const Fixture&f){VertexProgramState s;s.attributes=f.attributes;
    for(unsigned i=0;i<192;++i)s.constants[i]=bound_constant(f,i);execute_vertex_program(program(f),s);return s;}
std::vector<Sample> production_receivers(const std::vector<Sample>&captured){
    std::vector<Sample> result;
    for(const auto size:host_extents)for(const auto&s:captured)for(unsigned poison=0;poison<2;++poison){
        auto sample=s;bind_camera(sample);auto&f=sample.fixture;f.camera=host_camera(size[0],size[1]);
        f.flags[2]=10;f.control[3]=1;
        f.declaration=pack_source_vertex_declaration(SourceVertexDeclaration{0,{0,4}});
        f.decoded_position=f.attributes[0];f.decoded_normal=f.attributes[1];
        f.decoded_uv=poison?ShaderVector{-17,23,0,1}:ShaderVector{.125f,-.75f,0,1};
        check(!exact(f.decoded_uv,f.decoded_normal),"UV poison does not distinguish the captured normal");
        result.push_back(sample);
    }
    check(result.size()==80,"production receiver coverage changed");return result;
}
std::vector<Fixture> controlled_generations(const std::filesystem::path&root){
    // No retained executed generation class0 exists. The original authored
    // world-class0 program is tested with explicit controlled world/UV inputs
    // and the independently captured generation7758 constant bank. Its c172
    // is identity because production supplies already decompressed world UVs.
    std::ifstream file(root/"tests/fixtures/retail_shadow_vertex.tsv");check(bool(file),"original generation bank missing");
    Sample bank;std::string line;unsigned count=0;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;
        std::istringstream s(line);char kind;unsigned event,index;check(bool(s>>kind>>event>>index),"invalid generation record");
        if(kind=='C'&&event==7758){check(index==count++&&index<192,"generation7758 bank incomplete");for(auto&v:bank.reference.constants[index])v=word(s);}}
    check(count==192,"generation7758 lacks192 constants");
    std::ifstream code(root/"tests/fixtures/retail_shadow_generate_world_class0.vp",std::ios::binary);check(bool(code),"authored generation world0 program missing");
    const std::string bytes((std::istreambuf_iterator<char>(code)),{});const auto p=decode_vertex_program({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});
    check(p.count==14,"authored generation world0 instruction count changed");bank.fixture.instructions=p.instructions;bank.fixture.flags[3]=p.count;bind_camera(bank);
    std::vector<Fixture> result;
    for(const auto size:host_extents)for(unsigned uv=0;uv<2;++uv)for(unsigned poison=0;poison<2;++poison){
        auto f=bank.fixture;f.camera=host_camera(size[0],size[1]);f.flags[2]=10;f.control[3]=0;
        f.declaration=pack_source_vertex_declaration(SourceVertexDeclaration{0,{0,3}});
        f.decoded_position={1.25f,-.5f,2,1};f.decoded_uv=uv?ShaderVector{.875f,.625f,0,1}:ShaderVector{.125f,.25f,0,1};
        f.decoded_normal=poison?ShaderVector{17,-23,31,1}:ShaderVector{-1,0,0,1};
        f.attributes[0]=f.decoded_position;f.attributes[1]=f.decoded_uv;f.attributes[2]=f.decoded_normal;result.push_back(f);
    }
    check(result.size()==16,"controlled generation coverage changed");return result;
}
void controlled_generation_cpu(const std::vector<Fixture>&fixtures){
    for(unsigned n=0;n<fixtures.size();n+=4){
        const auto a=source_state(fixtures[n]),poison=source_state(fixtures[n+1]),uv=source_state(fixtures[n+2]);
        for(unsigned i:outputs)check(exact(a.outputs[i],poison.outputs[i]),"controlled source generation unexpectedly consumes the normal");
        check(!exact(a.outputs[9],uv.outputs[9]),"controlled source generation UV control has no response");
        check(exact(a.outputs[0],uv.outputs[0]),"controlled source generation UV changes its position");
    }
}
void compare_controlled_generation(std::span<const ShaderVector>gpu,const std::vector<Fixture>&fixtures){
    check(gpu.size()==fixtures.size()*208,"controlled generation output count differs");
    std::array<unsigned,7> maximum{},differing{};
    for(unsigned n=0;n<fixtures.size();++n){const auto expected=source_state(fixtures[n]);
        for(unsigned o=0;o<7;++o)for(unsigned axis=0;axis<4;++axis){const auto value=gpu[n*208+outputs[o]][axis];
            const auto a=std::bit_cast<std::uint32_t>(value),b=std::bit_cast<std::uint32_t>(expected.outputs[outputs[o]][axis]);
            const auto ulp=(a>>31)==(b>>31)?(a>b?a-b:b-a):0xffffffffu;maximum[o]=std::max(maximum[o],ulp);differing[o]+=a!=b;
            check(std::isfinite(value)&&std::isfinite(expected.outputs[outputs[o]][axis]),"nonfinite controlled generation output");
            check(a==b||(o==0&&ulp<=4),"controlled generation GPU mismatch o"+std::to_string(outputs[o])+" axis"+std::to_string(axis)+" ULP"+std::to_string(ulp));
        }
    }
    for(unsigned n=0;n<fixtures.size();n+=4){
        for(unsigned i:outputs)check(exact(gpu[n*208+i],gpu[(n+1)*208+i]),"production generation consumed poison normal");
        check(!exact(gpu[n*208+9],gpu[(n+2)*208+9]),"production generation UV control has no response");
        check(exact(gpu[n*208],gpu[(n+2)*208]),"production generation UV changes its position");
    }
    for(unsigned o=0;o<7;++o)std::cout<<"GPU controlled generation o"<<outputs[o]<<" differing "<<differing[o]<<" maxULP "<<maximum[o]<<'\n';
}
void production_inputs_cpu(const std::vector<Sample>&samples){
    unsigned controls=0;
    for(const auto&s:samples){const auto source=source_state(s.fixture);auto wrong=s.fixture;wrong.attributes[1]=wrong.decoded_uv;
        check(!exact(source.outputs[3],source_state(wrong).outputs[3]),"wrong Apply declaration negative control has no D0 response");++controls;}
    std::cout<<controls<<" captured Apply wrong-UV CPU declaration controls respond\n";
}
void production_inputs_gpu(const std::vector<Sample>&samples,const std::vector<Fixture>&generations){
    std::vector<Fixture> fixtures;for(const auto&s:samples)fixtures.push_back(s.fixture);
    const auto wrong_begin=fixtures.size();for(const auto&s:samples){auto f=s.fixture;
        // Keep the correct phase marker: a legacy fallback must not conceal
        // the deliberately wrong authored declaration in the preferred path.
        f.declaration=pack_source_vertex_declaration(SourceVertexDeclaration{0,{0,3}});fixtures.push_back(f);}
    const auto generation_begin=fixtures.size();fixtures.insert(fixtures.end(),generations.begin(),generations.end());
    halo2_test::Compute<Fixture,208> compute(HALO2_VERTEX_BANK_PRODUCTION_SHADER);const auto gpu=compute.run(fixtures);
    std::vector<ShaderVector> actual;
    for(unsigned n=0;n<fixtures.size();++n){for(unsigned i=0;i<192;++i)
            check(exact(gpu[n*208+16+i],bound_constant(fixtures[n],i)),"production-input GLSL constant bank changed");
        if(n<samples.size())actual.insert(actual.end(),gpu.begin()+n*208,gpu.begin()+n*208+16);}
    std::cout<<"Production Apply declaration: ";compare(actual,samples,true);
    for(unsigned n=0;n<samples.size();++n){
        check(!exact(gpu[n*208+3],gpu[(wrong_begin+n)*208+3]),"Apply wrong-declaration GPU control has no D0 response");
        if((n&1u)==0)for(unsigned i:outputs)check(exact(gpu[n*208+i],gpu[(n+1)*208+i]),"Apply consumed poison UV");
    }
    compare_controlled_generation(std::span<const ShaderVector>(gpu).subspan(generation_begin*208),generations);
    std::cout<<samples.size()<<" actual Apply production inputs + "<<samples.size()<<" wrong-declaration controls + "<<generations.size()<<" controlled generation inputs; "<<fixtures.size()*192*4<<" production-input constant words exact\n";
}
}
int main(int argc,char**argv){try{
    check(argc==1||(argc==2&&std::string_view(argv[1])=="--gpu"),"expected optional --gpu");packer_controls();
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);auto captured=receiver_samples(root);std::vector<Sample> samples;
    for(const auto size:host_extents)for(auto s:captured){
        bind_camera(s);s.fixture.camera=host_camera(size[0],size[1]);samples.push_back(s);
        for(unsigned i=0;i<49;++i)check(exact(bound_constant(s.fixture,i),s.reference.constants[i]),"actual bank/fog/material CPU contract mismatch");
    }
    std::vector<ShaderVector>cpu;for(const auto&s:samples){VertexProgram p;p.instructions=s.fixture.instructions;p.count=s.fixture.flags[3];
        VertexProgramState state;state.attributes=s.fixture.attributes;for(unsigned i=0;i<192;++i)state.constants[i]=bound_constant(s.fixture,i);
        execute_vertex_program(p,state);cpu.insert(cpu.end(),state.outputs.begin(),state.outputs.end());}
    compare(cpu,samples,false);
    const auto production=production_receivers(captured);const auto generation=controlled_generations(root);
    production_inputs_cpu(production);controlled_generation_cpu(generation);
    std::vector<Fixture> fixtures;for(const auto&s:samples)fixtures.push_back(s.fixture);
    const auto prt=original_prt_constants(root);
    for(const auto size:host_extents){
        auto ordinary=samples.front().fixture;ordinary.camera=host_camera(size[0],size[1]);ordinary.control={};ordinary.flags[3]=0;
        VertexProgramBank globals;globals.constants.assign(samples.front().reference.constants.begin()+34,samples.front().reference.constants.begin()+49);
        ordinary.bank=pack_vertex_program_bank(globals);
        check(ordinary.bank.range==std::array<std::uint32_t,4>{34,15,0,0},"ordinary global bank flags/range changed");
        check(!exact(bound_constant(ordinary,0),samples.front().reference.constants[0]),"ordinary camera control does not distinguish source override");
        fixtures.push_back(ordinary);
        globals.first_constant=68;globals.constants.assign(prt.begin(),prt.end());globals.prt_inputs=true;ordinary.bank=pack_vertex_program_bank(globals);
        check(ordinary.bank.range==std::array<std::uint32_t,4>{68,82,1,0},"PRT bank flags/range changed");fixtures.push_back(ordinary);
    }
    {
        auto extrema=samples.front().fixture;extrema.flags[3]=0;extrema.control={};
        VertexProgramBank bank;bank.constants.resize(158);
        for(unsigned i=0;i<158;++i)bank.constants[i]={float(i+1),-float(i+1),float(i+1)*.5f,1};
        extrema.bank=pack_vertex_program_bank(bank);
        check(exact(bound_constant(extrema,58),bank.constants[24]) &&
              exact(bound_constant(extrema,59),bank.constants[25]) &&
              exact(bound_constant(extrema,191),bank.constants[157]),
              "native application-owned or highest source constant is unavailable");
        fixtures.push_back(extrema);
    }
    if(argc==2){
        require_validation_environment();
        halo2_test::Compute<Fixture,208> compute(HALO2_VERTEX_BANK_TEST_SHADER);const auto gpu=compute.run(fixtures);
        VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(compute.physical,&properties);std::cout<<"Source vertex-bank GPU: "<<properties.deviceName<<'\n';
        std::vector<ShaderVector>outputs_gpu;
        for(unsigned n=0;n<fixtures.size();++n){
            for(unsigned i=0;i<192;++i){
                const auto&actual=gpu[n*208+16+i];const auto expected=bound_constant(fixtures[n],i);
                if(!exact(actual,expected)){
                    std::ostringstream mismatch;mismatch<<"production GLSL constant mismatch fixture"<<n<<" c"<<i<<" actual";
                    for(auto v:actual)mismatch<<' '<<std::hex<<std::bit_cast<std::uint32_t>(v);mismatch<<" expected";
                    for(auto v:expected)mismatch<<' '<<std::hex<<std::bit_cast<std::uint32_t>(v);
                    throw std::runtime_error(mismatch.str());
                }
            }
            if(n<samples.size())outputs_gpu.insert(outputs_gpu.end(),gpu.begin()+n*208,gpu.begin()+n*208+16);
        }
        compare(outputs_gpu,samples,true);
        std::cout<<fixtures.size()*192*4<<" production constant words exact, including "<<samples.size()*12*4<<" actual camera words; Vulkan/synchronization validation requested\n";
        production_inputs_gpu(production,generation);
    }
    std::cout<<samples.size()<<" actual receiver vertices,4 target sizes;8 ordinary/PRT bank controls; malformed camera banks rejected\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
