#include "halo2/original_data.hpp"
#include "halo2/object_type_construction.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &value){return std::stoul(value,nullptr,16);}
std::string string_bytes(const std::string &value){
  require(!value.empty()&&value.size()%2==0,"Invalid literal string bytes");std::string result;
  for(std::size_t i=0;i<value.size();i+=2)result.push_back(char(std::stoul(value.substr(i,2),nullptr,16)));
  require(result.back()=='\0'&&result.find('\0')==result.size()-1,"Original string terminator differs");
  return result;
}
}
int main(int argc,char **argv){
  try{
    require(argc==5,"Supply independent fixture and base/1.1/u5 owned images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const auto base=halo2::OriginalData::load_xbe(argv[2]);
    require(!base->sound_looping_constants&&!base->object_construction_operands,"Unverified base operands imported");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    const halo2::SoundLoopingConstants *fields=nullptr;
    std::ifstream input(argv[1]);unsigned images_compared=0,strings=0,within=0;std::size_t bytes=0;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string extra;require(bool(row>>kind),"Empty immutable operand row");
      if(kind=='I'){
        unsigned image;std::string one,minimum,method,sentinel;
        require(bool(row>>image>>one>>minimum>>method>>sentinel)&&!fields&&image==++images_compared&&image<=2,"Incorrect operand image order");
        const auto &data=*images[image-1];require(data.sound_looping_constants&&data.object_construction_operands&&data.sound_create_constants,"Authenticated original operands absent");
        fields=&*data.sound_looping_constants;const auto &construction=*data.object_construction_operands;
        require(fields->one_bits==word(one)&&fields->minimum_db_bits==word(minimum)&&fields->game_allocator_method==word(method),"Looping arithmetic/method operand differs");
        require(construction.one_bits==word(one)&&construction.sentinel_word==word(sentinel),"Generic construction operand differs");
        require(fields->one_bits==data.sound_create_constants->one_bits&&fields->minimum_db_bits==data.sound_create_constants->minimum_gain_bits,"Shared original arithmetic differs");
        within=0;
      }else if(kind=='S'){
        unsigned index;std::string hex;require(bool(row>>index>>hex)&&fields&&index==within&&index<57,"Incorrect original string order");
        const auto raw=string_bytes(hex);const auto expected=raw.substr(0,raw.size()-1);
        const auto &actual=index<54?fields->class_names[index]:index==54?fields->pool_name:index==55?fields->empty_selector:fields->dialogue_selector;
        require(actual==expected,"Imported original literal string differs");++within;++strings;bytes+=raw.size();
      }else if(kind=='E'){
        require(fields&&within==57,"Incomplete original string fields");fields=nullptr;
      }else throw std::runtime_error("Unknown immutable operand row");
      require(!(row>>extra),"Extra immutable operand field");
    }
    require(!fields&&images_compared==2&&strings==114&&bytes==1244,"Incomplete original immutable operand coverage");
    std::cout<<"originalLoopingStrings=114 originalStringBytes=1244 extraConstructorBytes=4 baseUnknown=absent sharedOperandsAndHeaderCoexistence=pass\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
