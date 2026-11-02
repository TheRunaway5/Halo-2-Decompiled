#include "halo2/native_tu15_body_constraint_update.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_geometry.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&memory,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>bytes{};memory.read(p,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::int32_t signed_short(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
NativeTu15GeometryVector vector(const SourcePhysicsContactMemory&memory,std::uint32_t p){
    if(p&15u)throw std::runtime_error("Native TU1.5 model constraint frame requires aligned motion storage");
    std::array<std::byte,16>bytes{};memory.read_vector(p,bytes);NativeTu15GeometryVector result{};
    for(unsigned n=0;n<16;++n)result[n/4u]|=std::to_integer<unsigned>(bytes[n])<<((n&3u)*8u);return result;
}
class FrameComposition final:public NativeTu15GeometryStorage {
    const SourcePhysicsContactMemory&memory_;std::uint32_t source_;
    const std::array<NativeTu15GeometryVector,4>basis_;
public:
    std::array<NativeTu15GeometryVector,4>columns;
    FrameComposition(const SourcePhysicsContactMemory&m,std::uint32_t source,
      const std::array<NativeTu15GeometryVector,4>&basis):memory_(m),source_(source),basis_(basis),columns(basis){}
    NativeTu15GeometryVector basis_column(unsigned n)const override{return basis_.at(n);}
    NativeTu15GeometryVector source_column(unsigned n)const override{return vector(memory_,source_+n*16u);}
    NativeTu15GeometryVector destination_column(unsigned n)const override{return columns.at(n);}
    void destination_column(unsigned n,const NativeTu15GeometryVector&v,NativeTu15GeometryPublication)override{columns.at(n)=v;}
};
class SimdEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 model constraint reference distance requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 subtract(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char result;asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result;}
class Update {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
    const NativeTu15ModelConstraintServices&services_;NativeTu15PhysicsFrameFp&fp_;
    NativeTu15ModelConstraintScratch scratch_;NativeTu15ModelConstraintUpdateStats*stats_;
public:
    Update(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
      const NativeTu15ModelConstraintServices&s,NativeTu15PhysicsFrameFp&fp,
      NativeTu15ModelConstraintScratch scratch,NativeTu15ModelConstraintUpdateStats*stats)
      :memory_(m),observer_(o),services_(s),fp_(fp),scratch_(scratch),stats_(stats){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{return read(memory_,p,width);}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(p,std::span(bytes).first(width));if(observer_)observer_(event);
    }
    template<class Service>void require(const Service&service,std::uint32_t owner){
        if(stats_)++stats_->dependent_owners[owner];
        if(!service){if(stats_)++stats_->unbound_services;throw std::runtime_error("Native TU1.5 model constraint activation requires its actual positive-loop owner "+std::to_string(owner));}
    }
    std::uint32_t property(std::uint32_t body,std::uint32_t count,std::uint32_t key){
        if(!less(0,count))return 0;const auto bank=word(body+0x30u);std::uint32_t index{};
        do{if(word(bank+index*8u)==key)return word(bank+index*8u+4u);++index;}while(less(index,count));return 0;
    }
    std::array<std::uint32_t,2>body_slots(std::uint32_t component,std::uint32_t index){ //DE8C0
        if(stats_)++stats_->body_slot_queries;
        const auto entries=word(component+0x7cu),constraint=word(entries+index*12u+8u),
            first=word(constraint+0xcu),first_count=word(first+0x34u),second=word(constraint+0x10u);
        const auto first_slot=property(first,first_count,0x2002u),second_count=word(second+0x34u);
        if(property(second,second_count,0x2001u)==0xffffffffu)return{first_slot,0xffffffffu};
        return{first_slot,property(second,second_count,0x2002u)};
    }
    std::array<std::uint32_t,2>definition(std::uint32_t model,std::uint16_t type){ //1F3460
        if(stats_)++stats_->definition_queries;
        // Authenticated jump-table order at1F34B4: hinge,limited hinge,
        // ragdoll,stiff spring,ball/socket,prismatic. No guessed shape type.
        constexpr std::array<std::uint32_t,6>offsets{0xb0u,0xe8u,0xb8u,0xf8u,0xf0u,0x100u};
        constexpr std::array<std::uint32_t,6>strides{0x78u,0x84u,0x94u,0x7cu,0x78u,0x84u};
        if(signed_short(type)<0||type>=offsets.size()){
            if(stats_)++stats_->unknown_definition_kinds;
            throw std::runtime_error("Native TU1.5 model constraint type is outside its authored six-owner domain");
        }
        // Source1F3460 reads this authenticated classification DATA before
        // entering its native enum branch. Never execute the stored code ID.
        constexpr std::array<std::uint32_t,6>targets{
            0x1f346au,0x1f3476u,0x1f3482u,0x1f348eu,0x1f349au,0x1f34a6u};
        if(word(0x1f34b4u+std::uint32_t(type)*4u)!=targets[type]){
            if(stats_)++stats_->unknown_definition_kinds;
            throw std::runtime_error("Native TU1.5 model constraint definition has an unknown authored classification");
        }
        return{model+offsets[type],strides[type]};
    }
    void request(std::uint32_t datum,const std::array<std::uint32_t,2>&slots,
      std::uint16_t first_node,std::uint16_t second_node,std::uint16_t type,std::uint16_t index,
      const NativeTu15ModelConstraintFrame&frame){ //20D8B0
        if(stats_)++stats_->requests;const auto p=scratch_.request.identity;
        word(0x20d8b0u,p,0u,1);word(0x20d8b6u,p+4u,datum);word(0x20d8bdu,p+8u,slots[0]);
        word(0x20d8c5u,p+0xcu,first_node,2);word(0x20d8cdu,p+0x10u,slots[1]==0xffffffffu?0xffffffffu:datum);
        word(0x20d8d4u,p+0x14u,slots[1]);word(0x20d8dcu,p+0x18u,second_node,2);
        word(0x20d8ebu,p+0x1cu,frame.position[0]);word(0x20d8f0u,p+0x20u,frame.position[1]);word(0x20d8f6u,p+0x24u,frame.position[2]);
        word(0x20d8feu,p+0x28u,frame.forward[0]);word(0x20d903u,p+0x2cu,frame.forward[1]);word(0x20d910u,p+0x30u,frame.forward[2]);
        word(0x20d917u,p+0x34u,0xffffffffu);word(0x20d91au,p+0x38u,0u,1);
        word(0x20d935u,p+0x3au,std::uint32_t(type)|(std::uint32_t(index)<<16u));
    }
    std::uint32_t scaled_extent(std::uint32_t value,std::uint32_t scale){
        if((fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 model constraint activation requires its unsupported SIMD fault owner");
        const auto old=_mm_getcsr();_mm_setcsr(fp_.mxcsr);
        auto a=_mm_castsi128_ps(_mm_cvtsi32_si128(std::int32_t(value)));const auto b=_mm_castsi128_ps(_mm_cvtsi32_si128(std::int32_t(scale)));
        asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");const auto result=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(a)));fp_.mxcsr=_mm_getcsr();_mm_setcsr(old);return result;
    }
    void append(std::uint32_t component,std::uint32_t constraint){ //20D940
        if(stats_)++stats_->appends;const auto pool=word(0x52d690u);
        if(word(component+0x20u)==0xffffffffu){
            if(stats_)++stats_->list_allocations;const auto datum=native_tu15_data_array_allocate(memory_,pool),entries=word(pool+0x44u);
            word(0x20d963u,entries+(datum&0xffffu)*64u+2u,0u,2);word(0x20d96au,component+0x20u,datum);
        }
        const auto datum=word(component+0x20u),entries=word(pool+0x44u),row=entries+(datum&0xffffu)*64u;
        const auto count=signed_short(word(row+2u,2));word(0x20d985u,row+std::uint32_t(count)*4u+4u,constraint);
        word(0x20d98bu,row+2u,word(row+2u,2)+1u,2);
    }
    void run(std::uint32_t datum){ //22D460
        if(stats_)++stats_->calls;
        const auto manager=word(0x52be5cu),components=word(manager+0x44u),component=components+(datum&0xffffu)*160u;
        if(!(word(component+4u)&0x800u)){if(stats_)++stats_->flag_skips;return;}
        if(!native_tu15_component_has_active_body(memory_,{component},stats_)){if(stats_)++stats_->inactive_skips;return;}
        const auto object=word(component+8u),pool=word(0x4ecbd0u),headers=word(pool+0x44u),object_data=word(headers+(object&0xffffu)*12u+8u),
            object_definition=word(object_data),tags=word(0x4f0414u),object_tag=word(tags+(object_definition&0xffffu)*16u+8u),
            model_datum=word(object_tag+0x38u),model=word(tags+(model_datum&0xffffu)*16u+8u),physics_datum=word(model+0x24u),physics=word(tags+(physics_datum&0xffffu)*16u+8u);
        auto count=word(component+0x80u);if(!less(0,count)){if(stats_)++stats_->empty_banks;return;}
        if(!scratch_.request.identity)throw std::runtime_error("Native TU1.5 model constraint activation requires64 caller-owned request DATA bytes");
        std::uint32_t ordinal{},offset{};
        do{
            if(stats_)++stats_->rows;const auto entries=word(component+0x7cu),row=entries+offset;
            if(word(row+4u)==0xffffffffu){
                const auto type=std::uint16_t(word(row,2)),index=std::uint16_t(word(row+2u,2));const auto block=definition(physics,type);
                const auto index_signed=signed_short(index);const auto block_count=word(block[0]);std::uint32_t shape{};
                if(index_signed<std::bit_cast<std::int32_t>(block_count))shape=word(block[0]+4u)+std::uint32_t(index_signed)*block[1];
                const auto edge_index=signed_short(word(shape+0x70u,2));
                if(edge_index!=-1){
                    const auto edges=word(physics+0x34u),edge=edges+std::uint32_t(edge_index)*24u,links=word(edge+8u);bool disabled{};
                    if(less(0,links)){
                        const auto bank=word(edge+0xcu);std::uint32_t n{};
                        do{
                            const auto link=bank+n*12u;
                            if(word(link,2)==type&&word(link+2u,2)==index){disabled=(word(link+4u,1)&2u)!=0;break;}
                            ++n;
                        }while(less(n,links));
                    }
                    if(!disabled){
                        word(0x22d5bbu,scratch_.request.identity+0xcu,0xffffu,2);word(0x22d5c0u,scratch_.request.identity+0x18u,0xffffu,2);
                        const auto slots=body_slots(component,ordinal);
                        const auto frame=native_tu15_model_constraint_body_frame(memory_,{component},slots[0],fp_,stats_);const auto second_node=std::uint16_t(word(edge+2u,2)),first_node=std::uint16_t(word(edge,2));
                        request(datum,slots,first_node,second_node,type,index,frame);require(services_.find_existing,0x1dd710u);
                        auto constraint=services_.find_existing({component},scratch_.request,0u,fp_);
                        if(constraint==0xffffffffu){
                            const auto measured=native_tu15_model_constraint_reference_distance(memory_,scratch_.request.identity+0x1cu,1u,fp_,stats_),extent=scaled_extent(measured,word(0x46a190u));
                            require(services_.allow_creation,0x1dd7b0u);
                            if(!services_.allow_creation({component},0xffffffffu,extent,fp_)){if(stats_)++stats_->creation_refusals;}
                            else{
                                require(services_.create,0x22cea0u);constraint=services_.create(scratch_.request,1u,fp_);if(stats_)++stats_->created;
                                append(component,constraint);
                            }
                        }else if(stats_)++stats_->existing;
                        if(constraint!=0xffffffffu){
                            const auto pool=word(0x52d68cu),bank=word(pool+0x44u),instance=bank+(constraint&0xffffu)*160u,current_entries=word(component+0x7cu);
                            word(0x22d6a2u,current_entries+offset+4u,constraint);word(0x22d6a6u,instance+8u,word(instance+8u,2)+1u,2);if(stats_)++stats_->published;
                        }
                    }
                }
            }
            count=word(component+0x80u);++ordinal;offset+=12u;
        }while(less(ordinal,count));
    }
};
}
NativeTu15ModelConstraintFrame native_tu15_model_constraint_body_frame(const SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsComponent component,std::uint32_t slot,NativeTu15PhysicsFrameFp&fp,NativeTu15ModelConstraintUpdateStats*stats){
    if(stats)++stats->body_frame_calls;
    const auto entries=read(memory,component.identity+0x70u),body=read(memory,entries+slot*0x60u+0x40u),motion=read(memory,body+0x3cu);
    std::array<NativeTu15GeometryVector,4>columns;columns[0]=vector(memory,motion+0x80u);
    const auto flags=read(memory,component.identity+4u);
    columns[1]=vector(memory,motion+0x90u);columns[2]=vector(memory,motion+0xa0u);columns[3]=vector(memory,motion+0xb0u);
    if(flags&0x40u){
        if(stats)++stats->frame_compositions;FrameComposition composition(memory,component.identity+0x30u,columns);
        native_tu15_compose_affine_geometry(composition,fp);columns=composition.columns;
    }
    NativeTu15ModelConstraintFrame out;out.scale=read(memory,0x46a0d0u);
    for(unsigned n=0;n<3;++n){out.forward[n]=columns[0][n];out.side[n]=columns[1][n];out.up[n]=columns[2][n];out.position[n]=columns[3][n];}
    return out;
}
std::uint32_t native_tu15_model_constraint_reference_distance(const SourcePhysicsContactMemory&memory,
 std::uint32_t position,std::uint32_t mode,NativeTu15PhysicsFrameFp&fp,NativeTu15ModelConstraintUpdateStats*stats){
    if(mode>3u)throw std::runtime_error("Native TU1.5 model constraint reference distance requires an authenticated mode0..3");
    if(stats)++stats->reference_distance_calls;SimdEnvironment environment(fp);
    auto minimum=scalar(read(memory,0x455dc4u));const auto pool=read(memory,0x4f54f4u);std::uint32_t index=0xffffffffu;
    for(;;){
        ++index;if(std::bit_cast<std::int32_t>(index)<0)break;
        const auto count=read(memory,pool+0x38u);if(!less(index,count))break;
        const auto bitmap=read(memory,pool+0x48u);
        while(!(read(memory,bitmap+std::uint32_t(std::bit_cast<std::int32_t>(index)>>5)*4u)&(1u<<(index&31u)))){
            ++index;if(!less(index,count))return bits(minimum);
        }
        if(index==0xffffffffu)break;
        const auto stride=read(memory,pool+0x24u),entries=read(memory,pool+0x44u),record=entries+stride*index;
        if(!record)break;const auto controller=std::uint16_t(read(memory,record+0x28u,2));if(controller==0xffffu)continue;
        const auto slot=signed_short(controller);std::uint32_t point{};
        if(slot!=-1&&read(memory,0x474c24u,2)!=0xffffu)point=0x4f64a4u+std::uint32_t(slot)*0x358u+0xb8u;
        const auto px=scalar(read(memory,point)),py=scalar(read(memory,point+4u)),pz=scalar(read(memory,point+8u));
        const auto x=subtract(px,scalar(read(memory,position))),y=subtract(py,scalar(read(memory,position+4u))),z=subtract(pz,scalar(read(memory,position+8u)));
        if(read(memory,0x231038u+mode*4u)!=0x23100bu)
            throw std::runtime_error("Native TU1.5 model constraint reference-distance jump table differs from its authority");
        const auto xx=multiply(x,x),zz=multiply(z,z),zx=add(zz,xx),yy=multiply(y,y),distance=add(zx,yy);
        if(stats)++stats->reference_distance_points;if(!greater(distance,minimum))minimum=distance;
    }
    return bits(minimum);
}
bool native_tu15_component_has_active_body(const SourcePhysicsContactMemory&memory,NativeTu15PhysicsComponent component,
 NativeTu15ModelConstraintUpdateStats*stats){
    if(stats)++stats->active_queries;auto count=read(memory,component.identity+0x74u);if(!less(0,count))return false;
    std::uint32_t index{},offset{};
    do{
        if(stats)++stats->active_body_queries;const auto entries=read(memory,component.identity+0x70u),body=read(memory,entries+offset+0x40u),island=read(memory,body+0x44u);
        if(island&&read(memory,island+0x29u,1))return true;
        count=read(memory,component.identity+0x74u);++index;offset+=0x60u;
    }while(less(index,count));return false;
}
void native_tu15_update_body_constraints(SourcePhysicsContactMemory&memory,std::uint32_t datum,
 NativeTu15ModelConstraintScratch scratch,NativeTu15PhysicsFrameFp&fp,const NativeTu15ModelConstraintServices&services,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ModelConstraintUpdateStats*stats){Update(memory,observer,services,fp,scratch,stats).run(datum);}
} // namespace halo2
