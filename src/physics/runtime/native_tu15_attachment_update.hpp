#pragma once
// Boundary adapter for the object widget render
// (native_tu15_attachment_update_systems.hpp), entered at D7230 in the model
// render unit. Its caller (the render prepare queue) and its children (the
// widget types' routines and the render queue 41480) are still translated
// TU1.5 code, so this adapter owns that ABI and nothing else:
//   - the arguments: AX the object handle, BX the mode, [ESP+4] the request
//     flags (RET 4);
//   - the original's register file at every child call: the widget walk in
//     EBX/EBP/ESI/EDI (handed back after each call, as the callee returns
//     it), the values the original leaves in EAX/ECX/EDX, and the flags of
//     the TEST that decides the call (the shader routine's TEST ECX,ECX, the
//     direct routine's TEST ESI,ESI; the render queue sees the flags the
//     shader routine returned);
//   - the stack: register saves and restores, the render submission's eight
//     arguments, pushed between the logic's reads as the original pushes
//     them, the direct routine's two, return addresses, and the stack
//     pointer as the children actually leave it;
//   - the exit flags (CMP EAX,-1 on the last link) and the RET 4;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Every child stays genuine and is entered through the unit's own dispatch
// with the exact register, flag, XMM, x87 and stack ABI. Delete the adapter
// when the caller and the children go native.
// HALO2_REFERENCE_ATTACHMENT_UPDATE=1 restores the translated body.
// HALO2_VERIFY_ATTACHMENT_UPDATE=1 records the native run (children genuine,
// once) and replays the unmodified translated body against it, as in
// native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_attachment_update_systems.hpp"

namespace halo2::native_attachment_update_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_widgets;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ATTACHMENT_UPDATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ATTACHMENT_UPDATE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_attachments, mode_two_run, mode_two_skip, mode_one_run, mode_one_skip, other_mode,
    update_callback, no_update_callback, mode_callback, no_mode_callback, more_attachments, last_attachment, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Attachment update verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// ---- D7230 --------------------------------------------------------------------
