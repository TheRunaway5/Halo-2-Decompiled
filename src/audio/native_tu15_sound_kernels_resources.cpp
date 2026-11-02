// Native kernels: sound cache binding, query and chunk readiness (TU1.5 sound manager).
//
// A sound binding (0xBC-byte row of the binding table) names one chunk of a
// sound: its tag datum (+0xC), pitch range (+0x9C, signed byte), permutation
// (+0x9D, signed byte) and chunk (+0x9E, signed word) indices. Bit 1 of +4
// records that the binding holds a lock on the chunk's cache entry.
// Chunk rows (gestalt +0x44, 12 bytes) keep their cache handle at +8, -1 for
// none. Cache entries (16 bytes) keep loaded (+2), used (+3), lock count (+4)
// and a second retention byte (+5).
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/native_tu15_data_array_allocate_specific.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_page_allocator.hpp"

namespace halo2::native_tu15_sound {
namespace {
// Retail globals read by the sound cache.
constexpr std::uint32_t sound_cache=0x50ebdcu;      // +0x44: 16-byte cache entries
constexpr std::uint32_t cache_allocator=0x50ebe4u;  // +0x38 stamp; +0x64 -> +0x44: 24-byte blocks (+0x14 last use)
constexpr std::uint32_t binding_table=0x4f2c4cu;    // +0x44: 0xBC-byte sound bindings
constexpr std::uint32_t codec_playback=0x48b8fcu;   // .data dword (0 in the XBE image), read at run time
constexpr std::uint32_t no_handle=0xffffffffu;

// Core datum owners used by the real sound-cache allocation path. Their
// implementation and full integer ABI remain owned by the core module.
void allocate_specific(M& m,Abi& a,FP,const O& o,const Services&){
    native_tu15_data_array_allocate_specific_abi(m,a,o);
}
void initialize_record(M& m,Abi& a,FP,const O& o,const Services&){
    native_tu15_data_array_initialize_record_abi(m,a,o);
}
void allocate_datum(M& m,Abi& a,FP,const O& o,const Services&){
    native_tu15_data_array_allocate_abi(m,a,o);
}
void delete_datum(M& m,Abi& a,FP,const O& o,const Services&){
    native_tu15_data_array_delete_abi(m,a,o);
}
void page_prefer(M& m,Abi& a,FP,const O&,const Services&){
    native_tu15_page_window_prefer_abi(m,a);
}
void page_allocate(M& m,Abi& a,FP,const O& o,const Services& s){
    native_tu15_page_allocate_abi(m,a,s,o);
}
void page_delete(M& m,Abi& a,FP,const O& o,const Services& s){
    native_tu15_page_delete_abi(m,a,s,o);
}

// Actual21F3F0 page eviction callback. The chunk loses its cache handle
// before the complete core datum-deletion ABI retires the cache entry.
void cache_page_deleted(M& m,Abi& a,FP,const O& o,const Services&){
    auto& g=a.gp;g[0]=argument(m,a,0);g[2]=u32(m,sound_cache);g[1]=g[0]&0xffffu;
    g[4]-=4u;store(m,{},0x21f402u,g[4],g[6]);g[6]=u32(m,g[2]+0x44u);g[1]<<=4u;
    g[1]=u32(m,g[1]+g[6]+12u);store(m,o,0x21f40du,g[1]+8u,0xffffffffu);
    g[4]-=4u;store(m,{},0x21f414u,g[4],0x21f419u);native_tu15_data_array_delete_abi(m,a,o);
    g[6]=u32(m,g[4]);g[4]+=4u;pop_return(m,a,4);
}

// MOV r8,value: replaces only the register's low byte.
std::uint32_t low_byte(std::uint32_t reg,std::uint32_t value){return (reg&~0xffu)|(value&0xffu);}

// The chunk row a binding names: definition +8 (first pitch range) plus the
// binding's range; that 12-byte range row's +8 (first permutation) plus its
// permutation; that 16-byte permutation row's +0xC (first chunk) plus its
// chunk. scaled is (first chunk + chunk)*3, which callers leave in a register.
// chunk_first reads the binding's chunk index before the gestalt tables, as
// 1293F0 does; 129470 and 128970 read it last.
struct Chunk {std::uint32_t row,scaled;};
Chunk binding_chunk(const M &m,std::uint32_t binding,std::uint32_t definition,bool chunk_first) {
    const auto range=s16(m,definition+8u)+s8(m,binding+0x9cu);
    const auto gestalt=u32(m,sound_gestalt);
    const auto ranges=u32(m,gestalt+0x24u);
    const auto permutation=s16(m,ranges+range*12u+8u)+s8(m,binding+0x9du);
    std::uint32_t chunk=chunk_first?s16(m,binding+0x9eu):0u;
    const auto permutations=u32(m,gestalt+0x2cu),chunks=u32(m,gestalt+0x44u);
    const auto first=s16(m,(permutation<<4)+permutations+0xcu);
    if(!chunk_first)chunk=s16(m,binding+0x9eu);
    const auto scaled=(first+chunk)*3u;
    return {chunks+scaled*4u,scaled};
}

// 129470 body: a binding (ECX) that holds a cache lock drops it: the entry's
// lock count (+4) decrements and the binding's bit 1 of +4 clears. AL takes
// the binding's +4 shifted right once; when the lock was held EAX ends as the
// cache entry and EDX as the cache pointer with the new count in DL.
void release_lock(M &m,Abi &a,const O &o,std::uint32_t binding) {
    const auto held=std::uint8_t(u8(m,binding+4u)>>1);
    a.gp[0]=low_byte(a.gp[0],held);
    if(!(held&1u))return;
    const auto definition=tag_data(m,u32(m,binding+0xcu));
    const auto chunk=binding_chunk(m,binding,definition,false);
    const auto handle=u32(m,chunk.row+8u);
    const auto cache=u32(m,sound_cache);
    const auto entry=((handle&0xffffu)<<4)+u32(m,cache+0x44u);
    const auto locks=std::uint8_t(u8(m,entry+4u)-1u);
    store8(m,o,0x1294edu,entry+4u,locks);
    store8(m,o,0x1294f0u,binding+4u,u8(m,binding+4u)&0xfdu);
    // ESI was pushed and popped around the lookup; it is unchanged.
    a.gp[0]=entry;a.gp[2]=low_byte(cache,locks);
}

// 129470: release a binding's cache lock. ECX: binding. Returns nothing (AL,
// and EAX/EDX when the lock was held, as release_lock describes).
void release_binding(M &m,Abi &a,FP,const O &o,const Services &) {
    release_lock(m,a,o,a.gp[1]);
    pop_return(m,a);
}

// 12B1C0: rebind binding AX (low 16 bits, row of the binding table) to a new
// chunk. Releases its cache lock (129470), then stores the stack arguments:
// sound tag datum (+0xC), pitch range byte (+0x9C), permutation byte (+0x9D)
// and chunk word (+0x9E). ret 16. Leaves ECX = binding, AX = chunk word,
// EDX = datum with DL = permutation byte.
void rebind(M &m,Abi &a,FP,const O &o,const Services &) {
    const auto table=u32(m,binding_table);
    const auto rows=u32(m,table+0x44u);
    const auto binding=(a.gp[0]&0xffffu)*0xbcu+rows;
    a.gp[0]=binding;a.gp[1]=binding;a.gp[2]=rows;
    // call 129470: its return address and saved ESI are dead stack scratch.
    release_lock(m,a,o,binding);
    const auto sp=a.gp[4];
    const auto datum=u32(m,sp+4u);
    const auto range=u8(m,sp+8u);
    store(m,o,0x12b1e5u,binding+0xcu,datum);
    const auto permutation=u8(m,sp+0xcu);
    store8(m,o,0x12b1ecu,binding+0x9cu,range);
    const auto chunk=u16(m,sp+0x10u);
    store8(m,o,0x12b1f7u,binding+0x9du,permutation);
    store16(m,o,0x12b1fdu,binding+0x9eu,chunk);
    a.gp[0]=(a.gp[0]&0xffff0000u)|chunk;
    a.gp[2]=low_byte(datum,permutation);
    pop_return(m,a,16);
}

// 1296E0: whether sound tag EAX (datum, low 16 bits) is playable from the
// cache. Its definition must be in the directly playable format (+5 == 1 with
// +4 != 2), or in format +4 == 2 while the 48B8FC global is set; it must have
// pitch ranges (+0xA) and its first range must have permutations (+0xA word).
// Returns EAX = 1 or 0. ECX and EDX hold what the original's tests leave.
void sound_playable(M &m,Abi &a,FP,const O &,const Services &) {
    const auto tags=u32(m,tag_instances);
    const auto definition=u32(m,((a.gp[0]&0xffffu)<<4)+tags+8u);
    const auto finish=[&](std::uint32_t result){a.gp[0]=result;pop_return(m,a);};
    const bool direct=u8(m,definition+5u)==1u;
    a.gp[1]=low_byte(tags,2u);
    const bool codec=u8(m,definition+4u)==2u;
    if(!direct || codec) {
        if(!codec)return finish(0);
        a.gp[1]=u32(m,codec_playback);
        if(!a.gp[1])return finish(0);
    }
    const auto ranges=u8(m,definition+0xau);
    a.gp[1]=low_byte(a.gp[1],ranges);
    if(!ranges)return finish(0);
    a.gp[2]=s16(m,definition+8u)*3u;
    a.gp[1]=u32(m,u32(m,sound_gestalt)+0x24u);
    finish(u16(m,a.gp[1]+a.gp[2]*4u+0xau)?1u:0u);
}

// 21F260 body: request the cache entry of a chunk row (12 bytes, cache handle
// at +8). Request bits: 0 wait until loaded, 1 allocate when absent, 2 lock.
// Allocation (21F420: chunk row in EBX, the sound datum on the stack, ret 4)
// and the wait's yield (2DBF74, no arguments) are original routines called
// through Services. A present entry gets its block's last-use stamp; a loaded
// one is marked used (+3); a lock increments +4. Returns the state in EAX and
// EDX: bit 0 pending (a handle that is not ready), bit 1 ready, bit 2 locked.
// ECX keeps the caller's upper bits with CL = request bit 0, unless a callee
// returned another ECX. EBX, ESI, EDI are restored; EBP is not touched.
//
// frame is ESP at entry (the return address); ESP is left there for the
// caller's RET 12. The callees are assumed to keep the ABI the compiled code
// relies on: they preserve EBX, ESI, EDI, EBP and ESP (net of their own
// argument pops) and do not touch this frame, so the values the original
// reloads from its frame after a call (CL, BL, EDX) are kept here in C++.
std::uint32_t request_chunk(M &m,Abi &a,const O &o,const Services &services,std::uint32_t frame,
    std::uint32_t datum,std::uint32_t row,std::uint32_t request) {
    // Retain private stack writes too: another source caller may reuse
    // these bytes, as the completion-to-pump chain already demonstrates.
    const auto push=[&](std::uint32_t pc,std::uint32_t value){a.gp[4]-=4u;store(m,{},pc,a.gp[4],value);};
    const auto pop=[&](){const auto value=u32(m,a.gp[4]);a.gp[4]+=4u;return value;};
    a.gp[4]=frame-8u;push(0x21f267u,a.gp[3]);push(0x21f268u,a.gp[6]);push(0x21f269u,a.gp[7]);
    auto wait=request&1u,lock=(request>>2)&1u;
    a.gp[0]=request;
    a.gp[1]=low_byte(a.gp[1],wait);
    a.gp[2]=0u;
    a.gp[3]=low_byte(request>>2,lock);
    a.gp[7]=row;
    a.gp[6]=u32(m,row+8u);
    store(m,{},0x21f283u,frame-4u,0u);store8(m,{},0x21f287u,frame+12u,wait);
    store8(m,{},0x21f28bu,frame-5u,lock);
    if(a.gp[6]==no_handle) {
        a.gp[6]=datum;
        if(datum!=no_handle) {
            a.gp[0]=request>>1;
            if(a.gp[0]&1u) {
                // push esi (datum); mov ebx,edi; call 21F420.
                a.gp[3]=row;
                const std::uint32_t arguments[]{datum};
                call_out(m,a,services,o,0x21f2a3u,0x21f420u,0x21f2a8u,arguments,true);
                wait=u8(m,frame+12u);lock=u8(m,frame-5u);
                a.gp[1]=low_byte(a.gp[1],wait);
                a.gp[3]=low_byte(a.gp[3],lock);
                a.gp[2]=u32(m,frame-4u);
            }
        }
    }
    std::uint32_t state=0u;
    a.gp[0]=u32(m,row+8u);
    if(a.gp[0]!=no_handle) {
        const auto block=(a.gp[0]&0xffffu)*3u;
        const auto allocator=u32(m,cache_allocator);
        push(0x21f2c9u,a.gp[5]);
        const auto blocks=u32(m,u32(m,allocator+0x64u)+0x44u);
        const auto stamp=u32(m,allocator+0x38u);
        store(m,o,0x21f2d3u,blocks+block*8u+0x14u,stamp);
        const auto handle=u32(m,row+8u);
        a.gp[0]=u32(m,sound_cache);
        const auto entry=((handle&0xffffu)<<4)+u32(m,a.gp[0]+0x44u);
        a.gp[6]=entry;a.gp[5]=pop();
        // Each test of the entry's bytes goes through AL.
        const auto al=[&](std::uint32_t at){const auto v=u8(m,at);a.gp[0]=low_byte(a.gp[0],v);return v!=0u;};
        bool loaded;
        if(wait) {
            loaded=al(entry+2u) || al(entry+2u);
            if(!loaded) {
                do call_out(m,a,services,o,0x21f300u,0x2dbf74u,0x21f305u,{},false);
                while(!al(entry+2u));
                a.gp[2]=u32(m,frame-4u);          // EDX reloaded from the actual frame
                loaded=al(entry+2u);
            }
        } else loaded=al(entry+2u);
        if(loaded) {
            if(!al(entry+3u))store8(m,o,0x21f31eu,entry+3u,1u);
            state=2u;store(m,{},0x21f322u,frame-4u,2u);
        }
        if(lock) {
            store8(m,o,0x21f332u,entry+4u,u8(m,entry+4u)+1u);
            state|=4u;
        }
    }
    if(!(state&2u) && u32(m,row+8u)!=no_handle)state|=1u;
    a.gp[0]=a.gp[2]=state;
    a.gp[7]=pop();a.gp[6]=pop();a.gp[3]=pop();a.gp[4]+=8u;
    return state;
}

// 21F260: request a chunk's cache entry. Stack: sound datum, chunk row,
// request bits (ret 12). Returns the state in EAX (request_chunk).
void request_chunk_entry(M &m,Abi &a,FP,const O &o,const Services &services) {
    const auto frame=a.gp[4];
    // The original reads the datum argument only when the row has no handle;
    // all three sit beside the return address the RET reads, so reading it
    // here faults no differently.
    const auto request=argument(m,a,2),row=argument(m,a,1),datum=argument(m,a,0);
    request_chunk(m,a,o,services,frame,datum,row,request);
    pop_return(m,a,12);
}

// 1293F0 body: a binding (EAX) that holds no cache lock takes one: bit 1 of
// +4 is set and its chunk's entry is locked through 21F260 (no datum,
// request 4, which never calls out). CL takes the binding's +4 shifted right
// once; when the lock is taken ECX ends as (chunk*3 with CL = 0) and EAX/EDX
// as 21F260's state. ESI is pushed and popped. ESP points at the return
// address on entry and is left there for the caller's RET.
void lock_binding(M &m,Abi &a,const O &o,const Services &services,std::uint32_t binding) {
    const auto frame=a.gp[4];
    const auto held=std::uint8_t(u8(m,binding+4u)>>1);
    a.gp[1]=low_byte(a.gp[1],held);
    if(held&1u)return;
    store8(m,o,0x1293fau,binding+4u,u8(m,binding+4u)|2u);
    const auto definition=tag_data(m,u32(m,binding+0xcu));
    const auto chunk=binding_chunk(m,binding,definition,true);
    // push esi; push 4; push row; push -1; call 21F260; pop esi.
    a.gp[1]=chunk.scaled;
    request_chunk(m,a,o,services,frame-0x14u,no_handle,chunk.row,4u);
    a.gp[4]+=16u+4u;
}

// 1293F0: lock a binding's chunk cache entry. EAX: binding. Returns nothing
// (registers as lock_binding describes).
void lock_binding_entry(M &m,Abi &a,FP,const O &o,const Services &services) {
    lock_binding(m,a,o,services,a.gp[0]);
    pop_return(m,a);
}

// 128970: whether a binding's chunk is ready. Stack: binding (ret 4).
// Requests the chunk's entry with allocation (21F260 request 2, which may
// call 21F420); when it is pending or ready the binding takes its lock
// (1293F0). Returns EAX = the ready bit. EBX, ESI, EDI are restored; ECX and
// EDX are what the last nested request leaves.
void binding_ready(M &m,Abi &a,FP,const O &o,const Services &services) {
    const auto tags=u32(m,tag_instances);       // read before the argument
    const auto entry=a.gp[4];
    const auto ebx=a.gp[3],esi=a.gp[6],edi=a.gp[7];
    const auto binding=u32(m,entry+4u);
    const auto datum=u32(m,binding+0xcu);
    const auto definition=u32(m,((datum&0xffffu)<<4)+tags+8u);
    const auto chunk=binding_chunk(m,binding,definition,false);
    // push ebx/esi/edi; push 2; push row; push ecx (datum); call 21F260.
    a.gp[1]=datum;
    const auto state=request_chunk(m,a,o,services,entry-0x1cu,datum,chunk.row,2u);
    a.gp[4]+=16u;
    if(state&3u) {
        // mov eax,esi; call 1293F0.
        a.gp[0]=binding;
        a.gp[4]-=4u;
        lock_binding(m,a,o,services,binding);
        a.gp[4]+=4u;
    }
    a.gp[0]=(state>>1)&1u;
    a.gp[3]=ebx;a.gp[6]=esi;a.gp[7]=edi;
    a.gp[4]+=12u;
    pop_return(m,a,4);
}

// 21F3B0: whether a cache entry must stay resident. Stack: cache handle
// (ret 4). True when it is locked (+4), held (+5) or not loaded (+2 zero).
// Returns AL (EAX's upper bits are the cache pointer's); ECX = entry; EDX =
// cache entries, with DL = the loaded byte when that is tested.
void cache_entry_retained(M &m,Abi &a,FP,const O &,const Services &) {
    const auto handle=argument(m,a,0);
    const auto cache=u32(m,sound_cache);
    const auto entries=u32(m,cache+0x44u);
    const auto entry=((handle&0xffffu)<<4)+entries;
    a.gp[1]=entry;a.gp[2]=entries;
    std::uint32_t retained=1u;
    if(!u8(m,entry+4u) && !u8(m,entry+5u)) {
        const auto loaded=u8(m,entry+2u);
        a.gp[2]=low_byte(entries,loaded);
        retained=loaded?0u:1u;
    }
    a.gp[0]=low_byte(cache,retained);
    pop_return(m,a,4);
}

//21F360: retain a loaded chunk's cache-data reference and resolve the actual
//allocator block. EDX is the chunk descriptor. The byte+5 reference saturates
//atFF; the allocator's shift uses x86's low-five-bit count and addresses wrap
//in the original32-bit domain. This creates no entry, readiness or cache pin.
void chunk_data_body(M& m,Abi& a,const O& o,std::uint32_t chunk) {
    auto handle=u32(m,chunk+8u);
    const auto cache=u32(m,sound_cache),entries=u32(m,cache+0x44u);
    const auto entry=entries+(handle&0xffffu)*16u;
    const auto references=u8(m,entry+5u);
    if(references!=0xffu)store8(m,o,0x21f382u,entry+5u,references+1u);
    handle=u32(m,chunk+8u);
    const auto allocator=u32(m,cache_allocator),owner=u32(m,allocator+0x64u);
    const auto shift=u32(m,allocator+0x34u);
    a.gp[2]=(handle&0xffffu)*3u;
    const auto blocks=u32(m,owner+0x44u);
    const auto offset=u32(m,blocks+a.gp[2]*8u+8u);
    a.gp[1]=u32(m,0x50ebe0u);
    a.gp[0]=(offset<<(shift&31u))+a.gp[1];
}
void chunk_data(M& m,Abi& a,FP,const O& o,const Services&) {
    chunk_data_body(m,a,o,a.gp[2]);pop_return(m,a);
}

//2B98D0: stock encoding0/1 packet construction. The submitted count is a
//signed three-bit field; malformed negative counts retain their real source
//access, rather than being clamped into the two-slot queue. The literal72-byte
//tail clamp is shared by mono/stereo. The caller's signed cursor comparison,
//all six packet stores, AL return and ret8 are original instruction behavior.
void stream_packet(M& m,Abi& a,FP,const O& o,const Services&) {
    const auto sp=a.gp[4],stream=u32(m,sp+4u);
    const auto flags=u8(m,stream+2u);
    const auto submitted=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(flags<<2u)))>>5;
    const auto chunk=u32(m,stream+0x1cu+std::uint32_t(submitted)*4u);
    chunk_data_body(m,a,o,chunk);
    const auto size=u32(m,chunk+4u)&0x3fffffffu;
    const auto tail=std::max(std::int32_t(size)-72,0);
    auto offset=u32(m,stream+0x2cu);
    if(std::bit_cast<std::int32_t>(offset)>tail)offset=std::uint32_t(tail);
    const auto packet=u32(m,sp+8u),data=a.gp[0]+offset;
    store(m,o,0x2b991cu,packet,data);store(m,o,0x2b991eu,packet+0x10u,chunk);
    store(m,o,0x2b9921u,packet+8u,0);store(m,o,0x2b9928u,packet+0xcu,0);
    store(m,o,0x2b992fu,packet+0x14u,0);
    a.gp[2]=(u32(m,chunk+4u)&0x3fffffffu)-offset;
    store(m,o,0x2b9942u,packet+4u,a.gp[2]);
    a.gp[1]=packet;a.gp[0]=low_byte(data,1u);pop_return(m,a,8);
}

