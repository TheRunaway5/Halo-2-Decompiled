#pragma once

namespace halo2::native_object_function_query_detail {

enum class Selection {
    linked,
    effect_ratio,
    progress,
    complement_progress,
    bit_gate,
    object_sentinel,
    linked_sentinel,
    field_2c4,
    field_208,
    byte_243,
    field_1b8,
    field_2b0,
    field_25c,
    field_260,
    field_264,
    byte_1f8,
};

template<class View>
void evaluate(View& view) {
    const auto selection = view.select();
    switch (selection) {
    case Selection::linked:
        if (view.prepare_inherited()) view.inherited_query();
        return view.finish_inherited();
    case Selection::bit_gate:
        view.publish_bit_gate();
        break;
    case Selection::object_sentinel:
        view.publish_object_sentinel();
        break;
    case Selection::linked_sentinel:
        view.publish_linked_sentinel();
        break;
    default:
        view.acquire_scalar(selection);
        view.clamp_scalar();
        break;
    }
    view.publish_scalar();
}

} // namespace halo2::native_object_function_query_detail
