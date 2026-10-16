#include "halo2/frontend_model_primary.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
std::array<float,4> normalized_integer_blend(std::array<std::int16_t,4>a,
    std::array<std::int16_t,4>b,float wa,float wb) {
  std::array<float,4> out;
  for(unsigned j=0;j<4;++j)out[j]=float(a[j])*wa+float(b[j])*wb;
  const float x=out[0]*out[0]+out[2]*out[2];
  const float y=out[1]*out[1]+out[3]*out[3];
  const float norm=x+y;
  if(!(norm>0))throw FormatError("zero source primary quaternion");
  const float seed=1.f/std::sqrt(norm);
  for(auto&v:out)v*=seed;
  return out;
}
}
namespace {
std::vector<BonePose> sample_primary(const RenderModel&model,
    const FrontendModelPrimary&clip,float frame,std::span<const std::uint32_t>mask,bool interpolate,
    bool model_node_order) {
  if(!clip.frame_count || !std::isfinite(frame) || frame<0 || frame>=float(clip.frame_count) ||
      clip.nodes.size()!=clip.channels.size() || clip.nodes.size()!=clip.components.size())
    throw FormatError("invalid retained source frontend primary frame");
  if(model_node_order && model.bones.size()>clip.nodes.size())
    throw FormatError("World object primary does not cover its model node bank");
  const auto frame_index=unsigned(frame);
  float fraction=interpolate?frame-float(frame_index):0.f;
  unsigned index=frame_index;
  if(index==clip.frame_count-1 || fraction<.0001f)fraction=0;
  else if(fraction>.9999f){++index;fraction=0;}
  const float sample=float(index)+fraction;
  std::vector<BonePose> out;out.reserve(model.bones.size());
  for(unsigned n=0;n<model.bones.size();++n) {
    auto pose=model.bones[n].bind;
    if(!mask.empty()) {
      if(mask.size()<(model.bones.size()+31)/32)throw FormatError("source primary mask truncated");
      if(!(mask[n/32]&(1u<<(n%32)))){out.push_back(pose);continue;}
    }
    auto node=std::size_t(n);
    if(!model_node_order) {
      const auto found=std::find(clip.nodes.begin(),clip.nodes.end(),model.bones[n].name);
      if(found==clip.nodes.end()){out.push_back(pose);continue;}
      node=std::size_t(found-clip.nodes.begin());
    }
    for(unsigned c=0;c<3;++c)if(clip.components[node]&(1u<<c)) {
      const auto&track=clip.channels[node][c];
      const auto upper=std::upper_bound(track.times.begin(),track.times.end(),index);
      const auto k=upper==track.times.begin()?0u:unsigned(upper-track.times.begin()-1);
      const auto next=std::min(k+1,unsigned(track.times.size()-1));
      const bool blend=k!=next && (fraction!=0 || index!=track.times[k]);
      float wa=1,wb=0;
      if(blend){wa=float(track.times[next])-sample;wb=sample-float(track.times[k]);}
      if(c==0)pose.rotation=normalized_integer_blend(track.rotations.at(k),track.rotations.at(next),wa,wb);
      else {
        const auto&a=track.values.at(k);const auto&b=track.values.at(next);
        float weight=0;
        if(blend) {
          const float seed=1.f/float(track.times[next]-track.times[k]);
          weight=seed*(sample-float(track.times[k]));
        }
        std::array<float,4> value{};
        for(unsigned j=0;j<(c==1?3u:1u);++j)value[j]=blend?(b[j]-a[j])*weight+a[j]:a[j];
        if(c==1)pose.translation={value[0],value[1],value[2]};else pose.scale=value[0];
      }
    }
    out.push_back(pose);
  }
  return out;
}
}
std::vector<BonePose> sample_frontend_model_primary(const RenderModel&model,
    const FrontendModelPrimary&clip,float frame,std::span<const std::uint32_t>mask,bool interpolate) {
  return sample_primary(model,clip,frame,mask,interpolate,false);
}
std::vector<BonePose> sample_world_object_primary(const RenderModel&model,
    const FrontendModelPrimary&clip,float frame,std::span<const std::uint32_t>mask,bool interpolate) {
  return sample_primary(model,clip,frame,mask,interpolate,true);
}
FrontendModelPrimary Cache::load_frontend_model_primary_animation(std::uint32_t graph,unsigned index)const {
  const Bytes data(cache_bytes());
  const auto tag=query_tag({.handle=TagHandle{graph}, .group=tag_groups::model_animation_graph, .require_address=true});
  if(!tag)throw FormatError("source primary graph missing");
  const auto root=metadata_offset(tag->address,0xbc);
  const auto count=data.u32(root+0xc),clips=data.u32(root+0x2c);
  if(!count || count>255 || index>=clips || clips>65536)throw FormatError("source primary range invalid");
  const auto nodes=metadata_offset(data.u32(root+0x10),count*32);
  const auto at=metadata_offset(data.u32(root+0x30),clips*0x6c)+index*0x6c;
  const auto byte=[&](std::size_t p){return std::to_integer<unsigned>(data.slice(p,1)[0]);};
  const auto mask_size=((count+31)/32)*4;
  const auto sf=byte(at+0x3c),af=byte(at+0x3d),as=data.u32(at+0x48);
  const auto ss=data.u16(at+0x42);
  if(byte(at+0x10)!=0 || byte(at+0x13)!=count || !data.u16(at+0x14) ||
      sf!=mask_size*3 || af!=mask_size*3 || data.u16(at+0x3e) || data.u16(at+0x40) ||
      data.u32(at+0x44) || data.u32(at+0x64))
    throw FormatError("source primary is outside proven replacement codec path");
  const auto group=data.u32(at+0x24),groups=data.u32(root+0xac);
  if(group>=groups || groups>65536)throw FormatError("source primary resource group invalid");
  const auto table=metadata_offset(data.u32(root+0xb0),groups*20);
  const auto resource=raw_data(data.u32(table+group*20+8),data.u32(table+group*20+4));
  const Bytes raw(resource);const auto start=data.u32(at+0x28);
  raw.require(start,std::size_t(ss)+as+sf+af);
  FrontendModelPrimary out;out.source_graph=graph;out.source_index=index;out.frame_count=data.u16(at+0x14);
  out.nodes.resize(count);out.components.resize(count);out.channels.resize(count);
  for(unsigned n=0;n<count;++n)out.nodes[n]=data.u32(nodes+n*32);
  for(unsigned section=0;section<2;++section) {
    const unsigned size=section?as:ss;if(!size)continue;
    const Bytes codec(raw.slice(std::size_t(start)+(section?ss:0),size));codec.require(0,32);
    const auto b=[&](std::size_t p){return std::to_integer<unsigned>(codec.slice(p,1)[0]);};
    const auto id=b(0);
    if((!section && id!=1) || (section && id!=3 && (id<4 || id>7)))
      throw FormatError("source primary codec is not proven");
    std::array<unsigned,3> ordinals{};
    for(unsigned n=0;n<count;++n)for(unsigned c=0;c<3;++c) {
      const auto mask_at=std::size_t(start)+ss+as+(section?sf:0)+c*mask_size+(n/32)*4;
      if(!(raw.u32(mask_at)&(1u<<(n%32))))continue;
      const auto ordinal=ordinals[c]++;if(ordinal>=b(c+1))throw FormatError("source primary mask exceeds codec count");
      auto&track=out.channels[n][c];track={};track.codec=id;out.components[n]|=1u<<c;
      const unsigned element=c==0?8:c==1?12:4;
      unsigned key_count=section?out.frame_count:1,first=0,width=0,frame_base=0,key_base=0,stride=element;
      if(section && id>=4) {
        codec.require(0,48);const auto descriptor=codec.u32((c?codec.u32(8+c*4):48)+ordinal*4);
        first=descriptor>>12;key_count=descriptor&0xfff;width=(id==5 || id==7)?2:1;
        frame_base=codec.u32(20+c*4);key_base=codec.u32(32+c*4);
        if(!key_count)throw FormatError("source primary sparse channel empty");
      } else {key_base=c?codec.u32(8+c*4):32;stride=section?codec.u32(20+c*4):element;}
      for(unsigned k=0;k<key_count;++k) {
        unsigned time=section?k:0;std::size_t key_at;
        if(width) {
          const auto at=std::size_t(frame_base)+(first+k)*width;
          time=width==1?b(at):codec.u16(at);key_at=std::size_t(key_base)+(first+k)*element;
        } else key_at=std::size_t(key_base)+ordinal*stride+(section?k*element:0);
        if((k && time<=track.times.back()) || time>out.frame_count)throw FormatError("source primary key times invalid");
        track.times.push_back(time);
        if(c==0) {
          std::array<std::int16_t,4> packed;for(unsigned j=0;j<4;++j)packed[j]=std::int16_t(codec.u16(key_at+j*2));
          track.rotations.push_back(packed);
        } else {
          std::array<float,4> value{};for(unsigned j=0;j<(c==1?3u:1u);++j)value[j]=codec.f32(key_at+j*4);
          track.values.push_back(value);
        }
      }
    }
    for(unsigned c=0;c<3;++c)if(ordinals[c]!=b(c+1))throw FormatError("source primary codec/mask count differs");
  }
  return out;
}
}
