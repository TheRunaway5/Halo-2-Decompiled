#include "halo2/physics_body_matrix.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
std::uint32_t word(const std::string&text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string&text) {
  check(text.size()%2==0,"Original matrix fixture byte extent");
  std::vector<std::byte>out;
  for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(word(text.substr(n,2))));
  return out;
}
struct Memory final:halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>buffers;
  auto at(std::uint32_t p,std::size_t n)const {
    auto q=buffers.upper_bound(p);
    if(q==buffers.begin())throw std::runtime_error("Required original body matrix storage unavailable");
    --q;
    if(std::uint64_t(p-q->first)+n>q->second.size())
      throw std::runtime_error("Required original body matrix storage unavailable");
    return q;
  }
  void read(std::uint32_t p,std::span<std::byte>out)const override {
    const auto q=at(p,out.size());std::copy_n(q->second.begin()+(p-q->first),out.size(),out.begin());
  }
  void write(std::uint32_t p,std::span<const std::byte>in)override {
    const auto q=at(p,in.size());std::copy(in.begin(),in.end(),buffers.at(q->first).begin()+(p-q->first));
  }
};
using Store=halo2::SourcePhysicsContactRecordStore;
struct ByteStore {
  std::uint32_t instruction,address;std::byte before,written;
  friend bool operator==(const ByteStore&,const ByteStore&)=default;
};
std::vector<ByteStore> byte_stores(const std::vector<Store>&rows) {
  std::vector<ByteStore>out;
  for(const auto&s:rows) {
    check(s.before.size()==s.written.size(),"Complete original matrix store");
    for(std::size_t n=0;n<s.before.size();++n)
      out.push_back({s.instruction,s.address+std::uint32_t(n),s.before[n],s.written[n]});
  }
  return out;
}
struct Case {
  std::uint32_t index,component,output,scratch;
  Memory before;std::map<std::uint32_t,std::vector<std::byte>>after;
  std::vector<Store>stores;
};
}
int main(int argc,char**argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original ten-body matrix fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream input(argv[1]);std::vector<Case>cases;
    for(std::string line;std::getline(input,line);) {
      std::istringstream in(line);std::string kind;in>>kind;
      if(kind.empty())continue;
      if(kind=="C") {
        std::string index,component,output,scratch;in>>index>>component>>output>>scratch;
        cases.push_back({word(index),word(component),word(output),word(scratch),{},{},{}});
      } else {
        check(!cases.empty(),"Original matrix owning case");auto&c=cases.back();
        if(kind=="M"||kind=="A") {
          std::string address,raw;in>>address>>raw;
          auto&bank=kind=="M"?c.before.buffers:c.after;
          check(bank.emplace(word(address),bytes(raw)).second,"Unique actual matrix input span");
        } else if(kind=="W") {
          std::string site,address,before,written;in>>site>>address>>before>>written;
          c.stores.push_back({word(site),word(address),bytes(before),bytes(written)});
        } else throw std::runtime_error("Unknown original matrix fixture record");
      }
    }
    check(cases.size()==10,"Actual ten-body constructor fixture count");
    std::size_t stores=0,compared_bytes=0;
    for(const auto&c:cases) {
      auto memory=c.before;std::vector<Store>observed;
      halo2::source_physics_body_matrix_1_1(memory,c.component,c.index,c.output,c.scratch,{},
          [&](const auto&s){observed.push_back(s);});
      check(memory.buffers==c.after,"Actual original full body/slot/motion/output buffers differ");
      // Unicorn splits each original MOVAPS into two8-byte memory callbacks.
      // Compare ordered instruction/address/before/written bytes without making
      // that emulator callback width a different native algorithm.
      const auto actual=byte_stores(observed),expected=byte_stores(c.stores);
      check(actual==expected,"Actual original matrix ordered stores differ");
      check(observed.size()==17 && expected.size()==116,"Complete matrix store extent");
      stores+=observed.size();compared_bytes+=actual.size();
    }
    // Native required-owner negative control only. This deliberately changes
    // bit6 and is not advertised as an original positive correction case.
    const auto&c=cases.front();auto memory=c.before;
    auto&component=memory.buffers.at(c.component);component[4]|=std::byte{0x40};
    const auto before_output=memory.buffers.at(c.output);
    std::vector<Store>observed;bool stopped=false;
    try {
      halo2::source_physics_body_matrix_1_1(memory,c.component,c.index,c.output,c.scratch,{},
          [&](const auto&s){observed.push_back(s);});
    } catch(const std::runtime_error&error) {
      stopped=std::string(error.what()).find("2E0B60")!=std::string::npos;
    }
    check(stopped && observed.size()==4 && memory.buffers.at(c.output)==before_output,
        "Unmeasured component correction must stop after actual transform copies");
    check(memory.buffers.at(c.scratch)==c.after.at(c.scratch),
        "Required correction boundary changed original preceding matrix copies");
    std::cout<<cases.size()<<" actual ten-body matrices, "<<stores
             <<" logical stores / "<<compared_bytes
             <<" ordered source bytes; required component correction stops\n";
    return 0;
  }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
}
