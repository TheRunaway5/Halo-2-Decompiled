#include "halo2/vista_loading_thread.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t vista_loading_thread_run(const VistaLoadingThreadServices& services) {
  if(!services.sleep || !services.mode || !services.present || !services.fade_active)
    throw std::invalid_argument("Vista loading thread requires its actual sleep, mode, presentation and fade owners");
  do {
    services.sleep(33);
    (void)services.present(services.mode());
  } while(!services.fade_active());
  auto pending=services.present(services.mode());
  while(pending) {
    services.sleep(33);
    pending=services.present(services.mode());
  }
  return 0;
}
} // namespace halo2
