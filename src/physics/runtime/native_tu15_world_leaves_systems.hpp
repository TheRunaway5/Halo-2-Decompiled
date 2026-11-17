#pragma once
// Native TU1.5 physics world leaves (group H of
// docs/native_replacement/REMAINING_TRANSLATED_20261115.md):
//
//   surface_polygon::build             1F5FE0  the Havok polygon shape of one
//                                              collision BSP surface
//   havok_rotation::multiply_inverse   2E4550  inverse(a) * b of two Havok
//                                              transforms
//   contact_record::copy               303BA0  copy of a contact record
//   havok_rotation::rotate_inverse     2E47B0  a vector rotated by the inverse
//                                              (transpose) of a rotation
//   contact_array::expand_one          303EE0  append a contact record to an
//                                              hkArray
//   local_buffers::construct           303C70  hkLocalArray of shape buffers
//   local_buffers::destroy             303D00  on the thread memory's stack
//   collection_agent::process_collision 304350 the welded collision agent of
//                                              a shape collection
//   manifold_report::verify_and_report 30CF40  a child agent's cached contact
//                                              points checked and reported
//   contact_weld::weld                 304130  contact records welded against
//                                              a contact
//   key_sort::sort                     2F33B0  quicksort of unsigned keys
//   shape_key_filter::may_collide      1F76A0  whether a shape key's child
//                                              shape may still collide
//   closest_point::find                2F8630  the GSK agent's closest point
//                                              between two convex shapes
//   swept_pair::closest_point          2E4FE0  the same, for the game's swept
//                                              shapes
//   havok_rotation::rotate             2E4760  a vector rotated (2F8630 runs
//                                              it inline)
//   bsp_child_shape::child_shape       1F7C10  the child shape a key names in
//                                              the structure's collision shape
//   structure_aabb_query::query_aabb   1F6D10  the keys of the structure's
//                                              child shapes near an AABB
//   child_agent_update::update         2F36F0  a collection agent's child
//                                              agents, one per child near body A
//   object_flag_update::update         E8D80   an object's flags, kept or
//                                              cleared by their update functions
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (each system's Access enum), and the boundary adapter
// (native_tu15_world_leaves.hpp) maps that role to the original instruction
// for the diagnostic cursor and the store observer. Arguments, and the locals
// the original keeps in its stack frame, come from a Frame object that the
// adapter also supplies. Registers, flags and stack slots belong to the
// adapter, not to this file.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the store
//     observer sees every write, a faulting read must be the original's
//     first fault, and verification replays every read in order);
//   - single-precision arithmetic runs the original SSE instructions with the
//     original operand order and association, so NaN payloads, rounding and
//     the MXCSR sticky flags are identical; comments mark where it matters;
//   - address arithmetic is 32-bit and wraps as the original's does;
//   - the values the original leaves behind in scratch registers are returned
//     in each result's Residue, for the adapter only.
//
// Field names describe what the code does with each field. Where the game
// meaning is not established, the comment says so.
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>
#include <emmintrin.h>
#include <xmmintrin.h>

namespace halo2::native_world_leaves {

// ------------------------------------------------------------------ helpers
// base + offset in the original's 32-bit address arithmetic.
constexpr std::uint32_t at(std::uint32_t base,std::size_t offset){return base+std::uint32_t(offset);}
// The address of element index of an array of size-byte elements.
constexpr std::uint32_t element(std::uint32_t base,std::uint32_t index,std::size_t size){return base+index*std::uint32_t(size);}

// Four single-precision lanes, as an SSE register or an hkVector4 holds them.
using Vector4=__m128;
using Lanes=std::array<std::uint32_t,4>;
inline Lanes lanes(Vector4 v){return std::bit_cast<Lanes>(v);}
inline Vector4 vector(Lanes bits){return std::bit_cast<Vector4>(bits);}
// Every lane set to lane i of v.
inline Vector4 splat(Vector4 v,unsigned i){const auto x=lanes(v)[i];return vector({x,x,x,x});}

// Single-precision operations exactly as the original SSE instructions: the
// first operand is the destination, so it owns a NaN payload when both
// operands are NaNs, and nothing is reordered or fused. Scalar forms act on
// lane 0 only.
namespace sse {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline float multiply(float a,float b){__m128 x=_mm_set_ss(a);asm volatile("mulss %1,%0":"+x"(x):"x"(_mm_set_ss(b)));return _mm_cvtss_f32(x);}
inline float add(float a,float b){__m128 x=_mm_set_ss(a);asm volatile("addss %1,%0":"+x"(x):"x"(_mm_set_ss(b)));return _mm_cvtss_f32(x);}
// UCOMISS (quiet): a == b and both ordered.
inline bool ordered_equal(float a,float b) {
    unsigned char equal,unordered;
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1":"=qm"(equal),"=qm"(unordered):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc");
    return equal && !unordered;
}
inline Vector4 multiply(Vector4 a,Vector4 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b));return a;}
inline Vector4 add(Vector4 a,Vector4 b){asm volatile("addps %1,%0":"+x"(a):"x"(b));return a;}
inline Vector4 subtract(Vector4 a,Vector4 b){asm volatile("subps %1,%0":"+x"(a):"x"(b));return a;}
// CMPLTPS: all ones in each lane where a < b (a NaN gives zero and signals invalid).
inline Vector4 less(Vector4 a,Vector4 b){asm volatile("cmpltps %1,%0":"+x"(a):"x"(b));return a;}
#endif
} // namespace sse

// ------------------------------------------------------------- game layouts
// A tag block: the element count and the address of the first element.
struct TagBlock {
    std::int32_t count;
    std::uint32_t elements;
};

// collision_bsp: the blocks of a collision BSP, in tag order.
struct CollisionBsp {
    TagBlock bsp3d_nodes,planes,leaves,bsp2d_references,bsp2d_nodes;
    TagBlock surfaces;  // BspSurface[]
    TagBlock edges;     // BspEdge[]
    TagBlock vertices;  // BspVertex[]
};
static_assert(offsetof(CollisionBsp,surfaces.elements)==0x2c && offsetof(CollisionBsp,edges.elements)==0x34 &&
              offsetof(CollisionBsp,vertices.elements)==0x3c);

// A collision surface. Its boundary is a ring of edges starting at first_edge.
struct BspSurface {
    std::int16_t plane;
    std::uint16_t first_edge;
    std::uint8_t flags;
    std::uint8_t breakable_surface;
    std::uint16_t material;
};
static_assert(sizeof(BspSurface)==8);

// A collision edge. It borders two surfaces, one on each side. Walking the
// boundary of the left surface goes start vertex -> forward edge; walking
// the right surface goes end vertex -> reverse edge. So each array below is
// indexed by the side that faces the surface being walked (0 left, 1 right).
struct BspEdge {
    std::uint16_t vertex[2];     // start vertex, end vertex
    std::uint16_t next_edge[2];  // forward edge, reverse edge
    std::uint16_t surface[2];    // left surface, right surface
};
static_assert(sizeof(BspEdge)==12);

// A collision vertex: the point, then the first edge that uses it.
struct BspVertex {
    float point[3];
    std::uint16_t first_edge;
    std::uint16_t padding;
};
static_assert(sizeof(BspVertex)==16);

// real_matrix4x3: a uniform scale, the three axes, then the position. A
// point p maps to position + forward*p.x + left*p.y + up*p.z, after scaling.
struct RealMatrix4x3 {
    float scale;
    float forward[3];
    float left[3];
    float up[3];
    float position[3];
};
static_assert(offsetof(RealMatrix4x3,forward)==0x04 && offsetof(RealMatrix4x3,left)==0x10 &&
              offsetof(RealMatrix4x3,up)==0x1c && offsetof(RealMatrix4x3,position)==0x28);

// hkTransform: the rotation as three column vectors, then the translation.
// Each is an hkVector4 (16 bytes; the rotation does not use w).
struct HavokTransform {
    Lanes column[3];
    Lanes translation;
};
static_assert(offsetof(HavokTransform,translation)==0x30);

// --------------------------------------------------------- havok rotations
namespace havok_rotation {
// The rotation of an hkTransform: three column vectors.
struct Rotation {
    Vector4 column[3];
};

// Column i of the transposed rotation: coordinate i of each of the three
// columns. The original's transpose repeats the last value in w.
inline Vector4 transposed_column(const Rotation &rotation,unsigned i) {
    const auto z=lanes(rotation.column[2])[i];
    return vector({lanes(rotation.column[0])[i],lanes(rotation.column[1])[i],z,z});
}

// A vector rotated by the transpose of a rotation (the inverse, for a pure
// rotation), with the partial products the original leaves in its scratch
// registers. The adapter needs them, and the inputs, to publish those
// registers.
struct InverseRotated {
    Vector4 value;
    Vector4 input;
    Vector4 last_column;  // rotation.column[2]
    Vector4 y_terms;
    Vector4 z_terms;
};

// R^T v. Lane i of the result is column[i] . v (lane w repeats lane z).
// The sum is (x terms + y terms) + z terms, and each product takes the
// transposed column first and the broadcast coordinate second. This is the
// original's order; it decides the rounding and which NaN payload survives.
inline InverseRotated rotate_inverse(const Rotation &rotation,Vector4 v) {
    const auto x_terms=sse::multiply(transposed_column(rotation,0),splat(v,0));
    const auto y_terms=sse::multiply(transposed_column(rotation,1),splat(v,1));
    const auto z_terms=sse::multiply(transposed_column(rotation,2),splat(v,2));
    return {sse::add(sse::add(x_terms,y_terms),z_terms),v,rotation.column[2],y_terms,z_terms};
}

// 2E47B0, hkVector4::setRotatedInverseDir: out = rotation^T * vector.
// Frame: rotation() and vector() are the two argument addresses.
enum class RotateAccess {column_2,column_1,column_0,input,result};
template<class Memory,class Frame> InverseRotated rotate_inverse(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum RotateAccess;
    const auto source=frame.rotation();
    Rotation rotation;
    // The original loads the columns last to first.
    rotation.column[2]=memory.load_vector(column_2,at(source,offsetof(HavokTransform,column[2])));
    rotation.column[1]=memory.load_vector(column_1,at(source,offsetof(HavokTransform,column[1])));
    rotation.column[0]=memory.load_vector(column_0,at(source,offsetof(HavokTransform,column[0])));
    const auto v=memory.load_vector(input,frame.vector());
    const auto rotated=rotate_inverse(rotation,v);
    memory.store_vector(result,out,rotated.value);
    return rotated;
}

// 2E4550, hkTransform::setMulInverseMul: out = inverse(a) * b, for rigid
// transforms. The rotation is a^T * b.rotation and the translation is
// a^T * (b.translation - a.translation), computed as a^T * b.translation
// minus a^T * a.translation.
// Frame: first() is a, second() is b (argument addresses).
enum class MultiplyAccess {
    a_column_2,a_column_0,a_column_1,
    b_row,out_row,
    a_column_2_again,a_translation,a_column_0_again,a_column_1_again,
    out_translation,translation_store
};
struct MultiplyResidue {
    std::uint32_t b;             // the second transform's address
    InverseRotated translation;  // a^T * a.translation
    Vector4 result_translation;  // the stored translation
};
template<class Memory,class Frame> MultiplyResidue multiply_inverse(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum MultiplyAccess;
    const auto a=frame.first();
    Rotation rotation;
    rotation.column[2]=memory.load_vector(a_column_2,at(a,offsetof(HavokTransform,column[2])));
    rotation.column[0]=memory.load_vector(a_column_0,at(a,offsetof(HavokTransform,column[0])));
    rotation.column[1]=memory.load_vector(a_column_1,at(a,offsetof(HavokTransform,column[1])));
    const auto b=frame.second();
    // Rotate each of b's four vectors (three columns and the translation)
    // into out. Each row is stored before the next is read: out may overlap b.
    for(std::uint32_t row=0;row<4;++row) {
        const auto value=memory.load_vector(b_row,element(b,row,sizeof(Lanes)));
        memory.store_vector(out_row,element(out,row,sizeof(Lanes)),rotate_inverse(rotation,value).value);
    }
    // a is read again, because out may overlap it. The original loads column
    // 2, the translation, then columns 0 and 1.
    Rotation again;
    again.column[2]=memory.load_vector(a_column_2_again,at(a,offsetof(HavokTransform,column[2])));
    const auto a_translation_value=memory.load_vector(a_translation,at(a,offsetof(HavokTransform,translation)));
    again.column[0]=memory.load_vector(a_column_0_again,at(a,offsetof(HavokTransform,column[0])));
    again.column[1]=memory.load_vector(a_column_1_again,at(a,offsetof(HavokTransform,column[1])));
    const auto rotated_b_translation=memory.load_vector(out_translation,at(out,offsetof(HavokTransform,translation)));
    const auto rotated_a_translation=rotate_inverse(again,a_translation_value);
    const auto translation=sse::subtract(rotated_b_translation,rotated_a_translation.value);
    memory.store_vector(translation_store,at(out,offsetof(HavokTransform,translation)),translation);
    return {b,rotated_a_translation,translation};
}
} // namespace havok_rotation

// --------------------------------------------------------- contact records
// 303BA0 copies a contact record field by field. Two of the six vectors are
// a contact point (position, separating normal); 303800 tests the one at
// +0x50 against a probe, and 304130 selects records whose state byte is 1.
// The other fields' game meaning is not established.
namespace contact_record {
struct ContactPoint {
    Lanes position;
    Lanes separating_normal;
};
struct ContactRecord {
    ContactPoint contact;
    std::uint32_t word_20,word_24;
    std::uint8_t not_copied_28[8];  // not part of the copy
    Lanes vector_30,vector_40;
    ContactPoint second_contact;
    std::uint32_t word_70,word_74,word_78;
    std::uint8_t state;
    std::uint8_t byte_7d;
    std::uint8_t not_copied_7e[2];  // not part of the copy
};
static_assert(offsetof(ContactRecord,second_contact)==0x50 && offsetof(ContactRecord,state)==0x7c &&
              sizeof(ContactRecord)==0x80);

// The copied fields, in the original's order.
enum class Field : std::uint8_t {
    contact_position,contact_normal,word_20,word_24,vector_30,vector_40,
    second_position,second_normal,word_70,word_74,word_78,state,byte_7d
};
struct Access {
    Field field;
    bool store;
};
struct Residue {
    Vector4 last_vector;   // second_contact.separating_normal
    std::uint32_t word_78;
    std::uint8_t state,byte_7d;
};
// Each field is read and then written before the next is read, which
// matters when the two records overlap.
template<class Memory> Residue copy(Memory &memory,std::uint32_t destination,std::uint32_t source) {
    const auto vector_field=[&](Field field,std::size_t offset) {
        const auto value=memory.load_vector(Access{field,false},at(source,offset));
        memory.store_vector(Access{field,true},at(destination,offset),value);
        return value;
    };
    const auto word_field=[&](Field field,std::size_t offset) {
        const auto value=memory.load32(Access{field,false},at(source,offset));
        memory.store32(Access{field,true},at(destination,offset),value);
        return value;
    };
    const auto byte_field=[&](Field field,std::size_t offset) {
        const auto value=memory.load8(Access{field,false},at(source,offset));
        memory.store8(Access{field,true},at(destination,offset),value);
        return std::uint8_t(value);
    };
    using enum Field;
    Residue residue{};
    vector_field(contact_position,offsetof(ContactRecord,contact.position));
    vector_field(contact_normal,offsetof(ContactRecord,contact.separating_normal));
    word_field(word_20,offsetof(ContactRecord,word_20));
    word_field(word_24,offsetof(ContactRecord,word_24));
    vector_field(vector_30,offsetof(ContactRecord,vector_30));
    vector_field(vector_40,offsetof(ContactRecord,vector_40));
    vector_field(second_position,offsetof(ContactRecord,second_contact.position));
    residue.last_vector=vector_field(second_normal,offsetof(ContactRecord,second_contact.separating_normal));
    word_field(word_70,offsetof(ContactRecord,word_70));
    word_field(word_74,offsetof(ContactRecord,word_74));
    residue.word_78=word_field(word_78,offsetof(ContactRecord,word_78));
    residue.state=byte_field(state,offsetof(ContactRecord,state));
    residue.byte_7d=byte_field(byte_7d,offsetof(ContactRecord,byte_7d));
    return residue;
}
} // namespace contact_record

// ---------------------------------------------------- contact record arrays
// 303EE0, hkArray<ContactRecord>::expandOne: appends one record to an array of
// contact records and returns the new record's address. The record is a copy
// of a local whose state and byte 7D are cleared. The original leaves the
// local's other bytes uninitialized, so whatever the stack holds there is
// copied too. A full array first grows through 2E3250
// (hkArrayUtil::_reserveMore).
namespace contact_array {
struct HavokArray {
    std::uint32_t data;
    std::uint32_t size;
    std::uint32_t capacity_and_flags;  // bit 31: the array does not own its storage
};
inline constexpr std::uint32_t capacity_mask=0x7fffffffu;
enum class Access {size,clear_byte_7d,clear_state,capacity,grown_size,data,size_store,result_size,result_data};
struct Result {
    std::uint32_t record;     // the last record
    std::uint32_t index;      // the appended record's index
    std::uint32_t size,data;  // the array as re-read for the result
};
// Frame (the adapter's): local_record() is the local's address. reserve_more()
// and copy_record() are the two children (2E3250 and 303BA0); reserve_more()
// also receives the full size, which the original holds in registers there.
template<class Memory,class Frame> Result expand_one(Memory &memory,Frame &frame,std::uint32_t array) {
    using enum Access;
    using contact_record::ContactRecord;
    const auto local=frame.local_record();
    const auto count=memory.load32(size,at(array,offsetof(HavokArray,size)));
    memory.store8(clear_byte_7d,at(local,offsetof(ContactRecord,byte_7d)),0u);
    memory.store8(clear_state,at(local,offsetof(ContactRecord,state)),0u);
    const auto limit=memory.load32(capacity,at(array,offsetof(HavokArray,capacity_and_flags)))&capacity_mask;
    if(count==limit)frame.reserve_more(array,std::uint32_t(sizeof(ContactRecord)),count);
    // The array may have moved, so it is read again.
    const auto index=memory.load32(grown_size,at(array,offsetof(HavokArray,size)));
    const auto elements=memory.load32(data,at(array,offsetof(HavokArray,data)));
    memory.store32(size_store,at(array,offsetof(HavokArray,size)),index+1u);
    frame.copy_record(element(elements,index,sizeof(ContactRecord)),local);
    // The result re-reads the array: the address of its last record.
    const auto final_size=memory.load32(result_size,at(array,offsetof(HavokArray,size)));
    const auto final_data=memory.load32(result_data,at(array,offsetof(HavokArray,data)));
    return {element(final_data,final_size-1u,sizeof(ContactRecord)),index,final_size,final_data};
}
} // namespace contact_array

// ------------------------------------------------------ local shape buffers
// 303C70 and 303D00 construct and destroy an hkLocalArray of 256-byte shape
// buffers: an hkArray whose storage is a block of the thread memory's stack.
// The stack is a bump allocator (next free byte, bytes left). When a block
// does not fit, it comes from the thread memory's virtual allocate_stack()
// instead, and the destructor gives such blocks back through
// deallocate_stack().
namespace local_buffers {
namespace globals {
inline constexpr std::uint32_t thread_memory=0x48c938u;  // hkThreadMemory *
}
// The thread memory object: only the stack fields are used here.
struct ThreadMemory {
    std::uint32_t vtable;
    std::uint32_t not_used_4;
    std::uint32_t stack_next;    // the next free byte of the stack
    std::int32_t stack_left;     // the bytes left on the stack
    std::uint32_t not_used_10;
    // Not established. 303D00 gives a block back through deallocate_stack()
    // when it starts here, or when this is 0, instead of popping it.
    std::uint32_t stack_mark;
};
static_assert(offsetof(ThreadMemory,stack_next)==0x8 && offsetof(ThreadMemory,stack_left)==0xc &&
              offsetof(ThreadMemory,stack_mark)==0x14);
// The thread memory's virtual functions used here, by vtable offset.
enum class ThreadMemoryFunction : std::uint32_t {deallocate=0x14,allocate_stack=0x24,deallocate_stack=0x28};
// The memory class deallocate() receives for an array's storage.
inline constexpr std::uint32_t array_storage_class=0x12u;

// hkLocalArray<T>: an hkArray, the stack block and the capacity it was made
// with.
struct LocalArray {
    contact_array::HavokArray array;
    std::uint32_t stack_block;
    std::int32_t initial_capacity;
};
static_assert(offsetof(LocalArray,stack_block)==0xc && offsetof(LocalArray,initial_capacity)==0x10 &&
              sizeof(LocalArray)==0x14);
inline constexpr std::uint32_t shape_buffer_size=0x100u;
inline constexpr std::uint32_t storage_not_owned=0x80000000u;  // HavokArray::capacity_and_flags bit 31
// A stack block holds the buffers rounded up to 16 bytes (32-bit arithmetic).
constexpr std::uint32_t block_bytes(std::uint32_t capacity){return (capacity*shape_buffer_size+15u)&~15u;}

enum class Access {
    // construction
    clear_data,clear_size,clear_capacity,memory_pointer,stack_left,stack_next,stack_next_store,stack_left_store,
    data_store,block_store,capacity_store,initial_capacity_store,
    // destruction
    initial_capacity_read,block_read,memory_pointer_again,stack_mark,next_to_pop,left_to_pop,popped_next_store,
    popped_left_store,capacity_read,owner_memory_pointer,storage
};
struct Constructed {
    std::uint32_t block;  // the stack block
    std::uint32_t bytes;
    bool allocated;       // the block came from allocate_stack()
};

// 303C70, hkLocalArray(capacity): ECX's array starts empty and not owning
// storage, then receives a stack block for `capacity` buffers.
// Frame (the adapter's): capacity() is the argument; allocate_stack() calls the
// thread memory's virtual function and returns the block.
template<class Memory,class Frame> Constructed construct(Memory &memory,Frame &frame,std::uint32_t array) {
    using enum Access;
    using contact_array::HavokArray;
    // The hkArray base: no storage yet.
    memory.store32(clear_data,at(array,offsetof(HavokArray,data)),0u);
    memory.store32(clear_size,at(array,offsetof(HavokArray,size)),0u);
    memory.store32(clear_capacity,at(array,offsetof(HavokArray,capacity_and_flags)),storage_not_owned);
    const auto capacity=frame.capacity();
    const auto thread=memory.load32(memory_pointer,globals::thread_memory);
    const auto left=memory.load32(Access::stack_left,at(thread,offsetof(ThreadMemory,stack_left)));
    const auto bytes=block_bytes(capacity);
    Constructed made{0u,bytes,false};
    // The comparison is signed, as the original's.
    if(std::int32_t(bytes)>std::int32_t(left)) {
        made.block=frame.allocate_stack(thread,bytes);
        made.allocated=true;
    } else {
        // Bump the stack.
        made.block=memory.load32(Access::stack_next,at(thread,offsetof(ThreadMemory,stack_next)));
        memory.store32(stack_next_store,at(thread,offsetof(ThreadMemory,stack_next)),made.block+bytes);
        memory.store32(stack_left_store,at(thread,offsetof(ThreadMemory,stack_left)),left-bytes);
    }
    memory.store32(data_store,at(array,offsetof(HavokArray,data)),made.block);
    memory.store32(block_store,at(array,offsetof(LocalArray,stack_block)),made.block);
    memory.store32(capacity_store,at(array,offsetof(HavokArray,capacity_and_flags)),capacity|storage_not_owned);
    memory.store32(initial_capacity_store,at(array,offsetof(LocalArray,initial_capacity)),capacity);
    return made;
}

struct Destroyed {
    std::uint32_t bytes;           // the stack block's size
    std::uint32_t capacity;        // the array's capacity and flags, as read
    bool released_virtually;       // deallocate_stack()
    std::optional<std::uint32_t> mark;  // ThreadMemory::stack_mark, when read
    std::uint32_t popped_next,popped_left;
};

// 303D00, ~hkLocalArray: gives the stack block back, then (~hkArray) frees the
// array's storage if it owns it.
// Frame (the adapter's): deallocate_stack() and deallocate() call the thread
// memory's virtual functions.
template<class Memory,class Frame> Destroyed destroy(Memory &memory,Frame &frame,std::uint32_t array) {
    using enum Access;
    using contact_array::HavokArray;
    const auto initial=memory.load32(initial_capacity_read,at(array,offsetof(LocalArray,initial_capacity)));
    const auto block_address=memory.load32(block_read,at(array,offsetof(LocalArray,stack_block)));
    const auto thread=memory.load32(memory_pointer_again,globals::thread_memory);
    Destroyed done{block_bytes(initial),0u,false,std::nullopt,0u,0u};
    // An empty block is always popped. Otherwise a block at the stack mark,
    // or any block while there is no mark, goes back through the virtual
    // function.
    if(done.bytes) {
        done.mark=memory.load32(stack_mark,at(thread,offsetof(ThreadMemory,stack_mark)));
        done.released_virtually=block_address==*done.mark || *done.mark==0u;
    }
    if(done.released_virtually) frame.deallocate_stack(thread,block_address,done.bytes);
    else {
        done.popped_next=memory.load32(next_to_pop,at(thread,offsetof(ThreadMemory,stack_next)))-done.bytes;
        done.popped_left=memory.load32(left_to_pop,at(thread,offsetof(ThreadMemory,stack_left)))+done.bytes;
        memory.store32(popped_next_store,at(thread,offsetof(ThreadMemory,stack_next)),done.popped_next);
        memory.store32(popped_left_store,at(thread,offsetof(ThreadMemory,stack_left)),done.popped_left);
    }
    // ~hkArray: storage the array owns goes back to the thread memory.
    done.capacity=memory.load32(capacity_read,at(array,offsetof(HavokArray,capacity_and_flags)));
    if(!(done.capacity&storage_not_owned)) {
        const auto owner=memory.load32(owner_memory_pointer,globals::thread_memory);
        const auto data=memory.load32(storage,at(array,offsetof(HavokArray,data)));
        frame.deallocate(owner,data,(done.capacity&contact_array::capacity_mask)*shape_buffer_size,array_storage_class);
    }
    return done;
}
} // namespace local_buffers

// ------------------------------------------------------------ x87 values
// Values the original holds on the x87 stack: extended precision, converted
// and rounded exactly as its FLD, FSUB and FSTP do (FLD quiets a signalling
// NaN; FSTP rounds under the x87 control word). Comparisons are x87
// comparisons, so MXCSR's denormals-are-zero does not apply.
namespace x87 {
using Extended=long double;
inline Extended load(float value){volatile Extended extended=static_cast<Extended>(value);return extended;}
inline float store(Extended value){volatile float rounded=static_cast<float>(value);return rounded;}
// FSUB m32 and FCOM m32 take their operand as FLD converts it (a signalling
// NaN is quieted first; then two NaNs keep the larger payload), so the
// conversion must not fold into the instruction's memory operand.
inline Extended subtract(Extended a,float b){const auto operand=load(b);volatile Extended difference=a-operand;return difference;}
// FCOM m32 tested with TEST AH,41: neither below nor equal, and ordered.
inline bool greater(Extended a,float b){const auto operand=load(b);return a>operand;}
// FMUL ST(0),ST(i): the product at the x87 stack's precision.
inline Extended multiply(Extended a,Extended b){volatile Extended product=a*b;return product;}
} // namespace x87

