#include "halo2/source_authority_delta.hpp"
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace halo2 {
namespace {
using Bytes=std::vector<std::byte>;
constexpr std::uint64_t window=64,stride=1024,multiplier=257;
constexpr std::size_t source_header=64,delta_header=144;
constexpr std::array<std::byte,8> source_magic{std::byte{'H'},std::byte{'2'},std::byte{'A'},std::byte{'U'},std::byte{'T'},std::byte{'H'},std::byte{'0'},std::byte{'1'}};
constexpr std::array<std::byte,8> delta_magic{std::byte{'H'},std::byte{'2'},std::byte{'D'},std::byte{'L'},std::byte{'T'},std::byte{'0'},std::byte{'0'},std::byte{'1'}};
void need(bool ok){if(!ok)throw std::invalid_argument("Invalid authority archive delta or admitted budget");}
void limits_ok(AuthorityDeltaLimits l){need(l.encoded_bytes>=delta_header&&l.encoded_bytes<=512ull*1024*1024&&
    l.decoded_bytes&&l.decoded_bytes<=1024ull*1024*1024&&l.patch_bytes>=8&&l.patch_bytes<=1024ull*1024*1024&&
    l.commands&&l.commands<=1'000'000&&l.working_bytes);}
void budget(AuthorityDeltaLimits l,std::initializer_list<std::uint64_t> parts){
    auto left=l.working_bytes;for(auto part:parts){need(part<=left);left-=part;}
}
// Generous enough for zero-filled native banks (and valid deflate's maximum
// expansion), but rejects a tiny forged stream's huge declaration preallocation.
void expansion(std::uint64_t raw,std::uint64_t compressed){
    need(compressed&&compressed<=UINT64_MAX/2048&&raw<=compressed*2048+65536);
}
void put(Bytes &p,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(v>>(8*i)));}
void append(Bytes &p,std::span<const std::byte> v){p.insert(p.end(),v.begin(),v.end());}
struct Reader {
    std::span<const std::byte> bytes;std::size_t position{};
    std::span<const std::byte> raw(std::uint64_t n){need(n<=bytes.size()-position);auto v=bytes.subspan(position,std::size_t(n));position+=std::size_t(n);return v;}
    std::uint64_t get(unsigned n){std::uint64_t v=0;auto p=raw(n);for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint64_t>(p[i])<<(8*i);return v;}
    AuthorityArchiveDigest digest(){AuthorityArchiveDigest value;auto p=raw(value.size());std::copy(p.begin(),p.end(),value.begin());return value;}
    bool done()const{return position==bytes.size();}
};
AuthorityArchiveDigest hash(std::span<const std::byte> bytes){AuthorityArchiveDigest result;
    need(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data())));return result;}
