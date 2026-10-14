#pragma once
#include "halo2/original_data.hpp"
#include "halo2/full_scoreboard_paint_constants.hpp"
#include <type_traits>

namespace halo2::reference {
// Reference tooling and tests only. Runtime uses the generated typed factory.
template<class Visitor,class T>
void operand_fields(Visitor&,const T&,const std::string&);
#define H2_OPERAND_RECORD(Type, ...) \
  template<class Visitor> void operand_fields(Visitor&,const Type&,const std::string&);
#include "native_tu15_operand_fields.inc"
#undef H2_OPERAND_RECORD

template<class> inline constexpr bool unsupported_operand_type=false;
template<class Visitor,class T>
void operand_fields(Visitor& visitor,const T& value,const std::string& path) {
  if constexpr(std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    visitor.scalar(value,path);
  } else if constexpr(requires{value.has_value();}) {
    visitor.optional(path,value.has_value());
    if(value)operand_fields(visitor,*value,"(*"+path+")");
  } else if constexpr(requires{typename T::element_type;value.use_count();}) {
    const auto owned=visitor.template shared<T>(path,bool(value));
    if(value)operand_fields(visitor,*value,owned);
    visitor.shared_end(path,bool(value));
  } else if constexpr(requires{typename T::mapped_type;}) {
    visitor.map(path,value.size());
    for(const auto& [key,element]:value)
      operand_fields(visitor,element,visitor.map_entry(path,key));
  } else if constexpr(requires{value.size();value.begin();value[0];}) {
    constexpr bool resizable=requires(T v){v.resize(0);};
    visitor.sequence(path,value.size(),resizable);
    for(std::size_t i=0;i<value.size();++i)
      operand_fields(visitor,value[i],path+"["+std::to_string(i)+"]");
  } else {
    static_assert(unsupported_operand_type<T>,"Unsupported native TU1.5 operand field; classify its ownership explicitly");
  }
}
#define H2_OPERAND_RECORD(Type, ...) \
  template<class Visitor> void operand_fields(Visitor& visitor,const Type& value,const std::string& path) {__VA_ARGS__}
#define H2_OPERAND_FIELD(Name) operand_fields(visitor,value.Name,path+"." #Name);
#include "native_tu15_operand_fields.inc"
#undef H2_OPERAND_FIELD
#undef H2_OPERAND_RECORD
} // namespace halo2::reference
