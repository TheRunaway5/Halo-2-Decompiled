#include "halo2/source_camouflage_context.hpp"
#include <stdexcept>
namespace halo2 {
float source_object_camouflage(std::uint32_t datum,const SourceCamouflageContextServices& services){
  if(!services.initial)throw std::runtime_error("Source camouflage initial lookup unavailable");
  auto object=services.initial(datum);
  while(object){
    if(object->type==0){
      if(!services.primary)throw std::runtime_error("Source camouflage primary owner unavailable");
      const float primary=services.primary(datum);
      if(0.f>primary)return 0.f;
      if(primary>1.f)return 1.f;
      return primary;
    }
    if(object->parent==0xffffffffu)return 0.f;
    if(!services.parent)throw std::runtime_error("Source camouflage parent projection unavailable");
    datum=object->parent;object=services.parent(datum);
  }
  return 0.f;
}
}
