#pragma once
// Included inside authority_archive after the native schema traits. This is a
// typed-value materializer, not a wire reader. Rust owns all framing, lengths,
// decompression, node topology and byte extents before native construction.
class Reader {
    std::unique_ptr<ParsedAuthorityArchive> values;
    std::size_t position{};
    AuthorityArchiveValue next(Tag expected) {
        if(position>=values->size())throw FormatError("Truncated authority value tree");
        const auto n=values->node(position++);
        if(n.tag!=expected)throw FormatError("Authority archive field type differs");
        return n;
    }
    void finish(const AuthorityArchiveValue &n)const {
        if(position!=n.next)throw FormatError("Authority archive typed subtree differs");
    }
public:
    static constexpr bool reading=true;
    explicit Reader(std::unique_ptr<ParsedAuthorityArchive> tree):values(std::move(tree)){}
    void claim(std::uint64_t count,std::uint64_t extent,bool elements=true) {
        if(!values->claim(count,extent,elements))throw FormatError("Authority archive exceeds materialization reservation budget");
    }
    void name(std::string_view expected)const {
        const auto incoming=values->name(position);
        if(!std::ranges::equal(incoming,std::as_bytes(std::span(expected))))
            throw FormatError("Authority archive schema field differs");
    }
    template<class... F>void object(F... fields) {
        const auto n=next(Record);
        if(n.count!=sizeof...(F))throw FormatError("Authority archive schema extent differs");
        ((name(fields.name),add(fields.value)),...);finish(n);
    }
    template<class T>void add(T &v) {
        if constexpr(std::is_pointer_v<T>||std::is_same_v<T,NativeRuntimeAddress>)throw FormatError("Authority archive rejects process addresses");
        else if constexpr(std::is_same_v<T,NativeObjectLifetimes>)throw FormatError("Source authority archive rejects a native-owned lifetime bank");
        else if constexpr(std::is_same_v<T,bool>)v=bool(next(Boolean).bits);
        else if constexpr(std::is_same_v<T,char>){unsigned char value{};add(value);v=std::bit_cast<char>(value);}
        else if constexpr(std::is_enum_v<T>){using U=std::underlying_type_t<T>;using B=std::make_unsigned_t<U>;
            const auto bits=next(Enumeration).bits;
            if(bits>std::numeric_limits<B>::max())throw FormatError("Authority archive enum value narrows");
            const auto value=std::bit_cast<U>(B(bits));
            if(!enum_allowed<T>(value))throw FormatError("Authority archive has an invalid state enum");v=T(value);
        }else if constexpr(std::is_integral_v<T>){
            const auto bits=next(std::is_signed_v<T>?Signed:Unsigned).bits;
            if constexpr(std::is_signed_v<T>){const auto value=std::bit_cast<std::int64_t>(bits);
                if(value<std::numeric_limits<T>::min()||value>std::numeric_limits<T>::max())throw FormatError("Authority archive signed value narrows");v=T(value);}
            else {if(bits>std::numeric_limits<T>::max())throw FormatError("Authority archive unsigned value narrows");v=T(bits);}
        }else if constexpr(std::is_same_v<T,float>)v=std::bit_cast<float>(std::uint32_t(next(Float).bits));
        else if constexpr(std::is_same_v<T,double>)v=std::bit_cast<double>(next(Double).bits);
        else if constexpr(std::is_same_v<T,std::string>){
            const auto index=position;const auto n=next(String);claim(n.count,1,false);
            const auto b=values->data(index);v.assign(reinterpret_cast<const char*>(b.data()),b.size());
        }else if constexpr(std::is_same_v<T,SourceRuntimeSnapshotBytes>){
            const auto index=position;const auto n=next(Blob);claim(n.count,1,false);v=SourceRuntimeSnapshotBytes(values->data(index));
        }else if constexpr(std::is_array_v<T>){
            const auto n=next(Fixed);if(n.count!=std::extent_v<T>)throw FormatError("Authority archive fixed C array differs");
            for(auto &item:v)add(item);finish(n);
        }else if constexpr(Kind<T>::value==1||Kind<T>::value==2){
            using V=typename T::value_type;
            if constexpr(byte_like<V>){
                const auto index=position;const auto n=next(Blob);
                if constexpr(Kind<T>::value==2){if(n.count!=v.size())throw FormatError("Authority archive fixed byte extent differs");}
                else {claim(n.count,sizeof(V),false);v.resize(std::size_t(n.count));}
                const auto b=values->data(index);std::copy(b.begin(),b.end(),reinterpret_cast<std::byte*>(v.data()));
            }else {
                const auto n=next(Kind<T>::value==2?Fixed:Sequence);
                if constexpr(Kind<T>::value==2){
                    if(n.count!=v.size())throw FormatError("Authority archive fixed extent differs");for(auto &item:v)add(item);
                }else {
                    claim(n.count,sizeof(V));v.clear();
                    if constexpr(requires {v.reserve(std::size_t(n.count));})v.reserve(std::size_t(n.count));
                    for(std::uint64_t i=0;i<n.count;++i){auto item=make<V>();add(item);v.push_back(std::move(item));}
                }
                finish(n);
            }
        }else if constexpr(Kind<T>::value==3){
            const auto n=next(Map);claim(n.count,sizeof(typename T::value_type)+64);v.clear();
            for(std::uint64_t i=0;i<n.count;++i){auto key=make<typename T::key_type>();auto item=make<typename T::mapped_type>();add(key);add(item);
                if(!v.emplace(std::move(key),std::move(item)).second)throw FormatError("Authority archive duplicates a map key");}finish(n);
        }else if constexpr(Kind<T>::value==4){
            const auto n=next(Set);claim(n.count,sizeof(typename T::value_type)+64);v.clear();
            for(std::uint64_t i=0;i<n.count;++i){auto key=make<typename T::key_type>();add(key);
                if(!v.emplace(std::move(key)).second)throw FormatError("Authority archive duplicates a set key");}finish(n);
        }else if constexpr(Kind<T>::value==5){
            const auto n=next(Optional);if(n.bits){auto item=make<typename T::value_type>();add(item);v=std::move(item);}else v.reset();finish(n);
        }else if constexpr(Kind<T>::value==6){next(Pointer);v.reset();}
        else if constexpr(Kind<T>::value==7){object(field("first",v.first),field("second",v.second));}
        else if constexpr(custom<T>)SourceAuthorityArchiveAccess::visit(*this,v);
        else if constexpr(std::is_aggregate_v<T>){
            constexpr auto names=boost::pfr::names_as_array<T>();const auto n=next(Record);
            if(n.count!=names.size())throw FormatError("Authority archive schema extent differs");
            boost::pfr::for_each_field(v,[&](auto &value,auto index){name(names[index]);add(value);});finish(n);
        }else static_assert(unhandled<T>,"Add an explicit owning archive visitor for this state type");
    }
    bool complete()const {return position==values->size();}
};
