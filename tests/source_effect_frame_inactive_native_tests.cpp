#include "source_match_rebind_fixture.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
int main(int argc,char **argv)try {
    if(argc!=3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    const std::array<unsigned,1> locals{0};
    Owner host(argv[1],argv[2],locals,1000),replica(argv[1],argv[2],locals,900000);
    auto saved=replica.authority->prepare(host.authority->checkpoint(binding),binding);
    check(replica.authority->commit(std::move(saved)),"Inactive frame replica admission failed");
    SourceWorldParticleDrawsTu15 graphics(host.cache,{});
    check(!graphics.active(),"Declared empty attachment census acquired graphics resources");
    const auto storage_before=word(host.world->memory(),0x4c2aa8u);
    for(const auto seconds:std::array{0.f,1.f/120.f}) {
        const auto phase=host.authority->capture_execution_phase([&]{graphics.advance(host.world->tu15_world(),seconds);});
        std::vector<std::uint32_t> owners;
        using K=SourceExecutionCallKind;
        for(const auto &call:phase.calls) {
            if(call.kind==K::CallActualSource)owners.push_back(Bytes(call.arguments).u32(0));
            if(call.kind==K::EffectParticles)owners.push_back(0x179df0u);
        }
        check(owners==(seconds>0?std::vector<std::uint32_t>{0x17ead0u,0x180520u,0x1378e0u,0x179df0u}:std::vector<std::uint32_t>{0x179df0u}),
            "Inactive graphics omitted or repeated a global source frame owner");
        replica.authority->replay_execution_phase(decode_source_execution_phase(encode_source_execution_phase(phase)));
        check(encode_source_authority(host.authority->checkpoint(binding))==encode_source_authority(replica.authority->checkpoint(binding)),
            "Inactive graphics global frame did not replay the complete coupled world");
        check(word(host.world->memory(),0x4c2aa8u)==storage_before,"Inactive graphics invented transparent storage");
    }
    std::cout<<"PASS inactive graphics census:zero and positive source frames execute original global owners once, automatic complete-world replay, no fabricated transparent storage\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
