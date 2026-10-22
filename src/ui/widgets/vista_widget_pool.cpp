#include "halo2/vista_widget_pool.hpp"
#include "halo2/vista_widget_manager_lifetime.hpp"
#include "halo2/cache.hpp"
#include <utility>

namespace halo2 {
namespace {
constexpr std::uint32_t heap_global=0xd758e4;
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Vista UI pool reached an unbound genuine service");
  return f(std::forward<A>(a)...);
}
std::uint32_t get(std::uint32_t address,const VistaWidgetPoolServices& s,
    std::size_t n=4) {return call(s.read,address,n);}
void put(std::uint32_t address,std::uint32_t value,
    const VistaWidgetPoolServices& s,std::size_t n=4) {call(s.write,address,value,n);}
void clear_header(std::uint32_t address,const VistaWidgetPoolServices& s) {
  // Actual687BA9's size40 zero-fill path: byte-align, DWORDs, byte tail.
  std::uint32_t left=0x40;
  for(std::uint32_t n=(0u-address)&3u;n;--n,--left)put(address++,0,s,1);
  while(left>=4){put(address,0,s);address+=4;left-=4;}
  while(left--!=0)put(address++,0,s,1);
}
void debug_header(std::uint32_t address,std::uint32_t metadata1,
    std::uint32_t metadata2,const VistaWidgetPoolServices& s) {
  if(!address)return;
  put(address,0x68656164,s);put(address+4,metadata1,s);
  put(address+8,metadata2,s);const auto timestamp=call(s.timestamp);
  put(address+0xc,timestamp,s);
}
std::uint32_t insert(std::uint32_t heap,std::uint32_t address,
    std::uint32_t previous,std::uint32_t next,std::uint32_t total,
    std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices& s) {
  // Complete48BEAB; registerEAX supplies the unadjusted insertion address.
  const auto debug=get(heap+0x3f,s,1);
  if(debug){address+=0x10;debug_header(address-0x10,metadata1,metadata2,s);}
  put(address,total,s);put(address+8,next,s);put(address+0xc,previous,s);
  if(previous)put(previous+8,address,s);else put(heap+0x34,address,s);
  if(next)put(next+0xc,address,s);else put(heap+0x38,address,s);
  return address;
}
std::uint32_t gap(std::uint32_t heap,std::uint32_t total,
    std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices& s) {
  // Complete48C029, including fresh next/header/flag reads per iteration.
  auto previous=get(heap+0x34,s);
  while(previous) {
    const auto next=get(previous+8,s);
    if(next) {
      const auto debug=get(heap+0x3f,s,1);
      const auto next_begin=debug?next-0x10:next;
      const auto previous_begin=debug?previous-0x10:previous;
      const auto length=get(previous,s);
      const auto address=length+previous_begin;
      if(address+total<=next_begin)
        return insert(heap,address,previous,next,total,metadata1,metadata2,s);
    }
    previous=next;
  }
  return 0;
}
std::uint32_t front(std::uint32_t heap,std::uint32_t total,
    std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices& s) {
  // Complete48BFA4 (its original heap is registerEDI).
  const auto next=get(heap+0x34,s);
  std::uint32_t limit{};
  if(!next){limit=get(heap+0x2c,s);limit+=get(heap+0x28,s);}
  else {const auto debug=get(heap+0x3f,s,1);limit=debug?next-0x10:next;}
  auto address=get(heap+0x28,s);
  if(address+total>limit)return 0;
  const auto debug=get(heap+0x3f,s,1);
  if(debug){address+=0x10;debug_header(address-0x10,metadata1,metadata2,s);}
  put(address,total,s);put(address+8,next,s);put(address+0xc,0,s);
  put(heap+0x34,address,s);
  if(next)put(next+0xc,address,s);else put(heap+0x38,address,s);
  return address;
}
std::uint32_t tail(std::uint32_t heap,std::uint32_t total,
    std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices& s) {
  // Complete48BF12. The final insert order differs from48BEAB.
  const auto previous=get(heap+0x38,s);
  std::uint32_t address{};
  if(!previous)address=get(heap+0x28,s);
  else {
    const auto debug=get(heap+0x3f,s,1);
    const auto begin=debug?previous-0x10:previous;
    address=get(previous,s)+begin;
  }
  auto limit=get(heap+0x2c,s);limit+=get(heap+0x28,s);
  if(address+total>limit)return 0;
  const auto debug=get(heap+0x3f,s,1);
  if(debug){address+=0x10;debug_header(address-0x10,metadata1,metadata2,s);}
  put(address,total,s);put(address+8,0,s);put(address+0xc,previous,s);
  if(previous)put(previous+8,address,s);else put(heap+0x34,address,s);
  put(heap+0x38,address,s);return address;
}
} //namespace
void vista_widget_pool_birth(std::uint32_t kind,
    const VistaWidgetPoolServices& s) {
  // Actual4085F0/4085BF jump tables, not a guessed cache-kind convention.
  const std::uint32_t capacity=(kind==2||kind==3)?0x80000u:
      (kind==4||kind==5)?0x40000u:0x1fa000u;
  const VistaWidgetIdentity allocator{0x8637b4};
  const auto vtable=get(allocator.value,s);const auto target=get(vtable,s);
  const auto heap=call(s.backing_allocate,allocator,target,capacity+0x50);
  if(heap) {
    clear_header(heap.value,s);put(heap.value,0x706f6f6c,s);
    // Actual43802C→68A189 valid fixed source name, count20, truncate=-1.
    // Unwritten name padding remains from the earlier source zero-fill.
    for(std::uint32_t i=0;i<0x20;++i) {
      const auto byte=get(0x7cba7c+i,s,1);
      put(heap.value+4+i,byte,s,1);if(!byte)break;
      if(i==0x1f)put(heap.value+4+i,0,s,1);
    }
    put(heap.value+0x28,(heap.value+0x4f)&0xfffffff0u,s);
    put(heap.value+0x34,0,s);put(heap.value+0x38,0,s);
    put(heap.value+0x3c,0,s,1);put(heap.value+0x3d,0,s,1);
    put(heap.value+0x3f,0,s,1);put(heap.value+0x24,allocator.value,s);
    put(heap.value+0x2c,capacity,s);put(heap.value+0x30,capacity,s);
  }
  put(heap_global,heap.value,s);
  if(heap) {
    put(heap.value+0x3c,1,s,1);
    const auto fresh=get(heap_global,s);put(fresh+0x3d,1,s,1);
  }
}
void vista_widget_pool_shutdown(const VistaWidgetPoolServices& s) {
  const VistaWidgetIdentity heap{get(heap_global,s)};
  if(!heap)return;
  const VistaWidgetIdentity allocator{get(heap.value+0x24,s)};
  clear_header(heap.value,s);
  const auto vtable=get(allocator.value,s);const auto target=get(vtable+4,s);
  call(s.backing_release,allocator,target,heap);put(heap_global,0,s);
}
bool vista_widget_pool_allocate(VistaWidgetIdentity id,std::uint32_t output,
    std::uint32_t size,std::uint32_t metadata1,std::uint32_t metadata2,
    const VistaWidgetPoolServices& s) {
  const auto heap=id.value;
  auto total=size+(get(heap+0x3f,s,1)?0x20u:0x10u);
  if(total&0xfu)total=(total|0xfu)+1u;
  std::uint32_t address{};
  if(get(heap+0x3d,s,1)) {
    address=gap(heap,total,metadata1,metadata2,s);
    if(!address)address=front(heap,total,metadata1,metadata2,s);
  }
  if(!address)address=tail(heap,total,metadata1,metadata2,s);
  if(!address)return false;
  const auto no_backref=get(heap+0x3c,s,1);
  put(address+4,no_backref?0:output,s);
  const auto length=get(address,s);
  const auto available=get(heap+0x30,s);put(heap+0x30,available-length,s);
  put(output,address+0x10,s);return true;
}
std::uint32_t vista_widget_pool_length(std::uint32_t argument,
    const VistaWidgetPoolServices& s) {
  const auto data=get(argument,s);return get(data-0x10,s);
}
void vista_widget_pool_unlink(VistaWidgetIdentity id,std::uint32_t argument,
    const VistaWidgetPoolServices& s) {
  const auto data=get(argument,s);const auto length=get(data-0x10,s);
  const auto available=get(id.value+0x30,s);put(id.value+0x30,available+length,s);
  const auto previous=get(data-4,s);const auto header=data-0x10;
  const auto next=get(header+8,s);
  if(previous)put(previous+8,next,s);else put(id.value+0x34,next,s);
  const auto fresh_next=get(header+8,s);
  const auto fresh_previous=get(header+0xc,s);
  if(fresh_next)put(fresh_next+0xc,fresh_previous,s);
  else put(id.value+0x38,fresh_previous,s);
}
void vista_widget_pool_retire(const VistaWidgetPoolServices& s) {
  VistaWidgetManagerLifetimeServices manager;
  manager.read=s.read;manager.write=s.write;manager.dispose=s.dispose;
  const auto retire=[&](std::uint32_t address){vista_widget_manager_retire({address},manager);};
  retire(0xd7190c);
  for(std::uint32_t i=0;i<5;++i) {
    retire(0xd71978+i*0x44);retire(0xd71acc+i*0x3c);
    retire(0xd71d24+i*0x44);retire(0xd71bf8+i*0x3c);
  }
  retire(0xd71e78);retire(0xd71ebc);
}
VistaWidgetIdentity vista_widget_ui_allocate(std::uint32_t size,
    std::uint32_t ignored,std::uint32_t scratch,const VistaWidgetPoolServices& s) {
  (void)ignored;
  return vista_widget_ui_allocate_raw(size,0,0,scratch,s);
}
VistaWidgetIdentity vista_widget_ui_allocate_raw(std::uint32_t size,
    std::uint32_t metadata1,std::uint32_t metadata2,std::uint32_t scratch,
    const VistaWidgetPoolServices& s) {
  auto heap=get(heap_global,s);
  if(!vista_widget_pool_allocate({heap},scratch,size,metadata1,metadata2,s)) {
    vista_widget_pool_retire(s);heap=get(heap_global,s);
    if(!vista_widget_pool_allocate({heap},scratch,size,metadata1,metadata2,s)) {
      (void)get(heap_global,s); // Actual48BD29 isRET; no invented diagnostic.
      return {};
    }
  }
  (void)get(heap_global,s);(void)vista_widget_pool_length(scratch,s);
  return {get(scratch,s)};
}
void vista_widget_ui_release(std::uint32_t argument,const VistaWidgetPoolServices& s) {
  (void)get(heap_global,s);(void)vista_widget_pool_length(argument,s);
  const auto heap=get(heap_global,s);vista_widget_pool_unlink({heap},argument,s);
}
} //namespace halo2
