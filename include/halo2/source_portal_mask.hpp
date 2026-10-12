#pragma once
#include "halo2/cache.hpp"
#include <span>

namespace halo2 {
struct SourcePortalOwnerIdentity {
    std::uint32_t unique_id=0xffffffffu;
    std::int16_t origin_bsp=-1;
    std::int8_t type=-1,source=-1;
};
struct SourcePortalAssociation {
    SourcePortalOwnerIdentity owner;
    std::int16_t first=-1,count{};
};
struct SourcePortalMaskDefinition {
    std::vector<std::uint32_t> portal_flags;
    std::vector<SourcePortalAssociation> associations;
    std::vector<std::int16_t> portal_indices;
};
struct SourceMachinePortalDefinition {
    std::uint16_t type{}; // mach+11C, door0/platform1/gear2.
    float closed_acoustic{},open_acoustic{}; // mach+124/+128.
};
struct SourcePortalOwnerState {
    std::uint32_t handle=0xffffffffu; // Full object generation; low16 is original pool slot.
    std::uint8_t header_type=7; // BAEB0 uses the header type, not body+AA.
    SourcePortalOwnerIdentity identity;
    std::int16_t first=-1,count{}; // Actual body+1DA/+1DC; constructor115190 setsNONE/0.
    SourceMachinePortalDefinition definition;
    float current_position{}; // Actual body+140 at the mask slice, after earlier tick logic.
    std::uint32_t position_group=0xffffffffu; // Actual body+13C, NONE has special behavior.
    float group_position{}; // Actual selected group record+4, separate from current body position.
    std::uint32_t device_flags{}; // Actual runtime body+12C; bit8 chooses group-value admission.
};
// Native group services retain presence/value directly. They need no invented
// Xbox heap datum; authored groupNONE still creates a present private group.
struct SourcePortalDoorInput {
    bool group_present{};
    float current_position{},group_position{};
    std::uint32_t device_flags{};
};
//1076E0 after the zero-initialized106510 constructor; this is initial device
//state, separate from source_machine_object_flags and later106790 bit8.
std::uint32_t source_machine_initial_device_flags(std::uint32_t placement_flags);
// Persistent source state. Initialization210DF0 changes words and ownership,
// retaining the separate acoustic bank. 188870/188980 initialize that bank.
struct SourcePortalMaskState {
    std::array<std::uint32_t,16> words{};
    std::array<std::uint16_t,128> acoustic{};
    void initialize(const SourcePortalMaskDefinition &,std::span<SourcePortalOwnerState>);
    void bind(const SourcePortalMaskDefinition &,std::span<SourcePortalOwnerState>);
    void reset_owners(std::span<SourcePortalOwnerState>); //210E80, no mask changes.
    void clear_flag8(const SourcePortalMaskDefinition &); //210E20, explicit caller order.
    void step(const SourcePortalMaskDefinition &,const SourcePortalOwnerState &); //1157D5..11592D only.
    void step(const SourcePortalMaskDefinition &,const SourcePortalOwnerState &,const SourcePortalDoorInput &);
    void destroy(const SourcePortalMaskDefinition &,const SourcePortalOwnerState &); //115220, before freeing body.
    void reset_acoustic(); //188980's 128ushort bank clear, no portal changes.
};
}
