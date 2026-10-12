#include "halo2/sound_sdk_setters.hpp"
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {namespace {
struct Retained {
  const SoundSdkDeviceMemory &memory;
  std::span<std::byte> bytes(std::uint32_t address,unsigned size)const{
    if(!memory)throw std::runtime_error("Original SDK setter retained memory unavailable");
    auto raw=memory(address,size);
    if(raw.size()!=size)throw std::runtime_error("Original SDK setter retained extent differs");
    return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=bytes(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value,
      const SoundDriverStartupObserver &observer)const{
    const auto before=read(address,size);const auto raw=bytes(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(i*8));
    if(observer)observer({pc,address,size,before,value});
  }
};
struct StateAccess {
  const SoundSdkSetterServices &services;bool acquired;
  explicit StateAccess(const SoundSdkSetterServices &s):services(s),acquired(false){
    if(!s.acquire_state_access||!s.release_state_access)
      throw std::runtime_error("Original SDK setter state access unavailable");
    acquired=s.acquire_state_access();
  }
  ~StateAccess(){if(acquired)services.release_state_access();}
};
std::uint32_t object(std::uint32_t interface){return interface?interface-0x1cu:0u;}
void commit(std::uint32_t owner,const Retained &r,const SoundSdkSetterServices &services){
  if(!services.commit)throw std::runtime_error("Original SDK setter commit CPU owner unavailable");
  services.commit(r.read(owner+0xcu));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
  std::fenv_t saved;unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context):mxcsr(_mm_getcsr()){
    if((context.x87_control_word&0x3fu)!=0x3fu||(context.mxcsr&0x1f80u)!=0x1f80u||(context.mxcsr&0xffff0000u))
      throw std::runtime_error("Unmasked source SDK setter floating-point exception delivery unavailable");
    if(std::fegetenv(&saved)!=0)throw std::runtime_error("Original SDK setter floating-point state unavailable");
    _mm_setcsr(context.mxcsr);asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
void spill(std::array<std::uint32_t,3> &position){
  for(unsigned i:{2u,1u,0u}){
    std::uint32_t value;
    asm volatile("flds %1\n\tfstps %0":"=m"(value):"m"(position[i]):"st");
    position[i]=value;
  }
}
#endif
} // namespace

std::uint32_t source_sound_sdk_set_position(std::uint32_t interface,
    std::array<std::uint32_t,3> position,std::uint32_t apply,const SoundSdkSetterBindings &bindings,
    const SoundDriverFloatingPointContext &fp,const SoundSdkSetterServices &services,
    const SoundDriverStartupObserver &observer){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);spill(position);
  StateAccess access(services);const Retained r{services.memory};
  if(r.read(bindings.busy_address))return 0x80004005u;
  spill(position);const auto owner=object(interface);
  for(unsigned i=0;i<3;++i){
    const auto source=r.read(r.read(owner+0x10u)+0xb4u);
    r.write(0x37c043u+i*0xfu,source+8u+i*4u,4,position[i],observer);
  }
  const auto source=r.read(r.read(owner+0x10u)+0xb4u);
  r.write(0x37c06du,source+2u,1,r.read(source+2u,1)|1u,observer);
  if(!(apply&1u))commit(owner,r,services);
  return 0;
#else
  (void)interface;(void)position;(void)apply;(void)bindings;(void)fp;(void)services;(void)observer;
  throw std::runtime_error("Original x87 SDK setter backend unavailable");
#endif
}

std::uint32_t source_sound_sdk_set_environment(std::uint32_t interface,std::uint32_t payload,
    std::uint32_t apply,const SoundSdkSetterBindings &bindings,const SoundSdkSetterServices &services,
    const SoundDriverStartupObserver &observer){
  StateAccess access(services);const Retained r{services.memory};
  if(r.read(bindings.busy_address))return 0x80004005u;
  const auto owner=object(interface);
  // First destination is loaded before the first source word. Each remaining
  // source word precedes its destination reload; overlapping payloads matter.
  auto source=r.read(r.read(owner+0x10u)+0xb4u);
  for(unsigned i=0;i<9;++i){
    const auto value=r.read(payload+i*4u);
    if(i)source=r.read(r.read(owner+0x10u)+0xb4u);
    r.write(i==8?0x37c18du:0x37c0fdu+i*0x12u,source+0x80u+i*4u,4,value,observer);
  }
  source=r.read(r.read(owner+0x10u)+0xb4u);
  r.write(0x37c19cu,source+0x7eu,1,r.read(source+0x7eu,1)|0x7fu,observer);
  if(!(apply&1u))commit(owner,r,services);
  return 0;
}

std::uint32_t source_sound_sdk_set_filter(std::uint32_t interface,std::uint32_t payload,
    const SoundSdkSetterBindings &bindings,const SoundSdkSetterServices &services){
  StateAccess access(services);const Retained r{services.memory};
  if(r.read(bindings.busy_address))return 0x80004005u;
  if(!services.filter)throw std::runtime_error("Original SDK setter filter CPU owner unavailable");
  services.filter(r.read(object(interface)+0xcu),payload);
  return 0;
}

std::uint32_t source_sound_sdk_driver_operation(const SoundDriverSpatialRequest &request,
    const SoundSdkSetterBindings &bindings,const SoundDriverFloatingPointContext &fp,
    const SoundSdkSetterServices &services,const SoundSdkPayloadPublisher &publish,
    const SoundDriverStartupObserver &observer){
  if(request.operation==SoundDriverSpatialOperation::Position){
    if(request.words.size()!=3||!request.apply)
      throw std::runtime_error("Original SDK position operation payload differs");
    return source_sound_sdk_set_position(request.buffer_identity,
        {request.words[0],request.words[1],request.words[2]},*request.apply,bindings,fp,services,observer);
  }
  const bool environment=request.operation==SoundDriverSpatialOperation::I3dl2Source;
  if((!environment&&request.operation!=SoundDriverSpatialOperation::Filter)||
      request.words.size()!=(environment?9u:6u)||(environment&&!request.apply)||(!environment&&request.apply))
    throw std::runtime_error("Original SDK environmental/filter operation payload differs");
  if(!publish)throw std::runtime_error("Original SDK operation payload owner unavailable");
  std::optional<std::uint32_t> result;unsigned calls=0;
  publish(request.words,[&](std::uint32_t identity){
    if(calls++)throw std::runtime_error("Original SDK operation payload body repeated");
    result=environment?source_sound_sdk_set_environment(request.buffer_identity,identity,*request.apply,bindings,services,observer):
        source_sound_sdk_set_filter(request.buffer_identity,identity,bindings,services);
  });
  if(calls!=1||!result)throw std::runtime_error("Original SDK operation payload body not executed");
  return *result;
}
} // namespace halo2
