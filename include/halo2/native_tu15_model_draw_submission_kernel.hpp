#pragma once

namespace halo2::native_model_draw_submission_detail {
// Full 136A80 model section draw-unit submission policy. For every section
// of the model render record: an optional "same part only" argument skips
// sections whose first part differs from section 0's; a single-owner record
// of a unit carrying shadow geometry is first copied (0x79 dwords) into the
// caller frame with its override fields patched, and that copy is used for
// the rest of the section. Each part of the section then packs its flags and
// arguments and submits one draw unit through the separately owned 2CBD0.
// The view owns all ordered accesses, the 8-aligned 0x214 frame and the
// 2CBD0 required boundary; the policy owns only control flow.
template<class View>void submit_model(View&view){
    if(view.begin()){
        do{
            if(view.skip_section())continue;
            if(view.owned()&&view.unit_owner()&&view.shadowed())view.copy_record();
            if(view.parts()){
                view.begin_parts();
                do view.submit();while(view.next_part());
            }
        }while(view.next_section());
    }
    view.finish();
}
} // namespace halo2::native_model_draw_submission_detail
