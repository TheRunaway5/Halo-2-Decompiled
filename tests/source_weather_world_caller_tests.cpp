#include "halo2/source_weather_world.hpp"
#include "halo2/source_weather_rsqrt.hpp"
#include "halo2/source_weather_execution.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <xmmintrin.h>
#endif

using namespace halo2;
namespace {
void require(bool ok,const std::string &message){if(!ok)throw FormatError(message);}
struct Reader {
    std::vector<char> bytes;
    std::size_t cursor{};
    explicit Reader(const std::filesystem::path &path) {
        std::ifstream stream(path,std::ios::binary);
        require(bool(stream),"weather caller fixture unavailable");
        bytes.assign(std::istreambuf_iterator<char>(stream),{});
    }
    std::span<const std::byte> take(std::size_t count) {
        require(cursor<=bytes.size() && count<=bytes.size()-cursor,"weather caller fixture truncated");
        const auto result=std::as_bytes(std::span(bytes)).subspan(cursor,count);cursor+=count;return result;
    }
    template<class T> T get(){T result;const auto b=take(sizeof(T));std::memcpy(&result,b.data(),sizeof(T));return result;}
};
void copy(std::span<std::byte> out,std::span<const std::byte> in) {
    require(out.size()==in.size(),"weather owned bank has wrong extent");std::copy(in.begin(),in.end(),out.begin());
}
void same(std::span<const std::byte> actual,std::span<const std::byte> expected,const std::string &name) {
    require(actual.size()==expected.size(),name+" extent differs");
    for(std::size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])
        throw FormatError(name+" differs at byte "+std::to_string(i));
}
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
    std::uint32_t result;std::memcpy(&result,b.data()+at,4);return result;
}
void store(std::span<std::byte> b,std::size_t at,std::uint32_t value){std::memcpy(b.data()+at,&value,4);}
struct DrawBoundary{};
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
struct Environment {
    std::uint16_t old_control{};
    unsigned old_csr{};
    Environment(std::uint16_t control,unsigned csr):old_csr(_mm_getcsr()) {
        asm volatile("fnstcw %0":"=m"(old_control));
        asm volatile("fldcw %0"::"m"(control));_mm_setcsr(csr);
    }
    ~Environment(){asm volatile("fldcw %0"::"m"(old_control));_mm_setcsr(old_csr);}
};
#endif
}
int main(int argc,char **argv) {
#if !defined(__GNUC__) || (!defined(__i386__) && !defined(__x86_64__))
    (void)argc;(void)argv;return 77;
#else
try {
    require(argc==2,"workspace root required");const std::filesystem::path root=argv[1];
    const auto map=root/"assets/disc/maps/lockout.map";if(!std::filesystem::exists(map))return 77;
    Reader fixture(root/"tests/fixtures/retail_source_weather_world_caller.bin");
    const auto magic=fixture.take(8);require(std::memcmp(magic.data(),"H2WCF001",8)==0,"weather caller fixture schema differs");
    const auto cases=fixture.get<std::uint32_t>();require(cases==7,"weather caller fixture lost source cases");
    Cache cache(map);const auto atmosphere=cache.load_bsp_atmosphere(0);
    std::size_t source_words=0,isa_pairs=0,profile_pairs=0,draw_boundaries=0;
    for(unsigned c=0;c<cases;++c) {
        const auto index=fixture.get<std::uint32_t>(),datum=fixture.get<std::uint32_t>();
        const auto count=fixture.get<std::uint32_t>();require(index==c && count==1024,"weather caller fixture identity differs");
        const auto views=std::int16_t(fixture.get<std::uint32_t>());
        const auto cluster=std::int16_t(fixture.get<std::uint32_t>());
        const bool has_record=fixture.get<std::uint32_t>()!=0,expected_draw=fixture.get<std::uint32_t>()!=0;
        const auto fcw=std::uint16_t(fixture.get<std::uint32_t>());
        const auto mxcsr=fixture.get<std::uint32_t>();
        auto auxiliary=fixture.get<std::uint32_t>();const auto source_auxiliary=auxiliary;
        auto intensity=fixture.get<SourceWeatherState>();
        auto clock=fixture.get<SourceWeatherClock>();auto record=fixture.get<SourceWeatherRecord>();
        const auto realtime=fixture.get<double>();
        SourcePortalVisibilitySet visibility;const auto clusters=fixture.get<std::uint32_t>();
        require(clusters<1024,"weather caller fixture cluster count exceeds source bank");
        for(unsigned i=0;i<clusters;++i) {
            const auto id=fixture.get<std::uint32_t>();visibility.clusters.emplace_back().source_cluster=int(id);
        }
        const auto view=fixture.take(244),weather=fixture.take(176),plate=fixture.take(936),system=fixture.take(140);
        const auto render=fixture.take(std::size_t(count)*20),state=fixture.take(std::size_t(count)*32);
        const auto n_isa=fixture.get<std::uint32_t>();require(n_isa<=count,"weather caller ISA bank exceeds particle count");
        std::vector<std::array<std::uint32_t,3>> isa;
        for(unsigned i=0;i<n_isa;++i)isa.push_back(fixture.get<std::array<std::uint32_t,3>>());
        const auto expected_auxiliary=fixture.get<std::uint32_t>();
        const auto expected_intensity=fixture.take(20),expected_clock=fixture.take(8),expected_record=fixture.take(24);
        const auto expected_weather=fixture.take(176),expected_plate=fixture.take(936),source_system=fixture.take(140);
        const auto expected_render=fixture.take(std::size_t(count)*20),expected_state=fixture.take(std::size_t(count)*32);
        Environment arithmetic(fcw,mxcsr);
        SourceWeatherWorld world(cache,0);auto construction_control=auxiliary;world.initialize(construction_control);
        // Test-only import of the actual captured running banks. The cold
        // constructor control is discarded; no launch history is inferred.
        auto &owner=world.definition(datum);require(owner.particles.size()==1 && owner.definition.plates.size()==1,"actual caller ownership differs");
        auto &resource=owner.particles.front();std::array<std::uint32_t,3> tokens;
        const std::array<std::size_t,3> token_offsets{0x54,0x60,0x68};
        for(unsigned i=0;i<3;++i)tokens[i]=word(resource.system(),token_offsets[i]);
        copy(resource.system(),system);
        for(unsigned i=0;i<3;++i)store(resource.system(),token_offsets[i],tokens[i]);
        copy(resource.render_rows(),render);copy(resource.state_rows(),state);
        copy(owner.weather,weather);copy(owner.definition.plates.front().authored,plate);
        std::int16_t slot;std::memcpy(&slot,view.data(),2);
        source_weather_view_tick(intensity,clock,realtime,slot,has_record?&record:nullptr);
        world.begin_frame();world.admit_view(view,atmosphere,cluster,visibility,intensity);
        same(world.view_inputs(unsigned(slot)),view,"owned actual view");
        // Independently check that the native camera adapter copies every
        // weather-consumed field from the same original submitting bank.
        Vec3 eye,forward;float angle;SourceViewProjectionBank projection;
        std::memcpy(&eye,view.data()+0x18,12);std::memcpy(&forward,view.data()+0x24,12);
        std::memcpy(&angle,view.data()+0x40,4);
        std::memcpy(&projection.inverse_view,view.data()+0x8c,sizeof(Transform));
        std::memcpy(&projection.camera_to_world,view.data()+0xc0,sizeof(Transform));
        const auto produced=source_weather_view_inputs(slot,eye,forward,angle,projection);
        for(const auto [at,n]:std::array<std::pair<std::size_t,std::size_t>,5>{{{0,2},{0x18,24},{0x40,4},{0x8c,52},{0xc0,52}}})
            same(std::span(produced).subspan(at,n),view.subspan(at,n),"native weather view producer");
        std::size_t next_isa=0,draw_calls=0;
        SourceWeatherWorldSimulation simulation{views,clock.elapsed,fcw,mxcsr,
            [&](std::size_t particle,std::uint32_t speed) {
                require(next_isa<isa.size(),"unexpected source ISA request");const auto &answer=isa[next_isa++];
                require(answer[0]==particle && answer[1]==speed,"source ISA operand/order differs");return answer[2];
            }};
        SourceWeatherWorldDrawServices draw;
        draw.plate=[&](auto,std::size_t,auto,auto,unsigned)->bool{++draw_calls;throw DrawBoundary{};};
        draw.particle=[](auto,std::size_t,auto &,float,std::int16_t,std::int16_t)->bool{
            throw FormatError("particle draw overtook the actual first plate draw boundary");};
        if(c==0) {
            bool rejected=false;
            try{world.visit_view(unsigned(slot),intensity,simulation,auxiliary,{});}
            catch(const FormatError &){rejected=true;}
            require(rejected && auxiliary==source_auxiliary,"missing draw service mutated auxiliary state");
            same(owner.weather,weather,"rejected caller weather");same(resource.render_rows(),render,"rejected caller rows");
        }
        bool reached_draw=false;
        try{world.visit_view(unsigned(slot),intensity,simulation,auxiliary,draw);}
        catch(const DrawBoundary &){reached_draw=true;}
        require(reached_draw==expected_draw && draw_calls==unsigned(expected_draw),"source first-draw boundary differs");
        require(next_isa==isa.size() && auxiliary==expected_auxiliary,"weather shared auxiliary chronology differs");
        const auto prefix="case "+std::to_string(c)+" ";
        same(std::as_bytes(std::span(&intensity,1)),expected_intensity,prefix+"intensity");
        same(std::as_bytes(std::span(&clock,1)),expected_clock,prefix+"clock");
        same(std::as_bytes(std::span(&record,1)),expected_record,prefix+"record");
        same(owner.weather,expected_weather,prefix+"owning weather/wind");
        same(owner.definition.plates.front().authored,expected_plate,prefix+"plate");
        std::array<std::byte,140> normalized_system;copy(normalized_system,source_system);
        for(unsigned i=0;i<3;++i)store(normalized_system,token_offsets[i],tokens[i]);
        same(resource.system(),normalized_system,prefix+"system");
        same(resource.render_rows(),expected_render,prefix+"render rows");
        same(resource.state_rows(),expected_state,prefix+"state rows");
        require(resource.published() && word(resource.system(),0x68)==resource.raw_token()+8,"caller lost native row ownership");
        if(c==0) {
            // Explicit false draw-service control: the independent submission
            // chain stops after the first plate; all update outputs still
            // equal the unchanged source producer prefix. This is no GPU AL
            // claim and supplies no service result to normal runtime code.
            copy(resource.system(),system);
            for(unsigned i=0;i<3;++i)store(resource.system(),token_offsets[i],tokens[i]);
            copy(resource.render_rows(),render);copy(resource.state_rows(),state);
            copy(owner.weather,weather);copy(owner.definition.plates.front().authored,plate);
            auxiliary=source_auxiliary;next_isa=0;world.begin_frame();
            world.admit_view(view,atmosphere,cluster,visibility,intensity);
            draw.plate=[](auto,std::size_t,auto,auto,unsigned){return false;};
            const auto step=world.visit_view(unsigned(slot),intensity,simulation,auxiliary,draw);
            require(step.wind_steps==1 && step.plate_steps==1 && step.particle_steps==1 &&
                step.plate_draws==1 && step.particle_draws==0 && next_isa==isa.size() && auxiliary==expected_auxiliary,
                "independent weather submission AL chain differs");
            same(owner.weather,expected_weather,"controlled draw-chain weather");
            same(owner.definition.plates.front().authored,expected_plate,"controlled draw-chain plate");
            same(resource.render_rows(),expected_render,"controlled draw-chain render rows");
            same(resource.state_rows(),expected_state,"controlled draw-chain state rows");
        }
        isa_pairs+=next_isa;draw_boundaries+=draw_calls;
        // Independent calculated reference-emulator provider over exactly the
        // same actual caller inputs. The production provider reads no fixture.
        copy(resource.system(),system);
        for(unsigned i=0;i<3;++i)store(resource.system(),token_offsets[i],tokens[i]);
        copy(resource.render_rows(),render);copy(resource.state_rows(),state);
        copy(owner.weather,weather);copy(owner.definition.plates.front().authored,plate);
        auxiliary=source_auxiliary;next_isa=0;world.begin_frame();
        world.admit_view(view,atmosphere,cluster,visibility,intensity);
        simulation.rsqrt=[&](std::size_t particle,std::uint32_t speed) {
            const auto csr=_mm_getcsr();
            const auto answer=source_weather_rsqrt_xemu_0_8_136(speed);
            require(_mm_getcsr()==csr,"reference profile changed SSE control/exception flags");
            require(next_isa<isa.size() && isa[next_isa][0]==particle && isa[next_isa][1]==speed &&
                isa[next_isa][2]==answer,"calculated reference profile differs from unchanged original instruction");
            ++next_isa;return answer;
        };
        draw.plate=[](auto,std::size_t,auto,auto,unsigned)->bool{throw DrawBoundary{};};
        reached_draw=false;
        try{world.visit_view(unsigned(slot),intensity,simulation,auxiliary,draw);}
        catch(const DrawBoundary&){reached_draw=true;}
        require(reached_draw==expected_draw && next_isa==isa.size() && auxiliary==expected_auxiliary,
            "calculated reference profile caller chronology differs");
        same(std::as_bytes(std::span(&intensity,1)),expected_intensity,prefix+"reference-profile intensity");
        same(std::as_bytes(std::span(&clock,1)),expected_clock,prefix+"reference-profile clock");
        same(std::as_bytes(std::span(&record,1)),expected_record,prefix+"reference-profile record");
        same(owner.weather,expected_weather,prefix+"reference-profile weather");
        same(owner.definition.plates.front().authored,expected_plate,prefix+"reference-profile plate");
        same(resource.system(),normalized_system,prefix+"reference-profile system");
        same(resource.render_rows(),expected_render,prefix+"reference-profile render rows");
        same(resource.state_rows(),expected_state,prefix+"reference-profile state rows");
        profile_pairs+=next_isa;
        // The normal loop uses the producer-only seam until its graphics
        // consumer is bound. It must reach the same independently retained
        // original boundary without pretending to have submitted a draw.
        copy(resource.system(),system);
        for(unsigned i=0;i<3;++i)store(resource.system(),token_offsets[i],tokens[i]);
        copy(resource.render_rows(),render);copy(resource.state_rows(),state);
        copy(owner.weather,weather);copy(owner.definition.plates.front().authored,plate);
        auxiliary=source_auxiliary;next_isa=0;world.begin_frame();
        world.admit_view(view,atmosphere,cluster,visibility,intensity);
        const auto produced_step=world.simulate_view(unsigned(slot),intensity,simulation,auxiliary);
        require(produced_step.plate_draws==0 && produced_step.particle_draws==0 &&
            next_isa==isa.size() && auxiliary==expected_auxiliary,"producer-only caller manufactured a draw or changed RNG");
        same(owner.weather,expected_weather,prefix+"live-producer weather");
        same(owner.definition.plates.front().authored,expected_plate,prefix+"live-producer plate");
        same(resource.system(),normalized_system,prefix+"live-producer system");
        same(resource.render_rows(),expected_render,prefix+"live-producer render rows");
        same(resource.state_rows(),expected_state,prefix+"live-producer state rows");
        source_words+=(20+8+24+176+936+140+count*52)/4+1;
        world.begin_frame();bool rejected=false;
        try{world.view_inputs(unsigned(slot));}catch(const FormatError&){rejected=true;}
        require(rejected,"new frame exposed a stale camera bank");
    }
    for(const auto invalid:{0u,0x007fffffu,0x80000000u,0xbf800000u,0x7f800000u,0x7fc00000u}) {
        const auto csr=_mm_getcsr();bool rejected=false;
        try{source_weather_rsqrt_xemu_0_8_136(invalid);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected && _mm_getcsr()==csr,"reference profile invalid operand changed floating controls");
    }
    {
        Environment unsupported(0x023f,0x1fc0);bool rejected=false;
        try{source_weather_rsqrt_xemu_0_8_136(0x40000000u);}catch(const std::invalid_argument&){rejected=true;}
        require(rejected && _mm_getcsr()==0x1fc0,"reference profile silently admitted DAZ");
        {
            SourceWeatherExecution native_scope;
            std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
            // Native x87 reads reserved bit6 as1 (027F); the saved emulator
            // reports023F. The precision/rounding/mask fields are identical.
            require((control&~0x40u)==0x023f && _mm_getcsr()==0x1f80,
                "native execution did not enter the declared environment");
            require(source_weather_rsqrt_xemu_0_8_136(0x40000000u)>0,"native execution failed its actual profile");
        }
        require(_mm_getcsr()==0x1fc0,"native execution failed to restore caller SSE state");
    }
    require(fixture.cursor==fixture.bytes.size(),"weather caller fixture has unconsumed data");
    std::cout<<cases<<" original full weather producer prefixes, "<<source_words
        <<" exact output words, "<<isa_pairs<<" explicit fixture ISA pairs, "<<profile_pairs
        <<" independently calculated xemu-profile pairs, "<<draw_boundaries<<" actual first-draw boundaries passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
#endif
}
