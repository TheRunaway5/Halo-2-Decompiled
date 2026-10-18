#include "halo2/mcc_model_vertex_layout.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{},negatives{};
void check(bool value,const char* message){++checks;if(!value)throw FormatError(message);}
struct Input {
    unsigned cls=2,nodes_per_vertex=1,nodes=33,count=1;
    std::vector<std::byte> header=std::vector<std::byte>(120),declarations=std::vector<std::byte>(96),positions=std::vector<std::byte>(16),uv=std::vector<std::byte>(8),basis=std::vector<std::byte>(36),map{std::byte{14},std::byte{16},std::byte{17},std::byte{18}};
    Input(){declarations[0]=std::byte{2};declarations[1]=std::byte{16};declarations[32]=std::byte{23};declarations[33]=std::byte{8};declarations[64]=std::byte{25};declarations[65]=std::byte{36};positions[12]=std::byte{3};positions[13]=std::byte{171};positions[14]=std::byte{172};positions[15]=std::byte{173};}
    MccModelVertexLayout run()const{return project_mcc_rigid_model_vertex_layout(cls,nodes_per_vertex,nodes,count,header,declarations,positions,uv,basis,map);}
};
void portable(){
    Input f;const auto saved=f;const auto out=f.run();check(out.descriptors[0]==std::byte{3} && out.descriptors[1]==std::byte{16} && out.descriptors[32]==std::byte{24} && out.descriptors[64]==std::byte{27},"Genuine rigid-boned FLOAT/UBYTE declaration differs");check(out.node_map==std::vector<std::uint8_t>({14,16,17,18}),"Actual node map changed");check(f.positions==saved.positions && f.declarations==saved.declarations && f.basis==saved.basis && f.map==saved.map,"Pure model projection mutated source/index padding");
    const auto again=f.run();check(again.descriptors==out.descriptors && again.basis==out.basis && again.node_map==out.node_map,"Repeated source projection changed");
    Input rigid;rigid.cls=1;rigid.nodes_per_vertex=0;rigid.positions.resize(12);rigid.declarations[0]=std::byte{1};rigid.declarations[1]=std::byte{12};check(rigid.run().descriptors[0]==std::byte{1},"Rigid FLOAT3 declaration changed");
    Input alternate;alternate.declarations[0]=std::byte{3};check(alternate.run().descriptors==out.descriptors,"Verified alternate MCC rigid-boned declaration changed");
    const auto reject=[&](auto mutate){Input n;mutate(n);bool threw=false;try{(void)n.run();}catch(const FormatError&){threw=true;}check(threw,"Unsupported model source was admitted");++negatives;};
    for(unsigned cls:{0,3,4})reject([&](Input& n){n.cls=cls;});
    reject([](Input& n){n.nodes_per_vertex=0;});reject([](Input& n){n.nodes_per_vertex=2;});reject([](Input& n){n.nodes=0;});reject([](Input& n){n.nodes=257;});reject([](Input& n){n.map.resize(257);});reject([](Input& n){n.map[3]=std::byte{33};});reject([](Input& n){n.positions[12]=std::byte{4};});
    reject([](Input& n){n.declarations[0]=std::byte{4};n.declarations[1]=std::byte{8};n.positions.resize(8);});
    reject([](Input& n){n.declarations[2]=std::byte{1};});reject([](Input& n){n.positions.pop_back();});reject([](Input& n){n.uv.pop_back();});reject([](Input& n){n.basis.pop_back();});
    reject([](Input& n){n.declarations.resize(95);});reject([](Input& n){n.count=0;});
    reject([](Input& n){n.positions[0]=std::byte{0};n.positions[1]=std::byte{0};n.positions[2]=std::byte{128};n.positions[3]=std::byte{127};});
    for(unsigned bytes:{0,111,119,121,128})reject([&](Input& n){n.header.resize(bytes);});
    reject([](Input& n){n.map.clear();n.positions[12]=std::byte{33};});
    Input direct;direct.map.clear();direct.positions[12]=std::byte{32};check(direct.run().node_map.empty(),"Explicit source direct-index domain invented a node map");
}
struct Row {
    unsigned cls{},nodes_per_vertex{},nodes{},count{},flags{},section{};
    Tag owner;
    std::vector<std::byte> header;
    std::vector<MenuWorldSourceResource> resources;
};
std::vector<Row> rows(const Cache& cache,const Tag& tag){
    const auto meta=cache.load_source_tag_metadata(tag.datum);const Bytes root(meta.bytes_at(meta.source_address(),0x68));const auto raw=cache.load_model_resources(tag.datum);std::vector<Row> result;
    for(unsigned i=0;i<root.u32(0x24);++i){const Bytes s(meta.bytes_at(root.u32(0x28)+i*92,92));if(!s.u16(4))continue;const auto& allocation=raw.at(i);const Bytes data(allocation);const auto base=allocation.size()-s.u32(0x44)-4;Row row{s.u16(0),std::to_integer<unsigned>(s.slice(0x14,1)[0]),root.u32(0x48),s.u16(4),s.u16(0x1a),i,tag,{},{}};const auto h=data.slice(8,base-8);row.header.assign(h.begin(),h.end());for(unsigned r=0;r<s.u32(0x48);++r){const Bytes d(meta.bytes_at(s.u32(0x4c)+r*16,16));const auto bytes=data.slice(base+d.u32(12),d.u32(8));row.resources.push_back({d.u16(4),d.u16(6),unsigned(base+d.u32(12)),{bytes.begin(),bytes.end()}});}result.push_back(std::move(row));}return result;
}
const MenuWorldSourceResource& resource(const Row& row,unsigned type,unsigned channel){const MenuWorldSourceResource* out=nullptr;for(const auto& r:row.resources)if(r.type==type && r.channel==channel){check(!out,"Ambiguous source model resource identity");out=&r;}check(out,"Missing complete source model resource owner");return *out;}
MccModelVertexLayout project(const Row& row){return project_mcc_rigid_model_vertex_layout(row.cls,row.nodes_per_vertex,row.nodes,row.count,row.header,resource(row,56,32).bytes,resource(row,56,0).bytes,resource(row,56,1).bytes,resource(row,56,2).bytes,resource(row,104,1).bytes);}
void same_ranges(const SourceBspRangeGeometry& source,const SourceBspRangeGeometry& projected){
    check(source.part_count==projected.part_count && source.subpart_count==projected.subpart_count && source.accelerator_count==projected.accelerator_count && source.index_count==projected.index_count,"Projected model range header differs from direct source consumer");
    check(source.parts==projected.parts && source.subparts==projected.subparts && source.accelerator==projected.accelerator && source.mapping==projected.mapping && source.bounds.size()==projected.bounds.size(),"Projected model ranges changed literal resource ownership");
    for(unsigned i=0;i<source.bounds.size();++i){const auto& a=source.bounds[i];const auto& b=projected.bounds[i];for(const auto [x,y]:{std::pair{a.center.x,b.center.x},std::pair{a.center.y,b.center.y},std::pair{a.center.z,b.center.z},std::pair{a.radius,b.radius}})check(std::bit_cast<unsigned>(x)==std::bit_cast<unsigned>(y),"Projected model bound changed a source float bit");check(a.node==b.node,"Projected model bound changed its actual node owner");}
}
Json actual(const char* path,const char* cpu_path){
    Cache cache(path);check(cache.is_mcc(),"Actual model source must be MCC");Json cpu;if(cpu_path){std::ifstream f(cpu_path);f>>cpu;check(cpu.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong Original CPU authority");}
    unsigned owners{},sections{},vertices{},actual_nodes{},skinned{},cpu_cases{},range_cases{};
    for(const auto name:{"objects\\characters\\masterchief\\masterchief","scenarios\\skies\\multi\\lockout\\lockout"}){
        const auto* tag=cache.query_tag({.group=tag_groups::render_model,.name=name,.require_address=true});if(!tag)continue;const auto source=rows(cache,*tag);++owners;
        for(const auto& row:source){
            if(row.cls!=1 && row.cls!=2){++skinned;continue;}
            const auto converted=project(row);const auto adapter=load_menu_world_source_model_section(cache,tag->datum,row.section);++sections;vertices+=row.count;actual_nodes+=unsigned(converted.node_map.size());
            check(adapter.model_geometry_class()==row.cls && adapter.vertex_count()==row.count && adapter.owner_datum()==tag->datum && adapter.section()==row.section,"Actual full-salt model/section identity changed");
            check(adapter.compression_flags()==row.flags,"Source compression flags changed");check(adapter.node_map().size()==converted.node_map.size() && std::ranges::equal(adapter.node_map(),converted.node_map),"Actual node104 bank not retained");check(std::ranges::equal(adapter.raw_model_header(),row.header) && row.header.size()==120,"Actual MCC model header was changed or fabricated");
            check(adapter.resources().size()==row.resources.size(),"Model resource owner count changed");for(unsigned r=0;r<row.resources.size();++r){const auto& before=row.resources[r];const auto& after=adapter.resources()[r];auto expected=before.bytes;if(before.type==56 && before.channel==2)expected=converted.basis;if(before.type==56 && before.channel==32)std::copy(converted.descriptors.begin(),converted.descriptors.end(),expected.begin());check(after.type==before.type && after.channel==before.channel && after.raw_offset==before.raw_offset && after.bytes==expected,"Model projection changed source position/UV/index/padding/typed ownership");}
            same_ranges(load_source_model_range_geometry(cache,tag->datum,row.section),source_model_range_geometry(adapter));++range_cases;
            if(cpu_path)for(const auto& expected:cpu.at("compression"))if(expected.at("model")==tag->datum && expected.at("section")==row.section){for(unsigned i=0;i<12;++i)check(std::bit_cast<unsigned>(adapter.compression_rows()[i])==expected.at("constants").at(i),"Actual c170 rows differ from genuine Original arithmetic on MCC bounds");++cpu_cases;}
        }
        const auto after=rows(cache,*tag);check(after.size()==source.size(),"Source model bank count changed");for(unsigned i=0;i<source.size();++i){check(source[i].header==after[i].header,"Source model raw header changed");for(unsigned r=0;r<source[i].resources.size();++r)check(source[i].resources[r].bytes==after[i].resources[r].bytes,"Source model raw resource bytes mutated");}
    }
    check(owners && sections,"No actual supported model owners exercised");if(cpu_path)check(cpu_cases==21,"Incomplete actual MCC Original compression CPU differential");
    return {{"map",cache.name()},{"owners",owners},{"rigid_sections",sections},{"vertices",vertices},{"real_node_bytes",actual_nodes},{"separate_skinned_sections",skinned},{"original_cpu_compression_cases",cpu_cases},{"direct_source_range_consumer_cases",range_cases},{"source_resources_unchanged",true},{"unexecuted_native_guest_header_semantics_claim",false},{"original_mesh_or_precision_parity_claim",false}};
}
}
int main(int argc,char**argv)try{portable();Json result;if(argc>1){if(std::string(argv[1]).empty() || !std::filesystem::is_regular_file(argv[1]))return 77;if(argc>2 && (std::string(argv[2]).empty() || !std::filesystem::is_regular_file(argv[2])))return 77;result=actual(argv[1],argc>2?argv[2]:nullptr);}std::cout<<Json{{"checks",checks},{"negative_cases",negatives},{"actual",result}}.dump()<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
