#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// MinGW's legacy empty keyword macro collides with source ABI record fields.
#ifdef cdecl
#undef cdecl
#endif
#include <system_error>

namespace halo2::windows_detail {
[[noreturn]] inline void failure(const char* operation) {
    throw std::system_error(int(::GetLastError()),std::system_category(),operation);
}
}
