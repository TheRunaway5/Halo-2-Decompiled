#include "halo2/object_body_heap.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
class Owner {
  const SourceObjectBodyHeapServices &s;
  const ObjectConstructionObserver &observe;
  std::uint32_t read(std::uint32_t a,unsigned n=4)const {
    if(!s.read)throw std::runtime_error("Original object body heap read owner unavailable");
    auto b=s.read(a,n);if(b.size()!=n)throw std::runtime_error("Original object body heap read extent unavailable");
    std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
  }
  void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4)const {
    auto before=read(a,n);if(!s.memory)throw std::runtime_error("Original object body heap write owner unavailable");
    auto b=s.memory(a,n);if(b.size()!=n)throw std::runtime_error("Original object body heap write extent unavailable");
    if(n<4)v&=(1u<<(8*n))-1;for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8*i));if(observe)observe({pc,a,n,before,v});
  }
  std::uint32_t tick()const {
    if(!s.tick_count)throw std::runtime_error("Original object body heap debug clock owner unavailable");return s.tick_count();
  }
  void name32(std::uint32_t dest,std::uint32_t source)const {
    // Actual complete326970 bounded primitive, count32. Preserve its scalar,
    // aligned word, and zero-fill store sequence rather than clearing payload.
    std::uint32_t count=32,remaining=count;
    if(source&3) {
      do {
        const auto value=read(source++,1);write(0x3269a1,dest++,value,1);--count;
        if(!count)return;
        if(!value) {
          while(dest&3){write(0x3269e6,dest++,0,1);if(!--count)return;}
          remaining=count;
          for(std::uint32_t i=0;i<(count>>2);++i){write(0x326a79,dest,0);dest+=4;}
          for(std::uint32_t i=0;i<(remaining&3);++i)write(0x326a03,dest++,0,1);
          return;
        }
      }while(source&3);
      remaining=count;
    }
    auto words=count>>2;
    while(words) {
      const auto value=read(source);source+=4;unsigned zero=4;
      for(unsigned i=0;i<4;++i)if(((value>>(8*i))&255)==0){zero=i;break;}
      if(zero==4){write(0x326a15,dest,value);dest+=4;--words;continue;}
      const std::uint32_t pcs[]{0x326a6b,0x326a65,0x326a5b,0x326a51};
      write(pcs[zero],dest,zero==0?0:value&((1u<<(8*zero))-1));dest+=4;--words;
      while(words){write(0x326a79,dest,0);dest+=4;--words;}
      for(std::uint32_t i=0;i<(remaining&3);++i)write(0x326a03,dest++,0,1);
      return;
    }
    auto tail=remaining&3;
    while(tail){const auto value=read(source++,1);write(0x3269c8,dest++,value,1);--tail;
      if(!value){while(tail){write(0x326a03,dest++,0,1);--tail;}return;}}
  }
  std::uint32_t physical_size(std::uint32_t heap,std::uint32_t requested)const {
    auto value=requested+(read(heap+0x3f,1)?0x20u:0x10u);
    if(value&15)value=(value|15u)+1;return value;
  }
  std::uint32_t append(std::uint32_t h,std::uint32_t size,std::uint32_t m1,std::uint32_t m2)const {
    const auto last=read(h+0x38);std::uint32_t p;
    if(!last)p=read(h+0x28);
    else{auto base=last;if(read(h+0x3f,1))base-=0x10;p=read(last)+base;}
    const auto end=read(h+0x2c)+read(h+0x28);
    if(p+size>end)return 0;
    if(read(h+0x3f,1)) {
      p+=0x10;
      if(read(h+0x3f,1)){
        const auto debug=p-0x10;if(debug){write(0x18ffe5,debug,0x68656164);write(0x18ffeb,debug+4,m1);write(0x18ffee,debug+8,m2);write(0x18fff6,debug+0xc,tick());}
      }
    }
    write(0x190000,p,size);write(0x190002,p+8,0);write(0x190009,p+0xc,last);
    if(!last){write(0x19000f,h+0x34,p);write(0x190012,h+0x38,p);}
    else{write(0x19001c,last+8,p);write(0x19001f,h+0x38,p);}return p;
  }
  std::uint32_t head(std::uint32_t h,std::uint32_t size,std::uint32_t m1,std::uint32_t m2)const {
    const auto first=read(h+0x34);std::uint32_t end;
    if(!first)end=read(h+0x2c)+read(h+0x28);
    else{end=first;if(read(h+0x3f,1))end-=0x10;}
    auto p=read(h+0x28);if(p+size>end)return 0;
    if(read(h+0x3f,1)){
      p+=0x10;if(read(h+0x3f,1)){const auto debug=p-0x10;if(debug){write(0x19007a,debug,0x68656164);write(0x190081,debug+4,m1);write(0x190084,debug+8,m2);write(0x19008c,debug+0xc,tick());}}
    }
    write(0x190096,p,size);write(0x190098,p+8,first);write(0x19009b,p+0xc,0);write(0x1900a2,h+0x34,p);
    if(!first)write(0x1900a7,h+0x38,p);else write(0x1900b1,first+0xc,p);return p;
  }
  std::uint32_t insert(std::uint32_t h,std::uint32_t p,std::uint32_t previous,std::uint32_t next,std::uint32_t size,std::uint32_t m1,std::uint32_t m2)const {
    if(read(h+0x3f,1)){
      p+=0x10;if(read(h+0x3f,1)){const auto debug=p-0x10;if(debug){write(0x18ff42,debug,0x68656164);write(0x18ff48,debug+4,m1);write(0x18ff4b,debug+8,m2);write(0x18ff53,debug+0xc,tick());}}
    }
    write(0x18ff61,p,size);write(0x18ff63,p+8,next);write(0x18ff66,p+0xc,previous);
    if(!previous)write(0x18ff6b,h+0x34,p);else write(0x18ff70,previous+8,p);
    if(!next)write(0x18ff77,h+0x38,p);else write(0x18ff81,next+0xc,p);return p;
  }
  std::uint32_t middle(std::uint32_t h,std::uint32_t size,std::uint32_t m1,std::uint32_t m2)const {
    auto previous=read(h+0x34);if(!previous)return 0;
    for(;;){const auto next=read(previous+8);if(next){const bool debug=read(h+0x3f,1)!=0;const auto end=next-(debug?0x10u:0u),base=previous-(debug?0x10u:0u);const auto p=read(previous)+base;
        if(p+size<=end)return insert(h,p,previous,next,size,m1,m2);}
      previous=next;if(!previous)return 0;}
  }
