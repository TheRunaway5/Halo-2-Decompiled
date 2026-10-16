#include "halo2/native_tu15_sound_gp_output.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using namespace halo2::tu15_gp_output;
static_assert(!std::is_copy_constructible_v<OutputOwner> && !std::is_move_constructible_v<OutputOwner>);
static_assert(!std::is_default_constructible_v<LogicalCopyReceipt>);
static_assert(std::is_nothrow_copy_constructible_v<LogicalCopyReceipt> && std::is_nothrow_copy_assignable_v<LogicalCopyReceipt>);
static_assert(std::is_nothrow_move_constructible_v<LogicalCopyReceipt> && std::is_nothrow_move_assignable_v<LogicalCopyReceipt>);
std::uint32_t read(){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated logical value fixture");return v;}
int main(){
    if(read()!=0x47504f31)throw std::runtime_error("Fixture protocol");
    const auto count=read();std::unique_ptr<OutputOwner> owner;unsigned copies=0,resets=0,guards=0;
    for(unsigned i=0;i<count;i++){
        const auto execute=read();std::array<std::uint32_t,6> cursors;for(auto& c:cursors)c=read();std::array<Word24,owned_words> expected;for(auto& w:expected)w=Word24(read());std::array<Word24,1024> mix;for(auto& w:mix)w=Word24(read());
        if(!execute){owner=OutputOwner::retained_phase({expected,cursors});resets++;}
        else{const auto old=owner->byte_cursors();const auto receipt=owner->copy_values(mix);copies++;if(owner->byte_cursors()!=cursors||!std::equal(expected.begin(),expected.end(),owner->retained_values().begin()))throw std::runtime_error("Original supervisor value replay mismatch");for(unsigned j=0;j<6;j++)if(receipt.writes()[j].mixbin!=mixbin_order[j]||receipt.writes()[j].before_byte_cursor!=old[j]||receipt.writes()[j].after_byte_cursor!=cursors[j])throw std::runtime_error("Actual copy receipt mismatch");}
    }
    const auto before=std::vector<Word24>(owner->retained_values().begin(),owner->retained_values().end());const auto cursors=owner->byte_cursors();
    std::array<Word24,1024> values{};
    for(auto& w:values){w.bits=0x1000000;try{(void)owner->copy_values(values);throw std::runtime_error("Invalid source accepted");}catch(const std::invalid_argument&){guards++;}w.bits=0;if(owner->byte_cursors()!=cursors||!std::equal(before.begin(),before.end(),owner->retained_values().begin()))throw std::runtime_error("Rejected source mutated owner");}
    try{(void)owner->copy_values(owner->retained_values().first<1024>());throw std::runtime_error("Alias accepted");}catch(const std::invalid_argument&){guards++;}
    std::array<Word24,owned_words> seed{};
    for(auto& w:seed){w.bits=0x1000000;try{(void)OutputOwner::cold_cursor_phase(seed);throw std::runtime_error("Invalid seed accepted");}catch(const std::invalid_argument&){guards++;}w.bits=0;}
    for(unsigned j=0;j<6;j++)for(auto invalid:{1u,2047u,2048u,0xffffffu}){std::array<std::uint32_t,6> c{};c[j]=invalid;try{(void)OutputOwner::retained_phase({seed,c});throw std::runtime_error("Invalid cursor accepted");}catch(const std::invalid_argument&){guards++;}}
    try{(void)owner->ring(6);throw std::runtime_error("Invalid index accepted");}catch(const std::out_of_range&){guards++;}
    // Cold cursor phase preserves actual retained values and resets only cursors.
    auto cold=OutputOwner::cold_cursor_phase(owner->retained_values());if(cold->byte_cursors()!=std::array<std::uint32_t,6>{}||!std::equal(before.begin(),before.end(),cold->retained_values().begin()))throw std::runtime_error("Cold constructor corrupted retained data");
    for(unsigned j=0;j<1024;j++)values[j]=Word24(j+1);
    auto receipt=cold->copy_values(values);cold.reset();if(receipt.retained_values()[0].bits!=1||receipt.retained_values()[6*512].bits!=353)throw std::runtime_error("Receipt failed actual resource retention");
    unsigned lifetime_controls=0;
    const auto verify_receipt=[&](const LogicalCopyReceipt& r){
        if(r.retained_values().data()!=receipt.retained_values().data()||r.retained_values()[0].bits!=1||r.retained_values()[6*512].bits!=353||r.copy_ordinal()!=1)throw std::runtime_error("Receipt retained value/identity mismatch");
        for(unsigned j=0;j<6;j++)if(r.writes()[j].mixbin!=mixbin_order[j]||r.writes()[j].before_byte_cursor!=0||r.writes()[j].after_byte_cursor!=128||r.writes()[j].split)throw std::runtime_error("Receipt immutable metadata mismatch");
        lifetime_controls++;
    };
    auto copied=receipt;verify_receipt(copied);verify_receipt(receipt);
    auto moved=std::move(copied);verify_receipt(moved);verify_receipt(copied);
    auto assigned=owner->copy_values(values);assigned=receipt;verify_receipt(assigned);verify_receipt(receipt);
    auto move_assigned=owner->copy_values(values);move_assigned=std::move(assigned);verify_receipt(move_assigned);verify_receipt(assigned);
    auto* same_receipt=&moved;
    moved=std::move(*same_receipt);verify_receipt(moved);
    moved=*same_receipt;verify_receipt(moved);
    {
        auto ephemeral=receipt;auto destination=std::move(ephemeral);verify_receipt(destination);verify_receipt(ephemeral);
    }
    const bool native64=reinterpret_cast<std::uintptr_t>(owner->retained_values().data())>0xffffffffull;if(!native64)throw std::runtime_error("Native64 allocation not observed");
    if(std::cin.peek()!=EOF)throw std::runtime_error("Trailing fixture data");
    std::cout<<"{\"passed\":true,\"original_frames\":"<<copies<<",\"phase_snapshots\":"<<resets<<",\"no_mutation_guards\":"<<guards<<",\"receipt_lifetime_controls\":"<<lifetime_controls<<",\"retained_receipt_after_owner_destruction\":true,\"native64_allocation\":true,\"install_ack\":false,\"graph_frame_receipt\":false,\"dma_packing_or_pcm\":false}\n";
}