//2B9950: accepted stock packet bookkeeping. Increment only submitted bits
//3..5, preserve the queued count/upper flags and clear the compressed cursor.
//The native output owner must call this only after a real accepted submission.
void stream_packet_submitted(M& m,Abi& a,FP,const O& o,const Services&) {
    const auto stream=argument(m,a,0);
    const auto flags=u8(m,stream+2u);
    const auto next=(flags&0xc7u)|((flags+8u)&0x38u);
    a.gp[1]=stream;a.gp[0]=low_byte(a.gp[0],flags);a.gp[2]=low_byte(a.gp[2],next);
    store8(m,o,0x2b996cu,stream+2u,next);store(m,o,0x2b996fu,stream+0x2cu,0);
    pop_return(m,a,8);
}

// The complete forward32A990 path reached by2B82A0: destination=stream+1C,
// source=stream+20 and byte count is a signed three-bit count times four.
// The source x86 ABI has DF clear. These operands always select forward copy,
// including32-bit wrap. Preserve
// its actual scalar/REP write order and sites instead of replacing a corrupt
// count with a two-slot clamp or allocating an enormous temporary buffer.
void retire_shift(M& m,const O& o,std::uint32_t stream,std::uint32_t bytes) {
    if(!bytes)return;
    auto from=stream+0x20u,to=stream+0x1cu;
    const auto alignment=to&3u;
    std::uint32_t repeat_pc=0x32a9c3u;
    if(alignment){
        const auto prefix=4u-alignment;
        constexpr std::array<std::array<std::uint32_t,3>,3> sites{{
            {0x32aa04u,0x32aa09u,0x32aa12u},{0x32aa30u,0x32aa38u,0},{0x32aa54u,0,0}}};
        for(unsigned n=0;n<prefix;++n)store8(m,o,sites[alignment-1u][n],to+n,u8(m,from+n));
        from+=prefix;to+=prefix;bytes-=prefix;
        constexpr std::array repeat{0x32aa20u,0x32aa46u,0x32aa64u};repeat_pc=repeat[alignment-1u];
    }
    const auto words=bytes>>2u;
    for(std::uint32_t n=0;n<words;++n){
        const auto site=words>=8u?repeat_pc:0x32aac4u-(words-1u-n)*8u;
        store(m,o,site,to,u32(m,from));from+=4u;to+=4u;
    }
    constexpr std::array<std::array<std::uint32_t,3>,3> tails{{
        {0x32aaf6u,0,0},{0x32ab02u,0x32ab07u,0},{0x32ab16u,0x32ab1bu,0x32ab21u}}};
    const auto tail=bytes&3u;
    for(unsigned n=0;n<tail;++n)store8(m,o,tails[tail-1u][n],to+n,u8(m,from+n));
}
std::int32_t queued_chunks(std::uint8_t flags){
    return std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(flags<<5u)))>>5;
}
//2B82A0: retire one queued/submitted chunk, debit its cache pin, shift the
//remaining source pointers and clear the freed slots. The queued count is
//reread after copy/clear exactly as the source does; malformed state must hit
//the actual missing storage rather than receive a fabricated successful stop.
void stream_retire_body(M& m,Abi& a,const O& o,std::uint32_t stream,std::uint32_t chunk){
    // These writes were dead at this helper's return but are live input to
    // 2B8530's later local/argument reuse through the 227140 callback.
    const auto push=[&](std::uint32_t pc,std::uint32_t value){a.gp[4]-=4u;store(m,{},pc,a.gp[4],value);};
    const auto pop=[&](){const auto value=u32(m,a.gp[4]);a.gp[4]+=4u;return value;};
    push(0x2b82a0u,a.gp[7]);
    const auto flags=u8(m,stream+2u);
    const auto next=(flags&0xc0u)|((flags-8u)&0x38u)|((flags-1u)&7u);
    store8(m,o,0x2b82c7u,stream+2u,next);
    const auto handle=u32(m,chunk+8u),cache=u32(m,sound_cache),entries=u32(m,cache+0x44u);
    const auto entry=entries+(handle&0xffffu)*16u;
    store8(m,o,0x2b82e6u,entry+4u,u8(m,entry+4u)-1u);
    const auto bytes=std::uint32_t(queued_chunks(u8(m,stream+2u)))*4u;
    push(0x2b82f8u,bytes);push(0x2b82fcu,stream+0x20u);push(0x2b8300u,stream+0x1cu);
    push(0x2b8301u,0x2b8306u);push(0x32a990u,a.gp[5]);a.gp[5]=a.gp[4];
    push(0x32a993u,a.gp[7]);push(0x32a994u,a.gp[6]);
    retire_shift(m,o,stream,bytes);
    a.gp[6]=pop();a.gp[7]=pop();a.gp[4]=a.gp[5];a.gp[5]=pop();pop_return(m,a);
    const auto count=queued_chunks(u8(m,stream+2u));
    const auto clear=(2u-std::uint32_t(count))*4u;
    auto at=stream+0x1cu+std::uint32_t(count)*4u;
    for(std::uint32_t n=0;n<(clear>>2u);++n){store(m,o,0x2b832fu,at,0);at+=4u;}
    const auto final=queued_chunks(u8(m,stream+2u));
    a.gp[4]+=12u;a.gp[7]=pop();
    a.gp[0]=std::uint32_t(final);a.gp[1]=0;a.gp[2]=clear;
    if(final==0){const auto state=(u8(m,stream+1u)&0xf9u)|1u;
        a.gp[2]=low_byte(clear,state);store8(m,o,0x2b8366u,stream+1u,state);}
    else{--a.gp[0];if(final==1){const auto state=(u8(m,stream+1u)&0xfau)|2u;
        a.gp[1]=state;store8(m,o,0x2b8359u,stream+1u,state);}}
}
void stream_retire(M& m,Abi& a,FP,const O& o,const Services&){
    stream_retire_body(m,a,o,a.gp[6],a.gp[2]);pop_return(m,a);
}
//2B99C0: stock chunk_finished callback, including failure/flush statuses.
//The outer227140 callback owns success-only pumping; this helper always
//performs queue retirement and then conditionally debits the data reference.
void stream_chunk_finished(M& m,Abi& a,FP,const O& o,const Services&){
    const auto push=[&](std::uint32_t pc,std::uint32_t value){a.gp[4]-=4u;store(m,{},pc,a.gp[4],value);};
    const auto pop=[&](){const auto value=u32(m,a.gp[4]);a.gp[4]+=4u;return value;};
    push(0x2b99c0u,a.gp[6]);a.gp[6]=u32(m,a.gp[4]+8u);
    push(0x2b99c5u,a.gp[7]);a.gp[7]=u32(m,a.gp[4]+0x10u);a.gp[2]=a.gp[7];
    push(0x2b99ccu,0x2b99d1u);stream_retire_body(m,a,o,a.gp[6],a.gp[2]);pop_return(m,a);
    const auto handle=u32(m,a.gp[7]+8u),cache=u32(m,sound_cache),entries=u32(m,cache+0x44u);
    a.gp[6]=entries;
    const auto entry=entries+(handle&0xffffu)*16u;
    const auto reference=u8(m,entry+5u);
    a.gp[0]=entry;a.gp[1]=low_byte(cache,reference);
    a.gp[7]=pop();a.gp[6]=pop();
    if(reference){a.gp[1]=low_byte(cache,reference-1u);store8(m,o,0x2b99f3u,entry+5u,reference-1u);}
    pop_return(m,a,12);
}

