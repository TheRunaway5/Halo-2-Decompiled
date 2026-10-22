#include "halo2/vista_widget_animation_descriptor.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
struct Owner {
  const VistaWidgetAnimationDescriptorServices& services;
  std::uint32_t read(std::uint32_t at, std::size_t bytes=4) const {
    if (!services.read) throw FormatError("Vista animation requires its retained source-address registry");
    return services.read(at,bytes);
  }
  std::int16_t signed_word(std::uint32_t at) const {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(at,2)));
  }
  std::uint32_t virtual_call(std::uint32_t widget,std::uint32_t slot) const {
    if (!services.invoke_virtual) throw FormatError("Vista animation requires its exact virtual method owner");
    return services.invoke_virtual(widget,slot);
  }
  std::uint32_t relocated(std::uint32_t offset) const {
    return offset==0xffffffffu ? 0u : read(0x882290u)+offset;
  }
  std::uint32_t tag(std::uint32_t datum) const {
    // 639623/639640/63965D/63967A deliberately use the low WORD only.
    const auto table=read(0x87cd50u);
    const auto offset=read(table+((datum&0xffffu)<<4)+8);
    const auto base=read(0x87cd54u);
    return offset+base;
  }
  std::uint32_t interface_globals() const {
    const auto scenario=read(0x879e74u);
    if (!scenario) return 0;
    const auto globals=read(0x879e70u);
    if (!globals || !read(globals+0x110)) return 0;
    const auto offset=read(globals+0x114);
    if (offset==0xffffffffu) return 0;
    const auto record=read(0x882290u)+offset;
    if (!record) return 0;
    std::uint32_t field{};
    switch (signed_word(scenario+0x10)) {
      case 0: field=0x8c;break;
      case 1: field=0x94;break;
      case 2: field=0x84;break;
      default:return 0;
    }
    const auto datum=read(record+field);
    return datum==0xffffffffu ? 0u : tag(datum);
  }
  std::uint32_t shared_globals() const {
    const auto interface=interface_globals();
    if (!interface) return 0;
    const auto datum=read(interface+4);
    return datum==0xffffffffu ? 0u : tag(datum);
  }
  std::uint32_t skin(std::uint32_t index) const {
    const auto globals=shared_globals();
    if (!globals) return 0;
    const auto ordinal=std::bit_cast<std::int16_t>(std::uint16_t(index));
    if (ordinal<0 || ordinal>=std::bit_cast<std::int32_t>(read(globals+0x138))) return 0;
    const auto records=relocated(read(globals+0x13c));
    const auto datum=read(records+std::uint32_t(ordinal)*8+4);
    return datum==0xffffffffu ? 0u : tag(datum);
  }
  std::uint32_t screen_tag(std::uint32_t screen_id) const {
    const auto interface=interface_globals();
    if (!interface) return 0xffffffffu;
    if (std::bit_cast<std::int32_t>(read(interface+8))<=0) return 0xffffffffu;
    std::uint32_t index{};
    do {
      const auto refs=relocated(read(interface+0xc));
      const auto datum=read(refs+index*8+4);
      if (datum!=0xffffffffu && std::int32_t(signed_word(tag(datum)+4))==std::bit_cast<std::int32_t>(screen_id))
        return read(refs+index*8+4);
      ++index;
    } while (std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(read(interface+8)));
    return 0xffffffffu;
  }
  std::uint32_t pane(std::uint32_t screen) const {
    if (read(screen+0x9f8,2)==0xffffu) return 0;
    const auto id=read(screen+0x70);
    const auto datum=screen_tag(id);
    if (datum==0xffffffffu) {
      if (read(0x863724u)!=id) {
        if (!services.write_dword) throw FormatError("Vista missing-screen logging requires its retained source bank");
        services.write_dword(0x863724u,id);
      }
      return 0;
    }
    const auto widget=tag(datum);
    if (!widget) return 0;
    const auto records=relocated(read(widget+0x24));
    const auto index=signed_word(screen+0x9f8);
    if (std::int32_t(index)>=std::bit_cast<std::int32_t>(read(widget+0x20))) return 0;
    return records+std::uint32_t(std::int32_t(index))*0x4cu;
  }
  std::uint32_t manager_focus(std::uint32_t argument,std::uint32_t kind,std::uint32_t channel) const {
    std::uint32_t base{};
    switch (kind) {
      case 0:base=channel==4 ? 0xd71e78u : 0u;break;
      case 1:base=0xd71bf8u+channel*60;break;
      case 2:base=channel==4 ? 0xd71ebcu : 0u;break;
      case 3:base=0xd71accu+channel*60;break;
      case 4:base=0xd71d24u+channel*68;break;
      case 5:base=0xd71978u+channel*68;break;
      case 6:base=0xd7190cu;break;
      default:break;
    }
    return read(base+argument*4+0x34);
  }
  bool focused(std::uint32_t widget,std::uint32_t argument) const {
    auto parent=read(widget+0x10);
    if (parent) {
      do {parent=read(parent+0x10);} while (parent);
    } else if (read(widget+4)!=0) return false;
    const auto channel=virtual_call(widget,0x38);
    const auto kind=virtual_call(widget,0x34);
    auto focus=manager_focus(argument,kind,channel);
    while (focus) {
      if (focus==widget) return true;
      focus=read(focus+0x10);
    }
    return false;
  }
  std::uint32_t list_animation(std::uint32_t list,std::uint32_t selector) const {
    const auto parent=read(list+0x10);
    std::uint32_t index{};
    if (parent) {
      const auto active=pane(parent);
      if (active && std::bit_cast<std::int32_t>(read(active+0xc))>0) {
        const auto records=relocated(read(active+0x10));
        if (records) index=read(records+4,2);
      }
    }
    // 613C51 does not check a missing skin before reading its +18 pointer.
    const auto frames=relocated(read(skin(index)+0x18));
    return frames+selector*16;
  }
};
void write(std::span<std::byte> d,std::size_t at,std::uint32_t value,std::size_t bytes=4) {
  for (std::size_t i=0;i<bytes;++i) d[at+i]=std::byte{std::uint8_t(value>>(i*8))};
}
std::int16_t descriptor_word(std::span<const std::byte> d,std::size_t at) {
  return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(d[at]) |
      (std::to_integer<unsigned>(d[at+1])<<8)));
}
} // namespace