// ------------------------------------------------- welded collection agent
// 304350: the collision agent of a body whose shape wraps a collection of
// child shapes (hkpCollisionAgent::processCollision). It keeps one child agent
// per child shape and welds the children's contacts, so that a body sliding
// across the seams between child shapes (for example the triangles of a
// collision mesh) meets one smooth surface:
//   1. 2F36F0 updates the child agents for the children near body A;
//   2. each child with an agent either gathers its closest contact into a
//      local array of contact records (when the game's weld predicate accepts
//      the pair of shape types), or processes its collision directly;
//   3. the gathered records are sorted by distance (selection sort); each
//      record with a contact is welded (304130) against the records after it;
//   4. every point each record's child agent already holds is welded against
//      the records too;
//   5. each record is emitted: 30CD00 confirms the contact of the records the
//      welds kept, and 30CF40 reports every record to the output.
// It also writes the profiler's begin and end timer events, and gives back
// the shape buffers and the records' storage.
namespace collection_agent {
namespace globals {
inline constexpr std::uint32_t monitor_next=0x48c974u;      // hkMonitorStream: the next event slot
inline constexpr std::uint32_t monitor_end=0x48c978u;       //                  the end of its buffer
inline constexpr std::uint32_t begin_timer_name=0x41d05cu;  // the timer names the events carry
inline constexpr std::uint32_t end_timer_name=0x41c394u;
inline constexpr std::uint32_t weld_predicate=0x46e46cu;    // a function pointer the game installs
inline constexpr std::uint32_t null_agent=0x48cdc4u;        // hkpNullAgent (30BAD0 returns it)
inline constexpr std::uint32_t emit_distance_bias=0x46a180u;  // float; its meaning is not established
inline constexpr std::uint32_t candidate_filter=0x2f3140u;  // the function 2F36F0 receives
}

// hkpCdBody: one shape of a body's shape hierarchy.
struct CdBody {
    std::uint32_t shape;
    std::uint32_t shape_key;
    std::uint32_t motion;  // the body's transform is at +20 of it (not established why)
    std::uint32_t parent;  // the CdBody this one is a child of
};
static_assert(sizeof(CdBody)==0x10);
inline constexpr std::uint32_t motion_rotation=0x20u;

// Body B's shape, and the collection shape it wraps, each hold the next level
// at +C: body B's shape -> the collection shape -> the object that answers
// getChildShape(). The exact Havok classes are not established.
struct ShapeHeader {
    std::uint32_t vtable;
    std::uint32_t not_used_4,not_used_8;
    std::uint32_t inner;
};
static_assert(offsetof(ShapeHeader,inner)==0xc);
// Virtual functions, by vtable offset.
enum class Virtual : std::uint32_t {
    shape_type=0x14,          // hkpShape::getType()
    child_shape=0x34,         // getChildShape(key, buffer) -> shape
    process_collision=0x10,   // child agent: processCollision(bodyA, bodyB, input, output)
    closest_contact=0x20,     // child agent: (result hkBool *, bodyA, bodyB, input, contact *)
};

// The agent (this): an hkpCollisionAgent and its child agents.
struct Agent {
    std::uint32_t vtable;
    std::uint32_t reference_count;
    std::uint32_t contact_manager;
    contact_array::HavokArray children;  // ChildEntry[]
};
static_assert(offsetof(Agent,contact_manager)==0x8 && offsetof(Agent,children)==0xc);
struct ChildEntry {
    std::uint32_t shape_key;
    std::uint32_t not_used_4;
    std::uint32_t agent;      // the child agent, or the null agent
};
static_assert(sizeof(ChildEntry)==12);

// A child agent, as far as this owner reads it: a byte that decides the
// record's state, and the contact points it already holds (+18: a point set
// that starts with the count; the points from +40, 0x30 bytes each, and the
// emitters receive +30).
struct ChildAgent {
    std::uint8_t not_used[0x15];
    std::uint8_t kind;              // 1 or 2 marks the record's state
    std::uint8_t not_used_16[2];
    std::int32_t point_count;       // +18
    std::uint8_t not_used_1c[0x14];
    std::uint8_t point_header[0x10];  // +30
    // ChildPoint points[] at +40
};
static_assert(offsetof(ChildAgent,kind)==0x15 && offsetof(ChildAgent,point_count)==0x18 && sizeof(ChildAgent)==0x40);
inline constexpr std::uint32_t point_set=0x18u,point_list=0x30u;  // the addresses the emitters receive
struct ChildPoint {
    Lanes position;
    Lanes normal;      // its w (+1C) is the distance
    Lanes not_used_20;
};
static_assert(sizeof(ChildPoint)==0x30);

// The input's fields used here: the dispatcher (+0), whose shape types the
// weld predicate receives, and the dword at +C that each weld receives by
// value (not established).
struct ProcessInput {
    std::uint32_t dispatcher;
    std::uint32_t not_used_4,not_used_8;
    std::uint32_t weld_parameter;
};
static_assert(offsetof(ProcessInput,weld_parameter)==0xc);

// The records are the contact_record::ContactRecord layout. Here:
//   contact (+0..+4F, except +28..+2F): the closest contact the child agent
//     returned (position, normal, distance at +20, the dword at +24, and the
//     vectors at +30 and +40);
//   second_contact: the contact as the welds compare it: the +30 vector with
//     the distance in its w, and the +40 vector rotated into body B's frame;
//   word_70, word_74, word_78: the child agent, the child shape, its key;
//   state: 1 when the child agent's kind is 1 or 2;
//   byte_7d: the child agent found a contact.
using contact_record::ContactRecord;
inline constexpr std::uint32_t inline_records=4u;  // hkInplaceArray<ContactRecord,4>
inline constexpr std::uint32_t record_size=sizeof(ContactRecord);
// The original addresses the record it just appended as data + size*80 - 80.
constexpr std::uint32_t last_record(std::uint32_t data,std::uint32_t size){return (size<<7)+data-record_size;}

// The addresses of the arguments, and of the locals the original keeps in its
// frame (the adapter supplies them). Several locals share a slot in turn.
struct Locals {
    std::uint32_t body_a,body_b,input,output;  // the arguments
    std::uint32_t agent;            // this, kept for the reloads
    std::uint32_t begin_time,end_time;  // the time stamp, as the timer macro keeps it
    std::uint32_t wrapper_body;     // CdBody: the collection shape below body B
    std::uint32_t child_body;       // CdBody: a child shape below body B
    std::uint32_t point_contact;    // a child agent's point in the weld's form (child_body's slot)
    std::uint32_t buffers;          // hkLocalArray of shape buffers (303C70)
    std::uint32_t rotated_normal;   // 2E47B0's result
    std::uint32_t contact;          // the closest contact a child agent writes
    std::uint32_t contact_found;    // hkBool: the child agent found a contact
    std::uint32_t records;          // hkInplaceArray<ContactRecord,4>
    std::uint32_t record_storage;   // its four inline records
    // loop state
    std::uint32_t collection_shape,child_index,entry_offset,buffer_offset;  // gather
    std::uint32_t sort_index,sort_offset;                                    // sort
    std::uint32_t swapped_key,swapped_state,swapped_has_contact;             // the swap's saved fields
    std::uint32_t record_count,point_offset,points_left;                     // the child points
    std::uint32_t emit_offset,separation;                                    // emission
};

enum class Access {
    // the profiler's timer events
    begin_next,begin_end,begin_name,begin_time_store,begin_time_load,begin_time,begin_advance,
    end_next,end_end,end_name,end_time_store,end_time_load,end_time,end_advance,
    // the collection's body and the child agents' update
    body_b_argument,body_b_motion,body_b_shape,wrapper_motion,wrapper_parent,collection_shape_read,body_b_key,
    wrapper_shape,input_argument,wrapper_key,body_a_argument,
    // the local records and shape buffers
    records_data_init,records_size_init,records_capacity_init,child_count,child_count_again,body_b_motion_again,
    wrapper_shape_again,buffers_size,child_body_parent,child_body_motion,collection_shape_keep,child_index_keep,
    buffer_offset_keep,entry_offset_keep,
    // per child
    children_data,entry_agent,collection_shape_reload,collection_inner,buffers_data,buffer_offset_reload,child_key,
    body_a_reload,body_a_shape,child_key_again,child_body_shape,child_body_key,input_reload,dispatcher,verdict,
    gather_input,gathered_agent,output_argument,processed_agent,agent_reload,
    child_index_reload,entry_offset_reload,buffer_offset_next,children_size,child_index_store,entry_offset_store,
    buffer_offset_store,
    // per gathered child: the appended record
    agent_size,agent_data,agent_store,shape_size,shape_data,record_key,shape_store,key_size,key_data,key_store,
    has_size,found,has_data,has_store,found_again,
    contact_size,contact_data,contact_position,contact_position_store,contact_normal,contact_normal_store,
    contact_distance,contact_distance_store,contact_word_24,contact_word_24_store,contact_vector_30,
    contact_vector_30_store,contact_vector_40,contact_vector_40_store,body_motion,
    welded_distance,welded_size,welded_data,welded_vector,welded_vector_store,welded_normal,welded_normal_store,
    welded_distance_store,agent_kind,state_size,state_data,state_store,
    // sort and weld
    sort_count,sort_index_keep,sort_offset_keep,sort_index_reload,sort_data,sort_offset_reload,first_distance,
    group_distance,group_distance_again,rest_data,rest_distance,rest_distance_again,swap_data,
    swap_first_position,swap_best_vector_50,swap_best_distance,swap_best_vector_60,swap_best_word_24,
    swap_best_shape_key,swap_best_position,swap_best_normal,swap_best_vector_30,swap_best_vector_40,swap_best_agent,
    swap_best_child_shape,swap_to_best_position,swap_first_normal,swap_to_best_normal,swapped_key_keep,
    swap_best_state,swapped_state_keep,swap_best_has,swapped_has_keep,
    swap_first_distance,swap_to_best_distance,swap_first_word_24,swap_to_best_word_24,
    swap_first_vector_30,swap_to_best_vector_30,swap_first_vector_40,swap_to_best_vector_40,
    swap_first_vector_50,swap_to_best_vector_50,swap_first_vector_60,swap_to_best_vector_60,
    swap_first_agent,swap_to_best_agent,swap_first_child_shape,swap_to_best_child_shape,
    swap_first_shape_key,swap_to_best_shape_key,swap_first_state,swap_to_best_state,swap_first_has,swap_to_best_has,
    swap_to_first_distance,swapped_key_reload,swapped_state_reload,swap_to_first_word_24,swap_to_first_position,
    swap_to_first_normal,swap_to_first_vector_30,swap_to_first_vector_40,swap_to_first_vector_50,
    swap_to_first_vector_60,swap_to_first_shape_key,swapped_has_reload,swap_to_first_agent,sorted_offset_reload,
    swap_to_first_child_shape,swap_to_first_state,swap_to_first_has,
    sorted_data,sorted_has_contact,weld_input,weld_parameter,weld_index_reload,next_index_reload,
    sort_index_store,sort_offset_store,sorted_size,record_count_keep,
    // the child agents' own points
    point_offset_keep,points_left_keep,points_data,point_offset_reload,points_agent,point_count,
    point_body_b,point_distance,point_motion,point_position,point_position_store,point_distance_store,
    point_input,point_weld_parameter,point_normal,point_normal_store,point_offset_next,points_left_next,
    point_offset_store,points_left_store,
    // emission
    record_count_reload,emit_offset_keep,emit_body_b,emit_motion,emit_offset_reload,emit_parent,emit_data,
    emit_motion_store,emit_shape_key,emit_child_shape,emit_shape_store,emit_key_store,emit_agent,emit_has_contact,
    distance_data,distance_offset_reload,emit_distance,emit_agent_reload,contact_manager,distance_bias,
    separation_store,separation_load,emit_offset_next,record_count_next,emit_offset_store,
    // the records' storage
    records_capacity,thread_memory,records_storage
};

// Frame (the adapter's) provides the locals (locals()), the time stamp
// counter, the children and the virtual calls, and the inline 30BAD0. Their
// exact ABI is the adapter's.
struct Result {
    std::uint32_t welded_count;      // the record count the welds leave
    std::uint32_t record_count;      // the records' size after the sort
    std::uint32_t records_capacity;  // as read at the end
};

// HK_TIMER_BEGIN/END: when the profiler's stream has room (an unsigned
// comparison), the event is the timer's name and the low half of the time
// stamp counter (which the macro keeps in a local), and the stream advances
// by 12 bytes.
struct TimerAccesses {Access next,end,name,time_store,time_load,time,advance;};
template<class Memory,class Frame> void timer_event(Memory &memory,Frame &frame,std::uint32_t name,std::uint32_t time_local,
    const TimerAccesses &a) {
    const auto next=memory.load32(a.next,globals::monitor_next);
    const auto end=memory.load32(a.end,globals::monitor_end);
    if(next>=end)return;
    memory.store32(a.name,next,name);
    memory.store32(a.time_store,time_local,frame.time_stamp_counter());
    memory.store32(a.time,at(next,4),memory.load32(a.time_load,time_local));
    memory.store32(a.advance,globals::monitor_next,next+12u);
}

// The appended record takes the child agent's contact.
template<class Memory,class Frame> void record_contact(Memory &memory,Frame &frame,const Locals &local,std::uint32_t child_agent) {
    using enum Access;
    using contact_array::HavokArray;
    const auto contact=local.contact;
    const auto size_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,size)));};
    const auto data_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,data)));};
    auto size=size_of(contact_size);
    auto record=last_record(data_of(contact_data),size);
    const auto vector_field=[&](Access load_role,Access store_role,std::size_t offset) {
        memory.store_vector(store_role,at(record,offset),memory.load_vector(load_role,at(contact,offset)));
    };
    const auto word_field=[&](Access load_role,Access store_role,std::size_t offset) {
        memory.store32(store_role,at(record,offset),memory.load32(load_role,at(contact,offset)));
    };
    vector_field(contact_position,contact_position_store,offsetof(ContactRecord,contact.position));
    vector_field(contact_normal,contact_normal_store,offsetof(ContactRecord,contact.separating_normal));
    word_field(contact_distance,contact_distance_store,offsetof(ContactRecord,word_20));
    word_field(contact_word_24,contact_word_24_store,offsetof(ContactRecord,word_24));
    vector_field(contact_vector_30,contact_vector_30_store,offsetof(ContactRecord,vector_30));
    vector_field(contact_vector_40,contact_vector_40_store,offsetof(ContactRecord,vector_40));
    // The weld's form of the contact: the +40 vector rotated into body B's
    // frame (2E47B0), and the +30 vector with the distance (through the x87
    // stack) in its w.
    const auto motion=memory.load32(body_motion,at(local.child_body,offsetof(CdBody,motion)));
    frame.rotate_contact_normal(local.rotated_normal,motion+motion_rotation,at(contact,offsetof(ContactRecord,vector_40)));
    const auto distance=x87::load(memory.load_float(welded_distance,at(contact,offsetof(ContactRecord,word_20))));
    size=size_of(welded_size);
    record=last_record(data_of(welded_data),size);
    memory.store_vector(welded_vector_store,at(record,offsetof(ContactRecord,second_contact.position)),
        memory.load_vector(welded_vector,at(contact,offsetof(ContactRecord,vector_30))));
    memory.store_vector(welded_normal_store,at(record,offsetof(ContactRecord,second_contact.separating_normal)),
        memory.load_vector(welded_normal,local.rotated_normal));
    memory.store_float(welded_distance_store,at(record,offsetof(ContactRecord,second_contact.position)+12u),x87::store(distance));
    // The state: the child agent's kind is 1 or 2.
    const auto kind=memory.load8(agent_kind,at(child_agent,offsetof(ChildAgent,kind)));
    size=size_of(state_size);
    record=last_record(data_of(state_data),size);
    memory.store8(state_store,at(record,offsetof(ContactRecord,state)),kind==1u || kind==2u?1u:0u);
}

// 1-2. Update the child agents, then gather or process each child. Returns
// this, as last reloaded.
template<class Memory,class Frame> std::uint32_t gather(Memory &memory,Frame &frame,const Locals &local,std::uint32_t agent) {
    using enum Access;
    using contact_array::HavokArray;
    // The collection's body: body B, with its shape replaced by the
    // collection shape body B's shape wraps.
    const auto body_b=memory.load32(body_b_argument,local.body_b);
    const auto motion=memory.load32(body_b_motion,at(body_b,offsetof(CdBody,motion)));
    const auto shape=memory.load32(body_b_shape,at(body_b,offsetof(CdBody,shape)));
    const auto wrapper=local.wrapper_body;
    memory.store32(wrapper_motion,at(wrapper,offsetof(CdBody,motion)),motion);
    memory.store32(wrapper_parent,at(wrapper,offsetof(CdBody,parent)),body_b);
    const auto collection=memory.load32(collection_shape_read,at(shape,offsetof(ShapeHeader,inner)));
    const auto key=memory.load32(body_b_key,at(body_b,offsetof(CdBody,shape_key)));
    memory.store32(Access::wrapper_shape,at(wrapper,offsetof(CdBody,shape)),collection);
    const auto input=memory.load32(input_argument,local.input);
    memory.store32(wrapper_key,at(wrapper,offsetof(CdBody,shape_key)),key);
    frame.update_child_agents(memory.load32(body_a_argument,local.body_a),wrapper,input);  // 2F36F0

    // The records: an hkInplaceArray of four, in the frame.
    memory.store32(records_data_init,at(local.records,offsetof(HavokArray,data)),local.record_storage);
    memory.store32(records_size_init,at(local.records,offsetof(HavokArray,size)),0u);
    memory.store32(records_capacity_init,at(local.records,offsetof(HavokArray,capacity_and_flags)),
        inline_records|local_buffers::storage_not_owned);
    // One shape buffer per child (303C70); the array's size is the count.
    frame.construct_buffers(memory.load32(child_count,at(agent,offsetof(Agent,children.size))));
    const auto count=memory.load32(child_count_again,at(agent,offsetof(Agent,children.size)));
    const auto child_motion=memory.load32(body_b_motion_again,at(body_b,offsetof(CdBody,motion)));
    const auto wrapped=memory.load32(wrapper_shape_again,at(wrapper,offsetof(CdBody,shape)));
    memory.store32(buffers_size,at(local.buffers,offsetof(HavokArray,size)),count);
    // Each child's body: a child shape below body B.
    memory.store32(child_body_parent,at(local.child_body,offsetof(CdBody,parent)),body_b);
    memory.store32(child_body_motion,at(local.child_body,offsetof(CdBody,motion)),child_motion);
    memory.store32(collection_shape_keep,local.collection_shape,wrapped);
    memory.store32(child_index_keep,local.child_index,0u);
    if(std::int32_t(count)<=0)return agent;

    // The original walks the children with three counters it keeps in its
    // frame: the index, the entry's offset and the buffer's offset. It holds
    // the entry's offset in a register for the pass.
    memory.store32(buffer_offset_keep,local.buffer_offset,0u);
    memory.store32(entry_offset_keep,local.entry_offset,0u);
    std::uint32_t entry_offset=0;
    for(;;) {
        const auto entry=memory.load32(children_data,at(agent,offsetof(Agent,children.data)))+entry_offset;
        const auto null=frame.null_agent();  // 30BAD0
        if(memory.load32(entry_agent,at(entry,offsetof(ChildEntry,agent)))!=null) {
            const auto collection_shape=memory.load32(collection_shape_reload,local.collection_shape);
            const auto inner=memory.load32(collection_inner,at(collection_shape,offsetof(ShapeHeader,inner)));
            const auto buffer=memory.load32(buffers_data,at(local.buffers,offsetof(HavokArray,data)))+
                memory.load32(buffer_offset_reload,local.buffer_offset);
            const auto child=frame.child_shape(inner,memory.load32(child_key,at(entry,offsetof(ChildEntry,shape_key))),buffer);
            const auto body_a=memory.load32(body_a_reload,local.body_a);
            const auto a_shape=memory.load32(body_a_shape,at(body_a,offsetof(CdBody,shape)));
            const auto key_again=memory.load32(child_key_again,at(entry,offsetof(ChildEntry,shape_key)));
            memory.store32(child_body_shape,at(local.child_body,offsetof(CdBody,shape)),child);
            memory.store32(child_body_key,at(local.child_body,offsetof(CdBody,shape_key)),key_again);
            const auto type_b=frame.child_shape_type(child);
            const auto type_a=frame.body_shape_type(a_shape);
            const auto process_input=memory.load32(input_reload,local.input);
            const auto dispatcher=memory.load32(Access::dispatcher,at(process_input,offsetof(ProcessInput,dispatcher)));
            // The weld predicate returns its hkBool through the pointer it
            // returns.
            if(memory.load8(verdict,frame.weld_predicate(dispatcher,type_a,type_b))) {
                // Gather: the child agent writes its closest contact into the
                // local contact and says whether it found one.
                const auto gather_input_value=memory.load32(gather_input,local.input);
                const auto child_agent_address=memory.load32(gathered_agent,at(entry,offsetof(ChildEntry,agent)));
                frame.closest_contact(child_agent_address,gather_input_value);
                frame.append_record();  // 303EE0
                // Each field re-reads the array, as the original does.
                const auto size_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,size)));};
                const auto data_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,data)));};
                auto size=size_of(agent_size);
                memory.store32(agent_store,at(last_record(data_of(agent_data),size),offsetof(ContactRecord,word_70)),child_agent_address);
                size=size_of(shape_size);
                auto data=data_of(shape_data);
                const auto shape_key=memory.load32(record_key,at(entry,offsetof(ChildEntry,shape_key)));
                memory.store32(shape_store,at(last_record(data,size),offsetof(ContactRecord,word_74)),child);
                size=size_of(key_size);
                memory.store32(key_store,at(last_record(data_of(key_data),size),offsetof(ContactRecord,word_78)),shape_key);
                size=size_of(has_size);
                const auto hit=memory.load8(found,local.contact_found);
                memory.store8(has_store,at(last_record(data_of(has_data),size),offsetof(ContactRecord,byte_7d)),hit);
                if(memory.load8(found_again,local.contact_found))record_contact(memory,frame,local,child_agent_address);
            } else {
                const auto output=memory.load32(output_argument,local.output);
                frame.process_child(memory.load32(processed_agent,at(entry,offsetof(ChildEntry,agent))),output);
            }
            agent=memory.load32(agent_reload,local.agent);
        }
        // The next child: the counters come back from the frame.
        const auto index=memory.load32(child_index_reload,local.child_index)+1u;
        entry_offset=memory.load32(entry_offset_reload,local.entry_offset)+std::uint32_t(sizeof(ChildEntry));
        const auto buffer_offset=memory.load32(buffer_offset_next,local.buffer_offset)+local_buffers::shape_buffer_size;
        const auto size=memory.load32(children_size,at(agent,offsetof(Agent,children.size)));
        memory.store32(child_index_store,local.child_index,index);
        memory.store32(entry_offset_store,local.entry_offset,entry_offset);
        memory.store32(buffer_offset_store,local.buffer_offset,buffer_offset);
        if(!(std::int32_t(index)<std::int32_t(size)))return agent;
    }
}

// Exchange two records. The original reads the closest record's fields that
// the copy overwrites first (six vectors, the distance and +24 through the
// x87 stack, three words, and the two bytes through its frame), copies the
// first record over it field by field, then writes the saved fields into the
// first record. With best == first this rewrites the record in place.
// Returns the first record's offset, which the original re-reads midway.
template<class Memory> std::uint32_t swap_records(Memory &memory,const Locals &local,std::uint32_t first,std::uint32_t best) {
    using enum Access;
    const auto first_position=memory.load_vector(swap_first_position,at(first,offsetof(ContactRecord,contact.position)));
    const auto best_50=memory.load_vector(swap_best_vector_50,at(best,offsetof(ContactRecord,second_contact.position)));
    const auto best_distance=x87::load(memory.load_float(swap_best_distance,at(best,offsetof(ContactRecord,word_20))));
    const auto best_60=memory.load_vector(swap_best_vector_60,at(best,offsetof(ContactRecord,second_contact.separating_normal)));
    const auto best_24=x87::load(memory.load_float(swap_best_word_24,at(best,offsetof(ContactRecord,word_24))));
    const auto best_key=memory.load32(swap_best_shape_key,at(best,offsetof(ContactRecord,word_78)));
    const auto best_position=memory.load_vector(swap_best_position,at(best,offsetof(ContactRecord,contact.position)));
    const auto best_normal=memory.load_vector(swap_best_normal,at(best,offsetof(ContactRecord,contact.separating_normal)));
    const auto best_30=memory.load_vector(swap_best_vector_30,at(best,offsetof(ContactRecord,vector_30)));
    const auto best_40=memory.load_vector(swap_best_vector_40,at(best,offsetof(ContactRecord,vector_40)));
    const auto best_agent=memory.load32(swap_best_agent,at(best,offsetof(ContactRecord,word_70)));
    const auto best_child_shape=memory.load32(swap_best_child_shape,at(best,offsetof(ContactRecord,word_74)));
    memory.store_vector(swap_to_best_position,at(best,offsetof(ContactRecord,contact.position)),first_position);
    memory.store_vector(swap_to_best_normal,at(best,offsetof(ContactRecord,contact.separating_normal)),
        memory.load_vector(swap_first_normal,at(first,offsetof(ContactRecord,contact.separating_normal))));
    memory.store32(swapped_key_keep,local.swapped_key,best_key);
    memory.store8(swapped_state_keep,local.swapped_state,memory.load8(swap_best_state,at(best,offsetof(ContactRecord,state))));
    memory.store8(swapped_has_keep,local.swapped_has_contact,memory.load8(swap_best_has,at(best,offsetof(ContactRecord,byte_7d))));
    const auto copy_word=[&](Access load_role,Access store_role,std::size_t offset) {
        memory.store32(store_role,at(best,offset),memory.load32(load_role,at(first,offset)));
    };
    const auto copy_vector=[&](Access load_role,Access store_role,std::size_t offset) {
        memory.store_vector(store_role,at(best,offset),memory.load_vector(load_role,at(first,offset)));
    };
    const auto copy_byte=[&](Access load_role,Access store_role,std::size_t offset) {
        memory.store8(store_role,at(best,offset),memory.load8(load_role,at(first,offset)));
    };
    copy_word(swap_first_distance,swap_to_best_distance,offsetof(ContactRecord,word_20));
    copy_word(swap_first_word_24,swap_to_best_word_24,offsetof(ContactRecord,word_24));
    copy_vector(swap_first_vector_30,swap_to_best_vector_30,offsetof(ContactRecord,vector_30));
    copy_vector(swap_first_vector_40,swap_to_best_vector_40,offsetof(ContactRecord,vector_40));
    copy_vector(swap_first_vector_50,swap_to_best_vector_50,offsetof(ContactRecord,second_contact.position));
    copy_vector(swap_first_vector_60,swap_to_best_vector_60,offsetof(ContactRecord,second_contact.separating_normal));
    copy_word(swap_first_agent,swap_to_best_agent,offsetof(ContactRecord,word_70));
    copy_word(swap_first_child_shape,swap_to_best_child_shape,offsetof(ContactRecord,word_74));
    copy_word(swap_first_shape_key,swap_to_best_shape_key,offsetof(ContactRecord,word_78));
    copy_byte(swap_first_state,swap_to_best_state,offsetof(ContactRecord,state));
    copy_byte(swap_first_has,swap_to_best_has,offsetof(ContactRecord,byte_7d));
    // The saved fields go to the first record; the two floats are rounded
    // back from the x87 stack.
    memory.store_float(swap_to_first_distance,at(first,offsetof(ContactRecord,word_20)),x87::store(best_distance));
    const auto key=memory.load32(swapped_key_reload,local.swapped_key);
    const auto state=memory.load8(swapped_state_reload,local.swapped_state);
    memory.store_float(swap_to_first_word_24,at(first,offsetof(ContactRecord,word_24)),x87::store(best_24));
    memory.store_vector(swap_to_first_position,at(first,offsetof(ContactRecord,contact.position)),best_position);
    memory.store_vector(swap_to_first_normal,at(first,offsetof(ContactRecord,contact.separating_normal)),best_normal);
    memory.store_vector(swap_to_first_vector_30,at(first,offsetof(ContactRecord,vector_30)),best_30);
    memory.store_vector(swap_to_first_vector_40,at(first,offsetof(ContactRecord,vector_40)),best_40);
    memory.store_vector(swap_to_first_vector_50,at(first,offsetof(ContactRecord,second_contact.position)),best_50);
    memory.store_vector(swap_to_first_vector_60,at(first,offsetof(ContactRecord,second_contact.separating_normal)),best_60);
    memory.store32(swap_to_first_shape_key,at(first,offsetof(ContactRecord,word_78)),key);
    const auto has_contact=memory.load8(swapped_has_reload,local.swapped_has_contact);
    memory.store32(swap_to_first_agent,at(first,offsetof(ContactRecord,word_70)),best_agent);
    const auto offset=memory.load32(sorted_offset_reload,local.sort_offset);
    memory.store32(swap_to_first_child_shape,at(first,offsetof(ContactRecord,word_74)),best_child_shape);
    memory.store8(swap_to_first_state,at(first,offsetof(ContactRecord,state)),state);
    memory.store8(swap_to_first_has,at(first,offsetof(ContactRecord,byte_7d)),has_contact);
    return offset;
}

