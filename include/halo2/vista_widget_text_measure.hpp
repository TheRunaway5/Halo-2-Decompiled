#pragma once
#include "halo2/vista_widget_font.hpp"
namespace halo2 {
using VistaTextStyleMeasureScratch=std::array<std::byte,0x28>;
using VistaTextCoreMeasureScratch=std::array<std::byte,0x83c>;
struct VistaTextMeasureServices {
 std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
 std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
 std::function<std::uint32_t(std::uint32_t)> text; //Actual style virtual+C.
 std::function<void(std::array<std::uint32_t,12>)> core; //Actual6307F1.
 std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> copy_wide; //44BD27(dest,input,capacity).
 std::function<std::uint8_t(std::uint32_t,std::uint32_t,std::uint32_t)> rewrite; //62F712.
 std::function<std::uint32_t(std::uint32_t)> font_definition; //431858.
 std::function<void(std::uint32_t,std::uint32_t)> project_point; //61D05F(point3,viewport8).
 std::function<void(std::array<std::uint32_t,5>)> layout; //499D97.
 std::uint16_t x87_control{};
 std::uint32_t mxcsr{};
};
//Complete630A68. Incoming six-DWORD argument bank is live source32 memory,
//so source rereads after callbacks remain observable. No native pointer casts.
//Scratch owns body locals/register-save bytes; CRT cookie/epilogue excluded.
void vista_text_style_measure(std::uint32_t style,std::uint32_t arguments,
 std::uint32_t scratch_identity,std::span<std::byte,0x28>,const VistaTextMeasureServices&);
//Complete6307F1, including actual498A87/498A9D font-state operations.
//Incoming twelve DWORDs retain original order. Genuine token/projection/layout
//services stay mandatory when reached; this owner supplies no measured widths.
void vista_text_core_measure(std::uint32_t arguments,std::uint32_t scratch_identity,
 std::span<std::byte,0x83c>,const VistaTextMeasureServices&);
} //namespace halo2
