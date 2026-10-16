// Differential of actual live source banks and their multiplayer render input.
#include "halo2/source_contrail_draw.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstring>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
std::vector<std::byte> read_file(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary|std::ios::ate);check(bool(f),"Actual contrail input file absent");std::vector<std::byte>b(static_cast<std::size_t>(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),b.size());check(bool(f),"Actual contrail input file truncated");return b;}
Vec3 xyz(const Json &v){return {v.at(0),v.at(1),v.at(2)};}
template<class T>bool bytes_equal(const T &a,const T &b){return !std::memcmp(&a,&b,sizeof(T));}
}
int main(int argc,char **argv)try {
 check(argc==7,"Supply map, shared maps, original XBE, live contrail capture, actual cameras and report");
 const auto original=OriginalData::load_xbe(argv[3]);check(original->image==OriginalData::Image::TitleUpdate15,"Contrail replay requires English TU1.5");
 Cache cache(argv[1],argv[2],original);SourceContrailScene scene(cache);
 const std::filesystem::path capture=argv[4];Json record,cameras;std::ifstream(capture/"capture.json")>>record;std::ifstream(argv[5])>>cameras;
 SourceRuntimeMemory memory;std::size_t retained{};
 for(const auto &r:record.at("regions")){auto bytes=read_file(capture/r.at("file").get<std::string>());check(bytes.size()==r.at("size"),"Contrail capture size differs");retained+=bytes.size();memory.mount(r.at("address"),bytes,SourceRuntimeMemory::Access::ReadWrite,"Actual live source capture");}
 const auto snapshot=capture_source_contrail_snapshot(memory);auto image=load_source_runtime_image(argv[3],SourcePhysicsContactImage::TitleUpdate15);
 SourceContrailSnapshotMemory sparse(snapshot,image);std::size_t snapshot_bytes{};
 for(const auto &r:snapshot.slices){std::vector<std::byte>b(r.bytes.size());sparse.read(r.address,b);check(b==r.bytes,"Sparse snapshot changed actual source bytes");snapshot_bytes+=b.size();}
 const auto before=memory.checkpoint();unsigned compared_views{},draws{},vertices{},colored_draws{},rejected_mutation{};Json cases=Json::array();
 std::sort(cameras.begin(),cameras.end(),[&](const auto&a,const auto&b){auto d=[&](const auto&c){return std::abs(std::int64_t(c.at("tick").template get<unsigned>())-record.at("tick").get<unsigned>());};return d(a)<d(b);});
 for(const auto &camera:cameras){if(compared_views==20)break;
  const auto eye=xyz(camera.at("position")),forward=xyz(camera.at("forward"));const std::array<ShaderVector,192> constants{};const ShaderVector viewport{0,0,960,270},clip{0,16777215.f,.015625f,1024};
  const auto full=scene.draw(memory,eye,forward,constants,viewport,clip),replica=scene.draw(snapshot,eye,forward,constants,viewport,clip);
  check(full.size()==replica.size(),"Snapshot original geometry draw count differs");
  for(unsigned n=0;n<full.size();++n){const auto&a=full[n],&b=replica[n];
   check(a.datum==b.datum && a.definition==b.definition && a.chain==b.chain && a.blend==b.blend && a.texture==b.texture && a.pixel.words==b.pixel.words && a.pixel.fragment_offsets==b.pixel.fragment_offsets && a.sampler==b.sampler && a.blend_operands==b.blend_operands && bytes_equal(a.constants,b.constants) && bytes_equal(a.viewport,b.viewport) && bytes_equal(a.clip_range,b.clip_range) && bytes_equal(a.command,b.command),"Snapshot original geometry inputs or sort command differ");
   check(a.vertices.size()==b.vertices.size() && (!a.vertices.size() || !std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(SourceContrailVertex))),"Snapshot original geometry vertex bytes differ");
   bool colored{};for(unsigned q=0;q+2<a.vertices.size();q+=3) {
    const auto &x=a.vertices[q],&y=a.vertices[q+1],&z=a.vertices[q+2];
    colored|=((x.color|y.color|z.color)&0xff000000u) && length(cross(y.position-x.position,z.position-x.position))>1e-6f;
   }
   colored_draws+=colored;++draws;vertices+=a.vertices.size();
  }
  cases.push_back({{"camera_tick",camera.at("tick")},{"view",camera.at("view")},{"draws",full.size()}});++compared_views;
 }
 check(memory.checkpoint()==before,"Contrail rendering mutated actual live source input");
 try{const std::array<std::byte,1>b{};sparse.write(snapshot.slices.front().address,b);}catch(const FormatError&){++rejected_mutation;}
 check(rejected_mutation==1,"Contrail replica input accepted a write");
 bool missing_rejected{};try{std::array<std::byte,4>b{};sparse.read(0x506b98,b);}catch(const FormatError&){missing_rejected=true;}
 check(missing_rejected,"Replica substituted missing mutable input");
 bool overlap_rejected{};try{auto invalid=snapshot;invalid.slices.push_back(invalid.slices.front());validate_source_contrail_snapshot(invalid);}catch(const FormatError&){overlap_rejected=true;}
 check(overlap_rejected,"Replica accepted overlapping source input");
 check(compared_views && draws && vertices && colored_draws,"Live contrail replay did not exercise nonempty original geometry");
 Json report{{"passed",true},{"reference","English TU1.5 actual live source input"},{"source_tick",record.at("tick")},{"retained_bytes",retained},{"snapshot_bytes",snapshot_bytes},{"snapshot_slices",snapshot.slices.size()},{"compared_actual_views",compared_views},{"original_geometry_draws",draws},{"original_colored_triangle_draws",colored_draws},{"compared_vertex_records",vertices},{"source_unchanged",true},{"mutable_missing_input_rejected",missing_rejected},{"overlap_rejected",overlap_rejected},{"cases",cases},{"boundary","Complete captured live bank versus sparse multiplayer inputs through same original TU1.5 geometry callback, using actual session camera vectors. Projection constants are equal declared fixture inputs. This is not a retail framebuffer comparison."}};
 std::ofstream(argv[6])<<report.dump(2)<<'\n';std::cout<<"PASS actual contrail snapshot geometry: "<<draws<<" draws / "<<vertices<<" vertices / "<<compared_views<<" actual views\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
