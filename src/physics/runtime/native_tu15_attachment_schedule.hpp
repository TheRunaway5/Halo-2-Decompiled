#pragma once
// Boundary adapter for the object's marker attachments
// (native_tu15_attachment_schedule_systems.hpp), entered at 41970 in the
// model render unit. Its caller (the render prepare 41C10) and its children
// (32A660, BAD60, C0700, BCD80, 3E9B0, 2DB80, 42840, 42990) are still
// translated TU1.5 code, so this adapter owns that ABI and nothing else:
//   - the arguments: [ESP+4] the skip word, [ESP+8] the object handle (RET 8);
//   - the stack frame: 32A660 allocates the 0x1CA0-byte locals (the logic's
//     Locals) below the return address; EDI, ESI, EBX and EBP are saved below
//     them. Frame::local gives a local's address as the original addresses
//     it, from the stack pointer as the last child left it;
//   - the original's register file at every child call: the logic's Walk in
//     ESI (object), EBP (attachment), EDI (index, across BAD60) and EBX (the
//     marker count, then the current marker's up axis), handed back after
//     each call as the callee returns it; the register arguments (ECX, EDX,
//     EAX, EDI, XMM0) and the values the original leaves in scratch
//     registers; the flags of the instruction that decides each call, or the
//     previous child's;
//   - the children's stack arguments, pushed between the logic's reads as
//     the original pushes them, and their return addresses;
//   - the exit flags (ADD ESP,1CA0) and the RET 8;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Every child stays genuine and is entered through the unit's dispatch with
// the exact register, flag, XMM, x87 and stack ABI. Delete the adapter when
// the caller and the children go native.
// HALO2_REFERENCE_ATTACHMENT_SCHEDULE=1 restores the translated body.
// HALO2_VERIFY_ATTACHMENT_SCHEDULE=1 records the native run (children
// genuine, once) and replays the unmodified translated body against it, as
// in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_attachment_schedule_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_attachment_schedule_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_attachments;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ATTACHMENT_SCHEDULE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ATTACHMENT_SCHEDULE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    busy_mode, no_attachments, other_tag, no_marker, resolved, offset_word, no_offset_word, no_markers,
    lens, lens_scaled, lens_plain, lens_full, lens_full_set, lens_full_seen, mgs2, mgs2_scaled, mgs2_plain_count,
    mgs2_plain_flag, tdtl, unknown_marker, more_markers, last_marker, more_attachments, last_attachment, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Attachment schedule verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
// MOVSS xmm,m32: lane 0 loaded, upper lanes zeroed.
inline __m128 load_ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}

