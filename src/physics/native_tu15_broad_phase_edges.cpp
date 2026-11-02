#include "halo2/native_tu15_broad_phase_edges.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
using Vec = std::array<std::uint32_t,4>;
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(p,std::span(bytes).first(width));
        std::uint32_t value{};for(unsigned k=0;k<width;++k)value|=std::to_integer<unsigned>(bytes[k])<<(8u*k);return value;
    }
    Vec vector(std::uint32_t p)const {
        std::array<std::byte,16> bytes{};memory_.read_vector(p,bytes);Vec out;std::memcpy(out.data(),bytes.data(),16);return out;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> bytes{};for(unsigned k=0;k<4;++k)bytes[k]=std::byte(value>>(8u*k));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observer_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(p,std::span(bytes).first(width));if(observer_)observer_(event);
    }
    void move_index(std::uint32_t site,std::uint32_t p,int delta) {
        const auto value=word(p,2);word(site,p,value+static_cast<std::uint32_t>(delta),2);
    }
};

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// Only SSE executes here. The incoming logical/host x87 stacks are untouched.
class SseEnvironment {
    NativeTu15PhysicsFrameFp& fp_;unsigned saved_;
public:
    explicit SseEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),saved_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native broad-phase edges require their SSE exception owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SseEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(saved_);}
};
__m128 packed(Vec value){__m128 out;std::memcpy(&out,value.data(),16);return out;}
Vec words(__m128 value){Vec out;std::memcpy(out.data(),&value,16);return out;}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 minimum(__m128 a,__m128 b){asm volatile("minps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 maximum(__m128 a,__m128 b){asm volatile("maxps %1,%0":"+x"(a):"x"(b):"memory");return a;}
#endif

struct Quantized {std::array<std::uint32_t,3> low,high;};
Quantized quantize(Memory&m,std::uint32_t broad,std::uint32_t bounds,NativeTu15PhysicsFrameFp&fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SseEnvironment environment(fp);
    const auto input_low=m.vector(bounds),offset_low=m.vector(broad+0x10u);
    const auto scale=m.vector(broad+0x30u),ceiling=m.vector(0x41c6a0u);
    const auto floor=m.vector(0x41c690u),bias=m.vector(0x41c680u);
    auto low=add(packed(input_low),packed(offset_low));
    const auto offset_high=m.vector(broad+0x20u);
    low=multiply(low,packed(scale));low=minimum(low,packed(ceiling));
    low=maximum(low,packed(floor));low=add(low,packed(bias));
    const auto input_high=m.vector(bounds+0x10u);
    auto high=add(packed(input_high),packed(offset_high));
    high=multiply(high,packed(scale));high=minimum(high,packed(ceiling));
    high=maximum(high,packed(floor));high=add(high,packed(bias));
    const auto lo=words(low),hi=words(high);Quantized result;
    for(unsigned axis=0;axis<3;++axis){result.low[axis]=(lo[axis]>>7u)&0xfffeu;result.high[axis]=((hi[axis]>>7u)&0xffffu)|1u;}
    return result;
#else
    (void)m;(void)broad;(void)bounds;(void)fp;
    throw std::runtime_error("Native TU1.5 broad-phase SSE backend unavailable");
#endif
}

struct PairSites {std::uint32_t count{},first{},second{},special_entry{},special_count{};};
struct PassSites {
    std::uint32_t active_index{},copy_first{},copy_second{},other_low{},other_high{},edge_id{},bound{};
    PairSites pair;
};
struct Axis {
    std::uint32_t edges,low_index,high_index;
    PassSites lower_left,upper_right,upper_left,lower_right;
};
// These are store-observation identities only, never interpreted instructions.
constexpr std::array<Axis,3> axes{{
 {0x4c,8,10,
  {0x2ea5a4,0x2ea4cf,0x2ea4d2,0x2ea58f,0x2ea4dc,0x2ea59c,0x2ea5c0,{0x2ea53b,0x2ea545,0x2ea54a,0x2ea582,0x2ea58a}},
  {0x2ea5e4,0x2ea5f9,0,0x2ea60b,0x2ea6b1,0x2ea601,0,{0x2ea662,0x2ea668,0x2ea670,0x2ea6a1,0x2ea6ac}},
  {0x2ea6e9,0x2ea6fa,0,0x2ea70c,0x2ea7b4,0x2ea702,0x2ea7cb,{0x2ea761,0x2ea767,0x2ea76c,0x2ea7a9,0x2ea79f}},
  {0x2ea7f7,0x2ea808,0,0x2ea8c2,0x2ea81a,0x2ea810,0x2ea8dd,{0x2ea86f,0x2ea875,0x2ea87a,0x2ea8b7,0x2ea8ad}}},
 {0x58,0,4,
  {0x2ea9a8,0x2ea918,0x2ea90f,0x2ea98c,0x2ea91d,0x2ea99c,0x2ea9b7,{0x2ea97c,0x2ea982,0x2ea987,0,0}},
  {0x2ea9e0,0x2ea9f5,0,0x2eaa03,0x2eaa71,0x2ea9fd,0,{0x2eaa61,0x2eaa67,0x2eaa6c,0,0}},
  {0x2eaab7,0x2eaac8,0,0x2eaad6,0x2eab44,0x2eaad0,0x2eab5c,{0x2eab34,0x2eab3a,0x2eab3f,0,0}},
  {0x2eab87,0x2eab97,0,0x2eac14,0x2eaba5,0x2eab9f,0x2eac27,{0x2eac04,0x2eac0a,0x2eac0f,0,0}}},
 {0x64,2,6,
  {0x2eacf7,0x2eac68,0x2eac5f,0x2eacda,0x2eac6d,0x2eaceb,0x2ead13,{0x2eacca,0x2eacd0,0x2eacd5,0,0}},
  {0x2ead33,0x2ead48,0,0x2ead56,0x2eadc3,0x2ead50,0,{0x2eadb3,0x2eadb9,0x2eadbe,0,0}},
  {0x2eae07,0x2eae18,0,0x2eae26,0x2eae93,0x2eae20,0x2eaeab,{0x2eae83,0x2eae89,0x2eae8e,0,0}},
  {0x2eaed7,0x2eaee8,0,0x2eaf63,0x2eaef6,0x2eaef0,0x2eaf7e,{0x2eaf53,0x2eaf59,0x2eaf5e,0,0}}}
}};

class Update {
    SourcePhysicsContactMemory&memory_;Memory m_;
    const SourcePhysicsContactRecordObserver&observer_;
    const NativeTu15BroadPhaseEdgeUpdate&request_;
    NativeTu15PhysicsFrameFp&fp_;const NativeTu15ArrayGrowthServices&services_;
    NativeTu15BroadPhaseEdgeActivity*stats_;
    void grow(std::uint32_t header,std::uint32_t stride) {
        if(stats_)++stats_->array_growths;
        NativeTu15ArrayGrowthMemory storage(memory_,{header},observer_);
        native_tu15_grow_array(storage,stride,fp_,services_);
    }
    bool overlap(unsigned axis,std::uint32_t node,std::uint32_t other) {
        if(axis==0){
            const auto low=m_.word(node),other_high=m_.word(other+4u);
            const auto high=m_.word(node+4u),other_low=m_.word(other);
            return(((other_high-low)|(high-other_low))&0x80008000u)==0;
        }
        const auto difference=[&](std::uint32_t a,std::uint32_t b){const auto first=m_.word(a,2),second=m_.word(b,2);return(first-second)&0xffffu;};
        std::uint32_t outside{};
        if(axis==1){
            outside=difference(node+6u,other+2u);outside|=difference(other+6u,node+2u);
            outside|=difference(node+10u,other+8u);outside|=difference(other+10u,node+8u);
        }else{
            outside=difference(other+10u,node+8u);outside|=difference(node+10u,other+8u);
            outside|=difference(other+4u,node);outside|=difference(node+4u,other);
        }
        return(outside&0x8000u)==0;
    }
    void emit(unsigned axis,bool created,std::uint32_t node,std::uint32_t other,
        std::uint32_t index,const PairSites&sites) {
        if(axis==0){
            std::uint32_t payload{},base{};
            if(created){payload=m_.word(other+12u);base=m_.word(request_.broad_phase+0x78u);}
            else{base=m_.word(request_.broad_phase+0x78u);payload=m_.word(other+12u);}
            if(payload&1u){
                const auto record=(payload&0xfffffffeu)+base;
                if(created){
                    const auto header=record+4u,capacity=m_.word(header+8u)&0x7fffffffu;
                    if(m_.word(header+4u)==capacity)grow(header,2);
                    const auto count=m_.word(header+4u),data=m_.word(header);
                    m_.word(sites.special_entry,data+count*2u,index,2);
                    const auto next=m_.word(header+4u)+1u;m_.word(sites.special_count,header+4u,next);
                    if(stats_)++stats_->special_additions;
                }else{
                    const auto count=m_.word(record+8u);std::uint32_t found=0xffffffffu;
                    if(std::bit_cast<std::int32_t>(count)>0){
                        auto data=m_.word(record+4u);std::uint32_t cursor{};
                        do{if(m_.word(data,2)==(index&0xffffu)){found=cursor;break;}++cursor;data+=2u;}while(std::bit_cast<std::int32_t>(cursor)<std::bit_cast<std::int32_t>(m_.word(record+8u)));
                    }
                    const auto last=m_.word(record+8u)-1u;m_.word(sites.special_count,record+8u,last);
                    const auto data=m_.word(record+4u),value=m_.word(data+last*2u,2);
                    m_.word(sites.special_entry,data+found*2u,value,2);
                    if(stats_)++stats_->special_removals;
                }
                return;
            }
        }
        const auto header=created?request_.created_pairs.identity:request_.removed_pairs.identity;
        const auto capacity=m_.word(header+8u)&0x7fffffffu;
        if(m_.word(header+4u)==capacity)grow(header,8);
        const auto count=m_.word(header+4u),data=m_.word(header),pair=data+count*8u;
        m_.word(sites.count,header+4u,count+1u);
        const auto first=m_.word(node+12u);m_.word(sites.first,pair,first);
        const auto second=m_.word(other+12u);m_.word(sites.second,pair+4u,second);
        if(stats_){if(created)++stats_->created_pairs;else++stats_->removed_pairs;}
    }
    void shifted(){if(stats_)++stats_->shifted_edges;}
public:
    Update(SourcePhysicsContactMemory&m,const NativeTu15BroadPhaseEdgeUpdate&r,
        NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&s,
        const SourcePhysicsContactRecordObserver&o,NativeTu15BroadPhaseEdgeActivity*a):
        memory_(m),m_(m,o),observer_(o),request_(r),fp_(fp),services_(s),stats_(a){}
    void object(std::uint32_t handle,std::uint32_t bounds) {
        const auto q=quantize(m_,request_.broad_phase,bounds,fp_);
        const auto entry=m_.word(handle),index=m_.word(entry),nodes=m_.word(request_.broad_phase+0x40u);
        const auto node=nodes+index*16u;
        for(unsigned axis=0;axis<3;++axis){
            const auto&a=axes[axis];const auto low=q.low[axis],high=q.high[axis];
            auto position=m_.word(node+a.low_index,2);
            auto cursor=m_.word(request_.broad_phase+a.edges)+position*4u;
            auto value=m_.word(cursor-4u,2);
            const auto&left=a.lower_left;
            while(low<value){
                const auto other_index=m_.word(cursor-2u,2),other=nodes+other_index*16u;
                if(axis==0){m_.word(left.copy_first,cursor,value,2);m_.word(left.copy_second,cursor+2u,other_index,2);}
                else{m_.word(left.copy_second,cursor+2u,other_index,2);m_.word(left.copy_first,cursor,value,2);}
                if(value&1u){m_.word(left.other_high,other+a.high_index,position,2);if(overlap(axis,node,other))emit(axis,true,node,other,index,left.pair);}
                else m_.word(left.other_low,other+a.low_index,position,2);
                cursor-=4u;--position;m_.word(left.edge_id,cursor+2u,index,2);
                m_.word(left.active_index,node+a.low_index,position,2);value=m_.word(cursor-4u,2);shifted();
            }
            m_.word(left.bound,cursor,low,2);

            position=m_.word(node+a.high_index,2);cursor=m_.word(request_.broad_phase+a.edges)+position*4u;
            value=m_.word(cursor+4u,2);cursor+=4u;const auto&right=a.upper_right;
            while(high>value){
                m_.move_index(right.active_index,node+a.high_index,1);
                const auto other_index=m_.word(cursor+2u,2),other=nodes+other_index*16u;
                const auto edge=m_.word(cursor);m_.word(right.copy_first,cursor-4u,edge);m_.word(right.edge_id,cursor+2u,index,2);
                if(!(value&1u)){m_.move_index(right.other_low,other+a.low_index,-1);if(overlap(axis,node,other))emit(axis,true,node,other,index,right.pair);}
                else m_.move_index(right.other_high,other+a.high_index,-1);
                value=m_.word(cursor+4u,2);cursor+=4u;shifted();
            }

            position=m_.word(node+a.high_index,2);cursor=m_.word(request_.broad_phase+a.edges)+position*4u;
            value=m_.word(cursor-4u,2);const auto&back=a.upper_left;
            while(high<value){
                cursor-=4u;m_.move_index(back.active_index,node+a.high_index,-1);
                const auto other_index=m_.word(cursor+2u,2),other=nodes+other_index*16u;
                const auto edge=m_.word(cursor);m_.word(back.copy_first,cursor+4u,edge);m_.word(back.edge_id,cursor+2u,index,2);
                if(!(value&1u)){m_.move_index(back.other_low,other+a.low_index,1);if(overlap(axis,node,other))emit(axis,false,node,other,index,back.pair);}
                else m_.move_index(back.other_high,other+a.high_index,1);
                value=m_.word(cursor-4u,2);shifted();
            }
            m_.word(back.bound,cursor,high,2);

            position=m_.word(node+a.low_index,2);cursor=m_.word(request_.broad_phase+a.edges)+position*4u;
            value=m_.word(cursor+4u,2);const auto&forward=a.lower_right;
            while(low>value){
                cursor+=4u;m_.move_index(forward.active_index,node+a.low_index,1);
                const auto other_index=m_.word(cursor+2u,2),other=nodes+other_index*16u;
                const auto edge=m_.word(cursor);m_.word(forward.copy_first,cursor-4u,edge);m_.word(forward.edge_id,cursor+2u,index,2);
                if(value&1u){m_.move_index(forward.other_high,other+a.high_index,-1);if(overlap(axis,node,other))emit(axis,false,node,other,index,forward.pair);}
                else m_.move_index(forward.other_low,other+a.low_index,-1);
                value=m_.word(cursor+4u,2);shifted();
            }
            m_.word(forward.bound,cursor,low,2);
        }
        if(stats_)++stats_->objects;
    }
};
} // namespace

void native_tu15_update_broad_phase_edges(SourcePhysicsContactMemory&memory,
    const NativeTu15BroadPhaseEdgeUpdate&request,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&services,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15BroadPhaseEdgeActivity*activity) {
    Update update(memory,request,fp,services,observer,activity);
    auto cursor=request.handles,bounds=request.bounds;const auto end=cursor+request.count*4u;
    while(cursor<end){update.object(cursor,bounds);cursor+=4u;bounds+=32u;}
}
} // namespace halo2
