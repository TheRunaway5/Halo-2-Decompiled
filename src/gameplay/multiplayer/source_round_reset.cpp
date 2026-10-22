#include "halo2/source_round_reset.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::int32_t signed32(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
std::int32_t signed16(std::uint16_t x){return std::bit_cast<std::int16_t>(x);}
class Owner {
  const SourceRoundResetBindings &b;
  const SourceRoundResetServices &s;
  const SourceRoundResetObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,unsigned size)const{
    if(!s.memory)throw std::runtime_error("Original round-reset backing unavailable");
    auto result=s.memory(address,size);
    if(result.size()!=size)throw std::runtime_error("Original round-reset extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=memory(address,size);std::uint32_t result=0;
    for(unsigned i=0;i<size;++i)result|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return result;
  }
  void write(std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);auto raw=memory(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(8*i));
    if(observe)observe({address,before,value&(size==4?none:(1u<<(size*8))-1),size});
  }
  template<class F,class... Args>static void call(const F &function,Args... args){
    if(!function)throw std::runtime_error("Original round-reset service unavailable");
    function(args...);
  }
  std::uint32_t body(std::uint32_t pool,std::uint32_t datum)const{
    return read(read(pool+0x44)+(datum&65535u)*12+8);
  }
  std::uint32_t next_player(std::uint32_t pool,std::uint32_t &previous)const{
    for(;;){
      auto index=previous+1;
      const auto high=read(pool+0x38);
      if(signed32(index)<0 || signed32(index)>=signed32(high))return 0;
      const auto bitmap=read(pool+0x48);
      while(!(read(bitmap+(index>>5)*4)&(1u<<(index&31)))){
        ++index;if(signed32(index)>=signed32(high))return 0;
      }
      previous=index;
      const auto row=read(pool+0x44)+index*read(pool+0x24);
      if(!row || read(row+0x2c)!=none)return row;
    }
  }
  struct Iterator {std::uint32_t mask,datum=none;std::uint16_t cursor{};};
  std::uint32_t next_object(Iterator &it)const{
    const auto pool=read(b.objects_slot),high=read(pool+0x38);
    auto index=signed16(it.cursor);
    auto header=read(pool+0x44)+std::uint32_t(index)*12;
    while(index<signed32(high)){
      const auto salt=read(header,2),datum=(salt<<16)|std::uint32_t(index);
      ++it.cursor;
      // The caller's original required-header-flags byte is0.
      if(salt && (it.mask&(1u<<(read(header+3,1)&31)))){
        it.datum=datum;return read(header+8);
      }
      index=signed16(it.cursor);header+=12;
    }
    return 0;
  }
  void erase(std::uint32_t datum)const{
    call(s.deactivate_object,datum,true);call(s.delete_object,datum,true);
  }
public:
  Owner(const SourceRoundResetBindings &bindings,const SourceRoundResetServices &services,
      const SourceRoundResetObserver &observer):b(bindings),s(services),observe(observer){}
  void reset()const{
    const auto game=read(b.game_globals_slot);
    const bool full=(read(game+0x184)&8)!=0;
    if(read(game+0xc,1)!=4){
      const auto players=read(b.players_slot);auto index=none;
      while(const auto row=next_player(players,index))erase(read(row+0x2c));
      Iterator it{full?none:0xfffff83fu};
      auto object=next_object(it);
      if(object){
        auto pool=read(b.objects_slot);
        do{
          auto ancestor=it.datum,root=none;
          while(ancestor!=none){root=ancestor;ancestor=read(body(pool,ancestor)+0x14);}
          bool remove;
          if(read(object+0xaf,1)!=255 || read(body(pool,root)+0xaf,1)!=255)remove=full;
          else if(read(object+0xaa,1)==2){
            const auto definition=read(object)&65535u;
            const auto tag=read(read(b.tags_slot)+definition*16+8);
            remove=read(tag+0x290,2)==0;
          }else if(read(object+0xaa,1)==11)remove=full || (read(object+4)&(1u<<14));
          else remove=true;
          if(remove){erase(it.datum);pool=read(b.objects_slot);}
          object=next_object(it);
        }while(object);
      }
    }else{
      if(!full)return;
      Iterator it{none};
      for(auto object=next_object(it);object;object=next_object(it))
        if(read(object+0x14)==none && read(object+0xd4)==none)erase(it.datum);
    }
    if(full){
      call(s.reset_decals);
      auto mask=read(b.mask_slot);
      for(unsigned i=0;i<0x1080;++i,mask+=4)write(mask,none);
      write(mask,255,1);
      call(s.reset_contrails);call(s.reset_effects);call(s.reset_registry);call(s.reset_lights);
      if(read(b.engine_table+read(read(b.engine_globals_slot)+0xc14)*4))call(s.reset_sound);
      call(s.reset_scenario,std::uint32_t(0));
      call(s.scenario_callback,read(read(b.game_globals_slot)+8));
      call(s.place_scenario);
    }
  }
};
} // namespace
void source_round_reset(const SourceRoundResetBindings &b,const SourceRoundResetServices &s,
    const SourceRoundResetObserver &observer){Owner(b,s,observer).reset();}
} // namespace halo2
