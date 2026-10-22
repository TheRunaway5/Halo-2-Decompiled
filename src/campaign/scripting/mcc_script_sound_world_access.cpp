#include "campaign/scripting/mcc_script_sound_world_access_internal.hpp"
#include "audio/mcc_sound_variation_provider_internal.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <limits>
#include <map>
#include <mutex>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
void require(bool v,const char* m){if(!v)throw FormatError(m);}
void status(MccSoundTimingReadStatus s){
    switch(s){
    case MccSoundTimingReadStatus::Executed:return;
    case MccSoundTimingReadStatus::StaleOwner:throw FormatError("Script sound World access has a stale epoch");
    case MccSoundTimingReadStatus::WrongThread:throw FormatError("Script sound World access requires its producer thread");
    case MccSoundTimingReadStatus::ReentrantLeaseOperation:throw FormatError("Script sound World access cannot reenter its lease");
    }
    throw FormatError("Script sound World access has an unknown lease status");
}
std::uint32_t add(std::uint32_t p,std::size_t n){
    require(n<=std::numeric_limits<std::uint32_t>::max()-p,"Script sound source extent wraps");
    return p+std::uint32_t(n);
}
std::uint32_t word(const MccSoundTimingOwnedReader& r,std::uint32_t p){return Bytes(r.read(p,4)).u32(0);}
template<std::size_t N>std::array<std::byte,N> fixed(std::span<const std::byte> b){
    require(b.size()==N,"Script sound fixed owner extent differs");
    std::array<std::byte,N> out;std::ranges::copy(b,out.begin());return out;
}
void put(std::span<std::byte> b,unsigned p,std::uint32_t v){for(unsigned n=0;n<4;++n)b[p+n]=std::byte(v>>(8*n));}
std::uint32_t advance_seed(std::uint32_t seed){return seed*0x19660du+0x3c6ef35fu;}
bool disjoint(std::uint32_t a,std::size_t an,std::uint32_t b,std::size_t bn){
    return std::uint64_t(a)+an<=b||std::uint64_t(b)+bn<=a;
}
bool same_definition(const SoundVariationDefinition& a,const SoundVariationDefinition& b){
    if(a.datum!=b.datum||a.sound!=b.sound||a.playback!=b.playback||a.sound_class!=b.sound_class||
       a.range!=b.range||a.scale!=b.scale||a.permutations!=b.permutations||a.used!=b.used||
       a.sample_rate_index!=b.sample_rate_index||a.pitch_ranges.size()!=b.pitch_ranges.size())return false;
    for(unsigned n=0;n<a.pitch_ranges.size();++n)
        if(a.pitch_ranges[n].interval!=b.pitch_ranges[n].interval||
           a.pitch_ranges[n].permutations!=b.pitch_ranges[n].permutations)return false;
    return true;
}
// Same scoped source FP admission used by the existing sound arithmetic
// helpers. The already verified ordinary selector is reused without changing
// its body or leaving its exception/rounding state in the host caller.
struct PermutationFloatingPointScope {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::fenv_t environment;unsigned mxcsr;
    explicit PermutationFloatingPointScope(const SoundDriverFloatingPointContext& fp):mxcsr(_mm_getcsr()){
        require((fp.x87_control_word&0x3fu)==0x3fu&&(fp.mxcsr&0x1f80u)==0x1f80u&&!(fp.mxcsr&0xffff0000u),
                "Script sound permutation requires masked source FP exceptions");
        require(std::fegetenv(&environment)==0,"Script sound permutation FP environment unavailable");
        _mm_setcsr(fp.mxcsr);asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");
    }
    ~PermutationFloatingPointScope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
#else
    explicit PermutationFloatingPointScope(const SoundDriverFloatingPointContext&){
        throw FormatError("Original sound permutation FP backend unavailable");
    }
#endif
};
struct Root {TagHandle gestalt;std::uint32_t source_root,address;std::array<std::byte,16> directory;std::array<std::byte,20> bytes;};
struct OperationGate {bool active=false;};
std::mutex operation_gates_mutex;
std::map<const MccSoundTimingMemoryOwner*,std::weak_ptr<OperationGate>> operation_gates;
std::shared_ptr<OperationGate> operation_gate(const std::shared_ptr<const MccSoundTimingMemoryOwner>& owner){
    std::lock_guard lock(operation_gates_mutex);
    std::erase_if(operation_gates,[](const auto& pair){return pair.second.expired();});
    auto& weak=operation_gates[owner.get()];auto gate=weak.lock();
    if(!gate){gate=std::make_shared<OperationGate>();weak=gate;}return gate;
}
SourceScriptSoundClockRowsTu15 parse_rows(std::span<const std::byte> data){
    require(data.size()==256,"Script sound clock bank extent differs");
    const Bytes b(data);SourceScriptSoundClockRowsTu15 rows;
    for(unsigned n=0;n<16;++n){const auto at=n*16u;
        require(b.u32(at+8)==0xffffffffu,"Script sound clock contains a genuine source voice");
        rows[n]={b.u32(at),b.u32(at+4),
            {std::to_integer<std::uint8_t>(data[at+12]),std::to_integer<std::uint8_t>(data[at+13])},
            std::to_integer<std::uint8_t>(data[at+14]),std::to_integer<std::uint8_t>(data[at+15])};
    }return rows;
}
} // namespace

