#pragma once

namespace halo2::native_object_record_ratio_detail {
enum class Record { other, ratio, constant };

// TU1.5 107140 applies the signed upper bound before reading the object
// record. A negative index below that bound still addresses a real record.
template<class View> auto query(View& view) {
    view.open_record_table();
    if(view.index_below_count()){
        switch(view.record_kind()){
        case Record::constant:
            view.load_constant();
            return view.finish_constant();
        case Record::ratio:
            view.subtract_scaled_ratio();
            break;
        case Record::other:
            break;
        }
    }
    return view.finish();
}
} // namespace halo2::native_object_record_ratio_detail
