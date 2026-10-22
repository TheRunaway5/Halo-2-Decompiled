#include "halo2/online_voice_protocol.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
constexpr auto protocol=std::uint16_t(native_multiplayer_protocol);
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void put(Bytes &bytes,std::uint32_t value,unsigned n){
    for(unsigned i=0;i<n;++i)bytes.push_back(std::byte(value>>(8*i)));
}
Bytes header(unsigned subtype){
    Bytes bytes;put(bytes,0x324e3248u,4);put(bytes,protocol,2);put(bytes,6,1);put(bytes,subtype,1);return bytes;
}
Bytes audio(){
    auto p=header(0);put(p,15,1);put(p,0x12345678u,4);put(p,1,1);
    put(p,std::bit_cast<std::uint32_t>(.5f),4);put(p,std::bit_cast<std::uint32_t>(1.f),4);
    put(p,0x19,1);put(p,0xa5,1);return p;
}
Bytes preferences(bool forwarded){
    auto p=header(forwarded?2:1);put(p,1,1);put(p,0x8001,2);if(forwarded)put(p,15,1);return p;
}

// Independent test-only extraction of the former C++ framing rules, with
// the maximum sixteen-seat roster. No production target links this decoder.
std::optional<ParsedOnlineVoice> reference(std::span<const std::byte> p,std::uint16_t version){
    if(p.size()<8 || p.size()>278)return {};
    const auto get=[&](unsigned at,unsigned n){std::uint32_t value{};
        if(at>p.size() || n>p.size()-at)throw std::runtime_error("truncated reference frame");
        for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(p[at+i])<<(8*i);return value;};
    if(get(0,4)!=0x324e3248u || get(4,2)!=version || get(6,1)!=6)return {};
    const auto subtype=get(7,1);
    if(subtype==1 || subtype==2){
        if(p.size()!=(subtype==1?11u:12u) || get(8,1)>1 || (subtype==2 && get(11,1)>=16))return {};
        ParsedOnlineVoicePreferences v;if(subtype==2)v.forwarded_source=get(11,1);
        v.enabled=get(8,1);v.muted=get(9,2);return v;
    }
    if(subtype!=0 || p.size()<=22 || get(8,1)>=16 || get(13,1)>1)return {};
    ParsedOnlineVoiceAudio v;v.source=get(8,1);v.sequence=get(9,4);v.radio=get(13,1);
    v.left=std::bit_cast<float>(get(14,4));v.right=std::bit_cast<float>(get(18,4));
    if(!std::isfinite(v.left) || !std::isfinite(v.right) || v.left<0 || v.left>1 || v.right<0 || v.right>1)return {};
    for(auto b:p.subspan(22))v.opus.push_back(std::to_integer<std::uint8_t>(b));return v;
}
bool same(const std::optional<ParsedOnlineVoice>&a,const std::optional<ParsedOnlineVoice>&b){
    if(bool(a)!=bool(b))return false;if(!a)return true;if(a->index()!=b->index())return false;
    if(const auto *x=std::get_if<ParsedOnlineVoicePreferences>(&*a)){
        const auto &y=std::get<ParsedOnlineVoicePreferences>(*b);
        return x->forwarded_source==y.forwarded_source && x->enabled==y.enabled && x->muted==y.muted;
    }
    const auto &x=std::get<ParsedOnlineVoiceAudio>(*a),&y=std::get<ParsedOnlineVoiceAudio>(*b);
    return x.source==y.source && x.sequence==y.sequence && x.radio==y.radio &&
        std::bit_cast<std::uint32_t>(x.left)==std::bit_cast<std::uint32_t>(y.left) &&
        std::bit_cast<std::uint32_t>(x.right)==std::bit_cast<std::uint32_t>(y.right) && x.opus==y.opus;
}
}
int main(){try{
    std::size_t comparisons{};
    const auto compare=[&](const Bytes &p,std::uint16_t version=protocol){
        check(same(decode_online_voice_frame(p,version),reference(p,version)),"Rust/C++ framing differs");++comparisons;};
    const std::array<Bytes,3> seeds{audio(),preferences(false),preferences(true)};
    for(const auto &seed:seeds){
        compare(seed);
        for(unsigned size=0;size<seed.size();++size)compare(Bytes(seed.begin(),seed.begin()+size));
        for(unsigned at=0;at<seed.size();++at)for(unsigned byte=0;byte<256;++byte){
            auto changed=seed;changed[at]=std::byte(byte);compare(changed);
        }
        compare(seed,std::uint16_t(protocol+1));
    }
    auto p=audio();p.resize(22);compare(p);
    for(unsigned n=1;n<=257;++n){p.resize(22+n,std::byte{0xa5});compare(p);}
    for(unsigned offset:{14u,18u})for(auto bits:{0x80000000u,0x7fc00000u,0x7f800000u,0xff800000u,0xbf800000u,0x3f800001u}){
        p=audio();for(unsigned n=0;n<4;++n)p[offset+n]=std::byte(bits>>(8*n));compare(p);
    }
    p=audio();auto owned=decode_online_voice_frame(p,protocol);check(bool(owned),"actual CXX decode failed");
    p.assign(400,std::byte{});p.shrink_to_fit();
    const auto &retained=std::get<ParsedOnlineVoiceAudio>(*owned);
    check(retained.opus==std::vector<std::uint8_t>{0x19,0xa5} && retained.source==15 &&
        retained.sequence==0x12345678u && retained.left==.5f,"bridge borrowed retired packet storage");
    std::atomic<unsigned> failures{};
    {std::array<std::jthread,4> threads;
     for(auto &thread:threads)thread=std::jthread([&]{for(unsigned i=0;i<300;++i)
        if(!same(decode_online_voice_frame(seeds[i%3],protocol),reference(seeds[i%3],protocol)))++failures;});}
    check(failures==0,"concurrent independent CXX calls differ");
    std::cout<<"PASS "<<comparisons<<" Rust/C++ differential frames, owned packet retirement, 1200 concurrent calls; syntax only, not sender authority.\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
