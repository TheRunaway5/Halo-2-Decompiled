#include "halo2/source_object_velocity_read_only_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class ReadOnlyVelocityMemory final:public SourcePhysicsContactMemory {
public:
    const SourceRuntimeMemory &source;
    std::array<std::byte,4096> scratch{};
    explicit ReadOnlyVelocityMemory(const SourceRuntimeMemory &bank):source(bank) {
        source.visit_committed([](auto p,auto b){
            if(std::uint64_t(p)<std::uint64_t(source_object_velocity_private_address)+4096u &&
                std::uint64_t(p)+b.size()>source_object_velocity_private_address)
                throw std::runtime_error("Private velocity scratch overlaps source storage");
        });
    }
    bool private_extent(std::uint32_t p,std::size_t n)const {
        return p>=source_object_velocity_private_address &&
            std::uint64_t(p)-source_object_velocity_private_address<=scratch.size() &&
            n<=scratch.size()-(std::uint64_t(p)-source_object_velocity_private_address);
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        if(private_extent(p,out.size()))std::copy_n(scratch.begin()+p-source_object_velocity_private_address,out.size(),out.begin());
        else source.read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte> bytes)override {
        if(!private_extent(p,bytes.size()))throw std::runtime_error("Presentation velocity query attempted a gameplay write");
        std::copy(bytes.begin(),bytes.end(),scratch.begin()+p-source_object_velocity_private_address);
    }
    void put(unsigned offset,std::uint32_t value){for(unsigned i=0;i<4;++i)scratch[offset+i]=std::byte(value>>(8*i));}
    float scalar(unsigned offset)const{std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(scratch[offset+i])<<(8*i);return std::bit_cast<float>(value);}
};
}
Vec3 source_object_linear_velocity_read_only_tu15(const SourceRuntimeMemory &source,
    std::uint32_t object,SourcePhysicsFactoryFloatingPoint fp,
    const SourceObjectVelocityReadOnlyObserverTu15 &observe) {
    ReadOnlyVelocityMemory memory(source);
    SourcePhysicsWorldRuntimeAbi abi{};abi.eflags=2;abi.floating_point=fp;
    constexpr auto sp=source_object_velocity_private_address+0x800u;
    abi.gp[0]=object;abi.gp[1]=source_object_velocity_private_address+0x40u;abi.gp[4]=sp;
    memory.put(0x800,0x30000000u);memory.put(0x804,0u); // Optional angular output is NULL.
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){trace.push_back(pc);};
    services.required_owner=[&](auto &m,const auto &call,auto &state,const auto &records){
        if(source_physics_live_support_tu15_owns_entry(call.target))
            source_run_physics_live_support_tu15(m,call.target,state,fp,services,records);
        else if(source_scene_device_constructor_tu15_owns_entry(call.target))
            source_run_scene_device_constructor_tu15(m,call.target,state,fp,services,records);
        else throw std::runtime_error("Read-only source velocity accessor requires an unavailable owner");
    };
    if(observe)observe(true,abi,memory.scratch,{});
    source_run_physics_live_support_tu15(memory,0xbc200u,abi,fp,services);
    if(abi.gp[4]!=sp+8u)throw std::runtime_error("Original BC200 did not complete its actual RET4");
    if(observe)observe(false,abi,memory.scratch,trace);
    return {memory.scalar(0x40),memory.scalar(0x44),memory.scalar(0x48)};
}
}
