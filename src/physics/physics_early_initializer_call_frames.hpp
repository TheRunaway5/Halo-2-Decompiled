#pragma once
#include <array>
#include <cstdint>
namespace halo2::physics_early_initializer_frames {
enum class Module { Arena, Pool, MainHeap, Renderer, Required };
struct Frame {std::uint32_t site,target,return_address,entry;Module module;};
inline constexpr std::array<Frame,8> frames{{
{0x1c5f2au,0x166a80u,0x1cb15fu,0x1656a0u,Module::Arena},
{0x1c5f40u,0x22cdc0u,0x1cb175u,0x2284c0u,Module::Pool},
{0x1c5f45u,0x1d6ee0u,0x1cb17au,0x1d2620u,Module::Pool},
{0x1c5f4au,0x149630u,0x1cb17fu,0x1482a0u,Module::MainHeap},
{0x1c5f4fu,0x149680u,0x1cb184u,0x1482f0u,Module::Renderer},
{0x1f15eau,0x166a80u,0x1f5edfu,0x1656a0u,Module::Arena},
{0x1482b0u,0x33b3acu,0x149645u,0x33785cu,Module::Pool},
{0x1482b9u,0x2dbc2eu,0x14964eu,0x2d817eu,Module::Required},
}};
} // namespace halo2::physics_early_initializer_frames
