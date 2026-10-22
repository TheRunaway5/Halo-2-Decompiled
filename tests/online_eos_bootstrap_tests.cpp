#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <iostream>
int main() {
  try {
    const auto options=halo2::online_protocol_bridge::eos_runtime_options();
    std::cout<<"EOS_BOOTSTRAP "<<options.transport<<' '<<options.profiles<<'\n';
    return 0;
  } catch(const std::exception &) { return 1; }
}
