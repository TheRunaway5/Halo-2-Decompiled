#include "halo2/vista_widget_resource_layout.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
struct Owner {
  const VistaWidgetResourceLayoutServices& s;
  std::uint32_t r(std::uint32_t at,std::size_t n=4)const {
    if(!s.read)throw FormatError("Vista resource layout requires its source32 registry");
    return s.read(at,n);
  }
  void w(std::uint32_t at,std::uint32_t value,std::size_t n=4)const {
    if(!s.write)throw FormatError("Vista resource layout requires its mutable source32 registry");
    s.write(at,value,n);
  }
  std::uint32_t tag(std::uint32_t datum)const {
    const auto table=r(0x87cd50u);const auto offset=r(table+(datum&0xffffu)*16u+8u);
    return offset+r(0x87cd54u);
  }
  std::uint32_t interface_globals()const {
    const auto scenario=r(0x879e74u);if(!scenario)return 0;
    const auto globals=r(0x879e70u);if(!globals || !r(globals+0x110))return 0;
    const auto offset=r(globals+0x114);if(offset==0xffffffffu)return 0;
    const auto record=r(0x882290u)+offset;if(!record)return 0;
    std::uint32_t field{};
    switch(std::bit_cast<std::int16_t>(std::uint16_t(r(scenario+0x10,2)))) {
      case 0:field=0x8c;break;case 1:field=0x94;break;case 2:field=0x84;break;
      default:return 0;
    }
    const auto datum=r(record+field);return datum==0xffffffffu?0u:tag(datum);
  }
  std::uint32_t screen_tag(std::uint32_t id)const {
    const auto interface=interface_globals();if(!interface)return 0xffffffffu;
    if(std::bit_cast<std::int32_t>(r(interface+8))<=0)return 0xffffffffu;
    std::uint32_t i{};
    do {
      const auto offset=r(interface+0xc);
      const auto records=offset==0xffffffffu?0u:r(0x882290u)+offset;
      const auto datum=r(records+i*8+4);
      if(datum!=0xffffffffu && std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(tag(datum)+4,2)))))==id)
        return r(records+i*8+4);
      ++i;
    }while(std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(r(interface+8)));
    return 0xffffffffu;
  }
  std::uint32_t logged_screen_tag(std::uint32_t id)const {
    const auto datum=screen_tag(id);
    if(datum==0xffffffffu && r(0x863724u)!=id)w(0x863724u,id);
    return datum;
  }
  std::uint32_t cursor_resource(std::uint32_t datum)const {
    //781C57 differs from the general low-WORD tag leaf: zero is also NULL.
    return datum==0 || datum==0xffffffffu?0u:tag(datum);
  }
};
}
void vista_widget_resource_layout(std::uint32_t widget,
    const VistaWidgetResourceLayoutServices& s) {
  const Owner o{s};const auto id=o.r(widget+0x70);
  o.w(widget+0x9fa,1,1);
  const auto datum=o.logged_screen_tag(id);
  if(datum!=0xffffffffu) {
    const auto screen=o.tag(datum);
    if(screen) {
      std::uint32_t i{};
      if(std::bit_cast<std::int32_t>(o.r(screen+0x38))>0)do {
        const auto offset=o.r(screen+0x3c);
        const auto records=offset==0xffffffffu?0u:o.r(0x882290u)+offset;
        const auto record=records+i*8;
        if(record) {
          const auto bitmap=o.r(record+4);
          if(bitmap!=0xffffffffu) {
            if(!s.request_bitmap)throw FormatError("Vista resource layout requires actual61D228 bitmap request");
            s.request_bitmap(bitmap);
          }
        }
        ++i;
      }while(std::bit_cast<std::int32_t>(i)<std::bit_cast<std::int32_t>(o.r(screen+0x38)));
      const auto resource=o.cursor_resource(o.r(screen+0x6c));
      const auto manager=o.r(0x87a5c4u);
      if(manager) {
        if(!s.apply_cursor_resource)throw FormatError("Vista resource layout requires actual48AA97 cursor resource owner");
        s.apply_cursor_resource(manager,resource);
      }
    }
  }
  o.w(widget+0x9fe,0);
}
} // namespace halo2
