#include "halo2/native_request_protocol.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include <algorithm>
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
void put(Bytes &p,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(value>>(8*i)));}
Bytes header(unsigned kind){Bytes p;put(p,0x324e3248,4);put(p,protocol,2);put(p,kind,1);put(p,0x123456789abcdef0ull,8);return p;}
Bytes hello(unsigned count,bool profiles){
    auto p=header(1);put(p,0xfedcba9876543210ull,8);put(p,count,1);
    for(unsigned i=0;i<count;++i)put(p,i?i:255,1);
    for(unsigned i=0;i<count;++i){put(p,profiles,1);if(profiles)for(unsigned j=0;j<144;++j)put(p,(i*144+j)&255,1);}
    return p;
}
void motion_block(Bytes &p,unsigned seed){
    put(p,1|((seed&1)<<1)|((seed&2)<<1),1);put(p,seed*37&255,1);
    for(int coordinate:{-(1<<23),(1<<23)-1,int(seed*0x10101)-0x400000})put(p,std::uint32_t(coordinate)&0xffffff,3);
    for(int velocity:{-32768,32767,int(seed)-7})put(p,std::uint16_t(velocity),2);
}
Bytes input(unsigned count,bool intents,bool view=true,bool motion=false,unsigned repeats=0){
    auto p=header(2);put(p,0x8877665544332211ull,8);put(p,0xffffffff,4);put(p,count,1);
    for(unsigned i=0;i<count;++i){
        for(float f:{-0.f,1.f,-10000.f,10000.f})put(p,std::bit_cast<std::uint32_t>(f),4);
        put(p,2047,2);put(p,65535,2);put(p,0xfffffffe,4);
        for(unsigned j=0;j<4;++j)put(p,65535-j,2);
        for(unsigned j=0;j<10;++j)put(p,0xfffffff0u+j,4);
        put(p,intents,1);if(intents){put(p,4+i%3,2);put(p,63,2);put(p,0x80011234u,4);}
        put(p,intents,1);if(intents){put(p,0,2);put(p,0,2);put(p,0xffffffff,4);}
    }
    put(p,0x01020304u,4);put(p,0x80000001u,4);put(p,intents,1);
    if(view){put(p,0xfedc,2);put(p,254,1);}else{put(p,0,2);put(p,255,1);}
    for(unsigned i=0;i<count;++i)if(motion)motion_block(p,i);else put(p,0,1);
    put(p,repeats,1);
    for(unsigned r=0;r<repeats;++r)for(unsigned i=0;i<count;++i){
        // Older inputs: changed forward/yaw/flags (motion is never repeated).
        put(p,1|4|16,1);put(p,std::bit_cast<std::uint32_t>(.25f*float(r+1)),4);
        put(p,std::bit_cast<std::uint32_t>(float(r+i+2)),4);put(p,r+i+1,2);
    }
    return p;
}

