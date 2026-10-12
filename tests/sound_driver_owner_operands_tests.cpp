#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text){
  check(text.size()%2==0,"Incomplete original operand bytes");std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(std::stoul(text.substr(2*i,2),nullptr,16));
  return result;
}
}
int main(int argc,char **argv){
  try {
    check(argc==5,"Supply owned operand fixture and base/1.1/u5 executables");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const auto base=halo2::OriginalData::load_xbe(argv[2]);
    check(!base->sound_driver_listener_constants&&!base->sound_submit_operands,"Unverified base sound operands were supplied");
    const std::array<std::shared_ptr<const halo2::OriginalData>,2> images{
        halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);std::size_t count=0;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;unsigned index;std::string orientation,environment,defaults,callback,table;
      check(bool(row>>kind>>index>>orientation>>environment>>defaults>>callback>>table)&&kind=='C'&&index==count+1&&index<=2,"Incomplete original sound owner operand row");
      const auto &listener=images[index-1]->sound_driver_listener_constants;
      const auto &submit=images[index-1]->sound_submit_operands;
      check(bool(listener)&&bool(submit),"Verified original sound owner operands unavailable");
      const auto default_bytes=bytes(defaults),callback_bytes=bytes(table);
      check(listener->orientation_epsilon_bits==std::stoul(orientation,nullptr,16)&&
            listener->environment_epsilon_bits==std::stoul(environment,nullptr,16)&&
            default_bytes.size()==72&&std::equal(default_bytes.begin(),default_bytes.end(),listener->default_environment.begin())&&
            submit->constants.effect_callbacks==std::stoul(callback,nullptr,16)&&
            callback_bytes.size()==32&&std::equal(callback_bytes.begin(),callback_bytes.end(),submit->callback_table.begin()),
            "Imported original listener or complete callback-table bytes differ");
      ++count;
    }
    check(count==2,"Incomplete source image coverage");
    std::cout<<"supportedImages=2 immutableSoundOwnerBytes=232 baseUnknown=absent mutableOwnersAndCallbackExecution=separate\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
