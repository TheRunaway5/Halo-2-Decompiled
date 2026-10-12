#include "halo2/source_vertex_inputs.hpp"
#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
struct Record {
    std::string map,leaf;std::uint32_t datum{};bool latent{};
    SourceVertexDeclaration declaration;std::vector<std::byte> code;
    std::array<std::uint32_t,64> original{};
};
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};
    std::array<ShaderVector,10> streams{};
    PackedSourceVertexDeclaration declaration=legacy_source_vertex_declaration;
};
static_assert(sizeof(Fixture)==5696);
void check(bool b,const std::string&m){if(!b)throw std::runtime_error(m);}
template<class F>void rejects(F&&f){bool caught=false;try{f();}catch(const FormatError&){caught=true;}check(caught,"invalid vertex declaration admitted");}
bool exact(const ShaderVector&a,const ShaderVector&b){return std::bit_cast<std::array<std::uint32_t,4>>(a)==std::bit_cast<std::array<std::uint32_t,4>>(b);}
std::uint32_t hex(std::string_view s){check(s.size()==8,"invalid original declaration word");std::size_t used=0;
    const auto n=std::stoul(std::string(s),&used,16);check(used==s.size()&&n<=0xfffffffful,"bad original declaration word");return n;}
std::vector<Record> records(const std::filesystem::path&root){
    std::ifstream file(root/"tests/fixtures/retail_world_vertex_declarations.tsv");check(bool(file),"original declarations fixture missing");
    std::vector<Record> result;std::string line;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;std::istringstream input(line);
        Record r;char kind;std::string datum,code;unsigned latent,count;
        check(bool(input>>kind>>r.map>>datum>>r.leaf>>latent>>count)&&kind=='D'&&latent<=1&&count<=16,"bad original declaration row");
        r.datum=hex(datum);r.latent=latent;
        for(unsigned i=0;i<count;++i){unsigned value;check(bool(input>>value)&&value<=65535,"invalid original semantic");r.declaration.semantics.push_back(std::uint16_t(value));}
        check(bool(input>>code)&&code.size()%2==0,"invalid original vertex payload");
        for(unsigned i=0;i<code.size();i+=2){std::size_t used;const auto n=std::stoul(code.substr(i,2),&used,16);check(used==2,"invalid original payload byte");r.code.push_back(std::byte(n));}
        for(auto&w:r.original){std::string value;check(bool(input>>value),"truncated original1C330 output");w=hex(value);}
        std::string extra;check(!(input>>extra),"trailing declaration fixture data");result.push_back(std::move(r));
    }
    check(result.size()==46,"original world-class corpus changed");return result;
}
Vertex streams(unsigned seed){
    const float d=float(seed)/32;
    Vertex v;v.position={.25f+d,-.5f,1.5f};v.uv={.125f,.625f-d};
    v.normal={-.75f,.25f,.5f};v.tangent={.5f,.75f,-.25f};v.binormal={.25f,-.5f,.75f};
    v.incident={.125f,.25f,.5f};v.lightmap_uv={.375f,.875f};v.color={.25f,.5f,.75f};
    v.prt_cluster=float(seed%3+1);v.prt_weights={.125f,-.375f,.625f,-.875f};return v;
}
std::array<ShaderVector,10> stream_vectors(const Vertex&v){
    const auto xyz=[](Vec3 p){return ShaderVector{p.x,p.y,p.z,1};};
    return {xyz(v.position),ShaderVector{v.uv.x,v.uv.y,0,1},xyz(v.normal),xyz(v.tangent),xyz(v.binormal),xyz(v.incident),
        ShaderVector{v.lightmap_uv.x,v.lightmap_uv.y,0,1},xyz(v.color),ShaderVector{v.prt_cluster,0,0,1},v.prt_weights};
}
// The independent unchanged1C330 output resolves the literal XBE stream table
// (types9,27,24,48,31,47,57). Derive register values from those descriptors,
// never from the native semantic-switch helper being tested.
std::array<ShaderVector,16> original_inputs(const Record&r,const Fixture&f){
    auto result=f.attributes;
    for(unsigned reg=0;reg<r.declaration.semantics.size();++reg) {
        const auto at=reg*4,stream=r.original[at],offset=r.original[at+1],format=r.original[at+2];
        check(r.original[at+3]==0,"source declaration method changed");unsigned index=10;
        if(stream==0&&offset==0&&format==0x32)index=0;
        if(stream==1&&offset<=8&&offset%4==0&&format==0x16)index=2+offset/4;
        if(stream==2&&offset==0&&format==0x22)index=1;
        if(stream==3&&offset==0&&format==0x16)index=5;
        if(stream==4&&offset==0&&format==0x21)index=6;
        if(stream==5&&offset==0&&format==0x34)index=7;
        if(stream==6&&offset==0&&format==0x15)index=8;
        if(stream==6&&offset==2&&format==0x41)index=9;
        check(index<10,"unproved original stream descriptor");result[reg]=f.streams[index];
    }
    return result;
}
Fixture fixture(const Record&r,unsigned seed){
    Fixture f;const auto p=decode_vertex_program(r.code);f.instructions=p.instructions;f.flags[3]=p.count;
    f.flags[2]=r.leaf.starts_with("prt")?9:r.leaf.starts_with("lightmap_pervertex")?4:0;
    for(unsigned i=0;i<16;++i)f.attributes[i]={.125f*float(i+1),-.25f,.5f,.75f};
    for(unsigned i=0;i<192;++i)f.constants[i]={.25f+float(i%7)/16,.125f,.375f,.5f};
    f.constants[0]={1,0,0,0};f.constants[1]={0,1,0,0};f.constants[2]={0,0,1,0};f.constants[3]={0,0,0,1};
    f.constants[7]={3,5,7,767.8125f};f.constants[8]={320,0,0,320.53125f};f.constants[9]={0,-240,0,240.53125f};
    f.constants[10]={320,-240,16777215,0};f.constants[11]={320.5f,240.5f,0,0};
    f.constants[69]={1,4,1,1};f.constants[170]={1,1,1,0};f.constants[171]={0,0,0,0};f.constants[172]={1,1,0,0};
    f.streams=stream_vectors(streams(seed));f.declaration=pack_source_vertex_declaration(r.declaration);return f;
}
void controls(){
    check(pack_source_vertex_declaration(std::nullopt)==legacy_source_vertex_declaration,"procedural adapter changed");
    for(auto d:std::vector<SourceVertexDeclaration>{{0,{}},{1,{0,3}},{0,{0,1}},{0,{0,65535}},{0,std::vector<std::uint16_t>(17,0)}})
        rejects([&]{(void)pack_source_vertex_declaration(d);});
    std::vector<std::byte> metadata(96);
    const auto write=[&](unsigned offset,std::uint32_t value,unsigned size=4){for(unsigned i=0;i<size;++i)metadata[offset+i]=std::byte(value>>(i*8));};
    write(0,1,2);write(4,1);write(8,0x1020);write(0x24,2);write(0x28,0x1060); // Semantic pointer exactly outside allocation.
    rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0);});
    write(0x28,0x1050);write(0x50,0,2);write(0x52,3,2);
    check(decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0).semantics==std::vector<std::uint16_t>{0,3},"literal declaration decoder changed");
    rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,1);});
    write(0x24,17);rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0);});write(0x24,2);
    write(8,0xfff);rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0);});write(8,0x1020);
    write(4,65537);rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0);});write(4,1);
    write(0,2,2);rejects([&]{decode_source_vertex_declaration(Bytes(metadata),0,0x1000,0);});
}
void validation(){
    const auto*l=std::getenv("VK_INSTANCE_LAYERS");const auto*s=std::getenv("VK_LAYER_VALIDATE_SYNC");
    check(l&&std::string_view(l).find("VK_LAYER_KHRONOS_validation")!=std::string_view::npos,"declaration GPU check requires Vulkan validation");
    check(s&&std::string_view(s)=="1","declaration GPU check requires synchronization validation");
}
}
int main(int argc,char**argv){try {
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);const auto rows=records(root);controls();
    std::map<std::string,std::unique_ptr<Cache>> caches;std::set<std::vector<std::uint16_t>> layouts;
    std::vector<Fixture> fixtures;std::vector<std::array<ShaderVector,16>> inputs,expected;
    unsigned cpu_words=0,raw=0,changed=0;
    for(const auto&r:rows) {
        if(!caches.contains(r.map)){const auto a=root/"assets/disc/maps"/(r.map+".map"),b=root/"assets/map-pack/maps"/(r.map+".map");
            if(!std::filesystem::exists(a)&&!std::filesystem::exists(b))return 77;caches[r.map]=std::make_unique<Cache>(std::filesystem::exists(a)?a:b);}
        const auto&cache=*caches.at(r.map);check(cache.load_source_vertex_declaration(r.datum,0)==r.declaration,"raw class0 semantic loader differs");
        check(cache.load_vertex_shader_code(r.datum,0)==r.code,"authored compiled payload differs");++raw;layouts.insert(r.declaration.semantics);
        for(unsigned seed=0;seed<4;++seed) {
            auto f=fixture(r,seed);const auto original=original_inputs(r,f);
            const auto native=source_world_vertex_inputs(r.declaration,streams(seed),f.attributes);
            for(unsigned i=0;i<16;++i){check(exact(native[i],original[i]),"native stream binding differs from original1C330 descriptor");cpu_words+=4;}
            VertexProgramState state;state.attributes=original;state.constants=f.constants;
            execute_vertex_program(decode_vertex_program(r.code),state);
            inputs.push_back(original);expected.push_back(state.outputs);fixtures.push_back(f);
            if(r.leaf=="prt_lightmap"||r.leaf=="transparent_generic_offset") {
                auto old=f;old.declaration=legacy_source_vertex_declaration;
                auto attributes=f.attributes;attributes[0]=f.streams[0];attributes[1]=f.streams[1];attributes[2]=f.streams[2];
                attributes[3]=f.streams[3];attributes[4]=f.streams[4];attributes[5]=r.leaf=="prt_lightmap"?f.streams[8]:f.streams[5];
                attributes[6]=r.leaf=="prt_lightmap"?f.streams[9]:f.streams[6];
                VertexProgramState legacy;legacy.attributes=attributes;legacy.constants=f.constants;execute_vertex_program(decode_vertex_program(r.code),legacy);
                check(legacy.outputs!=state.outputs,"wrong-register source program control did not respond");
                inputs.push_back(attributes);expected.push_back(legacy.outputs);fixtures.push_back(old);++changed;
            }
        }
    }
    check(raw==46&&layouts.size()==10&&changed==8,"declaration corpus/control coverage changed");
    if(argc>1&&std::string_view(argv[1])=="--gpu") {
        validation();halo2_test::Compute<Fixture,32> gpu(HALO2_VERTEX_DECLARATION_TEST_SHADER);const auto result=gpu.run(fixtures);
        check(result.size()==fixtures.size()*32,"GPU declaration results truncated");unsigned exact_input_words=0,output_words=0,max_ulp=0;
        for(unsigned n=0;n<fixtures.size();++n)for(unsigned i=0;i<16;++i) {
            check(exact(result[n*32+i],inputs[n][i]),"production GLSL register declaration differs from unchanged1C330");exact_input_words+=4;
            for(unsigned axis=0;axis<4;++axis) {
                const auto a=result[n*32+16+i][axis],b=expected[n][i][axis];
                const auto aw=std::bit_cast<std::uint32_t>(a),bw=std::bit_cast<std::uint32_t>(b);
                if(aw==bw){++output_words;continue;}
                check(std::isfinite(a)&&std::isfinite(b),"nonfinite authored output differs");
                const auto ulp=(aw>>31)==(bw>>31)?(aw>bw?aw-bw:bw-aw):0xffffffffu;
                if(ulp!=0xffffffffu)max_ulp=std::max(max_ulp,ulp);
                check(ulp<=8||std::abs(a-b)<=2e-5f,"production authored vertex output differs, case"+std::to_string(n)+" register"+std::to_string(i));
            }
        }
        std::cout<<exact_input_words<<" production register words exact; "<<output_words<<" authored output words exact, maxULP "<<max_ulp<<"\n";
    }
    std::cout<<raw<<" raw authored class0 records, "<<layouts.size()<<" declaration layouts, "<<cpu_words<<" original descriptor/input words exact, "<<changed<<" wrong-register controls; explicit decoded world inputs, no live appearance claim\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