// 3. Sort the records by distance (selection sort) and weld each contact
// against the records after it.
struct Sorted {
    std::uint32_t n;      // the record count the welds leave
    std::uint32_t count;  // the records' size afterwards
};
template<class Memory,class Frame> Sorted sort_and_weld(Memory &memory,Frame &frame,const Locals &local) {
    using enum Access;
    using contact_array::HavokArray;
    const auto data_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,data)));};
    auto n=memory.load32(sort_count,at(local.records,offsetof(HavokArray,size)));
    memory.store32(sort_index_keep,local.sort_index,0u);
    if(std::int32_t(n)<=0)return {n,n};
    memory.store32(sort_offset_keep,local.sort_offset,0u);
    // The original keeps the index and the record's offset in its frame; the
    // first pass uses the index it holds in a register.
    std::uint32_t index=0;
    for(bool first_pass=true;;first_pass=false) {
        if(!first_pass)index=memory.load32(sort_index_reload,local.sort_index);
        const auto first=data_of(sort_data)+memory.load32(sort_offset_reload,local.sort_offset);
        // The closest record from here on: a candidate replaces it only when
        // strictly closer (an x87 comparison: NaN never replaces), and is read
        // again to be held. The closest value is dropped after the search.
        auto closest=x87::load(memory.load_float(first_distance,at(first,offsetof(ContactRecord,word_20))));
        std::uint32_t best=index,candidate=index;
        // Four at a time while four or more remain, addressed from the first
        // record; then one at a time from the array's data, re-read.
        if(std::int32_t(n-index)>=4) {
            do {
                for(std::uint32_t lane=0;lane<4;++lane) {
                    const auto address=first+offsetof(ContactRecord,word_20)+(candidate-index+lane)*record_size;
                    if(x87::greater(closest,memory.load_float(group_distance,address))) {
                        best=candidate+lane;
                        closest=x87::load(memory.load_float(group_distance_again,address));
                    }
                }
                candidate+=4u;
            } while(std::int32_t(candidate)<std::int32_t(n-3u));
        }
        if(std::int32_t(candidate)<std::int32_t(n)) {
            auto address=data_of(rest_data)+(candidate<<7)+offsetof(ContactRecord,word_20);
            do {
                if(x87::greater(closest,memory.load_float(rest_distance,address))) {
                    best=candidate;
                    closest=x87::load(memory.load_float(rest_distance_again,address));
                }
                ++candidate;address+=record_size;
            } while(std::int32_t(candidate)<std::int32_t(n));
        }
        const auto best_record=(best<<7)+data_of(swap_data);
        const auto offset=swap_records(memory,local,first,best_record);
        // A record with a contact is welded (304130) against the records
        // after it; the weld returns the new count.
        const auto sorted=data_of(sorted_data)+offset;
        if(memory.load8(sorted_has_contact,at(sorted,offsetof(ContactRecord,byte_7d)))) {
            const auto input=memory.load32(weld_input,local.input);
            const auto parameter=memory.load32(weld_parameter,at(input,offsetof(ProcessInput,weld_parameter)));
            const auto after=memory.load32(weld_index_reload,local.sort_index)+1u;
            n=frame.weld_sorted(at(sorted,offsetof(ContactRecord,second_contact)),parameter,after,n);
        }
        const auto next=memory.load32(next_index_reload,local.sort_index)+1u;
        memory.store32(sort_index_store,local.sort_index,next);
        memory.store32(sort_offset_store,local.sort_offset,offset+record_size);
        if(!(std::int32_t(next)<std::int32_t(n)))break;
    }
    return {n,memory.load32(sorted_size,at(local.records,offsetof(HavokArray,size)))};
}

// 4. Weld every point each record's child agent holds. Returns the record
// count the welds leave.
template<class Memory,class Frame> std::uint32_t weld_child_points(Memory &memory,Frame &frame,const Locals &local,
    std::uint32_t count,std::uint32_t n) {
    using enum Access;
    using contact_array::HavokArray;
    memory.store32(point_offset_keep,local.point_offset,0u);
    memory.store32(points_left_keep,local.points_left,count);
    for(;;) {
        const auto record=memory.load32(points_data,at(local.records,offsetof(HavokArray,data)))+
            memory.load32(point_offset_reload,local.point_offset);
        const auto child_agent=memory.load32(points_agent,at(record,offsetof(ContactRecord,word_70)));
        auto remaining=std::int32_t(memory.load32(point_count,at(child_agent,offsetof(ChildAgent,point_count))));
        if(remaining>0) {
            auto point=child_agent+std::uint32_t(sizeof(ChildAgent));
            do {
                // The weld's form of the point: its position with the
                // distance in w, and its normal rotated into body B's frame.
                const auto body_b=memory.load32(point_body_b,local.body_b);
                const auto distance=x87::load(memory.load_float(point_distance,at(point,offsetof(ChildPoint,normal)+12u)));
                const auto motion=memory.load32(point_motion,at(body_b,offsetof(CdBody,motion)));
                const auto position=memory.load_vector(point_position,at(point,offsetof(ChildPoint,position)));
                // (The original pushes 2E47B0's vector argument before these
                // stores.)
                memory.store_vector(point_position_store,local.point_contact,position);
                memory.store_float(point_distance_store,at(local.point_contact,12u),x87::store(distance));
                frame.rotate_point_normal(local.rotated_normal,motion+motion_rotation,at(point,offsetof(ChildPoint,normal)));
                const auto input=memory.load32(point_input,local.input);
                const auto parameter=memory.load32(point_weld_parameter,at(input,offsetof(ProcessInput,weld_parameter)));
                const auto normal=memory.load_vector(Access::point_normal,local.rotated_normal);
                // (The original pushes the weld's arguments before this store.)
                memory.store_vector(point_normal_store,at(local.point_contact,16u),normal);
                n=frame.weld_point(local.point_contact,parameter,n);  // 304130
                point+=std::uint32_t(sizeof(ChildPoint));
            } while(--remaining);
        }
        const auto offset=memory.load32(point_offset_next,local.point_offset)+record_size;
        const auto left=memory.load32(points_left_next,local.points_left)-1u;
        memory.store32(point_offset_store,local.point_offset,offset);
        memory.store32(points_left_store,local.points_left,left);
        if(!left)return n;
    }
}

// 5. Emit each record: 30CD00 confirms the contact of each record the welds
// kept (index below n) that has one, then 30CF40 reports the record with its
// distance less the bias.
template<class Memory,class Frame> void emit(Memory &memory,Frame &frame,const Locals &local,std::uint32_t n) {
    using enum Access;
    using contact_array::HavokArray;
    const auto data_of=[&](Access role){return memory.load32(role,at(local.records,offsetof(HavokArray,data)));};
    memory.store32(emit_offset_keep,local.emit_offset,0u);
    for(std::uint32_t index=0;;) {
        const auto body_b=memory.load32(emit_body_b,local.body_b);
        const auto motion=memory.load32(emit_motion,at(body_b,offsetof(CdBody,motion)));
        const auto offset=memory.load32(emit_offset_reload,local.emit_offset);
        memory.store32(emit_parent,at(local.child_body,offsetof(CdBody,parent)),body_b);
        const auto record=data_of(emit_data)+offset;
        memory.store32(emit_motion_store,at(local.child_body,offsetof(CdBody,motion)),motion);
        const auto key=memory.load32(emit_shape_key,at(record,offsetof(ContactRecord,word_78)));
        const auto shape=memory.load32(emit_child_shape,at(record,offsetof(ContactRecord,word_74)));
        memory.store32(emit_shape_store,at(local.child_body,offsetof(CdBody,shape)),shape);
        memory.store32(emit_key_store,at(local.child_body,offsetof(CdBody,shape_key)),key);
        const auto child_agent=memory.load32(emit_agent,at(record,offsetof(ContactRecord,word_70)));
        if(std::int32_t(index)<std::int32_t(n) && memory.load8(emit_has_contact,at(record,offsetof(ContactRecord,byte_7d))))
            frame.confirm_contact(record,child_agent);  // 30CD00
        const auto distance_record=data_of(distance_data)+memory.load32(distance_offset_reload,local.emit_offset);
        const auto distance=x87::load(memory.load_float(emit_distance,at(distance_record,offsetof(ContactRecord,word_20))));
        const auto manager=memory.load32(contact_manager,at(memory.load32(emit_agent_reload,local.agent),offsetof(Agent,contact_manager)));
        const auto biased=x87::subtract(distance,memory.load_float(distance_bias,globals::emit_distance_bias));
        // The separation goes through a local (FSTP, then read back).
        memory.store_float(separation_store,local.separation,x87::store(biased));
        frame.report_contact(memory.load32(separation_load,local.separation),child_agent,manager);  // 30CF40
        ++index;
        const auto next_offset=memory.load32(emit_offset_next,local.emit_offset)+record_size;
        const auto count=memory.load32(record_count_next,local.record_count);
        memory.store32(emit_offset_store,local.emit_offset,next_offset);
        if(!(std::int32_t(index)<std::int32_t(count)))return;
    }
}

template<class Memory,class Frame> Result process_collision(Memory &memory,Frame &frame,std::uint32_t agent) {
    using enum Access;
    using contact_array::HavokArray;
    const auto &local=frame.locals();
    timer_event(memory,frame,globals::begin_timer_name,local.begin_time,
        {begin_next,begin_end,begin_name,begin_time_store,begin_time_load,begin_time,begin_advance});
    gather(memory,frame,local,agent);
    auto [n,count]=sort_and_weld(memory,frame,local);
    memory.store32(record_count_keep,local.record_count,count);
    if(std::int32_t(count)>0)n=weld_child_points(memory,frame,local,count,n);
    if(std::int32_t(memory.load32(record_count_reload,local.record_count))>0)emit(memory,frame,local,n);
    timer_event(memory,frame,globals::end_timer_name,local.end_time,
        {end_next,end_end,end_name,end_time_store,end_time_load,end_time,end_advance});
    frame.destroy_buffers();  // 303D00
    // ~hkInplaceArray: storage the records own goes back to the thread memory.
    Result result{n,count,memory.load32(Access::records_capacity,at(local.records,offsetof(HavokArray,capacity_and_flags)))};
    if(!(result.records_capacity&local_buffers::storage_not_owned)) {
        const auto owner=memory.load32(thread_memory,local_buffers::globals::thread_memory);
        const auto data=memory.load32(records_storage,at(local.records,offsetof(HavokArray,data)));
        frame.deallocate_records(owner,data,(result.records_capacity&contact_array::capacity_mask)*record_size,
            local_buffers::array_storage_class);
    }
    return result;
}
} // namespace collection_agent

