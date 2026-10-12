#include "halo2/native_grenade.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
template<class T> T read(std::span<const std::byte> b,std::size_t offset){
  if(offset>b.size()||sizeof(T)>b.size()-offset)throw std::runtime_error("Truncated timer fixture");
  T result;std::memcpy(&result,b.data()+offset,sizeof result);return result;
}
template<class T> void write(std::span<std::byte> b,std::size_t offset,T value){
  if(offset>b.size()||sizeof(T)>b.size()-offset)throw std::runtime_error("Truncated timer fixture");
  std::memcpy(b.data()+offset,&value,sizeof value);
}
}
int main(int argc,char**argv){try{
  if(argc!=2)throw std::runtime_error("Supply original GRT1 timer-prefix fixtures");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream stream(argv[1],std::ios::binary|std::ios::ate);
  if(!stream)throw std::runtime_error("Cannot open timer fixture");
  const auto size=stream.tellg();if(size<8)throw std::runtime_error("Truncated timer fixture");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  stream.seekg(0);if(!stream.read(reinterpret_cast<char*>(bytes.data()),size))throw std::runtime_error("Cannot read timer fixture");
  if(std::memcmp(bytes.data(),"GRT1",4))throw std::runtime_error("Invalid timer fixture magic");
  constexpr std::size_t body_size=522,record_size=2+2*body_size;
  const auto count=read<std::uint32_t>(bytes,4);
  if(count!=540||bytes.size()!=8+std::size_t(count)*record_size)throw std::runtime_error("Unexpected timer fixture extent");
  for(std::uint32_t index=0;index<count;++index){
    const auto record=std::span<const std::byte>(bytes).subspan(8+index*record_size,record_size);
    const auto before=record.subspan(2,body_size),expected=record.subspan(2+body_size,body_size);
    NativeGrenadeProjectile p;
    p.flags=read<std::uint32_t>(before,0x12c);p.state=read<std::int16_t>(before,0x130);
    p.timer=read<float>(before,0x158);p.timer_step=read<float>(before,0x15c);
    p.arming=read<float>(before,0x160);p.arming_step=read<float>(before,0x164);
    p.age=read<std::int16_t>(before,0x1aa);
    // The original fixture records the literal one word3F800000 from both
    // authenticated images. Other body bytes remain the captured source input.
    advance_native_grenade_timer(p,read<std::int16_t>(record,0),std::bit_cast<float>(0x3f800000u));
    std::vector<std::byte> actual(before.begin(),before.end());
    write(actual,0x12c,p.flags);write(actual,0x130,p.state);
    write(actual,0x158,p.timer);write(actual,0x160,p.arming);write(actual,0x1aa,p.age);
    if(!std::equal(actual.begin(),actual.end(),expected.begin()))
      throw std::runtime_error("Original complete projectile mismatch at case"+std::to_string(index));
  }
  std::cout<<"PASS "<<count<<" paired1.1/u5 original timer prefixes, "<<count*body_size<<" complete output bytes\n";
  std::cout<<"LIMIT declared scalar kernel controls; no world/contact/detonation services or Xbox FP-status claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
