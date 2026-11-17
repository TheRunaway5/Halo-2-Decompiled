#pragma once
#include <cstdint>
#include <stdexcept>

namespace halo2::native_object_body_detail {
// The two complete 35-instruction CFGs are authenticated independently from
// their original executables. Only observation PCs use the canonical image;
// the admitted TU1.5 lookup still reads its actual 004ECBD0 object root.
struct CanonicalSites {
    constexpr std::uint32_t operator()(std::uint32_t pc)const{
        switch(pc){
            case 0xbcdf0u:case 0xbcdf1u:case 0xbcdf2u:case 0xbcdf4u:
            case 0xbcdf6u:case 0xbcdf9u:case 0xbcdfbu:case 0xbce01u:
            case 0xbce04u:case 0xbce06u:case 0xbce0cu:case 0xbce0eu:
            case 0xbce10u:case 0xbce13u:case 0xbce16u:case 0xbce19u:
            case 0xbce1cu:case 0xbce1fu:case 0xbce21u:case 0xbce24u:
            case 0xbce27u:case 0xbce29u:case 0xbce2bu:case 0xbce2du:
            case 0xbce2fu:case 0xbce31u:case 0xbce33u:case 0xbce36u:
            case 0xbce3bu:case 0xbce3du:case 0xbce41u:case 0xbce43u:
            case 0xbce46u:case 0xbce47u:case 0xbce48u:return pc-0x1140u;
            default:throw std::logic_error("Unqualified canonical lookup observation PC");
        }
    }
};
} // namespace halo2::native_object_body_detail
