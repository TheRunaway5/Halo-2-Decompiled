#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
void check(bool v,const std::string&m){if(!v)throw FormatError(m);}
template<class F>void rejects(F&&f){bool caught=false;try{f();}catch(const FormatError&){caught=true;}check(caught,"unsupported CPU skin input was admitted");}
struct Record {std::string family;unsigned classification{};SourceVertexDeclaration declaration;std::vector<std::byte> code;};
using Key=std::tuple<std::string,std::uint32_t,unsigned,unsigned>;
struct Corpus {std::vector<Record> records;std::map<Key,unsigned> classes;};
Corpus load(){std::ifstream in(std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_cpu_skin_inputs.tsv");check(bool(in),"original CPU skin fixture missing");
    Corpus c;std::string line;unsigned rows=0;
    while(std::getline(in,line)){if(line.empty()||line[0]=='#')continue;std::istringstream row(line);char kind;row>>kind;
        if(kind=='C'){std::string map,model;unsigned section,part,mode,opaque,transparent,type,original;row>>map>>model>>section>>part>>mode>>opaque>>transparent>>type>>original;
            check(source_model_vertex_class(mode,opaque,transparent,type)==original,"class differs from untouched1CAA0/1CB20");
            const auto datum=std::uint32_t(std::stoul(model,nullptr,16));check(c.classes.emplace(Key{map,datum,section,part},original).second,"duplicate original part class");++rows;
        }else if(kind=='P'){Record r;unsigned count;std::string code;row>>r.family>>r.classification>>count;r.declaration.class_index=r.classification;
            for(unsigned i=0;i<count;++i){unsigned semantic;row>>semantic;r.declaration.semantics.push_back(std::uint16_t(semantic));}row>>code;
            check(code.size()%2==0,"original CPU skin payload malformed");for(unsigned i=0;i<code.size();i+=2)r.code.push_back(std::byte(std::stoul(code.substr(i,2),nullptr,16)));
            c.records.push_back(std::move(r));
        }else throw FormatError("invalid CPU skin fixture record");check(bool(row),"truncated CPU skin fixture");std::string extra;check(!(row>>extra),"trailing CPU skin fixture fields");}
    check(rows==378&&c.records.size()==49,"original FP class/payload corpus changed");return c;
}
std::array<ShaderVector,192> constants(){std::array<ShaderVector,192> c{};
    c[0]={1,0,0,0};c[1]={0,1,0,0};c[2]={0,0,1,0};c[3]={1,0,0,4};c[4]={1,0,0,1};c[5]={0,1,0,.5f};c[6]={-1,0,0,2};c[7]={3,1,2,767.8125f};
    c[8]={.5f,0,0,.5f};c[9]={0,-.5f,0,.5f};c[10]={320,-240,1,1};c[11]={.5f,.5f,0,1};
    for(unsigned n=18;n<34;++n)c[n]={.125f,.25f,.5f,.75f};c[26]={.25f,.5f,-.75f,0};c[27]={-.5f,.75f,.25f,0};
    c[170]={1,1,1,0};c[172]={1,1,0,0};return c;
}
std::array<Transform,4> nodes(){std::array<Transform,4> n{};
    n[0].basis={Vec3{0,1,0},Vec3{-1,0,0},Vec3{0,0,1}};n[0].position={.25f,-.5f,.125f};
    n[1].basis={Vec3{1,0,0},Vec3{0,0,1},Vec3{0,-1,0}};n[1].position={-.25f,.5f,.75f};
    n[2].basis={Vec3{0,0,1},Vec3{0,1,0},Vec3{-1,0,0}};n[2].position={.5f,.25f,-.5f};
    n[3].basis={Vec3{1,0,0},Vec3{0,0,1},Vec3{0,-1,0}};n[3].position={-.5f,-.25f,.5f};return n;
}
constexpr std::array<std::array<std::uint8_t,4>,3> weights{{{128,128,0,0},{0,128,128,0},{1,250,200,100}}};
RenderModel controlled(unsigned count,unsigned variant){RenderModel m;m.bones.resize(4);m.mesh.vertices.resize(1);m.skin.resize(1);
    for(auto&b:m.bones)b.inverse={1,0,0,0,1,0,0,0,1,0,0,0};auto&v=m.mesh.vertices[0];v.position={.375f+float(variant)/16,-.625f,1.25f};v.uv={.125f,.75f-float(variant)/16};
    v.normal={-.75f,.25f,.5f};v.tangent={.5f,.75f,-.25f};v.binormal={.25f,-.5f,.75f};v.color={.25f,.5f,.75f};
    m.skin[0].weights=source_model_skin_weights(weights[variant],count);m.skin[0].bones={0,1,2,3};return m;
}
ShaderVector xyz(Vec3 v){return {v.x,v.y,v.z,1};}
std::array<ShaderVector,16> attributes(){std::array<ShaderVector,16> a{};for(auto&v:a)v={.125f,.25f,.5f,.75f};return a;}
std::array<ShaderVector,16> source_inputs(const Record&r,const Vertex&v,const SkinBinding&skin,
    const std::array<float,4>&raw_weights){auto a=attributes();
    for(unsigned i=0;i<r.declaration.semantics.size();++i)switch(r.declaration.semantics[i]){
    case 0:a[i]=xyz(v.position);break;case 1:a[i]={float(skin.bones[0])/255.f,float(skin.bones[1])/255.f,float(skin.bones[2])/255.f,float(skin.bones[3])/255.f};break;
    case 2:a[i]=raw_weights;break;case 3:a[i]={v.uv.x,v.uv.y,0,1};break;
    case 4:a[i]=xyz(v.normal);break;case 5:a[i]=xyz(v.tangent);break;case 6:a[i]=xyz(v.binormal);break;
    default:throw FormatError("unproved local/skinned source semantic");}return a;
}
struct Fixture {std::array<VertexInstruction,136> instructions{};std::array<ShaderVector,16> attributes{};std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};std::array<ShaderVector,10> streams{};PackedSourceVertexDeclaration declaration=legacy_source_vertex_declaration;};
static_assert(sizeof(Fixture)==5696);
std::array<ShaderVector,10> streams(const Vertex&v){return {xyz(v.position),ShaderVector{v.uv.x,v.uv.y,0,1},xyz(v.normal),xyz(v.tangent),xyz(v.binormal),xyz(v.incident),
    ShaderVector{v.lightmap_uv.x,v.lightmap_uv.y,0,1},xyz(v.color),ShaderVector{v.prt_cluster,0,0,1},v.prt_weights};}
