#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <limits>

namespace halo2::test {
// Opt-in diagnostic of one actual joined-body force call. It only exports
// the running ABI and accessible memory before/after the original owner.
class LivePhantomOriginalCaptureTu15 {
  using Json=nlohmann::json;
  std::filesystem::path directory_;
  Json record_;
  bool active_{},complete_{};
  std::uint32_t component_{};
  static void require(bool yes,const char *why){if(!yes)throw FormatError(why);}
  static std::string hex(std::span<const std::byte> bytes){
    constexpr char digits[]="0123456789abcdef";std::string s;s.reserve(2*bytes.size());
    for(auto byte:bytes){const auto b=std::to_integer<unsigned>(byte);s+=digits[b>>4];s+=digits[b&15];}return s;
  }
  static Json abi(const SourcePhysicsWorldRuntimeAbi &state){
    Json values=Json::array();static_assert(std::numeric_limits<long double>::digits==64);
    for(unsigned n=0;n<state.x87_depth;++n)values.push_back(hex(std::as_bytes(std::span(&state.x87[n],1)).first(10)));
    require(state.floating_point.has_value(),"Original force capture requires actual FP controls");
    return {{"gp",state.gp},{"xmm",state.xmm},{"eflags",state.eflags},{"fs_base",state.fs_base},
        {"x87_values",values},{"x87_status",state.x87_status},
        {"x87_control_word",state.floating_point->x87_control_word},{"mxcsr",state.floating_point->mxcsr}};
  }
  static void write(const std::filesystem::path &path,std::span<const std::byte> bytes){
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
    require(bool(out),"Cannot retain actual original force memory");
  }
public:
  explicit LivePhantomOriginalCaptureTu15(std::filesystem::path path):directory_(std::move(path)){}
  void observe(bool entering,const SourcePhysicsWorldRuntimeAbi &state,const SourceRuntimeMemory &memory,
      std::uint32_t component,std::uint32_t unit){
    if(complete_)return;
    const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
    if(entering){
      if(active_ || unit==null_datum)return;
      const auto members=word(component+0x94u);if(!members)return;
      const auto count=word(members+4u),rows=word(members);require(count<=1024,"Invalid live force member extent");
      bool found=false;for(unsigned n=0;n<count;++n)found|=word(rows+8*n+4u)==unit;
      if(!found)return;
      std::filesystem::create_directories(directory_);
      record_={{"entry",0x1da750u},{"component",component},{"object",word(component+8u)},{"unit",unit},
          {"return",word(state.gp[4])},{"before",abi(state)},{"regions",Json::array()},
          {"reference","English TU1.5 only"},{"boundary","Read-only actual joined-body force call input/output; no supplied physical result"}};
      memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> bytes){
        const auto file="before-"+std::to_string(address)+".bin";write(directory_/file,bytes);
        record_["regions"].push_back({{"address",address},{"size",bytes.size()},{"before_file",file}});
      });
      component_=component;active_=true;
    }else if(active_ && component==component_){
      record_["after"]=abi(state);
      for(auto &region:record_["regions"]){
        const auto size=region.at("size").get<std::size_t>();std::vector<std::byte> prior(size);
        std::ifstream in(directory_/region.at("before_file").get<std::string>(),std::ios::binary);
        in.read(reinterpret_cast<char *>(prior.data()),size);require(bool(in),"Cannot read actual force input bank");
        const auto address=region.at("address").get<std::uint32_t>();const auto current=memory.view(address,size);
        if(std::memcmp(prior.data(),current.data(),size)){
          const auto file="after-"+std::to_string(address)+".bin";write(directory_/file,current);region["after_file"]=file;
        }
      }
      std::ofstream out(directory_/"capture.json");out<<record_.dump(2)<<'\n';require(bool(out),"Cannot retain actual force ABI");
      complete_=true;active_=false;
    }
  }
};
}
