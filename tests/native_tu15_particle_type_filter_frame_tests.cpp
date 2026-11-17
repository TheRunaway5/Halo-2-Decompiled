#include "rendering/effects/native_tu15_particle_type_filter_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char** argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"17DE60 particle type filter",
        [](auto& state,auto before){halo2::native_particle_type_filter_detail::run_private(state,before);});
}
