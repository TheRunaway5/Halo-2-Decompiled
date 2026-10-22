// Original authored writable camera-effect request type table; no live state.
{
constexpr std::array<std::uint8_t,14> types{0x00,0x00,0x01,0x00,0x02,0x00,0x03,0x00,0x04,0x00,0x05,0x00,0x06,0x00};
memory.mount(0x47524cu,std::as_bytes(std::span(types)),SourceRuntimeMemory::Access::ReadWrite,
    "Original TU1.5 authored camera-effect request type table");
}
