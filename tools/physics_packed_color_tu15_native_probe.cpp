// Diagnostic original-input probe for the complete TU1.5 linear packed-color branch.
// Link the current halo2_physics_world_runtime; production does not load this fixture.
#include "halo2/physics_live_support_tu15.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <bit>
struct Memory:halo2::SourcePhysicsContactMemory {
 std::vector<std::byte> original=std::vector<std::byte>(0x800000),scratch=std::vector<std::byte>(0x110000);
 std::span<std::byte> span(std::uint32_t p,std::size_t n) {
  if(p>=0x60000000u && std::uint64_t(p)-0x60000000u+n<=scratch.size())return std::span(scratch).subspan(p-0x60000000u,n);
  if(p>=0x10000u && std::uint64_t(p)-0x10000u+n<=original.size())return std::span(original).subspan(p-0x10000u,n);
  throw std::runtime_error("Packed color memory owner unavailable");
 }
 void read(std::uint32_t p,std::span<std::byte> b)const override {const auto s=const_cast<Memory*>(this)->span(p,b.size());std::copy(s.begin(),s.end(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte> b)override {const auto s=span(p,b.size());std::copy(b.begin(),b.end(),s.begin());}
 void put(std::uint32_t p,std::uint32_t v){write(p,std::as_bytes(std::span(&v,1)));}
};
int main(int argc,char**argv)try {
 if(argc!=2)throw std::runtime_error("Supply authenticated original section bytes");
 Memory m;std::ifstream file(argv[1],std::ios::binary);if(!file)throw std::runtime_error("Original section input is unavailable");std::uint32_t p,n;
 while(file.read(reinterpret_cast<char*>(&p),4)) {file.read(reinterpret_cast<char*>(&n),4);std::vector<std::byte>b(n);file.read(reinterpret_cast<char*>(b.data()),n);m.write(p,b);}
 std::uint32_t a,b,f,round;
 while(std::cin>>std::hex>>a>>b>>f>>round) {
  m.put(0x60020000,1);m.put(0x60020004,0x60020100);m.put(0x60020100,0x2002);m.put(0x60020104,a);m.put(0x60020110,b);
  m.put(0x600f0000,0x60300000);m.put(0x600f0004,f);
  halo2::SourcePhysicsWorldRuntimeAbi abi{};abi.gp[1]=0x60020000;abi.gp[4]=0x600f0000;abi.eflags=2;
  halo2::SourcePhysicsFactoryFloatingPoint fp{std::uint16_t(0x27fu|(round<<10)),0x1f80u|(round<<13)};abi.floating_point=fp;
  halo2::source_run_physics_live_support_tu15(m,0x13e6e0,abi,fp,{});
  for(auto v:abi.gp)std::cout<<std::hex<<v<<' ';
  for(const auto &x:abi.xmm)for(auto v:x)std::cout<<std::hex<<v<<' ';
  std::cout<<std::hex<<abi.eflags<<' '<<unsigned(abi.x87_depth)<<'\n';
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
