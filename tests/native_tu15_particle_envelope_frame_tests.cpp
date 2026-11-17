#include "rendering/effects/native_tu15_particle_envelope_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char** argv) {
    return halo2::object_property_frame_test::drive(argc,argv,"252EC0 particle envelope",
        [](auto& state,auto before){halo2::native_particle_envelope_detail::run_private(state,before);});
}