// --------------------------------------------------------- surface polygon
// 1F5FE0 builds the Havok shape that collides against one collision BSP
// surface: a convex polygon whose vertices are the surface's corners, in
// boundary order. It walks the surface's ring of edges; from each edge it
// takes the vertex on the side facing the surface. When the caller passes an
// object matrix, each corner is transformed into world space first.
//
// Callers: 1F7C10 (no matrix) passes the BSP, the top three bits of a shape
// key, -1, the key's low word as the surface index, and the key itself.
namespace surface_polygon {
namespace globals {
inline constexpr std::uint32_t convex_radius=0x48b880u;  // float: every polygon's convex radius
inline constexpr std::uint32_t unit_scale=0x46a0d0u;     // float 1.0
inline constexpr std::uint32_t shape_vtable=0x4620d0u;   // the polygon shape's vtable
}

// The shape's header. The first fields are Havok's (by position):
// hkReferencedObject (vtable, size and flags, reference count), hkShape (user
// data) and hkConvexShape (radius).
struct PolygonShapeHeader {
    std::uint32_t vtable;
    std::uint16_t memory_size_and_flags;  // not written
    std::uint16_t reference_count;
    std::uint32_t user_data;              // the caller's shape key
    float convex_radius;
    std::uint32_t surface_index;
    std::uint32_t vertex_count;
    std::uint16_t material;               // the surface's material
    std::uint8_t surface_flags;           // the surface's flags
    std::uint8_t key_type;                // the caller's byte (1F7C10: key bits 29..31)
    std::uint32_t owner;                  // the caller's dword (1F7C10: -1)
};
static_assert(sizeof(PolygonShapeHeader)==0x20);
// The vertices follow the header. w holds the vertex's index in its low
// bits, tagged with 0x3F000000.
struct PolygonVertex {
    float point[3];
    std::uint32_t id;
};
static_assert(sizeof(PolygonVertex)==16);
inline constexpr std::uint32_t vertex_id_tag=0x3f000000u;

enum class Access {
    // header
    convex_radius,unit_scale,clear_user_data,clear_vertex_count,key_type,owner,reference_count,radius,vtable,
    surface_index,surface_table,first_edge,
    // per edge
    edge_table,right_surface,edge_vertex,vertex_table,
    // per edge, transformed corners
    scale,vertex_x,vertex_y,vertex_z,scale_y,scale_z,
    up_x,left_x,forward_x,position_x,left_y,world_x,
    up_y,forward_y,position_y,world_y,
    up_z,left_z,forward_z,position_z,world_z,
    // per edge, the appended vertex
    vertex_count,corner_z,corner_y,corner_x,point_x,point_y,point_z,point_w,
    id_count,vertex_id,count_before_increment,vertex_count_store,next_edge,
    // trailer
    material,material_store,surface_flags,surface_flags_store,user_data
};

// The scratch values a transformed corner leaves in the original's registers.
struct TransformResidue {
    float left_z_times_y;
    float forward_y_times_x;
    float world_z;
};
struct Residue {
    float unit_scale;
    float last_z,last_y;  // the last corner's z and y, as appended
    std::optional<TransformResidue> transformed;
};

// One corner in world space, stored to the frame's local point (the caller's
// adapter supplies its address). The matrix is re-read for every corner.
template<class Memory> TransformResidue transform_corner(Memory &memory,std::uint32_t matrix,std::uint32_t vertex,
    float unit,std::uint32_t local) {
    using enum Access;
    const auto scale_value=memory.load_float(scale,at(matrix,offsetof(RealMatrix4x3,scale)));
    auto x=memory.load_float(vertex_x,at(vertex,offsetof(BspVertex,point[0])));
    auto y=memory.load_float(vertex_y,at(vertex,offsetof(BspVertex,point[1])));
    auto z=memory.load_float(vertex_z,at(vertex,offsetof(BspVertex,point[2])));
    // The scale is skipped only when it is exactly the unit scale (UCOMISS
    // ordered equality, so a NaN scale is applied). The original re-reads it
    // for y and z, and it is the first operand of each product.
    if(!sse::ordered_equal(scale_value,unit)) {
        x=sse::multiply(scale_value,x);
        y=sse::multiply(memory.load_float(scale_y,at(matrix,offsetof(RealMatrix4x3,scale))),y);
        z=sse::multiply(memory.load_float(scale_z,at(matrix,offsetof(RealMatrix4x3,scale))),z);
    }
    // Each world coordinate is ((up*z + left*y) + forward*x) + position, with
    // the matrix element as the first operand of each product. This is the
    // original's order; it decides the rounding and which NaN payload
    // survives. The original also loads left.y before it stores x.
    const auto load=[&](Access access,std::size_t offset){return memory.load_float(access,at(matrix,offset));};
    const auto sum=[](float up_term,float left_term,float forward_term,float position) {
        return sse::add(sse::add(sse::add(up_term,left_term),forward_term),position);
    };
    const auto up_x_term=sse::multiply(load(up_x,offsetof(RealMatrix4x3,up[0])),z);
    const auto left_x_term=sse::multiply(load(left_x,offsetof(RealMatrix4x3,left[0])),y);
    const auto forward_x_term=sse::multiply(load(forward_x,offsetof(RealMatrix4x3,forward[0])),x);
    const auto world_x_value=sum(up_x_term,left_x_term,forward_x_term,load(position_x,offsetof(RealMatrix4x3,position[0])));
    const auto left_y_term=sse::multiply(load(left_y,offsetof(RealMatrix4x3,left[1])),y);
    memory.store_float(world_x,at(local,offsetof(BspVertex,point[0])),world_x_value);

    const auto up_y_term=sse::multiply(load(up_y,offsetof(RealMatrix4x3,up[1])),z);
    const auto forward_y_term=sse::multiply(load(forward_y,offsetof(RealMatrix4x3,forward[1])),x);
    const auto world_y_value=sum(up_y_term,left_y_term,forward_y_term,load(position_y,offsetof(RealMatrix4x3,position[1])));
    memory.store_float(world_y,at(local,offsetof(BspVertex,point[1])),world_y_value);

    const auto up_z_term=sse::multiply(load(up_z,offsetof(RealMatrix4x3,up[2])),z);
    const auto left_z_term=sse::multiply(load(left_z,offsetof(RealMatrix4x3,left[2])),y);
    const auto forward_z_term=sse::multiply(load(forward_z,offsetof(RealMatrix4x3,forward[2])),x);
    const auto world_z_value=sum(up_z_term,left_z_term,forward_z_term,load(position_z,offsetof(RealMatrix4x3,position[2])));
    memory.store_float(world_z,at(local,offsetof(BspVertex,point[2])),world_z_value);
    return {left_z_term,forward_y_term,world_z_value};
}

// Appends the corner at `corner` (a BSP vertex, or the transformed local
// point) as the shape's next vertex. Returns the corner's z and y.
template<class Memory> std::pair<float,float> append_corner(Memory &memory,std::uint32_t shape,std::uint32_t corner) {
    using enum Access;
    const auto count=memory.load32(vertex_count,at(shape,offsetof(PolygonShapeHeader,vertex_count)));
    const auto z=memory.load_float(corner_z,at(corner,offsetof(BspVertex,point[2])));
    const auto y=memory.load_float(corner_y,at(corner,offsetof(BspVertex,point[1])));
    const auto x_bits=memory.load32(corner_x,at(corner,offsetof(BspVertex,point[0])));  // copied as an integer
    const auto slot=element(at(shape,sizeof(PolygonShapeHeader)),count,sizeof(PolygonVertex));
    memory.store32(point_x,at(slot,offsetof(PolygonVertex,point[0])),x_bits);
    memory.store_float(point_y,at(slot,offsetof(PolygonVertex,point[1])),y);
    memory.store_float(point_z,at(slot,offsetof(PolygonVertex,point[2])),z);
    // w is cleared with the point, then the id is written over it (the
    // original re-reads the count for the id).
    memory.store32(point_w,at(slot,offsetof(PolygonVertex,id)),0u);
    const auto id=memory.load32(id_count,at(shape,offsetof(PolygonShapeHeader,vertex_count)));
    const auto id_slot=element(at(shape,sizeof(PolygonShapeHeader)),id,sizeof(PolygonVertex));
    memory.store32(vertex_id,at(id_slot,offsetof(PolygonVertex,id)),id|vertex_id_tag);
    return {z,y};
}

// Frame (the adapter's): the arguments and the three values the original
// keeps in its argument slots. surface_index(), key_type(), owner(), bsp()
// and shape_key() are the arguments. keep_surface() and keep_first_edge()
// store the surface's address and its first edge into argument slots, and
// reload_surface(), reload_first_edge() and reload_surface_index() read
// them back. transformed_corner() is the address of the local point.
template<class Memory,class Frame> Residue build(Memory &memory,Frame &frame,std::uint32_t shape,std::uint32_t matrix) {
    using enum Access;
    Residue residue{};
    const auto radius_value=memory.load_float(Access::convex_radius,globals::convex_radius);
    const auto index=frame.surface_index();
    residue.unit_scale=memory.load_float(Access::unit_scale,globals::unit_scale);

    // The header, with no vertices yet. The user data is cleared now and
    // set to the shape key at the end.
    memory.store32(clear_user_data,at(shape,offsetof(PolygonShapeHeader,user_data)),0u);
    memory.store32(clear_vertex_count,at(shape,offsetof(PolygonShapeHeader,vertex_count)),0u);
    memory.store8(Access::key_type,at(shape,offsetof(PolygonShapeHeader,key_type)),frame.key_type());
    const auto owner_value=frame.owner();
    const auto bsp=frame.bsp();
    memory.store32(owner,at(shape,offsetof(PolygonShapeHeader,owner)),owner_value);
    memory.store16(reference_count,at(shape,offsetof(PolygonShapeHeader,reference_count)),1u);
    memory.store_float(radius,at(shape,offsetof(PolygonShapeHeader,convex_radius)),radius_value);
    memory.store32(vtable,at(shape,offsetof(PolygonShapeHeader,vtable)),globals::shape_vtable);
    memory.store32(surface_index,at(shape,offsetof(PolygonShapeHeader,surface_index)),index);

    const auto surfaces=memory.load32(surface_table,at(bsp,offsetof(CollisionBsp,surfaces.elements)));
    const auto surface=element(surfaces,index,sizeof(BspSurface));
    frame.keep_surface(surface);
    const auto first=memory.load16(Access::first_edge,at(surface,offsetof(BspSurface,first_edge)));
    frame.keep_first_edge(first);

    // Walk the boundary until it returns to the first edge.
    auto edge_index=first;
    for(bool first_corner=true;;first_corner=false) {
        // The original keeps the surface index in a register for the first
        // edge only, and re-reads its argument slot for every later edge.
        const auto walked=first_corner?index:frame.reload_surface_index();
        const auto edges=memory.load32(edge_table,at(bsp,offsetof(CollisionBsp,edges.elements)));
        const auto edge=element(edges,edge_index,sizeof(BspEdge));
        // The side facing this surface: the right side when the edge's right
        // surface is this one (the sign-extended word against the whole
        // index), otherwise the left side.
        const auto right=std::int16_t(memory.load16(right_surface,at(edge,offsetof(BspEdge,surface[1]))));
        const std::uint32_t side=std::int32_t(right)==std::int32_t(walked)?1u:0u;
        const auto vertex_index=memory.load16(edge_vertex,at(edge,offsetof(BspEdge,vertex)+side*2u));
        const auto vertices=memory.load32(vertex_table,at(bsp,offsetof(CollisionBsp,vertices.elements)));
        auto corner=element(vertices,vertex_index,sizeof(BspVertex));
        if(matrix) {
            residue.transformed=transform_corner(memory,matrix,corner,residue.unit_scale,frame.transformed_corner());
            corner=frame.transformed_corner();
        }
        const auto [z,y]=append_corner(memory,shape,corner);
        residue.last_z=z;residue.last_y=y;

        // Count the vertex. (The original re-reads the first edge from its
        // slot between loading and storing the count.)
        const auto count=memory.load32(count_before_increment,at(shape,offsetof(PolygonShapeHeader,vertex_count)));
        const auto stop=frame.reload_first_edge();
        memory.store32(vertex_count_store,at(shape,offsetof(PolygonShapeHeader,vertex_count)),count+1u);
        const auto next=memory.load16(next_edge,at(edge,offsetof(BspEdge,next_edge)+side*2u));
        if(next==stop)break;
        edge_index=next;
    }

    // The trailer: the surface's material and flags, then the shape key.
    const auto closed=frame.reload_surface();
    const auto material_value=memory.load16(material,at(closed,offsetof(BspSurface,material)));
    memory.store16(material_store,at(shape,offsetof(PolygonShapeHeader,material)),material_value);
    const auto flags=memory.load8(surface_flags,at(closed,offsetof(BspSurface,flags)));
    const auto key=frame.shape_key();
    memory.store8(surface_flags_store,at(shape,offsetof(PolygonShapeHeader,surface_flags)),flags);
    memory.store32(user_data,at(shape,offsetof(PolygonShapeHeader,user_data)),key);
    return residue;
}
} // namespace surface_polygon
// ------------------------------------------------------- manifold report
// 30CF40: the contact points a convex child agent keeps from one step to the
// next (its manifold, the point set 304350 passes as agent+18/agent+30) are
// checked against the two bodies' current transforms. Each cached point holds
// a point in body A's frame and one in body B's frame, the contact normal and
// a weight. A point stays when, in world space:
//   - its distance (A's point minus B's point, along the normal) is less
//     than the input's tolerance;
//   - that distance is greater than the separation the caller passes;
//   - A's point has not drifted along the contact plane: the squared offset
//     between A's point and B's point moved along the normal by the distance
//     is less than the square of the drift tolerance in the input's settings.
// A point that stays is appended to the output's contact points, positioned
// between the two world points by its weight (weight * A + (1 - weight) * B),
// with the distance in w. A point that fails is removed from the contact
// manager (its id) and from the manifold: the manifold's last point moves
// into its place and is checked next. The output's array is first grown
// (2E31E0, hkArrayUtil::_reserve) to hold every cached point, and it shrinks
// by one for each removed point.
//
// x87: the drift tolerance is squared on the x87 stack and rounded to single
// precision in the frame; the three tests are FLD/FCOMP of single-precision
// values, so each is an ordered comparison of the two floats. The tests run in
// order and stop at the first that fails (later operands are not read).
namespace manifold_report {
namespace globals {
inline constexpr std::uint32_t one=0x41c500u;  // hkVector4 (1, 1, 1, 1): the weight's complement
}
// A cached point of the manifold (0x30 bytes). 304350 reads the same points
// from agent+40 as ChildPoint (B's point, then the normal).
struct CachedPoint {
    Lanes point_a;  // in body A's frame; w is the weight of A's point
    Lanes point_b;  // in body B's frame; the low word of w is the contact point's id
    Lanes normal;   // world space; w is carried to the output unchanged
};
static_assert(sizeof(CachedPoint)==0x30 && offsetof(CachedPoint,point_b)==0x10 && offsetof(CachedPoint,normal)==0x20);
inline constexpr std::uint32_t weight_offset=offsetof(CachedPoint,point_a)+12u;
inline constexpr std::uint32_t id_offset=offsetof(CachedPoint,point_b)+12u;
// A reported contact point (0x30 bytes): the position with the distance in w,
// the normal, and the id. The rest of the element is not written here.
struct OutputPoint {
    Lanes position;
    Lanes normal;
    std::uint16_t id;
    std::uint8_t not_used_22[0xe];
};
static_assert(sizeof(OutputPoint)==0x30 && offsetof(OutputPoint,id)==0x20);
inline constexpr std::uint32_t distance_offset=offsetof(OutputPoint,position)+12u;
// The process input as read here: the tolerance (+8) and the settings (+C)
// whose float at +8 is the drift tolerance. 304350 passes the same pointer
// at +C to its welds (collection_agent::ProcessInput::weld_parameter).
struct ReportInput {
    std::uint32_t dispatcher;
    std::uint32_t not_used_4;
    float tolerance;
    std::uint32_t settings;
};
static_assert(offsetof(ReportInput,tolerance)==0x8 && offsetof(ReportInput,settings)==0xc);
inline constexpr std::uint32_t drift_tolerance_offset=0x8u;  // in the settings
// The output: its first field is the hkArray of reported points.
inline constexpr std::uint32_t output_points_offset=0x0u;

// The addresses of the arguments and of the frame's locals (the adapter's).
struct Locals {
    std::uint32_t body_a,body_b,input,separation,points,point_count,output,manager;  // the arguments
    std::uint32_t array;           // the output's point array; the same slot then holds each distance
    std::uint32_t kept;            // points reported so far
    std::uint32_t distance_copy;   // the distance, copied through an integer register
    std::uint32_t offset_squared;  // the point's squared drift
    std::uint32_t drift_squared;   // the squared drift tolerance
    std::uint32_t weight;          // the point's weight, copied
};

enum class Access {
    // the output's room
    input_argument,input_settings,drift_tolerance,output_argument,output_array,output_capacity,output_size,
    drift_squared_store,count_argument,point_count,kept_init,array_keep,array_reload,output_data,output_size_store,
    point_count_again,points_argument,
    // one point in world space
    point_a,body_a_argument,body_a_motion,a_translation,a_column_2,body_b_argument,body_b_motion,b_column_2,
    a_column_1,a_column_0,point_b,b_translation,b_column_1,b_column_0,normal,
    // the three tests
    distance_store,distance_load,distance_bits,plane_input,plane_tolerance,distance_copy_store,distance_copy_load,
    offset_store,separation_distance,separation_argument,offset_load,drift_squared_load,
    // a point that stays
    weight,kept_distance,weight_store,weight_load,one,position_store,kept_normal,normal_store,distance_out_store,
    kept_id,id_store,kept_load,kept_store,
    // a point that is removed
    manager_argument,removed_id,removed_count,removed_count_store,removed_points,last_point_a,point_a_store,
    last_point_b,point_b_store,last_normal,removed_output,point_normal_store,removed_array,output_size_load,
    output_size_decrement,
    // the loop
    kept_reload,count_reload
};

// The SSE values of one point's check, in the original's operations. The
// adapter publishes the scratch XMM registers from them.
struct PointCheck {
    Vector4 world_a,world_b;  // the two points in world space
    Vector4 normal;
    Vector4 separation_terms; // (A - B) * normal
    float distance;           // y + x, then z + (y + x) of separation_terms
    Vector4 drift_terms;      // (A - (B + normal * distance)) squared
    float drift_xy,drift;     // y + x, then z + (y + x) of drift_terms
};
// A point that stays: the interpolated position and its weight product.
struct Reported {
    Vector4 weighted_a;  // weight * A
    Vector4 position;    // (1 - weight) * B + weight * A
};
// Which test removed a point (none: it stays).
enum class Failed {none,tolerance,separation,drift};

// A point mapped into world space by an hkTransform's rotation columns and
// translation: (c0 * x + c1 * y) + (c2 * z + t). Each product takes the
// column first and the broadcast coordinate second; the order of the products
// and sums is the original's (rounding, and which NaN payload survives).
struct TransformTerms {Vector4 column[3],translation;};
inline Vector4 to_world(const TransformTerms &t,Vector4 point) {
    const auto z_and_translation=sse::add(sse::multiply(t.column[2],splat(point,2)),t.translation);
    const auto y_term=sse::multiply(t.column[1],splat(point,1));
    const auto x_term=sse::multiply(t.column[0],splat(point,0));
    return sse::add(sse::add(x_term,y_term),z_and_translation);
}
// The horizontal sum the original makes with two ADDSS: y + x, then z + that.
// MOVSS then SHUFPS 0: the value in every lane.
inline Vector4 broadcast(float value){const auto x=std::bit_cast<std::uint32_t>(value);return vector({x,x,x,x});}
inline std::pair<float,float> sum_yx_z(Vector4 v) {
    const auto l=lanes(v);
    const auto yx=sse::add(std::bit_cast<float>(l[1]),std::bit_cast<float>(l[0]));
    return {yx,sse::add(std::bit_cast<float>(l[2]),yx)};
}

template<class Memory,class Frame> struct Report {
    Memory &memory;Frame &frame;const Locals &local;
    using enum Access;

    // The world positions, the distance along the normal and the drift. The
    // reads interleave A's and B's transforms as the original does.
    PointCheck check(std::uint32_t point) {
        using collection_agent::CdBody;
        PointCheck c{};
        const auto a=memory.load_vector(point_a,at(point,offsetof(CachedPoint,point_a)));
        const auto motion_a=memory.load32(body_a_motion,at(memory.load32(body_a_argument,local.body_a),offsetof(CdBody,motion)));
        TransformTerms ta{},tb{};
        const auto transform_a=at(motion_a,collection_agent::motion_rotation);
        ta.translation=memory.load_vector(a_translation,at(transform_a,offsetof(HavokTransform,translation)));
        ta.column[2]=memory.load_vector(a_column_2,at(transform_a,offsetof(HavokTransform,column[2])));
        const auto motion_b=memory.load32(body_b_motion,at(memory.load32(body_b_argument,local.body_b),offsetof(CdBody,motion)));
        const auto transform_b=at(motion_b,collection_agent::motion_rotation);
        tb.column[2]=memory.load_vector(b_column_2,at(transform_b,offsetof(HavokTransform,column[2])));
        ta.column[1]=memory.load_vector(a_column_1,at(transform_a,offsetof(HavokTransform,column[1])));
        ta.column[0]=memory.load_vector(a_column_0,at(transform_a,offsetof(HavokTransform,column[0])));
        const auto b=memory.load_vector(point_b,at(point,offsetof(CachedPoint,point_b)));
        tb.translation=memory.load_vector(b_translation,at(transform_b,offsetof(HavokTransform,translation)));
        tb.column[1]=memory.load_vector(b_column_1,at(transform_b,offsetof(HavokTransform,column[1])));
        tb.column[0]=memory.load_vector(b_column_0,at(transform_b,offsetof(HavokTransform,column[0])));
        c.world_a=to_world(ta,a);
        c.world_b=to_world(tb,b);
        c.normal=memory.load_vector(normal,at(point,offsetof(CachedPoint,normal)));
        c.separation_terms=sse::multiply(sse::subtract(c.world_a,c.world_b),c.normal);
        c.distance=sum_yx_z(c.separation_terms).second;
        return c;
    }
    // The three tests. The distance goes through the frame (its slot is read
    // back for each use), and the drift is computed before the first test is
    // decided, as in the original. Each test keeps the point only when its
    // FCOMP finds the ordered relation (TEST AH,5 / JP: below; TEST AH,41 /
    // JNZ: above); equal values and NaNs remove it.
    Failed test(PointCheck &c) {
        memory.store_float(distance_store,local.array,c.distance);
        const auto distance=x87::load(memory.load_float(distance_load,local.array));
        const auto bits=memory.load32(distance_bits,local.array);
        const auto input=memory.load32(plane_input,local.input);
        const bool within=distance<x87::load(memory.load_float(plane_tolerance,at(input,offsetof(ReportInput,tolerance))));
        memory.store32(distance_copy_store,local.distance_copy,bits);
        const auto along=sse::multiply(broadcast(memory.load_float(distance_copy_load,local.distance_copy)),c.normal);
        const auto offset=sse::subtract(c.world_a,sse::add(c.world_b,along));
        c.drift_terms=sse::multiply(offset,offset);
        const auto [drift_xy,drift]=sum_yx_z(c.drift_terms);
        c.drift_xy=drift_xy;c.drift=drift;
        memory.store_float(offset_store,local.offset_squared,c.drift);
        if(!within)return Failed::tolerance;
        const auto again=x87::load(memory.load_float(separation_distance,local.array));
        if(!(again>x87::load(memory.load_float(separation_argument,local.separation))))return Failed::separation;
        const auto squared=x87::load(memory.load_float(offset_load,local.offset_squared));
        if(!(squared<x87::load(memory.load_float(drift_squared_load,local.drift_squared))))return Failed::drift;
        return Failed::none;
    }
    // A point that stays: appended at out, with the distance in w and its id.
    Reported append(const PointCheck &c,std::uint32_t point,std::uint32_t out) {
        const auto w=memory.load32(weight,at(point,weight_offset));
        const auto distance=memory.load32(kept_distance,local.array);
        memory.store32(weight_store,local.weight,w);
        const auto weights=broadcast(memory.load_float(weight_load,local.weight));
        Reported r{};
        r.weighted_a=sse::multiply(weights,c.world_a);
        r.position=sse::add(sse::multiply(sse::subtract(memory.load_vector(one,globals::one),weights),c.world_b),r.weighted_a);
        memory.store_vector(position_store,at(out,offsetof(OutputPoint,position)),r.position);
        memory.store_vector(normal_store,at(out,offsetof(OutputPoint,normal)),memory.load_vector(kept_normal,at(point,offsetof(CachedPoint,normal))));
        memory.store32(distance_out_store,at(out,distance_offset),distance);  // over the position's w
        memory.store16(id_store,at(out,offsetof(OutputPoint,id)),memory.load16(kept_id,at(point,id_offset)));
        memory.store32(kept_store,local.kept,memory.load32(kept_load,local.kept)+1u);
        return r;
    }
    // A point that fails: the contact manager forgets its id, and the
    // manifold's last point moves into its place (the output had room for it,
    // so the output's size shrinks by one).
    void remove(std::uint32_t point,std::uint32_t count_address) {
        const auto manager=memory.load32(manager_argument,local.manager);
        frame.remove_contact_point(manager,memory.load16(removed_id,at(point,id_offset)));
        const auto count=memory.load32(removed_count,count_address)-1u;
        memory.store32(removed_count_store,count_address,count);
        const auto last=element(memory.load32(removed_points,local.points),count,sizeof(CachedPoint));
        memory.store_vector(point_a_store,at(point,offsetof(CachedPoint,point_a)),memory.load_vector(last_point_a,at(last,offsetof(CachedPoint,point_a))));
        memory.store_vector(point_b_store,at(point,offsetof(CachedPoint,point_b)),memory.load_vector(last_point_b,at(last,offsetof(CachedPoint,point_b))));
        const auto moved_normal=memory.load_vector(last_normal,at(last,offsetof(CachedPoint,normal)));
        const auto output=memory.load32(removed_output,local.output);
        memory.store_vector(point_normal_store,at(point,offsetof(CachedPoint,normal)),moved_normal);
        const auto array=memory.load32(removed_array,at(output,output_points_offset));
        const auto size_address=at(array,offsetof(contact_array::HavokArray,size));
        memory.store32(output_size_decrement,size_address,memory.load32(output_size_load,size_address)-1u);
    }
};

// Frame (the adapter's): locals(), reserve() (2E31E0: the output's array, the
// capacity, the element size), remove_contact_point() (the contact manager's
// virtual removal of an id), and the hooks below that receive each point's
// check and outcome (the adapter's registers).
template<class Memory,class Frame> void verify_and_report(Memory &memory,Frame &frame) {
    using enum Access;
    using contact_array::HavokArray;
    const auto &local=frame.locals();
    const auto input=memory.load32(input_argument,local.input);
    const auto settings=memory.load32(input_settings,at(input,offsetof(ReportInput,settings)));
    const auto drift=x87::load(memory.load_float(drift_tolerance,at(settings,drift_tolerance_offset)));
    const auto output=memory.load32(output_argument,local.output);
    const auto array=memory.load32(output_array,at(output,output_points_offset));
    const auto capacity=memory.load32(output_capacity,at(array,offsetof(HavokArray,capacity_and_flags)))&contact_array::capacity_mask;
    const auto size=memory.load32(output_size,at(array,offsetof(HavokArray,size)));
    // Squared on the x87 stack, rounded to single precision when stored.
    memory.store_float(drift_squared_store,local.drift_squared,x87::store(x87::multiply(drift,drift)));
    const auto count_address=memory.load32(count_argument,local.point_count);
    const auto needed=memory.load32(point_count,count_address)+size;
    memory.store32(kept_init,local.kept,0u);
    memory.store32(array_keep,local.array,array);
    auto points_array=array;
    if(std::int32_t(capacity)<std::int32_t(needed)) {
        // Grow to twice the capacity, or to what is needed if that is more
        // (signed comparisons, as the original's).
        const auto doubled=capacity*2u;
        frame.reserve(array,std::int32_t(needed)<std::int32_t(doubled)?doubled:needed,std::uint32_t(sizeof(OutputPoint)));
        points_array=memory.load32(array_reload,local.array);
    }
    const auto data=memory.load32(output_data,at(points_array,offsetof(HavokArray,data)));
    memory.store32(output_size_store,at(points_array,offsetof(HavokArray,size)),needed);
    auto out=element(data,size,sizeof(OutputPoint));
    if(std::int32_t(memory.load32(point_count_again,count_address))<=0)return;
    auto point=memory.load32(points_argument,local.points);
    Report<Memory,Frame> report{memory,frame,local};
    do {
        auto c=report.check(point);
        const auto failed=report.test(c);
        frame.tested(c,failed);
        if(failed==Failed::none) {
            frame.reported(c,report.append(c,point,out));
            out+=std::uint32_t(sizeof(OutputPoint));
            point+=std::uint32_t(sizeof(CachedPoint));
        } else report.remove(point,count_address);  // the moved point is checked next
    } while(std::int32_t(memory.load32(kept_reload,local.kept))<std::int32_t(memory.load32(count_reload,count_address)));
}
} // namespace manifold_report
// ------------------------------------------------------------ contact weld
// 304130: welds the contact records [first, n) of an hkArray against one
// contact (304350 passes its sorted records, and each child agent's own
// points). Every record whose state is 1 is tested by 303800 (a thiscall on
// the agent: the verdict for the record's second contact against the contact,
// with the input's parameter). A record the test rejects leaves the first n:
// it is swapped with the last of them, n shrinks by one (in the argument's
// slot), and the record moved into its place is tested next. Returns the n
// that remains.
//
// The swap is a field-by-field exchange in the original's order, which
// matters when the two records are the same one (the last record rejected):
// the record's six vectors are held in XMM registers, its two floats at +20
// and +24 on the x87 stack (so a signalling NaN comes back quieted), and its
// words and bytes at +70..+7D in the frame; the last record is copied over
// it; then the held fields are written into the last record. The original
// also spills the addresses of the last record's fields to its frame while it
// copies, and reads them back for the write-back.
namespace contact_weld {
using contact_record::ContactRecord;
inline constexpr std::uint8_t tested_state=1u;  // ContactRecord::state of a record the weld tests
inline constexpr std::uint32_t weld_test=0x303800u;  // the test, thiscall on the agent; RET 10

// The addresses of the arguments and of the frame's locals (the adapter's).
struct Locals {
    std::uint32_t contact,records,first,count,parameter;  // the arguments; count is rewritten
    std::uint32_t self;     // this, kept for the reloads
    std::uint32_t index;    // the record being welded
    std::uint32_t verdict;  // the hkBool the test writes (its address is the test's argument)
    // The replaced record's words and bytes, held while the last record is
    // copied over it.
    std::uint32_t held_word_70,held_word_74,held_word_78,held_state,held_byte_7d;
    // Where the last record's fields are, kept for the write-back.
    std::uint32_t at_word_20,at_word_24,at_vector_30,at_vector_40,at_word_70,at_word_74,at_word_78,at_state,at_byte_7d;
};

enum class Access {
    // entry and the loop
    count_argument,first_argument,self_keep,index_init,records_argument,records_data,record_state,
    parameter_argument,contact_argument,self_reload,verdict,count_kept,index_load,index_store,index_compare,
    // the swap: the record's fields held
    held_vector_50,held_word_20,held_vector_60,held_word_24,held_word_74,held_word_70,last_position,held_word_78,
    held_position,held_normal,held_vector_30,held_vector_40,
    // the last record copied over it (and the addresses kept)
    position_store,last_normal,normal_store,held_word_78_store,last_word_20,word_20_store,held_word_70_store,
    held_state,held_word_74_store,held_byte_7d,held_state_store,held_byte_7d_store,at_word_20_store,at_word_24_store,
    last_word_24,word_24_store,last_vector_30,vector_30_store,last_vector_40,vector_40_store,last_vector_50,
    vector_50_store,last_vector_60,vector_60_store,at_vector_30_store,at_vector_40_store,at_word_70_store,last_word_70,
    word_70_store,at_word_74_store,last_word_74,word_74_store,at_word_78_store,last_word_78,word_78_store,
    at_state_store,last_state,state_store,at_byte_7d_store,last_byte_7d,byte_7d_store,
    // the held fields written into the last record
    at_word_24_load,written_normal,at_word_20_load,written_word_20,at_vector_30_load,written_vector_30,
    at_word_70_load,written_word_24,at_vector_40_load,written_vector_40,held_word_74_load,written_position,
    written_vector_50,written_vector_60,held_word_70_load,written_word_70,at_word_74_load,held_word_78_load,
    written_word_74,at_word_78_load,held_state_load,written_word_78,at_state_load,held_byte_7d_load,written_state,
    count_swap,at_byte_7d_load,written_byte_7d,count_store
};

// The record's vectors as the original holds them in XMM0..XMM5 during the
// swap (the adapter publishes them), and the last record's second normal,
// which it leaves in XMM6.
struct Swapped {
    Vector4 position,normal,vector_30,vector_40,second_position,second_normal;
    Vector4 last_second_normal;
    std::uint32_t at_word_78;  // read back for the write-back (EDX's upper bytes at the exit)
    std::uint32_t count;       // n after the swap
};

// record <-> last; returns the new n (the argument's slot less one).
template<class Memory> Swapped swap_with_last(Memory &memory,const Locals &local,std::uint32_t record,std::uint32_t last) {
    using enum Access;
    const auto field=[](std::uint32_t base,std::size_t offset){return at(base,offset);};
    Swapped held{};
    // Hold the record's fields (the original's load order).
    held.second_position=memory.load_vector(held_vector_50,field(record,offsetof(ContactRecord,second_contact.position)));
    const auto word_20=x87::load(memory.load_float(held_word_20,field(record,offsetof(ContactRecord,word_20))));
    held.second_normal=memory.load_vector(held_vector_60,field(record,offsetof(ContactRecord,second_contact.separating_normal)));
    const auto word_24=x87::load(memory.load_float(held_word_24,field(record,offsetof(ContactRecord,word_24))));
    const auto word_74=memory.load32(held_word_74,field(record,offsetof(ContactRecord,word_74)));
    const auto word_70=memory.load32(held_word_70,field(record,offsetof(ContactRecord,word_70)));
    const auto last_position_value=memory.load_vector(last_position,field(last,offsetof(ContactRecord,contact.position)));
    const auto word_78=memory.load32(held_word_78,field(record,offsetof(ContactRecord,word_78)));
    held.position=memory.load_vector(held_position,field(record,offsetof(ContactRecord,contact.position)));
    held.normal=memory.load_vector(held_normal,field(record,offsetof(ContactRecord,contact.separating_normal)));
    held.vector_30=memory.load_vector(held_vector_30,field(record,offsetof(ContactRecord,vector_30)));
    held.vector_40=memory.load_vector(held_vector_40,field(record,offsetof(ContactRecord,vector_40)));
    // Copy the last record over it, holding the record's words and bytes in
    // the frame on the way.
    memory.store_vector(position_store,field(record,offsetof(ContactRecord,contact.position)),last_position_value);
    memory.store_vector(normal_store,field(record,offsetof(ContactRecord,contact.separating_normal)),
        memory.load_vector(last_normal,field(last,offsetof(ContactRecord,contact.separating_normal))));
    memory.store32(held_word_78_store,local.held_word_78,word_78);
    memory.store32(word_20_store,field(record,offsetof(ContactRecord,word_20)),
        memory.load32(last_word_20,field(last,offsetof(ContactRecord,word_20))));
    memory.store32(held_word_70_store,local.held_word_70,word_70);
    const auto state=memory.load8(held_state,field(record,offsetof(ContactRecord,state)));
    memory.store32(held_word_74_store,local.held_word_74,word_74);
    const auto byte_7d=memory.load8(held_byte_7d,field(record,offsetof(ContactRecord,byte_7d)));
    memory.store8(held_state_store,local.held_state,state);
    memory.store8(held_byte_7d_store,local.held_byte_7d,byte_7d);
    memory.store32(at_word_20_store,local.at_word_20,field(last,offsetof(ContactRecord,word_20)));
    memory.store32(at_word_24_store,local.at_word_24,field(last,offsetof(ContactRecord,word_24)));
    memory.store32(word_24_store,field(record,offsetof(ContactRecord,word_24)),
        memory.load32(last_word_24,field(last,offsetof(ContactRecord,word_24))));
    const auto copy_vector=[&](Access from,Access to,std::size_t offset) {
        memory.store_vector(to,field(record,offset),memory.load_vector(from,field(last,offset)));
    };
    copy_vector(last_vector_30,vector_30_store,offsetof(ContactRecord,vector_30));
    copy_vector(last_vector_40,vector_40_store,offsetof(ContactRecord,vector_40));
    copy_vector(last_vector_50,vector_50_store,offsetof(ContactRecord,second_contact.position));
    held.last_second_normal=memory.load_vector(last_vector_60,field(last,offsetof(ContactRecord,second_contact.separating_normal)));
    memory.store_vector(vector_60_store,field(record,offsetof(ContactRecord,second_contact.separating_normal)),held.last_second_normal);
    memory.store32(at_vector_30_store,local.at_vector_30,field(last,offsetof(ContactRecord,vector_30)));
    memory.store32(at_vector_40_store,local.at_vector_40,field(last,offsetof(ContactRecord,vector_40)));
    const auto copy_word=[&](Access keep,std::uint32_t slot,Access from,Access to,std::size_t offset) {
        memory.store32(keep,slot,field(last,offset));
        memory.store32(to,field(record,offset),memory.load32(from,field(last,offset)));
    };
    copy_word(at_word_70_store,local.at_word_70,last_word_70,word_70_store,offsetof(ContactRecord,word_70));
    copy_word(at_word_74_store,local.at_word_74,last_word_74,word_74_store,offsetof(ContactRecord,word_74));
    copy_word(at_word_78_store,local.at_word_78,last_word_78,word_78_store,offsetof(ContactRecord,word_78));
    const auto copy_byte=[&](Access keep,std::uint32_t slot,Access from,Access to,std::size_t offset) {
        memory.store32(keep,slot,field(last,offset));
        memory.store8(to,field(record,offset),memory.load8(from,field(last,offset)));
    };
    copy_byte(at_state_store,local.at_state,last_state,state_store,offsetof(ContactRecord,state));
    copy_byte(at_byte_7d_store,local.at_byte_7d,last_byte_7d,byte_7d_store,offsetof(ContactRecord,byte_7d));
    // Write the held fields into the last record, through the addresses read
    // back (the two floats leave the x87 stack in its order: +20, then +24).
    const auto to_word_24=memory.load32(at_word_24_load,local.at_word_24);
    memory.store_vector(written_normal,field(last,offsetof(ContactRecord,contact.separating_normal)),held.normal);
    memory.store_float(written_word_20,memory.load32(at_word_20_load,local.at_word_20),x87::store(word_20));
    memory.store_vector(written_vector_30,memory.load32(at_vector_30_load,local.at_vector_30),held.vector_30);
    const auto to_word_70=memory.load32(at_word_70_load,local.at_word_70);
    memory.store_float(written_word_24,to_word_24,x87::store(word_24));
    memory.store_vector(written_vector_40,memory.load32(at_vector_40_load,local.at_vector_40),held.vector_40);
    const auto held_74=memory.load32(held_word_74_load,local.held_word_74);
    memory.store_vector(written_position,field(last,offsetof(ContactRecord,contact.position)),held.position);
    memory.store_vector(written_vector_50,field(last,offsetof(ContactRecord,second_contact.position)),held.second_position);
    memory.store_vector(written_vector_60,field(last,offsetof(ContactRecord,second_contact.separating_normal)),held.second_normal);
    memory.store32(written_word_70,to_word_70,memory.load32(held_word_70_load,local.held_word_70));
    const auto to_word_74=memory.load32(at_word_74_load,local.at_word_74);
    const auto held_78=memory.load32(held_word_78_load,local.held_word_78);
    memory.store32(written_word_74,to_word_74,held_74);
    held.at_word_78=memory.load32(at_word_78_load,local.at_word_78);
    const auto held_7c=memory.load8(held_state_load,local.held_state);
    memory.store32(written_word_78,held.at_word_78,held_78);
    const auto to_state=memory.load32(at_state_load,local.at_state);
    const auto held_7d=memory.load8(held_byte_7d_load,local.held_byte_7d);
    memory.store8(written_state,to_state,held_7c);
    held.count=memory.load32(count_swap,local.count)-1u;
    const auto to_byte_7d=memory.load32(at_byte_7d_load,local.at_byte_7d);
    memory.store8(written_byte_7d,to_byte_7d,held_7d);
    memory.store32(count_store,local.count,held.count);
    return held;
}

// Frame (the adapter's): locals(), test() (303800 on this: the verdict's
// address, the contact, the record's second contact and the parameter;
// returns the address of the verdict it wrote), and swapped() (each swap's
// held values, for the registers).
template<class Memory,class Frame> std::uint32_t weld(Memory &memory,Frame &frame,std::uint32_t self) {
    using enum Access;
    const auto &local=frame.locals();
    auto count=memory.load32(count_argument,local.count);
    const auto first=memory.load32(first_argument,local.first);
    memory.store32(self_keep,local.self,self);
    memory.store32(index_init,local.index,first);
    if(!(std::int32_t(first)<std::int32_t(count)))return count;
    // The record and the end of the first n, as byte offsets in the array.
    auto record_offset=first*std::uint32_t(sizeof(ContactRecord)),end_offset=count*std::uint32_t(sizeof(ContactRecord));
    do {
        const auto data=memory.load32(records_data,at(memory.load32(records_argument,local.records),offsetof(contact_array::HavokArray,data)));
        const auto record=data+record_offset;
        bool advance=true;
        if(memory.load8(record_state,at(record,offsetof(ContactRecord,state)))==tested_state) {
            const auto parameter=memory.load32(parameter_argument,local.parameter);
            const auto contact=memory.load32(contact_argument,local.contact);
            const auto verdict_at=frame.test(memory.load32(self_reload,local.self),local.verdict,contact,
                at(record,offsetof(ContactRecord,second_contact)),parameter);
            if(memory.load8(verdict,verdict_at)==0) {
                // Rejected: the last of the first n takes its place.
                const auto held=swap_with_last(memory,local,record,data+end_offset-std::uint32_t(sizeof(ContactRecord)));
                frame.swapped(held);
                count=held.count;
                end_offset-=std::uint32_t(sizeof(ContactRecord));
                advance=false;
            } else count=memory.load32(count_kept,local.count);
        }
        if(advance) {
            memory.store32(index_store,local.index,memory.load32(index_load,local.index)+1u);
            record_offset+=std::uint32_t(sizeof(ContactRecord));
        }
    } while(std::int32_t(memory.load32(index_compare,local.index))<std::int32_t(count));
    return count;
}
} // namespace contact_weld
// ---------------------------------------------------------------- key sort
// 2F33B0: hkAlgorithm::quickSortRecursive over an array of 32-bit keys
// compared unsigned (2F36F0 sorts the shape keys of the child shapes near a
// body this way). Hoare's partition around the middle key; the lower part is
// sorted by recursion, the upper part by looping (the original's tail call).
// The fourth argument (the comparator object of the template) is unused here
// and only passed down. Indices are signed, as the original compares them.
// After a swap the original re-reads both bounds from its argument slots, and
// the loop's new lower bound is written back to its slot.
namespace key_sort {
// The addresses of the arguments (the adapter's).
struct Locals {std::uint32_t keys,low,high,comparator;};
enum class Access {
    low_argument,high_argument,keys_argument,pivot,scan_up_first,scan_up,scan_down_first,scan_down,
    swap_low_key,swap_high_key,swap_high_store,high_reload,swap_low_store,low_reload,comparator_argument,low_store
};
inline constexpr std::uint32_t key_size=4u;
// Frame (the adapter's): locals(), sort_part() (the recursion on [low, high]),
// and partitioned() (the bounds each partition leaves, for the registers).
template<class Memory,class Frame> void sort(Memory &memory,Frame &frame) {
    using enum Access;
    const auto &local=frame.locals();
    auto low=memory.load32(low_argument,local.low);
    auto high=memory.load32(high_argument,local.high);
    const auto keys=memory.load32(keys_argument,local.keys);
    const auto key=[&](std::uint32_t index){return element(keys,index,key_size);};
    for(;;) {
        const auto middle=std::uint32_t(std::int32_t(low+high)>>1);  // SAR: an arithmetic halving
        const auto pivot=memory.load32(Access::pivot,key(middle));
        auto i=high,j=low;
        do {
            // Unsigned key comparisons, as the original's JB.
            if(memory.load32(scan_up_first,key(j))<pivot)
                do ++j; while(memory.load32(scan_up,key(j))<pivot);
            if(pivot<memory.load32(scan_down_first,key(i)))
                do --i; while(pivot<memory.load32(scan_down,key(i)));
            if(std::int32_t(i)<std::int32_t(j))break;
            if(i!=j) {
                const auto at_j=memory.load32(swap_low_key,key(j));
                const auto at_i=memory.load32(swap_high_key,key(i));
                memory.store32(swap_high_store,key(i),at_j);
                high=memory.load32(high_reload,local.high);
                memory.store32(swap_low_store,key(j),at_i);
                low=memory.load32(low_reload,local.low);
            }
            --i;++j;
        } while(std::int32_t(j)<=std::int32_t(i));
        frame.partitioned(pivot,i,j,low,high);
        if(std::int32_t(low)<std::int32_t(i))
            frame.sort_part(keys,low,i,memory.load32(comparator_argument,local.comparator));
        if(!(std::int32_t(j)<std::int32_t(high)))return;
        low=j;
        memory.store32(low_store,local.low,low);
    }
}
} // namespace key_sort
// ------------------------------------------------------- shape key filter
// 1F76A0: whether the child shape a shape key names may still collide (the
// BSP and mesh collision agents 1F6D10/1F7C10 skip the others). A key packs
// its kind in bits 29-31, a secondary index in bits 16-28 and an index in
// bits 0-15:
//   1  a surface of an instanced geometry definition (index: the instance,
//      secondary: the surface): it collides unless it is breakable and 185AA0
//      reports its breakable surface gone;
//   2  a surface of the structure's collision BSP (index: the surface): it
//      collides unless it is breakable and its bit in the breakable surfaces'
//      state bit vector is clear;
//   3  a region of an object (index: the object's slot in a handle table,
//      secondary: the region in bits 0-4 and a permutation above): it
//      collides when the object exists and is not flagged (header bit 4), and
//      the region's current permutation is the key's;
//   4  always collides.
// The original selects the kind through a jump table (the adapter's); other
// kinds have no case there.
namespace shape_key_filter {
namespace globals {
inline constexpr std::uint32_t instanced_structure=0x4ecc18u;  // the structure: instances +144, definitions +13C
inline constexpr std::uint32_t collision_structure=0x4ecc10u;  // the structure: collision surfaces +2C
inline constexpr std::uint32_t breakable_set=0x474c24u;        // int16: which 8-dword set of the bit vector
inline constexpr std::uint32_t breakable_state=0x4f9b50u;      // the bit vector (its words start at +1)
inline constexpr std::uint32_t object_handles=0x52be70u;       // the handle table for kind 3 indices
inline constexpr std::uint32_t object_headers=0x4ecbd0u;       // the object data array (its headers at +44)
}
inline constexpr std::uint32_t instance_size=0x58u,instance_definition=0x34u;
inline constexpr std::uint32_t definition_size=0xc8u,definition_surfaces=0x9cu;
inline constexpr std::uint32_t structure_instances=0x144u,structure_definitions=0x13cu,structure_surfaces=0x2cu;
// A collision surface record (8 bytes) as read here.
struct SurfaceRecord {
    std::uint8_t not_used[4];
    std::uint8_t flags;            // bit 3: breakable
    std::uint8_t breakable_index;
    std::uint8_t not_used_6[2];
};
static_assert(sizeof(SurfaceRecord)==8 && offsetof(SurfaceRecord,flags)==4);
inline constexpr std::uint8_t breakable_flag=0x08u;
// An object header (12 bytes) as read here.
struct ObjectHeader {
    std::uint8_t not_used[2];
    std::uint8_t flags;            // bit 4: the object does not collide here (not established)
    std::uint8_t not_used_3[5];
    std::uint32_t object;
};
static_assert(sizeof(ObjectHeader)==12 && offsetof(ObjectHeader,object)==8);
inline constexpr std::uint8_t excluded_flag=0x10u;
inline constexpr std::uint32_t object_headers_data=0x44u,object_region_states=0x11au;  // int16 offset of the object's region bytes
inline constexpr std::uint32_t no_object=0xffffffffu;

enum class Kind {instanced_surface,structure_surface,object_region,always};
enum class Access {
    instanced_structure,instances,definition_index,definitions,instance_surfaces,instance_flags,instance_breakable,
    collision_structure,collision_surfaces,surface_flags,surface_breakable,breakable_set,breakable_state,breakable_word,
    handles,handle,headers,headers_data,header_flags,object,region_offset,region_state
};
struct Key {
    std::uint32_t kind;       // bits 29-31
    std::uint32_t secondary;  // bits 16-28
    std::uint32_t index;      // bits 0-15
};
constexpr Key unpack(std::uint32_t key){return {key>>29,(key>>16)&0x1fffu,key&0xffffu};}
struct Result {Kind kind;bool collides;};

// Frame (the adapter's): kind() selects the case of a kind as the original's
// jump table does (it throws where the original has none), and
// breakable_surface_intact() is 185AA0.
template<class Memory,class Frame> Result may_collide(Memory &memory,Frame &frame,std::uint32_t shape_key) {
    using enum Access;
    const auto key=unpack(shape_key);
    const auto kind=frame.kind(key.kind);
    switch(kind) {
    case Kind::instanced_surface: {
        const auto structure=memory.load32(instanced_structure,globals::instanced_structure);
        const auto instance=element(memory.load32(instances,at(structure,structure_instances)),key.index,instance_size);
        // The definition index is a signed word; the multiply wraps as IMUL.
        const auto definition=std::uint32_t(std::int32_t(std::int16_t(memory.load16(definition_index,at(instance,instance_definition)))));
        const auto definition_record=element(memory.load32(definitions,at(structure,structure_definitions)),definition,definition_size);
        const auto surface=element(memory.load32(instance_surfaces,at(definition_record,definition_surfaces)),key.secondary,sizeof(SurfaceRecord));
        if(!(memory.load8(instance_flags,at(surface,offsetof(SurfaceRecord,flags)))&breakable_flag))return {kind,true};
        return {kind,frame.breakable_surface_intact(memory.load8(instance_breakable,at(surface,offsetof(SurfaceRecord,breakable_index))))};
    }
    case Kind::structure_surface: {
        const auto surfaces=memory.load32(collision_surfaces,at(memory.load32(collision_structure,globals::collision_structure),structure_surfaces));
        const auto surface=element(surfaces,key.index,sizeof(SurfaceRecord));
        if(!(memory.load8(surface_flags,at(surface,offsetof(SurfaceRecord,flags)))&breakable_flag))return {kind,true};
        const auto breakable=memory.load8(surface_breakable,at(surface,offsetof(SurfaceRecord,breakable_index)));
        // (The original also compares the zero-extended byte with -1, which
        // never matches.)
        const auto set=std::uint32_t(std::int32_t(std::int16_t(memory.load16(breakable_set,globals::breakable_set))));
        const auto word=(breakable>>5)+set*8u;
        const auto bits=memory.load32(breakable_state,globals::breakable_state);
        return {kind,(memory.load32(breakable_word,bits+word*4u+1u)&(1u<<(breakable&31u)))!=0};
    }
    case Kind::object_region: {
        const auto handle=memory.load32(Access::handle,element(memory.load32(handles,globals::object_handles),key.index,4));
        if(handle==no_object)return {kind,false};
        const auto data=memory.load32(headers_data,at(memory.load32(headers,globals::object_headers),object_headers_data));
        const auto header=element(data,handle&0xffffu,sizeof(ObjectHeader));
        if(memory.load8(header_flags,at(header,offsetof(ObjectHeader,flags)))&excluded_flag)return {kind,false};
        const auto object=memory.load32(Access::object,at(header,offsetof(ObjectHeader,object)));
        const auto states=object+std::uint32_t(std::int32_t(std::int16_t(memory.load16(region_offset,at(object,object_region_states)))));
        // The region's permutation byte is signed; the key's is 0..255.
        const auto permutation=std::int32_t(std::int8_t(memory.load8(region_state,states+(key.secondary&31u))));
        return {kind,permutation==std::int32_t((key.secondary>>5)&0xffu)};
    }
    case Kind::always:break;
    }
    return {kind,true};
}
} // namespace shape_key_filter
// ------------------------------------------------------ rotation into world
// 2E4760, hkVector4::setRotatedDir: out = rotation * vector. The sum is
// (x term + y term) + z term, each product the column first and the
// broadcast coordinate second (the original's order; the z term is computed
// first, then y, then x).
namespace havok_rotation {
struct Rotated {
    Vector4 value;
    Vector4 x_splat;          // the x coordinate in every lane
    Vector4 y_term,z_term;
};
inline Rotated rotate(const Rotation &rotation,Vector4 v) {
    const auto z_term=sse::multiply(rotation.column[2],splat(v,2));
    const auto y_term=sse::multiply(rotation.column[1],splat(v,1));
    const auto x_term=sse::multiply(rotation.column[0],splat(v,0));
    return {sse::add(sse::add(x_term,y_term),z_term),splat(v,0),y_term,z_term};
}
// Frame: rotation() and vector() are the two argument addresses. The vector
// is read first, then columns 2, 1 and 0.
enum class ForwardAccess {input,column_2,column_1,column_0,result};
template<class Memory,class Frame> Rotated rotate(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum ForwardAccess;
    const auto v=memory.load_vector(input,frame.vector());
    const auto source=frame.rotation();
    Rotation rotation;
    rotation.column[2]=memory.load_vector(column_2,at(source,offsetof(HavokTransform,column[2])));
    rotation.column[1]=memory.load_vector(column_1,at(source,offsetof(HavokTransform,column[1])));
    rotation.column[0]=memory.load_vector(column_0,at(source,offsetof(HavokTransform,column[0])));
    const auto rotated=rotate(rotation,v);
    memory.store_vector(result,out,rotated.value);
    return rotated;
}
} // namespace havok_rotation

