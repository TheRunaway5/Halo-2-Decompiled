#pragma once

namespace halo2::native_codec_format_four_decode_detail {
template<class View>void decode(View& view) {
    view.begin();
    for(unsigned lane=0;lane<3;++lane) {
        if(lane)view.begin_lane(lane);
        if(!view.has_tracks())continue;
        do {
            if(!view.load_bitmap(lane)) {
                view.skip_bitmap(lane);
            } else if(view.begin_bits(lane)) {
                do {
                    if(view.bit_set())view.decode_track(lane);
                    view.finish_bit(lane);
                } while(view.more_tracks());
                view.reload_bitmap(lane);
            }
        } while(view.more_bitmaps());
        view.end_lane(lane);
    }
    view.finish();
}
} // namespace halo2::native_codec_format_four_decode_detail
