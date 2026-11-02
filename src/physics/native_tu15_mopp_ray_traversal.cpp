#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t n{};
        for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_)observe_(event);
    }
    std::array<std::uint32_t,4> vector(std::uint32_t p)const{
        std::array<std::byte,16>b{};memory_.read_vector(p,b);std::array<std::uint32_t,4>v{};std::memcpy(v.data(),b.data(),16);return v;
    }
};
[[noreturn]]void missing(NativeTu15MoppRayTraversalStats*stats,std::uint32_t owner){
    if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
    throw std::runtime_error("Native TU1.5 MOPP ray requires native owner="+std::to_string(owner));
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// This owns the host processor's numerical environment. Arithmetic below
// executes directly on native x87/SSE; there is no emulated register file.
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;bool active_{};
    void validate(){
        if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||
            (fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))
            throw std::runtime_error("Native MOPP ray requires its FP exception owner");
    }
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_mxcsr_(_mm_getcsr()){
        if(depth)throw std::runtime_error("Native MOPP ray requires its balanced caller FP domain");validate();
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
};
struct Comparison {
    std::uint16_t status{};
    bool not_less()const{const auto pair=(status>>8u)&5u;return pair==0u||pair==5u;}
    bool less()const{return!not_less();}
    bool greater()const{return(status&0x4100u)==0u;}
};
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void integer(std::int32_t value){asm volatile("fildl %0"::"m"(value):"memory","st");}
void add(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
void reverse_subtract(std::uint32_t bits){asm volatile("fsubrs %0"::"m"(bits):"memory","st");}
void multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void reverse_divide(std::uint32_t bits){asm volatile("fdivrs %0"::"m"(bits):"memory","st");}
void subtract_second(){asm volatile(".intel_syntax noprefix; fsub st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
void reverse_subtract_second(){asm volatile(".intel_syntax noprefix; fsubr st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
void multiply_second(){asm volatile(".intel_syntax noprefix; fmul st(0),st(1); .att_syntax prefix":::"memory","st","st(1)");}
void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
std::uint32_t store(bool pop=true){std::uint32_t bits;if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory","st");return bits;}
Comparison compare(std::uint32_t bits,bool pop=true){
    std::uint16_t status;if(pop)asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");else asm volatile("fcoms %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");return{status};
}
Comparison compare_second(bool third=false){
    std::uint16_t status;if(third)asm volatile(".intel_syntax noprefix; fcomp st(2); .att_syntax prefix; fnstsw %0":"=a"(status)::"memory","st","st(1)","st(2)");else asm volatile(".intel_syntax noprefix; fcomp st(1); .att_syntax prefix; fnstsw %0":"=a"(status)::"memory","st","st(1)");return{status};
}
using Vec=std::array<std::uint32_t,4>;
__m128 packed(Vec value){__m128 out;std::memcpy(&out,value.data(),16);return out;}
Vec words(__m128 value){Vec out;std::memcpy(out.data(),&value,16);return out;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 broadcast(std::uint32_t value){Vec v{value,value,value,value};return packed(v);}
Vec interpolate(Vec start,Vec end,std::uint32_t fraction,Vec one){
    const auto weight=broadcast(fraction);auto right=multiply(weight,packed(end));
    auto left=subtract(packed(one),weight);left=multiply(left,packed(start));return words(add(left,right));
}
Vec interpolate(Memory&m,Vec start,Vec end,std::uint32_t fraction){
    const auto weight=broadcast(fraction);auto right=multiply(weight,packed(end));
    auto left=subtract(packed(m.vector(0x41c500u)),weight);left=multiply(left,packed(start));return words(add(left,right));
}
class Traversal {
    Memory&m_;Environment&environment_;NativeTu15PhysicsFrameFp&fp_;
    const NativeTu15MoppRayTraversalServices&services_;NativeTu15MoppRayTraversalStats*stats_;
    std::uint32_t cache_;
    std::uint32_t byte(std::uint32_t p){return m_.word(p,1);}
    void leaf(std::uint32_t key,const NativeTu15MoppTraversalVariables&variables){
        if(stats_)++stats_->leaves;key+=variables.fields10_1c[2];
        const auto filter=m_.word(cache_+0x44u);
        if(filter){
            const auto collection=m_.word(cache_+0x64u),table=m_.word(filter),owner=m_.word(table);
            if(!services_.filter)missing(stats_,owner);if(stats_)++stats_->filters;
            if(!environment_.dependency([&]{return services_.filter(filter,owner,cache_+0x20u,collection,key,fp_);}))return;
        }
        const auto collection=m_.word(cache_+0x64u),table=m_.word(collection),owner=m_.word(table+0x34u);
        if(!services_.child_shape)missing(stats_,owner);if(stats_)++stats_->child_shapes;
        const auto shape=environment_.dependency([&]{return services_.child_shape(collection,owner,key,fp_);});
        const auto output=m_.word(cache_+0x58u),shape_table=m_.word(shape.identity);
        if(output){
            const auto cast_owner=m_.word(shape_table+0x20u);if(!services_.cast_direct)missing(stats_,cast_owner);if(stats_)++stats_->direct_casts;
            if(!environment_.dependency([&]{return services_.cast_direct(shape,cast_owner,cache_+0x20u,output,fp_);}))return;
            const auto current_output=m_.word(cache_+0x58u);
            m_.word(0x30b65bu,cache_+0x50u,1u,1);const auto fraction=m_.word(current_output+0x14u);
            m_.word(0x30b662u,cache_+0x54u,fraction);m_.word(0x30b665u,current_output+0x10u,key);return;
        }
        const auto parent=m_.word(cache_+0x60u),parent_field8=m_.word(parent+8u),collector=m_.word(cache_+0x5cu),cast_owner=m_.word(shape_table+0x24u);
        if(!services_.cast_collector)missing(stats_,cast_owner);if(stats_)++stats_->collector_casts;
        environment_.dependency([&]{services_.cast_collector(shape,cast_owner,cache_+0x20u,{shape.identity,key,parent_field8,parent},collector,fp_);});
        const auto current_collector=m_.word(cache_+0x5cu),fraction=m_.word(current_collector+4u);m_.word(0x30b69fu,cache_+0x54u,fraction);
    }
    void shorten_end(const NativeTu15MoppTraversalVariables&v,NativeTu15MoppTraversalBounds&bounds){
        const auto fraction=m_.word(cache_+0x54u);const auto original_end=m_.vector(cache_+0x30u);const auto code=m_.word(cache_+0x10u);
        auto one=packed(m_.vector(0x41c500u));const auto code_info=m_.vector(code);const auto weight=broadcast(fraction);
        auto right=multiply(weight,packed(original_end));const auto original_start=m_.vector(cache_+0x20u);
        one=subtract(one,weight);one=multiply(one,packed(original_start));auto end=subtract(add(one,right),packed(code_info));bounds.end=words(end);
        end=multiply(broadcast(v.fields10_1c[1]),packed(bounds.end));bounds.end=words(end);
        bounds.end=words(subtract(packed(bounds.end),packed(v.translation)));
    }
    // Entry has the first endpoint projection on the native x87 stack.
    // A true result is the source's early return after a closer collector hit.
    bool split(std::uint32_t&code,unsigned axis,std::uint32_t low,std::uint32_t high,
        std::uint32_t left,std::uint32_t right,NativeTu15MoppTraversalVariables&v,
        NativeTu15MoppTraversalBounds&bounds,unsigned depth,std::uint32_t end){
        push(end);if(compare(high).less()){
            if(compare(high,false).less()){code+=left;discard();return false;}
        }
        // The second end-point compare above leaves the first projection live.
        // Original30AE5D advances to the right child before its remaining tests.
        code+=right;
        if(compare(low,false).greater()){
            push(end);if(compare(low).greater()){discard();return false;}
        }
        push(low);reverse_subtract_second();const auto start_low=store();
        subtract(high);const auto start_high=store();
        push(end);subtract(low);const auto end_low=store();
        push(end);subtract(high);const auto end_high=store();
        push(start_low);const bool increasing=compare(end_low).less();
        NativeTu15MoppTraversalBounds child=bounds;
        if(increasing){
            push(end_low);multiply(start_low);if(compare(m_.word(0x46a0e8u)).less()){
                push(start_low);subtract(end_low);reverse_divide(start_low);child.end=interpolate(m_,bounds.start,bounds.end,store());
            }
            if(stats_){++stats_->branches;++stats_->recursive_calls;}run(code+left-right,v,child,depth+1u);
            push(end_high);multiply(start_high);if(compare(m_.word(0x46a0e8u)).less()){
                push(start_high);subtract(end_high);const auto one=m_.vector(0x41c500u);reverse_divide(start_high);bounds.start=interpolate(bounds.start,bounds.end,store(),one);
            }
            push(m_.word(cache_+0x54u));if(compare(m_.word(0x46a0d0u)).less()){
                shorten_end(v,bounds);if(axis<3u){push(bounds.end[axis]);if(compare(high).less())return true;}
            }
        }else{
            push(end_high);multiply(start_high);if(compare(m_.word(0x46a0e8u)).less()){
                push(start_high);subtract(end_high);reverse_divide(start_high);child.end=interpolate(m_,bounds.start,bounds.end,store());
            }
            if(stats_){++stats_->branches;++stats_->recursive_calls;}run(code,v,child,depth+1u);
            push(end_low);multiply(start_low);if(compare(m_.word(0x46a0e8u)).less()){
                push(start_low);subtract(end_low);const auto one=m_.vector(0x41c500u);reverse_divide(start_low);bounds.start=interpolate(bounds.start,bounds.end,store(),one);
            }
            push(m_.word(cache_+0x54u));if(compare(m_.word(0x46a0d0u)).less()){
                shorten_end(v,bounds);if(axis<3u){push(bounds.end[axis]);if(compare(low).greater())return true;}
            }
            code+=left-right;
        }
        return false;
    }
    // Entry has low then high slab planes on the native x87 stack.
    bool clip(unsigned axis,NativeTu15MoppTraversalBounds&bounds){
        push(bounds.start[axis]);const auto start=store();push(bounds.end[axis]);const auto end=store();
        push(start);const bool reverse=compare(end).not_less();
        if(!reverse){
            push(end);if(compare_second(true).less()){discard();discard();return true;}
            push(start);if(compare_second().greater()){discard();discard();return true;}
        }else{
            push(start);if(compare_second(true).less()){discard();discard();return true;}
            push(end);if(compare_second().greater()){discard();discard();return true;}
        }
        const auto first=bounds.start,last=bounds.end;
        push(start);subtract_second();const auto start_high=store();reverse_subtract(end);
        push(start_high);multiply_second();if(compare(m_.word(0x46a0e8u)).less()){
            push(start_high);const auto one=m_.vector(0x41c500u);subtract_second();reverse_divide(start_high);const auto value=interpolate(first,last,store(),one);
            if(reverse)bounds.start=value;else bounds.end=value;
        }
        discard();push(start);subtract_second();const auto start_low=store();reverse_subtract(end);
        push(start_low);multiply_second();if(compare(m_.word(0x46a0e8u)).less()){
            push(start_low);subtract_second();reverse_divide(start_low);const auto value=interpolate(m_,first,last,store());
            if(reverse)bounds.end=value;else bounds.start=value;
        }
        discard();return false;
    }
public:
    Traversal(Memory&m,Environment&e,NativeTu15PhysicsFrameFp&fp,std::uint32_t cache,
        const NativeTu15MoppRayTraversalServices&s,NativeTu15MoppRayTraversalStats*stats):m_(m),environment_(e),fp_(fp),services_(s),stats_(stats),cache_(cache){}
    void run(std::uint32_t code,const NativeTu15MoppTraversalVariables&input,
        NativeTu15MoppTraversalBounds&bounds,unsigned depth=0){
        if(depth>512u)throw std::runtime_error("Native MOPP ray requires its recursion-capacity owner");
        auto variables=input;
        for(;;){
            const auto opcode=byte(code);if(stats_)++stats_->opcodes;
            if(opcode==0u)return;
            if(opcode>=1u&&opcode<=4u){
                const auto x=byte(code+1u),y=byte(code+2u),z=byte(code+3u);
                integer(std::int32_t(x));const auto xf=store();integer(std::int32_t(y));const auto yf=store();integer(std::int32_t(z));const auto zf=store();
                const Vec translation{xf,yf,zf,0u};auto begin=subtract(packed(bounds.start),packed(translation));bounds.start=words(begin);
                auto end=subtract(packed(bounds.end),packed(translation));bounds.end=words(end);
                integer(std::int32_t(1u<<opcode));const auto scale=store(false);
                bounds.start=words(multiply(broadcast(scale),packed(bounds.start)));bounds.end=words(multiply(broadcast(scale),packed(bounds.end)));
                variables.translation=words(multiply(broadcast(scale),add(packed(variables.translation),packed(translation))));
                variables.fields10_1c[0]+=opcode;multiply(variables.fields10_1c[1]);variables.fields10_1c[1]=store();code+=4u;continue;
            }
            if(opcode>=5u&&opcode<=7u){
                const unsigned size=opcode-4u;std::uint32_t jump{};for(unsigned n=0;n<size;++n)jump=(jump<<8u)+byte(code+n+1u);code+=jump+size+1u;continue;
            }
            if(opcode==9u||opcode==10u||opcode==11u){
                const unsigned size=opcode==9u?1u:opcode==10u?2u:4u;std::uint32_t value{};for(unsigned n=0;n<size;++n)value=(value<<8u)+byte(code+n+1u);
                if(opcode==11u)variables.fields10_1c[2]=value;else variables.fields10_1c[2]+=value;code+=size+1u;continue;
            }
            if(opcode>=0x10u&&opcode<=0x25u){
                unsigned axis=999u;std::uint32_t low{},high{},left{},right{};
                if(opcode<=0x12u){
                    const auto a=byte(code+1u),b=byte(code+2u);integer(std::int32_t(a));low=store();integer(std::int32_t(b));high=store();axis=opcode-0x10u;push(bounds.start[axis]);push(bounds.end[axis]);
                    right=byte(code+3u);code+=4u;
                }else if(opcode<=0x1cu){
                    const auto a=byte(code+1u),b=byte(code+2u);const bool triple=opcode>=0x19u;
                    const int bias=(opcode==0x14u||opcode==0x16u||opcode==0x18u)?255:opcode==0x1au||opcode==0x1bu?0x55:opcode==0x1cu?0xaa:0;
                    const auto boundary=[&](std::uint32_t n){return triple?3*(std::int32_t(n)-bias):2*std::int32_t(n)-bias;};
                    integer(boundary(a));low=store();integer(boundary(b));high=store();
                    const auto project=[&](const Vec&v){
                        if(opcode==0x13u){push(v[2]);add(v[1]);}
                        else if(opcode==0x14u){push(v[1]);subtract(v[2]);}
                        else if(opcode==0x15u){push(v[2]);add(v[0]);}
                        else if(opcode==0x16u){push(v[0]);subtract(v[2]);}
                        else if(opcode==0x17u){push(v[1]);add(v[0]);}
                        else if(opcode==0x18u){push(v[0]);subtract(v[1]);}
                        else if(opcode==0x19u){push(v[2]);add(v[1]);add(v[0]);}
                        else if(opcode==0x1au){push(v[1]);add(v[0]);subtract(v[2]);}
                        else{push(v[0]);subtract(v[1]);if(opcode==0x1cu)subtract(v[2]);else add(v[2]);}
                    };project(bounds.start);project(bounds.end);right=byte(code+3u);code+=4u;
                }else if(opcode>=0x20u&&opcode<=0x22u){
                    const auto a=byte(code+1u);right=byte(code+2u);integer(std::int32_t(a));high=store(false);add(m_.word(0x46a0d0u));low=store();axis=opcode-0x20u;push(bounds.start[axis]);push(bounds.end[axis]);code+=3u;
                }else if(opcode>=0x23u){
                    const auto a=byte(code+1u),b=byte(code+2u),ll=byte(code+4u),rr=byte(code+6u);
                    integer(std::int32_t(a));low=store();integer(std::int32_t(b));high=store();axis=opcode-0x23u;push(bounds.start[axis]);push(bounds.end[axis]);left=(byte(code+3u)<<8u)+ll;right=(byte(code+5u)<<8u)+rr;code+=7u;
                }else{if(stats_)++stats_->invalid_opcodes;throw std::runtime_error("Native MOPP ray encountered invalid geometry opcode");}
                const auto end=store();if(split(code,axis,low,high,left,right,variables,bounds,depth,end))return;continue;
            }
            if(opcode>=0x26u&&opcode<=0x2bu){
                const auto axis=opcode<0x29u?opcode-0x26u:opcode-0x29u;
                if(opcode<0x29u){const auto a=byte(code+1u),b=byte(code+2u);integer(std::int32_t(a));integer(std::int32_t(b));code+=3u;}
                else{
                    const auto a=byte(code+1u),b=byte(code+2u),c=byte(code+3u),upper_mid=byte(code+5u);
                    integer(std::int32_t((a<<16u)+(b<<8u)+c));multiply(m_.word(cache_+0x14u));const auto upper_hi=byte(code+4u),upper_low=byte(code+6u);
                    multiply(variables.fields10_1c[1]);subtract(variables.translation[axis]);
                    integer(std::int32_t((upper_hi<<16u)+(upper_mid<<8u)+upper_low));multiply(m_.word(cache_+0x14u));multiply(variables.fields10_1c[1]);subtract(variables.translation[axis]);code+=7u;
                }
                if(clip(axis,bounds))return;continue;
            }
            if(opcode>=0x30u&&opcode<=0x53u){
                std::uint32_t key{};if(opcode<0x50u)key=opcode-0x30u;else{const auto size=opcode-0x4fu;for(unsigned n=0;n<size;++n)key=(key<<8u)+byte(code+n+1u);}leaf(key,variables);return;
            }
            if(opcode>=0x60u&&opcode<=0x6bu){
                const unsigned size=opcode<0x64u?1u:opcode<0x68u?2u:4u;std::uint32_t value{};for(unsigned n=0;n<size;++n)value=(value<<8u)+byte(code+n+1u);
                // Only property0 joins the copied variables; the other source
                // local words are overwritten by the later ray-body packet.
                if((opcode&3u)==0u)variables.fields10_1c[3]=value;code+=size+1u;continue;
            }
            if(stats_)++stats_->invalid_opcodes;throw std::runtime_error("Native MOPP ray requires its invalid-program diagnostic owner");
        }
    }
};
#endif
}
void native_tu15_traverse_mopp_ray(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t code,const NativeTu15MoppTraversalVariables&variables,
    NativeTu15MoppTraversalBounds&bounds,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15MoppRayTraversalServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MoppRayTraversalStats*stats){
    if(stats)++stats->calls;try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(memory,observe);Traversal walk(m,environment,fp,cache,services,stats);walk.run(code,variables,bounds);if(stats)++stats->completed;
#else
    (void)memory;(void)cache;(void)code;(void)variables;(void)bounds;(void)fp;(void)depth;(void)services;(void)observe;
    throw std::runtime_error("Native MOPP ray FP backend is unavailable");
#endif
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
