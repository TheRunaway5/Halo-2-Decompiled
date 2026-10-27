#include "halo2/native_tu15_contact_records.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Record=NativeTu15ContactRecord;using Storage=NativeTu15ContactRecordStorage;
using Vector=NativeTu15ContactRecordVector;using Phase=NativeTu15ContactRecordPublication;
Record offset(Record r,std::uint32_t n){return{r.identity+n};}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned host_simd_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth,unsigned temporaries):fp_(fp),host_simd_(_mm_getcsr()){
        if(depth+temporaries>8u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 contact records require their unsupported FP owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_simd_);}
};
__m128 lanes(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t dot3(__m128 product){const auto y=_mm_shuffle_ps(product,product,0x55);const auto yx=add_scalar(y,product);const auto z=_mm_shuffle_ps(product,product,0xaa);return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(add_scalar(z,yx)));}
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
bool parity(std::uint16_t value){return std::popcount(static_cast<unsigned>(value))%2u==0;}
std::uint8_t test(Storage&s,Record a,Record b,NativeTu15ContactRecordSettings settings){
    push(s.word(b,0x0c));
    const auto first=compare_pop(s.comparison_limit());
    if(!parity((first>>8u)&5u))return 1;
    const auto av=lanes(s.vector(a,0x10));const auto bv=lanes(s.vector(b,0x10));
    push(dot3(multiply(av,bv)));
    const auto second=compare_pop(s.setting(settings,0x10));
    if(parity((second>>8u)&5u))return 1;
    const auto ap=lanes(s.vector(a,0));const auto bp=lanes(s.vector(b,0));
    push(dot3(multiply(subtract(bp,ap),av)));
    const auto third=compare_pop(s.setting(settings,0x0c));
    return parity((third>>8u)&0x41u)?1:0;
}
void publish_pop(Storage&s,Record destination,unsigned field,Phase phase){
    alignas(16)std::array<std::byte,108>prior{};std::uint32_t value;
    asm volatile("fnsave %0\n\tfrstor %0\n\tfstps %1":"=m"(prior),"=m"(value)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    bool committed=false;
    try{s.word(destination,field,value,phase,[&]{committed=true;});}
    catch(...){if(!committed)asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
    if(!committed)throw std::runtime_error("Native TU1.5 scalar contact publication did not acknowledge its commit");
}
void swap_records(Storage&s,Record selected,Record tail){
    const auto v50=s.vector(selected,0x50);push(s.word(selected,0x20));
    const auto v60=s.vector(selected,0x60);push(s.word(selected,0x24));
    const auto w74=s.word(selected,0x74);asm volatile("fxch %%st(1)":::"memory","st","st(1)");
    const auto w70=s.word(selected,0x70);const auto t00=s.vector(tail,0);
    const auto w78=s.word(selected,0x78);const auto v00=s.vector(selected,0);
    const auto v10=s.vector(selected,0x10);const auto v30=s.vector(selected,0x30);const auto v40=s.vector(selected,0x40);
    s.vector(selected,0,t00,Phase::selected);s.vector(selected,0x10,s.vector(tail,0x10),Phase::selected);
    s.word(selected,0x20,s.word(tail,0x20),Phase::selected);
    const auto b7c=s.byte(selected,0x7c),b7d=s.byte(selected,0x7d);
    s.word(selected,0x24,s.word(tail,0x24),Phase::selected);
    for(unsigned field:{0x30u,0x40u,0x50u,0x60u})s.vector(selected,field,s.vector(tail,field),Phase::selected);
    for(unsigned field:{0x70u,0x74u,0x78u})s.word(selected,field,s.word(tail,field),Phase::selected);
    s.byte(selected,0x7c,s.byte(tail,0x7c),Phase::selected);s.byte(selected,0x7d,s.byte(tail,0x7d),Phase::selected);
    s.vector(tail,0x10,v10,Phase::tail);publish_pop(s,tail,0x20,Phase::tail);
    s.vector(tail,0x30,v30,Phase::tail);publish_pop(s,tail,0x24,Phase::tail);
    s.vector(tail,0x40,v40,Phase::tail);s.vector(tail,0,v00,Phase::tail);
    s.vector(tail,0x50,v50,Phase::tail);s.vector(tail,0x60,v60,Phase::tail);
    s.word(tail,0x70,w70,Phase::tail);s.word(tail,0x74,w74,Phase::tail);s.word(tail,0x78,w78,Phase::tail);
    s.byte(tail,0x7c,b7c,Phase::tail);s.byte(tail,0x7d,b7d,Phase::tail);
}
#endif
std::uint32_t site(unsigned field,Phase phase){
    switch(phase){
    case Phase::copy:switch(field){case 0:return 0x303ba9;case 0x10:return 0x303bb0;case 0x20:return 0x303bb7;case 0x24:return 0x303bbd;case 0x30:return 0x303bc4;case 0x40:return 0x303bcc;case 0x50:return 0x303bd4;case 0x60:return 0x303bdc;case 0x70:return 0x303be3;case 0x74:return 0x303be9;case 0x78:return 0x303bef;case 0x7c:return 0x303bf5;case 0x7d:return 0x303bfb;}break;
    case Phase::selected:switch(field){case 0:return 0x3041cc;case 0x10:return 0x3041d5;case 0x20:return 0x3041e5;case 0x24:return 0x304223;case 0x30:return 0x30422c;case 0x40:return 0x304236;case 0x50:return 0x304240;case 0x60:return 0x30424a;case 0x70:return 0x30426d;case 0x74:return 0x30427b;case 0x78:return 0x304289;case 0x7c:return 0x304297;case 0x7d:return 0x3042a5;}break;
    case Phase::tail:switch(field){case 0:return 0x3042d1;case 0x10:return 0x3042ad;case 0x20:return 0x3042b4;case 0x24:return 0x3042c1;case 0x30:return 0x3042ba;case 0x40:return 0x3042c7;case 0x50:return 0x3042d6;case 0x60:return 0x3042d9;case 0x70:return 0x3042e4;case 0x74:return 0x3042f1;case 0x78:return 0x3042fe;case 0x7c:return 0x30430b;case 0x7d:return 0x304315;}break;
    case Phase::filter_false:return 0x30389f;case Phase::filter_true:return 0x3038ac;
    }
    throw std::runtime_error("Native TU1.5 contact record publication has no verified field");
}
}
Record native_tu15_copy_contact_record(Storage&s,Record destination,Record source){
    s.vector(destination,0,s.vector(source,0),Phase::copy);s.vector(destination,0x10,s.vector(source,0x10),Phase::copy);
    s.word(destination,0x20,s.word(source,0x20),Phase::copy);s.word(destination,0x24,s.word(source,0x24),Phase::copy);
    for(unsigned field:{0x30u,0x40u,0x50u,0x60u})s.vector(destination,field,s.vector(source,field),Phase::copy);
    for(unsigned field:{0x70u,0x74u,0x78u})s.word(destination,field,s.word(source,field),Phase::copy);
    s.byte(destination,0x7c,s.byte(source,0x7c),Phase::copy);s.byte(destination,0x7d,s.byte(source,0x7d),Phase::copy);return destination;
}
std::uint8_t native_tu15_test_contact_record(Storage&s,Record a,Record b,NativeTu15ContactRecordSettings settings,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth,1);return test(s,a,b,settings);
#else
    (void)s;(void)a;(void)b;(void)settings;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 contact record backend unavailable");
#endif
}
std::uint32_t native_tu15_filter_contact_records(Storage&s,Record probe,NativeTu15ContactRecordArray array,std::uint32_t index,std::uint32_t count,NativeTu15ContactRecordSettings settings,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth,2);auto cursor=index*128u,tail=count*128u;
    while(less(index,count)){const auto base=s.data(array);const auto record=offset(base,cursor);
        if(s.byte(record,0x7c)!=1||test(s,probe,offset(record,0x50),settings)){++index;cursor+=128u;}
        else{swap_records(s,record,offset(base,tail-128u));--count;tail-=128u;}
    }
    return count;
