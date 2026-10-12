#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaFontIteratorBank=std::array<std::byte,0x44>;
using VistaFontPreparedBank=std::array<std::byte,0x2000>;
using VistaWidgetMeasureScratch=std::array<std::byte,0x20>;
struct VistaWidgetFontServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<std::uint32_t(std::uint32_t)> fallback_font; //49867C.
  std::function<std::uint32_t(std::uint32_t)> font_definition; //431858.
  std::function<std::uint8_t(std::uint32_t,std::uint32_t)> pair_classify; //44CB2C.
  std::function<std::uint8_t(std::uint32_t,std::uint32_t)> glyph_request; //48DCB2.
  std::function<std::uint32_t()> clock; //437E51, only unknown enabled |escape.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> bounds; //Virtual20.
  std::function<std::uint32_t(VistaWidgetIdentity)> style; //Virtual4C.
  //Actual630A68 with six original stack arguments: bounds8 source address,
  //z-scale bits, caller rect8 address, prefix count, result8 address, flag.
  std::function<void(std::uint32_t,std::array<std::uint32_t,6>)> style_measure;
  //Masked declared source arithmetic context. Host control state is restored
  //at return; physical FP status/exception propagation is outside this API.
  std::uint16_t x87_control{};
  std::uint32_t mxcsr{};
};
//Complete44CB0F/44CA5F/44BF59/44BDEC UTF16-to-DWORD conversion and |escapes.
//Count is signed DWORD. Capacity1 writes only a terminator; no tail fill.
//The NUL escape-table entry terminates its scan: a trailing | remains literal.
//All source32 input/output banks remain registered and live through callbacks.
std::uint32_t vista_font_convert(std::uint32_t input_utf16,
    std::uint32_t output_dwords,std::uint32_t capacity,
    const VistaWidgetFontServices&);
//Complete498F61. First argument is the original EAX vector16 identity;
//the six DWORDs preserve the original stack inputs. Viewport dimensions and
//font/color globals are genuine registry reads, never native display defaults.
std::uint8_t vista_font_iterator_birth(std::uint32_t state_identity,
    std::span<std::byte,0x44>,std::uint32_t vector_identity,
    std::array<std::uint32_t,6>,const VistaWidgetFontServices&);
//Complete498B74, including signed WORD cursor, LF skipping, type history and
//optional genuine pair classification. Returns the original kind DWORD.
std::uint32_t vista_font_iterator_next(std::uint32_t state_identity,
    std::span<std::byte,0x44>,const VistaWidgetFontServices&);
//Complete4996E4. Actual font/glyph providers remain lazy mandatory services;
//a failed glyph request makes AL false without stopping later requests.
std::uint8_t vista_font_prepare_dwords(std::uint32_t input_dwords,
    std::uint32_t font,std::uint32_t state_identity,std::span<std::byte,0x44>,
    const VistaWidgetFontServices&);
//Complete499D1F through the above owners. The converted and iterator banks must
//also be registered in services so nested owners share their exact byte spans.
std::uint8_t vista_font_prepare(std::uint32_t input_utf16,std::uint32_t font,
    std::uint32_t converted_identity,std::span<std::byte,0x2000>,
    std::uint32_t state_identity,std::span<std::byte,0x44>,
    const VistaWidgetFontServices&);
//Complete611EA1 orchestration. Body scratch includes retained register-save
//slots; compiler cookie/epilogue machinery is excluded. Genuine630A68 remains
//required when reached; neither a width nor font readiness is synthesized.
void vista_widget_measure(VistaWidgetIdentity,std::uint32_t rect_identity,
    std::uint32_t prefix,std::uint32_t result_identity,std::uint32_t flag,
    std::uint32_t scratch_identity,std::span<std::byte,0x20>,
    const VistaWidgetFontServices&);
} //namespace halo2
