#include "halo2/unit_locomotion.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset) {
  std::uint32_t value=0;
  for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::to_integer<unsigned>(bytes[offset+n]))<<(n*8);
  return value;
}
bool unordered(std::uint32_t absolute) { return absolute>0x7f800000u; }
void store(std::span<std::byte,0x70> output,SourceLocomotionUpdate &update,
    std::uint32_t instruction,std::uint16_t offset,std::uint8_t width,std::uint32_t value) {
  update.stores[update.store_count++]={instruction,offset,width,value};
  for(unsigned n=0;n<width;++n)output[offset+n]=std::byte((value>>(n*8))&255u);
}
}

SourceLocomotionUpdate classify_source_locomotion_request(
    std::span<const std::byte,0x154> request,std::span<std::byte,0x70> classification) {
  SourceLocomotionUpdate update;
  const auto x=word(request,0xdc),y=word(request,0xe0),z=word(request,0xe4);
  const auto ax=x&0x7fffffffu,ay=y&0x7fffffffu,az=z&0x7fffffffu;
  // UCOMISS/LAHF/TEST parity admits unordered and nonzero operands. Both
  // signed zeros take the no-write return; there is no movement epsilon.
  if(ax==0 && ay==0 && az==0)return update;
  // FLD/FABS/FCOMPI compares |y| against |x|. Binary32 magnitudes have the
  // same exact ordering as their nonnegative bit patterns, including infinity.
  // Unordered and ties take JBE's x branch. Masked exception side effects
  // are outside this stored-output API; no host FP operation changes NaNs.
  if(!unordered(ax) && !unordered(ay) && ay>ax) {
    const bool negative=(y&0x80000000u)!=0 && ay!=0;
    store(classification,update,negative?0x1f5349:0x1f5351,4,4,negative?2:3);
  }else {
    // COMISS 0,x/JBE treats equality and unordered as the positive branch.
    const bool negative=(x&0x80000000u)!=0 && ax!=0 && !unordered(ax);
    store(classification,update,negative?0x1f5362:0x1f536a,4,4,negative?5:4);
  }
  return update;
}

SourceLocomotionUpdate dispatch_source_locomotion(
    std::span<const std::byte,0x154> request,std::span<const std::byte> physical_state,
    std::span<std::byte,0x70> classification) {
  if((std::to_integer<unsigned>(request[0x14])&0x20u)!=0) {
    SourceLocomotionUpdate update;update.path=SourceLocomotionPath::forced_class_one;
    store(classification,update,0x1e94eb,4,4,1);
    store(classification,update,0x1e94ee,8,1,1);
    return update;
  }
  if(physical_state.empty())throw std::invalid_argument("Missing original physical-state byte");
  const auto state=std::to_integer<unsigned>(physical_state[0]);
  if(state==1)return classify_source_locomotion_request(request,classification);
  SourceLocomotionUpdate update;update.path=SourceLocomotionPath::unavailable_physical_state;
  // Literal source table 1E9520. These routes have not been executed by this
  // measured state1 suite; exposing the stop is not a native source return.
  constexpr std::uint32_t targets[]={0x1e94fd,0x1e9506,0x1e94f1,0x1e9514,0x1e9514,0x1e950d};
  if(state>=1 && state<=6)update.unavailable_dispatch_target=targets[state-1];
  return update;
}
} // namespace halo2
