#pragma once
// Native TU1.5 rotation product, entered at 1459B0 (scene device constructor
// unit): one 3x3 matrix (nine floats, row after row) times another.
//
//   rotation_product::multiply   1459B0(EAX = out, EDX = left, ECX = right)
//
// A factor that is also the output is first copied (as REP MOVSD copies it)
// to a matrix in the frame. Each element of the product is the original's
// sum of three products, in its order: out[r][c] is the sum over k of
// left[k][c] * right[r][k], each element's terms in the order and operand
// order the original evaluates them (which decides which NaN survives).
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory
// is the retained TU1.5 bank, reached through a Memory object with typed
// loads and stores; every access names its role (Access), and the boundary
// adapter (native_tu15_rotation_product.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_rotation_product {
namespace sse=native_aim_clusters::sse;
inline constexpr std::uint32_t words=9u;
// One term: which factor is loaded first, its element and the other factor's element (byte offsets).
struct Term {bool left_first;std::uint32_t first,second;};
// An element: its offset in the output and its three terms, in the original's order.
struct Element {std::uint32_t out;Term terms[3];};
inline constexpr Element elements[9]{
    {0x00u,{{true,0x00u,0x00u},{true,0x0cu,0x04u},{false,0x08u,0x18u}}},
    {0x04u,{{true,0x10u,0x04u},{true,0x04u,0x00u},{true,0x1cu,0x08u}}},
    {0x08u,{{true,0x14u,0x04u},{true,0x08u,0x00u},{true,0x20u,0x08u}}},
    {0x0cu,{{true,0x0cu,0x10u},{false,0x0cu,0x00u},{false,0x14u,0x18u}}},
    {0x10u,{{true,0x1cu,0x14u},{true,0x10u,0x10u},{true,0x04u,0x0cu}}},
    {0x14u,{{true,0x20u,0x14u},{true,0x14u,0x10u},{true,0x08u,0x0cu}}},
    {0x18u,{{true,0x00u,0x18u},{true,0x0cu,0x1cu},{true,0x18u,0x20u}}},
    {0x1cu,{{true,0x1cu,0x20u},{true,0x10u,0x1cu},{true,0x04u,0x18u}}},
    {0x20u,{{true,0x20u,0x20u},{true,0x14u,0x1cu},{true,0x08u,0x18u}}}};

namespace rotation_product {
enum class Access {copy_left_read,copy_left_write,copy_right_read,copy_right_write,first,second,product_store};
enum class Outcome {separate,left_aliased,right_aliased,both_aliased};
// Frame:
//   copy_to()                       the frame's matrix now
//   compared(factor, out)           CMP EDX,EAX (then the saved registers) and CMP EBX,EAX
//   copying(which, from, to), step()   a REP MOVSD's registers and direction (which: 0 left, 1 right)
//   at_term(element, term), term(element, term, product, sum)   the term's place; its MOVSS/MULSS/ADDSS in XMM0 and XMM1
template<class Memory,class Frame> Outcome multiply(Memory &memory,Frame &frame,std::uint32_t out,std::uint32_t left,std::uint32_t right) {
    using enum Access;
    const auto copy=[&](unsigned which,std::uint32_t from) {
        const auto to=frame.copy_to();
        frame.copying(which,from,to);
        const auto step=frame.step();
        const auto read=which?copy_right_read:copy_left_read;
        const auto write=which?copy_right_write:copy_left_write;
        auto at_from=from,at_to=to;
        for(std::uint32_t word=0;word<words;++word,at_from+=step,at_to+=step)memory.store32(write,at_to,memory.load32(read,at_from));
        return to;
    };
    frame.compared(left,out);
    const bool left_aliased=left==out;
    if(left_aliased)left=copy(0u,left);
    frame.compared(right,out);
    const bool right_aliased=right==out;
    if(right_aliased)right=copy(1u,right);
    for(unsigned e=0;e<9u;++e) {
        std::uint32_t sum=0u;
        for(unsigned k=0;k<3u;++k) {
            const auto &t=elements[e].terms[k];
            frame.at_term(e,k);
            const auto first_value=memory.load32(first,(t.left_first?left:right)+t.first);
            const auto product=sse::multiply(first_value,memory.load32(second,(t.left_first?right:left)+t.second));
            sum=k?sse::add(sum,product):product;
            frame.term(e,k,product,sum);
        }
        memory.store32(product_store,out+elements[e].out,sum);
    }
    return left_aliased?(right_aliased?Outcome::both_aliased:Outcome::left_aliased):(right_aliased?Outcome::right_aliased:Outcome::separate);
}
} // namespace rotation_product
} // namespace halo2::native_rotation_product
