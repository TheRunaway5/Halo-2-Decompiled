#pragma once

#include <cstdint>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::detail {
namespace tu15_quaternion_matrix {
inline void push(std::uint32_t n){asm volatile("flds %0"::"m"(n):"memory","st");}
inline void multiply(std::uint32_t n){asm volatile("fmuls %0"::"m"(n):"memory","st");}
inline std::uint32_t spill(){std::uint32_t out;asm volatile("fstps %0":"=m"(out)::"memory","st");return out;}
// 37C5D0 keeps doubled X/Y and their W products extended, spills doubled Z,
// then writes the 12 rotation words in source order.
inline void doubled_component(std::uint32_t n){asm volatile("flds %0\n\tfadd %%st(0),%%st"::"m"(n):"memory","st");}
inline std::uint32_t doubled_z(std::uint32_t n){std::uint32_t out;asm volatile("flds %1\n\tfadd %%st(0),%%st\n\tfstps %0":"=m"(out):"m"(n):"memory","st");return out;}
inline std::uint32_t x_product(auto& s,unsigned lane){asm volatile("fld %%st(1)":::"memory","st");multiply(s.quaternion_component(lane));return spill();}
inline std::uint32_t y_product(auto& s,unsigned lane){asm volatile("fld %%st(0)":::"memory","st");multiply(s.quaternion_component(lane));return spill();}
inline std::uint32_t z_product(auto& s,std::uint32_t z2,unsigned lane){push(z2);multiply(s.quaternion_component(lane));return spill();}
inline std::uint32_t x_w(auto& s){asm volatile("fxch %%st(1)":::"memory","st");multiply(s.quaternion_component(3));return spill();}
inline std::uint32_t diagonal(auto& s,std::uint32_t a,std::uint32_t b){asm volatile("flds %0\n\tfadds %1"::"m"(a),"m"(b):"memory","st");const auto one=s.one();std::uint32_t out;asm volatile("fsubrs %1\n\tfstps %0":"=m"(out):"m"(one):"memory","st");return out;}
inline std::uint32_t xy_plus_zw(std::uint32_t xy){std::uint32_t out;asm volatile("flds %1\n\tfadd %%st(1),%%st\n\tfstps %0":"=m"(out):"m"(xy):"memory","st");return out;}
inline std::uint32_t xz_minus_yw(std::uint32_t xz){std::uint32_t out;asm volatile("flds %1\n\tfsub %%st(2),%%st\n\tfstps %0":"=m"(out):"m"(xz):"memory","st");return out;}
inline std::uint32_t xy_minus_zw(std::uint32_t xy){std::uint32_t out;asm volatile("flds %1\n\tfsub %%st(1),%%st\n\tfstps %0":"=m"(out):"m"(xy):"memory","st");return out;}
inline void discard_zw(){asm volatile("fstp %%st(0)":::"memory","st");}
inline std::uint32_t scalar_sum(std::uint32_t a,std::uint32_t b){std::uint32_t out;asm volatile("flds %1\n\tfadds %2\n\tfstps %0":"=m"(out):"m"(a),"m"(b):"memory","st");return out;}
inline std::uint32_t yw_plus_xz(std::uint32_t xz){std::uint32_t out;asm volatile("fadds %1\n\tfstps %0":"=m"(out):"m"(xz):"memory","st");return out;}
inline std::uint32_t scalar_difference(std::uint32_t a,std::uint32_t b){std::uint32_t out;asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(out):"m"(a),"m"(b):"memory","st");return out;}
}

// The caller supplies the selected x87 environment and owns the input and
// output banks. Keeping those accesses lazy preserves the source store order.
inline void native_tu15_quaternion_matrix(auto& s) {
  using namespace tu15_quaternion_matrix;
  doubled_component(s.quaternion_component(0));
  doubled_component(s.quaternion_component(1));
  const auto z2=doubled_z(s.quaternion_component(2));
  const auto xx=x_product(s,0),xy=y_product(s,0),xz=z_product(s,z2,0);
  const auto yy=y_product(s,1),yz=z_product(s,z2,1),zz=z_product(s,z2,2);
  const auto xw=x_w(s);
  multiply(s.quaternion_component(3));
  push(z2);
  multiply(s.quaternion_component(3));
  s.rotation_component(0,diagonal(s,zz,yy));
  s.rotation_component(1,xy_plus_zw(xy));
  s.rotation_component(2,xz_minus_yw(xz));
  s.rotation_component(3,0);
  s.rotation_component(4,xy_minus_zw(xy));
  discard_zw();
  s.rotation_component(5,diagonal(s,zz,xx));
  s.rotation_component(6,scalar_sum(xw,yz));
  s.rotation_component(7,0);
  s.rotation_component(8,yw_plus_xz(xz));
  s.rotation_component(9,scalar_difference(yz,xw));
  s.rotation_component(10,diagonal(s,yy,xx));
  s.rotation_component(11,0);
}
} // namespace halo2::detail
#endif
