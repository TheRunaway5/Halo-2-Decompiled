#include "halo2/source_instance_accelerator.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>

namespace halo2 {
namespace {
struct Code {
    std::span<const std::byte> bytes;
    unsigned byte(std::size_t at)const {
        if(at>=bytes.size())throw FormatError("instance accelerator instruction outside bytecode");
        return std::to_integer<unsigned>(bytes[at]);
    }
    std::uint32_t be(std::size_t at,unsigned size)const {
        std::uint32_t value=0;for(unsigned i=0;i<size;++i)value=(value<<8)|byte(at+i);return value;
    }
};
struct Branch { unsigned axis=999;float lower{},upper{};std::size_t left{},right{}; };
bool split(unsigned op){return(op>=0x10&&op<=0x1c)||(op>=0x20&&op<=0x25);}
Branch branch(const Code&c,std::size_t pc,unsigned op) {
    Branch out;
    if(op>=0x20&&op<=0x22) {
        out.axis=op-0x20;out.upper=float(c.byte(pc+1));out.lower=out.upper+1;
        out.left=pc+3;out.right=out.left+c.byte(pc+2);return out;
    }
    out.lower=float(c.byte(pc+1));out.upper=float(c.byte(pc+2));
    if(op>=0x23&&op<=0x25) {
        out.axis=op-0x23;out.left=pc+7+c.be(pc+3,2);out.right=pc+7+c.be(pc+5,2);return out;
    }
    if(op<=0x12)out.axis=op-0x10;
    else if(op==0x13||op==0x15||op==0x17){out.lower*=2;out.upper*=2;}
    else if(op==0x14||op==0x16||op==0x18){out.lower=out.lower*2-255;out.upper=out.upper*2-255;}
    else if(op==0x19){out.lower*=3;out.upper*=3;}
    else if(op==0x1a||op==0x1b){out.lower=(out.lower-85)*3;out.upper=(out.upper-85)*3;}
    else if(op==0x1c){out.lower=(out.lower-170)*3;out.upper=(out.upper-170)*3;}
    out.left=pc+4;out.right=out.left+c.byte(pc+3);return out;
}
bool terminal(unsigned op){return(op>=0x30&&op<=0x53);}
unsigned terminal_size(unsigned op){return op<0x50?1:op-0x50+2;}
unsigned sequential_size(unsigned op) {
    if(op>=1&&op<=4)return 4;
    if(op==9)return 2;if(op==10)return 3;if(op==11)return 5;
    if(op>=0x26&&op<=0x28)return 3;
    if(op>=0x29&&op<=0x2b)return 7;
    if(op>=0x60&&op<=0x63)return 2;
    if(op>=0x64&&op<=0x67)return 3;
    if(op>=0x68&&op<=0x6b)return 5;
    throw FormatError("unsupported original instance accelerator opcode: "+std::to_string(op));
}
using Vector=std::array<float,4>;
struct Segment {Vector a{},b{};};
struct State { Vector offset{};unsigned shift{};float scale{};std::uint32_t key{}; };
Vector interpolate(const Vector&a,const Vector&b,float t) {
    Vector out;
    const volatile float s=1.f-t;
    for(unsigned i=0;i<4;++i){const volatile float left=s*a[i],right=t*b[i];out[i]=left+right;}
    return out;
}
double projection(const Vector&v,unsigned op) {
    switch(op) {
    case 0x13:return double(v[2])+v[1];case 0x14:return double(v[1])-v[2];
    case 0x15:return double(v[2])+v[0];case 0x16:return double(v[0])-v[2];
    case 0x17:return double(v[1])+v[0];case 0x18:return double(v[0])-v[1];
    case 0x19:return(double(v[2])+v[1])+v[0];
    case 0x1a:return(double(v[1])+v[0])-v[2];
    case 0x1b:return(double(v[0])-v[1])+v[2];
    case 0x1c:return(double(v[0])-v[1])-v[2];
    default:throw FormatError("instance accelerator projection opcode missing");
    }
}
struct Visitor {
    const SourceInstanceAccelerator&bank;
    Code c;
    SourceInstanceAcceleratorCallback callback;
    SourceInstanceAcceleratorVisit result;
    Vector world_a{},world_b{};
    float inverse_scale{};
    unsigned steps{};
    bool shorten(Segment&segment,const State&s,const Branch&b,bool decreasing) {
        if(result.fraction<1) {
            auto end=interpolate(world_a,world_b,result.fraction);
            for(unsigned i=0;i<4;++i){const volatile float delta=end[i]-bank.offset_scale[i];
                const volatile float scaled=delta*s.scale;segment.b[i]=scaled-s.offset[i];}
            if(b.axis<3 && (decreasing?b.lower<segment.b[b.axis]:segment.b[b.axis]<b.upper))return false;
        }
        return true;
    }
    void walk(std::size_t pc,State s,Segment segment,unsigned depth=0) {
        if(depth>256)throw FormatError("instance accelerator recursion outside source bank");
        for(;;) {
            if(++steps>1000000)throw FormatError("instance accelerator bytecode does not terminate");
            const auto op=c.byte(pc);result.instructions.push_back({unsigned(pc),op});
            if(op==0)return;
            if(terminal(op)) {
                const auto count=terminal_size(op);(void)c.byte(pc+count-1);
                const auto key=s.key+(op<0x50?op-0x30:c.be(pc+1,count-1));
                result.keys.push_back(key);result.fraction=callback(key,result.fraction);return;
            }
            if(op>=1&&op<=4) {
                const float scale=float(1u<<op);
                Vector shift{float(c.byte(pc+1)),float(c.byte(pc+2)),float(c.byte(pc+3)),0};
                for(unsigned i=0;i<4;++i){segment.a[i]=(segment.a[i]-shift[i])*scale;
                    segment.b[i]=(segment.b[i]-shift[i])*scale;s.offset[i]=(s.offset[i]+shift[i])*scale;}
                s.shift+=op;s.scale=float(double(scale)*s.scale);pc+=4;continue;
            }
            if(op>=5&&op<=7){const auto width=op-4;pc+=width+1+c.be(pc+1,width);continue;}
            if(op>=9&&op<=11) {
                const auto width=op==9?1:op==10?2:4;const auto value=c.be(pc+1,width);
                if(op==11)s.key=value;else s.key+=value;pc+=width+1;continue;
            }
            if(op>=0x60&&op<=0x6b) {
                // Source properties are retained by the generic interpreter,
                // but this exact244980 shape-container/collector never reads
                // them. They affect neither source key nor visitation state.
                const auto count=sequential_size(op);(void)c.byte(pc+count-1);pc+=count;continue;
            }
            if(op>=0x26&&op<=0x2b) {
                const unsigned axis=op>=0x29?op-0x29:op-0x26;
                double lower=c.byte(pc+1),upper=c.byte(pc+2);unsigned size=3;
                if(op>=0x29){lower=(double(c.be(pc+1,3))*inverse_scale)*s.scale-s.offset[axis];
                    upper=(double(c.be(pc+4,3))*inverse_scale)*s.scale-s.offset[axis];size=7;}
                const auto a=segment.a[axis],b=segment.b[axis];
                const bool decreasing=b<=a;
                if(decreasing?(a<lower||upper<b):(b<lower||upper<a))return;
                const auto original=segment;
                const float au=float(double(a)-upper);const double bu=double(b)-upper;
                if(double(au)*bu<0) {
                    const auto t=float(double(au)/(double(au)-bu));
                    (decreasing?segment.a:segment.b)=interpolate(original.a,original.b,t);
                }
                const float al=float(double(a)-lower);const double bl=double(b)-lower;
                if(double(al)*bl<0) {
                    const auto t=float(double(al)/(double(al)-bl));
                    (decreasing?segment.b:segment.a)=interpolate(original.a,original.b,t);
                }
                pc+=size;continue;
            }
            if(split(op)) {
                const auto b=branch(c,pc,op);
                const double pa=b.axis<3?double(segment.a[b.axis]):projection(segment.a,op);
                const float pb=float(b.axis<3?double(segment.b[b.axis]):projection(segment.b,op));
                if(!(b.upper<=pb||b.upper<=pa)) {pc=b.left;continue;}
                if(!(pa<=b.lower||pb<=b.lower)) {pc=b.right;continue;}
                const float al=float(pa-b.lower),au=float(pa-b.upper),bl=float(double(pb)-b.lower),bu=float(double(pb)-b.upper);
                auto first=segment;
                if(bl<=al) {
                    if(double(bu)*au<0)first.b=interpolate(segment.a,segment.b,float(double(au)/(double(au)-bu)));
                    walk(b.right,s,first,depth+1);
                    if(double(bl)*al<0)segment.a=interpolate(segment.a,segment.b,float(double(al)/(double(al)-bl)));
                    if(!shorten(segment,s,b,true))return;
                    pc=b.left;
                } else {
                    if(double(bl)*al<0)first.b=interpolate(segment.a,segment.b,float(double(al)/(double(al)-bl)));
                    walk(b.left,s,first,depth+1);
                    if(double(bu)*au<0)segment.a=interpolate(segment.a,segment.b,float(double(au)/(double(au)-bu)));
                    if(!shorten(segment,s,b,false))return;
                    pc=b.right;
                }
                continue;
            }
            (void)sequential_size(op); // Exact unsupported-op diagnostic.
        }
    }
};
} // namespace
SourceInstanceAccelerator decode_source_instance_accelerator(
    std::span<const std::byte> raw,unsigned padded_code_size) {
    const Bytes b(raw);b.require(0,0x30);const auto length=b.u32(0x20);
    if(length>padded_code_size)throw FormatError("instance accelerator code length exceeds authored capacity");
    b.require(0x30,padded_code_size);
    SourceInstanceAccelerator out;
    for(unsigned i=0;i<4;++i){out.offset_scale[i]=b.f32(i*4);
        if(!std::isfinite(out.offset_scale[i]))throw FormatError("nonfinite instance accelerator info");}
    if(!(out.offset_scale[3]>0))throw FormatError("instance accelerator scale is not positive");
    const auto code=b.slice(0x30,length);out.code.assign(code.begin(),code.end());
    return out;
}
std::vector<SourceInstanceAcceleratorInstruction> source_instance_accelerator_inventory(
    const SourceInstanceAccelerator&bank) {
    const Code c{bank.code};std::set<std::size_t> visited;
    std::vector<SourceInstanceAcceleratorInstruction> out;
    std::function<void(std::size_t)> walk=[&](std::size_t pc) {
        while(visited.insert(pc).second) {
            const auto op=c.byte(pc);out.push_back({unsigned(pc),op});
            if(op==0)return;
            if(terminal(op)){(void)c.byte(pc+terminal_size(op)-1);return;}
            if(split(op)){const auto b=branch(c,pc,op);walk(b.left);walk(b.right);return;}
            if(op>=5&&op<=7){const auto width=op-4;pc+=width+1+c.be(pc+1,width);}
            else {const auto size=sequential_size(op);(void)c.byte(pc+size-1);pc+=size;}
        }
    };
    walk(0);return out;
}
SourceInstanceAcceleratorVisit source_instance_accelerator_visit(
    const SourceInstanceAccelerator&bank,Vec3 origin,Vec3 ray,float fraction,
    const SourceInstanceAcceleratorCallback&callback) {
    if(!callback)throw FormatError("instance accelerator collector missing");
    const float scale=float(double(bank.offset_scale[3])/65536.);
    State state;state.scale=scale;
    Visitor visitor{bank,{bank.code},callback};visitor.result.fraction=fraction;
    visitor.inverse_scale=float(1./double(bank.offset_scale[3]));
    visitor.world_a={origin.x,origin.y,origin.z,0};
    visitor.world_b={origin.x+ray.x,origin.y+ray.y,origin.z+ray.z,0};
    Segment segment;
    for(unsigned i=0;i<4;++i){const volatile float a=visitor.world_a[i]-bank.offset_scale[i],b=visitor.world_b[i]-bank.offset_scale[i];
        segment.a[i]=scale*a;segment.b[i]=scale*b;}
    visitor.walk(0,state,segment);return std::move(visitor.result);
}
} // namespace halo2
