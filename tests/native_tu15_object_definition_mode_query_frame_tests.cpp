#include "gameplay/objects/native_tu15_object_definition_mode_query_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char**argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"165590 object definition mode query",
        [](auto& state,auto before){return halo2::native_object_definition_mode_query_detail::run_private(state,before);});
}
