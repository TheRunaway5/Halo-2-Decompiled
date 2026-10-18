#include "halo2/campaign_source_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
constexpr std::array<std::pair<unsigned,unsigned>,3> fields{{{0x52dfd8u,0x40c0u},{0x532098u,4u},{0x4922c0u,8u}}};
using LensBytes=std::array<std::vector<std::byte>,3>;
LensBytes read(const SourceRuntimeMemory &memory) {
    LensBytes out;
    for(unsigned i=0;i<fields.size();++i){const auto b=memory.view(fields[i].first,fields[i].second);out[i].assign(b.begin(),b.end());}
    return out;
}
void pattern(SourceRuntimeMemory &memory,unsigned seed) {
    for(unsigned i=0;i<fields.size();++i) {
        std::vector<std::byte> bytes(fields[i].second);
        for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte((seed+13*n+29*i)&255u);
        memory.write(fields[i].first,bytes);
    }
}
std::uint32_t word(const SourceRuntimeMemory &memory,unsigned address){return Bytes(memory.view(address,4)).u32(0);}
void put(SourceRuntimeMemory &memory,unsigned address,std::uint32_t value){memory.write(address,std::as_bytes(std::span(&value,1)));}
}
int main(int argc,char **argv)try {
    if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    CampaignSourceRuntime runtime(cache,argv[1],0,1,0x12345678u);
    auto lifetimes=runtime.lifetimes();auto &objects=runtime.objects();auto &memory=objects.memory();
    auto saved=runtime.checkpoint(lifetimes);const auto saved_lens=read(memory);
    const auto rng=word(memory,0x4f3cd8u),saved_random=word(memory,rng);
    pattern(memory,0xa4);put(memory,rng,saved_random^0x12345678u);
    runtime.restore(saved,lifetimes);
    check(read(memory)==saved_lens && word(memory,rng)==saved_random,"Unregistered campaign restore changed full-RAM semantics");
    runtime.live_lens_renderer(true);
    pattern(memory,0x51);put(memory,rng,saved_random^0x87654321u);const auto live=read(memory);
    // Even with a local live campaign consumer, the generic object API must
    // continue to stage the exact full-RAM authority checkpoint.
    {auto ordinary=objects.prepare_authority(saved.objects);check(read(ordinary.replacement_memory())==saved_lens,"Generic authority inherited campaign renderer retention");}
    {auto staged=objects.prepare_campaign_authority(saved.objects);
        check(read(staged.replacement_memory())==live,"Named campaign preparation did not retain current renderer bytes");
        check(word(staged.replacement_memory(),rng)==saved_random,"Named campaign preparation failed to restore game RNG");
        bool refused{};try{runtime.live_lens_renderer(false);}catch(const std::exception&){refused=true;}
        check(refused,"Consumer registration changed during the enclosing authority barrier");
    }
    check(read(memory)==live && word(memory,rng)==(saved_random^0x87654321u),"Cancelled preparation changed live source state");
    const auto mutations=objects.execution_mutations();const auto lifetime_before=lifetimes.source_authority_checkpoint();
    ++saved.lifetimes.version;bool refused{};
    try{runtime.restore(saved,lifetimes);}catch(const std::exception&){refused=true;}
    --saved.lifetimes.version;
    check(refused && objects.execution_mutations()==mutations && lifetimes.source_authority_checkpoint()==lifetime_before,
        "Failed coupled lifetime staging changed the live authority transaction");
    check(read(memory)==live && word(memory,rng)==(saved_random^0x87654321u),"Failed transaction changed live lens or game bytes");
    runtime.restore(saved,lifetimes);
    check(read(memory)==live && word(memory,rng)==saved_random,"Active campaign restore did not combine live renderer with saved game state");
    runtime.live_lens_renderer(false);pattern(memory,0xd1);
    runtime.restore(saved,lifetimes);
    check(read(memory)==saved_lens,"Unregistering actual renderer did not restore exact full-RAM default");
    std::cout<<"PASS public campaign default/full-RAM, local active renderer retention, generic API isolation, cancellation, failed coupled transaction and unregister\n"
        <<"LIMIT declared CPU consumer controls; no GPU completion or full original post-restore callback parity proof\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
