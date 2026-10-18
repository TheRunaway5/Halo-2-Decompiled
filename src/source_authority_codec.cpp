#include "halo2/source_authority_codec.hpp"
#include "halo2/source_runtime_snapshot_bytes.hpp"
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <bit>
#include <deque>
#include <limits>
#include <set>
#include <tuple>
#include <type_traits>

namespace halo2 {
namespace authority_archive {
static_assert(boost::pfr::tuple_size_v<SmgState> == 26,"Update the explicit SMG archive fields when its state changes");
static_assert(sizeof(NativeGrenadeAction)==88,"Update the explicit grenade action archive when its state changes");
template<class T>struct Field {std::string_view name;T &value;};
template<class T>Field<T> field(std::string_view name,T &value){return {name,value};}
template<class T>inline constexpr bool custom=
    std::is_same_v<T,DatumPool>||std::is_same_v<T,SourceObjectBodyStorage>||
    std::is_same_v<T,SourceHsListBank>||std::is_same_v<T,SoundClassFades>||std::is_same_v<T,NativeGrenadeAction>||std::is_same_v<T,SmgState>;
}
struct SourceAuthorityArchiveAccess {
    template<class A,class T>static void visit(A &a,T &v){
        using namespace authority_archive;using R=std::remove_cv_t<T>;
        if constexpr(std::is_same_v<R,DatumPool>){
            if(v.source_memory_||!v.source_pool_.is_null()||!v.bitmap_view_.empty())
                throw FormatError("Authority archive cannot import a process-backed datum view");
            a.object(field("name",v.name_),field("capacity",v.capacity_),field("stride",v.stride_),
                field("cursor",v.cursor_),field("high_water",v.high_water_),field("count",v.count_),
                field("flags",v.flags_),field("next_salt",v.next_salt_),field("elements",v.elements_),field("bitmap",v.bitmap_));
            if constexpr(A::reading){
                if(v.capacity_<=0||v.capacity_>65535||v.stride_<2||v.name_.size()<2||v.name_.size()>31||
                    std::uint64_t(v.capacity_)*std::uint64_t(v.stride_)!=v.elements_.size()||
                    v.bitmap_.size()!=(std::size_t(v.capacity_)+31)/32||v.cursor_<0||v.cursor_>v.capacity_||
                    v.high_water_<0||v.high_water_>v.capacity_||v.count_<0||v.count_>v.high_water_)
                    throw FormatError("Authority archive has an invalid owned datum shape");
                unsigned count=0;
                for(unsigned i=0;i<v.bitmap_.size()*32;++i)if(v.bitmap_[i/32]&(1u<<(i%32))){
                    if(i>=unsigned(v.high_water_)||!v.salt(int(i)))throw FormatError("Authority archive has invalid datum occupancy");++count;}
                if(count!=unsigned(v.count_))throw FormatError("Authority archive datum census differs");
            }
        }else if constexpr(std::is_same_v<R,SourceObjectBodyStorage>){
            a.object(field("capacity",v.capacity_),field("arena",v.arena_),field("scratch",v.scratch_),field("handles",v.handles_));
            if constexpr(A::reading)if(!v.capacity_||v.capacity_>0x100000||v.arena_.size()!=((v.capacity_+0x53u)&~3u))
                throw FormatError("Authority archive has invalid native body backing");
        }else if constexpr(std::is_same_v<R,SourceHsListBank>){
            a.object(field("headers",v.headers_),field("references",v.references_));
            if constexpr(A::reading){
                if(v.headers_.capacity()!=48||v.references_.capacity()!=128||v.headers_.stride()!=12||v.references_.stride()!=12)
                    throw FormatError("Authority archive changes native script list capacities");
                const auto bits=v.headers_.bitmap();
                for(unsigned i=0;i<unsigned(v.headers_.high_water());++i)if(bits[i/32]&(1u<<(i%32))){
                    const auto row=v.headers_.elements().subspan(i*12,12);
                    const auto id=(std::uint32_t(row[0]|(std::uint16_t(row[1])<<8))<<16)|i;
                    (void)v.objects(id);
                }
            }
        }else if constexpr(std::is_same_v<R,SoundClassFades>){a.object(field("fades",v.fades_));}
        else if constexpr(std::is_same_v<R,NativeGrenadeAction>){
            a.object(field("source",v.source),field("graph",v.graph),field("clip",v.clip),field("activity",v.activity),
                field("frame",v.frame),field("animation_active",v.animation_active));
        }else if constexpr(std::is_same_v<R,SmgState>){
            a.object(field("barrel",v.barrel),field("loaded",v.loaded),field("reserve",v.reserve),
                field("magazine_mode",v.magazine_mode),field("magazine_ticks",v.magazine_ticks),
                field("transfer_ticks",v.transfer_ticks),field("blocking_ticks",v.blocking_ticks),
                field("weapon_flags",v.weapon_flags),field("input_flags",v.input_flags),field("trigger_flags",v.trigger_flags),
                field("trigger_ticks",v.trigger_ticks),field("trigger_state",v.trigger_state),field("hand_timers",v.hand_timers),
                field("weapon_state",v.weapon_state),field("held",v.held),field("reload_empty",v.reload_empty),field("marker",v.marker),
                field("second_trigger_state",v.second_trigger_state),field("own_flags",v.own_flags),field("own_events",v.own_events),
                field("magazine_other_ticks",v.magazine_other_ticks),field("trigger_effect_handle",v.trigger_effect_handle),
                field("own_manager",v.own_manager),field("attached",v.attached),field("hand_timer_registered",v.hand_timer_registered));
        }
    }
};
namespace authority_archive {
enum Tag:std::uint8_t {Boolean,Unsigned,Signed,Float,Double,String,Blob,Sequence,Fixed,Map,Set,Optional,Pointer,Record,Enumeration};
template<class T>struct Kind {static constexpr int value=0;};
template<class T,class A>struct Kind<std::vector<T,A>>{static constexpr int value=1;};
template<class T,class A>struct Kind<std::deque<T,A>>{static constexpr int value=1;};
template<class T,std::size_t N>struct Kind<std::array<T,N>>{static constexpr int value=2;};
template<class K,class V,class C,class A>struct Kind<std::map<K,V,C,A>>{static constexpr int value=3;};
template<class K,class C,class A>struct Kind<std::set<K,C,A>>{static constexpr int value=4;};
template<class T>struct Kind<std::optional<T>>{static constexpr int value=5;};
template<class T>struct Kind<std::shared_ptr<T>>{static constexpr int value=6;};
template<class A,class B>struct Kind<std::pair<A,B>>{static constexpr int value=7;};
template<class T>inline constexpr bool byte_like=std::is_same_v<T,std::byte>||std::is_same_v<T,char>||
    std::is_same_v<T,unsigned char>||std::is_same_v<T,signed char>;
template<class T>inline constexpr bool unhandled=false;
template<class T>bool enum_allowed(std::underlying_type_t<T> value){
    using U=std::underlying_type_t<T>;
    // A scoped enum has a fixed underlying domain. Unscoped state enums need
    // admission before constructing/reading an out-of-range enum object.
    if constexpr(!std::is_convertible_v<T,int>)return true;
    else if constexpr(std::is_same_v<T,Unit::State>)return value>=U(Unit::Idle)&&value<=U(Unit::Dead);
    else if constexpr(std::is_same_v<T,Unit::SeatAction::Phase>)return value>=U(Unit::SeatAction::None)&&value<=U(Unit::SeatAction::Exiting);
    else if constexpr(std::is_same_v<T,SoundEvent::Action>)return value>=U(SoundEvent::Impulse)&&value<=U(SoundEvent::NonSpatialImpulse);
    else if constexpr(std::is_same_v<T,NativeGrenadeEvent::Kind>)return value>=U(NativeGrenadeEvent::Begin)&&value<=U(NativeGrenadeEvent::Unavailable);
    else static_assert(unhandled<T>,"Define the archive admission domain of this unscoped state enum");
}
template<class T>T make(){
    if constexpr(std::is_same_v<T,DatumPool>)return DatumPool("archive",1,2);
    else if constexpr(std::is_same_v<T,NativeObjectLifetimes>)throw FormatError("Source authority archive rejects a native-owned lifetime bank");
    else return T{};
}
class Writer {
public:
    static constexpr bool reading=false;
    SourceAuthorityCodecLimits limits;std::vector<std::byte> bytes;
    explicit Writer(SourceAuthorityCodecLimits l):limits(l){}
    void raw(std::span<const std::byte> value){
        if(value.size()>limits.decoded_bytes-bytes.size())throw FormatError("Authority archive exceeds decoded byte budget");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void put(std::uint64_t value,unsigned n){std::array<std::byte,8> b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(value>>(8*i));raw(std::span(b).first(n));}
    void tag(Tag t){put(t,1);}
    void name(std::string_view value){if(value.size()>65535)throw FormatError("Authority archive field name is too long");put(value.size(),2);raw(std::as_bytes(std::span(value)));}
    template<class... F>void object(F... fields){tag(Record);put(sizeof...(F),4);((name(fields.name),add(fields.value)),...);}
    template<class T>void add(const T &v){
        if constexpr(std::is_pointer_v<T>||std::is_same_v<T,NativeRuntimeAddress>)throw FormatError("Authority archive rejects process addresses");
        else if constexpr(std::is_same_v<T,NativeObjectLifetimes>)throw FormatError("Source authority archive rejects a native-owned lifetime bank");
        else if constexpr(std::is_same_v<T,bool>){tag(Boolean);put(v,1);}
        else if constexpr(std::is_same_v<T,char>){add(static_cast<unsigned char>(v));}
        else if constexpr(std::is_enum_v<T>){using U=std::underlying_type_t<T>;using B=std::make_unsigned_t<U>;
            static_assert(sizeof(U)<=8);if(!enum_allowed<T>(U(v)))throw FormatError("Authority archive has an invalid state enum");
            tag(Enumeration);put(std::uint64_t(B(U(v))),8);}
        else if constexpr(std::is_integral_v<T>){static_assert(sizeof(T)<=8);tag(std::is_signed_v<T>?Signed:Unsigned);put(std::uint64_t(v),8);}
        else if constexpr(std::is_same_v<T,float>){tag(Float);put(std::bit_cast<std::uint32_t>(v),4);}
        else if constexpr(std::is_same_v<T,double>){tag(Double);put(std::bit_cast<std::uint64_t>(v),8);}
        else if constexpr(std::is_same_v<T,std::string>){tag(String);put(v.size(),8);raw(std::as_bytes(std::span(v)));}
        else if constexpr(std::is_same_v<T,SourceRuntimeSnapshotBytes>){tag(Blob);put(v.size(),8);raw(std::span(v.data(),v.size()));}
        else if constexpr(std::is_array_v<T>){tag(Fixed);put(std::extent_v<T>,8);for(const auto &item:v)add(item);}
        else if constexpr(Kind<T>::value==1||Kind<T>::value==2){
            using V=typename T::value_type;
            if constexpr(byte_like<V>){tag(Blob);put(v.size(),8);raw(std::as_bytes(std::span(v)));}
            else {tag(Kind<T>::value==2?Fixed:Sequence);put(v.size(),8);for(const auto &item:v)add(item);}
        }else if constexpr(Kind<T>::value==3){tag(Map);put(v.size(),8);for(const auto &[k,value]:v){add(k);add(value);}}
        else if constexpr(Kind<T>::value==4){tag(Set);put(v.size(),8);for(const auto &k:v)add(k);}
        else if constexpr(Kind<T>::value==5){tag(Optional);put(bool(v),1);if(v)add(*v);}
        else if constexpr(Kind<T>::value==6){if(v)throw FormatError("Authority archive requires explicit deep pointer ownership");tag(Pointer);put(0,1);}
        else if constexpr(Kind<T>::value==7){object(field("first",v.first),field("second",v.second));}
        else if constexpr(custom<T>)SourceAuthorityArchiveAccess::visit(*this,v);
        else if constexpr(std::is_aggregate_v<T>){
            constexpr auto names=boost::pfr::names_as_array<T>();tag(Record);put(names.size(),4);
            boost::pfr::for_each_field(v,[&](const auto &value,auto index){name(names[index]);add(value);});
        }else static_assert(unhandled<T>,"Add an explicit owning archive visitor for this state type");
    }
};
class Reader {
    std::span<const std::byte> bytes;std::size_t position{};
    std::uint64_t allocation,remaining_elements;
public:
    static constexpr bool reading=true;
    Reader(std::span<const std::byte> b,SourceAuthorityCodecLimits l):bytes(b),allocation(l.allocated_bytes),remaining_elements(l.container_elements){}
    std::span<const std::byte> raw(std::uint64_t n){if(n>bytes.size()-position)throw FormatError("Truncated authority archive");auto v=bytes.subspan(position,std::size_t(n));position+=std::size_t(n);return v;}
    std::uint64_t get(unsigned n){const auto b=raw(n);std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint64_t>(b[i])<<(8*i);return v;}
    void tag(Tag expected){if(get(1)!=expected)throw FormatError("Authority archive field type differs");}
    bool present(){const auto value=get(1);if(value>1)throw FormatError("Authority archive has an invalid boolean");return value;}
    void claim(std::uint64_t count,std::uint64_t extent,bool elements=true){
        if(elements){if(count>remaining_elements||count>bytes.size()-position)throw FormatError("Authority archive exceeds element budget");remaining_elements-=count;}
        if(extent&&count>allocation/extent)throw FormatError("Authority archive exceeds allocation budget");allocation-=count*extent;
    }
    void name(std::string_view value){const auto n=get(2);const auto incoming=raw(n);
        if(n!=value.size()||!std::ranges::equal(incoming,std::as_bytes(std::span(value))))throw FormatError("Authority archive schema field differs");}
    template<class... F>void object(F... fields){tag(Record);if(get(4)!=sizeof...(F))throw FormatError("Authority archive schema extent differs");((name(fields.name),add(fields.value)),...);}
    template<class T>void add(T &v){
        if constexpr(std::is_pointer_v<T>||std::is_same_v<T,NativeRuntimeAddress>)throw FormatError("Authority archive rejects process addresses");
        else if constexpr(std::is_same_v<T,NativeObjectLifetimes>)throw FormatError("Source authority archive rejects a native-owned lifetime bank");
        else if constexpr(std::is_same_v<T,bool>){tag(Boolean);v=present();}
        else if constexpr(std::is_same_v<T,char>){unsigned char value{};add(value);v=std::bit_cast<char>(value);}
        else if constexpr(std::is_enum_v<T>){using U=std::underlying_type_t<T>;using B=std::make_unsigned_t<U>;
            static_assert(sizeof(U)<=8);tag(Enumeration);const auto bits=get(8);
            if(bits>std::numeric_limits<B>::max())throw FormatError("Authority archive enum value narrows");
            const auto value=std::bit_cast<U>(B(bits));if(!enum_allowed<T>(value))throw FormatError("Authority archive has an invalid state enum");v=T(value);}
        else if constexpr(std::is_integral_v<T>){
            static_assert(sizeof(T)<=8);
            tag(std::is_signed_v<T>?Signed:Unsigned);const auto bits=get(8);
            if constexpr(std::is_signed_v<T>){const auto value=std::bit_cast<std::int64_t>(bits);
                if(value<std::numeric_limits<T>::min()||value>std::numeric_limits<T>::max())throw FormatError("Authority archive signed value narrows");v=T(value);}
            else {if(bits>std::numeric_limits<T>::max())throw FormatError("Authority archive unsigned value narrows");v=T(bits);}
        }else if constexpr(std::is_same_v<T,float>){tag(Float);v=std::bit_cast<float>(std::uint32_t(get(4)));}
        else if constexpr(std::is_same_v<T,double>){tag(Double);v=std::bit_cast<double>(get(8));}
        else if constexpr(std::is_same_v<T,std::string>){tag(String);const auto n=get(8);claim(n,1,false);const auto b=raw(n);v.assign(reinterpret_cast<const char *>(b.data()),b.size());}
        else if constexpr(std::is_same_v<T,SourceRuntimeSnapshotBytes>){tag(Blob);const auto n=get(8);claim(n,1,false);v=SourceRuntimeSnapshotBytes(raw(n));}
        else if constexpr(std::is_array_v<T>){tag(Fixed);if(get(8)!=std::extent_v<T>)throw FormatError("Authority archive fixed C array differs");for(auto &item:v)add(item);}
        else if constexpr(Kind<T>::value==1||Kind<T>::value==2){
            using V=typename T::value_type;
            if constexpr(byte_like<V>){tag(Blob);const auto n=get(8);
                const auto b=raw(n);
                if constexpr(Kind<T>::value==2){if(n!=v.size())throw FormatError("Authority archive fixed byte extent differs");}
                else {claim(n,sizeof(V),false);v.resize(std::size_t(n));}
                std::copy(b.begin(),b.end(),reinterpret_cast<std::byte *>(v.data()));
            }else {tag(Kind<T>::value==2?Fixed:Sequence);const auto n=get(8);
                if constexpr(Kind<T>::value==2){if(n!=v.size())throw FormatError("Authority archive fixed extent differs");for(auto &item:v)add(item);}
                else {claim(n,sizeof(V));v.clear();
                    if constexpr(requires {v.reserve(std::size_t(n));})v.reserve(std::size_t(n));
                    for(std::uint64_t i=0;i<n;++i){auto item=make<V>();add(item);v.push_back(std::move(item));}}
            }
        }else if constexpr(Kind<T>::value==3){tag(Map);const auto n=get(8);claim(n,sizeof(typename T::value_type)+64);v.clear();
            for(std::uint64_t i=0;i<n;++i){auto key=make<typename T::key_type>();auto item=make<typename T::mapped_type>();add(key);add(item);
                if(!v.emplace(std::move(key),std::move(item)).second)throw FormatError("Authority archive duplicates a map key");}}
        else if constexpr(Kind<T>::value==4){tag(Set);const auto n=get(8);claim(n,sizeof(typename T::value_type)+64);v.clear();
            for(std::uint64_t i=0;i<n;++i){auto key=make<typename T::key_type>();add(key);if(!v.emplace(std::move(key)).second)throw FormatError("Authority archive duplicates a set key");}}
        else if constexpr(Kind<T>::value==5){tag(Optional);if(present()){auto item=make<typename T::value_type>();add(item);v=std::move(item);}else v.reset();}
        else if constexpr(Kind<T>::value==6){tag(Pointer);if(present())throw FormatError("Authority archive imports a process pointer");v.reset();}
        else if constexpr(Kind<T>::value==7){object(field("first",v.first),field("second",v.second));}
        else if constexpr(custom<T>)SourceAuthorityArchiveAccess::visit(*this,v);
        else if constexpr(std::is_aggregate_v<T>){constexpr auto names=boost::pfr::names_as_array<T>();tag(Record);
            if(get(4)!=names.size())throw FormatError("Authority archive schema extent differs");
            boost::pfr::for_each_field(v,[&](auto &value,auto index){name(names[index]);add(value);});
        }else static_assert(unhandled<T>,"Add an explicit owning archive visitor for this state type");
    }
    bool complete()const{return position==bytes.size();}
};
constexpr std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'A'},std::byte{'U'},std::byte{'T'},std::byte{'H'},std::byte{'0'},std::byte{'1'}};
std::array<std::byte,32> digest(std::span<const std::byte> bytes){std::array<std::byte,32> result{};
    if(!SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data())))
        throw FormatError("Cannot checksum authority archive");return result;}
