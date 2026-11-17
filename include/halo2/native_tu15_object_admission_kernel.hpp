#pragma once

namespace halo2::native_object_admission_detail {
// Full 135B60 render-object admission policy. The object's frame stamp
// (0x4EABD0 indexed by the low handle half) selects one of two policies:
//
// - Already stamped this frame: locate the object's existing queue row (the
//   source retains the last probed cursor, -1 when nothing matched), merge its
//   cluster-association bit unless the row is contained or unassociated, then
//   OR the optional primary (4) and secondary (2) row flags. A secondary
//   search that exhausts a nonempty queue returns through the source's
//   second epilogue; an empty queue uses the common one.
// - Not yet stamped: run the genuine 135970 geometry admission on the
//   object's selected bounding sphere. A visible (or force-admitted) object
//   classifies its plane sides, packs its record flags, allocates a visibility
//   association unless contained, appends its queue row and, only when the
//   row was appended, publishes the frame stamp.
//
// The view owns every ordered access, the saved-register frame, the inlined
// leaf ABIs (135AE0, 135620, 166B60, C0CA0) and the genuine child handoffs
// (135970, 136E10). The policy owns only control flow.
template<class View>void admit_object(View&view){
    if(view.stamped()){
        view.find_row();
        if(!view.row_merged()&&view.row_associated())view.merge_association();
        if(view.primary()&&view.find_primary())view.mark_primary();
        if(view.secondary()&&view.secondary_rows()){
            if(!view.find_secondary())return view.finish(View::unmatched_epilogue);
            view.mark_secondary();
        }
        return view.finish(View::common_epilogue);
    }
    view.object();
    view.sphere();
    if(view.geometry()||view.forced())view.accept();
    view.override_sides();
    if(!view.visible())return view.finish(View::common_epilogue);
    view.sides();
    view.record_flags();
    if(!view.contained()&&view.association_requested())view.allocate_association();
    if(!view.append())return view.finish(View::common_epilogue);
    view.stamp();
    return view.finish(View::stamped_epilogue);
}
} // namespace halo2::native_object_admission_detail