Bytes compress(std::span<const std::byte> raw,std::uint64_t maximum){
    need(!raw.empty()&&raw.size()<=std::numeric_limits<uLong>::max());
    const auto capacity=std::min<std::uint64_t>(compressBound(uLong(raw.size())),maximum);Bytes output(static_cast<std::size_t>(capacity));
    uLongf size=uLongf(capacity);need(compress2(reinterpret_cast<Bytef *>(output.data()),&size,
        reinterpret_cast<const Bytef *>(raw.data()),uLong(raw.size()),Z_BEST_SPEED)==Z_OK);
    output.resize(size);return output;
}
Bytes expand(std::span<const std::byte> compressed,std::uint64_t size){
    need(size&&size<=std::numeric_limits<uLongf>::max()&&compressed.size()<=std::numeric_limits<uLong>::max());
    Bytes output(static_cast<std::size_t>(size));uLongf actual=uLongf(size);uLong consumed=uLong(compressed.size());
    need(uncompress2(reinterpret_cast<Bytef *>(output.data()),&actual,reinterpret_cast<const Bytef *>(compressed.data()),&consumed)==Z_OK&&
        actual==size&&consumed==compressed.size());return output;
}
std::uint64_t rolling(std::span<const std::byte> bytes){std::uint64_t value=0;for(auto b:bytes)value=value*multiplier+std::to_integer<unsigned>(b)+1;return value;}
struct Bucket {std::array<std::uint32_t,8> offsets{};unsigned count{};};
Bytes patch(std::span<const std::byte> base,std::span<const std::byte> next,AuthorityDeltaLimits limits){
    std::unordered_map<std::uint64_t,Bucket> index;index.reserve(base.size()/stride+1);
    for(std::uint64_t at=0;at+window<=base.size();at+=stride){auto &bucket=index[rolling(base.subspan(at,window))];
        if(bucket.count<bucket.offsets.size())bucket.offsets[bucket.count++]=std::uint32_t(at);}
    Bytes result;put(result,0,8);std::uint64_t count=0,at=0,literal=0,last_base_end=0,last_target_end=0;
    auto command=[&](unsigned type,std::uint64_t a,std::uint64_t b=0){
        need(++count<=limits.commands);const auto extent=type?9+a:17;
        need(extent<=limits.patch_bytes-result.size());put(result,type,1);put(result,a,8);if(!type)put(result,b,8);
    };
    auto flush=[&]{if(literal<at){command(1,at-literal);append(result,next.subspan(literal,at-literal));}};
    std::uint64_t power=1;for(unsigned i=1;i<window;++i)power*=multiplier;
    auto value=next.size()>=window?rolling(next.first(window)):0;
    while(at+window<=next.size()){
        std::uint64_t best=0,offset=0;
        auto candidate=[&](std::uint64_t source){
            if(source>base.size()||window>base.size()-source)return;
            const auto n=std::min<std::uint64_t>(base.size()-source,next.size()-at);
            const auto [a,b]=std::mismatch(base.begin()+source,base.begin()+source+n,next.begin()+at);(void)b;
            const auto matched=std::uint64_t(a-(base.begin()+source));if(matched>best){best=matched;offset=source;}
        };
        // The expected continuation avoids scanning long identical zero runs
        // repeatedly. Other hash matches are checked byte-for-byte as well.
        const auto expected=last_base_end+at-last_target_end;candidate(expected);
        if(best<window)if(auto found=index.find(value);found!=index.end())
            for(unsigned i=0;i<found->second.count;++i)candidate(found->second.offsets[i]);
        if(best>=window){flush();command(0,offset,best);at+=best;literal=at;last_base_end=offset+best;last_target_end=at;
            if(at+window<=next.size())value=rolling(next.subspan(at,window));
        }else {if(at+window<next.size())value=(value-(std::to_integer<unsigned>(next[at])+1)*power)*multiplier+
                std::to_integer<unsigned>(next[at+window])+1;++at;}
    }
    at=next.size();flush();for(unsigned i=0;i<8;++i)result[i]=std::byte(count>>(8*i));return result;
}
}
struct AuthorityArchiveImage::Data {Bytes raw;AuthorityArchiveDigest digest;};
AuthorityArchiveImage::AuthorityArchiveImage(std::shared_ptr<const Data> data):data_(std::move(data)){}
AuthorityArchiveDigest AuthorityArchiveImage::digest()const{need(bool(data_));return data_->digest;}
std::uint64_t AuthorityArchiveImage::decoded_size()const{need(bool(data_));return data_->raw.size();}
std::span<const std::byte> AuthorityArchiveImage::raw()const{need(bool(data_));return data_->raw;}
AuthorityArchiveImage AuthorityArchiveImage::from_raw(Bytes raw,AuthorityDeltaLimits limits){
    limits_ok(limits);need(!raw.empty()&&raw.size()<=limits.decoded_bytes&&raw.capacity()<=limits.decoded_bytes);
    budget(limits,{raw.capacity(),raw.size(),sizeof(Data),128*1024});
    Bytes sealed(raw.begin(),raw.end());need(sealed.capacity()<=limits.decoded_bytes);
    budget(limits,{raw.capacity(),sealed.capacity(),sizeof(Data),128*1024});
    return from_unaliased_raw(std::move(sealed),limits);
}
AuthorityArchiveImage AuthorityArchiveImage::from_unaliased_raw(Bytes raw,AuthorityDeltaLimits limits){
    limits_ok(limits);need(!raw.empty()&&raw.size()<=limits.decoded_bytes&&raw.capacity()<=limits.decoded_bytes);
    budget(limits,{raw.capacity(),sizeof(Data),128*1024});
    auto data=std::make_shared<Data>();data->digest=hash(raw);data->raw=std::move(raw);
    return AuthorityArchiveImage(std::move(data));
}
AuthorityArchiveImage AuthorityArchiveImage::read(std::span<const std::byte> encoded,AuthorityDeltaLimits limits){
    limits_ok(limits);need(encoded.size()>=source_header&&encoded.size()<=limits.encoded_bytes);Reader r{encoded};
    need(std::ranges::equal(r.raw(8),source_magic)&&r.get(4)==1&&r.get(4)==0);
    const auto size=r.get(8),compressed=r.get(8);const auto expected=r.digest();
    need(size&&size<=limits.decoded_bytes&&compressed==encoded.size()-source_header);
    expansion(size,compressed);
    budget(limits,{encoded.size(),size,128*1024});
    auto data=std::make_shared<Data>();data->raw=expand(r.raw(compressed),size);data->digest=hash(data->raw);
    need(r.done()&&data->digest==expected);return AuthorityArchiveImage(std::move(data));
}
Bytes AuthorityArchiveImage::encode(AuthorityDeltaLimits limits)const{
    limits_ok(limits);need(bool(data_)&&data_->raw.size()<=limits.decoded_bytes);
    budget(limits,{data_->raw.size(),limits.encoded_bytes,limits.encoded_bytes,128*1024});
    auto compressed=compress(data_->raw,limits.encoded_bytes-source_header);
    Bytes result;append(result,source_magic);put(result,1,4);put(result,0,4);put(result,data_->raw.size(),8);
    put(result,compressed.size(),8);append(result,data_->digest);append(result,compressed);return result;
}
Bytes encode_authority_delta(const AuthorityArchiveImage &base,const AuthorityArchiveImage &next,AuthorityDeltaLimits limits){
    limits_ok(limits);need(base.decoded_size()<=limits.decoded_bytes&&next.decoded_size()<=limits.decoded_bytes);
    const auto raw=patch(base.data_->raw,next.data_->raw,limits);auto compressed=compress(raw,limits.encoded_bytes-delta_header);
    Bytes result;append(result,delta_magic);put(result,1,4);put(result,0,4);put(result,base.decoded_size(),8);put(result,next.decoded_size(),8);
    put(result,raw.size(),8);put(result,compressed.size(),8);append(result,base.digest());append(result,next.digest());append(result,hash(raw));
    append(result,compressed);return result;
}
AuthorityArchiveImage apply_authority_delta(const AuthorityArchiveImage &base,std::span<const std::byte> encoded,AuthorityDeltaLimits limits){
    limits_ok(limits);need(encoded.size()>=delta_header&&encoded.size()<=limits.encoded_bytes);Reader r{encoded};
    need(std::ranges::equal(r.raw(8),delta_magic)&&r.get(4)==1&&r.get(4)==0);
    const auto base_size=r.get(8),next_size=r.get(8),patch_size=r.get(8),compressed_size=r.get(8);
    const auto base_digest=r.digest(),next_digest=r.digest(),patch_digest=r.digest();
    need(base_size==base.decoded_size()&&base_size<=limits.decoded_bytes&&base_digest==base.digest()&&
        next_size&&next_size<=limits.decoded_bytes&&patch_size>=8&&patch_size<=limits.patch_bytes&&compressed_size==encoded.size()-delta_header);
    expansion(patch_size,compressed_size);
    budget(limits,{encoded.size(),base_size,patch_size,next_size,128*1024});
    const auto raw=expand(r.raw(compressed_size),patch_size);need(r.done()&&hash(raw)==patch_digest);Reader commands{raw};
    const auto count=commands.get(8);need(count&&count<=limits.commands&&count<=(raw.size()-8)/10);
    auto result=std::make_shared<AuthorityArchiveImage::Data>();result->raw.reserve(std::size_t(next_size));
    for(std::uint64_t i=0;i<count;++i){const auto type=commands.get(1),a=commands.get(8);need(type<=1);
        if(type){need(a&&a<=next_size-result->raw.size());append(result->raw,commands.raw(a));}
        else {const auto n=commands.get(8);need(n>=window&&a<=base_size&&n<=base_size-a&&n<=next_size-result->raw.size());
            append(result->raw,std::span(base.data_->raw).subspan(a,n));}
    }
    need(commands.done()&&result->raw.size()==next_size);result->digest=hash(result->raw);need(result->digest==next_digest);
    return AuthorityArchiveImage(std::move(result));
}
}
