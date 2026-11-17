#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

int main(int argc,char** argv)try {
    using namespace halo2;
    using namespace halo2::object_property_source_test;
    check(argc==5,"Supply Coagulation, TU1.5 XBE, save directory and ParticleDraw fixture");
    Cache cache(argv[1]);
    PhysicsLiveWorldTu15 world(cache,argv[2],argv[3]);
    auto& bank=world.memory();
    bank.mount(0x61000000u,std::vector<std::byte>(16384),SourceRuntimeMemory::Access::ReadWrite,
        "ParticleDraw inherited original fixture");
    bank.mount(0x62000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,
        "ParticleDraw inherited original stack");
    std::ifstream stream(argv[4]);const auto rows=J::parse(stream);
    J reply=J::array();
    for(const auto& row:rows){
        const Seed seed{row};
        for(const auto& [address,data]:seed.banks)
            if(address==0x61000000u||address==0x62000000u)bank.write(address,data);
        for(const auto& seed:row.at("seeds"))
            bank.write(seed.at("address"),bytes(seed.at("bytes")));
        auto abi=seed.abi;
        J stores=J::array();
        const SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore& event) {
            stores.push_back(J{{"instruction",event.instruction},{"address",event.address},
                {"before",hex(event.before)},{"written",hex(event.written)}});
        };
        try {
            world.run_actual(0x1770c0u,abi,observe);
        }catch(const std::exception& error){
            throw std::runtime_error(row.at("name").get<std::string>()+": "+error.what());
        }
        check(abi.floating_point.has_value()&&!abi.active_mmx,"Unexpected inherited ParticleDraw FP state");
        J live=J::array(),banks=J::array();
        for(unsigned n=0;n<abi.x87_depth;++n)
            live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
        for(const auto& [address,data]:seed.banks){
            if(address!=0x61000000u&&address!=0x62000000u)continue;
            std::vector<std::byte> after(data.size());bank.read(address,after);
            banks.push_back(J{{"address",address},{"after",hex(after)}});
        }
        reply.push_back(J{{"name",row.at("name")},{"expected",{{"entry",0x1772afu},{"gp",abi.gp},
            {"eflags",abi.eflags},{"xmm",abi.xmm},{"fp",J::array({abi.floating_point->x87_control_word,
                abi.x87_status,abi.floating_point->mxcsr})},{"live_x87",live}}},{"stores",stores},{"banks",banks}});
    }
    std::cout<<reply.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
