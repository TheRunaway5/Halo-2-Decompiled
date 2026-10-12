#pragma once
#include "halo2/vista_widget_registration.hpp"
namespace halo2 {
using VistaGlyphEllipsisScratch=std::array<std::byte,0x400>;
struct VistaGlyphSupportServices {
 std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
 std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
 std::function<std::uint32_t()> context; //60BB89, genuine localized context.
 std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> localized; //43E3AC(list,SID,out).
 std::function<std::uint32_t()> errno_address; //68EE22, genuine TLS errno bank.
 std::function<void(std::array<std::uint32_t,5>)> invalid_parameter; //68EDC3, returning handler.
};
//Complete498C44 including the supplied executable's literal jump tables.
std::uint8_t vista_glyph_color_classify(std::uint32_t character);
//Complete687567. Source32 banks remain live; partial writes and overlap retain
//the original order. Error paths require the genuine returning CRT services.
std::uint32_t vista_glyph_copy_wide(std::uint32_t destination,
 std::uint32_t capacity,std::uint32_t input,const VistaGlyphSupportServices&);
//Complete499680. Destination is entry-captured ESI. Scratch identity is the
//actual original entry ESP minus404; only the first WORD is initially cleared.
//The supplied400-byte span excludes CRT cookie/epilogue and is caller-registered.
void vista_glyph_ellipsis(std::uint32_t destination,std::uint32_t scratch_identity,
 std::span<std::byte,0x400>,const VistaGlyphSupportServices&);
} //namespace halo2