// Test-only extraction of the removed host request reader and its one-to-four
// seat grammar. No endpoint/cookie admission or native object validation is
// asserted here; those are exercised by the actual host consumer separately.
struct Reader {
    std::span<const std::byte> p;std::size_t at{};
    std::uint64_t get(unsigned n){
        if(n>8||at>p.size()||n>p.size()-at)throw std::runtime_error("truncated reference");
        std::uint64_t v{};for(unsigned i=0;i<n;++i)v|=std::uint64_t(std::to_integer<unsigned>(p[at++]))<<(8*i);return v;
    }
    float real(){float f=std::bit_cast<float>(std::uint32_t(get(4)));if(!std::isfinite(f))throw std::runtime_error("nonfinite");return f;}
};
std::optional<NativeVehicleIntent> intent(Reader &r){
    const auto present=r.get(1);check(present<=1,"presence");if(!present)return {};
    NativeVehicleIntent v;v.action=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    v.index=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));v.object=r.get(4);
    check(v.action==0?(!v.index&&v.object==0xffffffffu):(v.action>=4&&v.action<=6&&v.index>=0&&v.index<64&&v.object!=0xffffffffu),"intent");return v;
}
std::optional<NativeMotionReport> motion(Reader &r){
    const auto flags=r.get(1);check(!(flags&~7u),"motion flags");
    if(!(flags&1)){check(!flags,"absent motion encoding");return {};}
    NativeMotionReport m;m.grounded=flags&2;m.crouching=flags&4;m.baseline_age=r.get(1);
    for(auto &v:m.position){const auto raw=std::uint32_t(r.get(3));v=std::int32_t(raw<<8)>>8;}
    for(auto &v:m.velocity)v=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));
    return m;
}
NativeRedundantControl repeated(Reader &r,const NativeRedundantControl &newer){
    const auto mask=r.get(1);check(!(mask&0xc0),"repeat mask");auto v=newer;
    const auto real=[&](float limit,float &out,float before){
        out=r.real();check(std::abs(out)<=limit,"repeat range");
        check(std::bit_cast<std::uint32_t>(out)!=std::bit_cast<std::uint32_t>(before),"unchanged repeat");};
    if(mask&1)real(1,v.forward,newer.forward);if(mask&2)real(1,v.right,newer.right);
    if(mask&4)real(10000,v.yaw,newer.yaw);if(mask&8)real(10000,v.pitch,newer.pitch);
    if(mask&16){v.flags=r.get(2);check(v.flags<=2047&&v.flags!=newer.flags,"repeat flags");}
    if(mask&32){v.zoom=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));check(v.zoom!=newer.zoom,"repeat zoom");}
    return v;
}
NativeInputControl control(Reader &r){
    NativeInputControl c;c.forward=r.real();c.right=r.real();c.yaw=r.real();c.pitch=r.real();c.flags=r.get(2);
    check(std::abs(c.forward)<=1&&std::abs(c.right)<=1&&std::abs(c.yaw)<=10000&&std::abs(c.pitch)<=10000&&c.flags<=2047,"control");
    c.zoom=std::bit_cast<std::int16_t>(std::uint16_t(r.get(2)));c.zoom_reset_serial=r.get(4);
    c.pickup_milliseconds=r.get(2);c.dual_wield_milliseconds=r.get(2);
    for(auto &v:c.completed)v=r.get(2);for(auto &v:c.presses)v=r.get(4);
    c.vehicle=intent(r);c.completed_vehicle=intent(r);return c;
}
std::optional<NativeRequest> reference(const Bytes &p,std::uint16_t version){try{
    Reader r{p};check(p.size()<=1400&&r.get(4)==0x324e3248u&&r.get(2)==version,"header");
    const auto kind=r.get(1);const auto nonce=r.get(8);std::optional<NativeRequest> result;
    if(kind==1){
        NativeHelloRequest v;v.nonce=nonce;v.fingerprint=r.get(8);const auto count=r.get(1);check(count>=1&&count<=4,"seats");
        v.players.resize(count);for(auto &player:v.players)player.team=std::bit_cast<std::int8_t>(std::uint8_t(r.get(1)));
        for(auto &player:v.players){const auto present=r.get(1);check(present<=1,"profile presence");
            if(present){player.profile.emplace();for(auto &byte:*player.profile)byte=r.get(1);}}
        result=std::move(v);
    }else if(kind==2){
        NativeInputRequest v;v.nonce=nonce;v.cookie=r.get(8);v.sequence=r.get(4);const auto count=r.get(1);check(count>=1&&count<=4,"seats");
        for(unsigned i=0;i<count;++i)v.controls.push_back(control(r));
        v.ack.latest=r.get(4);v.ack.mask=r.get(4);v.ack.flags=r.get(1);check(v.ack.flags<=1,"ack flags");
        const auto tick=std::uint16_t(r.get(2));const auto fraction=std::uint8_t(r.get(1));
        if(fraction==255)check(tick==0,"absent view encoding");else v.view=NativeSnapshotView{tick,fraction};
        for(auto &c:v.controls)c.motion=motion(r);
        const auto repeats=r.get(1);check(repeats<=2&&v.sequence>repeats,"repeat count");
        std::vector<NativeRedundantControl> newer;
        for(const auto &c:v.controls)newer.push_back({c.forward,c.right,c.yaw,c.pitch,c.flags,c.zoom});
        for(unsigned n=0;n<repeats;++n){
            NativeRedundantInput entry;entry.sequence=v.sequence-1-n;
            for(const auto &c:newer)entry.controls.push_back(repeated(r,c));
            newer=entry.controls;v.redundant.push_back(std::move(entry));
        }
        result=std::move(v);
    }else if(kind==4)result=NativeLeaveRequest{nonce,r.get(8)};
    else if(kind==6)result=NativeSessionQuery{nonce};
    check(r.at==p.size(),"trailing");return result;
}catch(const std::runtime_error &){return {};}}
bool same(const NativeInputControl &a,const NativeInputControl &b){
    return std::bit_cast<std::uint32_t>(a.forward)==std::bit_cast<std::uint32_t>(b.forward)&&
        std::bit_cast<std::uint32_t>(a.right)==std::bit_cast<std::uint32_t>(b.right)&&
        std::bit_cast<std::uint32_t>(a.yaw)==std::bit_cast<std::uint32_t>(b.yaw)&&
        std::bit_cast<std::uint32_t>(a.pitch)==std::bit_cast<std::uint32_t>(b.pitch)&&
        a.flags==b.flags&&a.zoom==b.zoom&&a.zoom_reset_serial==b.zoom_reset_serial&&
        a.pickup_milliseconds==b.pickup_milliseconds&&a.dual_wield_milliseconds==b.dual_wield_milliseconds&&
        a.completed==b.completed&&a.presses==b.presses&&a.vehicle==b.vehicle&&a.completed_vehicle==b.completed_vehicle&&
        a.motion==b.motion;
}
bool same(const NativeRedundantControl &a,const NativeRedundantControl &b){
    return std::bit_cast<std::uint32_t>(a.forward)==std::bit_cast<std::uint32_t>(b.forward)&&
        std::bit_cast<std::uint32_t>(a.right)==std::bit_cast<std::uint32_t>(b.right)&&
        std::bit_cast<std::uint32_t>(a.yaw)==std::bit_cast<std::uint32_t>(b.yaw)&&
        std::bit_cast<std::uint32_t>(a.pitch)==std::bit_cast<std::uint32_t>(b.pitch)&&
        a.flags==b.flags&&a.zoom==b.zoom;
}
bool same(const std::optional<NativeRequest> &a,const std::optional<NativeRequest> &b){
    if(bool(a)!=bool(b))return false;if(!a)return true;if(a->index()!=b->index())return false;
    if(const auto *x=std::get_if<NativeHelloRequest>(&*a)){
        const auto &y=std::get<NativeHelloRequest>(*b);
        if(x->nonce!=y.nonce||x->fingerprint!=y.fingerprint||x->players.size()!=y.players.size())return false;
        for(unsigned i=0;i<x->players.size();++i)if(x->players[i].team!=y.players[i].team||x->players[i].profile!=y.players[i].profile)return false;
        return true;
    }
    if(const auto *x=std::get_if<NativeInputRequest>(&*a)){
        const auto &y=std::get<NativeInputRequest>(*b);
        return x->nonce==y.nonce&&x->cookie==y.cookie&&x->sequence==y.sequence&&x->controls.size()==y.controls.size()&&
            x->ack.latest==y.ack.latest&&x->ack.mask==y.ack.mask&&x->ack.flags==y.ack.flags&&x->view==y.view&&
            std::equal(x->controls.begin(),x->controls.end(),y.controls.begin(),[](const auto &c,const auto &d){return same(c,d);})&&
            x->redundant.size()==y.redundant.size()&&
            std::equal(x->redundant.begin(),x->redundant.end(),y.redundant.begin(),[](const auto &e,const auto &f){
                return e.sequence==f.sequence&&e.controls.size()==f.controls.size()&&
                    std::equal(e.controls.begin(),e.controls.end(),f.controls.begin(),[](const auto &c,const auto &d){return same(c,d);});});
    }
    if(const auto *x=std::get_if<NativeLeaveRequest>(&*a)){const auto &y=std::get<NativeLeaveRequest>(*b);return x->nonce==y.nonce&&x->cookie==y.cookie;}
    return std::get<NativeSessionQuery>(*a).nonce==std::get<NativeSessionQuery>(*b).nonce;
}
}
int main(){try{
    auto team=header(19);put(team,0x8877665544332211ull,8);put(team,7,4);put(team,3,1);put(team,6,1);
    const auto request=decode_native_request(team,protocol);
    check(request && std::holds_alternative<NativeTeamChangeRequest>(*request),"Team change did not cross the typed Rust bridge");
    const auto accepted=std::get<NativeTeamChangeRequest>(*request);
    check(accepted.nonce==0x123456789abcdef0ull && accepted.cookie==0x8877665544332211ull && accepted.sequence==7 &&
        accepted.view==3 && accepted.team==6,"Team bridge lost request identity or local ownership operands");
    for(unsigned n=0;n<team.size();++n)check(!decode_native_request(Bytes(team.begin(),team.begin()+n),protocol),"Truncated team request was accepted");
    auto invalid=team;invalid.back()=std::byte{8};check(!decode_native_request(invalid,protocol),"Invalid team crossed the typed boundary");
    invalid=team;invalid[27]=std::byte{4};check(!decode_native_request(invalid,protocol),"Arbitrary player slot crossed the local-view boundary");

    std::size_t comparisons{};
    const auto compare=[&](const Bytes &p,std::uint16_t version=protocol){
        check(same(decode_native_request(p,version),reference(p,version)),"Native Rust/reference request differs");++comparisons;};
    auto leave=header(4);put(leave,0x8877665544332211ull,8);
    const std::array<Bytes,12> seeds{hello(1,false),hello(4,true),hello(4,false),input(1,false),input(1,true),input(4,true),
        input(2,false,false),leave,header(6),input(1,false,true,true,2),input(2,true,false,true,1),input(4,false,true,false,2)};
    for(const auto &seed:seeds){
        compare(seed);compare(seed,std::uint16_t(protocol+1));
        for(unsigned n=0;n<seed.size();++n)compare(Bytes(seed.begin(),seed.begin()+n));
        auto trailing=seed;trailing.push_back(std::byte{});compare(trailing);
        for(unsigned at=0;at<seed.size();++at)for(unsigned byte=0;byte<256;++byte){auto changed=seed;changed[at]=std::byte(byte);compare(changed);}
    }
    for(unsigned offset:{28u,32u,36u,40u})for(auto bits:{0x7fc00000u,0x7f800000u,0xff800000u,0x80000000u,0x3f800001u}){
        auto p=input(1,false);for(unsigned n=0;n<4;++n)p[offset+n]=std::byte(bits>>(8*n));compare(p);
    }
    compare(Bytes(1400,std::byte{}));compare(Bytes(1401,std::byte{}));compare({});
    {   // Displayed snapshot view: three strict bytes, one absent encoding.
        const auto viewed=decode_native_request(input(1,false),protocol);
        check(viewed&&std::get<NativeInputRequest>(*viewed).view==NativeSnapshotView{0xfedc,254},"Snapshot view lost");
        const auto absent=decode_native_request(input(1,false,false),protocol);
        check(absent&&!std::get<NativeInputRequest>(*absent).view,"Absent snapshot view decoded as present");
        auto ambiguous=input(1,false,false);ambiguous[ambiguous.size()-5]=std::byte{1};
        check(!decode_native_request(ambiguous,protocol),"Absent snapshot view admitted a second encoding");
        compare(ambiguous);
    }
    {   // Motion blocks and repeated inputs: exact values, newest-first sequences.
        const auto decoded=decode_native_request(input(1,false,true,true,2),protocol);
        check(decoded.has_value(),"Predicted input rejected");
        const auto &request=std::get<NativeInputRequest>(*decoded);
        check(request.controls[0].motion&&request.controls[0].motion->position[0]==-(1<<23)&&
            request.controls[0].motion->position[1]==(1<<23)-1&&request.controls[0].motion->velocity[0]==-32768&&
            request.controls[0].motion->grounded==false,"Motion block values lost");
        check(request.redundant.size()==2&&request.redundant[0].sequence==0xfffffffeu&&
            request.redundant[1].sequence==0xfffffffdu&&request.redundant[0].controls[0].forward==.25f&&
            request.redundant[1].controls[0].flags==2,"Repeated inputs lost their sequence or values");
        auto repeated_motion=input(1,false,true,true,1);repeated_motion[repeated_motion.size()-11]|=std::byte{64};
        check(!decode_native_request(repeated_motion,protocol),"A repeat carried a motion bit");compare(repeated_motion);
        auto absent=input(1,false,true,false,0);absent[absent.size()-2]=std::byte{2};
        check(!decode_native_request(absent,protocol),"Absent motion admitted a second encoding");compare(absent);
        auto three=input(1,false,true,false,2);three[three.size()-1-2*(1+4+4+2)]=std::byte{3};
        check(!decode_native_request(three,protocol),"Three repeated inputs admitted");compare(three);
    }
    auto p=hello(4,true);const auto owned=decode_native_request(p,protocol);check(owned.has_value(),"profile seed rejected");
    const auto expected=reference(p,protocol);p.assign(1401,std::byte{});p.clear();p.shrink_to_fit();
    check(same(owned,expected),"Native profile result borrowed retired packet");
    const auto source=input(4,true);const auto expected_input=reference(source,protocol);
    std::atomic<bool> good{true};std::vector<std::jthread> workers;
    for(unsigned n=0;n<4;++n)workers.emplace_back([&]{for(unsigned i=0;i<300;++i)if(!same(decode_native_request(source,protocol),expected_input))good=false;});
    workers.clear();check(good,"Concurrent native parser changed data");
    std::cout<<"PASS "<<comparisons<<" native request differential frames, owned profile retirement and 1200 concurrent calls; no sender authority claimed.\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
