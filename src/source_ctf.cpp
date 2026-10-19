#include "halo2/source_ctf.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint16_t half(std::span<const std::uint8_t> bytes,std::size_t at) {
  return std::uint16_t(bytes[at])|std::uint16_t(std::uint16_t(bytes[at+1])<<8);
}
std::uint32_t word(std::span<const std::uint8_t> bytes,std::size_t at) {
  return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
      (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);
}
void half(std::span<std::uint8_t> bytes,std::size_t at,std::uint16_t value) {
  bytes[at]=std::uint8_t(value);bytes[at+1]=std::uint8_t(value>>8);
}
void word(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)bytes[at+i]=std::uint8_t(value>>(i*8));
}
float real(std::span<const std::uint8_t> bytes,std::size_t at) {
  return std::bit_cast<float>(word(bytes,at));
}
void real(std::span<std::uint8_t> bytes,std::size_t at,float value) {
  word(bytes,at,std::bit_cast<std::uint32_t>(value));
}
std::int16_t signed_half(std::uint16_t value) {return std::bit_cast<std::int16_t>(value);}
std::int16_t low_half(std::int32_t value) {return signed_half(std::uint16_t(std::uint32_t(value)));}
void markers_supported(std::span<const SourceCtfMarker> markers) {
  // Original1A8850 sign-extends its16-bit iteration index. Above32767 it wraps
  // into unavailable negative source memory rather than completing this owner.
  if(markers.size()>32767)throw std::runtime_error("CTF markers exceed the original signed16 iterator");
}
bool eligible(const SourceCtfMarker& marker,const SourceCtfContext& context) {
  const auto flags=half(marker.bytes,0x16);
  return ((flags&1u) && context.objective_kind!=1 && context.objective_kind!=2) ||
      ((flags&2u) && context.objective_kind==1) || ((flags&4u) && context.objective_kind==2);
}
}

std::int32_t source_ctf_query_markers(std::span<const SourceCtfMarker> markers,
    const SourceCtfMarkerQuery& query,std::span<std::int32_t> output) {
  markers_supported(markers);
  if(output.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))
    throw std::runtime_error("CTF marker output exceeds the original signed capacity");
  const float maximum_squared=query.maximum_distance*query.maximum_distance;
  float replacement_distance=0.f;std::int32_t written=0;
  for(std::size_t index=0;index<markers.size();++index) {
    const auto& row=markers[index].bytes;
    if((query.type!=-1 && query.type!=signed_half(half(row,0x10))) ||
       (query.team!=-1 && query.team!=signed_half(half(row,0x12))) ||
       (query.channel!=-1 && query.channel!=signed_half(half(row,0x14))))continue;
    float squared=0.f;
    if(query.point) {
      const auto& point=*query.point;
      const float dx=point[0]-real(row,0),dy=point[1]-real(row,4),dz=point[2]-real(row,8);
      const float xx=dx*dx,zz=dz*dz,yy=dy*dy;
      squared=(zz+xx)+yy; // Literal scalar SSE order, not a vector dot product.
      if(query.maximum_distance>=0.f && squared>maximum_squared)continue;
      // This source comparison uses x87 differences with53-bit intermediates,
      // not a narrowed scalar SSE subtraction.
      if(query.maximum_vertical_distance>0.f &&
          std::fabs(double(real(row,8))-double(point[2]))>double(query.maximum_vertical_distance))continue;
    }
    if(std::size_t(written)<output.size())output[std::size_t(written++)]=std::int32_t(index);
    else if(written==1 && replacement_distance>squared) {
      output[0]=std::int32_t(index);replacement_distance=squared;
    }
  }
  return written;
}

std::int32_t source_ctf_select_marker(std::span<const SourceCtfMarker> markers,
    const SourceCtfContext& context,std::int32_t team,bool spawn,std::int32_t* second) {
  SourceCtfMarkerQuery query;
  query.type=std::int16_t(spawn ? (context.source_engine==9 ? 2 : 0) :
      (context.source_engine==9 ? 3 : 1));
  query.team=low_half(team);query.channel=0;
  std::array<std::int32_t,8> selected{};
  const auto count=source_ctf_query_markers(markers,query,selected);
  std::int32_t first=-1;
  for(std::int32_t i=0;i<count;++i) {
    const auto index=selected[std::size_t(i)];
    if(!eligible(markers[std::size_t(index)],context))continue;
    if(first!=-1) {if(second)*second=index;break;}
    first=index;
  }
  return first;
}

