#include "halo2/source_shadow.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
int main(int argc,char **argv) {
  if(argc!=2)throw FormatError("Original TU1.5 attachment fixture required");
  std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
      fixture.at("cases").size()!=80)throw FormatError("Incomplete original attachment fixture");
  const auto read=[]<class T>(const auto &words) {
    std::array<std::uint32_t,sizeof(T)/4> raw{};
    for(unsigned i=0;i<raw.size();++i)raw[i]=words.at(i).template get<std::uint32_t>();
    return std::bit_cast<T>(raw);
  };
  unsigned count=0;
  const auto check=[&](const auto &actual,const auto &expected,const char *phase) {
    const auto words=std::bit_cast<std::array<std::uint32_t,9>>(actual);
    for(unsigned i=0;i<words.size();++i)if(words[i]!=expected.at(i).template get<std::uint32_t>())
      throw FormatError(std::string("Original attachment ")+phase+" mismatch at case "+
          std::to_string(count)+" word "+std::to_string(i));
  };
  for(const auto &row:fixture.at("cases")) {
    const auto current=read.template operator()<ObjectAttachmentFrame>(row.at("current"));
    const auto parent=read.template operator()<Transform>(row.at("parent_marker"));
    const auto child=read.template operator()<Transform>(row.at("child_marker"));
    const auto node=read.template operator()<Transform>(row.at("parent_node"));
    const auto aligned=source_object_attachment_frame(current,parent,child,row.at("named"));
    check(aligned,row.at("aligned"),"aligned");
    check(source_object_attachment_local_frame(aligned,node),row.at("local"),"local");
    ++count;
  }
  std::cout<<count<<" exact TU1.5 attachment alignment/local-frame pairs\n";
}