//2E4D60: the authenticated stock codec's initialize/queue-ready methods
//are a literal ret4. Keep this CPU owner distinct from device admission.
void stock_queue_noop(M& m,Abi& a,FP,const O&,const Services&){pop_return(m,a,4);}

//2B8210: enqueue one chunk, request its cache pin and wake the selected
//codec when none is submitted. The signed three-bit queued count is not
//clamped, and request(-1,chunk,4) never allocates or waits for data.
void stream_enqueue(M& m,Abi& a,FP,const O& o,const Services& services){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    store8(m,o,0x2b8210u,g[6]+3u,u8(m,g[6]+3u)&0xf7u);
    g[0]=low_byte(g[0],u8(m,g[6]+1u));g[1]=low_byte(g[1],g[0]);g[1]=low_byte(g[1],g[1]<<5u);
    g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[1]))));
    g[1]=std::uint32_t(std::bit_cast<std::int32_t>(g[1])>>5);
    // Original table2B8288: modes0/1 ->822E,2 ->8234,3 ->823B.
    if(g[1]<=2u){g[0]=low_byte(g[0],g[1]==2u?((g[0]&0xfbu)|3u):((g[0]&0xfau)|2u));
        store8(m,o,0x2b8238u,g[6]+1u,g[0]);}
    g[2]=low_byte(g[2],u8(m,g[6]+2u));g[2]=low_byte(g[2],g[2]<<5u);
    if(std::bit_cast<std::int8_t>(std::uint8_t(g[2]))<0x40){
        push(0x2b8246u,4u);push(0x2b8248u,g[7]);push(0x2b8249u,0xffffffffu);
        call_out(m,a,services,o,0x2b824bu,0x21f260u,0x2b8250u,{},true);
        g[0]=low_byte(g[0],u8(m,g[6]+2u));g[0]=low_byte(g[0],g[0]<<5u);
        g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[0]))));
        g[1]=std::uint32_t(std::bit_cast<std::int32_t>(g[1])>>5);
        store(m,o,0x2b825cu,g[6]+g[1]*4u+0x1cu,g[7]);
        g[0]=low_byte(g[0],u8(m,g[6]+2u));g[2]=low_byte(g[2],g[0]);
        g[2]=low_byte(g[2],g[2]<<5u);
        g[2]=low_byte(g[2],std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[2])))>>5));
        g[2]=low_byte(g[2],g[2]+1u);g[2]=low_byte(g[2],g[2]^g[0]);
        g[2]=low_byte(g[2],g[2]&7u);g[2]=low_byte(g[2],g[2]^g[0]);
        store8(m,o,0x2b8274u,g[6]+2u,g[2]);
    }
    if(!(u8(m,g[6]+2u)&0x38u)){
        g[1]=u32(m,g[6]+0x30u);g[0]=u32(m,g[1]);push(0x2b8282u,g[6]);
        call_out(m,a,services,o,0x2b8283u,u32(m,g[0]+4u),0x2b8286u,{},true);
    }
    pop_return(m,a);
}

