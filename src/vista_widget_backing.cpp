#include "halo2/vista_widget_backing.hpp"
#include "halo2/cache.hpp"
#include <utility>
namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Vista aligned backing reached an unbound genuine service");
  return f(std::forward<A>(a)...);
}
} //namespace
VistaWidgetIdentity vista_widget_aligned_backing_allocate(std::uint32_t size,
    std::uint32_t exponent,const VistaWidgetBackingServices& s) {
  const auto requested_alignment=std::uint32_t{1}<<(exponent&31u);
  const auto alignment=requested_alignment>8u?requested_alignment:8u;
  const auto total=size+alignment;
  const auto heap=call(s.process_heap);
  const auto original=call(s.allocate,heap,0u,total);
  if(!original)return {};
  const auto aligned=(original.value+alignment)&~(alignment-1u);
  call(s.write,aligned-8u,size,std::size_t{4});
  call(s.write,aligned-4u,aligned-original.value,std::size_t{4});
  return {aligned};
}
VistaWidgetIdentity vista_widget_backing_allocate(std::uint32_t size,
    const VistaWidgetBackingServices& s) {
  return vista_widget_aligned_backing_allocate(size,12,s);
}
std::uint32_t vista_widget_backing_release(VistaWidgetIdentity data,
    const VistaWidgetBackingServices& s) {
  const auto offset=call(s.read,data.value-4u,std::size_t{4});
  const VistaWidgetIdentity original{data.value-offset};
  const auto heap=call(s.process_heap);
  return call(s.release,heap,0u,original);
}
} //namespace halo2