#else
    (void)s;(void)probe;(void)array;(void)index;(void)count;(void)settings;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 contact record backend unavailable");
#endif
}
NativeTu15ContactRecordMemory::NativeTu15ContactRecordMemory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
std::uint32_t NativeTu15ContactRecordMemory::read(std::uint32_t p,unsigned width)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
Vector NativeTu15ContactRecordMemory::vector(Record r,unsigned field)const{const auto p=r.identity+field;if(p&15u)throw std::runtime_error("Native TU1.5 contact records require aligned vector storage");std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[n*4u+k])<<(k*8u);return v;}
std::uint32_t NativeTu15ContactRecordMemory::word(Record r,unsigned field)const{return read(r.identity+field,4);}
std::uint8_t NativeTu15ContactRecordMemory::byte(Record r,unsigned field)const{return std::uint8_t(read(r.identity+field,1));}
std::uint32_t NativeTu15ContactRecordMemory::setting(NativeTu15ContactRecordSettings s,unsigned field)const{return read(s.identity+field,4);}
std::uint32_t NativeTu15ContactRecordMemory::comparison_limit()const{return read(0x41d018,4);}
Record NativeTu15ContactRecordMemory::data(NativeTu15ContactRecordArray a)const{return{read(a.identity,4)};}
void NativeTu15ContactRecordMemory::publish(std::uint32_t p,unsigned field,std::span<const std::byte>b,Phase phase,const std::function<void()>&committed){
    if(b.size()==16u&&(p&15u))throw std::runtime_error("Native TU1.5 contact records require aligned vector storage");SourcePhysicsContactRecordStore event{site(field,phase),p,{},{}};
    if(observer_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}
    memory_.write(p,b);if(committed)committed();if(observer_)observer_(event);
}
void NativeTu15ContactRecordMemory::vector(Record r,unsigned field,const Vector&v,Phase phase){std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)b[n*4+k]=std::byte(v[n]>>(k*8u));publish(r.identity+field,field,b,phase);}
void NativeTu15ContactRecordMemory::word(Record r,unsigned field,std::uint32_t value,Phase phase,const std::function<void()>&committed){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));publish(r.identity+field,field,b,phase,committed);}
void NativeTu15ContactRecordMemory::byte(Record r,unsigned field,std::uint8_t value,Phase phase){const std::array<std::byte,1>b{std::byte(value)};publish(r.identity+field,field,b,phase);}
void NativeTu15ContactRecordMemory::filter_result(std::uint32_t p,std::uint8_t value){byte({p},0,value,value?Phase::filter_true:Phase::filter_false);}
} // namespace halo2