struct MccScriptSoundWorldAccess::Impl {
    MccSoundTimingReadLease lease;
    PhysicsLiveWorldTu15* world;
    const Cache* cache;
    MccSoundVariationProvider provider;
    MccSoundTimingPlan timing;
    SoundDriverFloatingPointContext fp;
    std::uint32_t table{},clock{},state{},rows{},auxiliary{},gestalt{};
    TagBlock ranges{},permutations{},used{};
    std::map<std::uint32_t,Root> roots;
    std::shared_ptr<OperationGate> gate;
    std::shared_ptr<const MccSoundTimingMemoryOwner> memory;
    struct Operation {
        const Impl& owner;
        explicit Operation(const Impl& o):owner(o){owner.gate->active=true;}
        Operation(const Operation&)=delete;
        ~Operation(){owner.gate->active=false;}
    };
    Impl(MccSoundTimingReadLease l,PhysicsLiveWorldTu15& w,const Cache& c,
         MccSoundVariationProvider p,MccSoundTimingPlan t,SoundDriverFloatingPointContext f,std::shared_ptr<OperationGate> g,
         std::shared_ptr<const MccSoundTimingMemoryOwner> m)
        :lease(std::move(l)),world(&w),cache(&c),provider(std::move(p)),timing(std::move(t)),fp(f),gate(std::move(g)),memory(std::move(m)){}
    Operation enter(const Cache& expected)const{
        // The borrowed World is never touched before its actual retained
        // epoch/thread check. This is not a lock held across later World calls;
        // the World's existing single producer/restore discipline still applies.
        status(check_mcc_sound_timing_read_lease(lease));
        require(&expected==cache,"Script sound World access belongs to another Cache");
        require(!gate->active,"Script sound World access cannot reenter its operation");
        world->require_execution_boundary();
        return Operation(*this);
    }
    const Root& root(const MccSoundTimingOwnedReader& r,TagHandle sound)const{
        const auto it=roots.find(sound.value);
        require(it!=roots.end(),"Script sound has no complete full-salt native owner");
        require(word(r,0x4f0414u)==table,"Script sound native namespace changed");
        require(std::ranges::equal(r.read(add(table,std::size_t(sound.index())*16u),16),it->second.directory),
                "Script sound directory salt, group or root changed");
        require(std::ranges::equal(r.read(it->second.address,20),it->second.bytes),
                "Script sound complete canonical header changed");
        return it->second;
    }
    void identities(const MccSoundTimingOwnedReader& r,const SourceScriptSoundClockRowsTu15& data)const{
        for(const auto& row:data)if(row.sound!=0xffffffffu)(void)root(r,{row.sound});
    }
    std::vector<std::byte> row_bytes(const MccSoundTimingOwnedReader& r)const{
        require(word(r,0x4f9b58u)==state,"Script sound clock state owner changed");
        return r.read(rows,256);
    }
    unsigned sound_ranges(const Cache& expected,TagHandle sound)const{
        unsigned count=0;
        status(with_mcc_sound_timing_owned_read(lease,[&](const auto& r){
            const auto& native=root(r,sound);count=std::to_integer<unsigned>(native.bytes[10]);
        }));
        // These are the complete immutable range/permutation/Legacy-class
        // checks already used by the existing metadata/PCM ownership adapter.
        for(unsigned n=0;n<count;++n)(void)provider.load(expected,sound,n);
        return count;
    }
    std::int32_t stored(TagHandle sound)const{
        std::optional<std::int32_t> value;
        status(with_mcc_sound_timing_owned_read(lease,[&](const auto& r){
            const auto& native=root(r,sound);
            const auto all=timing.stored_references();
            const auto found=std::ranges::find_if(all,[&](const auto& q){return q.sound==sound;});
            require(found!=all.end()&&found->gestalt==native.gestalt&&found->source_root==native.source_root&&
                    found->native_root==native.address&&
                    found->expected_native_header==native.bytes,
                    "Script sound stored time lacks finite original identity");
            // root() checked the actual complete20B bytes in this transaction.
            require(std::bit_cast<std::int32_t>(Bytes(native.bytes).u32(16))==found->original_ms,
                    "Script sound stored time differs from its admitted original word");
            value=found->original_ms;
        }));require(value.has_value(),"Script sound stored-time read did not execute");return *value;
    }
};
struct MccScriptSoundWorldAccessFactory {
    static MccScriptSoundWorldAccess make(std::shared_ptr<const MccScriptSoundWorldAccess::Impl> p){
        return MccScriptSoundWorldAccess(std::move(p));
    }
    static MccScriptSoundWorldAccess build(PhysicsLiveWorldTu15& world,const Cache& actual,const Cache& expected,
        MccSoundVariationProvider provider,const MccSoundRuntimePlan& plan,MccSoundTimingPlan timing,
        const std::shared_ptr<const MccSoundTimingMemoryOwner>& memory,SoundDriverFloatingPointContext fp){
        auto lease=capture_mcc_sound_timing_read_lease(memory);
        status(check_mcc_sound_timing_read_lease(lease));
        require(&actual==&expected,"Script sound access requires this World's Cache");
        auto gate=operation_gate(memory);
        require(!gate->active,"Script sound access factory cannot reenter its actual owner");
        auto out=std::make_shared<MccScriptSoundWorldAccess::Impl>(std::move(lease),world,actual,
            std::move(provider),std::move(timing),fp,std::move(gate),memory);
        status(with_mcc_sound_timing_owned_read(out->lease,[&](const auto& r){
            out->table=word(r,0x4f0414u);out->clock=word(r,0x51df54u);out->state=word(r,0x4f9b58u);
            out->auxiliary=word(r,0x4f3cd8u);out->gestalt=word(r,0x52d664u);
            out->ranges=plan.blocks[4];out->permutations=plan.blocks[5];out->used=plan.blocks[7];
            require(out->auxiliary&&out->gestalt,"Script sound native auxiliary/gestalt owners are unavailable");
            (void)r.read(out->auxiliary,8);
            require(out->table&&out->clock&&out->state,"Script sound genuine state owners are unavailable");
            (void)r.read(out->clock,12);(void)r.read(out->state,0x244);
            out->rows=add(out->state,0x128);
            (void)parse_rows(r.read(out->rows,256));
            for(const auto& s:plan.sounds){
                const auto patch=std::ranges::find_if(plan.patches,[&](const auto& p){
                    return p.owner==s.sound&&p.source_address==s.source_root&&p.after.size()==20;
                });
                require(patch!=plan.patches.end(),"Script sound lacks its complete canonical projection");
                const auto directory=r.read(add(out->table,std::size_t(s.sound.index())*16u),16);const Bytes d(directory);
                require(d.u32(0)==TagGroup{"snd!"}.value&&d.u32(4)==s.sound.value&&d.u32(8)==s.native_root&&d.u32(12)>=20,
                        "Script sound published native full-salt owner differs");
                const auto* raw=actual.find_tag(s.sound,TagGroup{"snd!"});
                require(raw&&raw->address==s.source_root,"Script sound raw/native owner mapping differs");
                Root root{s.gestalt,s.source_root,s.native_root,fixed<16>(directory),fixed<20>(patch->after)};
                require(std::ranges::equal(r.read(root.address,20),root.bytes),"Script sound canonical preimage differs");
                require(out->roots.emplace(s.sound.value,root).second,"Script sound has duplicate salted roots");
            }
            require(!out->roots.empty(),"Script sound native catalog is empty");
        }));return make(std::move(out));
    }
};
MccScriptSoundWorldAccess::MccScriptSoundWorldAccess(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
void MccScriptSoundWorldAccess::require_current(const Cache& c)const{const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);}
void MccScriptSoundWorldAccess::require_metadata_owner(const Cache& c,
    const MccSoundVariationProvider& provider)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");
    const auto operation=o->enter(c);
    detail::require_mcc_sound_variation_owner(provider,c,o->memory);
}
SourceScriptSoundClockTimeTu15 MccScriptSoundWorldAccess::time(const Cache& c)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    SourceScriptSoundClockTimeTu15 result;
    status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
        require(word(r,0x51df54u)==o->clock,"Script sound game-time owner changed");
        const auto data=r.read(o->clock,12);const Bytes b(data);
        result={b.u32(8),std::bit_cast<std::int16_t>(b.u16(2)),b.u32(4)};
    }));return result;
}
void MccScriptSoundWorldAccess::select(const Cache& c,TagHandle sound,SourceScriptSoundClockSelectionTu15& selected)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    if(!sound.is_null())(void)o->sound_ranges(c,sound);
    auto a=o->world->prepare_source_call();const auto output=add(a.gp[4],0x40);
    o->world->memory().validate_mutable_view(output,2);
    o->world->write_source_call_input(output,std::array{std::byte(selected.pitch_range),std::byte(selected.permutation)});
    a=o->world->prepare_source_call(std::array{output,0u});a.gp[0]=sound.value;
    const auto copy=[&]{status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
        const auto bytes=r.read(output,2);selected={std::to_integer<std::uint8_t>(bytes[0]),std::to_integer<std::uint8_t>(bytes[1])};
    }));};
    // Keep the World's ordinary source observer and journal. A fault retains
    // all genuine earlier prefix/RNG and selector stores, never a rollback.
    try{o->world->run_actual(0x213410u,a);}catch(...){copy();throw;}
    copy();
}
std::uint32_t MccScriptSoundWorldAccess::selected_seconds(const Cache& c,TagHandle sound,
    const SourceScriptSoundClockSelectionTu15& selected)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    // Admission is read-only for unknown tuples. The complete check is repeated
    // after all ordinary scratch-input guards and immediately before execution;
    // a guard-induced change then retains only completed caller-input stores.
    std::optional<std::array<std::uint32_t,3>> options_identity;
    const auto admission=[&]{
        if(sound.is_null())return;
        const auto ranges=o->sound_ranges(c,sound);
        if(selected.pitch_range==0xff||selected.permutation==0xff){(void)o->stored(sound);return;}
        require(selected.pitch_range<ranges,"Script sound selected pitch is outside its native owner");
        const auto d=o->provider.load(c,sound,selected.pitch_range);
        require(selected.permutation<d.permutations.size(),"Script sound selected permutation is outside its native owner");
        status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
            const auto game=word(r,0x4f3218u);require(game,"Script sound game options owner is unavailable");
            const auto options=r.read(game,0x135);const Bytes b(options);
            const std::array identity{game,b.u32(8),std::uint32_t(std::to_integer<unsigned>(options[0x134]))};
            require(!options_identity||*options_identity==identity,"Script sound options changed during caller-input admission");
            options_identity=identity;
            const auto& native=o->root(r,sound);
            if(b.u32(8)==1&&options[0x134]!=std::byte{0}){
                const auto all=o->timing.permutation_references();
                const auto q=std::ranges::find_if(all,[&](const auto& p){
                    return p.sound==sound&&p.relative_pitch==selected.pitch_range&&
                           p.relative_permutation==selected.permutation;
                });
                require(q!=all.end()&&q->gestalt==native.gestalt&&q->source_root==native.source_root&&
                        q->native_root==native.address&&q->expected_native_header==native.bytes,
                        "Script sound21F670 lacks finite original permutation identity");
                require(word(r,0x52d664u)==o->gestalt&&o->ranges.count>=0&&o->permutations.count>=0&&
                        word(r,add(o->gestalt,0x20))==std::uint32_t(o->ranges.count)&&
                        word(r,add(o->gestalt,0x24))==o->ranges.address&&
                        word(r,add(o->gestalt,0x28))==std::uint32_t(o->permutations.count)&&
                        word(r,add(o->gestalt,0x2c))==o->permutations.address,
                        "Script sound21F670 merged namespace changed");
                const auto first=std::bit_cast<std::int16_t>(Bytes(native.bytes).u16(8));
                require(first>=0&&std::uint64_t(unsigned(first))+selected.pitch_range<unsigned(o->ranges.count),
                        "Script sound21F670 range escapes its owned bank");
                const auto range=add(o->ranges.address,(std::size_t(unsigned(first))+selected.pitch_range)*12u);
                require(range==q->native_range,"Script sound21F670 range identity changed");
                const auto live_range=r.read(range,12);
                for(unsigned n=0;n<12;++n)if(n!=4&&n!=5)
                    require(live_range[n]==q->expected_native_range[n],"Script sound21F670 immutable range changed");
                const auto base=std::bit_cast<std::int16_t>(Bytes(live_range).u16(8));
                const auto count=std::bit_cast<std::int16_t>(Bytes(live_range).u16(10));
                require(base>=0&&count>=1&&count<=32&&selected.permutation<unsigned(count)&&
                        std::uint64_t(unsigned(base))+selected.permutation<unsigned(o->permutations.count),
                        "Script sound21F670 permutation escapes its owned bank");
                const auto permutation=add(o->permutations.address,(std::size_t(unsigned(base))+selected.permutation)*16u);
                require(permutation==q->native_permutation&&
                        std::ranges::equal(r.read(permutation,16),q->expected_native_permutation)&&
                        Bytes(q->expected_native_permutation).u16(6)==q->original_ms,
                        "Script sound21F670 complete original permutation changed");
                require(disjoint(native.address,20,range,12)&&disjoint(native.address,20,permutation,16)&&
                        disjoint(range,12,permutation,16),"Script sound21F670 typed owners alias");
            }else{
                const auto all=o->timing.bindings();
                const auto binding=std::ranges::find_if(all,[&](const auto& q){
                    return q.sound==sound&&q.relative_pitch==selected.pitch_range&&
                        q.relative_permutation==selected.permutation&&q.gestalt==native.gestalt&&
                        q.source_root==native.source_root&&q.native_root==native.address;
                });
                require(binding!=all.end(),"Script sound selected duration lacks finite original timing identity");
            }
        }));
    };
    admission();
    auto a=o->world->prepare_source_call();const auto input=add(a.gp[4],0x40);
    o->world->memory().validate_mutable_view(input,2);
    o->world->write_source_call_input(input,std::array{std::byte(selected.pitch_range),std::byte(selected.permutation)});
    a=o->world->prepare_source_call();a.gp[0]=sound.value;a.gp[2]=input;
    admission();
    o->world->run_actual(0x213690u,a);return a.xmm[0][0];
}
std::int32_t MccScriptSoundWorldAccess::stored_milliseconds(const Cache& c,TagHandle sound)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);return o->stored(sound);
}
SoundDriverFloatingPointContext MccScriptSoundWorldAccess::floating_point_context(const Cache& c)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);return o->fp;
}
MccScriptSoundFirstPermutationResult MccScriptSoundWorldAccess::first_permutation_after_pitch(
    const Cache& c,const MccScriptSoundFirstPermutationInput& input)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    require(input.post_gain_pitch_seed==advance_seed(advance_seed(input.expected_auxiliary_seed)),
            "Script sound first permutation requires the successful two-draw post-pitch phase");
    const auto definition=o->provider.load(c,input.sound,input.relative_pitch);
    const Bytes snd(definition.sound),range(definition.range);
    const auto first=std::bit_cast<std::int16_t>(snd.u16(8));
    require(first>=0&&input.relative_pitch<std::to_integer<unsigned>(definition.sound[10])&&
            o->ranges.count>=0&&std::uint64_t(unsigned(first))+input.relative_pitch<unsigned(o->ranges.count),
            "Script sound first permutation pitch escapes its native range owner");
    const auto count=std::bit_cast<std::int16_t>(range.u16(10));
    require(count>=1&&count<=32&&definition.permutations.size()==unsigned(count),
            "Script sound first permutation has an unsupported count");
    const auto range_address=add(o->ranges.address,(std::size_t(unsigned(first))+input.relative_pitch)*12u);
    const auto seed_address=add(o->auxiliary,4);
    const bool persistent=!(Bytes(definition.sound_class).u16(10)&0x20u);
    const bool forced=persistent&&std::bit_cast<std::int8_t>(definition.range[5])!=-1;
    const auto used_bytes=persistent&&count>1?unsigned(count+7)/8u:0u;
    const auto used_offset=unsigned(range.u16(6))>>3;
    std::uint32_t used_address{};
    if(used_bytes){
        require(o->used.count>=0&&std::uint64_t(used_offset)+used_bytes<=unsigned(o->used.count)&&
                definition.used.size()==used_bytes,"Script sound first permutation used bank escapes its actual owner");
        used_address=add(o->used.address,used_offset);
    }else require(definition.used.empty(),"Script sound first permutation has an unowned used snapshot");
    require(disjoint(seed_address,4,range_address,12)&&
            (!used_bytes||(disjoint(seed_address,4,used_address,used_bytes)&&
                          disjoint(range_address,12,used_address,used_bytes))),
            "Script sound first permutation mutable owners alias");
    const auto preimage=[&]{
        status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
            const auto& root=o->root(r,input.sound);
            require(root.bytes==definition.sound&&word(r,0x4f3cd8u)==o->auxiliary&&
                    word(r,seed_address)==input.expected_auxiliary_seed,
                    "Script sound first permutation auxiliary preimage differs");
            require(word(r,0x52d664u)==o->gestalt,"Script sound first permutation gestalt owner changed");
            const auto ranges=r.read(add(o->gestalt,32),8),used=r.read(add(o->gestalt,56),8);
            require(Bytes(ranges).u32(0)==std::uint32_t(o->ranges.count)&&Bytes(ranges).u32(4)==o->ranges.address&&
                    Bytes(used).u32(0)==std::uint32_t(o->used.count)&&Bytes(used).u32(4)==o->used.address,
                    "Script sound first permutation typed table owner changed");
            require(std::ranges::equal(r.read(range_address,12),definition.range)&&
                    (!used_bytes||r.read(used_address,used_bytes)==definition.used),
                    "Script sound first permutation range/used preimage differs");
        }));
        require(same_definition(o->provider.load(c,input.sound,input.relative_pitch),definition),
                "Script sound first permutation complete metadata changed during admission");
    };
    preimage();auto state=imported_sound_range_state(definition);SoundLocalRandom random(input.post_gain_pitch_seed);
    std::int16_t permutation;
    {PermutationFloatingPointScope fp(o->fp);permutation=source_sound_permutation(definition,state,random);}
    require(permutation>=0&&unsigned(permutation)<definition.permutations.size(),
            "Script sound first permutation has an unsupported forced ordinal");
    auto& memory=o->world->memory();memory.validate_mutable_view(seed_address,4);
    if(persistent){
        memory.validate_mutable_view(range_address,12);
        if(forced)memory.validate_mutable_view(add(range_address,5),1);
        if(!forced&&used_bytes)memory.validate_mutable_view(used_address,used_bytes);
        memory.validate_mutable_view(add(range_address,4),1);
    }
    // Admission callbacks can alter native state. Repeat every input/identity
    // check before the first store. No owned-read lease spans World writes.
    preimage();std::array<std::byte,4> seed{},flags{};put(seed,0,random.seed);put(flags,0,state.used);
    // Final-state CAS, with ordinary journal/observers and fault semantics.
    // The two creation draws and selector draw trace are not replayed here.
    const std::array forced_byte{std::byte(std::uint8_t(state.forced))},last_byte{std::byte(std::uint8_t(state.last))};
    std::array<std::pair<std::uint32_t,std::span<const std::byte>>,3> writes;
    unsigned write_count=0;writes[write_count++]={seed_address,seed};
    if(persistent){
        if(forced)writes[write_count++]={add(range_address,5),forced_byte};
        else if(used_bytes)writes[write_count++]={used_address,std::span<const std::byte>(flags).first(used_bytes)};
        writes[write_count++]={add(range_address,4),last_byte};
    }
    o->world->write_script_sound_inputs(std::span(writes).first(write_count));
    return {permutation,random.seed};
}
SourceScriptSoundClockRowsTu15 MccScriptSoundWorldAccess::clock_rows(const Cache& c)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    SourceScriptSoundClockRowsTu15 result;
    status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){result=parse_rows(o->row_bytes(r));o->identities(r,result);}));
    return result;
}
void MccScriptSoundWorldAccess::publish_clock_rows(const Cache& c,const SourceScriptSoundClockRowsTu15& expected,
    const SourceScriptSoundClockRowsTu15& after)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    std::array<std::byte,256> image;
    status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
        const auto before=o->row_bytes(r);require(parse_rows(before)==expected,"Script sound clock CAS preimage differs");
        o->identities(r,expected);o->identities(r,after);std::ranges::copy(before,image.begin());
    }));
    // Validate the entire real mutable owner before publishing any metadata.
    // All sourcevoice slots are excluded from the writes, even identical NONE.
    auto& memory=o->world->memory();memory.validate_mutable_view(o->rows,256);
    std::array<std::pair<std::uint32_t,std::span<const std::byte>>,64> writes;unsigned write_count=0;
    for(unsigned n=0;n<16;++n)if(expected[n]!=after[n]){
        const auto at=o->rows+n*16u;
        memory.validate_mutable_view(at+12,2);memory.validate_mutable_view(at+4,4);
        memory.validate_mutable_view(at,4);memory.validate_mutable_view(at+14,2);
    }
    // Resource guards may run during mutable admission. Recheck the complete
    // CAS image and native sound identities after all those guards, before
    // the first actual journaled write. Guard callbacks must still obey the
    // existing World's single-producer/no-concurrent-mutation contract.
    status(with_mcc_sound_timing_owned_read(o->lease,[&](const auto& r){
        require(std::ranges::equal(o->row_bytes(r),image),"Script sound clock changed during mutable admission");
        o->identities(r,expected);o->identities(r,after);
    }));
    for(unsigned n=0;n<16;++n){const auto at=n*16u;const auto& row=after[n];
        put(image,at,row.sound);put(image,at+4,row.start_tick_bits);
        image[at+12]=std::byte(row.selection.pitch_range);image[at+13]=std::byte(row.selection.permutation);
        image[at+14]=std::byte(row.predicate);image[at+15]=std::byte(row.trailing);
    }
    for(unsigned n=0;n<16;++n)if(expected[n]!=after[n]){
        const auto at=n*16u;const auto bytes=std::span<const std::byte>(image);
        // Publish selector before start/sound, including partial selector
        // results; this is compatibility output, not original-event replay.
        writes[write_count++]={o->rows+at+12,bytes.subspan(at+12,2)};
        writes[write_count++]={o->rows+at+4,bytes.subspan(at+4,4)};
        writes[write_count++]={o->rows+at,bytes.subspan(at,4)};
        writes[write_count++]={o->rows+at+14,bytes.subspan(at+14,2)};
    }
    if(write_count)o->world->write_script_sound_inputs(std::span(writes).first(write_count));
}
SourceScriptSoundClockPolicyTu15 MccScriptSoundWorldAccess::policy(const Cache& c)const{
    const auto o=impl_;require(bool(o),"Script sound World access is absent");const auto operation=o->enter(c);
    const auto access=*this;const auto* cache=&c;
    return {[access,cache]{return access.time(*cache);},
        [access,cache](unsigned,std::uint32_t sound,const auto& selection){return access.selected_seconds(*cache,{sound},selection);},
        [access,cache](std::uint32_t sound,auto& selection){access.select(*cache,{sound},selection);},{}};
}
namespace detail {
MccScriptSoundWorldAccess make_mcc_script_sound_world_access(PhysicsLiveWorldTu15& w,const Cache& actual,const Cache& expected,
    MccSoundVariationProvider p,const MccSoundRuntimePlan& runtime,MccSoundTimingPlan timing,
    const std::shared_ptr<const MccSoundTimingMemoryOwner>& memory,SoundDriverFloatingPointContext fp){
    return MccScriptSoundWorldAccessFactory::build(w,actual,expected,std::move(p),runtime,std::move(timing),memory,fp);
}
}
} // namespace halo2
