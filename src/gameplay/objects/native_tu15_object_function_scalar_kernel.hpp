#pragma once

namespace halo2::native_object_function_scalar_detail {
enum class KeyRoute { constant_one, constant_zero, lookup };

template<class Frame> void evaluate(Frame& frame) {
    frame.open();
    if (frame.has_cache() && frame.lookup_cache()) {
        frame.finish_cache();
        return;
    }
    switch (frame.select_key()) {
    case KeyRoute::constant_one:
        frame.finish_constant_one();
        return;
    case KeyRoute::constant_zero:
        frame.finish_constant_zero();
        return;
    case KeyRoute::lookup:
        break;
    }
    if (!frame.has_datum() || frame.lookup_type()) {
        frame.finish_handled();
        return;
    }
    frame.open_rows();
    if (frame.has_rows()) {
        frame.begin_rows();
        do {
            if (frame.row_matches()) frame.lookup_row();
        } while (frame.advance_row());
        if (frame.row_handled()) {
            frame.finish_handled();
            return;
        }
        frame.reload_body();
    }
    if (frame.find_inherited_datum()) {
        frame.lookup_inherited();
        frame.finish_inherited();
        return;
    }
    frame.finish_absent();
}
} // namespace halo2::native_object_function_scalar_detail
