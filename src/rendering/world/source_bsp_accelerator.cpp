#include "halo2/source_bsp_accelerator.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <optional>
#include <set>

namespace halo2 {
namespace {
float add(float a,float b) {const volatile float result=a+b;return result;}
float sub(float a,float b) {const volatile float result=a-b;return result;}
float mul(float a,float b) {const volatile float result=a*b;return result;}
bool negative(float value) {return (std::bit_cast<std::uint32_t>(value)>>31)!=0;}
struct Code {
    std::span<const std::byte> bytes;
    unsigned byte(std::size_t at)const {
        if(at>=bytes.size())throw FormatError("BSP accelerator bytecode outside supplied tree");
        return std::to_integer<unsigned>(bytes[at]);
    }
    std::uint32_t be(std::size_t at,unsigned count)const {
        std::uint32_t out=0;for(unsigned i=0;i<count;++i)out=(out<<8)|byte(at+i);return out;
    }
};
bool terminal(unsigned op) {return op>=0x30&&op<=0x53;}
bool branch(unsigned op) {return (op>=0x10&&op<=0x1c)||(op>=0x20&&op<=0x25);}
unsigned size(unsigned op) {
    if(op==0||(op>=0x30&&op<=0x4f))return 1;
    if(op>=0x50&&op<=0x53)return op-0x50+2;
    if(op>=1&&op<=4)return 4;
    if(op>=5&&op<=7)return op-5+2;
    if(op==9)return 2;if(op==10)return 3;if(op==11)return 5;
    if(op>=0x10&&op<=0x1c)return 4;
    if(op>=0x20&&op<=0x22)return 3;
    if(op>=0x23&&op<=0x25)return 7;
    if(op>=0x26&&op<=0x28)return 3;
    if(op>=0x29&&op<=0x2b)return 7;
    if(op>=0x60&&op<=0x63)return 2;
    if(op>=0x64&&op<=0x67)return 3;
    if(op>=0x68&&op<=0x6b)return 5;
    throw FormatError("unsupported original BSP accelerator opcode "+std::to_string(op));
}
std::array<std::size_t,2> children(const Code&c,std::size_t pc,unsigned op) {
    if(op>=0x23&&op<=0x25)return {pc+7+c.be(pc+3,2),pc+7+c.be(pc+5,2)};
    const unsigned length=op>=0x20?3:4;
    return {pc+length,pc+length+c.byte(pc+length-1)};
}
void validate(const Code&c) {
    // One visited bit per bytecode offset, reused across queries. Offsets past
    // the tree are never revisited: their first byte read throws.
    thread_local std::vector<std::uint64_t> seen;
    thread_local std::vector<std::size_t> pending;
    seen.assign(c.bytes.size()/64+1,0);pending.assign(1,0);
    std::size_t visited=0;
    const auto first_visit=[&](std::size_t pc) {
        if(pc>=c.bytes.size())return true;
        auto &word=seen[pc/64];const auto bit=std::uint64_t{1}<<(pc%64);
        if(word&bit)return false;
        word|=bit;return true;
    };
    while(!pending.empty()) {
        auto pc=pending.back();pending.pop_back();
        while(first_visit(pc)) {
            if(++visited>1000000)throw FormatError("BSP accelerator instruction budget exceeded");
            const auto op=c.byte(pc),length=size(op);(void)c.byte(pc+length-1);
            if(op==0||terminal(op))break;
            if(branch(op)) {
                const auto next=children(c,pc,op);pending.push_back(next[1]);pc=next[0];
            } else if(op>=5&&op<=7)pc+=length+c.be(pc+1,length-1);
            else pc+=length;
        }
    }
}
struct State {
    std::uint32_t key{},property{};
    std::array<std::uint32_t,3> offset{};
    std::uint32_t shift=16;
};
struct Box {ShaderVector lower{},upper{};};
enum class Classification {outside,inside,partial};
struct Plane {ShaderVector normal{},absolute{};float distance{};};
struct Visitor {
    Code code;
    float inverse{};
    std::vector<Plane> planes;
    std::optional<ShaderVector> sphere;
    std::vector<std::uint32_t> direct,ranges;
    std::size_t direct_capacity{},range_capacity{};
    unsigned steps{};
    explicit Visitor(Code input):code(input){}
    void step() {
        if(++steps>1000000)throw FormatError("BSP accelerator traversal budget exceeded");
    }
    float coordinate(std::uint32_t value)const {
        // OriginalFILD signed32; FMUL inverse(float); FSTP float. Keep the
        // intermediate double rounding, independently of native SSE bounds.
        return float(double(std::bit_cast<std::int32_t>(value))*double(inverse));
    }
    float bound(const State&s,unsigned axis,unsigned value)const {
        return coordinate((std::uint32_t(value)<<(s.shift&31u))+s.offset[axis]);
    }
    void append(std::vector<std::uint32_t>&bank,std::size_t capacity,std::uint32_t key) {
        if(bank.size()>=capacity)throw FormatError("BSP accelerator requires original hkArray allocation");
        bank.push_back(key);
    }
    Classification classify(const Box&box)const {
        if(sphere) {
            std::array<float,3> closest,farthest;
            for(unsigned i=0;i<3;++i) {
                const auto extent=sub(box.upper[i],box.lower[i]);
                const auto distance=std::bit_cast<float>(std::bit_cast<std::uint32_t>(
                    sub(add(box.lower[i],box.upper[i]),(*sphere)[i]))&0x7fffffffu);
                closest[i]=sub(std::min(distance,extent),distance);
                farthest[i]=add(distance,extent);
            }
            const auto square_sum=[](const auto &p) {
                return add(mul(p[2],p[2]),add(mul(p[1],p[1]),mul(p[0],p[0])));
            };
            //2DD20B/2DD2E2/2DD4B8 retain the doubled radius squared on
            // the PC53 x87 stack, while all three coordinate products and
            // sums round as SSE floats. Tangency is intersecting, not inside.
            const double radius_squared=double((*sphere)[3])*double((*sphere)[3]);
            if(double(square_sum(closest))>radius_squared)return Classification::outside;
            return double(square_sum(farthest))<radius_squared?
                Classification::inside:Classification::partial;
        }
        std::array<float,3> center,radius;
        for(unsigned i=0;i<3;++i){center[i]=add(box.lower[i],box.upper[i]);radius[i]=sub(box.upper[i],box.lower[i]);}
        bool inside=true;
        for(const auto&p:planes) {
            // 2DC642..2DC69E /2DC7A0..2DC7FC /2DC9E0..2DCA3C.
            // MOVMSKPS observes the sign bit, including negative zero.
            const float extent=add(add(mul(p.absolute[0],radius[0]),mul(p.absolute[1],radius[1])),
                                   add(mul(p.absolute[2],radius[2]),0.f));
            const float distance=add(add(mul(p.normal[0],center[0]),mul(p.normal[1],center[1])),
                                     add(mul(p.normal[2],center[2]),p.distance));
            if(!negative(sub(distance,extent)))return Classification::outside;
            if(!negative(add(distance,extent)))inside=false;
        }
        return inside?Classification::inside:Classification::partial;
    }
    void key_or_property(std::size_t&pc,State&s,bool&local,unsigned op) {
        const auto length=size(op);
        if(op>=9&&op<=11) {
            const auto value=code.be(pc+1,length-1);
            if(op==11)s.key=value;else s.key+=value;
        } else {
            const unsigned property=(op-0x60)%4;
            const auto value=code.be(pc+1,length-1);
            // Literal scratch alias in2DCC20..2DCCAB: when the state is
            // already local, properties1..3 overwrite its integer offsets.
            // Before its first copy those writes are overwritten; only the
            // property0 scratch word is retained, never consumed by query0.
            if(property==0)s.property=value;
            else if(local)s.offset[property-1]=value;
        }
        local=true;pc+=length;
    }
    void bounds(std::size_t&pc,const State&s,Box&box) {
        for(;;) {
            const auto op=code.byte(pc);if(op<0x26||op>0x2b)return;
            step();const auto axis=op>=0x29?op-0x29:op-0x26;
            if(op>=0x29){box.lower[axis]=coordinate(code.be(pc+1,3));box.upper[axis]=coordinate(code.be(pc+4,3));}
            else {box.lower[axis]=bound(s,axis,code.byte(pc+1));box.upper[axis]=bound(s,axis,code.byte(pc+2));}
            pc+=size(op);
        }
    }
    void endpoint(std::size_t pc,State s,bool last) {
        for(;;) {
            step();const auto op=code.byte(pc),length=size(op);
            if(op==0)return;
            if(terminal(op)) {
                const auto key=op<0x50?op-0x30:code.be(pc+1,length-1);
                append(ranges,range_capacity,s.key+key);return;
            }
            if(branch(op)){pc=children(code,pc,op)[last];continue;}
            if(op>=5&&op<=7){pc+=length+code.be(pc+1,length-1);continue;}
            if(op>=9&&op<=11) {
                const auto value=code.be(pc+1,length-1);
                if(op==11)s.key=value;else s.key+=value;
            }
            pc+=length;
        }
    }
    void accept(std::size_t pc,const State&s) {endpoint(pc,s,false);endpoint(pc,s,true);}
    void walk(std::size_t pc,State s,Box box,unsigned depth=0) {
        if(depth>256)throw FormatError("BSP accelerator recursion budget exceeded");
        bool local=false;
        for(;;) {
            step();const auto op=code.byte(pc),length=size(op);
            if(op==0)return;
            if(terminal(op)) {
                const auto key=op<0x50?op-0x30:code.be(pc+1,length-1);
                append(direct,direct_capacity,s.key+key);return;
            }
            if(op>=1&&op<=4) {
                for(unsigned i=0;i<3;++i)s.offset[i]+=std::uint32_t(code.byte(pc+1+i))<<(s.shift&31u);
                s.shift-=op;local=true;pc+=4;continue;
            }
            if(op>=5&&op<=7){pc+=length+code.be(pc+1,length-1);continue;}
            if((op>=9&&op<=11)||(op>=0x60&&op<=0x6b)){key_or_property(pc,s,local,op);continue;}
            if(op>=0x13&&op<=0x1c) {
                // Query0 does not use diagonal split projections. It visits
                // left recursively with a copied box, then right unchanged.
                const auto next=children(code,pc,op);walk(next[0],s,box,depth+1);pc=next[1];continue;
            }
            if(branch(op)) {
                const unsigned axis=op>=0x23?op-0x23:op>=0x20?op-0x20:op-0x10;
                auto right=box;
                if(op>=0x20&&op<=0x22) {
                    box.upper[axis]=bound(s,axis,code.byte(pc+1)+1);
                    right.lower[axis]=bound(s,axis,code.byte(pc+1));
                } else {
                    box.upper[axis]=bound(s,axis,code.byte(pc+1));
                    right.lower[axis]=bound(s,axis,code.byte(pc+2));
                }
                auto next=children(code,pc,op);
                bounds(next[1],s,right);bounds(next[0],s,box);
                const auto right_result=classify(right);
                if(right_result==Classification::inside)accept(next[1],s);
                const auto left_result=classify(box);
                if(left_result==Classification::inside)accept(next[0],s);
                if(right_result==Classification::partial) {
                    if(left_result==Classification::partial){walk(next[1],s,right,depth+1);pc=next[0];}
                    else {pc=next[1];box=right;}
                } else if(left_result==Classification::partial)pc=next[0];
                else return;
                continue;
            }
            if(op>=0x26&&op<=0x2b) {
                bounds(pc,s,box);const auto result=classify(box);
                if(result==Classification::outside)return;
                if(result==Classification::inside){accept(pc,s);return;}
                continue;
            }
            throw FormatError("unsupported BSP accelerator traversal state");
        }
    }
};
} // namespace
SourceBspAcceleratorResult source_bsp_accelerator_query0(
    std::span<const std::byte> tree,std::span<const ShaderVector> planes,
    std::vector<std::uint32_t>& direct,std::vector<std::uint32_t>& ranges,
    std::size_t direct_capacity,std::size_t range_capacity) {
    try {
        if(planes.size()>32)throw FormatError("BSP accelerator exceeds original32-plane stack bank");
        if(direct_capacity>0x7fffffffu||range_capacity>0x7fffffffu)
            throw FormatError("BSP accelerator capacity exceeds original low31-bit bank");
        if(direct.size()>direct_capacity||ranges.size()>range_capacity)
            throw FormatError("BSP accelerator input list exceeds source capacity");
        if(ranges.size()%2)throw FormatError("BSP accelerator input range endpoint bank is odd");
        const Bytes raw(tree);raw.require(0,0x30);ShaderVector info;
        for(unsigned i=0;i<4;++i){info[i]=raw.f32(i*4);if(!std::isfinite(info[i]))throw FormatError("nonfinite BSP accelerator header");}
        if(!(info[3]>0))throw FormatError("BSP accelerator scale is not positive");
        const Code code{tree.subspan(0x30)};validate(code);
        Visitor visitor{code};visitor.inverse=float(1./double(info[3]));
        if(!std::isfinite(visitor.inverse))throw FormatError("BSP accelerator inverse scale overflows");
        visitor.direct=direct;visitor.ranges=ranges;
        visitor.direct_capacity=direct_capacity;visitor.range_capacity=range_capacity;
        for(const auto&p:planes) {
            for(float value:p)if(!std::isfinite(value))throw FormatError("nonfinite BSP accelerator plane");
            Plane plane;plane.normal=p;
            for(unsigned i=0;i<3;++i)plane.absolute[i]=std::bit_cast<float>(std::bit_cast<std::uint32_t>(p[i])&0x7fffffffu);
            plane.distance=add(add(add(mul(p[0],info[0]),mul(p[1],info[1])),mul(p[2],info[2])),p[3]);
            plane.distance=add(plane.distance,plane.distance);visitor.planes.push_back(plane);
        }
        Box box;box.upper.fill(float(double(visitor.inverse)*16777216.));
        visitor.walk(0,{},box);
        if(visitor.ranges.size()%2)throw FormatError("BSP accelerator produced incomplete range endpoint pair");
        direct=std::move(visitor.direct);ranges=std::move(visitor.ranges);return {true,{}};
    } catch(const FormatError&error) {return {false,error.what()};}
}
SourceBspAcceleratorResult source_bsp_accelerator_sphere(
    std::span<const std::byte> tree,Vec3 center,float radius,
    std::vector<std::uint32_t>& direct,std::vector<std::uint32_t>& ranges,
    std::size_t direct_capacity,std::size_t range_capacity) {
    try {
        if(direct_capacity>0x7fffffffu||range_capacity>0x7fffffffu ||
           direct.size()>direct_capacity||ranges.size()>range_capacity || ranges.size()%2)
            throw FormatError("BSP sphere accelerator input bank is incomplete");
        const Bytes raw(tree);raw.require(0,0x30);ShaderVector info;
        for(unsigned i=0;i<4;++i){info[i]=raw.f32(i*4);if(!std::isfinite(info[i]))throw FormatError("nonfinite BSP sphere accelerator header");}
        if(!(info[3]>0))throw FormatError("BSP sphere accelerator scale is not positive");
        const Code code{tree.subspan(0x30)};validate(code);
        Visitor visitor{code};visitor.inverse=float(1./double(info[3]));
        if(!std::isfinite(visitor.inverse))throw FormatError("BSP sphere accelerator inverse scale overflows");
        ShaderVector sphere{center.x,center.y,center.z,radius};
        for(unsigned i=0;i<4;++i) {
            if(!std::isfinite(sphere[i]))throw FormatError("nonfinite BSP sphere accelerator query");
            const auto value=i<3?sub(sphere[i],info[i]):sphere[i];sphere[i]=add(value,value);
        }
        visitor.sphere=sphere;visitor.direct=direct;visitor.ranges=ranges;
        visitor.direct_capacity=direct_capacity;visitor.range_capacity=range_capacity;
        Box box;box.upper.fill(float(double(visitor.inverse)*16777216.));
        visitor.walk(0,{},box);
        if(visitor.ranges.size()%2)throw FormatError("BSP sphere accelerator produced incomplete endpoint pair");
        direct=std::move(visitor.direct);ranges=std::move(visitor.ranges);return {true,{}};
    } catch(const FormatError&error) {return {false,error.what()};}
}
} // namespace halo2
