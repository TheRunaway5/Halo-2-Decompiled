#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
// Supplied Vista PE SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Literal leaves 5B1031..5B119E and controller query 60687F..606890.
// These are two separate caller-owned byte banks, not a platform identity,
// profile-storage implementation, or active native session.
inline constexpr std::size_t vista_local_user_count=4;
inline constexpr std::size_t vista_local_user_stride=0xb8;
inline constexpr std::size_t vista_local_user_bank_size=
    vista_local_user_count*vista_local_user_stride;
inline constexpr std::size_t vista_controller_user_stride=0x127c;
inline constexpr std::size_t vista_controller_user_bank_size=
    vista_local_user_count*vista_controller_user_stride;
using VistaLocalUserIdentity=std::array<std::byte,8>;
using VistaLocalUserPayload=std::array<std::byte,0x84>;

struct VistaLocalUserPayloadOutputs {
  std::uint32_t* controller_word{};
  VistaLocalUserPayload* payload{};
  std::uint32_t* first_trailer_word{};
  std::uint32_t* second_trailer_word{};
};

// The original scans increasing slots, clears all B8 bytes of the first
// inactive record, sets active=1, and writes the supplied exact eight bytes.
// A full original bank writes at slot -1 BEFORE returning FFFFFFFF. This
// bounded API rejects that unsafe context atomically with FormatError.
std::uint32_t vista_local_user_allocate(std::span<std::byte>,
    VistaLocalUserIdentity actual_identity);
// 5B1094 clears the complete selected record, including opaque bytes.
void vista_local_user_remove(std::span<std::byte>,std::uint32_t user_slot);

// Any nonzero active byte returns true. Inactive reads preserve every output.
// Payload outputs are individually nullable, as in 5B10E0. Only consumed AL
// is represented; incidental original EAX high bytes are not a boolean ABI.
// Outputs must be detached from the source bank and each other; overlapping
// output storage is outside the proved source boundary and rejects atomically
// when active. An inactive source guard does not consume output pointers.
bool vista_local_user_read_identity(std::span<const std::byte>,
    std::uint32_t user_slot,VistaLocalUserIdentity&);
bool vista_local_user_read_payload(std::span<const std::byte>,
    std::uint32_t user_slot,VistaLocalUserPayloadOutputs);

// Writes have no active-state guard. Inputs are explicit detached values;
// no guest pointers or arbitrary source pointer-alias behavior are modeled.
// Identity writes change only +1..+8. Payload writes change only controller
// +C, the complete 0x84-byte (132-byte) payload at +10, and DWORD trailers at +94/+98.
void vista_local_user_write_identity(std::span<std::byte>,
    std::uint32_t user_slot,VistaLocalUserIdentity actual_identity);
void vista_local_user_write_payload(std::span<std::byte>,
    std::uint32_t user_slot,std::uint32_t actual_controller_word,
    VistaLocalUserPayload actual_payload,std::uint32_t first_trailer_word,
    std::uint32_t second_trailer_word);

// Reads the controller's raw +4 mapping regardless of flags/user occupancy.
// FFFFFFFF is retained; other words are not coerced into valid user slots.
std::uint32_t vista_controller_local_user(std::span<const std::byte>,
    std::uint32_t controller_slot);
// All APIs reject truncated banks and indices outside [0,4) before writing.
// Native admission checks are explicit additions to original unchecked reads.
} // namespace halo2
