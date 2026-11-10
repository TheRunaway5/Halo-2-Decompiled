#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_ray_setup_detail {
using Vector=std::array<std::uint32_t,4>;
enum class Read {collection,context,collector,fraction,code,inverse_scale,scale,normalization,
                 ray,vector_scale,filter_info,filter};
enum class Vectors {translation,start,end,current_end};
enum class Store {collection,context,collector,output,fraction,code,inverse_scale,
                  variable_scale,vector_scale,start,end,filter_info,filter,hit};
enum class Arithmetic {start_delta,end_delta,start_scaled,end_scaled};
struct Progress {
    std::uint32_t collection{},context{},collector{},fraction{},code{},ray{},scale{};
    Vector translation{},start{},end{},start_delta{},end_delta{},normalized_start{},normalized_end{};
};

// Native quantization/cache preparation before a genuine traversal child.
// The view owns private caller storage and ABI publication. All numerical
// values come from current lazy inputs, rather than an oracle child packet.
template<class View>void prepare(View&view){
    Progress d;
    view.begin();d.collection=view.scalar(Read::collection,d);view.unit();
    view.store(Store::collection,d.collection,d);
    d.context=view.scalar(Read::context,d);view.store(Store::context,d.context,d);
    d.collector=view.scalar(Read::collector,d);view.store(Store::collector,d.collector,d);
    view.zero_output();view.store(Store::output,0u,d);
    d.fraction=view.scalar(Read::fraction,d);view.store(Store::fraction,d.fraction,d);
    d.code=view.scalar(Read::code,d);view.store(Store::code,d.code,d);
    view.zero_translation();view.divide_scalar(view.scalar(Read::inverse_scale,d));
    view.program();view.spill(Store::inverse_scale,true,d);
    view.floating(view.scalar(Read::scale,d));
    d.translation=view.vector(Vectors::translation,d);
    view.multiply_scalar(view.scalar(Read::normalization,d));
    d.ray=view.scalar(Read::ray,d);d.start=view.vector(Vectors::start,d);
    d.end=view.vector(Vectors::end,d);
    d.end_delta=view.subtract(d.end,d.translation,Arithmetic::end_delta,d);
    view.spill(Store::variable_scale,false,d);
    d.start_delta=view.subtract(d.start,d.translation,Arithmetic::start_delta,d);
    view.spill(Store::vector_scale,true,d);d.scale=view.scalar(Read::vector_scale,d);
    d.normalized_start=view.multiply(d.scale,d.start_delta,Arithmetic::start_scaled,d);
    view.store_vector(Store::start,d.start,d);
    const auto current_end=view.vector(Vectors::current_end,d);
    view.store_vector(Store::end,current_end,d);
    view.store(Store::filter_info,view.scalar(Read::filter_info,d),d);
    view.store(Store::filter,view.scalar(Read::filter,d),d);
    view.store(Store::hit,0u,d);view.variables();
    d.normalized_end=view.multiply(d.scale,d.end_delta,Arithmetic::end_scaled,d);
    view.arguments();view.bounds(d.normalized_start,d.normalized_end);
    view.traverse();view.finish();
}
} // namespace halo2::native_mopp_ray_setup_detail