void close(const std::array<ShaderVector,16>&a,const std::array<ShaderVector,16>&b,const std::string&what){
    for(unsigned i=0;i<16;++i)for(unsigned axis=0;axis<4;++axis){check(std::isfinite(a[i][axis])&&std::isfinite(b[i][axis]),"nonfinite "+what);
        check(std::abs(a[i][axis]-b[i][axis])<=2e-5f*std::max({1.f,std::abs(a[i][axis]),std::abs(b[i][axis])}),what+" output"+std::to_string(i)+"."+std::to_string(axis));}}
struct Checks {std::vector<Fixture> fixtures;std::vector<std::array<ShaderVector,16>> inputs,outputs;unsigned normalized{},wrong_normal{},asset{},source_components{};
    void add(const Record&world,const Vertex&posed,const std::array<ShaderVector,192>&c,SourceWorldVertexInputTransform transform,
             const std::array<ShaderVector,16>&expected){Fixture f;const auto p=decode_vertex_program(world.code);f.instructions=p.instructions;f.flags[3]=p.count;f.attributes=attributes();f.constants=c;f.streams=streams(posed);f.declaration=pack_source_vertex_declaration(world.declaration,transform);
        const auto input=source_world_vertex_inputs(world.declaration,posed,f.attributes,transform);VertexProgramState native;native.constants=c;native.attributes=input;execute_vertex_program(p,native);close(native.outputs,expected,"CPU skin world adapter");
        source_components+=64;fixtures.push_back(f);inputs.push_back(input);outputs.push_back(expected);normalized+=transform==SourceWorldVertexInputTransform::NormalizeSkinNormal;}
};
void variant_controls(const Corpus&corpus,Checks&checks){const auto world_nodes=nodes();std::array<Transform,4> inverse{};const auto palette=source_model_skin_palette(world_nodes,inverse);
    for(const auto&source:corpus.records){if(!source.classification)continue;const auto&world=*std::find_if(corpus.records.begin(),corpus.records.end(),[&](const auto&r){return r.family==source.family&&!r.classification;});
        if(source.family=="transparent_generic_reflection"&&source.classification==6){rejects([&]{source_cpu_skin_input_transform(world.code,source.code,6);});continue;}
        const auto transform=source_cpu_skin_input_transform(world.code,source.code,source.classification);
        check((transform==SourceWorldVertexInputTransform::NormalizeSkinNormal)==(source.family=="transparent_generic_reflection"&&source.classification>=3),"unrelated class/program was normalized");
        const unsigned count=source.classification<4?1:source.classification-2;
        for(unsigned variant=0;variant<3;++variant){const auto m=controlled(count,variant);const auto posed=skin_model_world_nodes(m,world_nodes).front();auto c=constants();
            for(unsigned n=0;n<palette.size();++n)for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<4;++axis)c[50+n*3+row][axis]=palette[n][row*4+axis];
            std::array<float,4> raw{};for(unsigned j=0;j<count;++j)raw[j]=float(weights[variant][j])/255.f;
            VertexProgramState original;original.constants=c;original.attributes=source_inputs(source,m.mesh.vertices.front(),m.skin.front(),raw);execute_vertex_program(decode_vertex_program(source.code),original);
            checks.add(world,posed,c,transform,original.outputs);
            if(transform!=SourceWorldVertexInputTransform::Preserve){VertexProgramState old;old.constants=c;old.attributes=source_world_vertex_inputs(world.declaration,posed,attributes());execute_vertex_program(decode_vertex_program(world.code),old);
                check(old.outputs!=original.outputs,"missing normal operation control did not respond");++checks.wrong_normal;checks.add(world,posed,c,SourceWorldVertexInputTransform::Preserve,old.outputs);}
        }
    }
    auto world=std::find_if(corpus.records.begin(),corpus.records.end(),[](const auto&r){return r.family=="transparent_generic_reflection"&&!r.classification;});auto source=std::find_if(corpus.records.begin(),corpus.records.end(),[](const auto&r){return r.family=="transparent_generic_reflection"&&r.classification==4;});
    auto bad=source->code;bad.back()^=std::byte{2};rejects([&]{source_cpu_skin_input_transform(world->code,bad,4);});rejects([&]{source_cpu_skin_input_transform(world->code,source->code,7);});
    rejects([&]{pack_source_vertex_declaration(std::nullopt,SourceWorldVertexInputTransform::NormalizeSkinNormal);});rejects([&]{pack_source_vertex_declaration(SourceVertexDeclaration{0,{0,3}},SourceWorldVertexInputTransform::NormalizeSkinNormal);});
}
void assets(const Corpus&corpus,Checks&checks){std::map<std::pair<std::string,std::uint32_t>,std::map<std::pair<unsigned,unsigned>,unsigned>> models;
    for(const auto&[key,value]:corpus.classes)models[{std::get<0>(key),std::get<1>(key)}][{std::get<2>(key),std::get<3>(key)}]=value;
    unsigned origins=0,loaded=0;std::string current;std::unique_ptr<Cache> cache;
    for(const auto&[key,classes]:models){if(current!=key.first){current=key.first;auto path=std::filesystem::path(HALO2_SOURCE_DIR)/"assets/disc/maps"/(current+".map");if(!std::filesystem::exists(path))path=std::filesystem::path(HALO2_SOURCE_DIR)/"assets/map-pack/maps"/(current+".map");cache=std::make_unique<Cache>(path);}
        auto m=cache->load_model(key.second,false);for(const auto&origin:m.vertex_origins){check(origin.vertex_class==classes.at({origin.section,origin.part}),"model expansion lost original compiled class");++origins;}++loaded;}
    check(loaded==84&&origins>100000,"all21 actual model-class metadata coverage changed");
    Cache c(std::filesystem::path(HALO2_SOURCE_DIR)/"assets/disc/maps/cyclotron.map");const auto tag=std::find_if(c.tags().begin(),c.tags().end(),[](const auto&t){return t.group=="mode"&&t.name=="objects\\characters\\elite\\fp_arms\\fp_arms";});check(tag!=c.tags().end(),"actual Elite arms missing");
    auto m=c.load_model(tag->datum,true);for(auto&mat:m.mesh.materials)if(!mat.lods.empty())check(prepare_dynamic_material(c,mat,{},true),"Elite authored pass was not prepared");
    const auto roots=m.mesh.materials;SourceDynamicModelShaders selection(c,m,tag->datum);
    const auto world_nodes=pose_model_nodes(m);auto posed=skin_model_world_nodes(m,world_nodes);
    SourceModelShaderView view;view.shader=source_shader_lod_view(0,480,false,false);view.part_shader_multiplier=1;selection.select(posed,{10000,255},view);
    unsigned selected=0;std::set<std::pair<unsigned,unsigned>> seen;
    for(unsigned i=0;i<posed.size();++i){const auto&root=m.mesh.materials.at(posed[i].material);bool any=false;auto inspect=[&](auto&&self,const Material&x)->void{any|=x.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve;if(x.environment_pass)self(self,*x.environment_pass);for(auto&p:x.world_overlays_before)self(self,*p);for(auto&p:x.world_overlays_after)self(self,*p);};inspect(inspect,root);selected+=any;}
    check(selected>0&&m.mesh.materials.size()>roots.size(),"live material selection did not route Elite normal binding");
    std::vector<Transform> inverse;for(const auto&b:m.bones){Transform v;v.scale=b.inverse_scale;v.basis={Vec3{b.inverse[0],b.inverse[1],b.inverse[2]},Vec3{b.inverse[3],b.inverse[4],b.inverse[5]},Vec3{b.inverse[6],b.inverse[7],b.inverse[8]}};v.position={b.inverse[9],b.inverse[10],b.inverse[11]};inverse.push_back(v);}const auto palette=source_model_skin_palette(world_nodes,inverse);
    const auto&source=*std::find_if(corpus.records.begin(),corpus.records.end(),[](const auto&r){return r.family=="transparent_generic_reflection"&&r.classification==4;});
    const auto&world=*std::find_if(corpus.records.begin(),corpus.records.end(),[](const auto&r){return r.family=="transparent_generic_reflection"&&!r.classification;});
    for(unsigned i=0;i<m.mesh.vertices.size();++i){const Material*pass=nullptr;auto find=[&](auto&&self,const Material&x)->void{if(x.world_vertex_input_transform==SourceWorldVertexInputTransform::NormalizeSkinNormal)pass=&x;if(x.environment_pass)self(self,*x.environment_pass);for(auto&p:x.world_overlays_before)self(self,*p);for(auto&p:x.world_overlays_after)self(self,*p);};find(find,m.mesh.materials.at(posed[i].material));
        if(!pass||!seen.emplace(m.vertex_origins[i].section,m.vertex_origins[i].vertex).second)continue;check(m.vertex_origins[i].vertex_class==4,"Elite reflection actual class changed");
        auto cb=constants();for(unsigned j=0;j<16;++j)cb[18+j]=pass->program_constants[1][j];for(unsigned j=0;j<palette.size();++j)for(unsigned r=0;r<3;++r)for(unsigned a=0;a<4;++a)cb[50+j*3+r][a]=palette[j][r*4+a];
        VertexProgramState original;original.constants=cb;original.attributes=source_inputs(source,m.mesh.vertices[i],m.skin[i],m.skin[i].weights);execute_vertex_program(decode_vertex_program(source.code),original);
        checks.add(world,posed[i],cb,pass->world_vertex_input_transform,original.outputs);++checks.asset;
        VertexProgramState old;old.constants=cb;old.attributes=source_world_vertex_inputs(world.declaration,posed[i],attributes());execute_vertex_program(decode_vertex_program(world.code),old);
        if(old.outputs!=original.outputs)++checks.wrong_normal;checks.add(world,posed[i],cb,SourceWorldVertexInputTransform::Preserve,old.outputs);
        const auto&base=m.mesh.materials.at(m.mesh.vertices[i].material);check(base.world_vertex_input_transform==SourceWorldVertexInputTransform::Preserve,"root base pass normal binding changed");
    }
    check(checks.asset==80,"actual Elite glow vertex coverage changed");std::cout<<loaded<<" authored FP model metadata banks, "<<origins<<" expanded class identities exact; "<<selected<<" selected Elite reflection face vertices\n";
}
void validation(){const char*l=std::getenv("VK_INSTANCE_LAYERS"),*s=std::getenv("VK_LAYER_VALIDATE_SYNC");check(l&&std::string_view(l).find("VK_LAYER_KHRONOS_validation")!=std::string_view::npos&&s&&std::string_view(s)=="1","CPU skin GPU test requires synchronization validation");}
}
int main(int argc,char**argv){try{const auto corpus=load();Checks checks;variant_controls(corpus,checks);assets(corpus,checks);
    if(argc>1&&std::string_view(argv[1])=="--gpu"){validation();halo2_test::Compute<Fixture,32> gpu(HALO2_VERTEX_DECLARATION_TEST_SHADER);const auto result=gpu.run(checks.fixtures);check(result.size()==checks.fixtures.size()*32,"CPU skin GPU result truncated");unsigned exact=0;
        for(unsigned n=0;n<checks.fixtures.size();++n){std::array<ShaderVector,16> input{},output{};for(unsigned i=0;i<16;++i){input[i]=result[n*32+i];output[i]=result[n*32+16+i];for(unsigned a=0;a<4;++a)exact+=std::bit_cast<std::uint32_t>(input[i][a])==std::bit_cast<std::uint32_t>(checks.inputs[n][i][a]);}close(input,checks.inputs[n],"production input loader");close(output,checks.outputs[n],"production source variant adapter");}
        std::cout<<exact<<" production input words exact; "<<result.size()*4<<" GPU input/output words checked with authored programs\n";}
    std::cout<<checks.fixtures.size()<<" variants/assets/negative-control cases, "<<checks.normalized<<" scoped normal bindings, "<<checks.asset<<" real Elite vertices, "<<checks.wrong_normal<<" missing-normal controls; "<<checks.source_components<<" source output components checked\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