void source_ctf_build_capture_regions(SourceCtfState& state,
    std::span<const SourceCtfMarker> markers,const SourceCtfContext& context,std::int32_t team) {
  markers_supported(markers);
  if(team<0 || team>8)throw std::runtime_error("CTF capture regions require an original team0..8 bank slot");
  for(unsigned channel=0;channel<2;++channel) {
    const auto selected=signed_half(half(state.bytes,(channel ? 0x26u : 0x14u)+unsigned(team)*2));
    if(selected==-1)continue;
    if(selected<0 || std::size_t(selected)>=markers.size())
      throw std::runtime_error("CTF capture region references an unavailable authored marker");
    SourceCtfMarkerQuery query;query.type=std::int16_t(context.source_engine==9 ? 3 : 1);
    query.team=low_half(team);query.channel=std::int16_t(channel+1);
    std::array<std::int32_t,8> bounds{};
    const auto count=source_ctf_query_markers(markers,query,bounds);
    const auto& origin=markers[std::size_t(selected)].bytes;
    const auto at=(channel ? 0xa8u : 0x3cu)+unsigned(team)*12;
    real(state.bytes,at,std::bit_cast<float>(0x3f000000u));
    real(state.bytes,at+4,0.f);real(state.bytes,at+8,0.f);
    for(std::int32_t i=0;i<count;++i) {
      const auto& marker=markers[std::size_t(bounds[std::size_t(i)])];
      if(!eligible(marker,context))continue;
      const double dx=double(real(origin,0))-double(real(marker.bytes,0));
      const double dy=double(real(origin,4))-double(real(marker.bytes,4));
      const double xx=dx*dx,yy=dy*dy;
      const float distance=float(std::sqrt(xx+yy));
      const auto radius=real(state.bytes,at);
      real(state.bytes,at,distance>radius ? distance : radius);
      const float lower=real(origin,8)-real(marker.bytes,8),upper=real(marker.bytes,8)-real(origin,8);
      const auto previous_lower=real(state.bytes,at+4),previous_upper=real(state.bytes,at+8);
      // Original COMISS/JBE unordered behavior chooses the new height operand.
      real(state.bytes,at+4,previous_lower>lower ? previous_lower : lower);
      real(state.bytes,at+8,previous_upper>upper ? previous_upper : upper);
    }
    real(state.bytes,at+4,real(state.bytes,at+4)+std::bit_cast<float>(0x3dcccccdu));
    real(state.bytes,at+8,real(state.bytes,at+8)+std::bit_cast<float>(0x3f666666u));
  }
}

bool source_ctf_initialize(SourceCtfState& state,std::span<const SourceCtfMarker> markers,
    const SourceCtfContext& context) {
  markers_supported(markers);state.bytes.fill(0);
  for(unsigned at=0x26;at<=0x32;at+=4)word(state.bytes,at,0xffffffffu);
  half(state.bytes,0x36,0xffffu);
  for(std::int32_t team=0;team<9;++team) {
    half(state.bytes,2+unsigned(team)*2,std::uint16_t(source_ctf_select_marker(markers,context,team,true)));
    std::int32_t second=-1;
    const auto first=source_ctf_select_marker(markers,context,team,false,&second);
    half(state.bytes,0x14+unsigned(team)*2,std::uint16_t(first));
    half(state.bytes,0x26+unsigned(team)*2,std::uint16_t(second));
    source_ctf_build_capture_regions(state,markers,context,team);
  }
  word(state.bytes,0x38,std::uint32_t(source_ctf_select_marker(markers,context,-1,true)));
  state.bytes[0]=1;
  for(unsigned at=0x114;at<=0x120;at+=4)word(state.bytes,at,0xffffffffu);
  half(state.bytes,0x124,0xffffu);
  for(unsigned at=0x14c;at<=0x16c;at+=4)word(state.bytes,at,0xffffffffu);
  for(unsigned at=0x17c;at<=0x19c;at+=4)word(state.bytes,at,0xffffffffu);
  return true;
}
} // namespace halo2
