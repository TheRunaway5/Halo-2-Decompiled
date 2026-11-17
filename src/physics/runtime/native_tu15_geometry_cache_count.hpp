#pragma once
// Boundary adapter for the geometry cache's pending read count
// (native_tu15_geometry_cache_count_systems.hpp), entered at 1306B0 in the
// geometry cache frame unit. Its callers are still translated TU1.5 code, so
// this adapter owns that ABI and nothing else:
//   - the result in EAX, and the registers the original leaves behind: ECX
//     and EDX keep the scan's last values (the probe's word index, the
//     datum's address, the index), EBX/EBP/ESI/EDI come back from the stack;
//   - the flags of the instruction that decides each exit: TEST DL,DL, the
//     scan's INC EDX or CMP EDX,EDI/-1, or the ADD of a null datum;
//   - the register saves and restores, and the RET;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Delete the adapter when the callers go native.
// HALO2_REFERENCE_GEOMETRY_CACHE_COUNT=1 restores the translated body.
// HALO2_VERIFY_GEOMETRY_CACHE_COUNT=1 records the native run and replays the
// unmodified translated body against it (native_tu15_object_tick.hpp): same
// ordered accesses, stores and final registers, flags, XMM, x87 and MXCSR.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_geometry_cache_count_systems.hpp"

namespace halo2::native_geometry_cache_count_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_geometry_cache;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_GEOMETRY_CACHE_COUNT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_GEOMETRY_CACHE_COUNT");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    disabled, scan_negative, scan_bound, scan_end, scan_invalid, null_record, skipped, counted, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Geometry cache count verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// ---- 1306B0 -------------------------------------------------------------------
// No arguments. EAX = the count. The scan keeps the array in EBX, the index
// in EDX, the limit in EDI, the bit vector (then the datums) in ESI and the
// probe's mask and word in EBP and ECX, as the original does.
template<class State> struct Count : Port<State> {
    using Access=logic::pending_reads::Access;
    native_object_tick_detail::SearchRegisters search;
    Count(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case predicted_array:return 0x1306b0u;
        case predicted_valid:return 0x1306b6u;
        case cache_array:this->push(0x1306bfu,gp[ebx]);return 0x1306c0u;
        case scan_limit:++gp[edx];return 0x1306d3u;                          // INC EDX
        case scan_bits:return 0x1306dau;
        case scan_word:search.probe(gp,edx,ebp,ecx);return 0x1306f1u;
        case datum_size:return 0x130705u;
        case datum_data:return 0x130708u;
        case ready:return 0x130712u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case predicted_array:case datum_size:gp[ecx]=v;break;
        // MOV DL; XOR EAX,EAX; TEST DL,DL.
        case predicted_valid:set_low_byte(gp[edx],v);gp[eax]=0;this->test8(v);break;
        // MOV EBX; PUSH EBP/ESI; OR EDX,-1; PUSH EDI.
        case cache_array:
            gp[ebx]=v;this->push(0x1306c6u,gp[ebp]);this->push(0x1306c7u,gp[esi]);
            gp[edx]=logic::none;this->push(0x1306cdu,gp[edi]);
            break;
        case scan_limit:gp[edi]=v;break;
        case scan_bits:gp[esi]=v;search.first_probe=true;break;
        // IMUL ECX,EDX; ADD ECX,ESI (decides a null datum's exit).
        case datum_data:gp[esi]=v;gp[ecx]=this->flags.plus(gp[ecx]*gp[edx],v,4);break;
        case ready:this->reached(v?skipped:counted);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The scan's end: INC EDX (CF is always clear before it: TEST, CMP byte,0
    // or the INC EAX after it) or CMP EDX,EDI / CMP EDX,-1.
    void scan_ended(const logic::Search &search) {
        auto &gp=this->gp;
        gp[edx]=search.index;
        switch(*search.end) {
        case logic::SearchEnd::negative_start:
            this->flags.cf=false;this->flags.increment(search.index-1u,4,false);break;
        case logic::SearchEnd::past_limit:this->compare32(search.index,gp[edi]);break;      // CMP EDX,EDI; JGE
        case logic::SearchEnd::bits_exhausted:this->compare32(search.index,gp[edi]);break;  // CMP EDX,EDI; JL
        case logic::SearchEnd::invalid_index:this->compare32(search.index,logic::none);break;
        }
        this->reached(scan_negative+unsigned(*search.end));  // negative, bound, end, invalid are consecutive
    }
    void run() {
        auto &gp=this->gp;
        Memory<Count> memory{*this};
        const auto result=logic::pending_reads::count(memory);
        gp[eax]=result.pending;
        if(result.exit==logic::pending_reads::Exit::no_caches) {
            this->reached(disabled);
            this->ret(0x1306ffu,0);this->publish(0x1306ffu);return;
        }
        if(result.exit==logic::pending_reads::Exit::scan_ended)scan_ended(result.scan);
        else this->reached(null_record);
        gp[edi]=this->pop(0x1306fbu);gp[esi]=this->pop(0x1306fcu);gp[ebp]=this->pop(0x1306fdu);gp[ebx]=this->pop(0x1306feu);
        this->ret(0x1306ffu,0);this->publish(0x1306ffu);
    }
};

template<class State,class Reference> bool bridge(State &s,Reference reference) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1306b0u,0x1306ffu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Count<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_geometry_cache_count_detail
