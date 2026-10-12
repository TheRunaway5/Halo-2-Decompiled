#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/source_shadow.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char**argv){try{
 if(argc!=3)throw FormatError("model root check needs fixture and maps");
 if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(std::filesystem::path(argv[2])/"mainmenu.map"))return 77;
 Cache cache(std::filesystem::path(argv[2])/"mainmenu.map");const auto level=cache.load_content(0);
 std::ifstream input(argv[1]);unsigned count=0,different=0;
 for(std::string line;std::getline(input,line);){if(line.empty()||line[0]=='#')continue;std::istringstream row(line);std::string name,datum;row>>name>>datum;const auto unit=std::uint32_t(std::stoul(datum,nullptr,16));std::array<std::uint32_t,13>expected;for(auto&w:expected){std::string hex;row>>hex;w=std::uint32_t(std::stoul(hex,nullptr,16));}if(!row)throw FormatError("incomplete original root fixture");
  const ScenarioObject*object=nullptr;
  for(const auto&candidate:level.objects)if(candidate.name>=0 &&level.object_names.at(candidate.name).name==name){if(object)throw FormatError("duplicate production scenario model name");object=&candidate;}
  if(!object || object->definition!=unit)throw FormatError("production scenario model is missing or uses a different source datum");
  const auto basis=authored_basis(object->rotation);
  const auto root=source_object_transform(object->position,basis.forward,basis.up,object->scale,false);
  const auto words=std::bit_cast<std::array<std::uint32_t,13>>(root);unsigned errors=0;for(unsigned i=0;i<13;++i)errors+=words[i]!=expected[i];
  different+=errors;++count;std::cout<<name<<": "<<errors<<" differing root words\n";
 }
 if(count!=10)throw FormatError("all10authored menu models were not checked");
 std::cout<<count<<" production roots / "<<count*13<<" source words; "<<different<<" differences\n";return different?2:0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
