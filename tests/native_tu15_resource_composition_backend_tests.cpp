#include "native_tu15_resource_composition_backend.hpp"
#include <fstream>
#include <iostream>
using namespace halo2;
using namespace halo2::resource_composition_fixture;
namespace {
void check(bool value,const char*why){if(!value)throw std::runtime_error(why);}
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>data=std::vector<std::byte>(65536);
    unsigned writes{};
    std::span<std::byte>view(std::uint32_t at,unsigned n){
        check(at>=0x61000000u&&std::uint64_t(at-0x61000000u)+n<=data.size(),"Composition provider escaped backing");return std::span(data).subspan(at-0x61000000u,n);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{const auto source=const_cast<Memory*>(this)->view(at,out.size());std::ranges::copy(source,out.begin());}
    void write(std::uint32_t at,std::span<const std::byte>in)override{++writes;std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto out=view(at,4);for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(8u*n));}
};
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply actual owned cache-sector file");std::uint64_t reads{},transferred{};
    constexpr std::array extents{1u,511u,512u,513u,1025u,4095u};
    for(unsigned mode=0;mode<9;++mode){
        Memory memory;FileBackend backend(argv[1],127u+mode);const auto extent=extents[mode%extents.size()],size=(extent+511u)&~511u;
        constexpr std::uint32_t tracking=0x61000100u,descriptor=0x61000200u,destination=0x61001000u,stack=0x61008000u;
        SourceRuntimeCacheAnimationImage resource;resource.bytes.resize(extent);memory.put(descriptor,0x12345678u);memory.put(descriptor+4u,extent);memory.put(descriptor+8u,0x40u);
        std::ranges::copy(memory.view(descriptor,20u),resource.descriptor.begin());memory.put(tracking+12u,descriptor);
        memory.put(stack,0x12613eu);memory.put(stack+4u,size);memory.put(stack+8u,destination);memory.put(stack+12u,0);memory.put(stack+16u,tracking+4u);memory.put(stack+20u,6);memory.put(stack+24u,3);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x87654321u;abi.gp[1]=0x40u;abi.gp[4]=stack;abi.gp[7]=tracking;const auto initial=abi;
        if(mode==6)backend.retained.next_resource_read=0x100000000ull;
        if(mode==7)backend.retained.resource_reads.emplace(0xf8000001u,ResourceReadOwner::NativeResourceRead{9,8,7});
        if(mode==8)memory.put(stack+20u,5);
        bool failed=false;
        try{
            check(source_try_animation_cache_io_tu15(memory,memory,{0x126139u,0x1a3b70u,false},abi,std::this_thread::get_id(),
                [&](auto p){check(p==descriptor,"Descriptor owner differs");return resource;},
                [&](auto p){return memory.view(p,12);},[&](const auto&r){return backend.read(r);},[&](const auto&r){return backend.retain(r);}),"Shared synchronous World provider declined");
        }catch(const FormatError&){failed=true;}
        check(failed==(mode>=6),"Actual resource provider failure boundary differs");
        reads+=backend.actual_file_calls;transferred+=backend.actual_file_bytes;
        if(mode<6){
            check(abi.gp[0]==0xf8000001u&&abi.gp[4]==stack+28u,"Actual retained-read token/ABI differs");
            check(backend.retained.next_resource_read==0xf8000002ull&&backend.retained.resource_reads.size()==1,"Actual map namespace did not own the returned token");
            const auto&r=backend.retained.resource_reads.at(abi.gp[0]);check(r.descriptor==descriptor&&r.destination==destination&&r.bytes==size,"Actual retained resource metadata differs");
            check(memory.view(tracking+4u,1)[0]==std::byte{1},"Shared synchronous policy failed to publish completion");
            std::ifstream file(argv[1],std::ios::binary);file.seekg(0x40);std::vector<std::byte>expected(size);file.read(reinterpret_cast<char*>(expected.data()),size);
            check(std::size_t(file.gcount())==size&&std::ranges::equal(memory.view(destination,size),expected),"Shared I/O output differs from the actual owned file");
        }else{
            check(abi.gp==initial.gp&&memory.view(tracking+4u,1)[0]==std::byte{0},"Failed resource retention fabricated completion/return state");
            check(mode==8?backend.actual_file_calls==0:backend.actual_file_bytes==size,"I/O/registry publication failure ordering differs");
        }
    }
    std::cout<<"PASS synchronous production-policy file/registry seam cases=9 file_calls="<<reads<<" file_bytes="<<transferred<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
