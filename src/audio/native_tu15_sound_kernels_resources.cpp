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

namespace halo2::native_tu15_sound {
namespace {
// Retail globals read by the sound cache.
constexpr std::uint32_t sound_cache=0x50ebdcu;      // +0x44: 16-byte cache entries
constexpr std::uint32_t cache_allocator=0x50ebe4u;  // +0x38 stamp; +0x64 -> +0x44: 24-byte blocks (+0x14 last use)
constexpr std::uint32_t binding_table=0x4f2c4cu;    // +0x44: 0xBC-byte sound bindings
constexpr std::uint32_t codec_playback=0x48b8fcu;   // .data dword (0 in the XBE image), read at run time
constexpr std::uint32_t no_handle=0xffffffffu;

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
    const auto ebx=a.gp[3],esi=a.gp[6],edi=a.gp[7];
    a.gp[4]=frame-0x14u;                        // sub esp,8; push ebx; push esi; push edi
    const auto wait=request&1u,lock=(request>>2)&1u;
    a.gp[0]=request;
    a.gp[1]=low_byte(a.gp[1],wait);
    a.gp[2]=0u;
    a.gp[3]=low_byte(request>>2,lock);
    a.gp[7]=row;
    a.gp[6]=u32(m,row+8u);
    if(a.gp[6]==no_handle) {
        a.gp[6]=datum;
        if(datum!=no_handle) {
            a.gp[0]=request>>1;
            if(a.gp[0]&1u) {
                // push esi (datum); mov ebx,edi; call 21F420.
                a.gp[3]=row;
                const std::uint32_t arguments[]{datum};
                call_out(m,a,services,o,0x21f2a3u,0x21f420u,0x21f2a8u,arguments,true);
                a.gp[1]=low_byte(a.gp[1],wait);
                a.gp[3]=low_byte(a.gp[3],lock);
                a.gp[2]=0u;
            }
        }
    }
    std::uint32_t state=0u;
    a.gp[0]=u32(m,row+8u);
    if(a.gp[0]!=no_handle) {
        const auto block=(a.gp[0]&0xffffu)*3u;
        const auto allocator=u32(m,cache_allocator);
        // EBP is pushed and popped around the block lookup; it is unchanged.
        const auto blocks=u32(m,u32(m,allocator+0x64u)+0x44u);
        const auto stamp=u32(m,allocator+0x38u);
        store(m,o,0x21f2d3u,blocks+block*8u+0x14u,stamp);
        const auto handle=u32(m,row+8u);
        a.gp[0]=u32(m,sound_cache);
        const auto entry=((handle&0xffffu)<<4)+u32(m,a.gp[0]+0x44u);
        a.gp[6]=entry;
        // Each test of the entry's bytes goes through AL.
        const auto al=[&](std::uint32_t at){const auto v=u8(m,at);a.gp[0]=low_byte(a.gp[0],v);return v!=0u;};
        bool loaded;
        if(wait) {
            loaded=al(entry+2u) || al(entry+2u);
            if(!loaded) {
                do call_out(m,a,services,o,0x21f300u,0x2dbf74u,0x21f305u,{},false);
                while(!al(entry+2u));
                a.gp[2]=0u;                     // EDX reloaded from the frame's state
                loaded=al(entry+2u);
            }
        } else loaded=al(entry+2u);
        if(loaded) {
            if(!al(entry+3u))store8(m,o,0x21f31eu,entry+3u,1u);
            state=2u;
        }
        if(lock) {
            store8(m,o,0x21f332u,entry+4u,u8(m,entry+4u)+1u);
            state|=4u;
        }
    }
    if(!(state&2u) && u32(m,row+8u)!=no_handle)state|=1u;
    a.gp[0]=a.gp[2]=state;
    a.gp[3]=ebx;a.gp[6]=esi;a.gp[7]=edi;
    a.gp[4]+=0x14u;                             // pop edi/esi/ebx; add esp,8
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

// Every kernel reproduces all eight general registers, so none needs a
// Contract exclusion.
constexpr Kernel resource_table[]{
    {0x129470u,release_binding},
    {0x12b1c0u,rebind},
    {0x1296e0u,sound_playable},
    {0x128970u,binding_ready,{},true},          // may call 21F420
    {0x1293f0u,lock_binding_entry},
    {0x21f260u,request_chunk_entry,{},true},    // may call 21F420 and 2DBF74
    {0x21f3b0u,cache_entry_retained},
};
} // namespace
std::span<const Kernel> resource_kernels(){return resource_table;}
} // namespace halo2::native_tu15_sound
