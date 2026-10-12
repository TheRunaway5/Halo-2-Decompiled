#include "halo2/frontend_model_locals.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace {
using namespace halo2;
void require(bool value,const char*message){if(!value)throw FormatError(message);}
std::array<std::uint32_t,8> packed(const BonePose&pose) {
  return {std::bit_cast<std::uint32_t>(pose.rotation[0]),std::bit_cast<std::uint32_t>(pose.rotation[1]),
      std::bit_cast<std::uint32_t>(pose.rotation[2]),std::bit_cast<std::uint32_t>(pose.rotation[3]),
      std::bit_cast<std::uint32_t>(pose.translation.x),std::bit_cast<std::uint32_t>(pose.translation.y),
      std::bit_cast<std::uint32_t>(pose.translation.z),std::bit_cast<std::uint32_t>(pose.scale)};
}
BonePose unpacked(std::array<std::uint32_t,8> words) {
  BonePose out;
  for(unsigned k=0;k<4;++k)out.rotation[k]=std::bit_cast<float>(words[k]);
  out.translation={std::bit_cast<float>(words[4]),std::bit_cast<float>(words[5]),std::bit_cast<float>(words[6])};
  out.scale=std::bit_cast<float>(words[7]);return out;
}
void raw_primary() {
  RenderModel model;ModelBone bone;bone.name=1;model.bones.push_back(bone);
  AnimationClip clip;clip.frame_count=2;clip.nodes={1};clip.components={1};clip.channels.resize(1);
  clip.channels[0][0].times={0,1};clip.channels[0][0].keys={{1,2,3,4},{2,3,4,5}};
  const auto locals=frontend_model_locals(model,clip,.5f,{},nullptr,true);
  require(locals[0].rotation==std::array<float,4>{1.5f,2.5f,3.5f,4.5f},
      "frontend primary normalized raw source quaternion");
  require(packed(locals[0])[7]==0x3f800000u,"frontend primary changed absent scale");
  require(frontend_model_locals(model,clip,.99995f)[0].rotation==clip.channels[0][0].keys[0],
      "menu primary interpolated its retained fractional clock");
}
void local_probe(const std::filesystem::path&maps) {
  Cache cache(maps/"mainmenu.map");
  for(std::string line;std::getline(std::cin,line);) {
    if(line.empty())continue;
    std::istringstream input(line);std::uint32_t unit,graph,index,frame,mask_count,grip_requested;
    require(bool(input>>std::hex>>unit>>graph>>index>>frame>>mask_count>>grip_requested),"invalid local probe input");
    require(mask_count==0 || mask_count==8,"invalid local probe mask count");
    std::vector<std::uint32_t> mask(mask_count);
    for(auto&word:mask)require(bool(input>>word),"local probe mask truncated");
    const auto model=cache.load_model(unit,false);const auto clip=cache.load_animation(graph,index);
    const auto primary=cache.load_frontend_model_primary_animation(graph,index);
    auto locals=frontend_model_locals(model,clip,std::bit_cast<float>(frame),mask,&primary);
    if(grip_requested)if(const auto grip=cache.load_frontend_model_grip(unit))apply_frontend_model_grip(model,locals,*grip,mask);
    std::cout<<std::hex<<locals.size();
    for(unsigned n=0;n<locals.size();++n) {
      std::cout<<' '<<model.bones[n].name;
      for(auto word:packed(locals[n]))std::cout<<' '<<std::setw(8)<<std::setfill('0')<<word;
    }
    std::cout<<'\n';
  }
}
void original_primary(const std::filesystem::path&fixture,const std::filesystem::path&maps) {
  Cache cache(maps/"mainmenu.map");std::ifstream input(fixture);
  require(bool(input),"original primary local fixture missing");unsigned cases=0,words=0;
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::uint32_t unit,graph,index,count,frame,mask_count;
    require(bool(row>>std::hex>>unit>>graph>>index>>count>>frame>>mask_count),"invalid primary fixture metadata");
    require(mask_count==0 || mask_count==8,"invalid primary fixture mask");
    std::vector<std::uint32_t> mask(mask_count);for(auto&word:mask)require(bool(row>>word),"primary mask truncated");
    std::vector<std::array<std::uint32_t,8>> expected(count);
    for(auto&node:expected)for(auto&word:node)require(bool(row>>word),"primary original output truncated");
    const auto model=cache.load_model(unit,false);const auto clip=cache.load_animation(graph,index);
    const auto primary=cache.load_frontend_model_primary_animation(graph,index);
    const auto locals=frontend_model_locals(model,clip,std::bit_cast<float>(frame),mask,&primary);
    require(locals.size()==count && clip.nodes.size()==count,"primary model/graph count changed");
    for(unsigned n=0;n<count;++n) {
      require(model.bones[n].name==clip.nodes[n],"primary original graph/model order differs");
      if(!mask.empty() && !(mask[n/32]&(1u<<(n%32))))continue;
      const auto actual=packed(locals[n]);
      for(unsigned k=0;k<8;++k) {
        const unsigned component=k<4?0:k<7?1:2;
        if(!(clip.components[n]&(1u<<component)))continue;
        if(actual[k]!=expected[n][k])throw FormatError("primary source codec differs at clip "+std::to_string(index)+
            " framebits "+std::to_string(frame)+" node "+std::to_string(n)+" word "+std::to_string(k));
        ++words;
      }
    }
    ++cases;
  }
  require(cases==54 && words>5000,"primary source codec coverage changed");
  std::cout<<cases<<" original full/initial/deferred primary cases, "<<words<<" authored component words exact\n";
}
void original_grips(const std::filesystem::path&fixture,const std::filesystem::path&maps,
                    bool omit_grip,bool visible) {
  Cache cache(maps/"mainmenu.map");std::ifstream input(fixture);
  require(bool(input),"original grip local fixture missing");std::string line;
  unsigned cases=0,rotations=0,words=0;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string label;std::uint32_t unit,graph,index,count,mask_count;
    require(bool(row>>label>>std::hex>>unit>>graph>>index>>count),"invalid grip fixture metadata");
    require(bool(row>>mask_count) && (mask_count==0 || mask_count==8),"invalid original grip mask size");
    std::vector<std::uint32_t> mask(mask_count);
    for(auto&word:mask)require(bool(row>>word),"original grip mask truncated");
    const auto model=cache.load_model(unit,false);
    const auto grip=cache.load_frontend_model_grip(unit);
    require(grip && grip->source_graph==graph && grip->source_index==index,
        "native authored menu grip differs from live original selector");
    require(model.bones.size()==count && grip->nodes.size()==count,"original grip node count differs");
    if(mask_count) {
      const auto deferred=cache.load_frontend_model_deferred_mask(unit);
      for(unsigned i=0;i<mask_count;++i)
        require((~deferred[i])==mask[i],"native authored deferred mask differs from original caller");
    }
    std::vector<BonePose> locals;std::vector<std::array<std::uint32_t,8>> expected(count);
    for(unsigned n=0;n<count;++n) {
      std::array<std::uint32_t,8> record;
      for(auto&v:record)require(bool(row>>v),"original grip input words truncated");
      locals.push_back(unpacked(record));
      require(model.bones[n].name==grip->nodes[n],"live bank graph/model order unverified");
      if(grip->rotations[n]) {
        ++rotations;
        if(omit_grip)locals[n].rotation=model.bones[n].bind.rotation;
      }
    }
    for(auto&record:expected)for(auto&v:record)require(bool(row>>v),"original grip output words truncated");
    if(!omit_grip)apply_frontend_model_grip(model,locals,*grip,mask);
    for(unsigned n=0;n<count;++n) {
      if(packed(locals[n])!=expected[n])
        throw FormatError("missing or differing authored grip at "+label+" node "+std::to_string(n));
      words+=8;
    }
    ++cases;
  }
  require(visible?(cases==1 && (rotations==11 || rotations==15) && (words==264 || words==440)):
      (cases==10 && rotations==130 && words==3520),"original menu grip coverage changed");
  std::cout<<cases<<" original objects, "<<rotations<<" grip rotations, "<<words<<" local words exact\n";
}
}
int main(int argc,char**argv) {
  try {
    if(argc==3 && std::string(argv[1])=="--local-probe") {local_probe(argv[2]);return 0;}
    if(argc==4 && std::string(argv[1])=="--primary") {
      if(!std::filesystem::exists(argv[2]) || !std::filesystem::exists(std::filesystem::path(argv[3])/"mainmenu.map"))return 77;
      original_primary(argv[2],argv[3]);return 0;
    }
    require(argc==3 || argc==4,"frontend model locals tests require fixture/maps");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(std::filesystem::path(argv[2])/"mainmenu.map"))return 77;
    raw_primary();original_grips(argv[1],argv[2],argc==4 && std::string(argv[3])=="--omit-grip",
        argc==4 && std::string(argv[3])=="--visible");return 0;
  }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
