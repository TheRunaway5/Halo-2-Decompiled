#pragma once
#include <array>
#include <cstdint>
#include <type_traits>

namespace halo2::eos_worker_fixture {
inline constexpr char observer_environment[]="HALO2_EOS_ACCOUNT_FIXTURE_OBSERVER";
inline constexpr char worker_environment[]="HALO2_EOS_ACCOUNT_FIXTURE_WORKER";
inline constexpr std::uint64_t observer_magic=0x4832454f534f4253;
enum SdkCall : unsigned {device_call,login_call,create_call,socket_call,tick_call,sdk_call_count};
struct ObserverData {
    std::uint64_t magic{},parent_pid{},worker_pid{},worker_entries{},worker_main{};
    std::uint64_t parent_calls{},foreign_calls{},off_main_calls{},worker_finished{},worker_result{};
    std::uint64_t worker_image_verified{};
    std::array<std::uint64_t,sdk_call_count> calls{};
};
static_assert(std::is_trivially_copyable_v<ObserverData>);
}
