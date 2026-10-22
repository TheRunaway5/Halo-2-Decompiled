// Exact uncovered bytes of admitted original vehicle/motion/math/update operands.
{
    constexpr std::array<std::uint8_t,48> bytes{0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x98,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x98,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    memory.mount(0x448634u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,8> bytes{0x00,0x00,0x87,0x43,0x00,0x00,0x90,0x41};
    memory.mount(0x450348u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
share_immutable_word(memory,0x46a0d4u,0xbf000000u,
    "native TU1.5 authored vehicle closure operand");
{
    constexpr std::array<std::uint8_t,4> bytes{0x00,0x00,0xa0,0x3f};
    memory.mount(0x46a198u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x66,0x66,0xe6,0x3e};
    memory.mount(0x46a330u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0xcd,0xcc,0x4c,0xbe};
    memory.mount(0x46a3f4u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,12> bytes{0x00,0x00,0x00,0xc0,0x9a,0x99,0x99,0xbd,0x00,0x00,0x44,0x40};
    memory.mount(0x46a448u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x83,0xf9,0xa2,0x3e};
    memory.mount(0x46a4b8u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,8> bytes{0x33,0x33,0x33,0x33,0x33,0x33,0xd3,0x3f};
    memory.mount(0x46a6d0u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    // The absent-unit camera also consumes this exact authored word. Retain
    // its existing immutable owner rather than mounting the same address twice.
    share_immutable_word(memory,0x46a75cu,0x3ea8f5c3u,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,12> bytes{0xdb,0x0f,0x49,0x3e,0x5c,0xc7,0xc2,0x40,0x74,0xd1,0x35,0x42};
    memory.mount(0x46a788u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,8> bytes{0x33,0x33,0x33,0x33,0x33,0x33,0xd3,0xbf};
    memory.mount(0x46a8c0u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x00,0x00,0x28,0x42};
    memory.mount(0x46a908u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x00,0x00,0x60,0x41};
    memory.mount(0x46aac4u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,64> bytes{0xd0,0xcc,0xcc,0x3d,0x00,0x00,0xa2,0x43,0x00,0x62,0x8e,0x47,0x33,0x33,0x33,0xbf,0xaf,0x4c,0xd9,0x3f,0xe4,0xcb,0x16,0x3f,0x67,0x66,0x66,0x3f,0x67,0x66,0xe6,0x3f,0x67,0x66,0x66,0x40,0x36,0x8d,0xa7,0xc0,0xf3,0x66,0x5f,0x40,0xc2,0xb8,0xb2,0x3f,0xe4,0xcb,0x96,0x40,0xa5,0x28,0x14,0x40,0x45,0x76,0x74,0x3f,0x92,0x0a,0x06,0x40};
    memory.mount(0x46ab30u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x00,0x00,0xc0,0x3f};
    memory.mount(0x473e18u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,16> bytes{0x00,0x00,0xc0,0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0xf0,0x40,0xcd,0xcc,0xcc,0x3d};
    memory.mount(0x473e24u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0xcd,0xcc,0xcc,0x3e};
    memory.mount(0x473e38u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored vehicle closure operand");
}
{
    constexpr std::array<std::uint8_t,4> bytes{0x00,0x00,0x20,0x40};
    memory.mount(0x48c748u,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadWrite,"native TU1.5 authored vehicle closure operand");
}
