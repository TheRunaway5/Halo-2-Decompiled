#pragma once
#include <cstdint>

namespace halo2 {
struct NativeTu15WordCopy {
    std::uint32_t source{},destination{},remaining{},word{};
};
// Ordered streaming DWORD copy, including overlapping storage and DF. Input
// words are acquired one at a time; future inputs observe completed stores.
// Advance follows a completed write/observer boundary in the retained bank
// protocol. Physical signal/REP retirement atomicity is a separate contract.
template<class Access>
void native_tu15_copy_words(Access &memory,NativeTu15WordCopy &s,bool backwards){
    const auto delta=backwards?0xfffffffcu:4u;
    while(s.remaining){
        s.word=memory.read_word(s.source);memory.write_word(s.destination,s.word);
        s.source+=delta;s.destination+=delta;--s.remaining;memory.advanced(s);
    }
}
} // namespace halo2
