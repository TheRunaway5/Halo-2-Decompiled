#include "halo2/source_transparent_queue.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace halo2 {
void SourceTransparentQueue::reset_world(){count_=first_=0;}
SourceTransparentCommand *SourceTransparentQueue::allocate() {
    if(count_==capacity){overflow_notice_pending_=false;return nullptr;}
    auto &c=storage_[count_++];
    c.priority=0;c.plane_present&=0xffffff00u;
    return &c;
}
SourceTransparentCommand *SourceTransparentQueue::append_part(const SourceTransparentPartInput &p) {
    const auto selected=source_transparent_part_command(p);
    if(!selected)return nullptr;
    auto *c=allocate();if(!c)return nullptr;
    c->priority=selected->priority;c->depth=selected->depth;
    c->secondary=0;c->callback=selected->callback;c->center=selected->center;
    std::copy_n(selected->payload.begin(),8,c->payload.begin());
    c->plane_present=(c->plane_present&0xffffff00u)|(selected->plane_present&255u);
    if(p.plane_present)c->plane=selected->plane;
    return c;
}
SourceTransparentCommand *SourceTransparentQueue::append_shader(const SourceTransparentShaderInput &p) {
    if(p.selected_template_layers&0x10006eu)
        throw std::invalid_argument("source shader submission requires its immediate-layer service");
    if(!(p.selected_template_layers&0x8000u) || p.view_type)return nullptr;
    auto *c=allocate();if(!c)return nullptr;
    c->priority=p.shader_priority_flags&1?((p.shader_priority_flags&2)?0:1):((p.shader_priority_flags&2)?2:3);
    const auto sub=[](float a,float b){const volatile float v=a-b;return v;};
    const auto mul=[](float a,float b){const volatile float v=a*b;return v;};
    const auto add=[](float a,float b){const volatile float v=a+b;return v;};
    const auto x=mul(sub(p.center.x,p.camera_position.x),p.camera_forward.x);
    const auto y=mul(sub(p.center.y,p.camera_position.y),p.camera_forward.y);
    const auto z=mul(sub(p.center.z,p.camera_position.z),p.camera_forward.z);
    c->depth=sub(0,add(add(z,x),y));c->secondary=0;c->callback=p.callback;c->center=p.center;
    std::memcpy(c->payload.data(),&p.shader,4);std::memcpy(c->payload.data()+4,&p.context,4);
    return c;
}
SourceTransparentCommand *SourceTransparentQueue::append_callback(Vec3 center,
    Vec3 camera_position,Vec3 camera_forward,std::uint32_t callback,
    std::span<const std::uint8_t> payload) {
    if(payload.size()>64)throw std::invalid_argument("source transparent callback payload exceeds64 bytes");
    auto *c=allocate();if(!c)return nullptr;
    const auto sub=[](float a,float b){const volatile float v=a-b;return v;};
    const auto mul=[](float a,float b){const volatile float v=a*b;return v;};
    const auto add=[](float a,float b){const volatile float v=a+b;return v;};
    const auto x=mul(camera_forward.x,sub(center.x,camera_position.x));
    const auto y=mul(camera_forward.y,sub(center.y,camera_position.y));
    const auto z=mul(camera_forward.z,sub(center.z,camera_position.z));
    c->priority=3;c->secondary=0;c->callback=callback;
    c->depth=sub(0,add(add(z,y),x));c->center=center;
    std::copy(payload.begin(),payload.end(),c->payload.begin());
    return c;
}
void SourceTransparentQueue::begin_first_person_suffix(){first_=count_;}
std::vector<std::uint16_t> SourceTransparentQueue::ordered_ordinals(Vec3 eye) const {
    return source_transparent_sort(commands(),eye,first_);
}
void SourceTransparentQueue::finish_first_person_suffix(){count_=first_;}
}