// x87 operations of the convex pair owners, each as its instruction computes
// it on the extended stack (a memory operand converted as FLD converts it).
namespace x87 {
inline Extended add(Extended a,float b){const auto operand=load(b);volatile Extended sum=a+operand;return sum;}  // FADD m32
inline Extended subtract_from(float a,Extended b){const auto operand=load(a);volatile Extended difference=operand-b;return difference;}  // FSUBR m32
inline Extended multiply(Extended a,float b){const auto operand=load(b);volatile Extended product=a*operand;return product;}  // FMUL m32
inline Extended negate(Extended a){volatile Extended negated=-a;return negated;}  // FCHS
// FCOM m32 tested with TEST AH,5 / JP: below, and ordered.
inline bool less(Extended a,float b){const auto operand=load(b);return a<operand;}
} // namespace x87

// ------------------------------------------------------- profiler timers
// HK_TIMER_BEGIN as these owners expand it: when the profiler's stream has
// room (an unsigned comparison), the event is the timer's name and the low
// half of the time stamp counter (which the macro keeps in a local), and the
// stream advances by 12 bytes. (collection_agent::timer_event is the same
// macro with 304350's roles.)
namespace monitor {
template<class A> struct TimerRoles {A next,end,name,time_store,time_load,time,advance;};
template<class Memory,class Frame,class A> void timer_event(Memory &memory,Frame &frame,std::uint32_t name,std::uint32_t time_local,
    const TimerRoles<A> &a) {
    const auto next=memory.load32(a.next,collection_agent::globals::monitor_next);
    const auto end=memory.load32(a.end,collection_agent::globals::monitor_end);
    if(next>=end)return;
    memory.store32(a.name,next,name);
    memory.store32(a.time_store,time_local,frame.time_stamp_counter());
    memory.store32(a.time,at(next,4),memory.load32(a.time_load,time_local));
    memory.store32(a.advance,collection_agent::globals::monitor_next,next+12u);
}
} // namespace monitor

// --------------------------------------------------------- convex pairs
// Havok's GSK agent finds the closest points between the convex shapes of two
// bodies and keeps, between steps, a simplex cache and a separating plane
// (a normal, and in w the distance by which the bodies are known to be apart).
// 2F8630 is the agent's closest-point query; 2E4FE0 wraps it for the game's
// swept shapes. These are the layouts both read.
namespace convex_pair {
namespace globals {
inline constexpr std::uint32_t plane_bound=0x46a0e8u;            // float: the plane is trusted while its distance is greater
inline constexpr std::uint32_t cached_plane_tolerance=0x46a190u; // float: the cached plane gives up this much of the tolerance
inline constexpr std::uint32_t plane_timer_name=0x41cd3cu;       // the profiler timer names of 2F8630's two stages
inline constexpr std::uint32_t search_timer_name=0x41cd34u;
}

// A body's motion as these owners read it (CdBody::motion): its centre before
// and after the step (each an hkVector4), then the transform (at +20, as
// collection_agent::motion_rotation says). The w of the first centre is added
// to the distance the bodies may have closed in the step: a bound on the
// movement the centres do not show (its exact meaning is not established).
struct BodyMotion {
    Lanes centre_before;
    Lanes centre_after;
    HavokTransform transform;
};
static_assert(offsetof(BodyMotion,centre_after)==0x10 && offsetof(BodyMotion,transform)==collection_agent::motion_rotation);
inline constexpr std::uint32_t movement_bound_offset=offsetof(BodyMotion,centre_before)+12u;

// A convex shape: its convex radius at +C (as surface_polygon's header).
struct ConvexShape {
    std::uint32_t vtable;
    std::uint32_t not_used_4,not_used_8;
    float radius;
};
static_assert(offsetof(ConvexShape,radius)==0xc);
// The game's swept shape (getType() 0x17): a convex shape swept along its
// owner's movement in the step. 2E4FE0 writes the sweep (in the body's frame)
// before each query; the shape sweeps only while its owner's squared speed is
// greater than the threshold.
struct SweptShape {
    ConvexShape convex;
    Lanes sweep;
    float speed_squared_threshold;
};
static_assert(offsetof(SweptShape,sweep)==0x10 && offsetof(SweptShape,speed_squared_threshold)==0x20);
inline constexpr std::uint32_t swept_shape_type=0x17u;
// Shape B's type that 2E4FE0 sets up the simplex cache for through 309150
// (the others: 309060). The class is not established.
inline constexpr std::uint32_t cache_type_5=5u;
// The root CdBody (the collidable) keeps its owner at +20; the owner's object
// at +3C holds the vector at +40 that 2E4FE0 scales by the step's time, read
// here as the linear velocity (the classes are not established).
inline constexpr std::uint32_t collidable_owner=0x20u,owner_motion=0x3cu,motion_velocity=0x40u;

// hkpProcessCollisionInput as these owners read it.
struct CollisionInput {
    std::uint32_t dispatcher,not_used_4;
    float tolerance;
    std::uint32_t settings;  // the collision settings; their first byte enables caching the separating plane
    float step_time;         // multiplies a velocity, so read as the step's duration (2E4FE0)
};
static_assert(offsetof(CollisionInput,tolerance)==0x8 && offsetof(CollisionInput,settings)==0xc &&
              offsetof(CollisionInput,step_time)==0x10);

// The GSK agent (this) as these owners read it.
struct GskAgent {
    std::uint32_t vtable;
    std::uint32_t not_used_4,not_used_8;
    std::uint32_t cache[3];           // +C: the simplex cache (its extent is not established)
    std::uint32_t plane_not_cached;   // +18: non-zero, 2F8630 does not cache the plane (not established)
    std::uint32_t not_used_1c;
    Lanes separating_plane;           // +20: the normal; w (+2C) the distance
    std::uint8_t not_used_30[0xc0];
    std::uint32_t sweep_flags;        // +F0: the shapes 2E4FE0 swept in the previous step (bit 0 A, bit 1 B)
};
static_assert(offsetof(GskAgent,cache)==0xc && offsetof(GskAgent,plane_not_cached)==0x18 &&
              offsetof(GskAgent,separating_plane)==0x20 && offsetof(GskAgent,sweep_flags)==0xf0);
inline constexpr std::uint32_t plane_distance_offset=offsetof(GskAgent,separating_plane)+12u;
inline constexpr std::uint32_t no_plane=0xbf800000u;  // -1.0f: the plane's distance that forces the search

// hkpExtendedGskOut as 2F8630 fills it. 30D350 writes the normal and A's point
// in A's frame and the distance between the shapes' cores; 2F8630 moves A's
// point onto A's surface, writes the distance between the surfaces and B's
// point in B's frame; 2E4760 writes the normal in world space.
struct ClosestPointOutput {
    Lanes normal_in_a;
    Lanes point_a;
    float distance;
    std::uint32_t not_used_24[3];
    Lanes point_b;
    Lanes normal;
};
static_assert(offsetof(ClosestPointOutput,distance)==0x20 && offsetof(ClosestPointOutput,point_b)==0x30 &&
              offsetof(ClosestPointOutput,normal)==0x40);
} // namespace convex_pair

// --------------------------------------------------------- closest point
// 2F8630: the closest point between the convex shapes of bodies A and B for
// the GSK agent (thiscall; the result is an hkBool through its pointer):
//   1. while the separating plane's distance is greater than the bound, it is
//      reduced by how far the bodies may have closed along its normal: the
//      change of B's centre less the change of A's, along the normal (Havok's
//      normals point from B to A), plus both movement bounds; the reduced
//      distance is kept in the agent, and while it stays greater there is no
//      contact;
//   2. otherwise the search: B's transform in A's frame (2E4550), the closest
//      features on the agent's simplex cache (30E930; non-zero: no contact),
//      the closest points (30D350, into the output) and the normal in world
//      space (2E4760);
//   3. the distance between the two surfaces is the cores' distance less both
//      convex radii; when it is less than the input's tolerance, A's point
//      moves onto A's surface, B's point (A's point moved back by the
//      distance) goes into B's frame, and there is a contact;
//   4. otherwise, when the agent and the input's settings allow it, the plane
//      is cached: the world normal, with the distance less a fraction of the
//      tolerance.
// Each stage writes a profiler timer event when the stream has room. The x87
// values are the original's: the reduced plane distance is compared before it
// is rounded into the agent, and the surfaces' distance after it is rounded.
namespace closest_point {
using collection_agent::CdBody;
using convex_pair::BodyMotion;using convex_pair::ConvexShape;using convex_pair::CollisionInput;
using convex_pair::GskAgent;using convex_pair::ClosestPointOutput;
namespace globals=convex_pair::globals;

// The addresses of the arguments and of the frame's locals (the adapter's).
// Several locals share a slot in turn.
struct Locals {
    std::uint32_t result,body_a,body_b,input,output;  // the arguments
    std::uint32_t time_stamp;   // the timer macro's copy of the time stamp
    std::uint32_t closing;      // the closing distance along the plane's normal (the same slot)
    std::uint32_t world_normal; // the output's world normal's address (the same slot)
    std::uint32_t negated;      // the negated radius, then the negated distance (the same slot)
    std::uint32_t shape_a;      // shape A; then the surfaces' distance
    std::uint32_t gap;          // (shape A's slot)
    std::uint32_t shape_b;
    std::uint32_t separation;   // 16 bytes 30E930 writes and 30D350 reads (not established)
    std::uint32_t relative;     // hkTransform: B's transform in A's frame
    std::uint32_t gsk;          // 30E930's and 30D350's object; its byte at +C is cleared first
};
inline constexpr std::uint32_t gsk_cleared_byte=0xcu;

enum class Access {
    body_a_argument,body_a_shape,plane_distance,body_b_argument,plane_bound,body_b_shape,shape_a_keep,shape_b_keep,
    // the separating plane
    plane_next,plane_end,plane_name,plane_time_store,plane_time_load,plane_time,plane_advance,
    motion_a,a_before,motion_b,a_after,b_after,plane_normal,motion_b_again,b_bound,b_before,closing_store,
    motion_a_again,a_bound,closing_load,plane_distance_again,plane_distance_store,plane_bound_again,
    // the search
    search_next,search_end,search_name,search_time_store,search_time_load,search_time,search_advance,
    search_motion_b,search_motion_a,search_shape_a,search_shape_b,gsk_clear,output_argument,normal_motion,
    world_normal_keep,
    // the surfaces' distance
    core_distance,gap_shape_a,radius_a,gap_shape_b,radius_b,input_argument,gap_store,gap_bits,gap_load,
    distance_store,tolerance,
    // a contact
    contact_radius,contact_normal,relative_column_2,relative_column_1,negated_radius_store,negated_radius_load,
    contact_gap,result_argument,negated_gap_store,contact_point,relative_translation,point_a_store,
    negated_gap_load,contact_normal_again,contact_point_again,relative_column_0,point_b_store,contact_result,
    // the plane cached
    plane_not_cached,settings,caching_enabled,world_normal_reload,world_normal,plane_normal_store,
    cache_tolerance,cache_fraction,cache_gap,cache_distance_store,
    // no contact
    no_contact_result_argument,no_contact_result
};
inline constexpr monitor::TimerRoles<Access> plane_timer{Access::plane_next,Access::plane_end,Access::plane_name,
    Access::plane_time_store,Access::plane_time_load,Access::plane_time,Access::plane_advance};
inline constexpr monitor::TimerRoles<Access> search_timer{Access::search_next,Access::search_end,Access::search_name,
    Access::search_time_store,Access::search_time_load,Access::search_time,Access::search_advance};

// The plane's reduction in SSE (the closing distance), for the adapter's
// registers.
struct PlaneCheck {
    Vector4 change_b;       // B's centre after less before
    Vector4 closing_terms;  // (B's change less A's) * the normal
    float closing_yx;       // y + x of the terms
    float closing;          // z + (y + x)
};
enum class Outcome {plane_apart,features_apart,contact,not_touching};
struct Result {
    Outcome outcome;
    PlaneCheck plane;                         // when the plane was reduced
    havok_rotation::InverseRotated point_b;   // a contact: B's point into B's frame
};

// The plane's distance less how far the bodies may have closed along its
// normal in the step. True while the plane still separates them.
template<class Memory> bool reduce_plane(Memory &memory,const Locals &local,std::uint32_t agent,std::uint32_t body_a,
    std::uint32_t body_b,PlaneCheck &check) {
    using enum Access;
    const auto a=memory.load32(motion_a,at(body_a,offsetof(CdBody,motion)));
    const auto a_start=memory.load_vector(a_before,at(a,offsetof(BodyMotion,centre_before)));
    const auto b=memory.load32(motion_b,at(body_b,offsetof(CdBody,motion)));
    const auto a_end=memory.load_vector(a_after,at(a,offsetof(BodyMotion,centre_after)));
    const auto b_end=memory.load_vector(b_after,at(b,offsetof(BodyMotion,centre_after)));
    const auto normal=memory.load_vector(plane_normal,at(agent,offsetof(GskAgent,separating_plane)));
    const auto b_again=memory.load32(motion_b_again,at(body_b,offsetof(CdBody,motion)));
    auto closed=x87::load(memory.load_float(b_bound,at(b_again,convex_pair::movement_bound_offset)));
    const auto change_a=sse::subtract(a_end,a_start);
    check.change_b=sse::subtract(b_end,memory.load_vector(b_before,at(b,offsetof(BodyMotion,centre_before))));
    check.closing_terms=sse::multiply(sse::subtract(check.change_b,change_a),normal);
    std::tie(check.closing_yx,check.closing)=manifold_report::sum_yx_z(check.closing_terms);
    memory.store_float(closing_store,local.closing,check.closing);
    const auto a_again=memory.load32(motion_a_again,at(body_a,offsetof(CdBody,motion)));
    // (B's bound + A's bound) + the closing distance, on the x87 stack.
    closed=x87::add(closed,memory.load_float(a_bound,at(a_again,convex_pair::movement_bound_offset)));
    closed=x87::add(closed,memory.load_float(closing_load,local.closing));
    const auto reduced=x87::subtract_from(memory.load_float(plane_distance_again,at(agent,convex_pair::plane_distance_offset)),closed);
    memory.store_float(plane_distance_store,at(agent,convex_pair::plane_distance_offset),x87::store(reduced));  // FST: kept on the stack
    return x87::greater(reduced,memory.load_float(plane_bound_again,globals::plane_bound));
}

// A contact: A's point moved onto A's surface (by the radius against the
// normal), and B's point, that point moved by the surfaces' distance against
// the normal, into B's frame (the relative transform's inverse).
template<class Memory> havok_rotation::InverseRotated contact(Memory &memory,const Locals &local,std::uint32_t output,
    std::uint32_t shape_a) {
    using enum Access;
    using havok_rotation::Rotation;
    const auto radius=x87::load(memory.load_float(contact_radius,at(shape_a,offsetof(ConvexShape,radius))));
    const auto normal=memory.load_vector(contact_normal,at(output,offsetof(ClosestPointOutput,normal_in_a)));
    Rotation relative;
    relative.column[2]=memory.load_vector(relative_column_2,at(local.relative,offsetof(HavokTransform,column[2])));
    relative.column[1]=memory.load_vector(relative_column_1,at(local.relative,offsetof(HavokTransform,column[1])));
    memory.store_float(negated_radius_store,local.negated,x87::store(x87::negate(radius)));
    const auto radius_step=manifold_report::broadcast(memory.load_float(negated_radius_load,local.negated));
    const auto gap=x87::load(memory.load_float(contact_gap,local.gap));
    const auto result=memory.load32(result_argument,local.result);
    memory.store_float(negated_gap_store,local.negated,x87::store(x87::negate(gap)));
    const auto point=memory.load_vector(contact_point,at(output,offsetof(ClosestPointOutput,point_a)));
    const auto translation=memory.load_vector(relative_translation,at(local.relative,offsetof(HavokTransform,translation)));
    memory.store_vector(point_a_store,at(output,offsetof(ClosestPointOutput,point_a)),
        sse::add(point,sse::multiply(radius_step,normal)));
    const auto gap_step=manifold_report::broadcast(memory.load_float(negated_gap_load,local.negated));
    const auto normal_again=memory.load_vector(contact_normal_again,at(output,offsetof(ClosestPointOutput,normal_in_a)));
    const auto on_surface=memory.load_vector(contact_point_again,at(output,offsetof(ClosestPointOutput,point_a)));
    relative.column[0]=memory.load_vector(relative_column_0,at(local.relative,offsetof(HavokTransform,column[0])));
    const auto in_a=sse::subtract(sse::add(on_surface,sse::multiply(gap_step,normal_again)),translation);
    const auto in_b=havok_rotation::rotate_inverse(relative,in_a);
    memory.store_vector(point_b_store,at(output,offsetof(ClosestPointOutput,point_b)),in_b.value);
    memory.store8(contact_result,result,1u);
    return in_b;
}

// No contact: the plane is cached when the agent and the settings allow it.
template<class Memory> void cache_plane(Memory &memory,const Locals &local,std::uint32_t agent,std::uint32_t input) {
    using enum Access;
    if(memory.load32(plane_not_cached,at(agent,offsetof(GskAgent,plane_not_cached))))return;
    if(!memory.load8(caching_enabled,memory.load32(settings,at(input,offsetof(CollisionInput,settings)))))return;
    const auto normal=memory.load_vector(world_normal,memory.load32(world_normal_reload,local.world_normal));
    memory.store_vector(plane_normal_store,at(agent,offsetof(GskAgent,separating_plane)),normal);
    // The gap less tolerance * the fraction, over the normal's w.
    auto given_up=x87::load(memory.load_float(cache_tolerance,at(input,offsetof(CollisionInput,tolerance))));
    given_up=x87::multiply(given_up,memory.load_float(cache_fraction,globals::cached_plane_tolerance));
    const auto distance=x87::subtract_from(memory.load_float(cache_gap,local.gap),given_up);
    memory.store_float(cache_distance_store,at(agent,convex_pair::plane_distance_offset),x87::store(distance));
}

template<class Memory> void no_contact(Memory &memory,const Locals &local) {
    memory.store8(Access::no_contact_result,memory.load32(Access::no_contact_result_argument,local.result),0u);
}

// Frame (the adapter's): locals(), time_stamp_counter() (RDTSC),
// relative_transform() (2E4550), closest_features() (30E930: non-zero when
// the shapes are apart), closest_points() (30D350) and rotate_into_world()
// (2E4760).
template<class Memory,class Frame> Result find(Memory &memory,Frame &frame,std::uint32_t agent) {
    using enum Access;
    const auto &local=frame.locals();
    Result result{};
    const auto body_a=memory.load32(body_a_argument,local.body_a);
    const auto shape_a=memory.load32(body_a_shape,at(body_a,offsetof(CdBody,shape)));
    const auto distance=x87::load(memory.load_float(plane_distance,at(agent,convex_pair::plane_distance_offset)));
    const auto body_b=memory.load32(body_b_argument,local.body_b);
    const bool plane_trusted=x87::greater(distance,memory.load_float(plane_bound,globals::plane_bound));
    const auto shape_b=memory.load32(body_b_shape,at(body_b,offsetof(CdBody,shape)));
    memory.store32(shape_a_keep,local.shape_a,shape_a);
    memory.store32(shape_b_keep,local.shape_b,shape_b);
    if(plane_trusted) {
        monitor::timer_event(memory,frame,globals::plane_timer_name,local.time_stamp,plane_timer);
        if(reduce_plane(memory,local,agent,body_a,body_b,result.plane)) {
            no_contact(memory,local);
            result.outcome=Outcome::plane_apart;
            return result;
        }
    }
    monitor::timer_event(memory,frame,globals::search_timer_name,local.time_stamp,search_timer);
    const auto b_moving=memory.load32(search_motion_b,at(body_b,offsetof(CdBody,motion)));
    const auto a_moving=memory.load32(search_motion_a,at(body_a,offsetof(CdBody,motion)));
    frame.relative_transform(local.relative,at(a_moving,offsetof(BodyMotion,transform)),at(b_moving,offsetof(BodyMotion,transform)));
    const auto core_a=memory.load32(search_shape_a,local.shape_a);
    const auto core_b=memory.load32(search_shape_b,local.shape_b);
    memory.store8(gsk_clear,at(local.gsk,gsk_cleared_byte),0u);
    if(frame.closest_features(local.gsk,core_a,core_b,local.relative,at(agent,offsetof(GskAgent,cache)),local.separation)) {
        no_contact(memory,local);
        result.outcome=Outcome::features_apart;
        return result;
    }
    const auto output=memory.load32(output_argument,local.output);
    frame.closest_points(local.gsk,at(agent,offsetof(GskAgent,cache)),local.separation,output);
    const auto motion=memory.load32(normal_motion,at(body_a,offsetof(CdBody,motion)));
    const auto normal=at(output,offsetof(ClosestPointOutput,normal));
    memory.store32(world_normal_keep,local.world_normal,normal);
    frame.rotate_into_world(normal,at(motion,offsetof(BodyMotion,transform)),at(output,offsetof(ClosestPointOutput,normal_in_a)));
    // The surfaces' distance: the cores' distance less both radii, rounded.
    auto gap=x87::load(memory.load_float(core_distance,at(output,offsetof(ClosestPointOutput,distance))));
    const auto radius_shape_a=memory.load32(gap_shape_a,local.shape_a);
    gap=x87::subtract(gap,memory.load_float(radius_a,at(radius_shape_a,offsetof(ConvexShape,radius))));
    const auto radius_shape_b=memory.load32(gap_shape_b,local.shape_b);
    gap=x87::subtract(gap,memory.load_float(radius_b,at(radius_shape_b,offsetof(ConvexShape,radius))));
    const auto input=memory.load32(input_argument,local.input);
    memory.store_float(gap_store,local.gap,x87::store(gap));
    const auto bits=memory.load32(gap_bits,local.gap);
    const auto rounded=x87::load(memory.load_float(gap_load,local.gap));
    memory.store32(distance_store,at(output,offsetof(ClosestPointOutput,distance)),bits);
    if(x87::less(rounded,memory.load_float(tolerance,at(input,offsetof(CollisionInput,tolerance))))) {
        result.point_b=contact(memory,local,output,radius_shape_a);
        result.outcome=Outcome::contact;
        return result;
    }
    cache_plane(memory,local,agent,input);
    no_contact(memory,local);
    result.outcome=Outcome::not_touching;
    return result;
}
} // namespace closest_point

