#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <utility>

namespace halo2 {
struct SourceScriptSoundClockVoice64Tu15::Impl {
    std::shared_ptr<const void> audio_epoch,platform,record;
    std::uint64_t voice;
    std::function<bool()> current;
};
SourceScriptSoundClockVoice64Tu15::SourceScriptSoundClockVoice64Tu15(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
std::uint64_t SourceScriptSoundClockVoice64Tu15::id()const{return impl_->voice;}
bool SourceScriptSoundClockVoice64Tu15::audio_owner_is(
    const std::shared_ptr<const void>&epoch,const std::shared_ptr<const void>&platform)const{
    const auto retained=impl_;
    return retained && retained->audio_epoch==epoch && retained->platform==platform;
}
bool SourceScriptSoundClockVoice64Tu15::voice_record_is(const std::shared_ptr<const void>&record)const{
    const auto retained=impl_;
    return retained && retained->record==record;
}
SourceScriptSoundClockVoice64Tu15 SourceScriptSoundClockVoice64Tu15::published(
    std::shared_ptr<const void>epoch,std::shared_ptr<const void>platform,
    std::shared_ptr<const void>record,std::uint64_t voice,std::function<bool()>current){
    if(!epoch || !platform || !record || !current)
        throw std::runtime_error("Native script sound voice requires actual owners");
    // This factory is private to the Audio adapter. Its current predicate is
    // intentionally first read by finish_voice, behind that clock's gate.
    return SourceScriptSoundClockVoice64Tu15(std::make_shared<const Impl>(Impl{
        std::move(epoch),std::move(platform),std::move(record),voice,std::move(current)}));
}

struct SourceScriptSoundClockAdmission64Tu15::Impl {
    std::weak_ptr<const void> owner;
    std::shared_ptr<const void> epoch;
    unsigned row{};
    std::uint32_t sound{};
    SourceScriptSoundClockSelectionTu15 selection;
    mutable bool finished=false;
};
SourceScriptSoundClockAdmission64Tu15::SourceScriptSoundClockAdmission64Tu15(std::shared_ptr<const Impl>p):impl_(std::move(p)){}
std::uint32_t SourceScriptSoundClockAdmission64Tu15::sound()const{return impl_->sound;}
SourceScriptSoundClockSelectionTu15 SourceScriptSoundClockAdmission64Tu15::selection()const{return impl_->selection;}

struct SourceScriptSoundClockNative64Tu15::Impl {
    struct Epoch {};
    using Ticket=SourceScriptSoundClockAdmission64Tu15::Impl;
    const std::thread::id thread=std::this_thread::get_id();
    bool active=false,closed=false;
    SourceScriptSoundClockRowsTu15 rows;
    std::shared_ptr<const void> epoch=std::make_shared<const Epoch>();
    std::array<std::shared_ptr<const Ticket>,16> generations;
    std::array<std::optional<Voice>,16> voices;
    std::shared_ptr<const Ticket> pending;
    std::shared_ptr<const void> audio_epoch;
    std::function<bool()> audio_alive;
    bool metadata_initialized=false;
    struct Operation {
        Impl&owner;
        explicit Operation(Impl&o):owner(o){
            if(owner.thread!=std::this_thread::get_id())
                throw std::runtime_error("Native script sound clock used from foreign thread");
            if(owner.active)throw std::runtime_error("Native script sound clock reentered");
            owner.active=true;
        }
        void validate_audio(){
            if(owner.closed)throw std::runtime_error("Native script sound Audio owner closed");
            if(owner.audio_epoch && (!owner.audio_alive || !owner.audio_alive()))
                throw std::runtime_error("Native script sound Audio owner unavailable");
        }
        ~Operation(){owner.active=false;}
    };
    bool current(const std::shared_ptr<const Ticket>&ticket)const{
        return ticket && ticket->owner.lock().get()==this && ticket->epoch==epoch &&
            ticket->row<rows.size() && generations[ticket->row]==ticket &&
            rows[ticket->row].sound==ticket->sound;
    }
    bool unfinished(const std::shared_ptr<const Ticket>&ticket)const{
        return current(ticket) && pending==ticket && !ticket->finished;
    }
    void finish(const std::shared_ptr<const Ticket>&ticket,std::optional<Voice>voice){
        voices[ticket->row]=std::move(voice);ticket->finished=true;pending.reset();
    }
};

SourceScriptSoundClockNative64Tu15::SourceScriptSoundClockNative64Tu15():impl_(std::make_shared<Impl>()){}
SourceScriptSoundClockNative64Tu15::~SourceScriptSoundClockNative64Tu15()=default;

void SourceScriptSoundClockNative64Tu15::initialize_metadata(
    const SourceScriptSoundClockRowsTu15&input){
    auto owner=impl_;Impl::Operation operation(*owner);operation.validate_audio();
    if(owner->audio_epoch || owner->pending || owner->metadata_initialized ||
       std::ranges::any_of(owner->voices,[](const auto&v){return bool(v);}) ||
       std::ranges::any_of(owner->generations,[](const auto&g){return bool(g);}) ||
       owner->rows!=SourceScriptSoundClockRowsTu15{})
        throw std::runtime_error("Restored script metadata requires a fresh unbound clock");
    owner->rows=input;owner->metadata_initialized=true;
}

void SourceScriptSoundClockNative64Tu15::bind_audio_owner(
    std::shared_ptr<const void>epoch,std::function<bool()>alive){
    auto owner=impl_;Impl::Operation operation(*owner);
    operation.validate_audio();
    if(owner->audio_epoch || !epoch || !alive || owner->pending)
        throw std::runtime_error("Native script sound Audio binding invalid");
    for(const auto&row:owner->rows)if(!owner->metadata_initialized && row.sound!=0xffffffffu)
        throw std::runtime_error("Native script sound Audio binding requires fresh owner");
    if(!alive())throw std::runtime_error("Native script sound Audio owner unavailable");
    owner->audio_epoch=std::move(epoch);owner->audio_alive=std::move(alive);
}

std::optional<SourceScriptSoundClockNative64Tu15::Admission> SourceScriptSoundClockNative64Tu15::begin(
    std::uint32_t sound,const SourceScriptSoundClockPolicyTu15&policy,
    const StopRequest&stop,const SoundDriverFloatingPointContext&fp){
    auto owner=impl_;Impl::Operation operation(*owner);
    const auto retained_policy=policy;const auto retained_stop=stop;const auto controls=fp;
    operation.validate_audio();
    if(owner->pending)throw std::runtime_error("Native script sound admission still pending");
    if(sound==0xffffffffu)return {}; //18CAC0 checks NONE before18CCD0.
    // Allocate the future identity before any original policy/RNG stores.
    auto ticket=std::make_shared<Admission::Impl>();ticket->owner=owner;ticket->epoch=owner->epoch;ticket->sound=sound;
    auto callbacks=retained_policy;
    if(retained_policy.selected_seconds)callbacks.selected_seconds=[&](unsigned row,std::uint32_t datum,const auto&selection){
        operation.validate_audio();const auto result=retained_policy.selected_seconds(row,datum,selection);operation.validate_audio();return result;
    };
    if(retained_policy.time)callbacks.time=[&]{operation.validate_audio();const auto result=retained_policy.time();operation.validate_audio();return result;};
    if(retained_policy.select)callbacks.select=[&](std::uint32_t datum,auto&selection){operation.validate_audio();retained_policy.select(datum,selection);operation.validate_audio();};
    callbacks.request_stop=[&](unsigned row){
        if(!retained_stop)throw std::runtime_error("Native script sound stop owner unavailable");
        operation.validate_audio();const auto voice=owner->voices[row];retained_stop(voice);operation.validate_audio();
    };
    const auto row=source_script_sound_clock_admit_tu15(owner->rows,sound,callbacks,controls);
    if(!row)return {};
    operation.validate_audio();
    ticket->row=*row;ticket->selection=owner->rows[*row].selection;
    owner->generations[*row]=ticket;owner->pending=ticket;
    return Admission(std::move(ticket));
}

bool SourceScriptSoundClockNative64Tu15::finish_failure(const Admission&admission){
    auto owner=impl_;Impl::Operation operation(*owner);
    const auto ticket=admission.impl_;operation.validate_audio();
    if(!owner->unfinished(ticket))return false;
    owner->finish(ticket,{});return true;
}
bool SourceScriptSoundClockNative64Tu15::finish_voice(const Admission&admission,const Voice&voice){
    auto owner=impl_;Impl::Operation operation(*owner);
    const auto ticket=admission.impl_;const auto retained=voice;operation.validate_audio();
    if(!owner->unfinished(ticket) || !owner->audio_epoch ||
       retained.impl_->audio_epoch!=owner->audio_epoch || !retained.impl_->current())return false;
    operation.validate_audio();owner->finish(ticket,retained);return true;
}
bool SourceScriptSoundClockNative64Tu15::retired(const Admission&admission,const Voice&voice){
    auto owner=impl_;Impl::Operation operation(*owner);
    const auto ticket=admission.impl_;const auto retained=voice;operation.validate_audio();
    if(!owner->current(ticket) || !ticket->finished)return false;
    auto&bound=owner->voices[ticket->row];
    if(!bound || bound->impl_!=retained.impl_ || retained.impl_->audio_epoch!=owner->audio_epoch ||
       retained.impl_->current())return false;
    operation.validate_audio();
    // Retirement is notification from that exact actual record after native
    // acknowledgement/destruction. It never changes clock timing metadata.
    bound.reset();return true;
}
bool SourceScriptSoundClockNative64Tu15::admission_pending(const Admission&admission)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto ticket=admission.impl_;operation.validate_audio();
    return owner->unfinished(ticket);
}
bool SourceScriptSoundClockNative64Tu15::audio_owner_is(const std::shared_ptr<const void>&epoch)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto retained=epoch;operation.validate_audio();
    return owner->audio_epoch && owner->audio_epoch==retained;
}
std::function<void()> SourceScriptSoundClockNative64Tu15::close_audio_binding()const{
    auto owner=impl_;Impl::Operation operation(*owner);operation.validate_audio();
    if(!owner->audio_epoch)throw std::runtime_error("Native script sound close requires actual Audio binding");
    return [weak=std::weak_ptr<Impl>(owner),epoch=owner->audio_epoch]()noexcept{
        if(const auto retained=weak.lock()){
            if(retained->thread!=std::this_thread::get_id())std::terminate();
            if(retained->audio_epoch!=epoch)return;
            // Genuine Audio teardown may occur inside a provider. Invalidate
            // before later provider reads; keep row storage intact for the
            // active operation's retained references. No allocation/callback.
            retained->closed=true;retained->voices={};retained->generations={};retained->pending.reset();
        }
    };
}
std::function<bool()> SourceScriptSoundClockNative64Tu15::retirement_callback(
    const Admission&admission,const Voice&voice)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto ticket=admission.impl_;const auto retained=voice.impl_;
    operation.validate_audio();
    if(!owner->current(ticket) || !ticket->finished || !owner->voices[ticket->row] ||
       owner->voices[ticket->row]->impl_!=retained)
        throw std::runtime_error("Native script sound retirement requires exact binding");
    return [weak=std::weak_ptr<Impl>(owner),ticket,receipt=std::weak_ptr<const Voice::Impl>(retained)]{
        const auto state=weak.lock();const auto voice_owner=receipt.lock();
        if(!state || !voice_owner)return true;
        if(state->thread!=std::this_thread::get_id())
            throw std::runtime_error("Native script sound retirement used from foreign thread");
        // A genuine stop callback may retire an actual packet while the clock
        // transaction is active. Defer this notification to its producer;
        // do not read providers or mutate a replacement row here.
        if(state->active)return false;
        if(state->closed)return true;
        Impl::Operation scope(*state);scope.validate_audio();
        if(!state->current(ticket) || !ticket->finished)return true;
        auto&bound=state->voices[ticket->row];
        if(!bound || bound->impl_!=voice_owner || voice_owner->audio_epoch!=state->audio_epoch)return true;
        if(voice_owner->current())return false;
        scope.validate_audio();bound.reset();return true;
    };
}

