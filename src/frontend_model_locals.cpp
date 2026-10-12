#include "halo2/frontend_model_locals.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
float grip_norm(std::array<std::int16_t,4> quantized) {
  const float x=quantized[0],y=quantized[1],z=quantized[2],w=quantized[3];
  const float a=x*x+z*z,b=y*y+w*w;
  return a+b;
}
bool admitted(std::span<const std::uint32_t>mask,std::size_t n,std::size_t count) {
  if(mask.empty())return true;
  if(mask.size()<(count+31)/32)throw FormatError("frontend local destination mask too short");
  return mask[n/32]&(1u<<(n%32));
}
}
std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4> input,
                                          float inverse_sqrt_seed) {
  if(!(grip_norm(input)>0) || !std::isfinite(inverse_sqrt_seed) || inverse_sqrt_seed<=0)
    throw FormatError("invalid original grip quaternion seed");
  std::array<float,4> out;
  for(unsigned i=0;i<out.size();++i)out[i]=float(input[i])*inverse_sqrt_seed;
  return out;
}
std::array<float,4> frontend_grip_rotation(std::array<std::int16_t,4> input) {
  const float norm=grip_norm(input);
  if(!(norm>0))throw FormatError("zero original grip quaternion");
  return frontend_grip_rotation(input,1.f/std::sqrt(norm));
}
std::vector<BonePose> frontend_model_locals(const RenderModel&model,
    const AnimationClip&clip,float frame,std::span<const std::uint32_t>mask,
    const FrontendModelPrimary*source,bool interpolate) {
  if(!clip.frame_count || !std::isfinite(frame) || frame<0 || frame>=float(clip.frame_count) ||
     clip.channels.size()!=clip.nodes.size() || clip.components.size()!=clip.nodes.size())
    throw FormatError("invalid retained frontend primary frame");
  if(source) {
    if(source->source_graph!=clip.source_graph || source->source_index!=clip.source_index ||
        source->frame_count!=clip.frame_count || source->nodes!=clip.nodes || source->components!=clip.components)
      throw FormatError("literal frontend primary does not match selected clip");
    return sample_frontend_model_primary(model,*source,frame,mask,interpolate);
  }
  if(!interpolate)frame=std::floor(frame);
  std::vector<BonePose> out;
  out.reserve(model.bones.size());
  for(std::size_t i=0;i<model.bones.size();++i) {
    const auto&bone=model.bones[i];
    auto pose=bone.bind;
    const auto found=std::find(clip.nodes.begin(),clip.nodes.end(),bone.name);
    if(found!=clip.nodes.end() && admitted(mask,i,model.bones.size())) {
      const auto n=std::size_t(found-clip.nodes.begin());
      for(unsigned c=0;c<3;++c)if(clip.components[n]&(1u<<c)) {
        const auto&track=clip.channels[n][c];
        if(track.times.empty() || track.times.size()!=track.keys.size())
          throw FormatError("missing frontend primary channel");
        const auto upper=std::upper_bound(track.times.begin(),track.times.end(),frame);
        const auto k=upper==track.times.begin()?0u:unsigned(upper-track.times.begin()-1);
        const auto next=std::min(k+1,unsigned(track.times.size()-1));
        const float t=frame<=float(track.times[k]) || k==next?0.f:
            std::clamp((frame-float(track.times[k]))/float(track.times[next]-track.times[k]),0.f,1.f);
        std::array<float,4> value{};
        for(unsigned j=0;j<value.size();++j)
          value[j]=track.keys[k][j]*(1.f-t)+track.keys[next][j]*t;
        if(c==0)pose.rotation=value;
        else if(c==1)pose.translation={value[0],value[1],value[2]};
        else pose.scale=value[0];
      }
    }
    out.push_back(pose);
  }
  return out;
}
void apply_frontend_model_grip(const RenderModel&model,std::span<BonePose> locals,
                               const FrontendModelGrip&grip,std::span<const std::uint32_t>mask) {
  if(locals.size()!=model.bones.size() || grip.nodes.size()!=grip.rotations.size())
    throw FormatError("frontend grip node bank differs from source");
  for(std::size_t n=0;n<grip.nodes.size();++n)if(grip.rotations[n]) {
    const auto found=std::find_if(model.bones.begin(),model.bones.end(),
        [&](const auto&bone){return bone.name==grip.nodes[n];});
    if(found==model.bones.end())throw FormatError("frontend grip node absent from model");
    const auto index=std::size_t(found-model.bones.begin());
    if(admitted(mask,index,model.bones.size()))locals[index].rotation=frontend_grip_rotation(*grip.rotations[n]);
  }
}
FrontendModelGrip Cache::load_frontend_model_grip_animation(std::uint32_t graph,unsigned index)const {
  const Bytes data(storage_);
  const auto tag=query_tag({.handle=TagHandle{graph}, .group=tag_groups::model_animation_graph, .require_address=true});
  if(!tag)throw FormatError("frontend grip graph missing");
  const auto root=metadata_offset(tag->address,0xbc);
  const auto node_count=data.u32(root+0xc),clip_count=data.u32(root+0x2c);
  if(!node_count || node_count>255 || index>=clip_count || clip_count>65536)
    throw FormatError("frontend grip source range invalid");
  const auto nodes=metadata_offset(data.u32(root+0x10),node_count*32);
  const auto clip=metadata_offset(data.u32(root+0x30),clip_count*0x6c)+index*0x6c;
  const auto byte=[&](std::size_t at){return std::to_integer<unsigned>(data.slice(at,1)[0]);};
  const auto mask_size=((node_count+31)/32)*4;
  if(byte(clip+0x10)!=2 || byte(clip+0x13)!=node_count || data.u16(clip+0x14)!=1 ||
     byte(clip+0x3c)!=0 || byte(clip+0x3d)!=mask_size*3 || data.u16(clip+0x3e)!=0 ||
     data.u16(clip+0x40)!=0 || data.u16(clip+0x42)!=0 || data.u32(clip+0x44)!=0 ||
     data.u32(clip+0x64)!=0)
    throw FormatError("frontend grip requires proven single-frame rotation replacement");
  const auto group=data.u32(clip+0x24),group_count=data.u32(root+0xac);
  if(group>=group_count || group_count>65536)throw FormatError("frontend grip resource group invalid");
  const auto groups=metadata_offset(data.u32(root+0xb0),group_count*20);
  const auto raw=raw_data(data.u32(groups+group*20+8),data.u32(groups+group*20+4));
  const Bytes resource(raw);const auto offset=data.u32(clip+0x28),size=data.u32(clip+0x48);
  resource.require(offset,std::size_t(size)+mask_size*3);
  const auto codec=resource.slice(offset,size);const Bytes c(codec);
  const auto codec_byte=[&](std::size_t at){return std::to_integer<unsigned>(c.slice(at,1)[0]);};
  c.require(0,32);
  if(codec_byte(0)!=3 || codec_byte(2)!=0 || codec_byte(3)!=0 || c.u32(20)!=8)
    throw FormatError("frontend grip codec is outside frozen source path");
  FrontendModelGrip out;out.source_graph=graph;out.source_index=index;
  out.nodes.resize(node_count);out.rotations.resize(node_count);unsigned ordinal=0;
  for(unsigned n=0;n<node_count;++n) {
    out.nodes[n]=data.u32(nodes+n*32);
    if(resource.u32(offset+size+(n/32)*4)&(1u<<(n%32))) {
      std::array<std::int16_t,4> input;
      for(unsigned k=0;k<4;++k)input[k]=std::int16_t(c.u16(32+ordinal*8+k*2));
      out.rotations[n]=input;++ordinal;
    }
  }
  if(ordinal!=codec_byte(1))throw FormatError("frontend grip codec mask count differs");
  for(unsigned n=0;n<mask_size*2;n+=4)
    if(resource.u32(offset+size+mask_size+n))throw FormatError("frontend grip has unresolved translation/scale channels");
  return out;
}
std::optional<FrontendModelGrip> Cache::load_frontend_model_grip(std::uint32_t unit)const {
  const auto weapon=load_frontend_model_weapon(unit);if(!weapon)return {};
  const auto model=load_model(unit,false);
  if(model.animation==null_datum)return {};
  auto identifiers=load_weapon_animation_identifiers(*weapon);
  const auto tag=query_tag({.handle=TagHandle{model.animation}, .group=tag_groups::model_animation_graph, .require_address=true});
  if(!tag)throw FormatError("frontend grip graph definition missing");
  const Bytes data(storage_);const auto root=metadata_offset(tag->address,0x5c);
  const auto aliases=data.u32(root+0x54);
  if(aliases>65536)throw FormatError("frontend grip class alias count invalid");
  const auto alias=aliases?metadata_offset(data.u32(root+0x58),aliases*8):0;
  for(unsigned i=0;i<aliases;++i)if(data.u32(alias+i*8)==identifiers[1]) {
    const auto replacement=data.u32(alias+i*8+4);
    if(replacement!=null_datum && replacement!=0x030000d9u && identifiers[0]!=0x0400054bu)
      identifiers[0]=replacement;
    break;
  }
  // Equipping the authored initial weapon constructs this combat overlay.
  // A subsequent UI primary request retains the observed43/99 grip channel.

  const auto find=[&](TagBlockRange b,std::size_t stride,std::uint32_t name)->std::optional<std::size_t> {
    for(unsigned i=0;i<b.count;++i)if(data.u32(b.offset+i*stride)==name)return b.offset+i*stride;
    return {};
  };
  constexpr std::uint32_t combat=0x06000086u,any=0x030000d9u,grip=0x0400060bu;
  const auto modes=metadata_block(root+0x34, 20);
  // 1CBAD0 searches the complete overlay fallback order first; only a NONE
  // result starts the ordinary action search. Invalid metadata still fails.
  for(const unsigned table:{12u,4u})for(unsigned step=0;step<8;++step) {
    if(((step&2)&&identifiers[0]==any)||((step&1)&&identifiers[1]==any))continue;
    const auto m=find(modes,20,step&4?any:combat);if(!m)continue;
    const auto c=find(metadata_block(*m+4, 20),20,step&2?any:identifiers[0]);if(!c)continue;
    const auto t=find(metadata_block(*c+4, 0x34),0x34,step&1?any:identifiers[1]);if(!t)continue;
    const auto a=find(metadata_block(*t+table, 8),8,grip);if(!a)continue;
    const auto index=std::int16_t(data.u16(*a+6));if(index==-1)continue;
    if(index<0)throw FormatError("frontend grip animation index invalid");
    auto selected_graph=model.animation;
    const auto inherited=std::int16_t(data.u16(*a+4));
    if(inherited!=-1) {
      const auto parents=metadata_block(root+0x4c, 32);
      if(inherited<0 || unsigned(inherited)>=parents.count)
        throw FormatError("frontend grip inheritance index invalid");
      selected_graph=data.u32(parents.offset+unsigned(inherited)*32+4);
    }
    return load_frontend_model_grip_animation(selected_graph,unsigned(index));
  }
  return {};
}
std::array<std::uint32_t,8> Cache::load_frontend_model_deferred_mask(std::uint32_t unit)const {
  const Bytes data(storage_);
  const auto tag=query_tag({.handle=TagHandle{unit}, .group=tag_groups::biped, .require_address=true});
  if(!tag)throw FormatError("frontend deferred-node unit missing");
  const auto definition=metadata_offset(tag->address,0x3c);
  const auto model_id=data.u32(definition+0x38);
  const auto model=query_tag({.handle=TagHandle{model_id}, .group=tag_groups::model, .require_address=true});
  if(!model)throw FormatError("frontend deferred-node model missing");
  const auto root=metadata_offset(model->address,0xc4);
  std::array<std::uint32_t,8> mask;
  for(unsigned i=0;i<mask.size();++i)mask[i]=data.u32(root+0xa4+i*4);
  return mask;
}
}
