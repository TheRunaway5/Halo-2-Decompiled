#include "halo2/source_player_unit.hpp"
#include <bit>
#include <stdexcept>
#include <utility>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed32(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
std::int16_t signed16(std::uint32_t x){return std::bit_cast<std::int16_t>(std::uint16_t(x));}
class Owner {
  const SourcePlayerUnitBindings &b;
  const SourcePlayerUnitServices &s;
  const SourcePlayerUnitObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,unsigned size)const{
    if(!s.memory)throw std::runtime_error("Original player-unit backing unavailable");
    auto result=s.memory(address,size);
    if(result.size()!=size)throw std::runtime_error("Original player-unit extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);const auto raw=memory(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(8*i));
    if(observe)observe({address,before,value&(size==4?none:(1u<<(size*8))-1),size});
  }
  template<class F,class... Args>static auto call(const F &function,Args... args){
    if(!function)throw std::runtime_error("Original player-unit service unavailable");
    return function(args...);
  }
  std::uint32_t player(std::uint32_t datum)const{
    return read(read(b.players_slot)+0x44)+(datum&65535u)*0x21c;
  }
  std::uint32_t object(std::uint32_t datum)const{
    return read(read(read(b.objects_slot)+0x44)+(datum&65535u)*12+8);
  }
  // Both iterators capture the pool, reload high-water at each advance, and
  // scan its availability bitmap. The status owner alone checks a NULL row.
  std::uint32_t next(std::uint32_t pool,std::uint32_t previous)const{
    auto index=previous+1;
    if(signed32(index)<0)return none;
    const auto high=read(pool+0x38);
    if(signed32(index)>=signed32(high))return none;
    const auto bitmap=read(pool+0x48);
    do{
      if(read(bitmap+(index>>5)*4)&(1u<<(index&31)))return index;
      ++index;
    }while(signed32(index)<signed32(high));
    return none;
  }
  void status()const{
    const auto globals=read(b.player_globals_slot),pool=read(b.players_slot);
    write(globals+4,1,1);write(globals+5,0,1);
    auto index=none;
    for(;;){
      index=next(pool,index);if(index==none)return;
      const auto row=read(pool+0x44)+index*read(pool+0x24);if(!row)return;
      if(read(row+0x2c)!=none || (read(row+2,1)&8))write(globals+4,0,1);
      else write(globals+5,1,1);
    }
  }
public:
  Owner(const SourcePlayerUnitBindings &bindings,const SourcePlayerUnitServices &services,
      const SourcePlayerUnitObserver &observer):b(bindings),s(services),observe(observer){}
  void bind(std::uint32_t datum,std::uint32_t unit)const{
    const auto row=player(datum);const auto previous=read(row+0x2c);
    if(previous==unit)return;
    if(previous!=none){
      const auto body=object(previous);
      if(signed16(read(row+0x28,2))!=-1){
        call(s.bind_view,signed16(read(row+0x28,2)),none);
        call(s.bind_control,signed16(read(row+0x28,2)),none,std::int32_t(-1));
      }
      write(body+0x13c,none);write(body+0x140,datum);
      const auto entity=read(object(read(row+0x2c))+0xd4);
      if(entity!=none)call(s.mark_network_dirty,entity,std::uint32_t(0x400));
      call(s.activate_unit,read(row+0x2c),false);
      write(row+0x2c,none);
    }
    if(signed16(read(row+0x28,2))!=-1)call(s.reset_local_state,signed16(read(row+0x28,2)));
    write(row+0x38,0);write(row+0x3c,0,2);write(row+0x17c,0,2);write(row+0x17e,0,2);
    if(unit!=none){
      const auto body=object(unit);
      write(body+0x13c,datum);write(body+0x140,none);
      const auto entity=read(object(unit)+0xd4);
      if(entity!=none)call(s.mark_network_dirty,entity,std::uint32_t(0x400));
      call(s.activate_unit,unit,true);
      const auto user=signed16(read(row+0x28,2));
      write(row+0x2c,unit);write(row+0x30,none);
      if(user!=-1){
        call(s.bind_view,user,unit);
        auto representation=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(row+0x88,1))));
        if(b.version==SourcePlayerUnitVersion::Retail && read(read(b.game_globals_slot)+8)==2){
          if(representation==0)representation=2;else if(representation==1)representation=3;
        }
        call(s.bind_control,signed16(read(row+0x28,2)),unit,representation);
        call(s.refresh_user,signed16(read(row+0x28,2)));
      }
      if(read(b.engine_table+read(read(b.engine_globals_slot)+0xc14)*4))
        write(body+0xf0,call(s.parameter_bits,datum,std::int32_t(2)));
    }
    status();
  }
  void release(std::uint32_t datum)const{
    const auto type=read(object(datum)+0xaa,1);
    if((1u<<(type&31))&3){
      const auto pool=read(b.players_slot);auto index=none;
      for(;;){
        index=next(pool,index);if(index==none)break;
        const auto row=read(pool+0x44)+index*read(pool+0x24);
        const auto owner=(read(row,2)<<16)|index;
        if(read(row+0x2c)==datum){
          const auto fixed=player(owner);
          write(fixed+0x30,read(fixed+0x2c));
          bind(owner,none);status();
        }
        if(read(row+0x174)==datum)write(row+0x174,none);
        if(read(row+0x30)==datum)write(row+0x30,none);
      }
    }
    call(s.forget_view_object,datum);
  }
};
} // namespace
void bind_source_player_unit(const SourcePlayerUnitBindings &b,std::uint32_t player,
    std::uint32_t unit,const SourcePlayerUnitServices &s,const SourcePlayerUnitObserver &observer){
  Owner(b,s,observer).bind(player,unit);
}
void release_source_object_player_references(const SourcePlayerUnitBindings &b,std::uint32_t object,
    const SourcePlayerUnitServices &s,const SourcePlayerUnitObserver &observer){
  Owner(b,s,observer).release(object);
}
} // namespace halo2
