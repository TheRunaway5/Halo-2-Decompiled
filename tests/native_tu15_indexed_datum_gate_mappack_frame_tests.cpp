#include "gameplay/objects/native_tu15_indexed_datum_gate_frame.hpp"
#include "native_tu15_object_property_frame_test_harness.hpp"

int main(int argc,char** argv) {
    using namespace halo2::native_indexed_datum_gate_detail;
    return halo2::object_property_frame_test::drive(argc,argv,"MapPack11 3EB10 indexed datum gate",
        [](auto& state,auto before){run_private<map_pack_11>(state,before);});
}
