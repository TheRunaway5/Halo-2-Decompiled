#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace halo2 {
// Source32 addresses include the caller argument bank and the original private
// stack ranges. Providers must register those ranges before invoking an owner.
// Arithmetic uses the caller's actual masked x87/MXCSR context; no display/font
// defaults or FP status resets are supplied here. Reached services are required.
struct VistaGlyphLayoutServices {
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
  std::function<std::uint8_t(std::uint32_t,std::uint32_t,
      std::array<std::uint32_t,6>)> iterator_birth; //498F61, live source44 bank.
  std::function<std::uint32_t(std::uint32_t)> iterator_next; //498B74.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> glyph; //48DC9A.
  std::function<std::uint32_t(std::uint32_t)> font_definition; //431858.
  std::function<std::uint8_t(std::uint32_t)> color_classify; //498C44.
  std::function<void(std::uint32_t,std::uint32_t)> ellipsis; //499680(ESI,entry).
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> convert_one; //44BF59.
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> copy; //687E18.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> convert; //44CB0F.
  std::function<void(std::uint32_t,std::array<std::uint32_t,14>)> raster;
};
// Complete44C6F6, the private character range E132..E13B (AL only).
std::uint8_t vista_glyph_character_classify(std::uint32_t character);
struct VistaGlyphLineWidth { std::uint32_t width_bits{},cursor{}; };
// Complete431891; returned WORD remains zero extended, including negative
// authored kerning values. The callers separately sign extend that WORD.
std::uint32_t vista_glyph_kerning(std::uint32_t definition,
    std::uint32_t previous,std::uint32_t character,const VistaGlyphLayoutServices&);
// The entry value is original ESP on entry, with DWORD arguments at entry+4.
// Complete498726 bounds callback (14 arguments).
void vista_glyph_collect_bounds(std::uint32_t entry,const VistaGlyphLayoutServices&);
// Complete498D52, EAX iterator and four stack arguments.
VistaGlyphLineWidth vista_glyph_line_width(std::uint32_t iterator,
    std::uint32_t entry,const VistaGlyphLayoutServices&);
// Complete4991AA, EAX optional float clip, CX initial signed WORD cursor,
// seven stack arguments. Real raster callbacks remain an explicit boundary.
void vista_glyph_draw(std::uint32_t clip,std::uint16_t cursor,
    std::uint32_t entry,const VistaGlyphLayoutServices&);
// Complete49975A seven-argument layout orchestration.
void vista_glyph_layout(std::uint32_t entry,const VistaGlyphLayoutServices&);
// Complete499C60 five-argument DWORD-text measurement wrapper.
void vista_glyph_measure_dwords(std::uint32_t entry,const VistaGlyphLayoutServices&);
// Complete499D97 five-argument UTF16 measurement wrapper. The source2000-byte
// conversion bank lives at entry-2000; the stack-probe/CRT cookie is excluded.
void vista_glyph_measure(std::uint32_t entry,const VistaGlyphLayoutServices&);
} //namespace halo2
