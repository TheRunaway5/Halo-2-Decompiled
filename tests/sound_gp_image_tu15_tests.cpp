#include "halo2/sound_gp_image_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

namespace {
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  if(!file||file.tellg()<0)throw std::runtime_error("Missing original GP image fixture: "+path.string());
  std::vector<std::byte> bytes(std::size_t(file.tellg()));file.seekg(0);
  if(!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))throw std::runtime_error("Truncated original GP image fixture");
  return bytes;
}
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
void compare(std::span<const std::uint32_t> words,std::span<const std::byte> bytes,const char *where) {
  check(words.size()*4==bytes.size(),"GP image comparison extent differs");
  for(std::size_t i=0;i<bytes.size();++i)if(std::uint8_t(words[i/4]>>(8*(i%4)))!=std::to_integer<std::uint8_t>(bytes[i]))
    throw std::runtime_error(std::string(where)+" differs at byte "+std::to_string(i));
}
}
int main(int argc,char **argv)try {
  if(argc!=3)throw std::runtime_error("Supply owned TU1.5 executable and original GP image fixture directory");
  const std::filesystem::path fixture=argv[2];
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(fixture))return 77;
  const auto image=halo2::load_sound_gp_image_tu15(argv[1]);
  const auto decoded=read(fixture/"decoded-resource.bin"),uploaded=read(fixture/"gp-upload.bin");
  check(image.decoded_resource==decoded,"Native decoded resource differs from unchanged original CPU execution");
  const auto control_bytes=image.control_program.size()*4;
  check(control_bytes<=uploaded.size(),"Captured GP control extent is incomplete");
  compare(image.control_program,std::span(uploaded).first(control_bytes),"Original uploaded GP control program");
  compare(image.effect_program,std::span(uploaded).subspan(control_bytes),"Original uploaded GP effect program");
  compare(image.initial_data,std::span(decoded).subspan(0x3f98,0x2e68),"Original GP initial data");
  check(image.effects.size()==15&&image.program_base_word==0x171&&image.data_base_word==0x80,"Original GP program/data geometry differs");
  check(image.user_scratch_base==0xc000&&image.user_scratch_bytes==0xbd000&&image.minimum_scratch_bytes==0xc80c0,"Original GP scratch allocation geometry differs");
  check(image.header==std::array<std::uint32_t,6>{0,0xde0,0x3f98,0xb9a,3,0},"Original GP immutable header differs");
  for(const auto &effect:image.effects) {
    check(effect.program_word>=image.program_base_word,"Invalid GP effect entry");
    const auto entry=effect.program_word-image.program_base_word;
    check(entry+1<image.effect_program.size()&&image.effect_program[entry]==0x65f400&&image.effect_program[entry+1]==effect.state_word,"GP effect entry does not bind its original state");
  }
  // Passing a different existing file must fail authentication. No temporary
  // executable or change to the owned original is needed for this control.
  bool rejected=false;
  try{(void)halo2::load_sound_gp_image_tu15(fixture/"decoded-resource.bin");}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"Unauthenticated GP image accepted");
  std::cout<<"PASS original GP image: "<<decoded.size()<<" decoded resource bytes, "<<uploaded.size()<<" uploaded program bytes, 15 effect/state bindings; wrong image rejected\n";
  std::cout<<"LIMIT immutable image preparation; device allocation, relocation, upload handshake, DSP execution and audible parity remain outside this test\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
