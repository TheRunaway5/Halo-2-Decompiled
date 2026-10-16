#include "halo2/native_tu15_sound_ep_module0_numeric.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2::tu15_ep_module0_numeric;
namespace {void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}}
int main()try{
    Inputs inputs{};State state{};Controls controls{Word24(0),{}};controls.address_modes.fill(AddressingMode::linear);Parameters parameters{Word24(1),Word24(31)};std::size_t guards=0,packing=0;
    const auto reject=[&]{const auto old_inputs=inputs;const auto old_state=state;const auto old_controls=controls;const auto old_parameters=parameters;bool did=false;try{(void)process(controls,parameters,inputs,state);}catch(const std::invalid_argument&){did=true;}check(did,"Unproved module0 input accepted");check(!std::memcmp(&old_inputs,&inputs,sizeof(inputs))&&!std::memcmp(&old_state,&state,sizeof(state))&&!std::memcmp(&old_controls,&controls,sizeof(controls))&&!std::memcmp(&old_parameters,&parameters,sizeof(parameters)),"Rejected module0 call mutated borrowed values");++guards;};
    for(auto&channel:inputs)for(auto&sample:channel){sample.bits=0x1000000;reject();sample.bits=0;}
    std::vector<Word24*>history;
    const auto append=[&](auto&sections){for(auto&section:sections){for(auto&sample:section.samples)history.push_back(&sample);history.push_back(&section.residual_high);history.push_back(&section.residual_low);}};append(state.left);append(state.right);append(state.surround);check(history.size()==78,"Typed history extent differs");
    for(auto*word:history){word->bits=0x1000000;reject();word->bits=0;}
    for(auto*word:{&controls.entry_status,&parameters.initialization_requested,&parameters.gain_level}){const auto saved=*word;word->bits=0x1000000;reject();*word=saved;}
    for(unsigned bit=8;bit<24;bit++){controls.entry_status=Word24(1u<<bit);reject();}controls.entry_status=Word24(0);
    for(auto&mode:controls.address_modes){for(auto bad:{AddressingMode::modulo,AddressingMode::reverse_carry,static_cast<AddressingMode>(42)}){mode=bad;reject();}mode=AddressingMode::linear;}
    const auto result=process(controls,parameters,inputs,state);check(result.scale_coefficient.bits==0x607000,"Source maximum gain selection changed");for(const auto&channel:result.output)for(auto sample:channel)check(sample.bits==0,"Zero nominal call is nonzero");
    parameters.initialization_requested=Word24(0);(void)process(controls,parameters,inputs,state);
    auto corrupted=result;for(auto&channel:corrupted.output)for(auto&sample:channel){sample.bits=0x1000000;const auto old=corrupted;bool did=false;try{(void)pack_interleaved_s16(corrupted);}catch(const std::invalid_argument&){did=true;}check(did&&!std::memcmp(&old,&corrupted,sizeof(corrupted)),"Rejected packing mutated result");sample.bits=0;++packing;}
    check(guards==1401&&packing==512,"Guard count changed");
    std::cout<<"{\"passed\":true,\"before_mutation_guards\":"<<guards<<",\"packing_guards\":"<<packing<<",\"typed_history_words\":78,\"native_owner_addresses_above4GB\":"<<((reinterpret_cast<std::uintptr_t>(&state)>0xffffffffu&&reinterpret_cast<std::uintptr_t>(&inputs)>0xffffffffu)?"true":"false")<<",\"Ready\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
