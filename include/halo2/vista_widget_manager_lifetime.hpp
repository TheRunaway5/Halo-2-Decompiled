#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
enum class VistaWidgetManagerBirth {
  base637BA8, extended637EF6, derived639051, derived63907E,
  derived639090, global6390A2
};
struct VistaWidgetManagerLifetimeServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<void(VistaWidgetIdentity)> dispose; // Actual60B9AC, not free.
  std::function<void(VistaWidgetIdentity)> extended_close; // Actual637FE0.
};
// Complete original constructors, including original60B0BC and padding.
// Source32 storage is supplied by the native lifetime owner. Constructors do
// not allocate, request renderer resources or infer manager initialization.
VistaWidgetIdentity vista_widget_manager_construct(VistaWidgetIdentity,
    VistaWidgetManagerBirth,const VistaWidgetManagerLifetimeServices&);
// Complete6092E3 and all reached constructors. The root needs3FF0 bytes;
// only source-written fields change. Its original global instance is D718E0.
VistaWidgetIdentity vista_widget_ui_construct(VistaWidgetIdentity,
    const VistaWidgetManagerLifetimeServices&);
// Complete637BD7/637F34/6390E2 virtual0 reset owners. Their preserved fields
// differ from construction; no guessed whole-bank zeroing is performed.
enum class VistaWidgetManagerReset {base637BD7,extended637F34,global6390E2};
void vista_widget_manager_reset(VistaWidgetIdentity,VistaWidgetManagerReset,
    const VistaWidgetManagerLifetimeServices&);
// Complete637628/638211/63910E virtual0C shutdown and63785B retirement.
// Disposal can mutate later slots. Every slot is reread in original order;
// failure/reentry retains all earlier writes.637FE0 remains a genuine service.
enum class VistaWidgetManagerShutdown {base637628,extended638211,global63910E};
void vista_widget_manager_shutdown(VistaWidgetIdentity,VistaWidgetManagerShutdown,
    const VistaWidgetManagerLifetimeServices&);
void vista_widget_manager_retire(VistaWidgetIdentity,
    const VistaWidgetManagerLifetimeServices&);
} //namespace halo2