//2B85F0: release every queued cache pin from the tail. Source pointers,
//submitted bits and retained data references are deliberately not rewritten.
void stream_release_queued(M& m,Abi& a,FP,const O& o,const Services&){
    auto& g=a.gp;
    g[0]=low_byte(g[0],u8(m,g[1]+2u));g[0]=low_byte(g[0],g[0]<<5u);
    if(std::bit_cast<std::int8_t>(std::uint8_t(g[0]))>0){
        g[4]-=4u;store(m,{},0x2b85fau,g[4],g[6]);g[6]=u32(m,sound_cache);
        do{
            g[2]=low_byte(g[2],u8(m,g[1]+2u));g[2]=low_byte(g[2],g[2]<<5u);
            g[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[2]))));
            g[0]=std::uint32_t(std::bit_cast<std::int32_t>(g[0])>>5);
            g[2]=u32(m,g[1]+g[0]*4u+0x18u);g[0]=u32(m,g[2]+8u);
            g[2]=u32(m,g[6]+0x44u);g[0]=(g[0]&0xffffu)<<4u;g[0]+=g[2];
            store8(m,o,0x2b8621u,g[0]+4u,u8(m,g[0]+4u)-1u);
            g[0]=low_byte(g[0],u8(m,g[1]+2u));g[2]=low_byte(g[2],g[0]);
            g[2]=low_byte(g[2],g[2]<<5u);
            g[2]=low_byte(g[2],std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[2])))>>5));
            g[2]=low_byte(g[2],g[2]-1u);g[2]=low_byte(g[2],g[2]^g[0]);
            g[2]=low_byte(g[2],g[2]&7u);g[2]=low_byte(g[2],g[2]^g[0]);
            g[0]=low_byte(g[0],g[2]);g[0]=low_byte(g[0],g[0]<<5u);
            store8(m,o,0x2b863fu,g[1]+2u,g[2]);
        }while(std::bit_cast<std::int8_t>(std::uint8_t(g[0]))>0);
        g[6]=u32(m,g[4]);g[4]+=4u;
    }
    pop_return(m,a);
}

