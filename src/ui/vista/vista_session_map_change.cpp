#include "halo2/vista_session_map_change.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned width=4) {
  if(at+width>b.size())throw FormatError("Truncated Vista map-change owner");
  std::uint32_t result{};
  for(unsigned i=0;i<width;++i)result|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return result;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned width=4) {
  for(unsigned i=0;i<width;++i)b[at+i]=std::byte(v>>(8*i));
}
template<class F>const F& required(const F& f) {
  if(!f)throw FormatError("Missing reached Vista map-change service owner");
  return f;
}
bool same(std::span<const std::byte> b,std::size_t a,std::size_t c,unsigned n) {
  return std::equal(b.begin()+std::ptrdiff_t(a),b.begin()+std::ptrdiff_t(a+n),b.begin()+std::ptrdiff_t(c));
}
bool zero(std::span<const std::byte> b,std::size_t a,unsigned n) {
  return std::all_of(b.begin()+std::ptrdiff_t(a),b.begin()+std::ptrdiff_t(a+n),[](auto v){return v==std::byte{};});
}
}
std::optional<VistaSessionPresenceContext> vista_session_map_change_presence(
    const VistaSessionMapChangeCall& call,std::span<const std::byte> options) {
  if(call.intent==VistaSessionMapChangeIntent::select_scenario)return VistaSessionPresenceContext{0,5,0};
  if(call.intent==VistaSessionMapChangeIntent::ui_reason)return VistaSessionPresenceContext{0,0x8001,call.argument};
  if(call.intent==VistaSessionMapChangeIntent::select_map) {
    constexpr std::array<std::array<std::uint32_t,2>,23> maps{{
      {10,4},{20,3},{30,8},{40,11},{50,1},{60,6},{70,7},{80,2},{100,5},
      {110,9},{120,12},{800,10},{1000,16},{1001,18},{1002,19},{1101,21},
      {1109,14},{1200,20},{1201,17},{1300,13},{1302,15},{91101,23},{444678,22}}};
    for(const auto& map:maps)if(map[0]==call.argument)return VistaSessionPresenceContext{0,5,map[1]};
    return {};
  }
  if(call.intent==VistaSessionMapChangeIntent::select_variant) {
    const auto engine=read(options,0x2a0+0x44);
    constexpr std::array<std::uint32_t,10> values{0xffffffffu,0,1,2,3,0xffffffffu,0xffffffffu,4,5,6};
    if(engine<values.size() && values[engine]!=0xffffffffu)return VistaSessionPresenceContext{0,2,values[engine]};
  }
  return {};
}
bool vista_session_multiplayer_options_valid(std::span<const std::byte> b) {
  if(b.size()<vista_world_options_size)throw FormatError("Truncated Vista world options");
  if(read(b,0)!=2 || read(b,4,1)>=6)return false;
  const auto rate=std::bit_cast<std::int16_t>(std::uint16_t(read(b,8,2)));
  if(rate<=0 || rate>300 || (read(b,0x1c,1)==0 && read(b,0x88,2)==0) || read(b,0x290,2)>=16)return false;
  const auto machines=read(b,0x3d4);
  if(machines&0xfffe0000u)return false;
  for(unsigned i=0;i<17;++i)if(machines&(1u<<i))
    for(unsigned j=i+1;j<17;++j)if((machines&(1u<<j)) && same(b,0x3d8+i*6,0x3d8+j*6,6))return false;
  unsigned players=0;
  for(unsigned i=0;i<16;++i) {
    const auto at=0x448+i*0xd4;if(read(b,at,1)==0)continue;++players;
    if(read(b,at+2,2)>=4 || zero(b,at+0xe,8))return false;
    for(unsigned j=i+1;j<16;++j) {
      const auto other=0x448+j*0xd4;
      if(read(b,other,1)!=0 && same(b,at+0xe,other+0xe,8))return false;
    }
    if(read(b,at+1,1)!=0) {
      if(!zero(b,at+8,6))return false;
    } else {
      bool joined=false;
      for(unsigned j=0;j<17;++j)
        if((machines&(1u<<j)) && same(b,at+8,0x3d8+j*6,6)){joined=true;break;}
      if(!joined)return false;
    }
  }
  return (read(b,6,1)!=0 || players>=1) && players<=16 && read(b,0x43e,1)!=0;
}
VistaSessionMapChangeRequest vista_session_build_map_change(
    std::span<const std::byte> b,const VistaSessionMapChangeServices& s) {
  if(b.size()<vista_session_source_size)throw FormatError("Truncated Vista map-change session");
  VistaSessionMapChangeRequest result;
  const auto state=read(b,0x73a0);
  if(state<3 || state>8)return result;
  const auto connection=std::bit_cast<std::int32_t>(read(b,0x4dd4));
  if(connection<0 || connection>=3 || read(b,0x5101,1)==0 || read(b,0x5104)==0)return result;
  const bool selected=read(b,0x4dd8)!=0xffffffffu && read(b,0x4cbc,2)!=0;
  const auto campaign=read(b,0x5eb8,2);
  const auto membership=std::size_t(0x70);
  const auto local=read(b,0x64),host=read(b,0x71c0);
  if(local>=17 || host>=17)throw FormatError("Vista map-change peer outside source membership");
  const auto count=read(b,membership+0x11e4),role=read(b,membership+0x14);
  if(campaign!=0xffff && (std::bit_cast<std::int32_t>(count)>2 || std::bit_cast<std::int32_t>(role)>1))return result;
  VistaWorldOptions options{};auto out=std::span<std::byte>(options);
  write(out,0,1);write(out,4,1,1);
  write(out,0x298,required(s.constructor_byte)(),1);
  const auto configured=required(s.configured_tick_rate)();
  write(out,8,std::uint32_t(configured>0?configured:60),2);
  const auto random=required(s.random_identity)();
  std::copy(random.begin(),random.end(),out.begin()+0x10);
  write(out,0x80,0xffffffffu);write(out,0x84,0xffffffffu);write(out,0x29a,1,2);
  write(out,0x18,0x78a8);
  const auto control=connection==0?1u:unsigned(connection*2)+unsigned(local==host);
  write(out,4,control,1);
  const auto type=read(b,0x1c);
  if(type<=2)write(out,5,type+1,1);
  write(out,6,read(b,0x4c74,1),1);
  write(out,0x10,read(b,0x4dc8));write(out,0x14,read(b,0x4dcc));write(out,0x18,read(b,0x4dd0));
  if(selected) {
    write(out,0x80,read(b,0x4cb8));write(out,0x84,read(b,0x4dd8));
    unsigned n=0;
    while(n<259 && read(b,0x4cbc+n*2,2)!=0){write(out,0x88+n*2,read(b,0x4cbc+n*2,2),2);++n;}
    write(out,0x88+n*2,0,2);
  } else {
    write(out,0x1c,1,1);
    std::copy_n(b.begin()+0x4ddc,0x60,out.begin()+0x1e);write(out,0x88,0,2);
  }
  if(campaign!=0xffff) {
    write(out,0,1);write(out,0x29a,campaign,2);write(out,0x29c,std::bit_cast<std::int32_t>(count)>1,1);write(out,0x29d,0,1);
  } else {
    write(out,0,2);std::copy_n(b.begin()+0x4e3c,0x130,out.begin()+0x2a0);
  }
  std::copy_n(b.begin()+0x5104,0x74,out.begin()+0x3d4);
  std::copy_n(b.begin()+0x5178,0xd40,out.begin()+0x448);
  write(out,0x43e,1,1);
  std::copy_n(b.begin()+std::ptrdiff_t(membership+host*0x10c+0x22),6,out.begin()+0x43f);
  required(s.pause)(false);
  result.options=options;
  const auto valid=s.validate_options?s.validate_options(options):std::uint8_t(vista_session_multiplayer_options_valid(options));
  if(valid && required(s.request_load)(options))result.accepted=1;
  required(s.pause)(true);
  return result;
}
VistaSessionMapChangeRequest vista_session_apply_map_change(
    VistaSessionTransitionState& state,std::span<const std::byte> b,const VistaSessionMapChangeServices& s) {
  auto result=vista_session_build_map_change(b,s);
  if(!result.accepted) {
    const auto phase=read(b,0x73a0);
    result.calls[result.call_count++]={phase>=5 && phase<=8?VistaSessionMapChangeIntent::phase_write:VistaSessionMapChangeIntent::remote_close,phase>=5 && phase<=8?1u:0u};
    return result;
  }
  const auto loaded=required(s.loaded_options)();
  if(loaded.size()<vista_world_options_size)throw FormatError("Missing actual loaded Vista options");
  result.loaded.emplace();std::copy_n(loaded.begin(),vista_world_options_size,result.loaded->begin());
  state.world_identity={read(loaded,0x10),read(loaded,0x14)};
  if(read(b,0x4c78)==0) {
    if(read(loaded,0x1c,1)==0)
      result.calls[result.call_count++]={VistaSessionMapChangeIntent::select_map,read(loaded,0x84)};
    else result.calls[result.call_count++]={VistaSessionMapChangeIntent::select_scenario,0};
    result.calls[result.call_count++]={VistaSessionMapChangeIntent::select_variant,0};
    result.calls[result.call_count++]={VistaSessionMapChangeIntent::ui_reason,read(b,0x1c)==2?6u:7u};
  } else result.calls[result.call_count++]={VistaSessionMapChangeIntent::ui_reason,8};
  return result;
}
} // namespace halo2
