#pragma once
#include <array>
#include <string_view>

namespace halo2_test {
// The original 15-row source-owned world registry contained these14 unique
// control keys. This bounded CPU numerical/addressing gate covers modes0/1/3/6;
// it does not claim numerical coverage of later catalog additions. The exact
// original registry is pinned here without a dependency on disposable storage.
inline constexpr std::string_view native_world_addressing_v1_registry_sha256=
    "222fd5b1960cd354f329d17521f0fd1901b3bb0418f0a0a17f7a4b51bf96c153";
inline constexpr std::array<std::string_view,14> native_world_addressing_v1_keys{{
    "10cf93e0f38d694f92ba9599d8f1bd323b66b56cb65bf26fb00b00649d78a557",
    "129219cfbdda6d8be0623de0eed4d9bef88cfa1d2b8a709bd7ed77994159a34f",
    "23465283faaf0b79ba1d7638d0374da936883cfc8838bbd55770c7ac1a88c20d",
    "47b555757a533e63068c20749be2bbbb0c40bb572361c22717c1482ec6e348a2",
    "4cbbab053f7cc91243f4b47586786114ecd4758121614212268e6794b69c3fd7",
    "5212bf6b7b878ca74a2f241254450b0a147725649761cd3a262acbcb95fde6f9",
    "751e6f6d90b5fc62234476d8caad4b9d306d004620bae2dea0ebf70b468fd15a",
    "899098cc0a9a59f266d81764b142786292231a100ad59d714aabe2c994999a3a",
    "8d1fdc4d8196ce945a24122b4dea00e297e0ed0f4010aab54ba92c4c879b5343",
    "98497e2553896fd279eef3784715b86245f926bfb8ffaf9d049047e729f115bd",
    "e51d3f2abf7b6ed085dcf1982aa6ad6e2f640c9c2a5e864a7171373b8562f282",
    "e97aa039e84d83e6a4dec15cde964f9d1dd3c5dc4281d34701debb1572af6abf",
    "f5447f689759d169af49feff591c6ed7265233b999216c668904b1460f2c5ce6",
    "fc108832f574575d10629894ec078b7e3ff80b841d42a2c6d3ac377d850b7f51",
}};
}
