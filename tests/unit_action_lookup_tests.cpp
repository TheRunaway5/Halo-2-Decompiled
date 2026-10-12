#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv) {try {
  if(argc!=3)throw FormatError("Supply map and original action lookup fixture");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  auto cache=std::make_shared<Cache>(argv[1]);UnitAnimationMetadataStore metadata(cache);
  const auto memory=metadata.graph_memory();std::ifstream input(argv[2]);unsigned count=0,mapped=0,missing=0;
  for(std::string line;std::getline(input,line);) {
    if(line.empty()||line[0]=='#')continue;
    std::uint32_t graph,mode,wc,weapon,action,alternate,expected_class,packed,resolved;
    std::istringstream row(line);
    if(!(row>>std::hex>>graph>>mode>>wc>>weapon>>action>>alternate>>expected_class>>packed>>resolved))
      throw FormatError("Truncated original action query");
    if(source_animation_weapon_class(memory.graph_address(graph),wc,weapon,memory)!=expected_class)
      throw FormatError("Original weapon class map differs");
    bool unavailable=false;
    try {
      const auto clip=cache->load_unit_weapon_action(graph,mode,wc,weapon,action,alternate);
      if((packed>>16)==65535||clip.source_graph!=resolved||clip.source_index!=(packed>>16))
        throw std::runtime_error("Live authored action differs from original graph query at row "+std::to_string(count));
    }catch(const FormatError &e) {
      if((packed>>16)!=65535)throw;
      if(std::string(e.what()).find("authored unit weapon action missing:")!=0)throw;
      unavailable=true;
    }
    ++count;mapped+=wc!=expected_class;missing+=unavailable;
  }
  if(count!=1584||!mapped||!missing)throw FormatError("Incomplete original action query coverage");
  std::cout<<count<<" original authored action queries, "<<mapped<<" mapped class queries, "<<missing<<" genuine unavailable actions\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