// ----------------------------------------------------------- swept pair
// 2E4FE0: the closest point between two bodies' convex shapes when either may
// be the game's swept shape (thiscall on the GSK agent; the result is an
// hkBool through its pointer):
//   1. each swept shape's sweep is set for the step: when its owner's squared
//      speed is greater than the shape's threshold and the game's predicate
//      (283380, on the owner) agrees, the owner's velocity rotated into the
//      body's frame and scaled by the step's time; otherwise zero. The flags
//      record which shapes swept (bit 0 A, bit 1 B);
//   2. when the flags differ from the previous step's, the input's settings
//      may not cache the separating plane in this query;
//   3. when the flags changed or a shape swept, the agent's cache no longer
//      holds: B's transform in A's frame (2E4550), the simplex cache set up
//      again for the two shapes (309150 when B's shape is of type 5, 309060
//      otherwise) and the plane's distance set to -1;
//   4. the query is 2F8630; the flags are kept in the agent, and the
//      settings' byte is restored.
namespace swept_pair {
using collection_agent::CdBody;
using convex_pair::BodyMotion;using convex_pair::SweptShape;using convex_pair::CollisionInput;using convex_pair::GskAgent;

struct Locals {
    std::uint32_t result,body_a,body_b,input,output;  // the arguments
    std::uint32_t settings_byte;  // the settings' first byte, restored at the end
    std::uint32_t found;          // the hkBool 2F8630 writes
    std::uint32_t agent;          // this
    std::uint32_t sweep_flags;
    std::uint32_t swept;          // byte: a shape swept; then shape A (the same slot)
    std::uint32_t shape_a;
    std::uint32_t speed;          // the squared speed, then A's step time (the same slot)
    std::uint32_t owner_b;        // B's owner, then B's step time (the same slot)
    std::uint32_t relative;       // hkTransform: B's transform in A's frame
};
enum class Side {a,b};
// The shape type queries, by call site.
enum class TypeQuery {shape_a,shape_b,cache_b};

enum class Access {
    input_argument,body_a_argument,agent_keep,settings,settings_byte,shape_a,vtable_a,swept_clear,settings_byte_keep,
    sweep_flags_clear,
    // shape A swept
    a_parent,a_swept_shape,a_parent_next,a_owner,a_owner_motion,a_velocity,a_speed_store,a_speed_load,a_threshold,
    a_owner_motion_again,a_motion,a_column_2,a_velocity_again,a_column_0,a_column_1,a_input,a_sweep_store,a_step_time,
    a_sweep_load,a_step_time_store,a_step_time_load,a_sweep_scaled_store,a_flags_store,a_swept_store,a_sweep_zero,
    // shape B
    body_b_argument,shape_b,vtable_b,
    b_parent,b_swept_shape,b_parent_next,b_owner,b_owner_motion,b_velocity,b_speed_store,b_speed_load,b_threshold,
    b_owner_keep,b_motion,b_column_2,b_column_0,b_column_1,b_owner_reload,b_owner_motion_again,b_velocity_again,
    b_input,b_sweep_store,b_step_time,b_sweep_load,b_step_time_store,b_step_time_load,b_flags_load,
    b_sweep_scaled_store,b_flags_store,b_swept_store,b_sweep_zero,
    // the cache
    agent_reload,flags_reload,previous_flags,changed_input,changed_settings,settings_byte_clear,swept_load,
    reset_motion_b,reset_shape_a,reset_motion_a,reset_shape_b,shape_a_keep,reset_vtable_b,
    cache_5_shape_a,cache_5_agent,cache_shape_a,cache_agent,plane_agent,plane_reset,
    // the query
    output_argument,query_input,flags_final,input_final,flags_store,settings_final,settings_byte_reload,
    result_argument,settings_byte_restore,found_load,result_store
};

// One shape's speed (SSE), for the adapter's registers.
struct Speed {
    Vector4 squared;    // velocity * velocity
    float yx,value;     // y + x, then z + (y + x)
};
// A sweep that was set: the velocity into the body's frame, then scaled.
struct Sweep {
    havok_rotation::InverseRotated rotated;
    Vector4 loaded;     // the rotated velocity, read back
    float step_time;
    Vector4 scaled;
};

inline constexpr std::uint32_t swept_a=1u,swept_b=2u;  // the sweep flags

// The squared speed: velocity * velocity, summed as y + x, then z + (y + x).
inline Speed speed_of(Vector4 velocity) {
    Speed speed{};
    speed.squared=sse::multiply(velocity,velocity);
    std::tie(speed.yx,speed.value)=manifold_report::sum_yx_z(speed.squared);
    return speed;
}
// The root CdBody (the collidable) at the end of a body's parent chain.
template<class Memory> std::uint32_t collidable(Memory &memory,std::uint32_t body,std::uint32_t parent,Access next) {
    auto root=body;
    while(parent){root=parent;parent=memory.load32(next,at(root,offsetof(CdBody,parent)));}
    return root;
}
// The sweep set: the velocity rotated into the body's frame (stored), read
// back and scaled by the step's time (MOVSS, broadcast, then times the
// sweep).
template<class Memory> Sweep scaled_sweep(Memory &memory,const havok_rotation::Rotation &rotation,Vector4 velocity,
    std::uint32_t shape,std::uint32_t input,std::uint32_t time_slot,Access stored,Access time,Access reloaded,
    Access time_kept,Access time_loaded) {
    Sweep sweep{};
    const auto at_sweep=at(shape,offsetof(SweptShape,sweep));
    sweep.rotated=havok_rotation::rotate_inverse(rotation,velocity);
    memory.store_vector(stored,at_sweep,sweep.rotated.value);
    const auto bits=memory.load32(time,at(input,offsetof(CollisionInput,step_time)));
    sweep.loaded=memory.load_vector(reloaded,at_sweep);
    memory.store32(time_kept,time_slot,bits);
    sweep.step_time=memory.load_float(time_loaded,time_slot);
    sweep.scaled=sse::multiply(manifold_report::broadcast(sweep.step_time),sweep.loaded);
    return sweep;
}

// Body A's swept shape. The owner stays in a register; the transform's
// columns and the velocity are read interleaved.
template<class Memory,class Frame> void sweep_a(Memory &memory,Frame &frame,const Locals &local,std::uint32_t body) {
    using enum Access;
    const auto parent=memory.load32(a_parent,at(body,offsetof(CdBody,parent)));
    const auto shape=memory.load32(a_swept_shape,at(body,offsetof(CdBody,shape)));
    const auto owner=memory.load32(a_owner,at(collidable(memory,body,parent,a_parent_next),convex_pair::collidable_owner));
    const auto moving=memory.load32(a_owner_motion,at(owner,convex_pair::owner_motion));
    const auto speed=speed_of(memory.load_vector(a_velocity,at(moving,convex_pair::motion_velocity)));
    memory.store_float(a_speed_store,local.speed,speed.value);
    const auto squared=x87::load(memory.load_float(a_speed_load,local.speed));
    const bool fast=x87::greater(squared,memory.load_float(a_threshold,at(shape,offsetof(SweptShape,speed_squared_threshold))));
    frame.measured(Side::a,speed);
    if(!fast || !frame.may_sweep(Side::a,owner)) {
        memory.store_vector(a_sweep_zero,at(shape,offsetof(SweptShape,sweep)),_mm_setzero_ps());
        return;
    }
    const auto moving_again=memory.load32(a_owner_motion_again,at(owner,convex_pair::owner_motion));
    const auto transform=at(memory.load32(a_motion,at(body,offsetof(CdBody,motion))),offsetof(BodyMotion,transform));
    havok_rotation::Rotation rotation;
    rotation.column[2]=memory.load_vector(a_column_2,at(transform,offsetof(HavokTransform,column[2])));
    const auto velocity=memory.load_vector(a_velocity_again,at(moving_again,convex_pair::motion_velocity));
    rotation.column[0]=memory.load_vector(a_column_0,at(transform,offsetof(HavokTransform,column[0])));
    rotation.column[1]=memory.load_vector(a_column_1,at(transform,offsetof(HavokTransform,column[1])));
    const auto input=memory.load32(a_input,local.input);
    const auto sweep=scaled_sweep(memory,rotation,velocity,shape,input,local.speed,a_sweep_store,a_step_time,a_sweep_load,
        a_step_time_store,a_step_time_load);
    memory.store_vector(a_sweep_scaled_store,at(shape,offsetof(SweptShape,sweep)),sweep.scaled);
    memory.store32(a_flags_store,local.sweep_flags,swept_a);
    memory.store8(a_swept_store,local.swept,1u);
    frame.swept(Side::a,sweep);
}

// Body B's swept shape. The owner is kept in the frame across 283380 and
// re-read after the transform's columns; the flags gain bit 1.
template<class Memory,class Frame> void sweep_b(Memory &memory,Frame &frame,const Locals &local,std::uint32_t body) {
    using enum Access;
    const auto parent=memory.load32(b_parent,at(body,offsetof(CdBody,parent)));
    const auto shape=memory.load32(b_swept_shape,at(body,offsetof(CdBody,shape)));
    const auto owner=memory.load32(b_owner,at(collidable(memory,body,parent,b_parent_next),convex_pair::collidable_owner));
    const auto moving=memory.load32(b_owner_motion,at(owner,convex_pair::owner_motion));
    const auto speed=speed_of(memory.load_vector(b_velocity,at(moving,convex_pair::motion_velocity)));
    memory.store_float(b_speed_store,local.speed,speed.value);
    const auto squared=x87::load(memory.load_float(b_speed_load,local.speed));
    const bool fast=x87::greater(squared,memory.load_float(b_threshold,at(shape,offsetof(SweptShape,speed_squared_threshold))));
    memory.store32(b_owner_keep,local.owner_b,owner);
    frame.measured(Side::b,speed);
    if(!fast || !frame.may_sweep(Side::b,owner)) {
        memory.store_vector(b_sweep_zero,at(shape,offsetof(SweptShape,sweep)),_mm_setzero_ps());
        return;
    }
    const auto transform=at(memory.load32(b_motion,at(body,offsetof(CdBody,motion))),offsetof(BodyMotion,transform));
    havok_rotation::Rotation rotation;
    rotation.column[2]=memory.load_vector(b_column_2,at(transform,offsetof(HavokTransform,column[2])));
    rotation.column[0]=memory.load_vector(b_column_0,at(transform,offsetof(HavokTransform,column[0])));
    rotation.column[1]=memory.load_vector(b_column_1,at(transform,offsetof(HavokTransform,column[1])));
    const auto kept_owner=memory.load32(b_owner_reload,local.owner_b);
    const auto moving_again=memory.load32(b_owner_motion_again,at(kept_owner,convex_pair::owner_motion));
    const auto velocity=memory.load_vector(b_velocity_again,at(moving_again,convex_pair::motion_velocity));
    const auto input=memory.load32(b_input,local.input);
    const auto sweep=scaled_sweep(memory,rotation,velocity,shape,input,local.owner_b,b_sweep_store,b_step_time,b_sweep_load,
        b_step_time_store,b_step_time_load);
    const auto flags=memory.load32(b_flags_load,local.sweep_flags)+swept_b;
    memory.store_vector(b_sweep_scaled_store,at(shape,offsetof(SweptShape,sweep)),sweep.scaled);
    memory.store32(b_flags_store,local.sweep_flags,flags);
    memory.store8(b_swept_store,local.swept,1u);
    frame.swept(Side::b,sweep);
}

// The agent's cached state no longer holds: B's transform in A's frame, the
// simplex cache set up again for the two shapes, the plane's distance -1.
// Returns the agent, as last read.
template<class Memory,class Frame> std::uint32_t reset_cache(Memory &memory,Frame &frame,const Locals &local,std::uint32_t body_a,
    std::uint32_t body_b) {
    using enum Access;
    const auto motion_b=memory.load32(reset_motion_b,at(body_b,offsetof(CdBody,motion)));
    const auto shape_a_address=memory.load32(reset_shape_a,at(body_a,offsetof(CdBody,shape)));
    const auto motion_a=memory.load32(reset_motion_a,at(body_a,offsetof(CdBody,motion)));
    const auto shape_b_address=memory.load32(reset_shape_b,at(body_b,offsetof(CdBody,shape)));
    memory.store32(shape_a_keep,local.shape_a,shape_a_address);
    frame.relative_transform(local.relative,at(motion_a,offsetof(BodyMotion,transform)),at(motion_b,offsetof(BodyMotion,transform)));
    const auto vtable=memory.load32(reset_vtable_b,shape_b_address);
    if(frame.shape_type(TypeQuery::cache_b,shape_b_address,vtable)==convex_pair::cache_type_5) {
        const auto a=memory.load32(cache_5_shape_a,local.shape_a);
        const auto self=memory.load32(cache_5_agent,local.agent);
        frame.set_up_cache_5(at(self,offsetof(GskAgent,cache)),a,shape_b_address,local.relative);  // 309150
    } else {
        const auto a=memory.load32(cache_shape_a,local.shape_a);
        const auto self=memory.load32(cache_agent,local.agent);
        frame.set_up_cache(at(self,offsetof(GskAgent,cache)),a,shape_b_address,local.relative);  // 309060
    }
    const auto self=memory.load32(plane_agent,local.agent);
    memory.store32(plane_reset,at(self,convex_pair::plane_distance_offset),convex_pair::no_plane);
    return self;
}

// Frame (the adapter's): locals(), shape_type() (the shape's virtual
// getType()), may_sweep() (283380), the observations measured() and swept()
// (the adapter's registers), relative_transform() (2E4550), set_up_cache_5()
// (309150) and set_up_cache() (309060), and closest_point() (2F8630).
template<class Memory,class Frame> bool closest_point(Memory &memory,Frame &frame,std::uint32_t agent) {
    using enum Access;
    const auto &local=frame.locals();
    const auto input=memory.load32(input_argument,local.input);
    const auto body_a=memory.load32(body_a_argument,local.body_a);
    memory.store32(agent_keep,local.agent,agent);
    const auto settings_address=memory.load32(settings,at(input,offsetof(CollisionInput,settings)));
    const auto caching=memory.load8(settings_byte,settings_address);
    const auto shape_a_address=memory.load32(shape_a,at(body_a,offsetof(CdBody,shape)));
    const auto vtable_a_address=memory.load32(vtable_a,shape_a_address);
    memory.store8(swept_clear,local.swept,0u);
    memory.store8(settings_byte_keep,local.settings_byte,caching);
    memory.store32(sweep_flags_clear,local.sweep_flags,0u);
    if(frame.shape_type(TypeQuery::shape_a,shape_a_address,vtable_a_address)==convex_pair::swept_shape_type)
        sweep_a(memory,frame,local,body_a);
    const auto body_b=memory.load32(body_b_argument,local.body_b);
    const auto shape_b_address=memory.load32(shape_b,at(body_b,offsetof(CdBody,shape)));
    const auto vtable_b_address=memory.load32(vtable_b,shape_b_address);
    if(frame.shape_type(TypeQuery::shape_b,shape_b_address,vtable_b_address)==convex_pair::swept_shape_type)
        sweep_b(memory,frame,local,body_b);
    // A change of the sweeps keeps the plane out of the cache this step; a
    // change or a sweep resets the agent's cache.
    auto self=memory.load32(agent_reload,local.agent);
    const auto flags=memory.load32(flags_reload,local.sweep_flags);
    bool reset=true;
    if(flags!=memory.load32(previous_flags,at(self,offsetof(GskAgent,sweep_flags)))) {
        const auto changed=memory.load32(changed_settings,at(memory.load32(changed_input,local.input),offsetof(CollisionInput,settings)));
        memory.store8(settings_byte_clear,changed,0u);
    } else reset=memory.load8(swept_load,local.swept)!=0;
    if(reset)self=reset_cache(memory,frame,local,body_a,body_b);
    const auto output=memory.load32(output_argument,local.output);
    const auto query=memory.load32(query_input,local.input);
    frame.closest_point(self,local.found,body_a,body_b,query,output);  // 2F8630
    const auto kept=memory.load32(flags_final,local.sweep_flags);
    const auto input_again=memory.load32(input_final,local.input);
    memory.store32(flags_store,at(self,offsetof(GskAgent,sweep_flags)),kept);
    const auto settings_again=memory.load32(settings_final,at(input_again,offsetof(CollisionInput,settings)));
    const auto saved=memory.load8(settings_byte_reload,local.settings_byte);
    const auto result=memory.load32(result_argument,local.result);
    memory.store8(settings_byte_restore,settings_again,saved);
    const auto found=memory.load8(found_load,local.found);
    memory.store8(result_store,result,found);
    return found!=0;
}
} // namespace swept_pair
// ------------------------------------------------------- BSP child shapes
// 1F7C10: the child shape a key names, built into the caller's buffer (the
// structure collision shape's getChildShape(key, buffer); thiscall, RET 8;
// the shape's address back, or 0). The key filter (1F76A0) decides first: a
// key that may no longer collide gives an empty shape in the buffer (its
// vtable, one reference, no user data). Otherwise by the key's kind (bits
// 29-31):
//   1  a surface of the structure's collision BSP: its polygon (1F5FE0; no
//      transform, owner -1);
//   2  an instance: the shape 2E88B0 builds in the buffer around the
//      collision object of the instance's definition; the reference the build
//      took is given back (hkReferencedObject::removeReference: at zero the
//      object is destroyed, unless 283450 reports it static, which keeps one
//      reference); then 184E20 places the shape at the instance;
//   5  a surface of the instance definition's collision BSP (1F67C0);
//   any other: 1F7330.
// Kinds 1 and 2 without a buffer give 0 (kind 2 still gives the reference
// back and places nothing at address 10).
namespace bsp_child_shape {
// The structure's globals and the instance and definition layouts are the
// shape key filter's.
namespace globals=shape_key_filter::globals;
using shape_key_filter::instance_size;using shape_key_filter::instance_definition;using shape_key_filter::definition_size;
using shape_key_filter::structure_instances;using shape_key_filter::structure_definitions;
inline constexpr std::uint32_t empty_shape_vtable=0x462180u;
inline constexpr std::uint32_t collision_data=0xb4u;         // the definition's collision data (1F67C0 reads its BSP)
inline constexpr std::uint32_t collision_shape_object=0x40u; // the Havok shape object in that data
inline constexpr std::uint32_t placed_transform=0x10u;       // the built shape's +10, which 184E20 sets (not established)
inline constexpr std::uint32_t no_owner=0xffffffffu;
inline constexpr std::uint32_t structure_surface_kind=1u,instance_kind=2u,instance_surface_kind=5u;
// hkReferencedObject, and the user data of an hkpShape that derives from it.
struct ReferencedObject {
    std::uint32_t vtable;
    std::uint16_t memory_size_and_flags;
    std::uint16_t reference_count;
    std::uint32_t user_data;
};
static_assert(offsetof(ReferencedObject,reference_count)==6 && offsetof(ReferencedObject,user_data)==8);

struct Locals {std::uint32_t key,buffer;};  // the arguments
enum class Access {
    key_argument,rejected_buffer,empty_count,empty_user_data,empty_vtable,
    surface_buffer,collision_bsp,
    instance_structure,instance_list,instance_definition_index,definitions,definition_collision,instance_buffer,
    reference_count,reference_count_store,reference_count_check,reference_kept,object_vtable,
    surface_structure,surface_instances,surface_definitions,surface_definition_index,surface_collision,surface_buffer_5,
    other_buffer
};

// The collision data of the definition of instance (a signed definition
// index; the multiply wraps as IMUL).
template<class Memory> std::uint32_t collision_of(Memory &memory,std::uint32_t instance,std::uint32_t definitions_address,
    Access index_access,Access collision_access) {
    const auto definition=std::uint32_t(std::int32_t(std::int16_t(memory.load16(index_access,at(instance,instance_definition)))));
    return memory.load32(collision_access,definition*definition_size+definitions_address+collision_data);
}

// Kind 2: the instance's shape, built around the definition's collision
// object, which keeps no extra reference.
template<class Memory,class Frame> std::uint32_t instance_shape(Memory &memory,Frame &frame,const Locals &local,std::uint32_t index) {
    using enum Access;
    const auto structure=memory.load32(instance_structure,globals::instanced_structure);
    const auto instance=memory.load32(instance_list,at(structure,structure_instances))+index*instance_size;
    const auto definition=std::uint32_t(std::int32_t(std::int16_t(memory.load16(instance_definition_index,at(instance,instance_definition)))));
    const auto definitions_address=memory.load32(definitions,at(structure,structure_definitions));
    const auto object=memory.load32(definition_collision,definition*definition_size+definitions_address+collision_data)+
        collision_shape_object;
    const auto buffer=memory.load32(instance_buffer,local.buffer);
    const auto shape=buffer?frame.build_instance_shape(buffer,object):0u;  // 2E88B0
    const auto count=at(object,offsetof(ReferencedObject,reference_count));
    memory.store16(reference_count_store,count,memory.load16(reference_count,count)-1u);
    if(memory.load16(reference_count_check,count)==0) {
        if(frame.is_static(object))memory.store16(reference_kept,count,1u);  // 283450
        else frame.destroy(object,memory.load32(object_vtable,object));        // the virtual destructor
    }
    frame.place_at_instance(at(shape,placed_transform),instance);  // 184E20
    return shape;
}

// Frame (the adapter's): locals(), may_collide() (1F76A0), surface_polygon()
// (1F5FE0), build_instance_shape() (2E88B0), is_static() (283450), destroy()
// (the object's virtual destructor), place_at_instance() (184E20),
// instance_surface_polygon() (1F67C0) and other_shape() (1F7330). Each
// returns what the original's EAX holds after the call.
template<class Memory,class Frame> std::uint32_t child_shape(Memory &memory,Frame &frame) {
    using enum Access;
    const auto &local=frame.locals();
    const auto key=memory.load32(key_argument,local.key);
    if(!frame.may_collide(key)) {
        const auto buffer=memory.load32(rejected_buffer,local.buffer);
        if(!buffer)return 0;
        memory.store16(empty_count,at(buffer,offsetof(ReferencedObject,reference_count)),1u);
        memory.store32(empty_user_data,at(buffer,offsetof(ReferencedObject,user_data)),0u);
        memory.store32(empty_vtable,buffer,empty_shape_vtable);
        return buffer;
    }
    const auto parts=shape_key_filter::unpack(key);
    switch(parts.kind) {
    case structure_surface_kind: {
        const auto buffer=memory.load32(surface_buffer,local.buffer);
        if(!buffer)return 0;
        const auto bsp=memory.load32(collision_bsp,globals::collision_structure);
        return frame.surface_polygon(buffer,bsp,parts.kind,no_owner,parts.index,key);  // 1F5FE0
    }
    case instance_kind:return instance_shape(memory,frame,local,parts.index);
    case instance_surface_kind: {
        const auto structure=memory.load32(surface_structure,globals::instanced_structure);
        const auto instances=memory.load32(surface_instances,at(structure,structure_instances));
        const auto definitions_address=memory.load32(surface_definitions,at(structure,structure_definitions));
        const auto instance=parts.index*instance_size+instances;
        const auto collision=collision_of(memory,instance,definitions_address,surface_definition_index,surface_collision);
        const auto buffer=memory.load32(surface_buffer_5,local.buffer);
        return frame.instance_surface_polygon(buffer,collision,parts.secondary,key,instance);  // 1F67C0
    }
    default:break;
    }
    return frame.other_shape(key,memory.load32(other_buffer,local.buffer));  // 1F7330
}
} // namespace bsp_child_shape
// --------------------------------------------------- structure AABB query
// 1F6D10: the structure collision shape's queryAabb(aabb, hits) (thiscall,
// RET 8): the keys of the child shapes that may overlap an AABB, appended to
// an hkArray<hkpShapeKey>.
//   1. The structure's own MOPP is queried (2E5BC0) and the keys that may no
//      longer collide are removed (1F26B0, through the key filter 1F76A0).
//   2. The keys are then walked. An instance key (kind 2) whose definition
//      has at most 0x2000 collision surfaces (a surface must fit the 13-bit
//      secondary index of a key) is expanded into keys of its surfaces:
//        - the instance's matrix becomes an hkTransform in the frame, and the
//          AABB a box (identity rotation, the AABB's centre; half its size as
//          the extents) made relative to it by 2E4550;
//        - the instance key is removed;
//        - the definition's MOPP shape (+50 of its collision data) appends the
//          keys of its surfaces in that box (its queryObb, vtable +2C, with a
//          tolerance of 1e-4);
//        - each new key k becomes an instance surface key (kind 5:
//          ((k | FFFFA000) << 16) | the instance), which the key filter keeps
//          or removes.
// The keys are walked by index while the array changes: after a removal the
// later keys move down, so the walk resumes at the same index. The appended
// instance surface keys are walked too (and are not kind 2).
namespace structure_aabb_query {
namespace globals {
using namespace shape_key_filter::globals;
inline constexpr std::uint32_t half=0x46a0ccu;  // the factor of the box's half extents (0.5)
inline constexpr std::uint32_t one=0x46a0d0u;   // 1.0f: the box rotation's diagonal
}
using shape_key_filter::instance_size;using shape_key_filter::instance_definition;using shape_key_filter::definition_size;
using shape_key_filter::structure_instances;using shape_key_filter::structure_definitions;using bsp_child_shape::collision_data;
inline constexpr std::uint32_t definition_surface_count=0x98u;  // the surfaces block's count (its elements at +9C)
inline constexpr std::int32_t expandable_surfaces=0x2000;       // the 13 bits of a key's secondary index
inline constexpr std::uint32_t instance_query_shape=0x50u;       // the MOPP shape in the collision data
inline constexpr std::uint32_t query_obb=0x2cu;                  // its vtable's queryObb
inline constexpr std::uint32_t query_tolerance=0x38d1b717u;      // 1e-4f
// The shape's subobject the original spills (as this+14) before calling the
// key filter, which does not read it (not established).
inline constexpr std::uint32_t filter_subobject=0x14u;
inline constexpr std::uint32_t kind_mask=0xe0000000u,instance_kind_bits=0x40000000u,index_mask=0xffffu;
inline constexpr std::uint32_t surface_kind_fill=0xffffa000u;    // the OR before the shift: kind 5 in bits 29-31
// hkArray<hkpShapeKey>.
struct KeyArray {
    std::uint32_t data;
    std::int32_t size;
    std::uint32_t capacity_and_flags;
};
static_assert(offsetof(KeyArray,size)==4);
// hkAabb.
struct Aabb {Lanes min,max;};
static_assert(offsetof(Aabb,max)==0x10);

// The arguments and the frame's locals (addresses; the adapter's).
struct Locals {
    std::uint32_t aabb_argument,hits_argument;
    std::uint32_t shape;               // this, kept across the calls
    std::uint32_t instance_index;      // the walked key's index (an instance's, for kind 2)
    std::uint32_t resume_index;        // the walk's index, kept across the expansion
    std::uint32_t collision;           // the definition's collision data; then filter_subobject's address
    std::uint32_t position;            // the instance's position, staged as (x, y, z, 0)
    std::uint32_t instance_transform;  // hkTransform
    std::uint32_t box_transform;       // hkTransform
    std::uint32_t half_extents;        // hkVector4
    std::uint32_t query_transform;     // hkTransform: the box in the instance's frame
};
enum class Access {
    aabb_argument,hits_argument,shape_store,walk_size,aabb_reload,walk_data,walk_key,index_store,
    structure_global,instances,definition_index,definitions,surface_count,
    matrix_field,box_maximum,collision_pointer,transform_field,half,zero_field,position_staged,translation_store,box_minimum,
    box_column_store,one,collision_store,half_extents_store,one_store,center_store,
    expanded_size,expanded_size_store,expanded_shift_data,expanded_shift_next,expanded_shift_store,expanded_shift_size,
    collision_reload,query_vtable,query_first_new,resume_store,query_function,query_size,shape_reload,subobject_store,
    new_data,new_key,instance_index_reload,accepted_data,accepted_store,
    rejected_size,rejected_size_store,rejected_shift_data,rejected_shift_next,rejected_shift_store,rejected_shift_size,
    new_size,resume_reload,walk_size_next
};

// hkArray::removeAtAndCopy: the size shrinks first, then each later key moves
// down one. The data and the size are re-read for every key moved (the first
// bound is the shrunk size). Indices compare signed. The two sites have their
// own instructions.
struct RemovalAccess {Access size,size_store,data,next,move,size_reload;};
inline constexpr RemovalAccess expanded_removal{Access::expanded_size,Access::expanded_size_store,Access::expanded_shift_data,
    Access::expanded_shift_next,Access::expanded_shift_store,Access::expanded_shift_size};
inline constexpr RemovalAccess rejected_removal{Access::rejected_size,Access::rejected_size_store,Access::rejected_shift_data,
    Access::rejected_shift_next,Access::rejected_shift_store,Access::rejected_shift_size};
template<class Memory> void remove_key(Memory &memory,std::uint32_t hits,std::uint32_t index,const RemovalAccess &role) {
    const auto size_address=at(hits,offsetof(KeyArray,size));
    const auto size=memory.load32(role.size,size_address)-1u;
    memory.store32(role.size_store,size_address,size);
    for(auto bound=size;std::int32_t(index)<std::int32_t(bound);++index) {
        const auto data=memory.load32(role.data,at(hits,offsetof(KeyArray,data)));
        memory.store32(role.move,element(data,index,4),memory.load32(role.next,element(data,index+1u,4)));
        bound=memory.load32(role.size_reload,size_address);
    }
}

// The box of the AABB in the frame of the instance, built in the frame's
// locals for 2E4550. The original copies the instance's twelve floats
// through two registers, so each store trails a later load, and it reads the
// AABB's maximum and the collision data among them; the access order is
// observable (the store observer, a faulting read) and is kept. The rotation's
// columns are the instance's forward, left and up axes (w = 0), and its
// position is staged, then copied as the translation by one MOVAPS.
// Returns the collision data.
template<class Memory> std::uint32_t build_box(Memory &memory,const Locals &local,std::uint32_t aabb,std::uint32_t instance,
    std::uint32_t definition) {
    using enum Access;
    const auto field=[&](std::size_t offset){return memory.load32(matrix_field,at(instance,offset));};
    const auto column=[&](unsigned c,unsigned lane){return at(local.instance_transform,c*sizeof(Lanes)+lane*4u);};
    const auto staged=[&](unsigned lane){return at(local.position,lane*4u);};
    const auto put=[&](std::uint32_t address,std::uint32_t bits){memory.store32(transform_field,address,bits);};
    const auto left_x=field(offsetof(RealMatrix4x3,left[0]));
    const auto forward_x=field(offsetof(RealMatrix4x3,forward[0]));
    const auto maximum=memory.load_vector(box_maximum,at(aabb,offsetof(Aabb,max)));
    const auto collision=memory.load32(collision_pointer,at(definition,collision_data));
    put(column(1,0),left_x);
    put(column(1,1),field(offsetof(RealMatrix4x3,left[1])));
    put(column(1,2),field(offsetof(RealMatrix4x3,left[2])));
    put(column(2,0),field(offsetof(RealMatrix4x3,up[0])));
    put(column(2,1),field(offsetof(RealMatrix4x3,up[1])));
    const auto up_z=field(offsetof(RealMatrix4x3,up[2]));
    put(column(0,0),forward_x);
    const auto forward_y=field(offsetof(RealMatrix4x3,forward[1]));
    put(column(2,2),up_z);
    put(staged(0),field(offsetof(RealMatrix4x3,position[0])));
    const auto position_y=field(offsetof(RealMatrix4x3,position[1]));
    put(column(0,1),forward_y);
    const auto forward_z=field(offsetof(RealMatrix4x3,forward[2]));
    put(staged(1),position_y);
    const auto position_z=field(offsetof(RealMatrix4x3,position[2]));
    put(column(0,2),forward_z);
    put(staged(2),position_z);
    const auto half_factor=memory.load32(half,globals::half);
    for(const auto address:{column(0,3),column(1,3),column(2,3),staged(3)})memory.store32(zero_field,address,0u);
    memory.store_vector(translation_store,at(local.instance_transform,offsetof(HavokTransform,translation)),
        memory.load_vector(position_staged,local.position));
    const auto minimum=memory.load_vector(box_minimum,at(aabb,offsetof(Aabb,min)));
    // Half the AABB's size (the broadcast factor first, as MULPS's
    // destination) and its centre (the minimum first).
    const auto half_extents=sse::multiply(splat(vector({half_factor,0u,0u,0u}),0),sse::subtract(maximum,minimum));
    for(unsigned c=0;c<3;++c)
        memory.store_vector(box_column_store,at(local.box_transform,c*sizeof(Lanes)),vector({0u,0u,0u,0u}));
    const auto unit=memory.load32(one,globals::one);
    memory.store32(collision_store,local.collision,collision);
    const auto center=sse::add(minimum,half_extents);
    memory.store_vector(half_extents_store,local.half_extents,half_extents);
    for(unsigned c=0;c<3;++c)memory.store32(one_store,at(local.box_transform,c*sizeof(Lanes)+c*4u),unit);
    memory.store_vector(center_store,at(local.box_transform,offsetof(HavokTransform,translation)),center);
    return collision;
}

// Expands the instance key at index (see the namespace comment). Returns the
// walk's index for the next step (the removed key's predecessor).
template<class Memory,class Frame> std::uint32_t expand_instance(Memory &memory,Frame &frame,const Locals &local,
    std::uint32_t aabb,std::uint32_t hits,std::uint32_t index,std::uint32_t instance_index) {
    using enum Access;
    const auto structure=memory.load32(structure_global,globals::instanced_structure);
    const auto instance=instance_index*instance_size+memory.load32(instances,at(structure,structure_instances));
    // The definition index is a signed word; the multiply wraps as IMUL.
    const auto offset=std::uint32_t(std::int32_t(std::int16_t(memory.load16(definition_index,at(instance,instance_definition)))))*
        definition_size;
    const auto definition=offset+memory.load32(definitions,at(structure,structure_definitions));
    if(std::int32_t(memory.load32(surface_count,at(definition,definition_surface_count)))>expandable_surfaces)return index;
    build_box(memory,local,aabb,instance,definition);
    frame.relative_transform(local.query_transform,local.instance_transform,local.box_transform);  // 2E4550
    remove_key(memory,hits,index,expanded_removal);
    const auto collision=memory.load32(collision_reload,local.collision);
    const auto query_shape=at(collision,instance_query_shape);
    const auto vtable=memory.load32(query_vtable,query_shape);
    const auto first_new=memory.load32(query_first_new,at(hits,offsetof(KeyArray,size)));
    const auto resume=index-1u;
    memory.store32(resume_store,local.resume_index,resume);
    frame.query_instance(memory.load32(query_function,at(vtable,query_obb)),query_shape,local.query_transform,local.half_extents,
        query_tolerance,hits);
    const auto size_address=at(hits,offsetof(KeyArray,size));
    if(!(std::int32_t(first_new)<std::int32_t(memory.load32(query_size,size_address))))return resume;
    memory.store32(subobject_store,local.collision,memory.load32(shape_reload,local.shape)+filter_subobject);
    for(auto k=first_new;;) {
        const auto surface=memory.load32(new_key,element(memory.load32(new_data,at(hits,offsetof(KeyArray,data))),k,4));
        const auto surface_key=((surface|surface_kind_fill)<<16)|memory.load32(instance_index_reload,local.instance_index);
        const bool kept=frame.may_collide(surface_key);  // 1F76A0
        if(kept)memory.store32(accepted_store,element(memory.load32(accepted_data,at(hits,offsetof(KeyArray,data))),k,4),surface_key);
        else remove_key(memory,hits,k,rejected_removal);
        const auto size=memory.load32(new_size,size_address);
        if(kept)++k;  // a removed key's successor moved into its place
        if(!(std::int32_t(k)<std::int32_t(size)))break;
    }
    return memory.load32(resume_reload,local.resume_index);
}

// Frame (the adapter's): locals(), query_structure() (2E5BC0),
// filter_structure_keys() (1F26B0), relative_transform() (2E4550),
// query_instance() (the definition's queryObb) and may_collide() (1F76A0).
template<class Memory,class Frame> void query_aabb(Memory &memory,Frame &frame,std::uint32_t shape) {
    using enum Access;
    const auto &local=frame.locals();
    auto aabb=memory.load32(aabb_argument,local.aabb_argument);
    const auto hits=memory.load32(hits_argument,local.hits_argument);
    memory.store32(shape_store,local.shape,shape);
    frame.query_structure(shape,aabb,hits);  // 2E5BC0
    frame.filter_structure_keys(shape,hits);  // 1F26B0
    const auto size_address=at(hits,offsetof(KeyArray,size));
    if(std::int32_t(memory.load32(walk_size,size_address))<=0)return;
    for(std::uint32_t index=0;;) {
        const auto key=memory.load32(walk_key,element(memory.load32(walk_data,at(hits,offsetof(KeyArray,data))),index,4));
        memory.store32(index_store,local.instance_index,key&index_mask);
        if((key&kind_mask)==instance_kind_bits)index=expand_instance(memory,frame,local,aabb,hits,index,key&index_mask);
        ++index;
        if(!(std::int32_t(index)<std::int32_t(memory.load32(walk_size_next,size_address))))return;
        aabb=memory.load32(aabb_reload,local.aabb_argument);
    }
}
} // namespace structure_aabb_query

