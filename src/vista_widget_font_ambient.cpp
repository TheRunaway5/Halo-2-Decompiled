#include "halo2/vista_widget_font_ambient.hpp"
#include "halo2/cache.hpp"
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif
namespace halo2 {
namespace {
#if defined(__i386__) || defined(__x86_64__)
struct Environment {
  unsigned mxcsr;std::uint16_t control;
  static Environment current() {
    Environment e{_mm_getcsr(),0};asm volatile("fnstcw %0":"=m"(e.control));return e;
  }
  void apply()const {asm volatile("fldcw %0"::"m"(control));_mm_setcsr(mxcsr);}
};
#endif
}
std::uint8_t vista_font_iterator_birth_ambient(std::uint32_t id,
    std::span<std::byte,0x44> bank,std::uint32_t vector,
    std::array<std::uint32_t,6> args,const VistaWidgetFontServices& providers) {
#if defined(__i386__) || defined(__x86_64__)
  const auto incoming=Environment::current();auto bound=providers;
  bound.x87_control=incoming.control;bound.mxcsr=incoming.mxcsr;
  Environment terminal{};bool observed=false;
  bound.font_definition=[&](std::uint32_t font) {
    if(!providers.font_definition)throw FormatError("Missing consumed Vista font service");
    const auto result=providers.font_definition(font);
    terminal=Environment::current();observed=true;return result;
  };
  const auto result=vista_font_iterator_birth(id,bank,vector,args,bound);
  if(!observed)throw FormatError("Unobserved terminal Vista font FP boundary");
  terminal.apply();return result;
#else
  (void)id;(void)bank;(void)vector;(void)args;(void)providers;
  throw FormatError("Missing exact x86 Vista ambient font FP provider");
#endif
}
std::uint32_t vista_font_iterator_next_ambient(std::uint32_t id,
    std::span<std::byte,0x44> bank,const VistaWidgetFontServices& providers) {
  return vista_font_iterator_next(id,bank,providers);
}
} //namespace halo2