// Coverage: the branch outcome is the original instruction that follows.
inline constexpr Edge widget_render_edges[]{
    {0xd7246u,0xd72e2u,no_attachments},
    {0xd7271u,0xd72a0u,other_mode},
    {0xd7288u,0xd72a0u,mode_two_run},{0xd7288u,0xd72d8u,mode_two_skip},
    {0xd7297u,0xd72a0u,mode_one_run},{0xd7297u,0xd72d8u,mode_one_skip},
    {0xd72a0u,0xd72a7u,update_callback},{0xd72a0u,0xd72cau,no_update_callback},
    {0xd72cau,0xd72d1u,mode_callback},{0xd72cau,0xd72d8u,no_mode_callback},
    {0xd72d8u,0xd7260u,more_attachments},{0xd72d8u,0xd72e0u,last_attachment},
};
template<class State,class Dispatch> struct Update : Port<State> {
    using Access=logic::widget_render::Access;
    Dispatch &dispatch;
    Update(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,widget_render_edges),dispatch(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0xd7230u;
        case object_rows:return 0xd7236u;
        case object:this->push(0xd7241u,gp[ebp]);return 0xd7242u;
        case first_widget:return 0xd7246u;
        case widget_table:return 0xd7260u;
        case widget_rows:return 0xd7266u;
        case widget_type:return 0xd7271u;
        case render_callback:return 0xd72a0u;
        case widget_data:return 0xd72a7u;
        // LEA EDX,[EBP+30]; PUSH EDX: the submission's last argument, the center.
        case render_context:gp[edx]=logic::center_of(gp[ebp]);this->push(0xd72adu,gp[edx]);return 0xd72aeu;
        // The rest of the submission (data, the second and first callbacks,
        // the filler, the limit, -1), then the shader routine's argument.
        case shader_routine:
            this->push(0xd72b1u,gp[eax]);this->push(0xd72b2u,gp[edx]);this->push(0xd72b3u,gp[ecx]);
            this->push(0xd72b4u,logic::submission::shader_record_filler);this->push(0xd72b9u,logic::submission::limit);
            this->push(0xd72beu,logic::submission::unlimited);this->push(0xd72c0u,gp[eax]);
            return 0xd72c1u;
        case direct_routine:return 0xd72cau;
        case direct_data:return 0xd72d1u;
        case next_widget:return 0xd72d8u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:case widget_table:gp[ecx]=v;break;
        // MOV EDX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2] (the row).
        case object_rows:case widget_rows:gp[edx]=v;gp[eax]=(gp[eax]&0xffffu)*3u;break;
        case object:gp[ebp]=v;break;
        // MOV EAX; CMP EAX,-1; then PUSH ESI/EDI when there is a widget.
        case first_widget:
            gp[eax]=v;
            if(v!=logic::none){this->push(0xd7255u,gp[esi]);this->push(0xd7256u,gp[edi]);}
            break;
        case render_callback:gp[ecx]=v;this->test32(v);break;                  // TEST ECX,ECX
        case widget_data:case direct_data:case next_widget:gp[eax]=v;break;
        case render_context:gp[edx]=v;break;
        case direct_routine:gp[esi]=v;this->test32(v);break;                   // TEST ESI,ESI
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // Frame. The request flags are the stack argument ([ESP+10] after the
    // three register saves); MOV EAX, then TEST AH,20 or TEST AH,10.
    std::uint32_t request_flags(std::uint32_t mode) {
        auto &gp=this->gp;
        if(mode==2u) {
            gp[eax]=this->read(0xd7288u,gp[esp]+0x10u,4);
            this->test8((gp[eax]>>8)&(logic::request_flags::mode_two>>8));
        } else {
            gp[eax]=this->read(0xd7297u,gp[esp]+0x10u,4);
            this->test8((gp[eax]>>8)&(logic::request_flags::mode_one>>8));
        }
        return gp[eax];
    }
    // The walk lives in EBP (object), BX (mode), ESI (type) and EDI (widget).
    void keep(const logic::widget_render::WidgetWalk &w,std::uint32_t esi_value) {
        auto &gp=this->gp;gp[ebp]=w.object;gp[ebx]=w.mode;gp[esi]=esi_value;gp[edi]=w.widget;
    }
    void take(logic::widget_render::WidgetWalk &w,std::uint32_t *esi_value)const {
        const auto &gp=this->gp;w.object=gp[ebp];w.mode=gp[ebx];if(esi_value)*esi_value=gp[esi];w.widget=gp[edi];
    }
    void child(std::uint32_t pc,std::uint32_t next,std::uint32_t target) {
        this->call(pc,next,target,[&](State &v){dispatch(v,target,pc);});
    }
    // At the shader routine's call EAX is the widget's data, ECX the first
    // callback and EDX the second, as the submission's arguments were pushed;
    // the flags are TEST ECX,ECX's. Then PUSH EBX (the mode) completes the
    // submission and the render queue is called.
    void render(logic::widget_render::WidgetWalk &w,std::uint32_t shader,const logic::widget_render::Submission &queued) {
        auto &gp=this->gp;
        keep(w,w.type);
        gp[eax]=queued.data;gp[ecx]=queued.render_callbacks[0];gp[edx]=queued.render_callbacks[1];
        child(0xd72c1u,0xd72c4u,shader);
        this->push(0xd72c4u,gp[ebx]);
        child(0xd72c5u,0xd72cau,logic::submission::render_queue);
        take(w,&w.type);
    }
    // PUSH EAX (the data); PUSH EBX (the mode); CALL ESI (the routine).
    void render_direct(logic::widget_render::WidgetWalk &w,std::uint32_t routine,std::uint32_t data) {
        auto &gp=this->gp;
        keep(w,routine);gp[eax]=data;
        this->push(0xd72d4u,gp[eax]);this->push(0xd72d5u,gp[ebx]);
        child(0xd72d6u,0xd72d8u,routine);
        take(w,nullptr);
    }
    // CMP EAX,-1 (the last link); POP EBP; RET 4.
    void finish() {
        auto &gp=this->gp;
        this->compare32(gp[eax],logic::none);
        gp[ebp]=this->pop(0xd72e2u);
        this->ret(0xd72e3u,4);this->publish(0xd72e3u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Update> memory{*this};
        const auto exit=logic::widget_render::run(memory,*this,gp[eax],gp[ebx]);
        if(exit==logic::widget_render::Exit::rendered){gp[edi]=this->pop(0xd72e0u);gp[esi]=this->pop(0xd72e1u);}
        finish();
    }
};

template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xd7230u,0xd72e3u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Update<State,Dispatch>(v,c,t,dispatch);});
    return true;
}
} // namespace halo2::native_attachment_update_detail
