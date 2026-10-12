#include "halo2/sound_sdk_spatial.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace {
using Json=nlohmann::json;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::uint32_t> words(const std::string &raw){
  require(raw.size()%8==0,"Incomplete original SDK word array");std::vector<std::uint32_t> result(raw.size()/8);
  for(std::size_t i=0;i<result.size();++i)for(unsigned j=0;j<4;++j)result[i]|=word(raw.substr(8*i+2*j,2))<<(8*j);
  return result;
}
std::array<std::uint32_t,3> vector(const Json &value){const auto v=words(value.get<std::string>());require(v.size()==3,"Original SDK vector extent");return {v[0],v[1],v[2]};}
}
int main(int argc,char **argv)try{
  require(argc==2,"Usage: sound_sdk_spatial_tests <original.json>");std::ifstream file(argv[1]);require(bool(file),"Original SDK fixture unavailable");Json fixture;file>>fixture;
  require(fixture.at("schema")=="ORIGINAL_SOUND_SDK_NUMERICAL_V1","Original SDK fixture schema");const auto &literals=fixture.at("original_literal_bytes");
  const auto constant=[&](const char *address){const auto v=words(literals.at(address).get<std::string>());require(v.size()==1,"Original SDK literal extent");return v[0];};
  const halo2::SoundSdkSpatialConstants c{constant("0045dbd8"),constant("0045dbc0"),constant("0045dbbc"),constant("00417238"),constant("004171f8"),constant("004171e8"),constant("004171ec"),constant("0045dfcc"),constant("0045e320"),constant("0045dccc"),vector(literals.at("00385a48")),vector(literals.at("00385804")),0x385a58,0x385a78};
  std::size_t actual=0,declared=0,position=0,angles=0,volume=0,kernel=0;
  for(const auto &q:fixture.at("cases")){
    const auto label=q.at("label").get<std::string>(),owner=q.at("owner").get<std::string>();
    const auto &b=q.at("backing_hex");const auto &expected=q.at("output_hex");std::vector<std::uint32_t> a;
    for(const auto &v:q.at("scalar_and_pointer_words"))a.push_back(word(v.get<std::string>()));
    const halo2::SoundDriverFloatingPointContext fp{q.at("x87_control_word").get<std::uint16_t>(),q.at("mxcsr").get<std::uint32_t>()};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned short before_cw;asm volatile("fnstcw %0":"=m"(before_cw));const auto before_mxcsr=_mm_getcsr();
#endif
    bool equal=false;
    if(owner=="position"){
      require(a.size()==5,"Original SDK position ABI");const auto got=halo2::source_sound_sdk_position(a[0],vector(b.at("1")),vector(b.at("2")),c,fp);
      equal=got.direction==vector(expected.at("3"))&&got.distance_bits==words(expected.at("4").get<std::string>())[0];++position;
    }else if(owner=="angles"){
      require(a.size()==8,"Original SDK angle ABI");const auto got=halo2::source_sound_sdk_angles(a[0],vector(b.at("1")),vector(b.at("3")),vector(b.at("4")),a[5],c,fp);
      equal=got.azimuth_bits==words(expected.at("6").get<std::string>())[0]&&got.elevation_bits==words(expected.at("7").get<std::string>())[0];++angles;
    }else if(owner=="front_rear"){
      require(a.size()==7,"Original SDK front/rear ABI");const auto got=halo2::source_sound_sdk_front_rear(a[0],a[1],a[2],a[4],c,fp);
      equal=std::bit_cast<std::uint32_t>(got.front)==words(expected.at("5").get<std::string>())[0]&&std::bit_cast<std::uint32_t>(got.rear)==words(expected.at("6").get<std::string>())[0];++volume;
    }else if(owner=="kernel"){
      require(a.size()==4,"Original SDK kernel ABI");const auto got=halo2::source_sound_sdk_kernel(a[0],a[2],c,fp);
      const auto wanted=words(expected.at("3").get<std::string>());
      require(wanted.size()==2,"Original SDK kernel output extent");equal=got[0]==wanted[0]&&got[1]==wanted[1];++kernel;
    }else throw std::runtime_error("Unknown original SDK owner");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned short after_cw;asm volatile("fnstcw %0":"=m"(after_cw));
    require(before_cw==after_cw&&before_mxcsr==_mm_getcsr(),"SDK owner leaked its floating-point context");
#endif
    if(!equal){std::cerr<<label<<'\n';throw std::runtime_error("Original SDK spatial numerical output differs");}
    q.at("actual_initialized_call").get<bool>()?++actual:++declared;
  }
  require(actual&&declared&&position&&angles&&volume&&kernel,"Incomplete original SDK numerical evidence");
  std::cout<<"PASS "<<actual<<" actual SDK captures and "<<declared<<" declared original CPU cases ("<<position<<" position, "<<angles<<" angles, "<<volume<<" front/rear, "<<kernel<<" kernel). Numerical outputs only; no native playback/DSP claim.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
