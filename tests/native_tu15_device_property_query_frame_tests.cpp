#include "native_tu15_object_property_frame_test_harness.hpp"
#include "gameplay/objects/native_tu15_device_property_query_frame.hpp"

int main(int argc,char**argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"10BE20",[](auto&state,auto before) {
        return halo2::native_device_property_query_detail::run_private(state,before);
    });
}
