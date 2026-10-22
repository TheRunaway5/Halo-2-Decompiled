#include "halo2/source_euler.hpp"
#include <cmath>

namespace halo2 {
AuthoredBasis source_euler_basis(Vec3 r) {
    // Original x87 FCOS/FSIN ->FSTP float. Calling the float libm overload
    // rounds differently for some authored source angles.
    const float ca=float(std::cos(double(r.x))),sa=float(std::sin(double(r.x))),
                cb=float(std::cos(double(r.y))),sb=float(std::sin(double(r.y))),
                cc=float(std::cos(double(r.z))),sc=float(std::sin(double(r.z)));
    const volatile float sb_sc=sb*sc,sb_cc=sb*cc;
    const volatile float sa_cc=sa*cc,sa_sc=sa*sc,ca_cc=ca*cc,ca_sc=ca*sc;
    const volatile float sb_sc_ca=sb_sc*ca,sb_cc_ca=sb_cc*ca,
                         sb_sc_sa=sb_sc*sa,sb_cc_sa=sb_cc*sa;
    const volatile float cb_sc=cb*sc;
    return {{ca*cb,sa_cc-sb_sc_ca,sb_cc_ca+sa_sc},
            {-(sa*cb),sb_sc_sa+ca_cc,ca_sc-sb_cc_sa},
            {0.f-sb,0.f-cb_sc,cb*cc}};
}
} // namespace halo2