// ------------------------------------------------------ child agent update
// 2F36F0 (thiscall, RET 10; 304350 calls it first): the update of a
// collection agent's child agents. Body B's shape is a bounding-volume tree
// over the child shapes of a collection; the agent keeps one entry per child
// shape near body A: the child's shape key and its collision agent.
//   1. Body A's AABB in body B's frame: body A's transform relative to body
//      B's (2E4550), and the shape's getAabb() with a tolerance of one and a
//      half times the input's (its half is rounded to a float, as the x87
//      FSTP does, before the input's is added).
//   2. While the game enables it ([46E468]), the update is skipped when no
//      face of the AABB moved by more than half the tolerance since the
//      last update (the agent keeps that AABB).
//   3. Otherwise the agent keeps the new AABB, and the tree returns the keys
//      of the children it overlaps (queryAabb, into a local
//      hkInplaceArray<hkpShapeKey,128>).
//   4. The entries are matched with the keys. An entry whose key was not
//      returned has its agent cleaned up (vtable +14) and is removed. A key
//      without an entry gets one: its child shape is built in a local
//      buffer, and when the input's collision filter enables the pair, the
//      function the caller passes creates the agent; otherwise the entry gets
//      the null agent (30BAD0). The game chooses the matching ([48CBB8]):
//        - set: in the order the query returns the keys, editing the entries
//          in place (an entry that does not have the next expected key is
//          searched among all the keys; a new key's entry is inserted at the
//          key's own index);
//        - clear: the keys are sorted (2F33B0) and merged with the entries,
//          which are kept in the same order, into a local hkLocalArray
//          (2F3430 and 2F34D0); the agent's array then receives the merged
//          entries (its storage is replaced by one of exactly their count
//          when it is too small).
//   5. The keys' storage goes back to the thread memory when the query grew
//      the array out of its inline buffer.
// Key comparisons in the merge are unsigned (the keys sort unsigned).
namespace child_agent_update {
namespace globals {
inline constexpr std::uint32_t aabb_cache=0x46e468u;      // byte: the game enables the AABB cache
inline constexpr std::uint32_t query_order=0x48cbb8u;     // byte: match the keys in the query's order (else sorted)
inline constexpr std::uint32_t absolute_mask=0x461950u;   // 7FFFFFFF in every lane
inline constexpr std::uint32_t half=0x46a0ccu;            // 0.5f
using local_buffers::globals::thread_memory;
using collection_agent::globals::null_agent;
}
using collection_agent::CdBody;using collection_agent::ChildEntry;using collection_agent::ShapeHeader;
using contact_array::HavokArray;using structure_aabb_query::Aabb;using structure_aabb_query::KeyArray;
// The agent (this): 304350's agent, and the AABB of body A it last queried
// the tree with.
struct Agent {
    collection_agent::Agent base;
    std::uint32_t not_used_18,not_used_1c;
    Aabb cached;
};
static_assert(offsetof(Agent,base.children)==0xc && offsetof(Agent,cached.min)==0x20 && offsetof(Agent,cached.max)==0x30);
// The collision input's fields used here.
struct Input {
    std::uint32_t dispatcher;
    std::uint32_t filter;  // hkpCollisionFilter
    float tolerance;
};
static_assert(offsetof(Input,filter)==4 && offsetof(Input,tolerance)==8);
// Virtual functions, by vtable offset.
namespace slot {
inline constexpr std::uint32_t shape_aabb=0x18u;         // hkpShape::getAabb(transform, tolerance, aabb)
inline constexpr std::uint32_t query_aabb=0x30u;         // the tree's queryAabb(aabb, hits)
inline constexpr std::uint32_t child_shape=0x34u;        // the container's getChildShape(key, buffer)
inline constexpr std::uint32_t cleanup=0x14u;            // a child agent's clean-up (no argument here)
inline constexpr std::uint32_t collision_enabled=0x0u;   // the filter: isCollisionEnabled(result, input, a, b, container, key)
inline constexpr std::uint32_t allocate=0x10u,deallocate=0x14u;  // the thread memory
}
inline constexpr std::uint32_t inline_keys=0x80u;  // hkInplaceArray<hkpShapeKey,128>
inline constexpr std::uint32_t entry_size=sizeof(ChildEntry);
inline constexpr std::uint32_t key_size=4u;

// The addresses of the arguments and of the locals the original keeps in
// its frame (the adapter supplies them). Several roles share a slot in turn.
struct Locals {
    std::uint32_t body_a,body_b,input,create_agent;  // the arguments
    std::uint32_t agent;              // this
    std::uint32_t half_tolerance;     // float
    std::uint32_t aabb_tolerance;     // getAabb()'s tolerance argument, in its pushed slot
    std::uint32_t slack,moved;        // the cache test: the half tolerance again, the lanes that moved
    std::uint32_t aabb;               // hkAabb
    std::uint32_t container;          // the collection below the tree
    std::uint32_t child_body;         // CdBody of a new child: its parent is body B
    std::uint32_t child_buffer;       // the new child's shape buffer
    std::uint32_t hits,hit_storage;   // hkInplaceArray<hkpShapeKey,128>
    std::uint32_t entries;            // hkLocalArray<ChildEntry> (the sorted merge)
    std::uint32_t a_in_b;             // hkTransform: body A in body B's frame
    std::uint32_t enabled;            // the hkBool the filter writes
    std::uint32_t comparator;         // 2F33B0's comparator argument (its first byte cleared)
    // the matching's state
    std::uint32_t contact_manager;    // the agent's, for the created agents (query order: over `agent`)
    std::uint32_t merge_contact_manager;  // the same (sorted merge)
    std::uint32_t entries_end,keys_end,key_position;
    std::uint32_t merge_keys_end;
    std::uint32_t shift_offset;       // a removal's offset of the next entry moved down
    std::uint32_t insert_entries_end,insert_keys_end,new_size,move_count,move_cursor,move_step;
    std::uint32_t merged_key;
};

enum class Access {
    // body A's AABB
    body_b_argument,body_b_motion,body_a_argument,body_a_motion,agent_store,input_argument,tolerance,body_a_shape,half_factor,
    shape_a_vtable,half_store,half_reload,tolerance_again,margin_store,shape_aabb_function,
    // the cache
    cache_enabled,aabb_minimum,aabb_maximum,slack_read,cached_maximum,absolute_mask_read,slack_store,slack_reload,moved_store,
    cached_minimum,moved_store_again,
    // the query
    cache_minimum_store,cache_maximum_store,tree_read,hits_data_init,hits_size_init,hits_capacity_init,tree_vtable,query_function,
    query_order_read,container_read,body_b_motion_again,child_parent_store,child_motion_store,container_store,
    // the query-order matching: removal
    manager_read,entries_data,keys_data,manager_store,entries_size,keys_size,entries_end_store,keys_end_store,
    entry_key,expected_key,search_start_store,search_entry_key,search_key,search_end_store,found_store,
    entry_agent,child_vtable,cleanup_function,removal_size,removal_data,removal_size_store,removal_size_reload,shift_offset_store,
    shift_data,shift_word0,shift_word0_store,shift_word1,shift_word1_store,shift_word2,shift_word2_store,shift_offset_reload,
    shift_size,shift_offset_store_next,position_reload,entries_end_reload,keys_data_reload,entries_end_store_again,keys_end_reload,
    entries_end_next,
    // the query-order matching: insertion
    insert_entries_size,insert_keys_size,insert_entries_data,insert_keys_size_again,insert_entries_end_store,insert_cursor_store,
    insert_keys_end_store,insert_end_read,insert_entry_key,insert_key,move_count_store,insert_capacity,new_size_store,insert_data,
    move_count_reload,move_first_store,move_first_reload,move_step_store,move_left_store,move_step_reload,move_word0,
    move_word0_store,move_word1,move_word1_store,move_word2,move_word2_store,move_left_reload,move_left_store_next,new_size_reload,
    insert_cursor_reload,insert_data_again,container_reload,insert_size_store,insert_key_read,container_vtable,child_shape_function,
    child_key_store,insert_cursor_again,child_shape_store,filter_key,filter_input,filter_read,filter_vtable,filter_container,
    filter_body_a,filter_function,enabled_read,create_manager,create_input,create_function,insert_cursor_final,new_agent_store,
    new_key,new_key_store,insert_size_reload,insert_data_reload,insert_keys_data_reload,insert_entries_end_store_again,
    insert_cursor_next,insert_keys_end_reload,insert_cursor_store_next,
    // the sorted merge
    sort_keys_size,comparator_store,comparator_read,sort_keys_data,sort_keys_size_again,merge_manager_read,merge_entries_data,
    merge_entries_size,merge_keys_data,merge_manager_store,merge_entries_end_store,merge_keys_end_store,entries_capacity_read,
    merge_keys_size,merge_entries_end_reload,entries_size_store,entries_data_read,merge_keys_end_read,merge_key,merge_entry_key,
    merge_key_store,kept_word0,kept_word0_store,kept_word1,kept_word1_store,kept_word2,kept_word2_store,
    merge_container,merge_container_vtable,merge_key_reload,merge_child_shape_function,merge_key_again,merge_filter_input,
    merge_child_key_store,merge_filter,merge_child_shape_store,merge_filter_key,merge_filter_vtable,merge_filter_container,
    merge_filter_body_a,merge_filter_input_again,merge_filter_function,merge_enabled,merge_create_manager,merge_create_input,
    merge_create_body_a,merge_create_function,merge_agent_store,merge_new_key,merge_new_key_store,
    gone_agent,gone_vtable,gone_function,merge_entries_end_next,rest_agent,rest_vtable,rest_function,rest_end,
    tail_keys_end_read,tail_key,tail_container,tail_container_vtable,tail_child_shape_function,tail_child_key_store,
    tail_child_shape_store,tail_filter_key,tail_filter_input,tail_filter,tail_filter_vtable,tail_filter_container,
    tail_filter_body_a,tail_filter_function,tail_enabled,tail_create_manager,tail_create_input,tail_create_function,
    tail_agent_store,tail_new_key,tail_keys_end_next,tail_new_key_store,
    install_agent,install_capacity,install_size,install_old_data,install_memory,install_memory_vtable,install_deallocate_function,
    install_size_again,install_memory_again,install_memory_vtable_again,install_allocate_function,install_size_reload,
    install_data_store,install_capacity_store,install_data,install_size_store,install_source,copy_word0,copy_word0_store,
    copy_word1,copy_word1_store,copy_word2,copy_word2_store,sorted_keys_data_reload,
    // the keys' storage
    hits_capacity_read,hits_memory,hits_memory_vtable,hits_deallocate_function
};

// The cache test of one corner: per lane, whether the corner moved by more
// than the slack (CMPLTPS slack < |cached - now|), as the adapter publishes
// the registers; `moved` is the lane mask of x, y and z.
struct FaceTest {Vector4 slack,distance,beyond;std::uint32_t moved;};
inline FaceTest face_test(Vector4 slack,Vector4 cached,Vector4 now,Vector4 mask) {
    FaceTest test{slack,_mm_and_ps(sse::subtract(cached,now),mask),{},0u};
    test.beyond=sse::less(slack,test.distance);
    test.moved=std::uint32_t(_mm_movemask_ps(test.beyond))&7u;
    return test;
}

// 2. True when a face moved (the maximum corner is tested first).
template<class Memory,class Frame> bool aabb_moved(Memory &memory,Frame &frame,const Locals &local,std::uint32_t agent,
    Vector4 minimum,Vector4 maximum) {
    using enum Access;
    const auto half=memory.load32(slack_read,local.half_tolerance);
    const auto cached_max=memory.load_vector(cached_maximum,at(agent,offsetof(Agent,cached.max)));
    const auto mask=memory.load_vector(absolute_mask_read,globals::absolute_mask);
    memory.store32(slack_store,local.slack,half);
    const auto slack=splat(vector({memory.load32(slack_reload,local.slack),0u,0u,0u}),0);
    auto test=face_test(slack,cached_max,maximum,mask);
    frame.faces_compared(test,false);
    memory.store32(moved_store,local.moved,test.moved);
    if(test.moved)return true;
    test=face_test(slack,memory.load_vector(cached_minimum,at(agent,offsetof(Agent,cached.min))),minimum,mask);
    frame.faces_compared(test,true);
    memory.store32(moved_store_again,local.moved,test.moved);
    return test.moved!=0u;
}

// hkArray<ChildEntry>::removeAt as the original inlines it: the size shrinks,
// the entry's index is its offset divided by 12 (signed), and each later
// entry moves down one, re-reading the data, the size and the offset kept in
// the frame. Returns whether any entry moved.
template<class Memory> bool remove_entry(Memory &memory,const Locals &local,std::uint32_t array,std::uint32_t entry) {
    using enum Access;
    const auto size_address=at(array,offsetof(HavokArray,size));
    const auto size=memory.load32(removal_size,size_address);
    const auto data=memory.load32(removal_data,at(array,offsetof(HavokArray,data)));
    memory.store32(removal_size_store,size_address,size-1u);
    auto index=std::uint32_t(std::int32_t(entry-data)/std::int32_t(entry_size));
    if(!(std::int32_t(index)<std::int32_t(memory.load32(removal_size_reload,size_address))))return false;
    auto offset=index*entry_size;
    memory.store32(shift_offset_store,local.shift_offset,offset);
    for(;;) {
        const auto target=offset+memory.load32(shift_data,at(array,offsetof(HavokArray,data)));
        const auto source=target+entry_size;
        memory.store32(shift_word0_store,target,memory.load32(shift_word0,source));
        memory.store32(shift_word1_store,at(target,4),memory.load32(shift_word1,at(source,4)));
        memory.store32(shift_word2_store,at(target,8),memory.load32(shift_word2,at(source,8)));
        offset=memory.load32(shift_offset_reload,local.shift_offset);
        const auto bound=memory.load32(shift_size,size_address);
        ++index;offset+=entry_size;
        memory.store32(shift_offset_store_next,local.shift_offset,offset);
        if(!(std::int32_t(index)<std::int32_t(bound)))return true;
    }
}

// The capacity hkArray::reserve() receives when an array of `capacity` must
// hold `needed` elements: twice the capacity, or the need when that is not
// less (signed, as the original's).
constexpr std::uint32_t grown_capacity(std::uint32_t capacity,std::uint32_t needed) {
    return std::int32_t(needed)<std::int32_t(capacity*2u)?capacity*2u:needed;
}

// 4. (query order) Returns the keys' data, as last read.
template<class Memory,class Frame> std::uint32_t match_in_query_order(Memory &memory,Frame &frame,const Locals &local,
    std::uint32_t agent) {
    using enum Access;
    const auto children=at(agent,offsetof(Agent,base.children));
    const auto manager=memory.load32(manager_read,at(agent,offsetof(Agent,base.contact_manager)));
    auto entry=memory.load32(entries_data,at(children,offsetof(HavokArray,data)));
    auto keys=memory.load32(keys_data,at(local.hits,offsetof(KeyArray,data)));
    memory.store32(manager_store,local.contact_manager,manager);
    const auto count=memory.load32(entries_size,at(children,offsetof(HavokArray,size)));
    const auto key_count=memory.load32(keys_size,at(local.hits,offsetof(KeyArray,size)));
    auto end=element(entry,count,entry_size);
    auto keys_end=element(keys,key_count,key_size);
    memory.store32(entries_end_store,local.entries_end,end);
    auto expected=keys;  // the key the next entry should have
    memory.store32(keys_end_store,local.keys_end,keys_end);
    // Remove the entries whose key the query did not return.
    while(entry!=end) {
        bool next=false;
        if(expected!=keys_end) {
            const auto held=memory.load32(entry_key,entry);
            next=held==memory.load32(expected_key,expected);
        }
        if(next)expected+=key_size;
        else {
            auto position=keys;
            memory.store32(search_start_store,local.key_position,position);
            bool found=false;
            if(keys!=keys_end) {
                const auto key=memory.load32(search_entry_key,entry);
                while(!(found=key==memory.load32(search_key,position)) && (position+=key_size)!=keys_end){}
                if(!found)memory.store32(search_end_store,local.key_position,position);
            }
            if(found) {
                memory.store32(found_store,local.key_position,position);
                expected=position+key_size;
            } else {
                const auto child=memory.load32(entry_agent,at(entry,offsetof(ChildEntry,agent)));
                frame.cleanup(memory.load32(cleanup_function,at(memory.load32(child_vtable,child),slot::cleanup)),child);
                expected=remove_entry(memory,local,children,entry)?memory.load32(position_reload,local.key_position):position;
                end=memory.load32(entries_end_reload,local.entries_end);
                keys=memory.load32(keys_data_reload,at(local.hits,offsetof(KeyArray,data)));
                entry-=entry_size;end-=entry_size;  // the next entry moved into this one's place
                memory.store32(entries_end_store_again,local.entries_end,end);
                keys_end=memory.load32(keys_end_reload,local.keys_end);
            }
        }
        end=memory.load32(entries_end_next,local.entries_end);
        entry+=entry_size;
    }
    // Give each new key an entry, at the key's own index (the entries left
    // are the keys' in order).
    auto size=memory.load32(insert_entries_size,at(children,offsetof(HavokArray,size)));
    if(memory.load32(insert_keys_size,at(local.hits,offsetof(KeyArray,size)))==size)return keys;
    entry=memory.load32(insert_entries_data,at(children,offsetof(HavokArray,data)));
    const auto inserted_keys=memory.load32(insert_keys_size_again,at(local.hits,offsetof(KeyArray,size)));
    memory.store32(insert_entries_end_store,local.insert_entries_end,element(entry,size,entry_size));
    auto cursor=keys;
    memory.store32(insert_cursor_store,local.key_position,cursor);
    memory.store32(insert_keys_end_store,local.insert_keys_end,element(keys,inserted_keys,key_size));
    if(keys==element(keys,inserted_keys,key_size))return keys;
    for(;;) {
        bool matched=false;
        if(entry!=memory.load32(insert_end_read,local.insert_entries_end)) {
            const auto held=memory.load32(insert_entry_key,entry);
            matched=held==memory.load32(insert_key,cursor);
        }
        if(!matched) {
            const auto index=std::uint32_t(std::int32_t(cursor-keys)>>2);
            const auto grown=size+1u;
            memory.store32(move_count_store,local.move_count,size-index);
            const auto capacity=memory.load32(insert_capacity,at(children,offsetof(HavokArray,capacity_and_flags)))&
                contact_array::capacity_mask;
            memory.store32(new_size_store,local.new_size,grown);
            if(std::int32_t(capacity)<std::int32_t(grown))frame.reserve(children,grown_capacity(capacity,grown));  // 2E31E0
            const auto place=element(memory.load32(insert_data,at(children,offsetof(HavokArray,data))),index,entry_size);
            const auto moving=memory.load32(move_count_reload,local.move_count);
            memory.store32(move_first_store,local.move_cursor,place+entry_size);
            auto new_size=grown;
            if(std::int32_t(moving-1u)>=0) {
                // The entries from the index on move up one, the last first.
                auto target=place+entry_size+(moving-1u)*entry_size;
                auto step=place-memory.load32(move_first_reload,local.move_cursor);
                memory.store32(move_step_store,local.move_step,step);
                memory.store32(move_left_store,local.move_cursor,moving);
                for(;;) {
                    const auto source=step+target;
                    memory.store32(move_word0_store,target,memory.load32(move_word0,source));
                    memory.store32(move_word1_store,at(target,4),memory.load32(move_word1,at(source,4)));
                    memory.store32(move_word2_store,at(target,8),memory.load32(move_word2,at(source,8)));
                    const auto left=memory.load32(move_left_reload,local.move_cursor)-1u;
                    target-=entry_size;
                    memory.store32(move_left_store_next,local.move_cursor,left);
                    if(!left)break;
                    step=memory.load32(move_step_reload,local.move_step);
                }
                new_size=memory.load32(new_size_reload,local.new_size);
            }
            const auto position=memory.load32(insert_cursor_reload,local.key_position);
            const auto data=memory.load32(insert_data_again,at(children,offsetof(HavokArray,data)));
            const auto container=memory.load32(container_reload,local.container);
            memory.store32(insert_size_store,at(children,offsetof(HavokArray,size)),new_size);
            const auto key=memory.load32(insert_key_read,position);
            entry=element(data,index,entry_size);
            const auto shape=frame.child_shape(memory.load32(child_shape_function,at(memory.load32(container_vtable,container),
                slot::child_shape)),container,key,local.child_buffer);
            memory.store32(child_key_store,at(local.child_body,offsetof(CdBody,shape_key)),key);
            const auto cursor_again=memory.load32(insert_cursor_again,local.key_position);
            memory.store32(child_shape_store,at(local.child_body,offsetof(CdBody,shape)),shape);
            const auto filter_key_value=memory.load32(filter_key,cursor_again);
            const auto input=memory.load32(filter_input,local.input);
            const auto filter=memory.load32(filter_read,at(input,offsetof(Input,filter)));
            const auto filter_table=memory.load32(filter_vtable,filter);
            const auto filter_collection=memory.load32(filter_container,local.container);
            const auto body_a=memory.load32(filter_body_a,local.body_a);
            const auto result=frame.collision_enabled(memory.load32(filter_function,at(filter_table,slot::collision_enabled)),filter,
                local.enabled,input,body_a,local.child_body,filter_collection,filter_key_value);
            std::uint32_t created;
            if(memory.load8(enabled_read,result)) {
                const auto agent_manager=memory.load32(create_manager,local.contact_manager);
                const auto create_input_value=memory.load32(create_input,local.input);
                created=frame.create_agent(memory.load32(create_function,local.create_agent),body_a,local.child_body,
                    create_input_value,agent_manager);
            } else created=frame.null_agent();  // 30BAD0
            const auto final_position=memory.load32(insert_cursor_final,local.key_position);
            memory.store32(new_agent_store,at(entry,offsetof(ChildEntry,agent)),created);
            memory.store32(new_key_store,at(entry,offsetof(ChildEntry,shape_key)),memory.load32(new_key,final_position));
            size=memory.load32(insert_size_reload,at(children,offsetof(HavokArray,size)));
            const auto end_again=element(memory.load32(insert_data_reload,at(children,offsetof(HavokArray,data))),size,entry_size);
            keys=memory.load32(insert_keys_data_reload,at(local.hits,offsetof(KeyArray,data)));
            memory.store32(insert_entries_end_store_again,local.insert_entries_end,end_again);
            cursor=memory.load32(insert_cursor_next,local.key_position);
        }
        const auto last=memory.load32(insert_keys_end_reload,local.insert_keys_end);
        cursor+=key_size;entry+=entry_size;
        memory.store32(insert_cursor_store_next,local.key_position,cursor);
        if(cursor==last)return keys;
    }
}

// An entry whose key is gone: its agent (if any) is cleaned up.
struct ReleaseAccess {Access agent,vtable,function;};
template<class Memory,class Frame> void release(Memory &memory,Frame &frame,std::uint32_t entry,const ReleaseAccess &a,bool rest) {
    const auto child=memory.load32(a.agent,at(entry,offsetof(ChildEntry,agent)));
    if(child)frame.cleanup_gone(memory.load32(a.function,at(memory.load32(a.vtable,child),slot::cleanup)),child,rest);
}

// 4. (sorted) Returns the keys' data, as last read.
template<class Memory,class Frame> std::uint32_t merge_sorted(Memory &memory,Frame &frame,const Locals &local,std::uint32_t agent) {
    using enum Access;
    const auto children=at(agent,offsetof(Agent,base.children));
    auto key_count=memory.load32(sort_keys_size,at(local.hits,offsetof(KeyArray,size)));
    memory.store8(comparator_store,local.comparator,0u);
    if(std::int32_t(key_count)>1) {
        const auto comparator=memory.load32(comparator_read,local.comparator);
        frame.sort_keys(memory.load32(sort_keys_data,at(local.hits,offsetof(KeyArray,data))),0u,key_count-1u,comparator);  // 2F33B0
        key_count=memory.load32(sort_keys_size_again,at(local.hits,offsetof(KeyArray,size)));
    }
    const auto manager=memory.load32(merge_manager_read,at(agent,offsetof(Agent,base.contact_manager)));
    auto entry=memory.load32(merge_entries_data,at(children,offsetof(HavokArray,data)));
    const auto count=memory.load32(merge_entries_size,at(children,offsetof(HavokArray,size)));
    auto key=memory.load32(merge_keys_data,at(local.hits,offsetof(KeyArray,data)));
    memory.store32(merge_manager_store,local.merge_contact_manager,manager);
    memory.store32(merge_entries_end_store,local.entries_end,element(entry,count,entry_size));
    memory.store32(merge_keys_end_store,local.merge_keys_end,element(key,key_count,key_size));
    frame.construct_entries(local.entries,key_count);  // 2F3430
    const auto capacity=memory.load32(entries_capacity_read,at(local.entries,offsetof(HavokArray,capacity_and_flags)))&
        contact_array::capacity_mask;
    const auto needed=memory.load32(merge_keys_size,at(local.hits,offsetof(KeyArray,size)));
    if(std::int32_t(capacity)<std::int32_t(needed))frame.reserve(local.entries,grown_capacity(capacity,needed));  // 2E31E0
    auto end=memory.load32(merge_entries_end_reload,local.entries_end);
    memory.store32(entries_size_store,at(local.entries,offsetof(HavokArray,size)),needed);
    auto out=memory.load32(entries_data_read,at(local.entries,offsetof(HavokArray,data)));
    while(entry!=end) {
        if(key==memory.load32(merge_keys_end_read,local.merge_keys_end)) {
            // No key is left: the remaining entries' agents are cleaned up.
            do {
                release(memory,frame,entry,{rest_agent,rest_vtable,rest_function},true);
                end=memory.load32(rest_end,local.entries_end);
                entry+=entry_size;
            } while(entry!=end);
            break;
        }
        const auto wanted=memory.load32(merge_key,key);
        const auto held=memory.load32(merge_entry_key,entry);
        memory.store32(merge_key_store,local.merged_key,wanted);
        if(wanted==held) {
            // The entry stays.
            memory.store32(kept_word0_store,out,memory.load32(kept_word0,entry));
            memory.store32(kept_word1_store,at(out,4),memory.load32(kept_word1,at(entry,4)));
            memory.store32(kept_word2_store,at(out,8),memory.load32(kept_word2,at(entry,8)));
            out+=entry_size;entry+=entry_size;key+=key_size;
        } else if(wanted>held) {
            // The entry's key is gone.
            release(memory,frame,entry,{gone_agent,gone_vtable,gone_function},false);
            entry+=entry_size;
        } else {
            // A new key.
            const auto container=memory.load32(merge_container,local.container);
            const auto table=memory.load32(merge_container_vtable,container);
            const auto shape_key=memory.load32(merge_key_reload,local.merged_key);
            const auto shape=frame.child_shape(memory.load32(merge_child_shape_function,at(table,slot::child_shape)),container,
                shape_key,local.child_buffer);
            const auto new_key_value=memory.load32(merge_key_again,local.merged_key);
            const auto input=memory.load32(merge_filter_input,local.input);
            memory.store32(merge_child_key_store,at(local.child_body,offsetof(CdBody,shape_key)),new_key_value);
            const auto filter=memory.load32(merge_filter,at(input,offsetof(Input,filter)));
            memory.store32(merge_child_shape_store,at(local.child_body,offsetof(CdBody,shape)),shape);
            const auto filter_key_value=memory.load32(merge_filter_key,key);
            const auto filter_table=memory.load32(merge_filter_vtable,filter);
            const auto filter_collection=memory.load32(merge_filter_container,local.container);
            const auto body_a=memory.load32(merge_filter_body_a,local.body_a);
            const auto filter_input_value=memory.load32(merge_filter_input_again,local.input);
            const auto result=frame.collision_enabled(memory.load32(merge_filter_function,at(filter_table,slot::collision_enabled)),
                filter,local.enabled,filter_input_value,body_a,local.child_body,filter_collection,filter_key_value);
            std::uint32_t created;
            if(memory.load8(merge_enabled,result)) {
                const auto agent_manager=memory.load32(merge_create_manager,local.merge_contact_manager);
                const auto create_input_value=memory.load32(merge_create_input,local.input);
                const auto creator_body=memory.load32(merge_create_body_a,local.body_a);
                created=frame.create_agent(memory.load32(merge_create_function,local.create_agent),creator_body,local.child_body,
                    create_input_value,agent_manager);
            } else created=frame.null_agent();  // 30BAD0
            memory.store32(merge_agent_store,at(out,offsetof(ChildEntry,agent)),created);
            memory.store32(merge_new_key_store,at(out,offsetof(ChildEntry,shape_key)),memory.load32(merge_new_key,key));
            out+=entry_size;key+=key_size;
        }
        end=memory.load32(merge_entries_end_next,local.entries_end);
    }
    // The keys after the last entry are all new.
    if(key!=memory.load32(tail_keys_end_read,local.merge_keys_end))for(;;) {
        const auto new_key_value=memory.load32(tail_key,key);
        const auto container=memory.load32(tail_container,local.container);
        const auto table=memory.load32(tail_container_vtable,container);
        const auto shape=frame.child_shape(memory.load32(tail_child_shape_function,at(table,slot::child_shape)),container,
            new_key_value,local.child_buffer);
        memory.store32(tail_child_key_store,at(local.child_body,offsetof(CdBody,shape_key)),new_key_value);
        memory.store32(tail_child_shape_store,at(local.child_body,offsetof(CdBody,shape)),shape);
        const auto filter_key_value=memory.load32(tail_filter_key,key);
        const auto input=memory.load32(tail_filter_input,local.input);
        const auto filter=memory.load32(tail_filter,at(input,offsetof(Input,filter)));
        const auto filter_table=memory.load32(tail_filter_vtable,filter);
        const auto filter_collection=memory.load32(tail_filter_container,local.container);
        const auto body_a=memory.load32(tail_filter_body_a,local.body_a);
        const auto result=frame.collision_enabled(memory.load32(tail_filter_function,at(filter_table,slot::collision_enabled)),filter,
            local.enabled,input,body_a,local.child_body,filter_collection,filter_key_value);
        std::uint32_t created;
        if(memory.load8(tail_enabled,result)) {
            const auto agent_manager=memory.load32(tail_create_manager,local.merge_contact_manager);
            const auto create_input_value=memory.load32(tail_create_input,local.input);
            created=frame.create_agent(memory.load32(tail_create_function,local.create_agent),body_a,local.child_body,
                create_input_value,agent_manager);
        } else created=frame.null_agent();  // 30BAD0
        memory.store32(tail_agent_store,at(out,offsetof(ChildEntry,agent)),created);
        const auto stored_key=memory.load32(tail_new_key,key);
        const auto last=memory.load32(tail_keys_end_next,local.merge_keys_end);
        memory.store32(tail_new_key_store,at(out,offsetof(ChildEntry,shape_key)),stored_key);
        key+=key_size;out+=entry_size;
        if(key==last)break;
    }
    // The agent's array receives the merged entries.
    const auto self=memory.load32(install_agent,local.agent);
    const auto array=at(self,offsetof(Agent,base.children));
    const auto held_capacity=memory.load32(install_capacity,at(array,offsetof(HavokArray,capacity_and_flags)));
    auto size=memory.load32(install_size,at(local.entries,offsetof(HavokArray,size)));
    if(std::int32_t(held_capacity&contact_array::capacity_mask)<std::int32_t(size)) {
        if(!(held_capacity&local_buffers::storage_not_owned)) {
            const auto old=memory.load32(install_old_data,at(array,offsetof(HavokArray,data)));
            const auto owner=memory.load32(install_memory,globals::thread_memory);
            frame.deallocate(memory.load32(install_deallocate_function,at(memory.load32(install_memory_vtable,owner),slot::deallocate)),
                owner,old,(held_capacity&contact_array::capacity_mask)*entry_size,local_buffers::array_storage_class,false);
        }
        const auto bytes=memory.load32(install_size_again,at(local.entries,offsetof(HavokArray,size)))*entry_size;
        const auto owner=memory.load32(install_memory_again,globals::thread_memory);
        const auto storage=frame.allocate(memory.load32(install_allocate_function,
            at(memory.load32(install_memory_vtable_again,owner),slot::allocate)),owner,bytes,local_buffers::array_storage_class);
        size=memory.load32(install_size_reload,at(local.entries,offsetof(HavokArray,size)));
        memory.store32(install_data_store,at(array,offsetof(HavokArray,data)),storage);
        memory.store32(install_capacity_store,at(array,offsetof(HavokArray,capacity_and_flags)),size);  // owned, exactly the count
    }
    auto target=memory.load32(install_data,at(array,offsetof(HavokArray,data)));
    memory.store32(install_size_store,at(array,offsetof(HavokArray,size)),size);
    if(std::int32_t(size)>0) {
        auto source=memory.load32(install_source,at(local.entries,offsetof(HavokArray,data)));
        for(auto left=size;left;--left,source+=entry_size,target+=entry_size) {
            memory.store32(copy_word0_store,target,memory.load32(copy_word0,source));
            memory.store32(copy_word1_store,at(target,4),memory.load32(copy_word1,at(source,4)));
            memory.store32(copy_word2_store,at(target,8),memory.load32(copy_word2,at(source,8)));
        }
    }
    frame.destroy_entries(local.entries);  // 2F34D0
    return memory.load32(sorted_keys_data_reload,at(local.hits,offsetof(KeyArray,data)));
}

// Frame (the adapter's): locals(), relative_transform() (2E4550),
// shape_aabb(), faces_compared(), query_tree(), cleanup(), cleanup_gone(),
// reserve() (2E31E0), child_shape(), collision_enabled(), create_agent(),
// null_agent() (30BAD0), sort_keys() (2F33B0), construct_entries() (2F3430),
// destroy_entries() (2F34D0), allocate() and deallocate().
template<class Memory,class Frame> void update(Memory &memory,Frame &frame,std::uint32_t agent) {
    using enum Access;
    const auto &local=frame.locals();
    // 1. Body A's AABB in body B's frame.
    const auto body_b=memory.load32(body_b_argument,local.body_b);
    const auto motion_b=memory.load32(body_b_motion,at(body_b,offsetof(CdBody,motion)));
    const auto body_a=memory.load32(body_a_argument,local.body_a);
    const auto motion_a=memory.load32(body_a_motion,at(body_a,offsetof(CdBody,motion)));
    memory.store32(agent_store,local.agent,agent);
    frame.relative_transform(local.a_in_b,at(motion_b,collection_agent::motion_rotation),
        at(motion_a,collection_agent::motion_rotation));  // 2E4550
    const auto input=memory.load32(input_argument,local.input);
    const auto tolerance_value=memory.load_float(tolerance,at(input,offsetof(Input,tolerance)));
    const auto shape_a=memory.load32(body_a_shape,at(body_a,offsetof(CdBody,shape)));
    const auto half=x87::multiply(x87::load(tolerance_value),memory.load_float(half_factor,globals::half));  // FMUL m32
    const auto table=memory.load32(shape_a_vtable,shape_a);
    // The half is rounded to a float (FSTP) before the tolerance is added.
    memory.store_float(half_store,local.half_tolerance,x87::store(half));
    const auto rounded=x87::load(memory.load_float(half_reload,local.half_tolerance));
    const auto margin=x87::add(rounded,memory.load_float(tolerance_again,at(input,offsetof(Input,tolerance))));  // FADD m32
    memory.store_float(margin_store,local.aabb_tolerance,x87::store(margin));
    frame.shape_aabb(memory.load32(shape_aabb_function,at(table,slot::shape_aabb)),shape_a,local.a_in_b,local.aabb);
    // 2. The cache.
    const bool cache=memory.load8(cache_enabled,globals::aabb_cache)!=0u;
    const auto minimum=memory.load_vector(aabb_minimum,at(local.aabb,offsetof(Aabb,min)));
    const auto maximum=memory.load_vector(aabb_maximum,at(local.aabb,offsetof(Aabb,max)));
    if(cache && !aabb_moved(memory,frame,local,agent,minimum,maximum))return;
    // 3. The query.
    memory.store_vector(cache_minimum_store,at(agent,offsetof(Agent,cached.min)),minimum);
    memory.store_vector(cache_maximum_store,at(agent,offsetof(Agent,cached.max)),maximum);
    const auto tree=memory.load32(tree_read,at(body_b,offsetof(CdBody,shape)));
    memory.store32(hits_data_init,at(local.hits,offsetof(KeyArray,data)),local.hit_storage);
    memory.store32(hits_size_init,at(local.hits,offsetof(KeyArray,size)),0u);
    memory.store32(hits_capacity_init,at(local.hits,offsetof(KeyArray,capacity_and_flags)),
        inline_keys|local_buffers::storage_not_owned);
    const auto tree_table=memory.load32(tree_vtable,tree);
    frame.query_tree(memory.load32(query_function,at(tree_table,slot::query_aabb)),tree,local.aabb,local.hits);
    const bool in_query_order=memory.load8(query_order_read,globals::query_order)!=0u;
    const auto container=memory.load32(container_read,at(tree,offsetof(ShapeHeader,inner)));
    const auto motion=memory.load32(body_b_motion_again,at(body_b,offsetof(CdBody,motion)));
    memory.store32(child_parent_store,at(local.child_body,offsetof(CdBody,parent)),body_b);
    memory.store32(child_motion_store,at(local.child_body,offsetof(CdBody,motion)),motion);
    memory.store32(container_store,local.container,container);
    // 4. The matching.
    const auto keys=in_query_order?match_in_query_order(memory,frame,local,agent):merge_sorted(memory,frame,local,agent);
    // 5. ~hkInplaceArray: storage the keys own goes back to the thread memory.
    const auto capacity=memory.load32(hits_capacity_read,at(local.hits,offsetof(KeyArray,capacity_and_flags)));
    if(!(capacity&local_buffers::storage_not_owned)) {
        const auto owner=memory.load32(hits_memory,globals::thread_memory);
        frame.deallocate(memory.load32(hits_deallocate_function,at(memory.load32(hits_memory_vtable,owner),slot::deallocate)),
            owner,keys,(capacity&contact_array::capacity_mask)*key_size,local_buffers::array_storage_class,true);
    }
}
} // namespace child_agent_update

