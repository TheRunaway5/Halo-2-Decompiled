#include "halo2/source_prt.hpp"
#include "halo2/model.hpp"
#include "halo2/world_material.hpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <string_view>
#include <cmath>
#include <fstream>
#include <bit>
using namespace halo2;
int main(int argc,char**argv){try{
 std::cout<<std::setprecision(9);
 const auto write=[](const SourcePrtSh&sh){for(auto&channel:sh.coefficients)for(auto v:channel)std::cout<<v<<' ';std::cout<<sh.incident.x<<' '<<sh.incident.y<<' '<<sh.incident.z<<'\n';};
 if(argc==2 && std::string_view(argv[1])=="--inverse-probe"){
  std::array<float,9> m;while(std::cin>>m[0]){for(unsigned i=1;i<9;++i)std::cin>>m[i];for(auto v:source_prt_inverse_basis(m))std::cout<<v<<' ';std::cout<<'\n';}return 0;
 }
 if(argc==2 && std::string_view(argv[1])=="--rotation-probe"){
  std::array<float,9> sh,m;
  while(std::cin>>sh[0]){for(unsigned i=1;i<9;++i)std::cin>>sh[i];for(auto&v:m)std::cin>>v;for(auto v:source_prt_rotate(sh,m))std::cout<<v<<' ';std::cout<<'\n';}return 0;
 }
 if(argc==2 && std::string_view(argv[1])=="--sample-probe"){
  unsigned count;Vec3 point;while(std::cin>>count>>point.x>>point.y>>point.z){std::vector<SourcePrtShRecord> records(count);
   for(auto&r:records){std::cin>>r.position.x>>r.position.y>>r.position.z;for(auto&channel:r.lighting.coefficients)for(auto&v:channel)std::cin>>v;std::cin>>r.lighting.incident.x>>r.lighting.incident.y>>r.lighting.incident.z;r.lighting.incident_valid=true;}write(source_prt_sample(records,point));}return 0;
 }
 if(argc==2 && std::string_view(argv[1])=="--projection-probe"){
  SourcePrtDefinition d;d.order=3;d.lods.push_back({});SourcePrtSh sh;
  while(std::cin>>d.clusters>>d.pca_vectors){d.cluster_basis.resize(std::size_t(d.clusters)*(d.pca_vectors+1)*27);
   for(auto&v:d.cluster_basis)std::cin>>v;for(auto&channel:sh.coefficients)for(auto&v:channel)std::cin>>v;
   for(auto v:source_prt_project(d,5,sh))for(auto component:v)std::cout<<component<<' ';std::cout<<'\n';}return 0;
 }
 if(argc==3 && std::string_view(argv[1])=="--nodes-probe"){
  Cache cache(argv[2]);unsigned datum,section;
  while(std::cin>>datum>>section){const auto nodes=cache.source_prt_section_nodes(datum,section);
   std::cout<<nodes.size();for(auto node:nodes)std::cout<<' '<<unsigned(node);std::cout<<'\n';}return 0;
 }
 if(argc==3 && std::string_view(argv[1])=="--bank-probe"){
  Cache cache(argv[2]);const auto lighting=cache.load_source_prt_lighting();unsigned datum,section,lod,count;
  while(std::cin>>datum>>section>>lod>>count){const auto model=cache.load_model(datum,false);const auto definition=*cache.load_source_prt_definition(datum);
   std::vector<SourcePrtSh> nodes;
   for(unsigned i=0;i<count;++i){unsigned node;Transform t;std::cin>>node>>t.scale;for(auto&v:t.basis)std::cin>>v.x>>v.y>>v.z;
    std::cin>>t.position.x>>t.position.y>>t.position.z;nodes.push_back(source_prt_node_lighting(model,node,t,lighting));}
   const auto bank=source_prt_bank(definition,lod,nodes);for(auto v:bank->constants)for(auto c:v)std::cout<<c<<' ';std::cout<<'\n';
  }return 0;
 }
 // Independently paused retail object/cache fixture. The source heap
 // includes unwritten ambient padding; only those words are excluded.
 unsigned live_banks=0,live_components=0;
 {
  Cache cache("assets/disc/maps/zanzibar.map");const auto lighting=cache.load_source_prt_lighting();
  std::ifstream fixture("tests/fixtures/retail_prt_object_banks.tsv");
  if(!fixture)throw std::runtime_error("missing retail PRT object-bank fixture");
  unsigned datum,lod,section_count,node_count;
  while(fixture>>datum>>lod>>section_count){
   std::vector<unsigned> sections(section_count);for(auto&v:sections)fixture>>v;
   fixture>>node_count;std::vector<Transform> world_nodes(node_count);
   for(auto&t:world_nodes){std::array<float,13> values;for(auto&v:values){std::uint32_t bits;fixture>>std::hex>>bits>>std::dec;v=std::bit_cast<float>(bits);}
    t={values[0],{{{values[1],values[2],values[3]},{values[4],values[5],values[6]},{values[7],values[8],values[9]}}},{values[10],values[11],values[12]}};}
   const auto model=cache.load_model(datum,false);const auto definition=*cache.load_source_prt_definition(datum);
   const auto banks=source_prt_section_banks(cache,model,definition,lod,sections,world_nodes,lighting);
   if(banks.size()!=sections.size())throw std::runtime_error("source PRT section bank admission");
   for(const auto&b:banks){unsigned section,count;fixture>>section>>count;
    if(section!=b.section || count!=b.bank->constants.size()*4)throw std::runtime_error("source PRT bank dimensions");
    for(unsigned i=0;i<count;++i){std::uint32_t expected;fixture>>std::hex>>expected>>std::dec;
     if(i==3 || (i>=8 && (i-8)%16==3))continue;
     if(std::bit_cast<std::uint32_t>(b.bank->constants[i/4][i%4])!=expected)
      throw std::runtime_error("paused source PRT coefficient/incident mismatch");++live_components;
    }++live_banks;
   }
  }
  if(!fixture.eof() || live_banks!=8)throw std::runtime_error("invalid retail PRT bank fixture");
 }
 unsigned models=0,records=0,streams=0,vertices=0;
 for(const auto& directory:{"assets/disc/maps","assets/map-pack/maps"})for(const auto&entry:std::filesystem::directory_iterator(directory)){
  if(entry.path().extension()!=".map")continue;
  const std::string maps=" ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate ";
  if(maps.find(" "+entry.path().stem().string()+" ")==std::string::npos)continue;
  Cache cache(entry.path());
  const auto lighting=cache.load_source_prt_lighting();records+=lighting.records.size();
  for(const auto&t:cache.tags())if(t.group=="mode"&&t.address){
   const auto definition=cache.load_source_prt_definition(t.datum);if(!definition)continue;const auto&d=*definition;
   if(d.order!=3 || !d.clusters || d.pca_vectors!=4 || d.lods.empty() || d.stream_descriptor[0]!=std::byte{57} || d.stream_descriptor[1]!=std::byte{10} || d.stream.size()%10)
    throw std::runtime_error("source PRT metadata inventory changed: "+cache.name()+" "+t.name);
   for(const auto& section:d.lods.front().sections){const auto stream=cache.load_source_prt_vertex_stream(t.datum,5,section.section);
    if(!stream)throw std::runtime_error("source PRT section stream missing");
    const auto nodes=cache.source_prt_section_nodes(t.datum,section.section);if(nodes.empty())throw std::runtime_error("source PRT section node list empty");
    for(const auto&v:*stream){if(v.cluster<0 || v.cluster>=d.clusters*2)throw std::runtime_error("source PRT cluster outside bank");
     for(auto w:v.weights)if(w< -1 || w>1)throw std::runtime_error("source PRT weight decode");}++streams;vertices+=stream->size();}
   const auto sh=source_prt_sample(lighting.records,lighting.records.empty()?Vec3{}:lighting.records.front().position);
   const auto constants=source_prt_project(d,5,sh);
   if(constants.size()!=std::size_t(d.clusters)*4)throw std::runtime_error("source PRT constant dimensions");
   for(auto v:constants)for(auto c:v)if(!std::isfinite(c))throw std::runtime_error("invalid source PRT projection");
   ++models;
  }
 }
 if(models!=13)throw std::runtime_error("source PRT model inventory changed");
 std::cout<<"PRT metadata: "<<models<<" models, "<<records<<" authored SH records, "<<streams<<" section streams, "<<vertices<<" source vertices, "<<live_banks<<" paused retail banks / "<<live_components<<" bit-exact components\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
