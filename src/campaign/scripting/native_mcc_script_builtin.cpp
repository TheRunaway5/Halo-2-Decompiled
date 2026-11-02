#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/physics_contact_record.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>

namespace halo2 {
struct NativeMccScriptBuiltinProfile::Impl {
    struct Receipt {
        std::uint32_t datum{},function_datum{},argument_datum{0xffffffffu};
        std::uint16_t opcode{};
        std::array<std::byte,20> node{},function;
        std::array<std::byte,20> argument{};
        std::string name;
    };
    std::map<std::uint16_t,Receipt> receipts;
};
namespace {
using Impl=NativeMccScriptBuiltinProfile::Impl;
struct Bound {const SourcePhysicsContactMemory* memory;std::shared_ptr<const Impl> profile;};
thread_local std::map<const SourcePhysicsContactMemory*,Bound> bindings;
unsigned little(std::span<const std::byte> bytes,unsigned offset,unsigned count) {
    if(offset>bytes.size() || count>bytes.size()-offset)throw FormatError("Native MCC builtin field escapes its record");
    unsigned value=0;for(unsigned n=0;n<count;++n)value|=std::to_integer<unsigned>(bytes[offset+n])<<(8*n);
    return value;
}
template<std::size_t N>std::array<std::byte,N> read(const SourcePhysicsContactMemory& memory,std::uint32_t address) {
    std::array<std::byte,N> bytes;memory.read(address,bytes);return bytes;
}
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address) {
    return little(read<4>(memory,address),0,4);
}
std::string_view name_at(std::span<const std::byte> strings,unsigned offset) {
    if(offset>=strings.size())throw FormatError("Native MCC builtin name escapes string bank");
    const auto first=strings.subspan(offset);const auto end=std::find(first.begin(),first.end(),std::byte{});
    if(end==first.end())throw FormatError("Native MCC builtin name is unterminated");
    return {reinterpret_cast<const char*>(first.data()),std::size_t(end-first.begin())};
}
struct Resolved {const SourcePhysicsContactMemory* memory{};const Impl::Receipt* receipt{};};
Resolved resolve(const SourcePhysicsContactMemory& owner,std::uint32_t address) {
    const auto binding=bindings.find(&owner);if(binding==bindings.end() || binding->second.profile->receipts.empty())return {};
    const auto& memory=*binding->second.memory;
    const auto scenario=word(memory,0x4ecc20u);
    const auto count=word(memory,scenario+0x238u),first=word(memory,scenario+0x23cu);
    if(address<first || (address-first)%20 || (address-first)/20>=count)return {};
    const auto index=(address-first)/20;
    const auto receipt=binding->second.profile->receipts.find(std::uint16_t(index));
    if(receipt==binding->second.profile->receipts.end())return {};
    const auto& expected=receipt->second;
    const auto pool=word(memory,0x505e6cu);
    // Original16E510 binds this pool directly to scnr238/23C.
    if(word(memory,pool+0x44u)!=first || word(memory,pool+0x20u)!=count ||
        word(memory,pool+0x24u)!=20 || read<20>(memory,address)!=expected.node)
        throw FormatError("Native MCC builtin lost its full current syntax identity");
    const auto child=expected.function_datum&0xffffu;
    if(child>=count || read<20>(memory,first+child*20)!=expected.function)
        throw FormatError("Native MCC builtin lost its salted function-name identity");
    if(expected.argument_datum!=0xffffffffu &&
        ((expected.argument_datum&0xffffu)>=count ||
         read<20>(memory,first+(expected.argument_datum&0xffffu)*20)!=expected.argument))
        throw FormatError("Native MCC command lost its salted argument identity");
    const auto bytes=word(memory,scenario+0x1b0u),strings=word(memory,scenario+0x1b4u);
    const auto offset=little(expected.function,12,4);
    if(offset>=bytes || expected.name.size()+1>bytes-offset)
        throw FormatError("Native MCC builtin current name escapes its string bank");
    std::array<std::byte,32> current_name;
    const auto text=std::span(current_name).first(expected.name.size()+1);memory.read(strings+offset,text);
    if(name_at(text,0)!=expected.name)
        throw FormatError("Native MCC builtin current name differs from its conversion receipt");
    return {&memory,&expected};
}
}
NativeMccScriptBuiltinProfile::NativeMccScriptBuiltinProfile(const MccScriptLayout& layout,
    std::span<const std::byte> strings,bool enable_mcc_blur) {
    auto profile=std::make_shared<Impl>();
    if(layout.nodes.size()%20 || layout.nodes.size()/20>0x10000)
        throw FormatError("Native MCC builtin syntax bank extent differs");
    for(const auto& need:layout.requirements)if(need.need==MccScriptLayoutNeed::NativeClockGetter ||
        (enable_mcc_blur && need.need==MccScriptLayoutNeed::NativeBuiltin && need.donor_index==0x2b3u)) {
        const auto index=need.node_datum&0xffffu;
        if(index>=layout.nodes.size()/20)throw FormatError("Native MCC builtin receipt escapes syntax bank");
        const auto node=std::span(layout.nodes).subspan(index*20,20);
        const auto opcode=little(node,2,2);
        const bool command=need.need==MccScriptLayoutNeed::NativeBuiltin;
        // These literal signatures are pinned by complete unchanged MCC
        // execution. Keep argumentful commands separate from LONG() getters.
        const std::string_view name=opcode==0x38fu?"game_tick_rate":
            opcode==0x390u?"game_tick_rate_scalar":opcode==0x2b3u && command?"rasterizer_blur":"";
        if(name.empty() || command!=(opcode==0x2b3u) || little(node,0,2)!=(need.node_datum>>16) ||
            !little(node,0,2) || (little(node,6,2)&7) ||
            need.donor_index!=opcode || need.name!=name)
            throw FormatError("Native MCC clock receipt does not declare an authenticated getter");
        const auto child=little(node,16,4),ordinal=child&0xffffu;
        if(ordinal>=layout.nodes.size()/20)throw FormatError("Native MCC builtin function escapes syntax bank");
        const auto function=std::span(layout.nodes).subspan(ordinal*20,20);
        if(little(function,0,2)!=(child>>16) || !little(function,0,2) || (little(function,6,2)&7)!=1 ||
            little(function,4,2)!=2 || (!command && little(function,8,4)!=0xffffffffu) ||
            name_at(strings,little(function,12,4))!=name)
            throw FormatError("Native MCC clock receipt has a different function signature");
        Impl::Receipt receipt;receipt.datum=need.node_datum;receipt.function_datum=child;
        receipt.opcode=std::uint16_t(opcode);receipt.name=name;
        if(command) {
            const auto argument=little(function,8,4),ordinal=argument&0xffffu;
            if(argument==0xffffffffu || ordinal>=layout.nodes.size()/20)
                throw FormatError("Native MCC command argument escapes syntax bank");
            const auto row=std::span(layout.nodes).subspan(ordinal*20,20);
            if(!little(row,0,2) || little(row,0,2)!=(argument>>16) ||
                little(row,4,2)!=7 || little(row,8,4)!=0xffffffffu)
                throw FormatError("Native MCC blur receipt does not declare exactly one SHORT argument");
            receipt.argument_datum=argument;std::copy(row.begin(),row.end(),receipt.argument.begin());
        }
        std::copy_n(layout.nodes.begin()+index*20,20,receipt.node.begin());
        std::copy(function.begin(),function.end(),receipt.function.begin());
        if(!profile->receipts.emplace(std::uint16_t(index),std::move(receipt)).second)
            throw FormatError("Native MCC clock receipt duplicates a syntax datum");
    }
    impl_=std::move(profile);
}
bool NativeMccScriptBuiltinProfile::owns(const MccScriptLayoutRequirement& need) const noexcept {
    if(need.need!=MccScriptLayoutNeed::NativeClockGetter && need.need!=MccScriptLayoutNeed::NativeBuiltin)return false;
    const auto found=impl_->receipts.find(std::uint16_t(need.node_datum));
    return found!=impl_->receipts.end() && found->second.datum==need.node_datum &&
        found->second.opcode==need.donor_index && found->second.name==need.name &&
        (need.need==MccScriptLayoutNeed::NativeBuiltin)==(found->second.opcode==0x2b3u);
}
std::size_t NativeMccScriptBuiltinProfile::calls() const noexcept {return impl_->receipts.size();}
NativeMccScriptBuiltinBinding::NativeMccScriptBuiltinBinding(SourcePhysicsContactMemory& memory,
    const NativeMccScriptBuiltinProfile& profile):memory_(&memory),profile_(profile.impl_) {
    const auto found=bindings.find(&memory);
    if(found!=bindings.end()) {
        if(found->second.profile!=profile_)throw FormatError("Different MCC script profile is already bound to this memory owner");
        previous_=found->second.profile;
    }
    bindings.insert_or_assign(&memory,Bound{&memory,profile_});
}
NativeMccScriptBuiltinBinding::~NativeMccScriptBuiltinBinding() {
    if(std::this_thread::get_id()!=executor_)std::terminate();
    const auto found=bindings.find(memory_);
    if(found==bindings.end() || found->second.profile!=profile_)std::terminate();
    if(previous_)found->second.profile=std::move(previous_);else bindings.erase(found);
}
std::optional<std::uint32_t> native_mcc_script_builtin_value(const SourcePhysicsContactMemory& memory,
    std::uint32_t syntax_address) {
    const auto found=resolve(memory,syntax_address);if(!found.receipt)return {};
    if(found.receipt->opcode!=0x38fu && found.receipt->opcode!=0x390u)return {};
    const auto clock=word(*found.memory,0x51df54u);
    const auto tick_rate=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(little(read<2>(*found.memory,clock+2u),0,2))));
    // MCC706770 reads the signed WORD directly. MCC706780's exact signed
    // multiply/shift sequence is division by30 with truncation toward zero.
    return std::uint32_t(found.receipt->opcode==0x38fu?tick_rate:tick_rate/30);
}
std::optional<std::uint16_t> native_mcc_script_builtin_return_type(const SourcePhysicsContactMemory& memory,
    std::uint32_t syntax_address) {
    const auto found=resolve(memory,syntax_address);if(!found.receipt)return {};
    return found.receipt->opcode==0x2b3u?4:8;
}
std::optional<NativeMccScriptCommand> native_mcc_script_builtin_command(
    const SourcePhysicsContactMemory& memory,std::uint32_t syntax_address) {
    const auto found=resolve(memory,syntax_address);
    if(found.receipt && found.receipt->opcode==0x2b3u)return NativeMccScriptCommand::RasterizerBlur;
    return {};
}
void native_mcc_legacy_rasterizer_blur(std::int16_t) noexcept {
    // MCC_PIN de65b4f4...: complete180030CC0 is C2 00 00. Do not
    // substitute the unrelated bloom blur setting or invent a prototype effect.
}
} // namespace halo2