// ------------------------------------------------------- object flag update
// E8D80 (stdcall, RET 4; an entry of the vehicle update unit): an object's
// 60 flags (bits of two dwords at +4 of a block the object locates by a
// signed word offset at +346) are each owned by a definition in the table at
// 473D28; while a flag is set and its definition has an update function
// (+4), the function receives the object's handle and the flag's index and
// answers whether the flag stays set. A flag answered false is cleared (its
// word is re-read: the update may have changed it). Returns whether any
// update answered true. The flags' meaning is not established.
namespace object_flag_update {
namespace globals {
using shape_key_filter::globals::object_headers;
inline constexpr std::uint32_t definitions=0x473d28u;  // 60 pointers, one per flag
}
using shape_key_filter::ObjectHeader;using shape_key_filter::object_headers_data;
inline constexpr std::uint32_t flag_block_offset=0x346u;  // int16: the flag block's offset from the object
inline constexpr std::uint32_t flag_words=0x4u;           // the flags in the block
inline constexpr std::uint32_t flag_count=0x3cu;
inline constexpr std::uint32_t update_function=0x4u;      // in a flag's definition
inline constexpr std::uint32_t handle_index=0xffffu;      // a handle's index in the object array

struct Locals {
    std::uint32_t handle;  // the argument
    std::uint32_t result;  // byte: an update answered true
};
enum class Access {
    handle_argument,headers_read,headers_data,object_read,block_offset,result_clear,flag_word,definition,function,handle_reload,
    result_set,flag_word_again,flag_clear,result_read
};

// Frame (the adapter's): locals() and keep_flag() (the update function).
template<class Memory,class Frame> bool update(Memory &memory,Frame &frame) {
    using enum Access;
    const auto &local=frame.locals();
    const auto handle=memory.load32(handle_argument,local.handle);
    const auto headers=memory.load32(headers_read,globals::object_headers);
    const auto data=memory.load32(headers_data,at(headers,object_headers_data));
    const auto object=memory.load32(object_read,at(element(data,handle&handle_index,sizeof(ObjectHeader)),offsetof(ObjectHeader,object)));
    const auto block=object+std::uint32_t(std::int32_t(std::int16_t(memory.load16(block_offset,at(object,flag_block_offset)))));
    memory.store8(result_clear,local.result,0u);
    for(std::uint32_t flag=0;flag<flag_count;++flag) {
        const auto word=at(block,flag_words+(flag>>5)*4u);
        const auto bit=1u<<(flag&31u);
        if(!(memory.load32(flag_word,word)&bit))continue;
        const auto definition_address=memory.load32(definition,element(globals::definitions,flag,4));
        const auto update_address=memory.load32(function,at(definition_address,update_function));
        if(!update_address)continue;
        if(frame.keep_flag(update_address,memory.load32(handle_reload,local.handle),flag))memory.store8(result_set,local.result,1u);
        else memory.store32(flag_clear,word,memory.load32(flag_word_again,word)&~bit);
    }
    return memory.load8(result_read,local.result)!=0u;
}
} // namespace object_flag_update
} // namespace halo2::native_world_leaves
