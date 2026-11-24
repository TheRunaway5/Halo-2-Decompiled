#pragma once
// Boundary adapter for the shadow schedule, 4AC20
// (native_tu15_shadow_schedule_systems.hpp). Its callers and 4B150 keep the
// original ABI, which the adapter owns: the cache, the output, the visible
// count and dt on the stack (RET 10), EAX the count; 0x30 bytes of frame
// with EBX, EBP, ESI and EDI saved below them, read from the ESP 4B150
// left. 4B150 (EAX the front; the cache, the sort and the last pushed, RET
// C) is direct; every register it returns comes back. The swaps keep the
// original's REP MOVSD (ESI, EDI and ECX stepping, in the direction flag's
// direction); the weights keep XMM0, XMM1 (zero) and XMM2 (dt) as the
// original's SSE leaves them. Each access publishes its original instruction.
// HALO2_REFERENCE_SHADOW_SCHEDULE=1 restores the translated body.
// HALO2_VERIFY_SHADOW_SCHEDULE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/shadows/native_tu15_shadow_schedule_systems.hpp"

namespace halo2::native_shadow_schedule_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_shadow_schedule;
using Access=logic::shadow_schedule::Access;
using Kept=logic::shadow_schedule::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SHADOW_SCHEDULE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SHADOW_SCHEDULE");return v;}
enum Branch : unsigned {paused, fronted, activated, stale, repeated, faded_four, faded, grown, clamped_zero, clamped_maximum, output,
    branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Shadow schedule"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr std::uint32_t sites[]{
    0x4ac20u,0x4ac24u,0x4ac39u,0x4ac43u,0x4ac4eu,0x4ac56u,0x4ac5du,0x4ac63u,0x4ac67u,0x4ac71u,0x4ac7du,0x4ac82u,0x4ac93u,0x4ac93u,
    0x4ac9eu,0x4ac9eu,0x4acabu,0x4acabu,0x4acadu,0x4acbau,0x4acc2u,0x4acd5u,0x4ace0u,0x4ad01u,0x4ad01u,0x4ad03u,0x4ad0eu,0x4ad0eu,
    0x4ad10u,0x4ad1eu,0x4ad1eu,0x4ad20u,0x4ad24u,0x4ad28u,0x4ad33u,0x4ad3bu,0x4ad4du,0x4ad54u,0x4ad57u,0x4ad5fu,0x4ad6fu,0x4ad6fu,
    0x4ad7au,0x4ad7au,0x4ad8cu,0x4ad8cu,0x4ad99u,0x4ad9du,0x4adaeu,0x4adb6u,0x4add4u,0x4addbu,0x4ade0u,0x4adeau,0x4adf8u,0x4ae06u,
    0x4ae06u,0x4ae0cu,0x4ae15u,0x4ae15u,0x4ae22u,0x4ae22u,0x4ae24u,0x4ae28u,0x4ae36u,0x4ae47u,0x4ae55u,0x4ae5fu,0x4ae90u,0x4ae97u,
    0x4aea0u,0x4aea9u,0x4aeaeu,0x4aeb5u,0x4aebeu,0x4aec4u,0x4aec8u,0x4aeceu,0x4aed3u,0x4aed9u,0x4aeddu,0x4aee3u,0x4aee8u,0x4aeefu,
    0x4aef9u,0x4af02u,0x4af07u,0x4af0fu,0x4af19u,0x4af20u,0x4af25u,0x4af2bu,0x4af30u,0x4af37u,0x4af3cu,0x4af43u,0x4af48u,0x4af4fu,
    0x4af59u,0x4af62u,0x4af67u,0x4af6fu,0x4af79u,0x4af80u,0x4af85u,0x4af8bu,0x4af90u,0x4af97u,0x4af9cu,0x4afa3u,0x4afa8u,0x4afb2u,
    0x4afbcu,0x4afc5u,0x4afcau,0x4afd2u,0x4afdcu,0x4afe3u,0x4afe8u,0x4afeeu,0x4aff3u,0x4affau,0x4afffu,0x4b006u,0x4b025u,0x4b02cu,
    0x4b035u,0x4b03eu,0x4b043u,0x4b04au,0x4b053u,0x4b059u,0x4b05du,0x4b063u,0x4b068u,0x4b06eu,0x4b072u,0x4b078u,0x4b085u,0x4b092u,
    0x4b097u,0x4b09cu,0x4b0a5u,0x4b0aau,0x4b0aeu,0x4b0b6u,0x4b0bfu,0x4b0c5u,0x4b0c9u,0x4b0cfu,0x4b0d4u,0x4b0dau,0x4b0deu,0x4b0e4u,
    0x4b0efu,0x4b0f3u,0x4b0ffu,0x4b107u,0x4b10bu,0x4b10du,0x4b110u,0x4b113u,0x4b116u,0x4b119u,0x4b11cu,0x4b11fu,0x4b122u,0x4b125u,
    0x4b128u,0x4b12bu,0x4b13cu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::count_final)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // 4B150
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case arg_cache:case count_load:case cache_again:case count_weights:case count_output:case count_final:gp[eax]=v;break;
        case pause_load:case limit_load:case limit_original:case record_stamp:case key_load:case output_load:gp[ecx]=v;break;
        case stale_flag:case repeat_flag:set_low_byte(gp[ecx],v);break;                                     // MOV CL
        case pause_flag:case decay_flag_rest:set_low_byte(gp[edx],v);break;                                // MOV DL
        case decay_flag_0:case decay_flag_1:case decay_flag_2:case decay_flag_3:set_low_byte(gp[ebx],v);break;                                                                   // MOV BL
        case cache_load:case front_load:case copy_load_0:case copy_load_1:case copy_load_2:case copy_load_3:gp[ebp]=v;break;
        case index_reload:case front_record_load:case front_sort:case front_reload:case visible_load:case visible_reload:
        case cache_output:gp[esi]=v;break;
        case front_record_again:case cache_sort:case cache_reload:case weight_load:case count_reload:case copy_load_4:gp[edi]=v;break;
        case index_sort_load:gp[edx]=v;break;
        case dt_load:x[2]=lane0(v);break;                                                                  // MOVSS XMM2
        case decay_weight_0:case decay_rate_0:case decay_again_0:case decay_max_load_0:case decay_keep_load_0:case decay_weight_1:case decay_rate_1:case decay_again_1:case decay_max_load_1:case decay_keep_load_1:case decay_weight_2:case decay_rate_2:case decay_again_2:case decay_max_load_2:case decay_keep_load_2:case decay_weight_3:case decay_rate_3:case decay_again_3:case decay_max_load_3:case decay_keep_load_3:case decay_weight_rest:case decay_rate_rest:case decay_again_rest:case decay_max_load_rest:case decay_keep_load_rest:case grow_max:case grow_rate:case grow_load:case grow_again:case grow_max_load:case grow_keep_load:x[0]=lane0(v);break;                                                                           // MOVSS XMM0
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {                                                  // REP MOVSD: ESI, EDI and ECX step
        using enum Access;
        auto &gp=this->gp;
        switch(a){case front_save_store:case front_move_store:case front_restore_store:case active_save_store:case active_move_store:case active_restore_store:case stale_save_store:case stale_move_store:case stale_restore_store:case repeat_save_store:case repeat_move_store:case repeat_restore_store:{const auto d=step();gp[esi]+=d;gp[edi]+=d;--gp[ecx];}break;default:break;}
    }
    // The frame and the front.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x30u,4);depth=0u-16u;}               // SUB ESP,30
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x4ac2fu,0x4ac30u,0x4ac31u,0x4ac32u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    bool positive_count(std::uint32_t v){this->test32(v);return std::int32_t(v)>0;}                          // TEST reg,reg; JLE
    bool present(std::uint32_t v){this->test32(v);return v!=0u;}                                              // TEST ECX,ECX
    bool set(std::uint32_t v) {                                                                          // TEST reg8,reg8
        this->test8(v);
        return (v&0xffu)!=0u;
    }
    bool flagged(std::uint32_t v){this->flags.minus(v&0xffu,0u,1);return (v&0xffu)!=0u;}                      // CMP byte,0
    bool paused(std::uint32_t v){this->test8(v);if(v&0xffu)this->reached(Branch::paused);return (v&0xffu)!=0u;}   // TEST DL,DL
    std::uint32_t no_front(){return this->gp[edx]=this->flags.logic(0u,4);}                                   // XOR EDX,EDX
    bool above_int(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>std::int32_t(b);}   // CMP; JLE
    std::uint32_t last_record(std::uint32_t n){return this->gp[ebx]=n-1u;}                                    // LEA EBX,[EAX-1]
    std::uint32_t first_index(){return this->gp[esi]=this->flags.logic(0u,4);}                                // XOR ESI,ESI
    bool unmoved(){set_low_byte(this->gp[ecx],this->flags.logic(0u,1));return false;}                          // XOR CL,CL
    bool non_negative(std::uint32_t v){this->test32(v);return std::int32_t(v)>=0;}                            // TEST EBX,EBX; JL
    std::uint32_t cursor_start(std::uint32_t base){return this->gp[eax]=base;}                                // MOV EAX,EBP
    bool before(std::uint32_t index,std::uint32_t last){this->compare32(last,index);return std::int32_t(last)>std::int32_t(index);}   // CMP EBX,ESI; JLE
    std::uint32_t copying(unsigned,std::uint32_t from,std::uint32_t to) {                                // (MOV/LEA ESI, EDI); MOV ECX,9
        auto &gp=this->gp;
        gp[esi]=from;gp[edi]=to;gp[ecx]=logic::record_words;
        return from;
    }
    std::uint32_t moved_front(std::uint32_t front){this->reached(Branch::fronted);return this->gp[edx]=this->flags.increment(front,4,false);}   // INC EDX
    bool moved(){set_low_byte(this->gp[ecx],1u);return true;}                                                  // MOV CL,1
    std::uint32_t next_index(std::uint32_t i){return this->gp[esi]=this->flags.increment(i,4,false);}         // INC ESI
    std::uint32_t next_cursor(std::uint32_t c){return this->gp[eax]=this->flags.plus(c,logic::record::size,4);}   // ADD EAX,24
    bool not_before(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>=std::int32_t(b);}   // CMP; JGE / JL
    void moved_test(bool){this->test8(this->gp[ecx]);}                                                        // TEST CL,CL
    bool within(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<=std::int32_t(b);}   // CMP; JG / JLE
    std::uint32_t front_record(std::uint32_t front,std::uint32_t base) {                                 // MOV EAX,EDX; LEA ECX,[EDX+EDX*8]; LEA ECX,[EBP+ECX*4]
        auto &gp=this->gp;
        gp[eax]=front;
        return gp[ecx]=base+logic::record::size*front;
    }
    std::uint32_t next_active(std::uint32_t i){return this->gp[eax]=this->flags.increment(i,4,false);}        // INC EAX
    std::uint32_t next_active_record(std::uint32_t r){return this->gp[ecx]=this->flags.plus(r,logic::record::size,4);}   // ADD ECX,24
    std::uint32_t found_record(std::uint32_t i,std::uint32_t base){this->reached(Branch::activated);return this->gp[eax]=base+logic::record::size*i;}   // LEA EAX x2
    // The stale records.
    void weightless(){this->s.xmm[1]=_mm_setzero_ps();}                                                       // XORPS XMM1,XMM1
    std::uint32_t stale_cursors(std::uint32_t cache,std::uint32_t last,std::uint32_t index) {            // MOV ECX,EBP; LEA EDX/ECX x2; LEA EDX; LEA EAX
        auto &gp=this->gp;
        gp[ecx]=9u*index;gp[edx]=cache+logic::record::size*last;gp[eax]=cache+logic::record::size*index;
        return gp[edx];
    }
    bool same(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a==b;}                            // CMP
    std::uint32_t zero()const{return low(this->s.xmm[1]);}                                                     // MOVSS [..],XMM1
    std::uint32_t earlier(std::uint32_t v,bool last) {                                                   // DEC EBX / DEC EBP
        if(last)this->reached(Branch::stale);
        return this->gp[last?ebx:ebp]=this->flags.increment(v,4,true);
    }
    std::uint32_t earlier_record(std::uint32_t r,unsigned which) {                                       // SUB EAX / EDX / EBP,24
        static constexpr unsigned regs[]{eax,edx,ebp};
        return this->gp[regs[which]]=this->flags.minus(r,logic::record::size,4);
    }
    std::uint32_t next_stale(std::uint32_t i){return this->gp[ebp]=this->flags.increment(i,4,false);}         // INC EBP
    std::uint32_t next_stale_record(std::uint32_t r){return this->gp[eax]=this->flags.plus(r,logic::record::size,4);}   // ADD EAX,24
    // The sorts and the repeated keys.
    void sorted(Kept &kept,std::uint32_t mode) {                                                         // PUSH EBX; PUSH n; PUSH EDI; MOV EAX,ESI; CALL 4B150
        auto &gp=this->gp;
        const auto pc=mode==logic::first_sort?0x4ada1u:0x4ae3cu;
        this->push(pc,kept.last);this->push(pc+1u,mode);this->push(pc+3u,kept.cache_address);gp[eax]=kept.front;
        auto child=[&](State &v){children(v,0x4b150u);};
        this->call(pc+6u,pc+11u,0x4b150u,child);
        kept.last=gp[ebx];kept.base=gp[ebp];kept.front=gp[esi];kept.cache_address=gp[edi];
    }
    std::uint32_t repeat_start(std::uint32_t front){return this->gp[edx]=front;}                              // MOV EDX,ESI
    std::uint32_t repeat_cursors(const Kept &kept) {                                                     // LEA EAX; LEA ECX; LEA EBP; LEA EAX
        auto &gp=this->gp;
        gp[ecx]=9u*kept.front;gp[ebp]=kept.cache_address+logic::record::size*kept.last;
        gp[eax]=kept.cache_address+logic::record::size*kept.front+logic::record::key;
        return gp[ebp];
    }
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP ECX,-1
    std::uint32_t repeated(std::uint32_t r){this->reached(Branch::repeated);return this->gp[edx]=r;}          // LEA EDX,[EAX-8]
    std::uint32_t negated(std::uint32_t dt) {                                                            // MOVAPS XMM0,XMM1; MOV ESI,EDX; SUBSS XMM0,XMM2
        auto &x=this->s.xmm;
        x[0]=x[1];this->gp[esi]=this->gp[edx];
        x[0]=_mm_move_ss(x[0],lane0(logic::sse::subtract(low(x[0]),dt)));
        return low(x[0]);
    }
    void repeat_move_registers(std::uint32_t end,std::uint32_t r){auto &gp=this->gp;gp[esi]=end;gp[edi]=r;}   // MOV ESI,EBP; MOV EDI,EDX
    std::uint32_t next_repeat(std::uint32_t i){return this->gp[edx]=this->flags.increment(i,4,false);}        // INC EDX
    std::uint32_t next_key(std::uint32_t k){return this->gp[eax]=this->flags.plus(k,logic::record::size,4);}  // ADD EAX,24
    bool after_int(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)>std::int32_t(b);}   // CMP EDX,EBX; JLE
    // The weights.
    bool covers(std::uint32_t limit,std::uint32_t total){this->compare32(limit,total);return std::int32_t(limit)>=std::int32_t(total);}   // CMP [ESP+18],EAX; JGE
    std::uint32_t last_index(std::uint32_t total){return this->gp[ecx]=total-1u;}                             // LEA ECX,[EAX-1]
    bool four_or_more(std::uint32_t last,std::uint32_t visible) {                                        // MOV EDX,ECX; SUB EDX,ESI; INC EDX; CMP EDX,4; JL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.increment(fl.minus(last,visible,4),4,false);
        this->compare32(gp[edx],logic::unrolled);
        const bool four=std::int32_t(gp[edx])>=std::int32_t(logic::unrolled);
        if(four)this->reached(Branch::faded_four);
        return four;
    }
    std::uint32_t passes(std::uint32_t last,std::uint32_t visible) {                                     // MOV EDX,ECX; SUB EDX,ESI; SUB EDX,3; SHR EDX,2; INC EDX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.minus(fl.minus(last,visible,4),logic::unrolled-1u,4);
        gp[edx]=fl.increment(fl.right(gp[edx],2u,4,false),4,false);
        return gp[edx];
    }
    std::uint32_t fade_cursor(std::uint32_t cache,std::uint32_t last) {                                  // MOV EBX,EDX; LEA EAX,[ECX+ECX*8]; LEA EAX,[EDI+EAX*4+18]
        auto &gp=this->gp;
        gp[ebx]=gp[edx];
        return gp[eax]=cache+logic::record::size*last+logic::record::weight;
    }
    std::uint32_t after_passes(std::uint32_t last,std::uint32_t passes_value) {                          // NEG EBX; LEA ECX,[ECX+EBX*4]
        auto &gp=this->gp;
        gp[ebx]=this->flags.minus(0u,passes_value,4);
        return gp[ecx]=last+4u*gp[ebx];
    }
    std::uint32_t next_pass(std::uint32_t a){return this->gp[eax]=this->flags.minus(a,logic::unrolled*logic::record::size,4);}   // SUB EAX,90
    std::uint32_t counted_pass(std::uint32_t p){return this->gp[edx]=this->flags.increment(p,4,true);}        // DEC EDX
    std::uint32_t rest(std::uint32_t last,std::uint32_t visible) {                                       // LEA EDX,[ECX+ECX*8]; SUB ECX,ESI; INC ECX
        auto &gp=this->gp;auto &fl=this->flags;
        this->reached(Branch::faded);
        gp[edx]=9u*last;
        return gp[ecx]=fl.increment(fl.minus(last,visible,4),4,false);
    }
    std::uint32_t rest_cursor(std::uint32_t cache,std::uint32_t last){return this->gp[eax]=cache+logic::record::size*last+logic::record::weight;}   // LEA EAX,[EDI+EDX*4+18]
    std::uint32_t previous_record(std::uint32_t a){return this->gp[eax]=this->flags.minus(a,logic::record::size,4);}   // SUB EAX,24
    std::uint32_t counted_left(std::uint32_t c){return this->gp[ecx]=this->flags.increment(c,4,true);}        // DEC ECX
    std::uint32_t grow_cursor(std::uint32_t cache) {                                                     // LEA EAX,[EDI+18]; MOV ECX,ESI
        auto &gp=this->gp;
        this->reached(Branch::grown);
        gp[ecx]=gp[esi];
        return gp[eax]=cache+logic::record::weight;
    }
    std::uint32_t next_record(std::uint32_t a){return this->gp[eax]=this->flags.plus(a,logic::record::size,4);}   // ADD EAX,24
    bool above(std::uint32_t a,std::uint32_t b) {                                                        // COMISS XMM0,[m]; JBE
        auto &fl=this->flags;
        fl.scalar_compare(lane0(a),lane0(b),false);
        return !(fl.cf||fl.zf);
    }
    bool positive(std::uint32_t w){return above(w,low(this->s.xmm[1]));}                                       // COMISS XMM0,XMM1; JBE
    bool exceeds(std::uint32_t w,std::uint32_t maximum){const bool past=above(w,maximum);if(past)this->reached(Branch::clamped_maximum);return past;}   // COMISS XMM0,[EAX-C]; JBE
    std::uint32_t summed(std::uint32_t v){auto &x=this->s.xmm;x[0]=_mm_move_ss(x[0],lane0(v));return v;}       // ADDSS XMM0,..
    bool negative(std::uint32_t sum) {                                                                   // COMISS XMM1,XMM0; JBE
        auto &fl=this->flags;
        fl.scalar_compare(this->s.xmm[1],lane0(sum),false);
        return !(fl.cf||fl.zf);
    }
    std::uint32_t zeroed_weight(){this->reached(Branch::clamped_zero);this->s.xmm[0]=this->s.xmm[1];return low(this->s.xmm[0]);}   // MOVAPS XMM0,XMM1
    // The output.
    std::uint32_t output_start(){this->reached(Branch::output);return this->gp[edx]=this->flags.logic(0u,4);}  // XOR EDX,EDX
    std::uint32_t output_cursor(std::uint32_t cache){return this->gp[eax]=cache;}                             // MOV EAX,ESI
    void output_registers(std::uint32_t r,std::uint32_t out){auto &gp=this->gp;gp[edi]=r;gp[ebx]=out;}         // MOV EDI,EAX; MOV EBX,ECX
    std::uint32_t next_output(std::uint32_t i){return this->gp[edx]=this->flags.increment(i,4,false);}        // INC EDX
    std::uint32_t next_output_record(std::uint32_t r){return this->gp[eax]=this->flags.plus(r,logic::record::size,4);}   // ADD EAX,24
    std::uint32_t next_output_entry(std::uint32_t o){return this->gp[ecx]=this->flags.plus(o,logic::output::size,4);}   // ADD ECX,14
    bool before_int(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP EDX,EDI; JL
    void finish() {                                                                                      // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,30; RET 10
        auto &gp=this->gp;
        gp[edi]=this->pop(0x4b142u);gp[esi]=this->pop(0x4b143u);gp[ebp]=this->pop(0x4b144u);gp[ebx]=this->pop(0x4b145u);
        gp[esp]=this->flags.plus(gp[esp],0x30u,4);
        this->ret(0x4b149u,0x10u);this->publish(0x4b149u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        depth=0u-0x40u;
        logic::shadow_schedule::schedule(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        finish();
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x4ac20u,0x4b149u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_shadow_schedule_detail
