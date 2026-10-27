#include "halo2/native_tu15_contact_producer.hpp"
#include "halo2/native_tu15_contact_buffers.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Vector=NativeTu15ContactRecordVector;
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class F>void require(const F&f,const char*name){if(!f)throw std::runtime_error(name);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
    Vector vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 contact producer requires aligned vector storage");std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[n*4u+k])<<(k*8u);return v;}
    void bytes(std::uint32_t site,std::uint32_t p,std::span<const std::byte>b,bool*committed=nullptr){SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(committed)*committed=true;if(observer_)observer_(event);}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4,bool*committed=nullptr){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));bytes(site,p,std::span(b).first(width),committed);}
    void vector(std::uint32_t site,std::uint32_t p,const Vector&v){if(p&15u)throw std::runtime_error("Native TU1.5 contact producer requires aligned vector storage");std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)b[n*4+k]=std::byte(v[n]>>(k*8u));bytes(site,p,b);}
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void scalar_pop(std::uint32_t site,std::uint32_t p){alignas(16)std::array<std::byte,108>prior{};std::uint32_t bits;asm volatile("fnsave %0\n\tfrstor %0\n\tfstps %1":"=m"(prior),"=m"(bits)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");bool committed=false;try{word(site,p,bits,4,&committed);}catch(...){if(!committed)asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_simd_;bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),host_simd_(_mm_getcsr()){
        if(depth>6)throw std::runtime_error("Native TU1.5 contact producer requires its overfull-stack owner");
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact producer requires its unsupported FP owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_simd_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact producer requires its dependency FP owner");const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto value=f();resume();return value;}}catch(...){resume();throw;}}
    // Unwinding does not resume the interrupted numerical expression. Publish
    // its current FP prefix once, retain genuine cleanup-service FP changes,
    // and let the outer destructor restore the unrelated host context.
    template<class F>void cleanup(F&&f){if(active_)publish();f();}
};
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
void subtract(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
std::uint16_t compare(std::uint32_t bits){std::uint16_t status;asm volatile("fcoms %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
// The selected record's scalar20 remains live through every comparison,
// including the original redundant comparison with the first candidate.
struct SelectedRecord {std::uint32_t current,selected;};
SelectedRecord select_minimum(Memory&m,std::uint32_t header,std::uint32_t index,std::uint32_t cursor,std::uint32_t count){
    const auto data=m.word(header);const auto current=data+cursor;push(m.word(current+0x20u));auto selected=index,scan=index;
    const auto candidate=[&](std::uint32_t p,std::uint32_t n){if(!(compare(m.word(p))&0x4100u)){discard();selected=n;push(m.word(p));}};
    if(!less(count-scan,4u)){auto p=current+0xa0u;do{candidate(p-0x80u,scan);candidate(p,scan+1u);candidate(p+0x80u,scan+2u);candidate(p+0x100u,scan+3u);scan+=4u;p+=0x200u;}while(less(scan,count-3u));}
    if(less(scan,count)){auto p=m.word(header)+scan*128u+0x20u;do{candidate(p,scan);++scan;p+=128u;}while(less(scan,count));}
    const auto selected_data=m.word(header);discard();return{current,selected_data+selected*128u};
}
// This producer's selection swap has a different access/store order from
//304130 compaction. Only its actual verified fields move; gaps stay in place.
void swap_selected(Memory&m,SelectedRecord r){
    const auto current00=m.vector(r.current);const auto v50=m.vector(r.selected+0x50u);push(m.word(r.selected+0x20u));const auto v60=m.vector(r.selected+0x60u);push(m.word(r.selected+0x24u));
    const auto w78=m.word(r.selected+0x78u);asm volatile("fxch %%st(1)":::"memory","st","st(1)");
    const auto v00=m.vector(r.selected),v10=m.vector(r.selected+0x10u),v30=m.vector(r.selected+0x30u),v40=m.vector(r.selected+0x40u);const auto w70=m.word(r.selected+0x70u),w74=m.word(r.selected+0x74u);
    m.vector(0x30479c,r.selected,current00);m.vector(0x3047a3,r.selected+0x10u,m.vector(r.current+0x10u));
    const auto b7c=m.word(r.selected+0x7cu,1),b7d=m.word(r.selected+0x7du,1);
    m.word(0x3047c5,r.selected+0x20u,m.word(r.current+0x20u));m.word(0x3047cb,r.selected+0x24u,m.word(r.current+0x24u));
    m.vector(0x3047d2,r.selected+0x30u,m.vector(r.current+0x30u));m.vector(0x3047da,r.selected+0x40u,m.vector(r.current+0x40u));m.vector(0x3047e2,r.selected+0x50u,m.vector(r.current+0x50u));m.vector(0x3047ea,r.selected+0x60u,m.vector(r.current+0x60u));
    m.word(0x3047f1,r.selected+0x70u,m.word(r.current+0x70u));m.word(0x3047f7,r.selected+0x74u,m.word(r.current+0x74u));m.word(0x3047fd,r.selected+0x78u,m.word(r.current+0x78u));m.word(0x304803,r.selected+0x7cu,m.word(r.current+0x7cu,1),1);m.word(0x304809,r.selected+0x7du,m.word(r.current+0x7du,1),1);
    m.scalar_pop(0x30480c,r.current+0x20u);m.scalar_pop(0x30481d,r.current+0x24u);
    m.vector(0x304820,r.current,v00);m.vector(0x304823,r.current+0x10u,v10);m.vector(0x304827,r.current+0x30u,v30);m.vector(0x30482b,r.current+0x40u,v40);m.vector(0x30482f,r.current+0x50u,v50);m.vector(0x304833,r.current+0x60u,v60);
    m.word(0x304837,r.current+0x78u,w78);m.word(0x304841,r.current+0x70u,w70);m.word(0x304848,r.current+0x74u,w74);m.word(0x30484b,r.current+0x7cu,b7c,1);m.word(0x30484e,r.current+0x7du,b7d,1);
}
void profile(Memory&m,bool ending,const NativeTu15ContactProducerServices&s){const auto cursor=m.word(0x48c974u);if(cursor>=m.word(0x48c978u))return;m.word(ending?0x304a42u:0x30438du,cursor,ending?0x41c394u:0x41d05cu);require(s.timestamp_counter,"Native TU1.5 contact producer requires its actual timestamp owner");const auto ticks=s.timestamp_counter();m.word(ending?0x304a52u:0x30439du,cursor+4u,std::uint32_t(ticks));m.word(ending?0x304a58u:0x3043a3u,0x48c974u,cursor+12u);}
#endif
}
void native_tu15_produce_contacts(SourcePhysicsContactMemory&memory,NativeTu15ContactProducer producer,NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15ContactContext context,NativeTu15ContactOutput output,NativeTu15ContactProducerScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ContactProducerServices&s,const SourcePhysicsContactRecordObserver&observer){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 contact producer requires aligned retained packets");Memory m(memory,observer);Environment environment(fp,depth);const auto p=scratch.identity,agent=producer.identity;const NativeTu15ContactRecordArray records{p+0x110u};NativeTu15ContactRecordMemory record_storage(memory,observer);
    NativeTu15ArrayGrowthServices record_cleanup;record_cleanup.release=[&](const auto&request,auto&context){require(s.release_records,"Native TU1.5 contact producer unwind requires actual record release owner");s.release_records(request.receiver,{request.owner},request.data,request.bytes,request.memory_class,context);};
    // Actual FuncInfo46C084: state1 destroys components, then state0 releases
    // owned records. Construction order gives that same native unwind order.
    native_contact_detail::UnwindGuard array_cleanup([&]{environment.cleanup([&]{native_tu15_release_owned_contact_records(memory,records,fp,record_cleanup);});});
    native_contact_detail::UnwindGuard component_cleanup([&]{environment.cleanup([&]{require(s.destroy_component_buffer,"Native TU1.5 contact producer unwind requires actual component destructor");s.destroy_component_buffer(p+0x6cu,fp);});});
    profile(m,false,s);
    const auto transform=m.word(second.identity+8u);const auto shape=m.word(second.identity);m.word(0x3043b1,p+0x38u,transform);const auto container=m.word(shape+0xcu);const auto key=m.word(second.identity+4u);m.word(0x3043c4,p+0x30u,container);m.word(0x3043d0,p+0x34u,key);
    environment.dependency([&]{require(s.update_candidates,"Native TU1.5 contact producer requires actual2F36F0 candidate owner");s.update_candidates(producer,first,{p+0x30u},context,0x2f3140u,fp);});
    m.word(0x3043e9,records.identity,p+0x120u);m.word(0x3043f0,records.identity+4u,0);m.word(0x3043f7,records.identity+8u,0x80000004u);
    const auto initial_components=m.word(agent+0x10u);array_cleanup.arm(); // Actual30440A, after count read.
    environment.dependency([&]{require(s.initialize_component_buffer,"Native TU1.5 contact producer requires actual303C70 buffer owner");s.initialize_component_buffer(p+0x6cu,initial_components,fp);});
    const auto components=m.word(agent+0x10u),second_transform=m.word(second.identity+8u),resolved_container=m.word(p+0x30u);component_cleanup.arm(); // Actual304422, after all three post-constructor reads.
    m.word(0x30442a,p+0x70u,components);m.word(0x30442e,p+0x4cu,second.identity);m.word(0x304432,p+0x48u,second_transform);
    if(less(0,components)){std::uint32_t index{},entry_offset{},component_offset{};do{
        const auto entry=m.word(agent+0xcu)+entry_offset;
        // Actual30BAD0 is only MOV EAX,48CDC4; RET. This is an opaque
        // data identity, with no service, backing read or FP side effect.
        constexpr NativeTu15ContactChild invalid{0x48cdc4u};
        if(m.word(entry+8u)!=invalid.identity){
            const NativeTu15ContactChild collection{m.word(resolved_container+0xcu)};const auto storage=m.word(p+0x6cu)+component_offset;const auto resolve_table=m.word(collection.identity);const auto component_key=m.word(entry);const NativeTu15ContactVirtualOwner resolve_owner{m.word(resolve_table+0x34u)};
            const auto component=environment.dependency([&]{require(s.resolve_component,"Native TU1.5 contact producer requires actual child virtual34 owner");return s.resolve_component(collection,resolve_owner,component_key,storage,fp);});
            const auto first_component=NativeTu15ContactComponent{m.word(first.identity)};const auto child_key=m.word(entry);m.word(0x30448a,p+0x40u,component.identity);m.word(0x30448e,p+0x44u,child_key);
            const auto component_owner=m.owner(component.identity,0x14u);const auto child_class=environment.dependency([&]{require(s.component_class,"Native TU1.5 contact producer requires actual component class owner");return s.component_class(component,component_owner,fp);});
            const auto first_owner=m.owner(first_component.identity,0x14u);const auto first_class=environment.dependency([&]{return s.component_class(first_component,first_owner,fp);});
            const auto pair_context=m.word(context.identity);const auto pair_owner=m.word(0x46e46cu);const auto result=environment.dependency([&]{require(s.test_pair,"Native TU1.5 contact producer requires actual46E46C pair owner");return s.test_pair(pair_owner,p+0x2fu,pair_context,first_class,child_class,fp);});
            if(m.word(result,1)){
                const NativeTu15ContactChild child{m.word(entry+8u)};const auto query_owner=m.owner(child.identity,0x20u);environment.dependency([&]{require(s.query_contact,"Native TU1.5 contact producer requires actual child virtual20 owner");s.query_contact(child,query_owner,p+0x18u,first,{p+0x40u},context,p+0x90u,fp);});
                environment.dependency([&]{require(s.append_record,"Native TU1.5 contact producer requires actual303EE0 append owner");s.append_record(records,fp);});
                auto count=m.word(records.identity+4u),data=m.word(records.identity);m.word(0x3044fd,data+count*128u-0x10u,child.identity);
                count=m.word(records.identity+4u);data=m.word(records.identity);const auto record_key=m.word(entry);m.word(0x304514,data+count*128u-0xcu,component.identity);
                count=m.word(records.identity+4u);data=m.word(records.identity);m.word(0x304529,data+count*128u-8u,record_key);
                count=m.word(records.identity+4u);const auto valid=m.word(p+0x18u,1);data=m.word(records.identity);m.word(0x304542,data+count*128u-3u,valid,1);
                if(m.word(p+0x18u,1)){
                    count=m.word(records.identity+4u);data=m.word(records.identity);const auto v00=m.vector(p+0x90u);const auto record=data+count*128u-128u;m.vector(0x30456f,record,v00);m.vector(0x30457a,record+0x10u,m.vector(p+0xa0u));m.word(0x304585,record+0x20u,m.word(p+0xb0u));m.word(0x30458f,record+0x24u,m.word(p+0xb4u));m.vector(0x30459a,record+0x30u,m.vector(p+0xc0u));m.vector(0x3045a6,record+0x40u,m.vector(p+0xd0u));
                    const auto basis=m.word(p+0x48u)+0x20u;NativeTu15GeometryMemory geometry(memory,p+0x80u,basis,p+0xd0u,observer);environment.dependency([&]{native_tu15_transform_transposed_direction(geometry,fp);});
                    push(m.word(p+0xb0u));count=m.word(records.identity+4u);data=m.word(records.identity);const auto source=m.vector(p+0xc0u);const auto packet=data+count*128u-0x30u;m.vector(0x3045ea,packet,source);m.vector(0x3045f5,packet+0x10u,m.vector(p+0x80u));m.scalar_pop(0x3045f9,packet+0xcu);
                    const auto type=m.word(child.identity+0x15u,1);count=m.word(records.identity+4u);data=m.word(records.identity);m.word(type==1u||type==2u?0x304635u:0x30461cu,data+count*128u-4u,type==1u||type==2u,1);
                }
            }else{
                const NativeTu15ContactChild child{m.word(entry+8u)};const auto owner=m.owner(child.identity,0x10u);environment.dependency([&]{require(s.process_rejected_pair,"Native TU1.5 contact producer requires actual child virtual10 owner");s.process_rejected_pair(child,owner,first,{p+0x40u},context,output,fp);});
            }
        }
        ++index;entry_offset+=12u;component_offset+=256u;
    }while(less(index,m.word(agent+0x10u)));}
    auto active=m.word(records.identity+4u);
    for(std::uint32_t index{},cursor{};less(index,active);++index,cursor+=128u){const auto selected=select_minimum(m,records.identity,index,cursor,active);swap_selected(m,selected);const auto data=m.word(records.identity);const auto valid=m.word(data+cursor+0x7du,1);if(valid){const NativeTu15ContactRecordSettings settings{m.word(context.identity+0xcu)};active=environment.dependency([&]{return native_tu15_filter_contact_records(record_storage,{data+cursor+0x50u},records,index+1u,active,settings,fp,depth);});}}
    const auto total=m.word(records.identity+4u);
    if(less(0,total))for(std::uint32_t record_index{},cursor{};record_index!=total;++record_index,cursor+=128u){const auto data=m.word(records.identity);auto child=m.word(data+cursor+0x70u);auto remaining=m.word(child+0x18u);if(less(0,remaining)){child+=0x40u;do{
        push(m.word(child+0x1cu));const auto actual_transform=m.word(second.identity+8u);const auto position=m.vector(child);m.vector(0x3048f3,p+0x40u,position);m.scalar_pop(0x3048f8,p+0x4cu);
        NativeTu15GeometryMemory geometry(memory,p+0x80u,actual_transform+0x20u,child+0x10u,observer);environment.dependency([&]{native_tu15_transform_transposed_direction(geometry,fp);});
        const NativeTu15ContactRecordSettings settings{m.word(context.identity+0xcu)};const auto direction=m.vector(p+0x80u);m.vector(0x30492c,p+0x50u,direction);active=environment.dependency([&]{return native_tu15_filter_contact_records(record_storage,{p+0x40u},records,0,active,settings,fp,depth);});child+=0x30u;--remaining;
    }while(remaining);}}
    if(less(0,total))for(std::uint32_t index{},cursor{};less(index,total);++index,cursor+=128u){
        const auto transform_now=m.word(second.identity+8u);m.word(0x30497a,p+0x4cu,second.identity);const auto record=m.word(records.identity)+cursor;m.word(0x304989,p+0x48u,transform_now);const auto record_key=m.word(record+0x78u),component=m.word(record+0x74u);m.word(0x304993,p+0x40u,component);m.word(0x304997,p+0x44u,record_key);const auto child=m.word(record+0x70u);
        if(less(index,active)&&m.word(record+0x7du,1)){const auto producer_value=m.word(agent+8u);environment.dependency([&]{require(s.publish_contact,"Native TU1.5 contact producer requires actual30CD00 publication owner");s.publish_contact(first,{p+0x40u},context,{record},producer_value,child+0x30u,child+0x18u,fp);});}
        const auto data=m.word(records.identity);push(m.word(data+cursor+0x20u));const auto producer_value=m.word(agent+8u);subtract(m.word(0x46a180u));m.scalar_pop(0x3049ee,p+0x24u);const auto distance=m.word(p+0x24u);environment.dependency([&]{require(s.publish_distance,"Native TU1.5 contact producer requires actual30CF40 publication owner");s.publish_distance(first,{p+0x40u},context,distance,child+0x30u,child+0x18u,output,producer_value,fp);});
    }
    profile(m,true,s);component_cleanup.disarm(); // Actual304A62 before explicit component destruction.
    environment.dependency([&]{require(s.destroy_component_buffer,"Native TU1.5 contact producer requires actual303D00 destroy owner");s.destroy_component_buffer(p+0x6cu,fp);});
    const auto capacity=m.word(records.identity+8u);array_cleanup.disarm(); // Actual304A78, after capacity read and before owned release.
    if(!less(capacity,0)){const auto allocator=m.word(0x48c938u);const auto table=m.word(allocator);const auto bytes=(capacity&0x7fffffffu)*128u;const auto data=m.word(records.identity);const NativeTu15ContactVirtualOwner owner{m.word(table+0x14u)};environment.dependency([&]{require(s.release_records,"Native TU1.5 contact producer requires actual allocator virtual14 owner");s.release_records(allocator,owner,data,bytes,0x12u,fp);});}
#else
    (void)memory;(void)producer;(void)first;(void)second;(void)context;(void)output;(void)scratch;(void)fp;(void)depth;(void)s;(void)observer;throw std::runtime_error("Exact native TU1.5 contact producer backend unavailable");
#endif
}
} // namespace halo2