public:
  Owner(const SourceObjectBodyHeapServices &services,const ObjectConstructionObserver &observer):s(services),observe(observer){}
  void initialize(std::uint32_t h,std::uint32_t capacity,std::uint32_t name,std::uint32_t context)const {
    for(unsigned i=0;i<16;++i)write(0x18fc8a,h+4*i,0);
    write(0x18fc97,h,0x706f6f6c);name32(h+4,name);write(0x18fca8,h+0x23,0,1);
    write(0x18fcb4,h+0x24,context);write(0x18fcb7,h+0x28,(h+0x4f)&~15u);write(0x18fcba,h+0x2c,capacity);write(0x18fcbd,h+0x30,capacity);
    write(0x18fcc0,h+0x34,0);write(0x18fcc3,h+0x38,0);write(0x18fcc6,h+0x3c,0,1);write(0x18fcc9,h+0x3d,0,1);write(0x18fccc,h+0x3f,0,1);
  }
  bool allocate(std::uint32_t h,std::uint32_t output,std::uint32_t requested,std::uint32_t m1,std::uint32_t m2)const {
    const auto size=physical_size(h,requested);std::uint32_t block=0;
    if(read(h+0x3d,1)){block=middle(h,size,m1,m2);if(!block)block=head(h,size,m1,m2);}
    if(!block)block=append(h,size,m1,m2);if(!block)return false;
    const auto owner=read(h+0x3c,1)?0:output;write(0x18fd4c,block+4,owner);write(0x18fd51,h+0x30,read(h+0x30)-read(block));write(0x18fd5a,output,block+0x10);return true;
  }
  void free(std::uint32_t h,std::uint32_t output)const {
    const auto block=read(output)-0x10;const auto size=read(block);write(0x18fe69,h+0x30,read(h+0x30)+size);
    auto previous=read(block+0xc);if(previous)write(0x18fe76,previous+8,read(block+8));else write(0x18fe7e,h+0x34,read(block+8));
    const auto next=read(block+8);if(next)write(0x18fe8c,next+0xc,read(block+0xc));else write(0x18fe93,h+0x38,read(block+0xc));
  }
  bool resize(std::uint32_t h,std::uint32_t output,std::uint32_t requested,std::uint32_t m1,std::uint32_t m2,const SourceObjectBodyHeapFrame &frame)const {
    const auto block=read(output)-0x10;const auto size=physical_size(h,requested);auto next=read(block+8);if(!next)next=read(h+0x2c)+read(h+0x28);
    if(block+size<=next){write(0x18fdb7,h+0x30,read(h+0x30)+read(block)-size);write(0x18fdba,block,size);
      if(read(h+0x3f,1)){const auto debug=block-0x10;if(debug){write(0x18fdd0,debug+4,m1);write(0x18fdd3,debug+8,m2);write(0x18fddb,debug+0xc,tick());}}return true;}
    if(!allocate(h,frame.resize_temporary_pointer,requested,m1,m2))return false;
    const auto overhead=read(h+0x3f,1)?0x20u:0x10u,copy=read(block)-overhead;auto from=read(output),to=read(frame.resize_temporary_pointer);
    for(std::uint32_t i=0;i<(copy>>2);++i){write(0x18fe20,to,read(from));to+=4;from+=4;}
    for(std::uint32_t i=0;i<(copy&3);++i)write(0x18fe2b,to++,read(from++,1),1);
    free(h,output);const auto owner=read(h+0x3c,1)?0:output,new_pointer=read(frame.resize_temporary_pointer);write(0x18fe46,new_pointer-0xc,owner);write(0x18fe4d,output,read(frame.resize_temporary_pointer));return true;
  }
};
}
void initialize_source_object_body_heap(std::uint32_t h,std::uint32_t capacity,std::uint32_t name,std::uint32_t context,const SourceObjectBodyHeapServices &s,const ObjectConstructionObserver &o){Owner(s,o).initialize(h,capacity,name,context);}
bool allocate_source_object_body(std::uint32_t h,std::uint32_t output,std::uint32_t bytes,std::uint32_t first,std::uint32_t second,const SourceObjectBodyHeapServices &s,const ObjectConstructionObserver &o){return Owner(s,o).allocate(h,output,bytes,first,second);}
bool resize_source_object_body(std::uint32_t h,std::uint32_t output,std::uint32_t bytes,std::uint32_t first,std::uint32_t second,const SourceObjectBodyHeapFrame &f,const SourceObjectBodyHeapServices &s,const ObjectConstructionObserver &o){return Owner(s,o).resize(h,output,bytes,first,second,f);}
void free_source_object_body(std::uint32_t h,std::uint32_t output,const SourceObjectBodyHeapServices &s,const ObjectConstructionObserver &o){Owner(s,o).free(h,output);}
} // namespace halo2
