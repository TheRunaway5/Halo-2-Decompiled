#include "halo2/native_tu15_sound_controller_arithmetic.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_controller_arithmetic;
int main(){try{
    std::uint32_t value;
    auto word=[&]{if(!(std::cin>>value))throw std::runtime_error("Missing native numeric input");return Word24(value);};
    const auto status=word();std::array<AddressingMode,3> modes;
    for(auto& mode:modes){if(!(std::cin>>value))throw std::runtime_error("Missing addressing input");mode=static_cast<AddressingMode>(value);}
    Parameters parameters{word(),word()};State state{word(),word()};std::array<Word24,32> storage;
    for(auto& w:storage)w=word();
    std::string extra;if(std::cin>>extra)throw std::runtime_error("Extra numeric input");
    const auto result=process({status,modes[0],modes[1],modes[2]},parameters,state,storage);
    nlohmann::json out={{"threshold",parameters.threshold.bits},{"gain",parameters.gain.bits},{"flags",state.flags.bits},{"counter",state.counter.bits},{"produced_count",result.produced_count},{"storage_words",nlohmann::json::array()},{"emissions",nlohmann::json::array()},{"native_addresses",{reinterpret_cast<std::uintptr_t>(&parameters),reinterpret_cast<std::uintptr_t>(&state),reinterpret_cast<std::uintptr_t>(storage.data())}}};
    for(auto w:storage)out["storage_words"].push_back(w.bits);
    for(unsigned i=0;i<result.produced_count;i++){const auto& e=result.emissions[i];out["emissions"].push_back({{"input_index",e.input_index},{"fractional_product",e.fractional_product},{"limited_product",e.limited_product.bits},{"addition_results",e.addition_results},{"output",e.output.bits}});}
    std::cout<<out.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
