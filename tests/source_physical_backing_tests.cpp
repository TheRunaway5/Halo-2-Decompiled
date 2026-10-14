#include "halo2/source_runtime_host_physical_memory.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
bool host_writable(std::uint64_t address){
  std::ifstream maps("/proc/self/maps");std::string line;
  while(std::getline(maps,line)){
    std::istringstream row(line);std::string range,permissions;row>>range>>permissions;
    const auto split=range.find('-');
    if(split!=std::string::npos && address>=std::stoull(range.substr(0,split),nullptr,16) &&
       address<std::stoull(range.substr(split+1),nullptr,16))return permissions.size()>1 && permissions[1]=='w';
  }
  throw std::runtime_error("Native physical mapping is unavailable");
}
void large(){
  SourceRuntimeMemory memory;constexpr auto resident=20u*1024u*1024u;
  memory.reserve(0x20000000u,resident,4096,"actual live source data");
  std::array<std::byte,4096> page;
  for(std::uint32_t at=0;at<resident;at+=page.size()){
    for(unsigned i=0;i<page.size();++i){auto v=(at+i)*0x9e3779b9u;v^=v>>16;page[i]=std::byte(v>>24);}
    memory.commit(0x20000000u+at,page);
  }
  std::cout<<"Ready: actual 20 MiB nonuniform source data committed\n";
  SourceRuntimeHostPhysicalMemory physical(memory);
  constexpr std::uint32_t extent=0x30e4000u;
  const auto base=physical.allocate({extent,0x61000u,0x61000u+extent-1u,4096,4});
  check(base==0x80061000u,"Original physical first-fit token differs");
  const auto bytes=memory.view(base,extent);
  for(auto byte:bytes)check(byte==std::byte{},"Fresh physical allocation is not zero-filled");
  memory.write(base+extent-1,std::array{std::byte{0xa7}});
  check(memory.view(base+extent-1,1)[0]==std::byte{0xa7},"Physical last-byte access changed");
  std::cout<<"PASS original-size physical allocation with existing live source data\n";
}
void protection(){
  SourceRuntimeMemory memory;SourceRuntimeHostPhysicalMemory physical(memory);
  const auto base=physical.allocate({0x3000,0x61000,0x63fff,4096,4});
  const auto native=memory.native_address(base,0x3000);
  check(native.value%4096==0,"Source physical bytes are copied away from actual host pages");
  check(host_writable(native.value),"Fresh physical mapping is not writable");
  physical.protect(base+0xfff,2,2);
  check(!host_writable(native.value) && !host_writable(native.value+4096),
        "Physical protection misses actual source backing pages");
  bool rejected=false;try{memory.mutable_view(base+0x1000,1);}catch(const std::exception&){rejected=true;}
  check(rejected,"Read-only physical bytes accepted a mutable view");
  physical.protect(base,0x2000,4);
  check(host_writable(native.value) && host_writable(native.value+4096),"Physical unprotection differs");
  std::cout<<"PASS actual physical/native address and page protection identity\n";
}
}
int main(int argc,char **argv)try{
  std::cout<<std::unitbuf;
#if !defined(__linux__)
  return 77;
#endif
  check(argc==2,"Select large or protection physical backing case");
  if(std::string_view(argv[1])=="large")large();else{check(std::string_view(argv[1])=="protection","Unknown physical backing case");protection();}
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
