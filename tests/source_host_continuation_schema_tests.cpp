#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_binding.hpp"
#include "runtime/continuation_wire.hpp"
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void rejects(F operation) {
    bool rejected=false;try{operation();}catch(const FormatError&){rejected=true;}
    check(rejected,"Malformed or foreign host admission was accepted");
}
void empty_input_continuation(continuation_wire::Writer& w) {
    w.boolean(false); // Consumed interpolation view.
    for(unsigned i=0;i<4*10;++i)w.u(0,4); // Latest accepted press serials.
    w.u(0,1);w.boolean(false);w.u(0,4); // Pending inputs and consumed receipt.
    for(unsigned i=0;i<4;++i)w.boolean(false); // Consumed motion reports.
    for(unsigned i=0;i<4;++i)w.u(0,4);
    for(unsigned i=0;i<3;++i)w.u(0,8);
}
void declared_control(continuation_wire::Writer& w) {
    for(unsigned i=0;i<4;++i)w.real(0);
    for(unsigned i=0;i<5;++i)w.boolean(false);
    w.u(65535,2);w.u(0,4);
    for(unsigned i=0;i<6;++i)w.boolean(false);
    w.u(0,2);w.u(0,2);
    for(unsigned i=0;i<6;++i)w.boolean(false);
    w.real(-0.f);w.boolean(false);
}
// Explicit declared wire values exercise the new grammar without granting a
// native World, accepted input or motion writer. Actual continuation replay
// separately checks their production effects and complete packet bytes.
void declared_input_continuation(continuation_wire::Writer& w,unsigned variant) {
    if(!variant){empty_input_continuation(w);return;}
    const auto motion=[&](bool present,bool bad_position=false){w.boolean(present);if(present){
        w.boolean(true);w.boolean(false);w.u(255,1);
        w.i32(bad_position?8388608:-8388608);w.i32(8388607);w.i32(0);
        w.u(32768,2);w.u(32767,2);w.u(0,2);
    }};
    w.u(variant==13?2u:1u,1);w.u(23,2);w.u(variant==5?255u:254u,1);
    for(unsigned view=0;view<4;++view)for(unsigned action=0;action<10;++action)w.u(view==0?3u:0u,4);
    w.u(variant==2?4u:2u,1);
    for(unsigned n=0;n<2;++n){
        const unsigned sequence=variant==3?7u-n:variant==8?7u+n:variant==9?5u+n:6u+n;
        w.u(sequence,4);for(unsigned view=0;view<4;++view)declared_control(w);
        for(unsigned view=0;view<4;++view)motion(view==0,variant==6);
        w.boolean(true);w.u(22,2);w.u(127,1);
        for(unsigned view=0;view<4;++view)for(unsigned action=0;action<10;++action)
            w.u(view==0?(variant==10?4u:3u):0u,4);
        for(unsigned value=0;value<4*2;++value)w.u(0,2);
        for(unsigned view=0;view<4;++view)w.boolean(false);
    }
    w.u(variant==12?2u:variant==4?0u:1u,1);w.u(5,4);
    for(unsigned view=0;view<4;++view)motion(view==0 || (variant==7 && view==1));
    for(unsigned view=0;view<4;++view)w.u(view==0?9u:variant==11 && view==1?1u:0u,4);
    w.u(17,8);w.u(23,8);w.u(31,8);
}
// Declared codec values only. They grant no native world, admission, transport
// authentication or selected rank. Production codecs read and retain them.
struct Fixture {
    LanHost::ContinuationBinding binding;
    OnlineAdmission admitted;
    SourcePlayerAdmission native;
    static constexpr std::uint64_t fingerprint=0x151337u;
    Fixture() {
        binding.context.session[0]=std::byte{0x71};binding.context.epoch=3;
        binding.context.membership_revision=7;binding.context.peers=1;binding.context.host=0;
        binding.world={1,23,1,{}};binding.world.digest[0]=std::byte{0x15};
        admitted.slot=0;admitted.name="Declared codec seat";admitted.team=-1;
        admitted.peer=0;admitted.identity.fill(std::byte{0x27});
        native.slot=0;native.player=0x10000u;native.identity.fill(0x38);
    }
    auto wire(unsigned presence,unsigned level,std::string_view magic="H2HOST05")const {
        using namespace continuation_wire;
        Writer w{4096,{}};w.u(native_multiplayer_protocol,2);
        const auto& c=binding.context;const auto& b=binding.world;
        w.octets(c.session);w.u(c.epoch,4);w.u(c.membership_revision,8);w.u(c.peers,2);w.u(c.host,1);
        w.u(b.id,8);w.u(b.tick,8);w.u(b.bytes,8);w.octets(b.digest);
        w.octets(source_authority_binding(c));w.u(fingerprint,8);w.u(b.tick,8);
        w.blob(std::array<std::byte,10>{},65536);for(unsigned i=0;i<4;++i)w.u(null_datum,4);
        w.u(1,1);w.u(admitted.slot,1);w.string(admitted.name);w.i32(admitted.team);
        w.octets(admitted.identity);w.u(admitted.peer,4);w.boolean(admitted.guest);
        if(magic!="H2HOST02"){w.u(presence,1);if(presence)w.u(level,1);}
        w.u(1,1);w.u(native.slot,1);w.u(native.player,4);w.octets(native.identity);w.u(0,1);
        w.boolean(false); // No native motion-adoption owner in this declared codec fixture.
        return frame(magic,w.bytes,4096);
    }
    auto decode(std::span<const std::byte> bytes,const OnlineAdmission& expected)const {
        return LanHost::decode_continuation(bytes,binding,fingerprint,std::span(&expected,1),std::span(&native,1));
    }
};
void peer_eye_roundtrip() {
    using namespace continuation_wire;
    Fixture f;f.binding.context.peers=3;
    const auto digest=source_authority_binding(f.binding.context);
    auto remote=f.admitted;remote.slot=1;remote.peer=1;remote.name="Declared remote eye";
    auto native=f.native;native.slot=1;native.player=0x10001u;
    const std::array admitted{f.admitted,remote};const std::array roster{f.native,native};
    // A declared camera checkpoint grants no native execution. The real
    // checkpoint decoder still binds its four views to this exact peer roster.
    Writer camera{8192,{}};camera.octets(digest);camera.u(f.binding.world.tick,4);
    for(unsigned view=0;view<4;++view) {
        camera.u(view?null_datum:native.player,4);camera.boolean(false);camera.boolean(false);
        camera.octets(view?SourcePlayerIdentity{}:native.identity);
        for(unsigned word=0;word<214+80;++word)camera.u(0,4);
    }
    camera.i32(-1);for(unsigned view=0;view<4;++view)camera.i32(-1);camera.u(7,4);
    const auto checkpoint=frame("H2CAM001",camera.bytes,8192);
    const auto eye=[&](Writer &w,unsigned mode,float fov) {
        w.boolean(true);
        for(float v:{1.f,2.f,3.f,0.f,.125f,-.992156744f,0.f,.992156744f,.125f,fov,-0.f})w.real(v);
        w.u(mode,1);
    };
    const auto wire=[&](unsigned mode=0,unsigned eye_view=0,bool overlap=false,bool checkpoint_present=true,float fov=1.2f,unsigned input_variant=0) {
        Writer w{16384,{}};w.u(native_multiplayer_protocol,2);
        const auto &c=f.binding.context;const auto &b=f.binding.world;
        w.octets(c.session);w.u(c.epoch,4);w.u(c.membership_revision,8);w.u(c.peers,2);w.u(c.host,1);
        w.u(b.id,8);w.u(b.tick,8);w.u(b.bytes,8);w.octets(b.digest);
        w.octets(digest);w.u(Fixture::fingerprint,8);w.u(b.tick,8);
        w.blob(std::array<std::byte,10>{},65536);for(unsigned view=0;view<4;++view)w.u(null_datum,4);
        w.u(admitted.size(),1);
        for(const auto &p:admitted) {
            w.u(p.slot,1);w.string(p.name);w.i32(p.team);w.octets(p.identity);w.u(p.peer,4);
            w.boolean(p.guest);w.boolean(false);
        }
        w.u(roster.size(),1);for(const auto &p:roster){w.u(p.slot,1);w.u(p.player,4);w.octets(p.identity);}
        w.u(1,1);w.u(1,1);w.u(123,8);w.u(456,8);w.u(1,1);w.u(1,1);w.u(255,1);w.boolean(false);
        w.u(7,4);w.u(9,4);w.boolean(false);w.boolean(true);w.u(0,8);
        for(unsigned view=0;view<4;++view) {
            for(unsigned value=0;value<4;++value)w.real(0);
            for(unsigned value=0;value<5;++value)w.boolean(false);
            w.u(65535,2);w.u(0,4);
            for(unsigned value=0;value<6;++value)w.boolean(false);
            w.u(0,2);w.u(0,2);
            for(unsigned value=0;value<6;++value)w.boolean(false);
            w.real(0);w.boolean(false);
        }
        for(unsigned value=0;value<4*10*2;++value)w.u(0,4);
        for(unsigned value=0;value<4*2;++value)w.u(0,2);
        for(unsigned view=0;view<4;++view)w.boolean(false);
        w.boolean(checkpoint_present);if(checkpoint_present)w.blob(checkpoint,8192);
        w.boolean(checkpoint_present);if(checkpoint_present)w.u(b.tick,4);
        for(unsigned view=0;view<4;++view)if(overlap && view==eye_view)eye(w,1,1.2f);else w.boolean(false);
        for(unsigned view=0;view<4;++view)w.boolean(false);
        for(unsigned view=0;view<4;++view)if(view==eye_view)eye(w,mode,fov);else w.boolean(false);
        for(unsigned view=0;view<4;++view)w.u(0,4);
        declared_input_continuation(w,input_variant);w.boolean(false);
        return frame("H2HOST05",w.bytes,16384);
    };
    const auto decode=[&](const auto &bytes){return LanHost::decode_continuation(bytes,f.binding,Fixture::fingerprint,admitted,roster);};
    const auto bytes=wire();const auto value=decode(bytes);
    check(LanHost::encode_continuation(value,f.binding)==bytes,
        "Host sidecar lost its actual ordinary-eye bank, negative zero or canonical camera bytes");
    rejects([&]{decode(wire(1));});rejects([&]{decode(wire(0,1));});
    rejects([&]{decode(wire(0,0,true));});rejects([&]{decode(wire(0,0,false,false));});
    rejects([&]{decode(wire(0,0,false,true,0));});
    const auto queued=wire(0,0,false,true,1.2f,1);
    check(LanHost::encode_continuation(decode(queued),f.binding)==queued,
        "Host sidecar lost pending motion/serials, consumed receipt, interpolation view or timing counters");
    for(unsigned invalid=2;invalid<=13;++invalid)
        rejects([&]{decode(wire(0,0,false,true,1.2f,invalid));});
}
}
int main()try {
    Fixture f;unsigned roundtrips{};
    for(unsigned level=0;level<=50;++level) {
        auto accepted=f.admitted;if(level)accepted.matchmaking_level=level;
        const auto bytes=f.wire(level!=0,level);
        const auto decoded=f.decode(bytes,accepted);
        check(LanHost::encode_continuation(decoded,f.binding)==bytes,
            "Host continuation lost optional authenticated level or canonical bytes");
        auto foreign=accepted;
        if(level)foreign.matchmaking_level.reset();else foreign.matchmaking_level=1;
        rejects([&]{f.decode(bytes,foreign);});
        foreign=accepted;foreign.matchmaking_level=level==50?49:50;
        rejects([&]{f.decode(bytes,foreign);});
        ++roundtrips;
    }
    for(unsigned level:{0u,51u,255u}) {
        auto invalid=f.admitted;invalid.matchmaking_level=level;
        rejects([&]{f.decode(f.wire(1,level),invalid);});
    }
    rejects([&]{f.decode(f.wire(2,27),f.admitted);});
    rejects([&]{f.decode(f.wire(0,0,"H2HOST02"),f.admitted);});
    rejects([&]{f.decode(f.wire(0,0,"H2HOST03"),f.admitted);});
    rejects([&]{f.decode(f.wire(0,0,"H2HOST04"),f.admitted);});
    auto accepted=f.admitted;accepted.matchmaking_level=27;
    const auto bytes=f.wire(1,27);
    for(std::size_t prefix=0;prefix<bytes.size();++prefix)
        rejects([&]{f.decode(std::span(bytes).first(prefix),accepted);});
    peer_eye_roundtrip();
    std::cout<<"PASS native host schema: "<<roundtrips
             <<" optional/level round trips, foreign metadata, invalid range/flag, legacy shape and all truncated prefixes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
