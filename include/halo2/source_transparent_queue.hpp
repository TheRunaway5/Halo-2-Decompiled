#pragma once
#include "halo2/source_transparent_sort.hpp"

namespace halo2 {
//41490's selected ordinary-shader path. LOD selection and immediate-layer
//services are external; the queue owns only the original sorted command.
struct SourceTransparentShaderInput {
    std::uint32_t selected_template_layers{},shader=0xffffffffu,context=0xffffffffu,callback{};
    std::uint16_t shader_priority_flags{},view_type{};
    Vec3 center{},camera_position{},camera_forward{};
};
// Persistent4B6288 storage. Producers share this bank; a drain sorts absolute
// ordinals without clearing records or count. The caller owns callback tokens.
class SourceTransparentQueue {
public:
    static constexpr unsigned capacity=1024;
    //4B6291 is an explicit source state, cleared on the first full allocation.
    explicit SourceTransparentQueue(bool overflow_notice_pending=false)
        :overflow_notice_pending_(overflow_notice_pending){}
    //40ED2..40EDE, only for the ordinary world context. Storage survives.
    void reset_world();
    //1E2D0 clears priority and only the plane-present low byte.
    SourceTransparentCommand *allocate();
    //41040's ordinary layer15 producer, retaining untouched record bytes.
    SourceTransparentCommand *append_part(const SourceTransparentPartInput &);
    SourceTransparentCommand *append_shader(const SourceTransparentShaderInput &);
    //1E4E0 generic effect/location callback producer. Payload length is the
    //actual caller's byte count, bounded by the resident64-byte payload bank.
    SourceTransparentCommand *append_callback(Vec3 center,Vec3 camera_position,
        Vec3 camera_forward,std::uint32_t callback,std::span<const std::uint8_t> payload);
    //Every3FDF0 entry replaces first, including entries that append nothing.
    void begin_first_person_suffix();
    //1E370 includes null callbacks in sorting. Consumers skip them afterwards.
    std::vector<std::uint16_t> ordered_ordinals(Vec3 actual_camera_position) const;
    //40875..4087A, after the enabled first-person drain has completed.
    void finish_first_person_suffix();
    unsigned count() const{return count_;}
    unsigned first() const{return first_;}
    bool overflow_notice_pending() const{return overflow_notice_pending_;}
    std::span<const SourceTransparentCommand> commands() const{return {storage_.data(),count_};}
    std::span<const SourceTransparentCommand,capacity> storage() const{return storage_;}
private:
    std::array<SourceTransparentCommand,capacity> storage_{};
    unsigned count_{},first_{};
    bool overflow_notice_pending_{};
};
}