// ---- 41970 --------------------------------------------------------------------
// Coverage: the branch outcome is the original instruction that follows.
inline constexpr Edge marker_attachment_edges[]{
    {0x4197du,0x41bfbu,busy_mode},{0x419d1u,0x41bfau,no_attachments},
    {0x419ebu,0x41bd2u,other_tag},{0x41a0au,0x41bd2u,no_marker},{0x41a0au,0x41a14u,resolved},
    {0x41a56u,0x41a61u,offset_word},{0x41a56u,0x41a7au,no_offset_word},{0x41a7au,0x41bd2u,no_markers},
    {0x41a90u,0x41a9eu,lens},{0x41a9eu,0x41bc2u,lens_full},{0x41bc2u,0x41bcbu,lens_full_set},{0x41bc2u,0x41bd2u,lens_full_seen},
    {0x41ad0u,0x41ad8u,lens_scaled},{0x41ad0u,0x41ae3u,lens_plain},
    {0x41a90u,0x41b34u,mgs2},{0x41b54u,0x41b68u,mgs2_plain_count},{0x41b5eu,0x41b63u,mgs2_scaled},{0x41b5eu,0x41b68u,mgs2_plain_flag},
    {0x41a90u,0x41b8au,tdtl},{0x41a90u,0x41ba8u,unknown_marker},
    {0x41bb6u,0x41a90u,more_markers},{0x41bb6u,0x41bd2u,last_marker},
    {0x41beeu,0x419e1u,more_attachments},{0x41beeu,0x41bf8u,last_attachment},
};
template<class State,class Dispatch> struct Schedule : Port<State> {
    using Access=logic::marker_attachments::Access;
    using Local=logic::marker_attachments::Local;
    using Walk=logic::marker_attachments::Walk;
    Dispatch &dispatch;
    std::uint32_t locals{};   // the Locals as the original addresses them
    Schedule(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,marker_attachment_edges),dispatch(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case skip:return 0x4197du;
        case object_table:return 0x4198bu;
        case object_rows:return 0x41991u;
        case object_argument:this->push(0x41994u,gp[esi]);return 0x41995u;
        case object_index_store:return 0x419a3u;
        case object:gp[eax]*=3u;return 0x419aau;                               // LEA EAX,[EAX+EAX*2]
        case definition_tag:return 0x419aeu;
        case tag_instances:return 0x419b0u;
        case definition_data:return 0x419bfu;
        case attachment_count:return 0x419c3u;
        case definition_store:return 0x419c9u;
        case lens_flares_clear:return 0x419cdu;
        case index_clear:return 0x419d1u;
        case offset_clear:this->push(0x419dbu,gp[ebx]);return 0x419dcu;
        case attachment_list:return 0x419e1u;
        case offset_read:return 0x419e7u;
        case attachment_group:return 0x419ebu;
        case attachment_tag:return 0x41a0au;
        case marker_name:return 0x41a14u;
        case marker_count_store:return 0x41a2du;
        case white:return 0x41a36u;
        case white_red:return 0x41a3bu;
        case red_store:return 0x41a3du;
        case white_green:return 0x41a41u;
        case green_store:return 0x41a44u;
        case white_blue:return 0x41a48u;
        case blue_store:return 0x41a4bu;
        case change_color:return 0x41a4fu;
        case primary_scale_store:return 0x41a56u;
        case object_for_color:gp[edi]=sx16(gp[eax]);return 0x41a61u;          // MOVSX EDI,AX
        case object_after_color:return 0x41a71u;
        case marker_index_clear:this->test32(gp[ebx]);return 0x41a7au;          // TEST EBX,EBX (the count)
        case marker_group:return 0x41a90u;
        case lens_count:return 0x41a9eu;
        case limit_seen:return 0x41bc2u;
        case limit_seen_set:return 0x41bcbu;
        case lens_tag:return 0x41aafu;
        case lens_instances:return 0x41ab2u;
        case lens_one:return 0x41ab8u;
        case lens_definition:return 0x41ac9u;
        case lens_flags:return 0x41acdu;
        case lens_scale_store:return 0x41ad0u;
        case lens_marker_scale:return 0x41ad8u;
        case lens_marker_scale_store:return 0x41addu;
        case lens_scale:return 0x41ae3u;
        case lens_primary:return 0x41ae7u;
        // 3E9B0's last two arguments: the flare's scale and the primary scale.
        case lens_index:this->push(0x41aebu,gp[edx]);this->push(0x41aecu,gp[eax]);return 0x41aedu;
        // Then 1.0 and the color.
        case lens_object_index:
            this->push(0x41af1u,logic::unit_scale);gp[ecx]=local(Local::color);this->push(0x41afau,gp[ecx]);
            return 0x41afbu;
        case lens_tag_again:return 0x41b0fu;
        case lens_count_again:return 0x41b18u;
        case lens_object_again:return 0x41b1cu;
        case lens_count_store:return 0x41b24u;
        case volume_tag:return 0x41b34u;
        case volume_instances:return 0x41b37u;
        case volume_one:return 0x41b3du;
        case volume_definition:return 0x41b50u;
        case volume_entries_count:return 0x41b54u;
        case volume_entries:return 0x41b5bu;
        case volume_flags:return 0x41b5eu;
        case volume_marker_scale:return 0x41b63u;
        case volume_primary:return 0x41b68u;
        case tdtl_tag:return 0x41b8au;
        case marker_index_read:return 0x41ba8u;
        case marker_count_read:return 0x41bacu;
        case marker_index_store:return 0x41bb6u;
        case index_read:return 0x41bd2u;
        case offset_again:return 0x41bd6u;
        case definition_again:return 0x41bdau;
        case count_again:return 0x41bdeu;
        case index_store:return 0x41beau;
        case offset_store:return 0x41beeu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case skip:this->flags.minus(v,gp[edi],2);break;                       // CMP word,DI (zero)
        case object_table:case definition_tag:case white_red:case lens_tag:case lens_definition:case lens_tag_again:
        case volume_instances:case volume_definition:case volume_primary:case count_again:
            gp[ecx]=v;
            if(a==count_again) {
                // INC EDI; ADD EDX,18; CMP EDI,ECX.
                gp[edi]=this->flags.increment(gp[edi],4,false);gp[edx]=this->flags.plus(gp[edx],0x18u,4);
                this->compare32(gp[edi],v);
            }
            break;
        case object_rows:case white_green:case lens_instances:case lens_scale:case volume_entries:case tdtl_tag:
        case offset_read:case offset_again:
            gp[edx]=v;break;
        // MOV ESI; MOV EAX,ESI; AND EAX,FFFF.
        case object_argument:gp[esi]=v;gp[eax]=this->flags.logic(v&0xffffu,4);break;
        case object:case definition_data:case marker_name:case white:case white_blue:case lens_primary:case lens_index:
        case lens_count_again:case volume_tag:case marker_index_read:case definition_again:
            gp[eax]=v;break;
        // MOV EDX; AND ECX,FFFF; SHL ECX,4 (the definition's tag row).
        case tag_instances:gp[edx]=v;gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),4,4);break;
        case attachment_count:this->compare32(v,gp[edi]);break;                 // CMP [EAX+94],EDI (zero)
        case attachment_list:gp[ebp]=v;break;
        case attachment_group:gp[eax]=v;gp[ebp]=this->flags.plus(gp[ebp],gp[edx],4);break;
        case attachment_tag:this->compare32(v,logic::none);break;
        case change_color:gp[eax]=(gp[eax]&0xffff0000u)|v;this->flags.logic(v,2);break;   // MOV AX; TEST AX,AX
        case object_after_color:gp[esi]=v;break;
        // MOV ECX; MOV EAX,40; CMP ECX,EAX.
        case lens_count:gp[ecx]=v;gp[eax]=std::uint32_t(logic::lens_flare_limit);this->compare32(v,gp[eax]);break;
        case limit_seen:set_low_byte(gp[eax],v);this->test8(v);break;
        // AND ECX,FFFF; SHL ECX,4 (XMM0 is set at the call, from the logic's scale).
        case lens_one:gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),4,4);break;
        case lens_flags:this->test8(v&low_byte(gp[eax]));break;                // TEST byte [ECX+28],AL
        case lens_object_index:gp[ecx]=v;break;
        case lens_object_again:gp[esi]=v;gp[eax]=this->flags.increment(gp[eax],4,false);break;
        // MOV EDX,EAX; AND EDX,FFFF; SHL EDX,4.
        case volume_one:gp[edx]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),4,4);break;
        case volume_entries_count:gp[edx]=v;this->test32(v);break;
        case volume_flags:this->test8(v&logic::light_volume_flags::marker_scale);break;
        case marker_count_read:gp[ecx]=v;gp[eax]=this->flags.increment(gp[eax],4,false);this->compare32(gp[eax],v);break;
        case index_read:gp[edi]=v;break;
        default:break;
        }
    }
    // PUSH EBP follows the attachment offset's first store.
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::offset_clear)this->push(0x419e0u,this->gp[ebp]);
    }
    // Frame.
    std::uint32_t local(Local l)const {
        using L=logic::Locals;
        switch(l) {
        case Local::lens_flares:return locals+offsetof(L,lens_flares);
        case Local::attachment_offset:return locals+offsetof(L,attachment_offset);
        case Local::attachment_index:return locals+offsetof(L,attachment_index);
        case Local::marker_index:return locals+offsetof(L,marker_index);
        case Local::primary_scale:return locals+offsetof(L,primary_scale);
        case Local::flare_scale:return locals+offsetof(L,flare_scale);
        case Local::object_index:return locals+offsetof(L,object_index);
        case Local::definition:return locals+offsetof(L,definition);
        case Local::color:return locals+offsetof(L,color);
        case Local::marker_count:return locals+offsetof(L,marker_count);
        case Local::markers:return locals+offsetof(L,markers);
        // Above the frame: the return address, then the arguments.
        case Local::skip:return locals+std::uint32_t(sizeof(L))+4u;
        case Local::object:return locals+std::uint32_t(sizeof(L))+8u;
        }
        return 0;
    }
    void child(std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        this->call(pc,next,target,[&](State &v){dispatch(v,target,pc);});
    }
    // In the loop the four register saves are below the locals; the original
    // addresses them from the stack pointer the last child left.
    void after_call(){locals=this->gp[esp]+0x10u;}
    // MOV EAX,1CA0; CALL 32A660 (the stack probe); PUSH EDI; XOR EDI,EDI.
    void allocate() {
        auto &gp=this->gp;
        gp[eax]=std::uint32_t(sizeof(logic::Locals));
        child(0x41975u,0x4197au,0x32a660u);
        locals=gp[esp];
        this->push(0x4197au,gp[edi]);gp[edi]=this->flags.logic(0,4);
    }
    // The walk's registers at a call (EBX: the marker count, or the current
    // marker's up axis while the markers are submitted). EDI holds the
    // attachment's index only up to C0700; later calls see whatever EDI holds.
    void keep(const Walk &w,std::uint32_t ebx_value) {
        auto &gp=this->gp;gp[esi]=w.object;gp[ebp]=w.attachment;gp[ebx]=ebx_value;
    }
    void take_marker(Walk &w)const {
        const auto &gp=this->gp;w.object=gp[esi];w.attachment=gp[ebp];
        w.marker=gp[ebx]-std::uint32_t(offsetof(logic::ObjectMarker,world.up));
    }
    // PUSH 41, PUSH name, PUSH ESI; XOR CL,CL; LEA EBX,[the markers]; BAD60.
    std::uint32_t find_markers(Walk &w,std::uint32_t name,std::uint32_t out) {
        auto &gp=this->gp;
        keep(w,gp[ebx]);gp[edi]=w.index;
        this->push(0x41a17u,logic::marker_limit);this->push(0x41a19u,name);this->push(0x41a1au,gp[esi]);
        set_low_byte(gp[ecx],0);this->flags.logic(0,1);
        gp[ebx]=out;
        child(0x41a21u,0x41a26u,0xbad60u);
        w.object=gp[esi];w.attachment=gp[ebp];w.index=gp[edi];
        after_call();
        return gp[eax];
    }
    // MOVSX EBX,AX; MOV ECX,EDI; MOV EDX,ESI; C0700. The scale returns in XMM0.
    std::uint32_t primary_scale(Walk &w) {
        auto &gp=this->gp;
        keep(w,w.marker_count);gp[edi]=w.index;gp[ecx]=w.index;gp[edx]=w.object;
        child(0x41a31u,0x41a36u,0xc0700u);
        w.object=gp[esi];w.attachment=gp[ebp];w.index=gp[edi];w.marker_count=gp[ebx];
        after_call();
        return lane0(this->s.xmm[0]);
    }
    // EDI = the change color, EAX = the object, ESI = the color; BCD80.
    void change_color(Walk &w,std::uint32_t object,std::uint32_t color,std::uint32_t out) {
        auto &gp=this->gp;
        gp[ebp]=w.attachment;gp[ebx]=w.marker_count;gp[edi]=color;gp[eax]=object;gp[esi]=out;
        child(0x41a6cu,0x41a71u,0xbcd80u);
        w.attachment=gp[ebp];w.marker_count=gp[ebx];
        after_call();
    }
    // The flare's position, then its index, the object's index and 0; EDI =
    // the forward axis; XMM0 = the flare's scale (MOVSS from memory: the
    // upper lanes are zero). 3E9B0 returns the flare.
    std::uint32_t lens_flare(Walk &w,const logic::marker_attachments::LensFlare &flare,std::uint32_t scale) {
        auto &gp=this->gp;
        keep(w,logic::marker_attachments::up_of(w.marker));
        gp[edx]=logic::marker_attachments::position_of(w.marker);this->push(0x41b02u,gp[edx]);
        gp[eax]=flare.flare_index;this->push(0x41b03u,gp[eax]);
        gp[ecx]=flare.object_index;this->push(0x41b04u,gp[ecx]);this->push(0x41b05u,0);
        gp[edi]=logic::marker_attachments::forward_of(w.marker);
        this->s.xmm[0]=load_ss(scale);
        child(0x41b0au,0x41b0fu,0x3e9b0u);
        take_marker(w);
        return gp[eax];
    }
    // ECX = the lens flare's tag; PUSH the flare; 2DB80.
    void submit_lens_flare(Walk &w,std::uint32_t tag,std::uint32_t state) {
        auto &gp=this->gp;
        keep(w,logic::marker_attachments::up_of(w.marker));gp[ecx]=tag;gp[eax]=state;
        this->push(0x41b12u,gp[eax]);
        child(0x41b13u,0x41b18u,0x2db80u);
        take_marker(w);
        after_call();
    }
    // PUSH primary scale, forward axis, tag, object; ECX = the position, EDX =
    // the color, EAX = the up axis, XMM0 = the scale; 42840.
    void light_volume(Walk &w,const logic::marker_attachments::LightVolume &volume) {
        auto &gp=this->gp;
        keep(w,logic::marker_attachments::up_of(w.marker));
        gp[ecx]=volume.primary_scale;this->push(0x41b6cu,gp[ecx]);
        gp[edx]=logic::marker_attachments::forward_of(w.marker);this->push(0x41b70u,gp[edx]);
        gp[eax]=volume.tag;this->push(0x41b71u,gp[eax]);
        gp[ecx]=logic::marker_attachments::position_of(w.marker);this->push(0x41b75u,gp[esi]);
        gp[edx]=local(Local::color);gp[eax]=gp[ebx];
        this->s.xmm[0]=load_ss(volume.scale);
        child(0x41b7cu,0x41b81u,0x42840u);
        take_marker(w);
        after_call();
    }
    // PUSH 1.0, forward axis, 0, -1, tag, object; ECX = the position, EDI =
    // the up axis; XOR DL,DL; 42990.
    void tdtl(Walk &w,std::uint32_t tag) {
        auto &gp=this->gp;
        keep(w,logic::marker_attachments::up_of(w.marker));
        this->push(0x41b8du,logic::unit_scale);
        gp[eax]=logic::marker_attachments::forward_of(w.marker);this->push(0x41b95u,gp[eax]);
        this->push(0x41b96u,0);this->push(0x41b98u,logic::none);
        gp[edx]=tag;this->push(0x41b9au,gp[edx]);
        gp[ecx]=logic::marker_attachments::position_of(w.marker);this->push(0x41b9eu,gp[esi]);
        gp[edi]=gp[ebx];set_low_byte(gp[edx],0);this->flags.logic(0,1);
        child(0x41ba3u,0x41ba8u,0x42990u);
        take_marker(w);
        after_call();
    }
    // ADD ESP,1CA0; RET 8.
    void leave() {
        auto &gp=this->gp;
        gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);
        this->ret(0x41c02u,8);this->publish(0x41c02u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Schedule> memory{*this};
        switch(logic::marker_attachments::submit(memory,*this)) {
        case logic::marker_attachments::Exit::skipped:
            gp[edi]=this->pop(0x41bfbu);break;
        case logic::marker_attachments::Exit::no_attachments:
            gp[esi]=this->pop(0x41bfau);gp[edi]=this->pop(0x41bfbu);break;
        case logic::marker_attachments::Exit::submitted:
            gp[ebp]=this->pop(0x41bf8u);gp[ebx]=this->pop(0x41bf9u);gp[esi]=this->pop(0x41bfau);gp[edi]=this->pop(0x41bfbu);break;
        }
        leave();
    }
};

template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x41970u,0x41c02u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Schedule<State,Dispatch>(v,c,t,dispatch);});
    return true;
}
} // namespace halo2::native_attachment_schedule_detail
