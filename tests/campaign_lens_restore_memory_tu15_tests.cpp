#include "halo2/source_campaign_lens_restore_tu15.hpp"
#include <iostream>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
constexpr std::array<std::pair<unsigned,unsigned>,3> fields{{{0x52dfd8u,0x40c0u},{0x532098u,4u},{0x4922c0u,8u}}};
SourceRuntimeMemory bank(unsigned seed,int fault=-1,bool read_only=false,bool coarse=false) {
    SourceRuntimeMemory memory;
    if(coarse) {
        memory.mount(0x52df00u,std::vector<std::byte>(0x4200,std::byte(seed)),SourceRuntimeMemory::Access::ReadWrite,"isolated offline data section covering actual lens ranges");
        memory.mount(0x492200u,std::vector<std::byte>(0x100,std::byte(seed)),SourceRuntimeMemory::Access::ReadWrite,"isolated offline data section covering display stamp");
    } else for(unsigned i=0;i<fields.size();++i) {
        const auto [address,bytes]=fields[i];
        if(fault==int(i))continue;
        std::vector<std::byte> data(bytes);
        for(unsigned n=0;n<bytes;++n)data[n]=std::byte((seed+17*i+13*n)&255u);
        memory.mount(address,data,read_only?SourceRuntimeMemory::Access::ReadOnly:SourceRuntimeMemory::Access::ReadWrite,"declared isolated original lens state");
    }
    memory.mount(0x80010000u,std::vector<std::byte>(64,std::byte(seed)),SourceRuntimeMemory::Access::ReadWrite,"isolated serialized game arena control");
    if(!coarse)memory.mount(0x53209cu,std::array{std::byte(seed)},SourceRuntimeMemory::Access::ReadWrite,"adjacent non-retained source byte");
    return memory;
}
}
int main()try {
    auto current=bank(0xa3);auto saved_bank=bank(0x21);const auto saved=saved_bank.checkpoint();
    const auto current_before=current.checkpoint();
    SourceRuntimeMemory ordinary;ordinary.restore(saved);
    check(ordinary.checkpoint()==saved,"Default full-RAM restore did not retain exact saved bytes");
    SourceRuntimeMemory staged;staged.restore(saved);
    source_stage_live_campaign_lens_tu15(current,staged);
    for(const auto &[address,bytes]:fields)
        check(std::ranges::equal(staged.view(address,bytes),current.view(address,bytes)),"Active lens field did not adopt actual current bytes");
    check(std::ranges::equal(staged.view(0x80010000u,64),saved_bank.view(0x80010000u,64)),"Lens adoption changed saved game arena");
    check(staged.view(0x53209cu,1).front()==std::byte{0x21},"Lens adoption escaped its exact final boundary");
    check(current.checkpoint()==current_before && saved_bank.checkpoint()==saved,"Staging mutated live source or saved checkpoint");
    unsigned rejected{};
    for(unsigned side=0;side<2;++side)for(unsigned range=0;range<3;++range) {
        auto missing=bank(0x32,int(range));auto valid=bank(0x49);const auto a=missing.checkpoint(),b=valid.checkpoint();
        bool refused{};
        try{if(side)source_stage_live_campaign_lens_tu15(valid,missing);else source_stage_live_campaign_lens_tu15(missing,valid);}
        catch(const std::exception&){refused=true;}
        check(refused,"Missing actual source/replacement lens region was silently skipped");
        check(missing.checkpoint()==a && valid.checkpoint()==b,"Failed range admission changed a bank");++rejected;
    }
    for(unsigned side=0;side<2;++side) {
        auto immutable=bank(0x38,-1,true);auto valid=bank(0x57);const auto a=immutable.checkpoint(),b=valid.checkpoint();bool refused{};
        try{if(side)source_stage_live_campaign_lens_tu15(valid,immutable);else source_stage_live_campaign_lens_tu15(immutable,valid);}
        catch(const std::exception&){refused=true;}
        check(refused,"Read-only owner was admitted as live writable renderer state");
        check(immutable.checkpoint()==a && valid.checkpoint()==b,"Read-only rejection changed a bank");++rejected;
    }
    for(unsigned side=0;side<2;++side)for(unsigned range=0;range<3;++range) {
        auto short_saved=bank(0x72).checkpoint();
        auto row=std::ranges::find(short_saved.regions,fields[range].first,&SourceRuntimeMemory::CheckpointRegion::address);
        check(row!=short_saved.regions.end(),"Truncated fixture lost target region");row->bytes.pop_back();
        SourceRuntimeMemory truncated;truncated.restore(short_saved);auto valid=bank(0x48);
        const auto a=truncated.checkpoint(),b=valid.checkpoint();bool refused{};
        try{if(side)source_stage_live_campaign_lens_tu15(valid,truncated);else source_stage_live_campaign_lens_tu15(truncated,valid);}
        catch(const std::exception&){refused=true;}
        check(refused && truncated.checkpoint()==a && valid.checkpoint()==b,"Truncated live/replacement region was admitted or changed a bank");++rejected;
    }
    bool same_refused{};try{source_stage_live_campaign_lens_tu15(current,current);}catch(const std::invalid_argument&){same_refused=true;}
    check(same_refused && current.checkpoint()==current_before,"Adoption accepted its live bank as the replacement");
    // Discarding a completed preparation must preserve the live timeline.
    {SourceRuntimeMemory cancelled;cancelled.restore(saved);source_stage_live_campaign_lens_tu15(current,cancelled);}
    check(current.checkpoint()==current_before,"Cancelled lens adoption changed live state");
    auto coarse=bank(0x62,-1,false,true);auto coarse_old=bank(0x17,-1,false,true);
    source_stage_live_campaign_lens_tu15(coarse,coarse_old);
    for(const auto &[address,bytes]:fields)check(std::ranges::equal(coarse.view(address,bytes),coarse_old.view(address,bytes)),"Explicit offline-section live renderer adoption failed");
    check(coarse_old.view(0x53209cu,1).front()==std::byte{0x17},"Coarse-section adoption retained unrelated data");
    current.swap_backing(staged);
    auto expected_live=bank(0xa3);
    for(const auto &[address,bytes]:fields)check(std::ranges::equal(current.view(address,bytes),expected_live.view(address,bytes)),"Prepared commit lost the retained live renderer bytes");
    check(current.view(0x80010000u,1).front()==std::byte{0x21},"Prepared commit did not restore actual game-state control");
    std::cout<<"PASS full-RAM default, three live lens regions, exact boundaries, "<<rejected<<" missing/RO rejections, cancellation and coarse-section opt-in\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
