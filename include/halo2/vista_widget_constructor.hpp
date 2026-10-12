#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
// Complete lower births from the supplied PC executable. Source identities
// belong to the caller's registry, never to a truncated host pointer. The
// borrowed bank stays live through the cached-clock callback, which may mutate
// it. Only the source-owned fields are written; padding/buffers are preserved.
VistaWidgetIdentity vista_widget_construct(std::span<std::byte>, VistaWidgetIdentity,
    std::uint32_t kind, std::uint32_t controller,
    const std::function<std::uint32_t()>& read_cached_ui_clock); //611D81, >=70 bytes.
VistaWidgetIdentity vista_widget_text_base_construct(std::span<std::byte>, VistaWidgetIdentity,
    std::uint32_t controller,
    const std::function<std::uint32_t()>& read_cached_ui_clock); //61BA07, >=75 bytes.
std::uint32_t vista_widget_string_construct(std::span<std::byte>, std::uint32_t source_identity); //62DEFA, >=44 bytes.
VistaWidgetIdentity vista_widget_text_construct(std::span<std::byte>, VistaWidgetIdentity,
    std::uint32_t controller,
    const std::function<std::uint32_t()>& read_cached_ui_clock); //60F65D, >=BE bytes.
VistaWidgetIdentity vista_widget_button_construct(std::span<std::byte>, VistaWidgetIdentity,
    std::uint32_t ordinal, std::uint32_t controller,
    const std::function<std::uint32_t()>& read_cached_ui_clock); //63F756, >=100 bytes.
} //namespace halo2