void source_only(const SourceMatchAuthority::Checkpoint &v){
    if(v.version!=2||v.match.version!=2||v.world.world.version!=2||!v.match.source_backed||!v.match.simulation.source_backed||v.match.owned_players||v.match.simulation.owned_lifetimes)
        throw FormatError("Authority archive requires the complete coupled TU1.5 source checkpoint");
}
}
std::vector<std::byte> encode_source_authority(const SourceMatchAuthority::Checkpoint &saved,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;source_only(saved);Writer body(limits);body.add(saved);
    if(body.bytes.size()>std::numeric_limits<uLong>::max())throw FormatError("Authority archive exceeds compression extent");
    const auto bound=compressBound(uLong(body.bytes.size()));
    if(limits.encoded_bytes<64)throw FormatError("Authority archive exceeds encoded byte budget");
    const auto capacity=std::min<std::uint64_t>(bound,limits.encoded_bytes-64);
    std::vector<std::byte> compressed(static_cast<std::size_t>(capacity));uLongf n=uLongf(capacity);
    if(compress2(reinterpret_cast<Bytef *>(compressed.data()),&n,reinterpret_cast<const Bytef *>(body.bytes.data()),uLong(body.bytes.size()),Z_BEST_SPEED)!=Z_OK)
        throw FormatError("Cannot compress authority archive");compressed.resize(n);
    if(n>limits.encoded_bytes-64)throw FormatError("Authority archive exceeds encoded byte budget");
    Writer envelope({limits.encoded_bytes,limits.encoded_bytes,limits.allocated_bytes,limits.container_elements});
    envelope.raw(magic);envelope.put(1,4);envelope.put(0,4);envelope.put(body.bytes.size(),8);envelope.put(n,8);envelope.raw(digest(body.bytes));envelope.raw(compressed);
    return std::move(envelope.bytes);
}
SourceMatchAuthority::Checkpoint decode_source_authority(std::span<const std::byte> bytes,SourceAuthorityCodecLimits limits){
    using namespace authority_archive;
    if(bytes.size()<64||bytes.size()>limits.encoded_bytes)throw FormatError("Authority archive envelope exceeds admitted extent");
    Reader envelope(bytes,limits);if(!std::ranges::equal(envelope.raw(8),magic)||envelope.get(4)!=1||envelope.get(4)!=0)
        throw FormatError("Unknown authority archive envelope");
    const auto raw_size=envelope.get(8),compressed_size=envelope.get(8);const auto expected=envelope.raw(32);
    if(raw_size>limits.decoded_bytes||raw_size>std::numeric_limits<uLongf>::max()||compressed_size!=bytes.size()-64||
        compressed_size>std::numeric_limits<uLong>::max())throw FormatError("Authority archive compression extent exceeds admitted budget");
    std::vector<std::byte> raw(static_cast<std::size_t>(raw_size));uLongf n=uLongf(raw_size);uLong source=uLong(compressed_size);
    if(uncompress2(reinterpret_cast<Bytef *>(raw.data()),&n,reinterpret_cast<const Bytef *>(bytes.data()+64),&source)!=Z_OK||n!=raw_size||source!=compressed_size||
        !std::ranges::equal(digest(raw),expected))throw FormatError("Authority archive has corrupt or trailing compressed data");
    Reader reader(raw,limits);SourceMatchAuthority::Checkpoint result;reader.add(result);
    if(!reader.complete())throw FormatError("Authority archive has trailing state");source_only(result);return result;
}
}
