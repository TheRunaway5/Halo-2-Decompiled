#pragma once

namespace halo2 {
// The two ranked axes build perpendicular directions. Evaluate both with
// the actual edge owner, then append the selected edge to the live hull.
// The view owns caller scratch, the two estimate instructions and the child
// ABI; a store or child can change what the next acquisition sees.
template<class View> void native_tu15_solver_hull_basis_kernel(View& view) {
    view.open();
    view.measure_axes();
    if (!view.first_axis_kept()) view.promote_y();
    if (!view.middle_axis_kept()) view.promote_z();
    view.build_perpendiculars();
    view.normalize_first();
    view.normalize_second_and_evaluate_first();
    view.evaluate_second();
    const bool second = view.select_second();
    view.append(second);
    view.finish(second);
}
} // namespace halo2
