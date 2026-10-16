#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/frontend_model_primary.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=3)throw FormatError("Original world-node fixture and opening map required");
  std::ifstream file(argv[1]);nlohmann::json original;file>>original;
  if(original.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
    throw FormatError("World-node fixture has the wrong original image");
  const Cache cache(argv[2]);
  const auto model=cache.load_model_hierarchy(std::stoul(original.at("object").get<std::string>(),nullptr,16));
  const auto clip=cache.load_frontend_model_primary_animation(std::stoul(original.at("graph").get<std::string>(),nullptr,16),original.at("clip_index"));
  if(model.bones.size()!=original.at("node_count") || model.bones.front().name==clip.nodes.front())
    throw FormatError("Actual world model/graph no longer exercise distinct authored node names");
  unsigned checked=0;
  for(const auto &row:original.at("cases")) {
    const auto root=std::bit_cast<Transform>(row.at("root").get<std::array<std::uint32_t,13>>());
    const auto nodes=pose_world_object_nodes_frame(model,root,clip,row.at("frame"));
    for(unsigned i=0;i<nodes.size();++i) {
      const auto actual=std::bit_cast<std::array<std::uint32_t,13>>(nodes[i]);
      for(unsigned k=0;k<13;++k) {
        const auto expected=row.at("world").at(i*13+k).get<std::uint32_t>();
        if(actual[k]!=expected) {
          std::cerr<<"frame="<<row.at("frame")<<" node="<<i<<" word="<<k<<std::hex
              <<" actual="<<actual[k]<<" original="<<expected<<'\n';
          throw FormatError("Original packed-sample/model-order world matrix differs");
        }
        ++checked;
      }
    }
  }
  std::cout<<checked<<" exact original sampled world-matrix words with distinct model/graph node names passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