//2B9A00: stock packet availability. Compare the two signed three-bit fields
//in their shifted byte form, preserving the source's partial-register writes.
void stream_packet_available(M& m,Abi& a,FP,const O&,const Services&){
    a.gp[0]=argument(m,a,0);a.gp[0]=low_byte(a.gp[0],u8(m,a.gp[0]+2u));
    a.gp[1]=low_byte(a.gp[1],a.gp[0]);a.gp[1]=low_byte(a.gp[1],a.gp[1]&0xf8u);
    a.gp[0]=low_byte(a.gp[0],a.gp[0]<<5u);a.gp[1]=low_byte(a.gp[1],a.gp[1]<<2u);
    a.gp[2]=std::bit_cast<std::int8_t>(std::uint8_t(a.gp[0]))>
        std::bit_cast<std::int8_t>(std::uint8_t(a.gp[1]));
    a.gp[0]=low_byte(a.gp[0],a.gp[2]);pop_return(m,a,8);
}
//2B9980: a rejected stock packet releases its retained data reference but
//does not increment submitted state, dequeue the chunk or release its pin.
void stream_packet_failed(M& m,Abi& a,FP,const O& o,const Services&){
    a.gp[0]=argument(m,a,0);a.gp[1]=low_byte(a.gp[1],u8(m,a.gp[0]+2u));
    a.gp[1]=low_byte(a.gp[1],a.gp[1]<<2u);
    a.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(a.gp[1]))));
    a.gp[1]=u32(m,sound_cache);a.gp[2]=std::uint32_t(std::bit_cast<std::int32_t>(a.gp[2])>>5);
    a.gp[0]=u32(m,a.gp[0]+a.gp[2]*4u+0x1cu);a.gp[0]=u32(m,a.gp[0]+8u);
    a.gp[2]=u32(m,a.gp[1]+0x44u);a.gp[0]=(a.gp[0]&0xffffu)<<4u;
    a.gp[1]=low_byte(a.gp[1],u8(m,a.gp[0]+a.gp[2]+5u));a.gp[0]+=a.gp[2];
    if(std::uint8_t(a.gp[1])){a.gp[1]=low_byte(a.gp[1],a.gp[1]-1u);
        store8(m,o,0x2b99b4u,a.gp[0]+5u,a.gp[1]);}
    pop_return(m,a,8);
}
//2B8530: complete stream submission loop. SDK/codec calls retain their real
//ABI owner. Pushes and local bytes stay in source order, including the live
//packet descriptor and the upper bytes of its submitted-something argument.
//No status, packet acceptance or discontinuity result is supplied here.
void stream_pump(M& m,Abi& a,FP,const O& o,const Services& services){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    const auto pop=[&](){const auto value=u32(m,g[4]);g[4]+=4u;return value;};
    const auto call=[&](std::uint32_t pc,std::uint32_t target,std::uint32_t next){
        call_out(m,a,services,o,pc,target,next,{},true);};
    g[4]-=0x24u;push(0x2b8533u,g[6]);g[6]=g[0];store8(m,{},0x2b8536u,g[4]+8u,0);
    push(0x2b853bu,g[3]);
    for(;;){
        g[0]=low_byte(g[0],u8(m,g[6]+1u));g[0]=low_byte(g[0],g[0]<<5u);
        g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(g[0]))));
        g[1]=std::uint32_t(std::bit_cast<std::int32_t>(g[1])>>5);g[2]=1u<<(g[1]&31u);
        if(!(g[2]&0xcu))break;
        g[0]=u32(m,g[6]+0x24u);g[1]=u32(m,g[0]);g[2]=g[4]+0x10u;
        push(0x2b8561u,g[2]);push(0x2b8562u,g[0]);call(0x2b8563u,u32(m,g[1]+0xcu),0x2b8566u);
        if(std::bit_cast<std::int32_t>(g[0])<0)break;
        g[0]=low_byte(g[0],u8(m,g[4]+0x10u));g[0]=low_byte(g[0],g[0]&1u);
        store8(m,{},0x2b8570u,g[4]+0xbu,g[0]);if(!std::uint8_t(g[0]))break;
        g[0]=u32(m,g[4]+0xcu);g[1]=u32(m,g[6]+0x30u);g[2]=u32(m,g[1]);
        push(0x2b857fu,g[0]);push(0x2b8580u,g[6]);call(0x2b8581u,u32(m,g[2]+0x20u),0x2b8584u);
        if(!std::uint8_t(g[0]))break;
        g[1]=u32(m,g[6]+0x30u);g[2]=u32(m,g[1]);g[0]=g[4]+0x14u;
        push(0x2b8591u,g[0]);push(0x2b8592u,g[6]);call(0x2b8593u,u32(m,g[2]+0xcu),0x2b8596u);
        if(!std::uint8_t(g[0]))break;
        g[0]=u32(m,g[6]+0x24u);g[1]=u32(m,g[0]);push(0x2b859fu,0);
        g[2]=g[4]+0x18u;push(0x2b85a5u,g[2]);push(0x2b85a6u,g[0]);
        call(0x2b85a7u,u32(m,g[1]+0x10u),0x2b85aau);
        g[1]=u32(m,g[6]+0x30u);const bool accepted=std::bit_cast<std::int32_t>(g[0])>=0;
        g[0]=u32(m,g[1]);g[2]=g[4]+0x14u;g[3]=low_byte(g[3],accepted);
        push(0x2b85bau,g[2]);push(0x2b85bbu,g[6]);
        if(std::uint8_t(g[3]))call(0x2b85beu,u32(m,g[0]+0x10u),0x2b85c1u);
        else call(0x2b85c3u,u32(m,g[0]+0x14u),0x2b85c6u);
        if(!std::uint8_t(g[3]))break;
        store8(m,{},0x2b85cau,g[4]+0xcu,1);
    }
    g[0]=low_byte(g[0],u8(m,g[4]+0xcu));const bool submitted=std::uint8_t(g[0])!=0;g[3]=pop();
    if(submitted){g[6]=u32(m,g[6]+0x24u);g[0]=u32(m,g[6]);push(0x2b85e2u,g[6]);
        call(0x2b85e3u,u32(m,g[0]+0x14u),0x2b85e6u);}
    g[6]=pop();g[4]+=0x24u;pop_return(m,a);
}
//227140: source channel callback. Codec retirement runs for every status;
//success-only refill additionally requires stream byte+3 bits0C clear.
void stream_completed(M& m,Abi& a,FP,const O& o,const Services& services){
    auto& g=a.gp;
    const auto push=[&](std::uint32_t pc,std::uint32_t value){g[4]-=4u;store(m,{},pc,g[4],value);};
    const auto pop=[&](){const auto value=u32(m,g[4]);g[4]+=4u;return value;};
    push(0x227140u,g[3]);g[3]=u32(m,g[4]+0xcu);
    if(g[3]){
        g[0]=u32(m,g[4]+8u);g[2]=u32(m,0x52d674u);
        const bool negative=std::bit_cast<std::int32_t>(g[0])<0;
        g[1]=negative?0u:s16(m,g[2]+4u)-1u;
        if(!negative && std::bit_cast<std::int32_t>(g[0])<=std::bit_cast<std::int32_t>(g[1])){
            g[0]*=0x34u;g[1]=u32(m,g[0]+g[2]+0x3cu);
            push(0x22716fu,g[6]);push(0x227170u,g[7]);g[7]=u32(m,g[4]+0x18u);
            g[6]=g[0]+g[2]+0xcu;g[0]=u32(m,g[1]);push(0x22717bu,g[7]);push(0x22717cu,g[3]);push(0x22717du,g[6]);
            call_out(m,a,services,o,0x22717eu,u32(m,g[0]+0x18u),0x227181u,{},true);
            if(!g[7] && !(u8(m,g[6]+3u)&0xcu)){
                g[0]=g[6];call_out(m,a,services,o,0x22718du,0x2b8530u,0x227192u,{},true);
            }
            g[7]=pop();g[6]=pop();
        }
    }
    g[3]=pop();pop_return(m,a,12);
}

