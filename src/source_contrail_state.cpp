#include "halo2/source_contrail_state.hpp"
namespace halo2 {
PixelProgram source_contrail_pixel_program(std::uint16_t blend,
    std::uint8_t properties,std::uint32_t declaration) {
    PixelProgram result;auto &p=result.words;
    p[0]=0x18202015u;p[8]=0xcu;p[9]=0x1c00u;p[26]=0xc4u;
    unsigned count;
    if(properties&2u) {
        p[34]=0x08080000u;p[35]=0x0c0c0000u;p[36]=0x250c0508u;
        p[45]=p[46]=0xc0u;p[47]=0xc00u;count=3;
    }else {p[34]=0x08050000u;p[45]=0xc0u;count=1;}
    p[54]=1;
    if(!(declaration&4u)) {
        if(blend<=7u)switch(blend) {
        case 0:p[count]=0x1c140000u;p[26+count]=0xc00u;break;
        case 1:case 5:p[34+count]=0x0c142034u;p[45+count]=0xc00u;break;
        case 2:p[34+count]=0x0c14a034u;p[45+count]=0xc00u;break;
        case 7:p[count]=0x1c140000u;p[26+count]=0xc00u;[[fallthrough]];
        default:p[34+count]=0x0c140000u;p[45+count]=0xc00u;break;
        }
        ++count;
    }
    p[53]=count;return result;
}
}
