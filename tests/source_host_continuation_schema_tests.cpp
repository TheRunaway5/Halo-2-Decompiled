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
    auto wire(unsigned presence,unsigned level,std::string_view magic="H2HOST03")const {
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
        return frame(magic,w.bytes,4096);
    }
    auto decode(std::span<const std::byte> bytes,const OnlineAdmission& expected)const {
        return LanHost::decode_continuation(bytes,binding,fingerprint,std::span(&expected,1),std::span(&native,1));
    }
};
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
    auto accepted=f.admitted;accepted.matchmaking_level=27;
    const auto bytes=f.wire(1,27);
    for(std::size_t prefix=0;prefix<bytes.size();++prefix)
        rejects([&]{f.decode(std::span(bytes).first(prefix),accepted);});
    std::cout<<"PASS native host schema: "<<roundtrips
             <<" optional/level round trips, foreign metadata, invalid range/flag, legacy shape and all truncated prefixes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
