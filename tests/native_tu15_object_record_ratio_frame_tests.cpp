#include "gameplay/objects/native_tu15_object_record_ratio_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char**argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"107140 object record ratio",
        [](auto& state,auto before){return halo2::native_object_record_ratio_detail::run_private(state,before);});
}
