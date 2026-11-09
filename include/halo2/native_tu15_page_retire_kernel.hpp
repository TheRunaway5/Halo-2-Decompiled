#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15PageRetirePhase {pool,rows,callback,previous,previous_pool,previous_rows,next_for_previous,
    previous_store,next,next_pool,next_rows,previous_for_next,next_store,next_for_head,head_store,
    previous_for_tail,tail_store,delete_pool,tail_erase};
struct NativeTu15PageRetirement {
    std::uint32_t owner{},datum{},pool{},rows{},record{},callback{},previous{},next{},link_pool{},link_rows{},
        fresh_next{},fresh_previous{},erase_pool{};
};
struct NativeTu15PageRetireResume {std::uint32_t owner{},datum{},record{};};
// Shared140310 DATA owner. The real callback may alter retained storage and
// its saved caller frame; acquire the resumed owner/datum/record explicitly.
template<class Access,class Services>void native_tu15_retire_page(Access &memory,NativeTu15PageRetirement &s,Services &services){
    using P=NativeTu15PageRetirePhase;
    s.pool=memory.read(P::pool,s,s.owner+0x64u);s.rows=memory.read(P::rows,s,s.pool+0x44u);
    s.record=s.rows+(s.datum&0xffffu)*24u;s.callback=memory.read(P::callback,s,s.owner+0x20u);
    if(s.callback){const auto resumed=services.callback(s.callback,s.datum,s);s.owner=resumed.owner;s.datum=resumed.datum;s.record=resumed.record;}
    s.previous=memory.read(P::previous,s,s.record+0x10u);
    if(s.previous!=0xffffffffu){
        s.link_pool=memory.read(P::previous_pool,s,s.owner+0x64u);s.link_rows=memory.read(P::previous_rows,s,s.link_pool+0x44u);
        s.fresh_next=memory.read(P::next_for_previous,s,s.record+0xcu);
        memory.write(P::previous_store,s,s.link_rows+(s.previous&0xffffu)*24u+0xcu,s.fresh_next);
    }else{
        s.fresh_next=memory.read(P::next_for_head,s,s.record+0xcu);memory.write(P::head_store,s,s.owner+0x3cu,s.fresh_next);
    }
    s.next=memory.read(P::next,s,s.record+0xcu);
    if(s.next!=0xffffffffu){
        s.link_pool=memory.read(P::next_pool,s,s.owner+0x64u);s.link_rows=memory.read(P::next_rows,s,s.link_pool+0x44u);
        s.fresh_previous=memory.read(P::previous_for_next,s,s.record+0x10u);
        memory.write(P::next_store,s,s.link_rows+(s.next&0xffffu)*24u+0x10u,s.fresh_previous);
    }else{
        s.fresh_previous=memory.read(P::previous_for_tail,s,s.record+0x10u);memory.write(P::tail_store,s,s.owner+0x40u,s.fresh_previous);
    }
    s.erase_pool=memory.read(P::delete_pool,s,s.owner+0x64u);memory.prepare(P::tail_erase,s);services.erase(s.erase_pool,s.datum,s);
}
} // namespace halo2
