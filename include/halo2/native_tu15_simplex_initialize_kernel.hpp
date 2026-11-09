#pragma once
#include "halo2/native_tu15_simplex_direction_kernel.hpp"
#include <bit>

namespace halo2 {
namespace native_simplex_initialize_numeric {
inline __m128 add(__m128 a,__m128 b){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;
#else
    throw std::runtime_error("Exact native simplex initialization backend unavailable");
#endif
}
}
enum class NativeTu15SimplexInitializePhase {
    cache_argument, first_count, save_work, publish_first, second_count,
    save_count_pointer, current_first, publish_second, first_argument,
    first_table, first_arguments, iteration, epsilon, first_owner, first_features,
    second_argument, second_table, second_output, current_second,
    second_count_argument, second_first, second_ids, second_owner, second_features,
    transform_count, transform_begin, transform_argument,
    point, matrix_x, matrix_y, matrix_z, translation, transformed,
    transform_advance, result, complete
};
struct NativeTu15SimplexInitialize {
    using Vector=native_simplex_direction_numeric::Vector;
    std::uint32_t work{},cache{},count_address{},first_count{},second_count{},current_count{};
    std::uint32_t shape{},table{},owner{},ids{},output{},source{},destination_offset{},remaining{},matrix{};
    Vector point{},x{},y{},z{},translation{},x_term{},y_term{},z_term{},zt{},xy{},transformed{};
};
// Initial feature acquisition and the caller-backed second-simplex transform.
// Access transports real feature calls and lazily resumes actual caller state;
// the algorithm never assumes that a child preserved aliased saved registers.
// No missing point rows are synthesized. The narrow semantic facade can retain
// its separate provided-row admission while sharing this transform policy.
template<class Access>
void native_tu15_initialize_simplex(Access&memory,NativeTu15SimplexInitialize&d){
    using P=NativeTu15SimplexInitializePhase;
    namespace N=native_simplex_direction_numeric;
    memory.begin(d);
    d.cache=memory.argument(P::cache_argument,d);
    d.first_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(
        std::uint8_t(memory.word_read(P::first_count,d,d.cache+8u,1u)))));
    memory.prepare(P::save_work,d);
    memory.word_write(P::publish_first,d,d.work,d.first_count);
    d.second_count=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(
        std::uint8_t(memory.word_read(P::second_count,d,d.cache+9u,1u)))));
    memory.prepare(P::save_count_pointer,d);
    d.current_count=memory.word_read(P::current_first,d,d.work,4u);
    memory.prepare(P::publish_second,d);
    memory.word_write(P::publish_second,d,d.count_address,d.second_count);
    d.shape=memory.argument(P::first_argument,d);
    d.table=memory.word_read(P::first_table,d,d.shape,4u);
    memory.prepare(P::first_arguments,d);
    memory.word_write(P::iteration,d,memory.frame(P::iteration,d),0u);
    memory.word_write(P::epsilon,d,memory.frame(P::epsilon,d),0x2edbe6ffu);
    d.owner=memory.word_read(P::first_owner,d,d.table+0x34u,4u);
    memory.features(P::first_features,d);
    d.shape=memory.argument(P::second_argument,d);
    d.table=memory.word_read(P::second_table,d,d.shape,4u);
    memory.prepare(P::second_output,d);
    d.current_count=memory.word_read(P::current_second,d,d.count_address,4u);
    memory.prepare(P::second_count_argument,d);
    d.current_count=memory.word_read(P::second_first,d,d.work,4u);
    memory.prepare(P::second_ids,d);
    d.owner=memory.word_read(P::second_owner,d,d.table+0x34u,4u);
    memory.features(P::second_features,d);
    d.remaining=memory.word_read(P::transform_count,d,d.count_address,4u);
    // The source tests SF after DEC, including INT_MIN's wrapped positive
    // result. A signed count<=0 shortcut would change that malformed domain.
    if(memory.transform_begin(d)){
        d.matrix=memory.argument(P::transform_argument,d);
        do{
            d.point=memory.vector_read(P::point,d,d.source);
            d.x=memory.vector_read(P::matrix_x,d,d.matrix);
            d.y=memory.vector_read(P::matrix_y,d,d.matrix+0x10u);
            d.x_term=N::multiply(d.x,N::shuffle<0>(d.point));
            d.y_term=N::multiply(d.y,N::shuffle<0x55>(d.point));
            d.z=memory.vector_read(P::matrix_z,d,d.matrix+0x20u);
            d.z_term=N::multiply(d.z,N::shuffle<0xaa>(d.point));
            d.translation=memory.vector_read(P::translation,d,d.matrix+0x30u);
            d.zt=native_simplex_initialize_numeric::add(d.z_term,d.translation);
            d.xy=native_simplex_initialize_numeric::add(d.x_term,d.y_term);
            d.transformed=native_simplex_initialize_numeric::add(d.xy,d.zt);
            memory.vector_write(P::transformed,d,d.source+d.destination_offset,d.transformed);
            memory.prepare(P::transform_advance,d);
        }while(d.remaining);
    }
    memory.word_write(P::result,d,memory.frame(P::result,d),0u);
    memory.complete(d);
}
} // namespace halo2
