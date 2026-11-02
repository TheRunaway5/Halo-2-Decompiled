#pragma once
#include "halo2/renderer.hpp"
#include "halo2/world_material_uniform.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace halo2 {
// Private finite-reference diagnostic only. The public observer supplies actual
// vkCmdDraw bindings; the existing upload-plan dump identifies this primary body
// without guessing an object index or confusing its secondary WORLD occurrence.
class FirstPersonBodyDrawDiagnostic {
  using Key=std::array<std::uint32_t,13>;
  std::filesystem::path stem_;
  unsigned point_{};
  std::vector<Vertex> expected_,uploaded_;
  Camera camera_;
  std::array<ShaderVector,7> world_;
  std::map<Key,unsigned> primary_;
  std::map<Key,unsigned> covered_;
  std::set<std::array<std::uint64_t,3>> ranges_;
  std::set<std::uint32_t> materials_;
  bool loaded_{};
  static Key key(const Vertex& v) {
    return {std::bit_cast<std::uint32_t>(v.position.x),std::bit_cast<std::uint32_t>(v.position.y),
      std::bit_cast<std::uint32_t>(v.position.z),std::bit_cast<std::uint32_t>(v.normal.x),
      std::bit_cast<std::uint32_t>(v.normal.y),std::bit_cast<std::uint32_t>(v.normal.z),
      std::bit_cast<std::uint32_t>(v.uv.x),std::bit_cast<std::uint32_t>(v.uv.y),v.source_section,v.source_part,
      std::bit_cast<std::uint32_t>(v.source_local_position.x),std::bit_cast<std::uint32_t>(v.source_local_position.y),
      std::bit_cast<std::uint32_t>(v.source_local_position.z)};
  }
  static void hex(std::ostream& out,std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";out<<'"';
    for(const auto b:bytes){const auto n=std::to_integer<unsigned>(b);out<<digits[n>>4]<<digits[n&15];}out<<'"';
  }
  template<class T>static void hex(std::ostream& out,const T& value) {
    hex(out,std::as_bytes(std::span(&value,1)));
  }
  template<class T>static void numbers(std::ostream& out,const T& values) {
    out<<'[';bool comma{};for(const auto& value:values){if(comma)out<<',';comma=true;out<<value;}out<<']';
  }
  void load() {
    if(loaded_)return;loaded_=true;
    std::filesystem::path latest;
    // The isolated fixture owns a fresh stem and at most2400 submissions.
    for(unsigned n=0;n<2400;++n){const auto path=stem_.string()+"-"+std::to_string(n)+".vertices.bin";
      if(!std::filesystem::is_regular_file(path))break;latest=path;}
    if(latest.empty())throw FormatError("Primary body draw diagnostic has no current upload-plan dump");
    const auto size=std::filesystem::file_size(latest);
    if(!size || size%sizeof(Vertex) || size>64u*1024u*1024u)
      throw FormatError("Primary body draw diagnostic upload-plan dump has an invalid extent");
    uploaded_.resize(size/sizeof(Vertex));std::ifstream input(latest,std::ios::binary);
    input.read(reinterpret_cast<char*>(uploaded_.data()),std::streamsize(size));
    if(!input || input.peek()!=EOF)throw FormatError("Primary body draw diagnostic could not read its completed upload plan");
  }
public:
  FirstPersonBodyDrawDiagnostic(std::filesystem::path stem,unsigned point,std::span<const Vertex> primary,
      const Camera& camera,const std::array<ShaderVector,7>& world)
      :stem_(std::move(stem)),point_(point),expected_(primary.begin(),primary.end()),camera_(camera),world_(world) {
    if(expected_.empty())throw FormatError("Primary body draw diagnostic requires its actual submitted body");
    for(const auto& v:expected_){++primary_[key(v)];materials_.insert(v.material);}
  }
  void observe(const RendererObservedGeometryDraw& draw) {
    load();if(draw.first>uploaded_.size() || draw.count>uploaded_.size()-draw.first)return; // Unrelated resident bank.
    std::map<Key,unsigned> found;unsigned matches{};
    for(const auto& v:std::span(uploaded_).subspan(draw.first,draw.count))if(primary_.contains(key(v))){++matches;++found[key(v)];}
    if(!matches)return;
    if(materials_.contains(draw.parent_material) && ranges_.insert({draw.vertex_buffer,draw.first,draw.count}).second)
      for(const auto& [k,n]:found)covered_[k]=std::min(primary_.at(k),covered_[k]+n);
    unsigned covered{};for(const auto& [k,n]:covered_)covered+=n;
    if(!draw.pipeline || !draw.descriptor || draw.camera_push.size()!=112 ||
       draw.material_uniform.size()!=sizeof(WorldMaterialUniform))
      throw FormatError("Primary body draw diagnostic lost its actual recorded bindings");
    const auto path=stem_.string()+"-body-"+std::to_string(point_)+"-"+std::to_string(draw.serial)+".json";
    std::ofstream out(path);out.precision(9);
    out<<"{\"point\":"<<point_<<",\"serial\":"<<draw.serial<<",\"first\":"<<draw.first<<",\"count\":"<<draw.count
      <<",\"primary_matches\":"<<matches<<",\"primary_expected\":"<<expected_.size()
      <<",\"primary_covered_occurrences\":"<<covered<<",\"vertex_stride\":"<<sizeof(Vertex)
      <<",\"body_material_match\":"<<(materials_.contains(draw.parent_material)?"true":"false")
      <<",\"material_slot\":"<<draw.material_slot<<",\"parent_material\":"<<draw.parent_material
      <<",\"owner_constants\":"<<(draw.owner_constants?"true":"false")<<",\"object_index\":"<<draw.object_index
      <<",\"pipeline_handle\":"<<draw.pipeline_handle<<",\"vertex_buffer\":"<<draw.vertex_buffer
      <<",\"descriptor_handle\":"<<draw.descriptor->handle<<",\"dynamic_offset\":"<<draw.dynamic_offset<<",\"source_material\":";
    numbers(out,draw.source_material);out<<",\"viewport\":";numbers(out,draw.viewport);out<<",\"scissor\":";numbers(out,draw.scissor);
    out<<",\"camera_push_hex\":";hex(out,draw.camera_push);out<<",\"material_uniform_hex\":";hex(out,draw.material_uniform);
    out<<",\"vertex_bank_uniform_hex\":";hex(out,draw.vertex_bank_uniform);
    out<<",\"expected_world_camera_hex\":";hex(out,world_);out<<",\"expected_eye_hex\":";hex(out,camera_.position);
    out<<",\"descriptor_buffers\":[";numbers(out,draw.descriptor->buffers[0]);out<<',';numbers(out,draw.descriptor->buffers[1]);
    out<<"],\"pipeline_raster\":";numbers(out,draw.pipeline->raster);out<<",\"pipeline_depth\":";numbers(out,draw.pipeline->depth);
    out<<",\"pipeline_vertex_attributes\":[";bool comma{};
    for(const auto& a:draw.pipeline->vertex_attributes){if(comma)out<<',';comma=true;numbers(out,a);}
    out<<"],\"shaders\":[";comma=false;
    for(const auto& shader:draw.pipeline->shaders){
      if(comma)out<<',';comma=true;
      // Modules are retired after pipeline creation and their handles may be
      // recycled. A recorded draw owns these copied words, not that handle's
      // later contents. Give each draw/stage its own exact dump.
      const auto file=stem_.filename().string()+"-shader-"+std::to_string(point_)+"-"+
          std::to_string(draw.serial)+"-"+std::to_string(shader.stage)+".spv";
      const auto target=stem_.parent_path()/file;
      std::ofstream spv(target,std::ios::binary);
      spv.write(reinterpret_cast<const char*>(shader.words.data()),std::streamsize(shader.words.size()*4));
      spv.flush();
      if(!spv)throw FormatError("Could not write this recorded draw's actual shader words");
      out<<"{\"stage\":"<<shader.stage<<",\"module\":"<<shader.module<<",\"file\":\""<<file<<"\",\"specialization_entries\":[";
      bool sep{};for(const auto& entry:shader.specialization_entries){if(sep)out<<',';sep=true;numbers(out,entry);}
      out<<"],\"specialization_hex\":";hex(out,std::span(shader.specialization_bytes));out<<'}';
    }
    out<<"]}\n";if(!out)throw FormatError("Could not write bounded primary body draw diagnostic");
  }
};
} // namespace halo2