// Every kernel reproduces all eight general registers, so none needs a
// Contract exclusion.
constexpr Kernel resource_table[]{
    {0x13fe00u,page_prefer},
    {0x13fe50u,page_allocate,{},true},
    {0x140310u,page_delete,{},true},
    {0x16e6a0u,allocate_datum},
    {0x16e770u,allocate_specific},
    {0x16e860u,initialize_record},
    {0x16e8a0u,delete_datum},
    {0x129470u,release_binding},
    {0x12b1c0u,rebind},
    {0x1296e0u,sound_playable},
    {0x128970u,binding_ready,{},true},          // may call 21F420
    {0x1293f0u,lock_binding_entry},
    {0x21f260u,request_chunk_entry,{},true},    // may call 21F420 and 2DBF74
    {0x21f3b0u,cache_entry_retained},
    {0x21f3f0u,cache_page_deleted},
    {0x21f360u,chunk_data},
    {0x2b98d0u,stream_packet},
    {0x2b9950u,stream_packet_submitted},
    {0x2b82a0u,stream_retire},
    {0x2b99c0u,stream_chunk_finished},
    {0x2e4d60u,stock_queue_noop},
    {0x2b8210u,stream_enqueue,{},true},
    {0x2b85f0u,stream_release_queued},
    {0x2b9a00u,stream_packet_available},
    {0x2b9980u,stream_packet_failed},
    {0x2b8530u,stream_pump,{},true},
    {0x227140u,stream_completed,{},true},
};
} // namespace
std::span<const Kernel> resource_kernels(){return resource_table;}
} // namespace halo2::native_tu15_sound
