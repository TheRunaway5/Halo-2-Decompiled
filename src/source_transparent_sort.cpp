#include "halo2/source_transparent_sort.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
float mul(float a,float b){const volatile float v=a*b;return v;}
float add(float a,float b){const volatile float v=a+b;return v;}
float sub(float a,float b){const volatile float v=a-b;return v;}
std::int32_t difference(std::int32_t a,std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b));
}
float plane_eye(const std::array<float,4> &p,Vec3 eye) {
    //1E765/1E816: Y+Z, thenX. Center tests instead startZ+Y.
    return add(add(mul(p[1],eye.y),mul(p[2],eye.z)),mul(p[0],eye.x));
}
float plane_center(const std::array<float,4> &p,Vec3 c) {
    return add(add(mul(c.z,p[2]),mul(c.y,p[1])),mul(c.x,p[0]));
}
void payload_word(SourceTransparentCommand &c,unsigned offset,std::uint32_t value) {
    std::memcpy(c.payload.data()+offset,&value,4);
}
}
bool source_transparent_compare(const SourceTransparentCommand &a,
    const SourceTransparentCommand &b,Vec3 eye) {
    auto d=difference(a.priority,b.priority);
    if(d!=0)return d>0;
    if(a.plane_present&255) {
        const bool camera_side=a.plane[3]>plane_eye(a.plane,eye);
        return a.plane[3]>plane_center(a.plane,b.center)?!camera_side:camera_side;
    }
    if(b.plane_present&255) {
        const bool camera_side=b.plane[3]>plane_eye(b.plane,eye);
        return b.plane[3]>plane_center(b.plane,a.center)?camera_side:!camera_side;
    }
    if(a.depth>b.depth)return true;
    if(b.depth>a.depth)return false;
    return difference(a.secondary,b.secondary)>0;
}
std::vector<std::uint16_t> source_transparent_sort(
    std::span<const SourceTransparentCommand> commands,Vec3 eye,unsigned first) {
    if(commands.size()>1024 || first>commands.size())
        throw std::invalid_argument("source transparent command range exceeds1024");
    std::vector<std::uint16_t> order(commands.size()-first);
    for(unsigned i=0;i<order.size();++i)order[i]=std::uint16_t(first+i);
    if(order.size()<2)return order;
    const auto compare=[&](unsigned a,unsigned b){return source_transparent_compare(commands[order[a]],commands[order[b]],eye);};
    struct Range{int left,right;};std::array<Range,30> pending{};int count=0;
    int left=0,right=int(order.size())-1;
    for(;;) {
        if(right-left+1<=8) {
            //13E140 selection sort places the comparator-maximum at the end.
            for(int end=right;left<end;--end) {
                int maximum=left;
                for(int i=left+1;i<=end;++i)if(compare(i,maximum))maximum=i;
                std::swap(order[maximum],order[end]);
            }
        } else {
            const int middle=left+(right-left+1)/2;
            std::swap(order[middle],order[left]);int i=left,j=right+1;
            for(;;) {
                do{++i;}while(i<=right && !compare(i,left));
                do{--j;}while(j>left && !compare(left,j));
                if(i>j)break;
                std::swap(order[i],order[j]);
            }
            std::swap(order[left],order[j]);
            const auto push=[&](int a,int b) {
                if(count==int(pending.size()))throw std::runtime_error("source transparent partition stack exceeded");
                pending[count++]={a,b};
            };
            if(j-left-1<right-i) {
                if(i<right)push(i,right);
                if(left+1<j){right=j-1;continue;}
            } else {
                if(left+1<j)push(left,j-1);
                if(i<right){left=i;continue;}
            }
        }
        if(!count)break;
        const auto r=pending[--count];left=r.left;right=r.right;
    }
    return order;
}
void source_transparent_append_groups(std::span<const SourceTransparentSection> sections,
    std::vector<SourceTransparentCommand> &commands,std::uint32_t argument,unsigned limit) {
    if(sections.size()>31 || limit>1024 || commands.size()>limit)
        throw std::invalid_argument("source transparent section/command capacity exceeded");
    std::uint32_t visited=0;
    for(unsigned first=0;first<sections.size();++first) {
        if(sections[first].kind!=1 || (visited&(1u<<first)) || commands.size()==limit)continue;
        const auto &s=sections[first];SourceTransparentCommand c;c.priority=3;c.callback=0x41cc0;
        std::array<std::uint8_t,32> members{};members[0]=std::uint8_t(first);unsigned member_count=1;
        visited|=1u<<first;Vec3 minimum=s.center,maximum=s.center;float depth=s.depth;
        for(unsigned next=0;next<sections.size();++next) {
            if(visited&(1u<<next))continue;
            const auto &n=sections[next];bool overlaps=false;
            for(unsigned m=0;m<member_count;++m) {
                const auto &r=sections[members[m]].rectangle;
                if(r[0]<n.rectangle[1] && r[2]<n.rectangle[3] &&
                    n.rectangle[0]<r[1] && n.rectangle[2]<r[3]){overlaps=true;break;}
            }
            if(overlaps)continue;
            //420A0's inner candidate loop does not filter kind1.
            members[member_count++]=std::uint8_t(next);visited|=1u<<next;
            if(!(n.center.x>minimum.x))minimum.x=n.center.x;
            if(!(n.center.y>minimum.y))minimum.y=n.center.y;
            if(!(n.center.z>minimum.z))minimum.z=n.center.z;
            if(n.center.x>maximum.x)maximum.x=n.center.x;
            if(n.center.y>maximum.y)maximum.y=n.center.y;
            if(n.center.z>maximum.z)maximum.z=n.center.z;
            if(depth>n.depth)depth=n.depth;
        }
        c.payload[0]=std::uint8_t(member_count);
        std::copy_n(members.begin(),member_count,c.payload.begin()+1);
        c.depth=sub(0,depth);c.center={mul(add(minimum.x,maximum.x),.5f),
            mul(add(minimum.y,maximum.y),.5f),mul(add(minimum.z,maximum.z),.5f)};
        commands.push_back(c);
    }
    for(unsigned i=0;i<sections.size();++i)if(sections[i].kind==3 && commands.size()<limit) {
        SourceTransparentCommand c;c.priority=3;c.callback=0x423c0;
        c.depth=sub(0,sections[i].depth);c.center=sections[i].center;
        payload_word(c,0,i);payload_word(c,4,argument);commands.push_back(c);
    }
}
std::optional<SourceTransparentCommand> source_transparent_part_command(const SourceTransparentPartInput &p) {
    if(p.part_type==0 || p.part_type==5 || !p.material_present ||
        p.selected_shader==0xffffffffu || !(p.selected_template_layers&0x8000) ||
        p.grouped_context || p.view_type!=0)return {};
    SourceTransparentCommand c;
    c.priority=p.shader_flags&1?((p.shader_flags&2)?0:1):((p.shader_flags&2)?2:3);
    const auto x=mul(sub(p.center.x,p.camera_position.x),p.camera_forward.x);
    const auto y=mul(sub(p.center.y,p.camera_position.y),p.camera_forward.y);
    const auto z=mul(sub(p.center.z,p.camera_position.z),p.camera_forward.z);
    c.depth=sub(sub(0,add(add(z,y),x)),p.depth_bias);c.center=p.center;
    c.plane_present=p.plane_present;
    if(p.plane_present)c.plane=p.plane;
    c.callback=0x41020;
    payload_word(c,0,p.selected_shader);payload_word(c,4,p.draw_token);return c;
}
}