SourceScriptSoundGameTicksTu15 SourceScriptSoundClockNative64Tu15::explicit_remaining(
    std::uint32_t sound,std::int32_t input,const SourceScriptSoundClockTimeTu15&time,
    const SoundDriverFloatingPointContext&fp)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto now=time;const auto controls=fp;operation.validate_audio();
    const auto row=source_script_sound_clock_find_first_tu15(owner->rows,sound);
    return {source_script_sound_explicit_remaining_tu15(bool(row),row?owner->rows[*row].start_tick_bits:0,input,now,controls)};
}
SourceScriptSoundTicks30Tu15 SourceScriptSoundClockNative64Tu15::stored_remaining(
    std::uint32_t sound,std::int32_t maximum_ms,const SourceScriptSoundClockTimeTu15&query,
    const SourceScriptSoundClockTimeTu15&returned,const SoundDriverFloatingPointContext&fp)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto queried=query;const auto result_time=returned;const auto controls=fp;operation.validate_audio();
    const auto row=source_script_sound_clock_find_first_tu15(owner->rows,sound);
    return {source_script_sound_stored_remaining_tu15(bool(row),row?owner->rows[*row].start_tick_bits:0,maximum_ms,queried,result_time,controls)};
}
SourceScriptSoundTicks30Tu15 SourceScriptSoundClockNative64Tu15::language_remaining(
    std::uint32_t sound,const SourceScriptSoundClockPolicyTu15&policy,const SoundDriverFloatingPointContext&fp)const{
    auto owner=impl_;Impl::Operation operation(*owner);const auto callbacks=policy;const auto controls=fp;operation.validate_audio();
    const auto row=source_script_sound_clock_find_first_tu15(owner->rows,sound);
    if(!row)return {source_script_sound_language_remaining_tu15(false,0,0,{},controls)};
    if(!callbacks.selected_seconds || !callbacks.time)
        throw std::runtime_error("Original script sound duration/time owner unavailable");
    const auto&r=owner->rows[*row];const auto seconds=callbacks.selected_seconds(*row,r.sound,r.selection);
    operation.validate_audio();
    const auto time=callbacks.time();
    operation.validate_audio();
    return {source_script_sound_language_remaining_tu15(true,r.start_tick_bits,seconds,time,controls)};
}
bool SourceScriptSoundClockNative64Tu15::stop_first(std::uint32_t sound,const StopRequest&stop){
    auto owner=impl_;Impl::Operation operation(*owner);
    const auto retained_stop=stop;operation.validate_audio();
    if(owner->pending)throw std::runtime_error("Native script sound admission still pending");
    const auto row=source_script_sound_clock_stop_first_tu15(owner->rows,sound,[&](unsigned i){
        if(!retained_stop)throw std::runtime_error("Native script sound stop owner unavailable");
        const auto voice=owner->voices[i];retained_stop(voice);operation.validate_audio();
    });
    if(row){owner->voices[*row].reset();owner->generations[*row].reset();}
    return bool(row);
}
void SourceScriptSoundClockNative64Tu15::reset(){
    auto owner=impl_;Impl::Operation operation(*owner);operation.validate_audio();auto epoch=std::make_shared<const Impl::Epoch>();
    owner->rows={};owner->voices={};owner->generations={};owner->pending.reset();owner->epoch=std::move(epoch);
}
SourceScriptSoundClockRowsTu15 SourceScriptSoundClockNative64Tu15::rows()const{
    auto owner=impl_;Impl::Operation operation(*owner);operation.validate_audio();return owner->rows;
}
} // namespace halo2
