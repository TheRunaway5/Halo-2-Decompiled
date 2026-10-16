#include "halo2/source_runtime_host_physical_memory.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Invalid native physical release was accepted");}
constexpr SourcePhysicalAllocationRequest request{0x3000,0x06100000,0x06102fff,4096,4};
void release_and_reallocate(){
    SourceRuntimeMemory memory;SourceRuntimeHostPhysicalMemory physical(memory),foreign(memory);
    const auto source=physical.allocate(request);const auto native=memory.native_address(source,request.bytes);
    physical.protect(source+0xfff,2,2);const auto saved=memory.checkpoint();const auto ownership=physical.ownership_checkpoint();
    check(ownership.allocations.size()==1&&ownership.allocations[0].read_only==std::vector<std::uint8_t>({1,1,0}),"Actual read-only physical rows differ");
    for(const auto address:{source+1,source+4096,source&0x7fffffffu,0x90000000u}){
        rejected([&]{physical.release(address);});check(memory.checkpoint()==saved&&physical.ownership_checkpoint()==ownership,"Rejected physical release changed actual ownership/bytes");
    }
    rejected([&]{foreign.release(source);});check(memory.native_address(source,request.bytes)==native,"Foreign physical release replaced actual backing");
    SourceRuntimeMemory replacement;replacement.restore(saved);
    {auto prepared=physical.prepare_ownership(ownership,saved,replacement);rejected([&]{physical.release(source);});
        check(memory.native_address(source,request.bytes)==native&&memory.checkpoint()==saved,"Prepared ownership release rejection changed the source bank");}
    physical.release(source);check(physical.ownership_checkpoint().allocations.empty()&&memory.checkpoint().regions.empty(),"Actual release retained source/native ownership");
    rejected([&]{memory.native_address(source,1);});rejected([&]{memory.source_address(native,1);});rejected([&]{physical.release(source);});
    const auto again=physical.allocate(request);check(again==source,"Released physical namespace was not available to actual reallocation");
    const auto writable=physical.ownership_checkpoint();check(writable.allocations.size()==1&&writable.allocations[0].read_only==std::vector<std::uint8_t>({0,0,0}),"Released read-only protection rows leaked into reallocation");
    memory.write(again,std::array{std::byte{0x71}});memory.write(again+4096,std::array{std::byte{0x92}});
    check(memory.view(again,1)[0]==std::byte{0x71}&&memory.view(again+4096,1)[0]==std::byte{0x92},"Actual released/reallocated protected pages are not writable");
    physical.release(again);check(physical.ownership_checkpoint().allocations.empty(),"Repeated actual release retained checkpoint ownership");
}
void changed_backing(){
    SourceRuntimeMemory memory;std::uint32_t source{};
    {SourceRuntimeHostPhysicalMemory physical(memory);source=physical.allocate(request);const auto ownership=physical.ownership_checkpoint();
        memory.release(source);memory.mount(source,std::vector<std::byte>(request.bytes,std::byte{0x5a}),SourceRuntimeMemory::Access::ReadWrite,"independent replacement owner");
        const auto replacement=memory.checkpoint();rejected([&]{physical.release(source);});
        check(memory.checkpoint()==replacement&&physical.ownership_checkpoint()==ownership,"Changed-backing release removed/replaced independent bytes");}
    check(memory.view(source,request.bytes)[17]==std::byte{0x5a},"Physical destructor removed a foreign replacement backing after rejection");memory.release(source);
}
}
int main(){try{release_and_reallocate();changed_backing();std::cout<<"PASS exact native physical release, staged/foreign/interior/double/replaced rejection, protection cleanup and logical reallocation\n";return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
