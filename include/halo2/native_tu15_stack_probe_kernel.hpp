#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15StackProbePhase {begin,small_probe,small_return_word,small_return_store,small_return,
    zero_return,large_begin,page_probe,page_compare,tail_probe,saved_counter,return_word,large_return_store,large_return};
struct NativeTu15StackProbe {
    std::uint32_t bytes{},entry_sp{},remaining{},cursor{},saved_cursor{},probe_word{},saved_counter{},return_pc{};
};
// Shared 32A660 DATA operation: touch each crossed 4K page, then transport the
// genuine return word and saved counter onto the reserved caller frame.
// The storage owner supplies actual reads/writes; no guard-page result or
// operating-system success is fabricated by this CPU kernel.
template<class Access>void native_tu15_probe_stack(Access &memory,NativeTu15StackProbe &s){
    using P=NativeTu15StackProbePhase;memory.prepare(P::begin,s);
    if(!s.bytes){memory.complete(P::zero_return,s);return;}
    s.cursor=s.entry_sp+4u-s.bytes;
    if(s.bytes<4096u){
        s.probe_word=memory.read(P::small_probe,s,s.cursor);s.return_pc=memory.read(P::small_return_word,s,s.entry_sp);
        memory.write(P::small_return_store,s,s.cursor-4u,s.return_pc);memory.complete(P::small_return,s);return;
    }
    s.saved_cursor=s.entry_sp-4u;s.cursor=s.entry_sp+4u;s.remaining=s.bytes;memory.prepare(P::large_begin,s);
    do{
        s.cursor-=4096u;s.remaining-=4096u;s.probe_word=memory.read(P::page_probe,s,s.cursor);memory.prepare(P::page_compare,s);
    }while(s.remaining>=4096u);
    s.cursor-=s.remaining;s.probe_word=memory.read(P::tail_probe,s,s.cursor);
    s.saved_counter=memory.read(P::saved_counter,s,s.saved_cursor);s.return_pc=memory.read(P::return_word,s,s.saved_cursor+4u);
    memory.write(P::large_return_store,s,s.cursor-4u,s.return_pc);memory.complete(P::large_return,s);
}
} // namespace halo2
