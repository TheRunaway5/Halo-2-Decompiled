#include "gameplay/objects/native_tu15_object_ratio_caller_extended_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char** argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"1046C0 extended record branch",
        [](auto& state,auto before){halo2::native_object_ratio_caller_detail::run_extended_record_path(state,before);});
}
