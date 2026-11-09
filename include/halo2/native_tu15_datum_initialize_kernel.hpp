#pragma once
#include <cstdint>

namespace halo2 {
struct NativeTu15DatumRecordInitialization {
    std::uint32_t pool{},record{};
    std::uint32_t clear_bytes{},word_count{},tail_count{},tail_initial{},clear_at{};
    std::uint32_t salt{},increment_old{},incremented{},salt_check{};
};
// Shared record clearing and generation publication for actual16E6A0 and
// actual16E860. Phase supplies their semantic storage boundaries; Access owns
// the distinct ABI and observer policy. Data retains the live loop counters
// so a completed repeated store can publish its advanced continuation.
template<class Phase,class Access,class Data>
void native_tu15_initialize_datum_record(Access&memory,Data&s,bool backwards=false) {
    const auto publish=[&](Phase phase,std::uint32_t at,std::uint32_t value,unsigned bytes=4){
        memory.write(phase,s,at,value,bytes,0,false);memory.complete(phase,s);
    };
    s.clear_bytes=memory.read(Phase::clear_stride,s,s.pool+0x24u,4);
    s.word_count=s.clear_bytes>>2u;s.clear_at=s.record;
    while(s.word_count) {
        memory.write(Phase::clear_word,s,s.clear_at,0,4,0,false);
        s.clear_at+=backwards?0xfffffffcu:4u;--s.word_count;memory.complete(Phase::clear_word,s);
    }
    s.tail_initial=s.tail_count=s.clear_bytes&3u;
    while(s.tail_count) {
        memory.write(Phase::clear_byte,s,s.clear_at,0,1,0,false);
        s.clear_at+=backwards?0xffffffffu:1u;--s.tail_count;memory.complete(Phase::clear_byte,s);
    }
    s.salt=memory.read(Phase::salt,s,s.pool+0x40u,2);
    publish(Phase::salt_record,s.record,s.salt,2);
    s.increment_old=memory.read(Phase::salt_increment,s,s.pool+0x40u,2);
    s.incremented=(s.increment_old+1u)&0xffffu;
    memory.write(Phase::salt_increment,s,s.pool+0x40u,s.incremented,2,s.increment_old,true);
    memory.complete(Phase::salt_increment,s);
    s.salt_check=memory.read(Phase::salt_check,s,s.pool+0x40u,2);
    if(s.salt_check==0xffffu)publish(Phase::salt_reset,s.pool+0x40u,0x8000u,2);
}
} // namespace halo2