std::uint8_t vista_widget_animation_descriptor(std::uint32_t widget,std::uint32_t animation_index,
    std::uint32_t reason,std::span<std::byte> descriptor,const VistaWidgetAnimationDescriptorServices& services) {
  const Owner owner{services};
  const auto globals=owner.shared_globals();
  if (!globals) return 0;
  auto root=owner.read(widget+0x10);
  if (root) {
    while (const auto parent=owner.read(root+0x10)) root=parent;
  } else if (owner.read(widget+4)==0) root=widget;
  if (descriptor.empty() || !root) return 0;
  if (descriptor.size()<0x34) throw FormatError("Vista animation descriptor is shorter than its source bank");
  const bool special=owner.read(root+0x70)==5;
  write(descriptor,0,reason);write(descriptor,4,0);write(descriptor,8,0,2);
  write(descriptor,0xa,0,2);write(descriptor,0xc,1,2);write(descriptor,0xe,0,2);write(descriptor,0x10,0);
  if (reason<=1) {
    write(descriptor,0xe,1,2);write(descriptor,0x10,owner.virtual_call(widget,0x1c));
  } else if (reason<=3) write(descriptor,0xe,2,2);
  else if (reason==4) write(descriptor,0xe,4,2);
  write(descriptor,0x2c,0);write(descriptor,0x28,0);write(descriptor,0x24,0);
  const auto unit=owner.read(0x79be18u);
  write(descriptor,0x14,0);write(descriptor,0x18,0);write(descriptor,0x1c,0);write(descriptor,0x20,0);write(descriptor,0x30,unit);
  if (special) {
    // 609EDE/63913F: actual manager produces two forward frames for any low WORD
    // other than NONE. Its constructor/lifetime remains an explicit bank owner.
    if (std::uint16_t(animation_index)==0xffffu) return 0;
    const auto duration=owner.read(0xd7190cu+0x64);
    write(descriptor,0xc,1,2);
    const auto now=owner.read(0xd71900u);
    const auto backward=descriptor_word(descriptor,0xc)<0;
    write(descriptor,0x1c,now);
    write(descriptor,0x20,duration);write(descriptor,8,2,2);write(descriptor,4,0xd7190cu+0x3c);
    write(descriptor,0xa,backward ? 1u : 0u,2);
    return 1;
  }
  if (owner.read(widget+4)==2 && reason==4) {
    const auto list=owner.read(widget+0x10);
    const auto selector=owner.focused(widget,0) ? 0u : owner.focused(widget,1) ? 3u : 2u;
    const auto record=owner.list_animation(list,selector);
    if (!record || !list) return 0;
    write(descriptor,4,owner.relocated(owner.read(record+0xc)));
    write(descriptor,8,owner.read(record+8,2),2);write(descriptor,0x20,owner.read(record+4));
    return 1;
  }
  const auto index=std::bit_cast<std::int16_t>(std::uint16_t(animation_index));
  if (index<0 || std::int32_t(index)>=std::bit_cast<std::int32_t>(owner.read(globals+0x120))) return 1;
  const auto records=owner.relocated(owner.read(globals+0x124));
  const auto record=records+std::uint32_t(index)*0x2c;
  if (!record) return 0;
  if (reason>4) return 1;
  const auto offset=(reason==1 || reason==3) ? 0x14u : reason==4 ? 0x24u : 8u;
  write(descriptor,4,owner.relocated(owner.read(record+offset+4)));
  const auto count=owner.read(record+offset,2);write(descriptor,8,count,2);
  if (reason==1 || reason==2) {
    const auto signed_count=std::bit_cast<std::int16_t>(std::uint16_t(count));
    write(descriptor,0xa,signed_count>0 ? std::uint32_t(signed_count)-1 : 0u,2);write(descriptor,0xc,0xffffu,2);
  } else write(descriptor,0xc,1,2);
  if (reason==4) {
    switch (owner.signed_word(record+0x20)) {
      case 0: case 3:write(descriptor,0x14,0);break;
      case 1:write(descriptor,0x14,3);break;
      case 2:write(descriptor,0x14,1);break;
      default:break;
    }
  }
  write(descriptor,0x20,owner.read(record+(reason==1 || reason==3 ? 0x10u : reason==4 ? 0x1cu : 4u)));
  return 1;
}
} // namespace halo2
