#include "halo2/native_tu15_sound_ep_mono_numeric.hpp"
#include <cstring>
#include <iostream>
using namespace halo2::tu15_ep_mono_numeric;
int main()try{
 Inputs inputs{};Parameters parameters{Word24(1),Word24(0x7fffff)};Controls controls{Word24(0),{}};controls.address_modes.fill(AddressingMode::linear);
 unsigned guards=0;
 const auto reject=[&]{const auto saved_inputs=inputs;const auto saved_parameters=parameters;const auto saved_controls=controls;bool caught=false;
  try{(void)process(controls,parameters,inputs);}catch(const std::invalid_argument&){caught=true;}
  if(!caught||std::memcmp(&inputs,&saved_inputs,sizeof(inputs))||std::memcmp(&parameters,&saved_parameters,sizeof(parameters))||std::memcmp(&controls,&saved_controls,sizeof(controls)))throw std::runtime_error("Guard accepted or mutated borrowed data");
  ++guards;
 };
 for(unsigned bit=8;bit<32;bit++){controls.entry_status.bits=1u<<bit;reject();}controls.entry_status=Word24(0);
 for(auto&mode:controls.address_modes){mode=AddressingMode::modulo;reject();mode=AddressingMode::reverse_carry;reject();mode=AddressingMode::linear;}
 for(auto*word:{&parameters.scale_enabled,&parameters.scale}){const auto saved=*word;word->bits=0x1000000;reject();*word=saved;}
 for(auto&channel:inputs)for(auto&sample:channel){sample.bits=0x1000000;reject();sample=Word24(0);}
 auto value=process(controls,parameters,inputs);for(const auto&channel:value.output)for(auto word:channel)if(word.bits)throw std::runtime_error("Silence output differs");
 unsigned packing_guards=0;
 for(auto&channel:value.output)for(auto&word:channel){word.bits=0x1000000;const auto saved=value;bool caught=false;try{(void)pack_interleaved_s16(value);}catch(const std::invalid_argument&){caught=true;}if(!caught||std::memcmp(&value,&saved,sizeof(value)))throw std::runtime_error("Packing guard changed input or admitted invalid word");word=Word24(0);++packing_guards;}
 for(auto byte:pack_interleaved_s16(value))if(byte!=std::byte{0})throw std::runtime_error("Packed silence differs");
 std::cout<<"{\"passed\":true,\"before_mutation_guards\":"<<guards<<",\"packing_guards\":"<<packing_guards<<",\"borrowed_inputs_and_parameters_unchanged\":true,\"initializer45_ready\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
