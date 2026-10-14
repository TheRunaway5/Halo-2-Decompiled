#include "halo2/item_point.hpp"
#include "halo2/first_person_ik.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
ItemPointDefinition Cache::load_item_point_definition(std::uint32_t definition) const {
  const Bytes data(cache_bytes());
  auto root=[&](std::uint32_t id,const char *group,std::size_t size) {
    const auto t=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
    if(!t)throw FormatError("Missing original item-point definition/model");
    return metadata_offset(t->address,size);
  };
  ItemPointDefinition d;d.definition=definition;
  const auto object=root(definition,"weap",0x3c);d.model=data.u32(object+0x38);
  const auto model=root(d.model,"hlmt",8);d.render_model=data.u32(model+4);
  const auto render=root(d.render_model,"mode",0x60);const auto count=data.u32(render+0x58);
  if(count>65536)throw FormatError("Excessive item-point marker groups");
  if(!count)return d;
  const auto groups=metadata_offset(data.u32(render+0x5c),std::size_t(count)*12);
  for(unsigned n=0;n<count;++n) {
    const auto g=groups+n*12;if(data.u32(g)!=0x0c0000c1)continue;
    d.group=std::int32_t(n);const auto entries=data.u32(g+4);
    if(entries>65536)throw FormatError("Excessive ground-point markers");
    if(entries) {
      const auto start=metadata_offset(data.u32(g+8),std::size_t(entries)*36);
      d.markers.resize(entries);
      for(unsigned i=0;i<entries;++i) {
        const auto raw=data.slice(start+i*36,36);std::copy(raw.begin(),raw.end(),d.markers[i].begin());
      }
    }
    break; // Original 1DC8F0 first matching group.
  }
  return d;
}
ItemPointResult source_item_point(const ItemPointDefinition &d,const ItemPointState &item,const RawCollisionBsp &collision) {
  if(item.parent!=0xffffffffu)throw FormatError("Parented item-point producer is unrecovered");
  ItemPointResult result;result.point=item.root;
  if(!(item.item_flags&0x20))return result;
  if(item.flags&1)throw FormatError("Item marker owner redirection is unrecovered");
  if(item.flags&0x400)throw FormatError("Mirrored ground-point branch lacks executed coverage");
  if(item.definition!=d.definition)throw FormatError("Item-point definition does not match current object");
  if(d.group==-1 || d.markers.empty())return result; // Listed, unexecuted zero-marker root path.
  const auto &raw=d.markers.front();const Bytes marker(raw);
  if(raw[0]!=std::byte{0xff})throw FormatError("Ground marker runtime region filter is unrecovered");
  ItemPointMarker m;m.node=std::int16_t(std::to_integer<unsigned>(raw[2]));
  if(unsigned(m.node)>=item.current_nodes.size())throw FormatError("Ground marker has no source current node");
  m.local=source_marker_transform({marker.f32(4),marker.f32(8),marker.f32(12)},
      {marker.f32(16),marker.f32(20),marker.f32(24),marker.f32(28)});
  m.world=compose_transform(item.current_nodes[unsigned(m.node)],m.local);m.scalar=marker.f32(32);
  if(item.contact_instance!=-1)throw FormatError("Instanced item-point collision producer is unrecovered");
  if(item.contact_surface<0 || unsigned(item.contact_surface)>=collision.surfaces.size())throw FormatError("Item contact surface outside active source collision bank");
  m.signed_plane=collision.surfaces[unsigned(item.contact_surface)].plane;
  const unsigned index=std::uint16_t(m.signed_plane)&0x7fff;
  if(index>=collision.planes.size())throw FormatError("Item contact plane outside source collision bank");
  m.plane=collision.planes[index];
  if(m.signed_plane<0) {
    m.plane.normal={0.f-m.plane.normal.x,0.f-m.plane.normal.y,0.f-m.plane.normal.z};m.plane.distance=0.f-m.plane.distance;
  }
  const auto p=m.world.position,n=m.plane.normal;
  const float z=p.z*n.z,y=p.y*n.y,x=n.x*p.x;
  const float zy=z+y,sum=zy+x;m.signed_distance=sum-m.plane.distance;
  m.shift=std::bit_cast<float>(0x3d4ccccdu)-m.signed_distance;
  const float dx=n.x*m.shift,dy=n.y*m.shift,dz=n.z*m.shift;
  result.point={dx+p.x,dy+p.y,dz+p.z};result.returned=1;result.marker=m;return result;
}
} // namespace halo2
