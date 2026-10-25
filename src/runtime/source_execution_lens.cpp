#include "halo2/source_execution_lens.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_lens_query_gpu.hpp"
#include "halo2/source_lens_draw_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <openssl/evp.h>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>

namespace halo2 {
namespace {
bool within(std::uint32_t p,std::size_t n,std::uint32_t base,std::size_t size) {
    return p>=base&&std::uint64_t(p)+n<=std::uint64_t(base)+size;
}
bool device_address(std::uint32_t p,std::size_t n) {
    return within(p,n,0x411568u,4)||within(p,n,0x411570u,4)||
        within(p,n,0x491e38u,0x74)||within(p,n,0x491eacu,0xc0)||
        within(p,n,0xf7100000u,4096)||within(p,n,0xf7110000u,0x20000)||
        within(p,n,0xf7140000u,((source_lens_query_capacity+255)/256)*4096);
}
bool device_write_address(std::uint32_t p,std::size_t n) {
    // Camera/projection and the driver pointer are read overlays only. The
    // original adapter forwards writes at those addresses to the source bank.
    return within(p,n,0x411570u,4)||within(p,n,0xf7100000u,4096)||
        within(p,n,0xf7110000u,0x20000)||
        within(p,n,0xf7140000u,((source_lens_query_capacity+255)/256)*4096);
}
struct QueryMemory final:SourcePhysicsContactMemory {
    SourceRuntimeMemory &source;
    SourcePhysicsContactMemory *device;
    bool draw{};
    std::span<const SourceLensDeviceRegion> regions;
    const SourceLensDeviceDomains *domains{};
    mutable bool recording;
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> digest{nullptr,EVP_MD_CTX_free};
    mutable std::map<std::uint32_t,std::byte> written;
    QueryMemory(SourceRuntimeMemory &s,SourcePhysicsContactMemory *d,bool drawing=false,
        std::span<const SourceLensDeviceRegion> r={},const SourceLensDeviceDomains *owned=nullptr)
        :source(s),device(d),draw(drawing),regions(r),domains(owned),recording(source_execution_inputs(s)!=nullptr) {
        if(recording)digest.reset(EVP_MD_CTX_new());
        if(recording&&(!digest||EVP_DigestInit_ex(digest.get(),EVP_sha256(),nullptr)!=1))disable_recording();
    }
    bool external(std::uint32_t p,std::size_t n,bool writing)const {
        if(!draw)return writing?device_write_address(p,n):device_address(p,n);
        if(!writing&&(within(p,n,0x491e38u,0x74)||within(p,n,0x4c65c0u,0x74)||
            within(p,n,0x491eacu,0xc0)||within(p,n,0x4c6634u,0xc0)))return true;
        if(domains){auto i=domains->upper_bound(p);if(i==domains->begin())return false;--i;return within(p,n,i->first,i->second);}
        auto i=std::upper_bound(regions.begin(),regions.end(),p,[](auto address,const auto &region){return address<region.address;});
        if(i==regions.begin())return false;--i;return within(p,n,i->address,i->bytes);
    }
    void disable_recording()const {
        auto *inputs=source_execution_inputs(source);
        if(!inputs)throw FormatError("Original lens device recorder lost its input owner");
        inputs->invalidate_recording();recording=false;written.clear();
    }
    void read(std::uint32_t p,std::span<std::byte> bytes)const override {
        if(!external(p,bytes.size(),false)){source.read(p,bytes);return;}
        if(!recording){if(!device)throw FormatError("Unrecorded original lens device read");device->read(p,bytes);return;}
        if(std::uint64_t(p)+bytes.size()>0x100000000ull)throw FormatError("Lens device read wraps its address");
        std::size_t at{};
        while(at<bytes.size()) {
            const auto left=bytes.size()-at;
            const unsigned count=left>=8?8:left>=4?4:left>=2?2:1;
            using K=SourceExecutionInputKind;
            const auto kind=count==8?K::GraphicsRead8:count==4?K::GraphicsRead4:count==2?K::GraphicsRead2:K::GraphicsRead1;
            const auto address=p+std::uint32_t(at);
            const auto value=source_execution_input(source,kind,address,[&] {
                if(!device)throw FormatError("Unrecorded original lens device read");
                std::array<std::byte,8> raw{};device->read(address,std::span(raw).first(count));
                std::uint64_t result{};for(unsigned n=0;n<count;++n)result|=std::uint64_t(std::to_integer<unsigned>(raw[n]))<<(n*8);
                return result;
            });
            for(unsigned n=0;n<count;++n) {
                const auto b=std::byte(value>>(n*8));
                // A read of a value written by this original kernel must
                // match that actual write; external inputs cannot mask it.
                if(const auto old=written.find(address+n);old!=written.end()&&old->second!=b)disable_recording();
                bytes[at+n]=b;
            }
            at+=count;
        }
    }
    void write(std::uint32_t p,std::span<const std::byte> bytes)override {
        if(!external(p,bytes.size(),true)){source.write(p,bytes);return;}
        if(!recording){if(!device)throw FormatError("Unrecorded original lens device write");device->write(p,bytes);return;}
        if(std::uint64_t(p)+bytes.size()>0x100000000ull)throw FormatError("Lens device write wraps its address");
        if(!draw&&within(p,bytes.size(),0x411570u,4))for(auto b:bytes)if(b!=std::byte{})
            throw FormatError("Lens query immutable native pipeline acquired unsupported SDK state");
        std::array<std::byte,8> raw;
        for(unsigned n=0;n<4;++n){raw[n]=std::byte(p>>(8*n));raw[n+4]=std::byte(bytes.size()>>(8*n));}
        if(EVP_DigestUpdate(digest.get(),raw.data(),raw.size())!=1||EVP_DigestUpdate(digest.get(),bytes.data(),bytes.size())!=1)
            disable_recording();
        if(recording)try{for(std::size_t n=0;n<bytes.size();++n)written[p+std::uint32_t(n)]=bytes[n];}
        catch(...){disable_recording();}
        if(device)device->write(p,bytes);
    }
    std::array<std::byte,32> finish() {
        if(!recording)return {};
        std::array<std::byte,32> result;unsigned size{};
        if(EVP_DigestFinal_ex(digest.get(),reinterpret_cast<unsigned char*>(result.data()),&size)!=1||size!=result.size())
            throw FormatError("Cannot finish original lens device validation");
        return result;
    }
};
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {return Bytes(memory.view(address,4)).u32(0);}
void put(SourceRuntimeMemory &memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes;for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));memory.write(address,bytes);
}
unsigned retirement_count(const SourceRuntimeMemory &memory) {
    const auto count=word(memory,0x531fbcu);
    if(count>511)throw FormatError("Original lens retirement count exceeds its request bank");
    if(count)memory.view(0x52dfdcu,count*32u);
    (void)memory.view(0x531fc0u,192);
    return count;
}
}
void source_require_retired_lens_queries(const SourceRuntimeMemory &memory) {
    const auto count=retirement_count(memory);
    for(unsigned ring=0;ring<3;++ring)for(unsigned index=0;index<16;++index){
        const auto first=index*32u;
        const auto allocated=count>=first+32u?0xffffffffu:count>first?(1u<<(count-first))-1u:0u;
        if(word(memory,0x531fc0u+ring*64u+index*4u)&allocated)
            throw FormatError("Incoming original lens ring still owns GPU queries");
    }
    std::array<std::byte,32> free{};free[4]=std::byte{0x80};free[5]=std::byte{0xff};
    for(unsigned n=0;n<count;++n)if(!std::ranges::equal(memory.view(0x52dfdcu+n*32u,32),free))
        throw FormatError("Incoming original lens request has not been retired");
}
unsigned source_retire_lens_queries(PhysicsLiveWorldTu15 &world) {
    world.require_execution_boundary();
    const auto count=retirement_count(world.memory());
    for(unsigned n=0;n<count;++n){
        auto abi=world.prepare_source_call();abi.gp[0]=n;world.run_actual(0x20e30u,abi);
    }
    source_require_retired_lens_queries(world.memory());return count;
}
void source_execute_lens_query(PhysicsLiveWorldTu15 &world,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactMemory *device) {
    source_execute_lens_query(world.memory(),entry,abi,device);
}
void source_execute_lens_query(SourceRuntimeMemory &source,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactMemory *device) {
    if(!source_lens_query_tu15_owns_entry(entry))throw FormatError("Unowned original lens query entry");
    SourceExecutionCallScope journal(&source,SourceExecutionCallKind::LensQuery,[&]{return source_execution_arguments(entry,abi);});
    QueryMemory memory(source,device);SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observe) {
        if(!source_lens_query_tu15_owns_entry(call.target))throw FormatError("Original lens query reached unowned graphics service "+std::to_string(call.target));
        source_run_lens_query_tu15(m,call.target,state,{0x27f,0x1f80},services,observe);
    };
    source_run_lens_query_tu15(memory,entry,abi,{0x27f,0x1f80},services);
    journal.complete([&]{return source_execution_lens_result(abi,memory.finish());});
}
void source_finish_lens_query(PhysicsLiveWorldTu15 &world,bool submitted,std::span<const SourceLensQueryRollback> rollback) {
    source_finish_lens_query(world.memory(),submitted,rollback);
}
void source_finish_lens_query(SourceRuntimeMemory &source,bool submitted,std::span<const SourceLensQueryRollback> rollback) {
    if(rollback.size()>source_lens_query_capacity)throw FormatError("Lens query rollback exceeds its ring");
    for(const auto &saved:rollback)if(saved.id>=source_lens_query_capacity||saved.area>65535)
        throw FormatError("Invalid original lens query rollback");
    SourceExecutionCallScope journal(&source,SourceExecutionCallKind::LensQueryFinish,[&]{return source_execution_arguments(submitted,rollback);});
    if(submitted) {
        // Whole13EF0:14172/1417C and ADC high, after a genuine submission.
        const std::uint64_t stamp=std::uint64_t(word(source,0x4922c0u))|(std::uint64_t(word(source,0x4922c4u))<<32);
        put(source,0x4922c0u,std::uint32_t(stamp+1));put(source,0x4922c4u,std::uint32_t((stamp+1)>>32));
    } else for(const auto &saved:rollback) {
        const auto slot=saved.id/3,ring=saved.id%3,bit=1u<<(slot%32),p=0x531fc0u+ring*64+(slot/32)*4;
        put(source,p,(word(source,p)&~bit)|(saved.bit?bit:0));
        const std::array<std::byte,2> area{std::byte(saved.area),std::byte(saved.area>>8)};source.write(0x52dfdcu+slot*32+6,area);
    }
}
bool source_lens_draw_execution_entry(std::uint32_t entry) {
    return source_lens_draw_tu15_owns_entry(entry)||source_lens_query_tu15_owns_entry(entry)||
        source_physics_live_support_tu15_owns_entry(entry)||entry==0x37dc0au||entry==0x37cde8u;
}
namespace {
struct WorldPassBoundary {std::array<std::uint32_t,6> arguments;};
SourceLensWorldPass draw(SourceRuntimeMemory &source,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    bool planning,std::span<const SourceLensDeviceRegion> regions,const SourceLensDeviceDomains *domains,
    SourcePhysicsContactMemory *device) {
    if(!source_lens_draw_execution_entry(entry))throw FormatError("Unowned original lens drawing entry");
    if(!domains)validate_source_lens_device_regions(regions);
    SourceExecutionCallScope journal(&source,SourceExecutionCallKind::LensDraw,[&]{
        return domains?source_execution_arguments(entry,abi,planning,*domains):source_execution_arguments(entry,abi,planning,regions);});
    QueryMemory memory(source,device,true,regions,domains);
    SourcePhysicsWorldRuntimeServices services;std::function<void(std::uint32_t,SourcePhysicsWorldRuntimeAbi&)> execute;
    services.required_owner=[&](auto &,const auto &call,auto &state,const auto &) {
        if(planning&&call.target==0x2bcb0u){
            WorldPassBoundary pass;for(unsigned n=0;n<6;++n){std::array<std::byte,4> bytes;memory.read(state.gp[4]+4+n*4,bytes);pass.arguments[n]=Bytes(bytes).u32(0);}
            throw pass;
        }
        execute(call.target,state);
    };
    execute=[&](std::uint32_t target,SourcePhysicsWorldRuntimeAbi &state) {
        const auto fp=state.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80});
        if(source_lens_draw_tu15_owns_entry(target))source_run_lens_draw_tu15(memory,target,state,fp,services);
        else if(source_lens_query_tu15_owns_entry(target))source_run_lens_query_tu15(memory,target,state,fp,services);
        else if(source_physics_live_support_tu15_owns_entry(target))source_run_physics_live_support_tu15(memory,target,state,fp,services);
        else if(target==0x37dc0au||target==0x37cde8u){auto f=fp;
            source_run_physics_vehicle_motion_math_runtime(memory,target==0x37dc0au?0x37a15au:0x379338u,
                state,SourcePhysicsContactImage::TitleUpdate15,f,services);
        }else throw FormatError("Original lens drawing reached an unowned service "+std::to_string(target));
    };
    SourceLensWorldPass pass;try{execute(entry,abi);}catch(const WorldPassBoundary &boundary){pass=boundary.arguments;}
    journal.complete([&]{return source_execution_lens_draw_result(abi,memory.finish(),pass);});return pass;
}
}
SourceLensWorldPass source_execute_lens_draw(PhysicsLiveWorldTu15 &world,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,bool planning,std::span<const SourceLensDeviceRegion> regions,SourcePhysicsContactMemory *device) {
    return draw(world.memory(),entry,abi,planning,regions,nullptr,device);
}
SourceLensWorldPass source_execute_lens_draw(PhysicsLiveWorldTu15 &world,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,bool planning,const SourceLensDeviceDomains &domains,SourcePhysicsContactMemory &device) {
    return draw(world.memory(),entry,abi,planning,{},&domains,&device);
}
SourceLensWorldPass source_execute_lens_draw(SourceRuntimeMemory &source,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,bool planning,std::span<const SourceLensDeviceRegion> regions,SourcePhysicsContactMemory *device) {
    return draw(source,entry,abi,planning,regions,nullptr,device);
}
SourceLensWorldPass source_execute_lens_draw(SourceRuntimeMemory &source,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,bool planning,const SourceLensDeviceDomains &domains,SourcePhysicsContactMemory &device) {
    return draw(source,entry,abi,planning,{},&domains,&device);
}
}
