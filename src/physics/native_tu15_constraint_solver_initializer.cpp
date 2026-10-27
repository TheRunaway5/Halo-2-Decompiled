#include "halo2/native_tu15_constraint_solver_initializer.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint8_t byte(std::uint32_t p)const{std::array<std::byte,1>b{};memory_.read(p,b);return std::to_integer<std::uint8_t>(b[0]);}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;}
    void ready(std::uint32_t instruction){const std::array bytes{std::byte{1}};SourcePhysicsContactRecordStore store{instruction,0x48d480u,{},{}};if(observe_){store.before.resize(1);memory_.read(store.address,store.before);store.written.assign(bytes.begin(),bytes.end());}memory_.write(store.address,bytes);if(observe_)observe_(store);}
};
std::int32_t signed_byte(std::uint8_t value){return std::bit_cast<std::int8_t>(value);}
bool signed_less(std::uint32_t first,std::uint32_t second){return std::bit_cast<std::int32_t>(first)<std::bit_cast<std::int32_t>(second);}
}
std::uint32_t native_tu15_constraint_solver_client_hash(SourcePhysicsContactMemory&memory,std::uint32_t key){
    const SourcePhysicsContactRecordObserver no_observer;Memory m(memory,no_observer);
    std::array<std::uint8_t,64>client{},salt{};
    for(unsigned n=0;n<client.size();++n){const auto value=m.byte(key+n);if(!value)break;client[n]=value;}
    for(unsigned n=0;n<salt.size();++n){const auto value=m.byte(0x41f4f8u+n);if(!value)break;salt[n]=value;}
    // Authenticated private coefficient initialization repeats this16-word
    // sequence four times. All products/additions have source uint32 wrap.
    constexpr std::array<std::uint32_t,16>coefficients{2,3,1,2,3,1,4,3,2,1,3,1,2,2,3,3};
    std::uint32_t result{},mix=0xc18f77a2u;
    for(unsigned n=0;n<client.size();++n){
        const auto difference=std::uint32_t(signed_byte(salt[n])^signed_byte(client[n]));
        const auto product=difference*0x01010101u;const auto coefficient=coefficients[n&15u];
        result+=(n&1u)?product/coefficient:product*coefficient;
        for(unsigned previous=0;previous<n;++previous){
            const auto sum=std::uint32_t(signed_byte(salt[previous])+signed_byte(client[previous]));
            mix=(mix+mix)^(sum*0x01010101u);result+=mix;
        }
    }
    return result;
}
void native_tu15_initialize_constraint_solver(SourcePhysicsContactMemory&memory,NativeTu15PhysicsFrameFp&fp,const NativeTu15ConstraintSolverInitializerServices&services,const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintSolverInitializerStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;
    auto fail=[&](std::uint32_t message){if(!services.log_failure)throw std::runtime_error("Native TU1.5 solver initializer requires actual failure logger2E3850");if(stats)++stats->log_calls;services.log_failure(message,fp);};
    if(m.word(0x450108u)){
        const auto key=m.word(0x47c1b0u);if(stats)++stats->client_hashes;
        const auto actual=native_tu15_constraint_solver_client_hash(memory,key);
        if(m.word(0x450108u)==actual){m.ready(0x3268f1u);if(stats)++stats->client_ready;}
        else fail(0x41f588u);
        return;
    }
    if(!services.read_time_seconds)throw std::runtime_error("Native TU1.5 solver initializer requires actual time owner37C5C0");
    if(stats)++stats->time_calls;const auto time=services.read_time_seconds(fp);
    auto first_dash=m.word(0x47c1b0u);auto character=m.byte(first_dash);
    while(character!=0x2du){if(!character){fail(0x41f510u);return;}character=m.byte(first_dash+1u);++first_dash;}
    const auto platform=first_dash+1u;auto second_dash=platform;character=m.byte(second_dash);
    while(character!=0x2du){if(!character){fail(0x41f510u);return;}character=m.byte(second_dash+1u);++second_dash;}
    // The original accepts an empty segment or a matching prefix. It does
    // not require whole-string equality with the public platform label.
    for(auto cursor=platform;cursor!=second_dash;++cursor){
        const auto actual=m.byte(cursor);if(!actual){fail(0x41f510u);return;}
        const auto expected=m.byte(0x41f4f4u+cursor-platform);
        if(!expected||actual!=expected){fail(0x41f510u);return;}
    }
    const auto fourth=std::uint32_t(signed_byte(m.byte(second_dash+4u)));
    const auto third=std::uint32_t(signed_byte(m.byte(second_dash+3u)));
    auto encoded=((fourth-0x41u)<<12u)|((third-0x41u)<<8u);
    encoded|=(std::uint32_t(signed_byte(m.byte(second_dash+7u)))-0x41u)<<24u;
    encoded|=(std::uint32_t(signed_byte(m.byte(second_dash+2u)))-0x41u)<<4u;
    const auto sixth=std::uint32_t(signed_byte(m.byte(second_dash+6u)))-0x41u;
    const auto mask=m.word(0x41f4f0u);encoded|=sixth<<20u;
    encoded|=(std::uint32_t(signed_byte(m.byte(second_dash+5u)))-0x41u)<<16u;
    const auto eighth=std::uint32_t(signed_byte(m.byte(second_dash+8u)));
    const auto first=std::uint32_t(signed_byte(m.byte(second_dash+1u)));
    encoded|=(eighth<<28u)-0x10000000u;encoded|=first-0x41u;encoded^=mask;
    // Source IDIV truncates toward zero. Keep its signed remainder and the
    // subsequent wrapped subtraction/comparisons, including unusual inputs.
    const auto remainder=std::bit_cast<std::int32_t>(encoded)%86400;
    const auto date=encoded-std::bit_cast<std::uint32_t>(remainder);
    if(!signed_less(time,date)||!signed_less(date-time,0x00ed4e00u)){fail(0x41f510u);return;}
    m.ready(0x326a04u);if(stats)++stats->evaluation_ready;
}
} // namespace halo2
