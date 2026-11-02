#include "halo2/native_tu15_sound_ep_input.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2::tu15_ep_input {
InputWindow copy_input_window(const tu15_gp_output::LogicalCopyReceipt& resource,std::uint32_t half){
    if(half!=0 && half!=256)throw std::invalid_argument("EP input requires the actual half0 or256 selection");
    InputWindow result;result.half_=half;
    const auto values=resource.retained_values();
    for(unsigned channel=0;channel<5;++channel)
        std::copy_n(values.begin()+channel*512+half,256,result.values_[channel].begin());
    return result;
}
}
