#include "gameplay/objects/native_tu15_object_table_ratio_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char** argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"24B430 object table ratio",
        [](auto& state,auto before){return halo2::native_object_table_ratio_detail::run_private(state,before);});
}
