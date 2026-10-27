#pragma once
#include "halo2/native_tu15_physics_frame_observation.hpp"
#include "halo2/physics_internal_phantom_observation.hpp"
#include "halo2/source_gameplay_fade_phase_tu15.hpp"
#include "halo2/source_memory_read_alias_observation_tu15.hpp"
#include "halo2/source_object_attachment_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_player_status_phase_tu15.hpp"
#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/source_sound_timing_input_observation_tu15.hpp"
#include "halo2/source_vehicle_input_tu15.hpp"
#include <atomic>
#include <tuple>
#include <type_traits>

namespace halo2::tests::tls {
// Also accepts the original variable form, so this same two-TU test first
// reproduces MinGW's duplicate TLS initializer rather than a compile error.
template<class T>decltype(auto) registry(T&& value) {
    if constexpr(std::is_function_v<std::remove_reference_t<T>>)return value();
    else return (value);
}
inline auto registries() {
    return std::tie(registry(native_tu15_physics_frame_observers),
        registry(source_internal_phantom_observers),registry(source_gameplay_fade_handlers_tu15),
        registry(source_memory_read_alias_observers_tu15),registry(source_object_attachment_handlers_tu15),
        registry(source_physics_entry_observers),registry(source_player_status_phase_handlers_tu15),
        registry(source_player_scoreboard_gate_handlers_tu15),registry(source_script_sound_detail::admissions),
        registry(source_sound_timing_input_observers_tu15),registry(source_vehicle_input_reset_handlers_tu15));
}
inline auto registry_addresses() {
    return std::apply([](auto&... map){return std::array<const void*,sizeof...(map)>{&map...};},registries());
}
inline bool registries_empty() {
    return std::apply([](const auto&... map){return (map.empty()&&...);},registries());
}
extern std::atomic<unsigned> constructed,destroyed;
struct LifetimeMarker {
    int value;
    explicit LifetimeMarker(int value):value(value){++constructed;}
    ~LifetimeMarker(){++destroyed;}
};
struct Memory final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override {}
    void write(std::uint32_t,std::span<const std::byte>)override {}
};
std::array<const void*,11> producer_addresses();
int producer_marker();
std::unique_ptr<SourceObjectAttachmentBindingTu15> producer_bind(
    const SourcePhysicsContactMemory&,SourceObjectAttachmentHandlerTu15);
void producer_emit(const SourcePhysicsContactMemory&);
const source_script_sound_detail::Admission* producer_admission();
}
