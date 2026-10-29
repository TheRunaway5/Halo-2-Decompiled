#include "halo2/native_tu15_data_array_iteration.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
    if(!observe){memory.write(address,bytes);return;}
    SourcePhysicsContactRecordStore event{site,address,{}, {bytes.begin(),bytes.end()}};
    event.before.resize(4);memory.read(address,event.before);memory.write(address,bytes);observe(event);
}
struct RetainedIterator {
    SourcePhysicsContactMemory &memory;std::uint32_t address;const SourcePhysicsContactRecordObserver &observe;
    std::uint32_t pool()const{return read(memory,address+4u);}
    std::uint32_t cursor()const{return read(memory,address+0xcu);}
    void cursor(std::uint32_t value,bool found){publish(memory,observe,found?0x6ca5f:0x6ca7b,address+0xcu,value);}
    void datum(std::uint32_t value,bool found){publish(memory,observe,found?0x6ca6e:0x6ca84,address+8u,value);}
    void current(std::uint32_t value,bool found){publish(memory,observe,found?0x6ca71:0x6ca8b,address,value);}
};
struct LocalIterator {
    NativeTu15DataArrayIterator &value;
    std::uint32_t pool()const{return value.pool;}
    std::uint32_t cursor()const{return value.cursor;}
    void cursor(std::uint32_t cursor,bool){value.cursor=cursor;}
    void datum(std::uint32_t datum,bool){value.datum=datum;}
    void current(std::uint32_t current,bool){value.current=current;}
};
template<class Iterator> bool advance(SourcePhysicsContactMemory &memory,Iterator &state) {
    const auto first=state.cursor()+1u,pool=state.pool();
    const auto index=native_tu15_data_array_next(memory,pool,std::bit_cast<std::int32_t>(first));
    if(index!=0xffffffffu) {
        const auto stride=read(memory,pool+0x24u),rows=read(memory,pool+0x44u),row=rows+index*stride;
        state.cursor(index,true);
        const auto salt=read(memory,row,2);
        state.datum((salt<<16u)|index,true);state.current(row,true);return row!=0;
    }
    const auto capacity=read(memory,pool+0x20u);state.cursor(capacity,false);
    state.datum(0xffffffffu,false);state.current(0,false);return false;
}
}
std::uint32_t native_tu15_data_array_next(const SourcePhysicsContactMemory &memory,
    std::uint32_t pool,std::int32_t first) {
    if(first<0)return 0xffffffffu;
    const auto limit=std::bit_cast<std::int32_t>(read(memory,pool+0x38u));
    if(first>=limit)return 0xffffffffu;
    const auto bitmap=read(memory,pool+0x48u);
    for(auto index=std::uint32_t(first);std::bit_cast<std::int32_t>(index)<limit;++index) {
        if(read(memory,bitmap+(index>>5u)*4u)&(1u<<(index&31u)))return index;
    }
    return 0xffffffffu;
}
bool native_tu15_bit_array_test(const SourcePhysicsContactMemory &memory,
    std::uint32_t words,std::int32_t index) {
    const auto address=words+std::uint32_t(index>>5)*4u;
    return (read(memory,address)&(1u<<(std::uint32_t(index)&31u)))!=0;
}
bool native_tu15_data_array_advance(SourcePhysicsContactMemory &memory,
    std::uint32_t iterator,const SourcePhysicsContactRecordObserver &observe) {
    RetainedIterator state{memory,iterator,observe};return advance(memory,state);
}
bool native_tu15_data_array_advance(SourcePhysicsContactMemory &memory,NativeTu15DataArrayIterator &iterator) {
    LocalIterator state{iterator};return advance(memory,state);
}
}
