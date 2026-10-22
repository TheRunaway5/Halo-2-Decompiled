#include "halo2/source_weather_world.hpp"
#include "halo2/source_geometry_resource.hpp"
#include "halo2/source_weather_initialization.hpp"
#include "halo2/source_weather_plate_simulation.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value;std::memcpy(&value,b.data()+p,4);return value;
}
std::uint16_t half(std::span<const std::byte> b,std::size_t p) {
    std::uint16_t value;std::memcpy(&value,b.data()+p,2);return value;
}
void store(std::span<std::byte> b,std::size_t p,std::uint32_t value) {
    std::memcpy(b.data()+p,&value,4);
}
}
SourceWeatherParticleResource::SourceWeatherParticleResource(const SourceWeatherParticles &source,
    std::uint32_t base):base_(base) {
    if(std::uint64_t(base)+0x1000+source.resource_storage.size()>UINT32_MAX)
        throw FormatError("native weather allocation exceeds its token address space");
    if(source.resources.size()>128)throw FormatError("native weather descriptor bank exceeds source bounds");
    raw_=base+0x1000;raw_size_=source.resource_storage.size();
    std::vector<std::byte> descriptors;
    for(const auto &resource:source.resources)
        descriptors.insert(descriptors.end(),resource.descriptor.begin(),resource.descriptor.end());
    auto bank=source.authored;
    if(word(bank,0x50)!=source.resources.size() || word(bank,0x44)!=raw_size_)
        throw FormatError("native weather resource backing differs from declaration");
    // The loader supplies cold disk banks. Live linked snapshots belong to a
    // different import path; accepting one here would lose its allocation.
    if(bank[0x5e]!=std::byte{} || word(bank,0x60)!=UINT32_MAX || word(bank,0x64)!=0)
        throw FormatError("native weather resource needs its cold disk descriptor");
    store(bank,0x54,descriptors.empty()?0:base+0x100);
    memory_.mount(base,bank,SourceRuntimeMemory::Access::ReadWrite,"weather particle definition");
    if(!descriptors.empty())memory_.mount(base+0x100,descriptors,
        SourceRuntimeMemory::Access::ReadOnly,"weather resource descriptors");
    if(raw_size_) {
        const Bytes raw(source.resource_storage);
        if(raw_size_<12 || raw.u32(0)!=0x626c6b68 || raw.u32(raw_size_-4)!=0x626c6b66)
            throw FormatError("native weather resource wrapper differs");
        memory_.mount(raw_,source.resource_storage,SourceRuntimeMemory::Access::ReadWrite,"weather geometry allocation");
    }
}
std::span<std::byte> SourceWeatherParticleResource::system(){return memory_.mutable_view(base_,0x8c);}
std::span<const std::byte> SourceWeatherParticleResource::system()const{return memory_.view(base_,0x8c);}
std::span<const std::byte> SourceWeatherParticleResource::raw_storage()const {
    return raw_size_?memory_.view(raw_,raw_size_):std::span<const std::byte>{};
}
bool SourceWeatherParticleResource::published()const{return system()[0x5e]!=std::byte{};}
void SourceWeatherParticleResource::publish() {
    auto bank=system();const auto section=std::bit_cast<std::int32_t>(word(bank,0x48));
    const auto bytes=std::bit_cast<std::int32_t>(word(bank,0x4c));
    if(section<=0 || bytes<0 || !raw_size_)return;
    const auto root=std::int32_t(0x40)+std::bit_cast<std::int16_t>(half(bank,0x5c));
    if(root!=0x64 || std::uint64_t(section)+8>raw_size_-4)
        throw FormatError("weather publication root/section is outside owned backing");
    std::vector<std::uint32_t> counts;bool render=false,state=false;
    const auto count=word(bank,0x50),pointer=word(bank,0x54);
    for(unsigned i=0;i<count;++i) {
        const auto descriptor=memory_.view(pointer+i*16,16);
        const auto offset=std::bit_cast<std::int32_t>(word(descriptor,12));
        if(offset==-1)continue;
        const auto locator=half(descriptor,4),stride=half(descriptor,6);
        const auto value=std::bit_cast<std::int32_t>(word(descriptor,8));
        const auto at=std::int64_t(section)+offset+8;
        if(descriptor[0]!=std::byte{} || (locator!=0 && locator!=8) ||
            stride!=(locator==0?20:32) || value<0 || value%stride || at<0 ||
            std::uint64_t(at)+std::uint32_t(value)>raw_size_-4 || unsigned(locator)+8>unsigned(section))
            throw FormatError("weather type0 fixup exceeds its owned arrays");
        if((locator==0 && render) || (locator==8 && state))
            throw FormatError("weather resource repeats a particle-array fixup");
        (locator==0?render:state)=true;
        counts.push_back(std::uint32_t(value/stride));
    }
    if(!render || !state || counts[0]!=counts[1])
        throw FormatError("weather resource publication needs paired particle arrays");
    // No mutation precedes validation. The complete shared223C70 owner reads
    // the actual descriptor/header/allocation banks and publishes in source order.
    source_geometry_resource_publish(base_+0x40,raw_,{
        [this](auto at,auto n){return memory_.view(at,n);},
        [this](auto at,auto n){return memory_.mutable_view(at,n);}});
    store(bank,0x60,0);bank[0x5e]=std::byte{1};
}
bool SourceWeatherParticleResource::request(std::int32_t cache_clock) {
    if(!published())publish();
    if(!published())return false;
    last_use_=cache_clock;return true;
}
void SourceWeatherParticleResource::rebase_age() {
    if(!last_use_)return;
    const auto age=0x7fffffffu-std::bit_cast<std::uint32_t>(*last_use_);
    last_use_=age>0x100?0:std::int32_t(0x100-age);
}
std::span<std::byte> SourceWeatherParticleResource::rows(unsigned locator,unsigned stride) {
    if(!published())throw FormatError("weather particle arrays are not published");
    const auto root=word(system(),0x68);
    const auto header=memory_.view(root+locator,8);
    const auto count=std::bit_cast<std::int32_t>(word(header,0));
    if(count<0)throw FormatError("weather particle resource has negative owned count");
    return memory_.mutable_view(word(header,4),std::size_t(count)*stride);
}
std::span<std::byte> SourceWeatherParticleResource::render_rows(){return rows(0,20);}
std::span<std::byte> SourceWeatherParticleResource::state_rows(){return rows(8,32);}
std::span<const std::byte> SourceWeatherParticleResource::rows(unsigned locator,unsigned stride)const {
    if(!published())throw FormatError("weather particle arrays are not published");
    const auto header=memory_.view(word(system(),0x68)+locator,8);
    const auto count=std::bit_cast<std::int32_t>(word(header,0));
    if(count<0)throw FormatError("weather particle resource has negative owned count");
    return memory_.view(word(header,4),std::size_t(count)*stride);
}
std::span<const std::byte> SourceWeatherParticleResource::render_rows()const{return rows(0,20);}
std::span<const std::byte> SourceWeatherParticleResource::state_rows()const{return rows(8,32);}
SourceWeatherSubmission SourceWeatherParticleResource::submission(float elapsed,std::int16_t views,std::int16_t slot)const {
    return source_weather_submission(system(),render_rows(),state_rows(),elapsed,views,slot);
}
SourceWeatherWorld::SourceWeatherWorld(const Cache &cache,std::size_t bsp):bsp_(cache.load_source_weather_bsp(bsp)) {
    for(const auto &palette:bsp_.palette) {
        if(palette.datum==UINT32_MAX || definitions_.contains(palette.datum))continue;
        auto &owner=definitions_[palette.datum];owner.definition=cache.load_source_weather_definition(palette.datum);
        owner.weather=owner.definition.authored;
        for(const auto &particles:owner.definition.particles) {
            auto &resource=owner.particles.emplace_back(particles);
            // Synchronous native load is complete before publication. This is
            // its real loaded bank, not an Xbox async completion prediction.
            resource.request(cache_clock_);
        }
    }
}
SourceWeatherWorldDefinition &SourceWeatherWorld::definition(std::uint32_t datum){return definitions_.at(datum);}
void SourceWeatherWorld::initialize(std::uint32_t &auxiliary) {
    //36AB0 traverses palettes, including repeated references to the same tag;
    //39880 initializes every plate before any particle in each reference.
    for(const auto &palette:bsp_.palette) {
        if(palette.datum==UINT32_MAX)continue;
        auto &owner=definition(palette.datum);
        for(auto &plate:owner.definition.plates)source_weather_initialize_plate(plate.authored,auxiliary);
        for(auto &resource:owner.particles) {
            const auto count=resource.published()?std::int32_t(resource.render_rows().size()/20):0;
            source_weather_initialize_particles(resource.system(),count,
                resource.published()?resource.render_rows():std::span<std::byte>{},
                resource.published()?resource.state_rows():std::span<std::byte>{},auxiliary);
        }
    }
    initialized_=true;
}
void SourceWeatherWorld::begin_frame() {
    if(cache_clock_==INT32_MAX) {
        for(auto &[datum,owner]:definitions_)for(auto &resource:owner.particles)resource.rebase_age();
        cache_clock_=0x101;
    } else ++cache_clock_;
    for(auto &view:admissions_)view.clear();
    view_valid_.fill(false);
}
std::span<const SourceWeatherAdmission> SourceWeatherWorld::admit_view(unsigned slot,const BspAtmosphere &atmosphere,
    int containing,const SourcePortalVisibilitySet &visibility,Vec3 eye,const SourceWeatherState &intensity) {
    if(slot>=admissions_.size())throw FormatError("weather view slot exceeds four local views");
    view_valid_[slot]=false;
    if(intensity.stopped!=0) {
        admissions_[slot].assign(bsp_.palette.size(),SourceWeatherAdmission{});
        return admissions_[slot];
    }
    std::vector<unsigned> clusters;
    for(const auto &entry:visibility.clusters) {
        if(entry.source_cluster<0)throw FormatError("weather clipped cluster entry is negative");
        clusters.push_back(unsigned(entry.source_cluster));
    }
    admissions_[slot]=source_weather_admission(bsp_,atmosphere,containing,clusters,eye);
    return admissions_[slot];
}
std::span<const SourceWeatherAdmission> SourceWeatherWorld::admission(unsigned slot)const{return admissions_.at(slot);}
std::array<std::byte,0xf4> source_weather_view_inputs(std::int16_t slot,Vec3 eye,
    Vec3 forward,float angle,const SourceViewProjectionBank &projection) {
    if(slot<0 || slot>3)throw FormatError("weather source view slot exceeds four local views");
    std::array<std::byte,0xf4> view{};
    //2BA10 copies camera+0..73 to packet+18 and2FD90's output to+8C.
    //132D0 publishes that packet unchanged at485600. Only the consumed
    // camera fields and first two complete transforms are needed here.
    std::memcpy(view.data(),&slot,2);
    std::memcpy(view.data()+0x18,&eye,sizeof(eye));
    std::memcpy(view.data()+0x24,&forward,sizeof(forward));
    std::memcpy(view.data()+0x40,&angle,4);
    std::memcpy(view.data()+0x8c,&projection.inverse_view,sizeof(Transform));
    std::memcpy(view.data()+0xc0,&projection.camera_to_world,sizeof(Transform));
    return view;
}
std::span<const SourceWeatherAdmission> SourceWeatherWorld::admit_view(std::span<const std::byte> view,
    const BspAtmosphere &atmosphere,int containing,const SourcePortalVisibilitySet &visibility,
    const SourceWeatherState &state) {
    if(view.size()!=0xf4)throw FormatError("weather view inputs require both complete source transforms");
    const auto slot=std::bit_cast<std::int16_t>(half(view,0));
    if(slot<0 || slot>3)throw FormatError("weather source view slot exceeds four local views");
    Vec3 eye;std::memcpy(&eye,view.data()+0x18,sizeof(eye));
    auto result=admit_view(unsigned(slot),atmosphere,containing,visibility,eye,state);
    std::copy(view.begin(),view.end(),views_[slot].begin());view_valid_[slot]=true;
    return result;
}
std::span<const std::byte> SourceWeatherWorld::view_inputs(unsigned slot)const {
    if(slot>=4 || !view_valid_[slot])throw FormatError("weather has no current owned view inputs");
    return views_[slot];
}
SourceWeatherWorldStep SourceWeatherWorld::visit_view(unsigned slot,const SourceWeatherState &state,
    const SourceWeatherWorldSimulation &input,std::uint32_t &auxiliary,
    const SourceWeatherWorldDrawServices &draw) {
    return visit_view_impl(slot,state,input,auxiliary,&draw);
}
SourceWeatherWorldStep SourceWeatherWorld::simulate_view(unsigned slot,const SourceWeatherState &state,
    const SourceWeatherWorldSimulation &input,std::uint32_t &auxiliary) {
    return visit_view_impl(slot,state,input,auxiliary,nullptr);
}
SourceWeatherWorldStep SourceWeatherWorld::visit_view_impl(unsigned slot,const SourceWeatherState &state,
    const SourceWeatherWorldSimulation &input,std::uint32_t &auxiliary,
    const SourceWeatherWorldDrawServices *draw) {
    const auto view=view_inputs(slot);
    if(admissions_[slot].size()!=bsp_.palette.size())throw FormatError("weather current view admission is missing");
    SourceWeatherWorldStep result;
    if(state.stopped!=0)return result;
    if(!initialized_)throw FormatError("weather arrays need their shared-stream constructors before simulation");
    if(slot>=unsigned(std::clamp<int>(input.view_count,1,4)) || input.x87_control!=0x023f ||
        (input.mxcsr&~0x3fu)!=0x1f80 || !std::isfinite(input.elapsed) || !std::isfinite(state.current))
        throw FormatError("weather caller view/time/arithmetic inputs are outside admitted controls");
    // Reject missing caller services before any wind/plate/particle mutation.
    for(std::size_t i=0;i<bsp_.palette.size();++i)if(admissions_[slot][i].visible) {
        const auto &owner=definitions_.at(bsp_.palette[i].datum);
        if(draw && !owner.definition.plates.empty() && !draw->plate)
            throw FormatError("weather plate draw service is absent");
        if(!owner.particles.empty() && ((draw && !draw->particle) || !input.rsqrt))
            throw FormatError("weather particle draw/ISA services are absent");
    }
    std::vector<SourceWeatherExclusionVolume> exclusions;
    for(const auto &volume:bsp_.polyhedra)exclusions.push_back({
        {volume.center.x,volume.center.y,volume.center.z},volume.radius,
        std::int32_t(volume.planes.size()),volume.planes});
    for(std::size_t palette=0;palette<bsp_.palette.size();++palette) {
        if(!admissions_[slot][palette].visible)continue;
        const auto datum=bsp_.palette[palette].datum;if(datum==UINT32_MAX)continue;
        auto &owner=definition(datum);++result.palettes;
        if(slot==0) {
            SourceWeatherWind wind;std::memcpy(&wind,owner.weather.data()+0x10,sizeof(wind));
            source_weather_wind_step(wind,input.elapsed,auxiliary);
            std::memcpy(owner.weather.data()+0x10,&wind,sizeof(wind));++result.wind_steps;
        }
        bool update=true;
        for(auto &plate:owner.definition.plates)if(update) {
            update=source_weather_step_plate(plate.authored,{view,owner.weather,input.elapsed,state.current},auxiliary);
            ++result.plate_steps;
        }
        for(auto &resource:owner.particles)if(update) {
            source_weather_advance_particle_elapsed(resource.system(),input.elapsed);
            if(resource.request(cache_clock_)) {
                SourceWeatherParticleSimulationInput particle{view,owner.weather,exclusions,
                    input.view_count,input.elapsed,state.current,admissions_[slot][palette].portal_distance,
                    input.x87_control,input.mxcsr,input.rsqrt};
                const auto step=source_weather_step_ready_particles(resource.system(),resource.render_rows(),
                    resource.state_rows(),particle,auxiliary);
                update=step.source_AL;result.particle_rows+=step.visited;result.rsqrt_calls+=step.rsqrt_calls;
            } //38450 returns AL1 for an actual not-ready request as well.
            ++result.particle_steps;
        }
        //36EB0 resets AL independently of the update chain's CL.
        if(!draw)continue;
        bool submit=true;
        for(std::size_t i=0;i<owner.definition.plates.size();++i)if(submit) {
            submit=draw->plate(datum,i,owner.definition.plates[i].authored,owner.weather,slot);
            ++result.plate_draws;
        }
        for(std::size_t i=0;i<owner.particles.size();++i)if(submit) {
            auto &resource=owner.particles[i];
            //391F0 performs its own request/age touch, independent of38450.
            if(resource.request(cache_clock_))
                submit=draw->particle(datum,i,resource,input.elapsed,input.view_count,std::int16_t(slot));
            ++result.particle_draws;
        }
    }
    return result;
}
}
