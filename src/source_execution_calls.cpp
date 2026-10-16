#include "halo2/source_execution_calls.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_execution_model_inputs.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_lens_query_gpu.hpp"
#include <boost/pfr/core.hpp>
#include <openssl/evp.h>
#include <bit>
#include <cstring>
#include <cmath>
#include <limits>
#include <tuple>
#include <type_traits>

namespace halo2 {
namespace {
template<class T>struct Shape{static constexpr unsigned value=0;};
template<class T,class A>struct Shape<std::vector<T,A>>{static constexpr unsigned value=1;};
template<class T,std::size_t N>struct Shape<std::array<T,N>>{static constexpr unsigned value=2;};
template<class T>struct Shape<std::optional<T>>{static constexpr unsigned value=3;};
template<class T>inline constexpr bool unsupported=false;
using Abi=SourcePhysicsContactRuntimeAbi;
// A new caller field must be explicitly added to the owning input boundary.
static_assert(boost::pfr::tuple_size_v<SourceLiveModelDrawContextTu15> == 8);
static_assert(boost::pfr::tuple_size_v<SourceExecutionModelInputTu15> == 7);
static_assert(boost::pfr::tuple_size_v<SourceBspPartCommandState> == 5);
static_assert(boost::pfr::tuple_size_v<SourcePlayerCameraCall> == 7);
static_assert(boost::pfr::tuple_size_v<SourceLensQueryRollback> == 3);
static_assert(boost::pfr::tuple_size_v<SourceLensDeviceRegion> == 2);
static_assert(boost::pfr::tuple_size_v<ParticleCollisionHit> == 6);
static_assert(std::tuple_size_v<SourceCameraControllerState> == 80 && std::tuple_size_v<SourceObserverState> == 214);
void validate_attachment_state(const SourceBspPartCommandState &state){
    if(state.calls.size()>65536)throw FormatError("Native attachment trace exceeds its bound");
    for(const auto &bank:state.banks)if(bank.header[0]>4096||bank.storage.size()!=bank.header[0]||bank.header[1]>bank.header[0])
        throw FormatError("Invalid native attachment command-bank extent");
}
void validate_attachment_input(const SourceBspPartCommandState &state,const SourceObjectAdmission &queue,std::uint32_t mode){
    validate_attachment_state(state);
    if((mode!=1&&mode!=2)||queue.objects.size()>source_object_main_queue_capacity||queue.associations.size()>512)
        throw FormatError("Invalid native attachment mode or admission extent");
}
void validate_model_state(const SourceObjectAdmission &admission,const SourceDrawUnitState &state){
    if(admission.objects.size()>source_object_main_queue_capacity||admission.associations.size()>512||
       state.draw_count>state.units.size()||state.main_prefix>state.draw_count||state.override_count>state.overrides.size()||
       state.section_highwater>state.sections.size()||state.arena.used>state.arena.words.size())
        throw FormatError("Invalid native live-model admission or draw-bank extent");
}
void validate_model_input(const SourceObjectAdmission &admission,const SourceDrawUnitState &state,const SourceExecutionModelInputTu15 &input){
    validate_model_state(admission,state);
    if(input.entry.mode!=input.draw.draw.mode||input.view>=4||!input.local_players||input.local_players>4)
        throw FormatError("Invalid native live-model view input");
}
constexpr bool native_extended=std::endian::native==std::endian::little&&sizeof(long double)>=10&&
    std::numeric_limits<long double>::digits==64&&std::numeric_limits<long double>::max_exponent==16384;
void validate_abi(const Abi &abi){
    if(abi.x87_depth>8||(abi.active_mmx&&(abi.x87_depth||(abi.x87_status&0x3800u))))
        throw FormatError("Invalid native call x87/MMX register-file state");
    if(abi.x87_depth&&!native_extended)throw FormatError("Native call extended precision representation unavailable");
    if(abi.floating_point){const auto &fp=*abi.floating_point;
        if((fp.x87_control_word&0x3fu)!=0x3fu||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw FormatError("Invalid or unsupported native ABI floating-point controls");}
}
template<class T>constexpr std::size_t sequence_bound(){
    if constexpr(std::is_same_v<T,std::vector<SourceFrustumCamera>>)return 6;
    else if constexpr(std::is_same_v<T,std::vector<SourceCameraFrustum>>)return 512;
    else if constexpr(std::is_same_v<T,std::vector<SourceClusterFrustumRanges>>||std::is_same_v<T,std::vector<std::uint32_t>>)return 128;
    else if constexpr(std::is_same_v<T,std::vector<SourceBspAdmissionRecord>>)return source_object_main_queue_capacity;
    else if constexpr(std::is_same_v<T,std::vector<std::array<std::uint32_t,4>>>)return 512;
    else if constexpr(std::is_same_v<T,std::vector<SourcePartCommand>>)return 4096;
    else return 65536;
}
struct Writer {
    std::vector<std::byte> bytes;
    template<class T>void put(const T &value){
        if constexpr(std::is_same_v<T,bool>)put(std::uint8_t(value));
        else if constexpr(std::is_integral_v<T>){
            if(sizeof(T)>SourceExecutionInputs::maximum_call_bytes-bytes.size())throw FormatError("Native call arguments exceed their byte bound");
            const auto bits=std::make_unsigned_t<T>(value);
            for(unsigned n=0;n<sizeof(T);++n)bytes.push_back(std::byte(bits>>(8*n)));
        }else if constexpr(std::is_same_v<T,Abi>){
            validate_abi(value);put(value.gp);put(value.xmm);put(value.eflags);put(value.fs_base);
            put(value.x87_depth);put(value.x87_status);
            // Only live 80-bit values, with no host long-double padding or
            // invented dead x87/MMX lanes. This matches the native owner.
            if constexpr(native_extended)for(unsigned n=0;n<value.x87_depth;++n){
                std::array<std::byte,10> raw;std::memcpy(raw.data(),&value.x87[n],raw.size());put(raw);
            }
            put(value.floating_point);put(value.active_mmx.has_value());
            if(value.active_mmx){put(value.active_mmx->known_mask);
                for(unsigned n=0;n<8;++n)if(value.active_mmx->known_mask&(1u<<n))put(value.active_mmx->physical[n]);}
        }else if constexpr(std::is_same_v<T,SourceBspPartCommandState>){
            validate_attachment_state(value);put(value.banks);put(value.special_units);put(value.special_count);
            // The diagnostic entry trace has its own bound, independent of
            // the portal collector's128-element vector<uint32_t> domain.
            put(std::uint32_t(value.calls.size()));for(auto call:value.calls)put(call);put(value.main_object_counts);
        }else if constexpr(std::is_same_v<T,NativeGrenadeAction>){
            put(value.source);put(value.graph);put(value.clip);put(value.activity);put(value.frame);put(value.animation_active);
        }else if constexpr(std::is_floating_point_v<T>){
            static_assert(sizeof(T)==4||sizeof(T)==8);put(std::bit_cast<std::conditional_t<sizeof(T)==4,std::uint32_t,std::uint64_t>>(value));
        }else if constexpr(std::is_enum_v<T>)put(std::underlying_type_t<T>(value));
        else if constexpr(std::is_same_v<T,std::string>){
            if(value.size()>8192)throw FormatError("Native state string exceeds its bound");
            put(std::uint32_t(value.size()));for(auto c:value)put(std::uint8_t(c));
        }
        else if constexpr(Shape<T>::value==1){
            if(value.size()>sequence_bound<T>())throw FormatError("Native call sequence exceeds its element bound");
            put(std::uint32_t(value.size()));for(const auto &v:value)put(v);
        }else if constexpr(Shape<T>::value==2){for(const auto &v:value)put(v);}
        else if constexpr(Shape<T>::value==3){put(value.has_value());if(value)put(*value);}
        else if constexpr(std::is_aggregate_v<T>)boost::pfr::for_each_field(value,[&](const auto &v){put(v);});
        else static_assert(unsupported<T>,"Native call arguments must be owning value fields");
    }
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    template<class T>T get(){
        if constexpr(std::is_same_v<T,bool>){const auto v=get<std::uint8_t>();if(v>1)throw FormatError("Invalid native call boolean");return v;}
        else if constexpr(std::is_integral_v<T>){
            if(sizeof(T)>bytes.size()-at)throw FormatError("Truncated native call arguments");
            std::make_unsigned_t<T> bits{};for(unsigned n=0;n<sizeof(T);++n)bits|=std::make_unsigned_t<T>(std::to_integer<unsigned>(bytes[at++]))<<(8*n);
            return std::bit_cast<T>(bits);
        }else if constexpr(std::is_same_v<T,Abi>){
            T value{};value.gp=get<decltype(value.gp)>();value.xmm=get<decltype(value.xmm)>();
            value.eflags=get<std::uint32_t>();value.fs_base=get<std::uint32_t>();
            value.x87_depth=get<unsigned>();value.x87_status=get<std::uint16_t>();
            validate_abi(value);
            if constexpr(native_extended)for(unsigned n=0;n<value.x87_depth;++n){
                const auto raw=get<std::array<std::byte,10>>();std::memcpy(&value.x87[n],raw.data(),raw.size());
            }
            value.floating_point=get<decltype(value.floating_point)>();
            if(get<bool>()){auto &mmx=value.active_mmx.emplace();mmx.known_mask=get<std::uint8_t>();
                for(unsigned n=0;n<8;++n)if(mmx.known_mask&(1u<<n))mmx.physical[n]=get<std::array<std::byte,10>>();}
            validate_abi(value);return value;
        }else if constexpr(std::is_same_v<T,SourceBspPartCommandState>){
            T value;value.banks=get<decltype(value.banks)>();value.special_units=get<decltype(value.special_units)>();value.special_count=get<std::int16_t>();
            const auto n=get<std::uint32_t>();if(n>65536||n>(bytes.size()-at)/4)throw FormatError("Invalid native attachment trace bound");
            value.calls.reserve(n);for(unsigned i=0;i<n;++i)value.calls.push_back(get<std::uint32_t>());
            value.main_object_counts=get<decltype(value.main_object_counts)>();validate_attachment_state(value);return value;
        }else if constexpr(std::is_floating_point_v<T>){
            static_assert(sizeof(T)==4||sizeof(T)==8);return std::bit_cast<T>(get<std::conditional_t<sizeof(T)==4,std::uint32_t,std::uint64_t>>());
        }else if constexpr(std::is_enum_v<T>){static_assert(!std::is_convertible_v<T,int>);return T(get<std::underlying_type_t<T>>());}
        else if constexpr(Shape<T>::value==1){
            const auto n=get<std::uint32_t>();if(n>sequence_bound<T>()||n>bytes.size()-at)throw FormatError("Invalid native call sequence bound");
            T value;value.reserve(n);for(unsigned i=0;i<n;++i)value.push_back(get<typename T::value_type>());return value;
        }else if constexpr(Shape<T>::value==2){T value;for(auto &v:value)v=get<typename T::value_type>();return value;}
        else if constexpr(Shape<T>::value==3){if(get<bool>())return T(get<typename T::value_type>());return {};}
        else if constexpr(std::is_aggregate_v<T>){T value{};boost::pfr::for_each_field(value,[&](auto &v){v=get<std::remove_cvref_t<decltype(v)>>();});return value;}
        else static_assert(unsupported<T>,"Native call arguments must be owning value fields");
    }
};
template<class...T>std::vector<std::byte> arguments(const T &...values){Writer out;(out.put(values),...);return std::move(out.bytes);}
template<class...T>std::tuple<T...> decode(std::span<const std::byte> bytes){
    Reader in{bytes};std::tuple<T...> result{in.get<T>()...};
    if(in.at!=bytes.size())throw FormatError("Trailing native call arguments");
    return result;
}
std::vector<std::uint32_t> read_native_arguments(Reader &in){
    const auto n=in.get<std::uint32_t>();
    if(n>0x1000||n>(in.bytes.size()-in.at)/4)throw FormatError("Invalid native ABI argument count");
    std::vector<std::uint32_t> args;args.reserve(n);for(unsigned i=0;i<n;++i)args.push_back(in.get<std::uint32_t>());return args;
}
void write_native_arguments(Writer &out,std::span<const std::uint32_t> args){
    if(args.size()>0x1000)throw FormatError("Native ABI argument count exceeds its frame");
    out.put(std::uint32_t(args.size()));for(auto arg:args)out.put(arg);
}
void complete_arguments(const Reader &in){if(in.at!=in.bytes.size())throw FormatError("Trailing native ABI arguments");}
void process(const SourceExecutionCall &call,MultiplayerMatch *match,MultiplayerObjectRuntime *world){
    using K=SourceExecutionCallKind;
    if(!source_execution_call_has_result(call.kind)&&!call.result.empty())throw FormatError("Unexpected result for a void native call");
    switch(call.kind){
    case K::MatchStep:{auto [controls]=decode<std::array<Controls,16>>(call.arguments);if(match)match->step(controls);break;}
    case K::MatchState:{decode<>(call.arguments);
        if(call.result.size()!=32)throw FormatError("Invalid actual native state result digest");
        if(match)(void)match->state();break;}
    case K::PlayerCamera:{auto [input]=decode<SourcePlayerCameraCall>(call.arguments);
        if(input.player==0xffffffffu||(input.player&0xffffu)>=16||!std::isfinite(input.seconds)||input.seconds<0||call.result.size()!=32)
            throw FormatError("Invalid native player-camera call or result digest");
        if(world){std::uint8_t mode{};(void)world->tu15_world().player_camera_packet(input.player,input.seconds,
            input.controller,input.observer,input.random,input.cycle,input.mode_output?&mode:nullptr);}break;}
    case K::LensQuery:{auto [entry,abi]=decode<std::uint32_t,Abi>(call.arguments);
        if(!source_lens_query_tu15_owns_entry(entry))throw FormatError("Unowned original lens query journal entry");
        decode<Abi,std::array<std::byte,32>>(call.result);
        if(world)source_execute_lens_query(world->tu15_world(),entry,abi);break;}
    case K::LensQueryFinish:{auto [submitted,rollback]=decode<bool,std::vector<SourceLensQueryRollback>>(call.arguments);
        if(rollback.size()>source_lens_query_capacity)throw FormatError("Lens query rollback exceeds its ring");
        for(const auto &r:rollback)if(r.id>=source_lens_query_capacity||r.area>65535)throw FormatError("Invalid original lens query rollback");
        if(world)source_finish_lens_query(world->tu15_world(),submitted,rollback);break;}
    case K::LensDraw:{auto [entry,abi,planning,regions]=decode<std::uint32_t,Abi,bool,std::vector<SourceLensDeviceRegion>>(call.arguments);
        if(!source_lens_draw_execution_entry(entry))throw FormatError("Unowned original lens drawing journal entry");
        validate_source_lens_device_regions(regions);auto [returned,writes,pass]=decode<Abi,std::array<std::byte,32>,SourceLensWorldPass>(call.result);
        if(pass&&!planning)throw FormatError("Unrequested original lens world-pass boundary");
        if(world)(void)source_execute_lens_draw(world->tu15_world(),entry,abi,planning,regions);break;}
    case K::ParticleCollision:{auto [flags,origin,delta]=decode<std::uint32_t,Vec3,Vec3>(call.arguments);
        decode<std::optional<ParticleCollisionHit>>(call.result);
        if(world)(void)world->particle_collision(flags,origin,delta);break;}
    case K::EffectParticles:{auto [seconds]=decode<float>(call.arguments);if(world)world->advance_effect_particles(seconds);break;}
    case K::BeginModelFrame:decode<>(call.arguments);if(world)world->begin_model_frame();break;
    case K::BeginModelView:decode<>(call.arguments);if(world)world->tu15_world().begin_model_view();break;
    case K::ObserverPosition:{auto [view,position]=decode<unsigned,Vec3>(call.arguments);if(world)world->publish_native_observer_position(view,position);break;}
    case K::RenderVisibility:{auto [visibility]=decode<SourcePortalVisibilitySet>(call.arguments);if(world)world->publish_render_visibility(visibility);break;}
    case K::WeatherView:{auto [time,view]=decode<double,std::int16_t>(call.arguments);if(match)match->advance_weather_view(time,view);break;}
    case K::BuildWorldModel:{auto [object,view,players,modifier,geometry,lighting]=decode<std::uint32_t,unsigned,unsigned,bool,float,float>(call.arguments);
        if(world)(void)world->build_world_model(object,view,players,modifier,geometry,lighting);break;}
    case K::FirstPersonModels:{auto [view,position,forward,up]=decode<unsigned,Vec3,Vec3,Vec3>(call.arguments);
        if(world)(void)world->tu15_world().prepare_first_person_models(view,position,forward,up);break;}
    case K::AdvanceAudio:{auto [seconds]=decode<float>(call.arguments);if(match)match->advance_audio(seconds);break;}
    case K::TakeSounds:decode<>(call.arguments);if(match)(void)match->take_sounds();break;
    case K::TakePropEvents:decode<>(call.arguments);if(match)(void)match->take_prop_events();break;
    case K::TakeGrenadeEvents:decode<>(call.arguments);if(match)(void)match->take_grenade_events();break;
    case K::DepartPlayer:{auto [slot]=decode<unsigned>(call.arguments);if(match)(void)match->depart(slot);break;}
    case K::WriteSourceCallInput:{
        Reader in{call.arguments};const auto address=in.get<std::uint32_t>(),n=in.get<std::uint32_t>();
        if(n!=in.bytes.size()-in.at||std::uint64_t(address)+n>0x100000000ull)throw FormatError("Invalid native caller input extent");
        if(world)world->tu15_world().write_source_call_input(address,in.bytes.subspan(in.at));break;}
    case K::PrepareSourceCall:{
        Reader in{call.arguments};const auto args=read_native_arguments(in);complete_arguments(in);decode<Abi>(call.result);
        if(world)(void)world->tu15_world().prepare_source_call(args);break;}
    case K::RunSourceCall:case K::RunActualSourceCall:{
        auto [entry,abi]=decode<std::uint32_t,Abi>(call.arguments);decode<Abi>(call.result);
        if(world){if(call.kind==K::RunSourceCall)world->tu15_world().run(entry,abi);else world->tu15_world().run_actual(entry,abi);}break;}
    case K::CallSource:case K::CallActualSource:{
        Reader in{call.arguments};const auto entry=in.get<std::uint32_t>();const auto args=read_native_arguments(in);
        complete_arguments(in);decode<std::uint32_t>(call.result);
        if(world){if(call.kind==K::CallSource)(void)world->tu15_world().call(entry,args);else (void)world->tu15_world().call_actual(entry,args);}break;}
    case K::LiveModelDraws:{
        auto [admission,state,input]=decode<SourceObjectAdmission,SourceDrawUnitState,SourceExecutionModelInputTu15>(call.arguments);
        validate_model_input(admission,state,input);
        if(call.result.size()!=32)throw FormatError("Invalid native live-model result digest");
        if(world)source_live_model_draws_tu15(world->tu15_world(),admission,state,source_execution_model_context(input));break;}
    case K::RenderAttachments:{
        auto [state,queue,mode]=decode<SourceBspPartCommandState,SourceObjectAdmission,std::uint32_t>(call.arguments);
        validate_attachment_input(state,queue,mode);if(call.result.size()!=32)throw FormatError("Invalid native attachment result digest");
        if(world)world->tu15_world().append_render_attachments(state,queue,mode);break;}
    case K::RenderGeometry:{
        const auto [flags,header]=decode<std::uint32_t,std::uint32_t>(call.arguments);const auto [result]=decode<std::uint32_t>(call.result);
        if(result>1)throw FormatError("Invalid native geometry request boolean");
        if(world)(void)world->tu15_world().request_render_geometry(flags,header);break;}
    default:throw FormatError("Unknown native execution call kind");
    }
}
}
std::vector<std::byte> source_execution_arguments(const std::array<Controls,16> &v){return arguments(v);}
std::vector<std::byte> source_execution_arguments(float v){return arguments(v);}
std::vector<std::byte> source_execution_arguments(unsigned v){return arguments(v);}
std::vector<std::byte> source_execution_arguments(std::uint32_t a,std::uint32_t b){return arguments(a,b);}
std::vector<std::byte> source_execution_arguments(unsigned v,Vec3 p){return arguments(v,p);}
std::vector<std::byte> source_execution_arguments(const SourcePortalVisibilitySet &v){return arguments(v);}
std::vector<std::byte> source_execution_arguments(double t,std::int16_t v){return arguments(t,v);}
std::vector<std::byte> source_execution_arguments(std::uint32_t o,unsigned v,unsigned p,bool m,float g,float l){return arguments(o,v,p,m,g,l);}
std::vector<std::byte> source_execution_arguments(unsigned v,Vec3 p,Vec3 f,Vec3 u){return arguments(v,p,f,u);}
std::vector<std::byte> source_execution_arguments(std::span<const std::uint32_t> args){Writer out;write_native_arguments(out,args);return std::move(out.bytes);}
std::vector<std::byte> source_execution_arguments(std::uint32_t entry,std::span<const std::uint32_t> args){Writer out;out.put(entry);write_native_arguments(out,args);return std::move(out.bytes);}
std::vector<std::byte> source_execution_arguments(std::uint32_t address,std::span<const std::byte> bytes){
    if(bytes.size()>SourceExecutionInputs::maximum_call_bytes-8||std::uint64_t(address)+bytes.size()>0x100000000ull)
        throw FormatError("Native caller input exceeds its extent or byte budget");
    Writer out;out.put(address);out.put(std::uint32_t(bytes.size()));out.bytes.insert(out.bytes.end(),bytes.begin(),bytes.end());return std::move(out.bytes);
}
std::vector<std::byte> source_execution_arguments(std::uint32_t entry,const Abi &abi){return arguments(entry,abi);}
std::vector<std::byte> source_execution_result(std::uint32_t value){return arguments(value);}
std::vector<std::byte> source_execution_result(const Abi &abi){return arguments(abi);}
std::vector<std::byte> source_execution_arguments(const SourceObjectAdmission &admission,const SourceDrawUnitState &state,const SourceExecutionModelInputTu15 &input){
    validate_model_input(admission,state,input);return arguments(admission,state,input);
}
std::vector<std::byte> source_execution_model_result(const SourceObjectAdmission &admission,const SourceDrawUnitState &state){
    validate_model_state(admission,state);const auto bytes=arguments(admission,state);
    std::vector<std::byte> digest(32);unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,EVP_sha256(),nullptr)!=1||size!=digest.size())
        throw FormatError("Cannot seal actual native live-model draw results");
    return digest;
}
std::vector<std::byte> source_execution_arguments(const SourceBspPartCommandState &state,const SourceObjectAdmission &queue,std::uint32_t mode){
    validate_attachment_input(state,queue,mode);return arguments(state,queue,mode);
}
std::vector<std::byte> source_execution_attachment_result(const SourceBspPartCommandState &state){
    validate_attachment_state(state);const auto bytes=arguments(state);std::vector<std::byte> digest(32);unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,EVP_sha256(),nullptr)!=1||size!=digest.size())
        throw FormatError("Cannot seal actual native attachment results");
    return digest;
}
std::vector<std::byte> source_execution_state_result(const MultiplayerState &state){
    const auto bytes=arguments(state);std::vector<std::byte> digest(32);unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,EVP_sha256(),nullptr)!=1||size!=digest.size())
        throw FormatError("Cannot seal actual native state publication");
    return digest;
}
std::vector<std::byte> source_execution_arguments(const SourcePlayerCameraCall &input){return arguments(input);}
std::vector<std::byte> source_execution_camera_result(const DeathCameraPacket &packet,
    const SourceCameraControllerState &controller,const SourceObserverState &observer,std::uint32_t random,
    std::optional<std::uint8_t> mode){
    const auto bytes=arguments(packet,controller,observer,random,mode);std::vector<std::byte> digest(32);unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,EVP_sha256(),nullptr)!=1||size!=digest.size())
        throw FormatError("Cannot seal actual native camera packet/controller/observer results");
    return digest;
}
std::vector<std::byte> source_execution_lens_result(const Abi &abi,const std::array<std::byte,32>&writes){return arguments(abi,writes);}
std::vector<std::byte> source_execution_arguments(bool submitted,std::span<const SourceLensQueryRollback> rollback){
    if(rollback.size()>source_lens_query_capacity)throw FormatError("Lens query rollback exceeds its ring");
    return arguments(submitted,std::vector<SourceLensQueryRollback>(rollback.begin(),rollback.end()));
}
void validate_source_lens_device_regions(std::span<const SourceLensDeviceRegion> regions){
    if(regions.size()>65536)throw FormatError("Original lens device has too many private regions");
    std::uint64_t end{};for(const auto &region:regions){
        const auto next=std::uint64_t(region.address)+region.bytes;
        if(!region.bytes||region.address<end||next>0x100000000ull)throw FormatError("Invalid or overlapping original lens device regions");
        end=next;
    }
}
std::vector<std::byte> source_execution_arguments(std::uint32_t entry,const Abi &abi,bool planning,std::span<const SourceLensDeviceRegion> regions){
    validate_source_lens_device_regions(regions);return arguments(entry,abi,planning,std::vector<SourceLensDeviceRegion>(regions.begin(),regions.end()));
}
std::vector<std::byte> source_execution_arguments(std::uint32_t entry,const Abi &abi,bool planning,const SourceLensDeviceDomains &domains){
    if(domains.size()>65536)throw FormatError("Original lens device has too many private regions");
    std::vector<SourceLensDeviceRegion> regions;regions.reserve(domains.size());
    for(auto [address,size]:domains){if(size>UINT32_MAX)throw FormatError("Original lens device region exceeds its address width");regions.push_back({address,std::uint32_t(size)});}
    return source_execution_arguments(entry,abi,planning,regions);
}
std::vector<std::byte> source_execution_lens_draw_result(const Abi &abi,const std::array<std::byte,32>&writes,const SourceLensWorldPass &pass){return arguments(abi,writes,pass);}
std::vector<std::byte> source_execution_arguments(std::uint32_t flags,Vec3 origin,Vec3 delta){return arguments(flags,origin,delta);}
std::vector<std::byte> source_execution_collision_result(const std::optional<ParticleCollisionHit> &hit){return arguments(hit);}
void validate_source_execution_calls(std::span<const SourceExecutionCall> calls){
    if(calls.size()>SourceExecutionInputs::maximum_calls)throw FormatError("Native call count exceeds its bound");
    std::size_t total{};for(const auto &call:calls){
        if(call.arguments.size()>SourceExecutionInputs::maximum_call_bytes-total)throw FormatError("Native call arguments exceed the phase budget");
        total+=call.arguments.size();
        if(call.result.size()>SourceExecutionInputs::maximum_call_bytes-total)throw FormatError("Native call results exceed the phase budget");
        total+=call.result.size();process(call,nullptr,nullptr);
    }
}
void replay_source_execution_calls(MultiplayerMatch &match,MultiplayerObjectRuntime &world,std::span<const SourceExecutionCall> calls){
    for(const auto &call:calls)process(call,&match,&world);
}
}
