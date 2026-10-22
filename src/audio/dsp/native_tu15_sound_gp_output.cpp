#include "halo2/native_tu15_sound_gp_output.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_gp_output {
namespace {
void validate(std::span<const Word24> words){
    for(const auto& word:words)if(word.bits>0xffffffu)throw std::invalid_argument("GP output word exceeds 24 bits");
}
bool overlaps(std::span<const Word24> a,std::span<const Word24> b){
    const auto p=reinterpret_cast<std::uintptr_t>(a.data()),q=reinterpret_cast<std::uintptr_t>(b.data());
    return p<=q?q-p<a.size_bytes():p-q<b.size_bytes();
}
}
LogicalCopyReceipt::LogicalCopyReceipt(std::shared_ptr<const Resources> resources,std::array<RingWrite,6> writes,std::uint64_t ordinal):resources_(std::move(resources)),writes_(writes),copy_ordinal_(ordinal){}
LogicalCopyReceipt::LogicalCopyReceipt(LogicalCopyReceipt&& other)noexcept:LogicalCopyReceipt(static_cast<const LogicalCopyReceipt&>(other)){}
LogicalCopyReceipt& LogicalCopyReceipt::operator=(LogicalCopyReceipt&& other)noexcept{return *this=static_cast<const LogicalCopyReceipt&>(other);}
std::span<const Word24,owned_words> LogicalCopyReceipt::retained_values()const noexcept{return resources_->output_values;}
OutputOwner::OutputOwner(const RetainedValues& seed){
    validate(seed.values);
    for(auto cursor:seed.byte_cursors)if(cursor>=2048||cursor%4!=0)throw std::invalid_argument("GP output byte cursor outside proven word-aligned ring domain");
    resources_=std::make_shared<LogicalCopyReceipt::Resources>();
    std::copy(seed.values.begin(),seed.values.end(),resources_->output_values.begin());
    resources_->byte_cursors=seed.byte_cursors;
}
std::unique_ptr<OutputOwner> OutputOwner::cold_cursor_phase(std::span<const Word24,owned_words> seed){return std::unique_ptr<OutputOwner>(new OutputOwner(RetainedValues{seed,{}}));}
std::unique_ptr<OutputOwner> OutputOwner::retained_phase(const RetainedValues& seed){return std::unique_ptr<OutputOwner>(new OutputOwner(seed));}
std::span<const Word24,ring_words> OutputOwner::ring(unsigned index)const{
    if(index>=6)throw std::out_of_range("GP output ring index");
    return std::span<const Word24,owned_words>(resources_->output_values).subspan(index*ring_words,ring_words).first<ring_words>();
}
std::span<const Word24,aux_words> OutputOwner::auxiliary_values()const noexcept{return std::span<const Word24,owned_words>(resources_->output_values).last<aux_words>();}
std::span<const Word24,owned_words> OutputOwner::retained_values()const noexcept{return resources_->output_values;}
std::array<std::uint32_t,6> OutputOwner::byte_cursors()const noexcept{return resources_->byte_cursors;}
LogicalCopyReceipt OutputOwner::copy_values(std::span<const Word24,1024> mix){
    validate(mix);
    if(overlaps(mix,resources_->output_values))throw std::invalid_argument("GP output source aliases its retained destination");
    if(resources_->copy_ordinal==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("GP output diagnostic copy ordinal overflow");
    std::array<RingWrite,6> writes{};
    for(unsigned j=0;j<6;j++){
        const auto before=resources_->byte_cursors[j];
        for(unsigned k=0;k<32;k++)resources_->output_values[j*ring_words+(before/4+k)%ring_words]=mix[mixbin_order[j]*32+k];
        const auto after=(before+128)%2048;
        resources_->byte_cursors[j]=after;
        writes[j]={mixbin_order[j],before,after,before+128>2048};
    }
    std::copy(mix.begin()+11*32,mix.begin()+31*32,resources_->output_values.begin()+6*ring_words);
    return LogicalCopyReceipt(resources_,writes,++resources_->copy_ordinal);
}
}
