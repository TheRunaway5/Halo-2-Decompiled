#include "halo2/native_campaign_ai_queries_tu15.hpp"
#include "halo2/native_campaign_ai_queries_tu15_binding.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::atomic<std::uint64_t> threat_calls{}, reset_calls{};
struct Access {
    SourcePhysicsContactMemory &memory;
    SourcePhysicsWorldRuntimeAbi &abi;
    const SourcePhysicsWorldRuntimeServices &services;
    const SourcePhysicsContactRecordObserver &stores;
    const NativeCampaignAiReadObserverTu15 &reads;
    void site(std::uint32_t pc) { if(services.instruction) services.instruction(pc); }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned bytes) {
        site(pc);
        if(reads) reads({pc,address,bytes});
        std::array<std::byte,4> data{};
        memory.read(address,std::span(data).first(bytes));
        std::uint32_t value{};
        for(unsigned i=0;i<bytes;++i) value |= std::to_integer<unsigned>(data[i]) << (i*8u);
        return value;
    }
    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        site(pc);
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(4)};
        if(stores) { event.before.resize(4); memory.read(address,event.before); }
        for(unsigned i=0;i<4;++i) event.written[i] = std::byte(value >> (i*8u));
        memory.write(address,event.written);
        if(stores) stores(event);
    }
    void finish(std::uint32_t pc, unsigned argument_bytes) {
        read(pc,abi.gp[4],4); // RET requires its real caller word, even for a void API.
        abi.gp[4] += 4u+argument_bytes;
    }
};
struct ThreatAccess final : NativeCampaignAiThreatAccessTu15 {
    Access &access;
    std::uint32_t property_record{}, query_payload{};
    explicit ThreatAccess(Access &value) : access(value) {}
    std::uint32_t actor_property(std::uint32_t actor) override {
        const auto header=access.read(0x1be5b4,0x5020c8,4);
        const auto rows=access.read(0x1be5c7,header+0x44u,4);
        const auto record=rows+(actor&0xffffu)*0x888u;
        return access.read(0x1be5ca,record+0x338u,4);
    }
    std::uint32_t property_query(std::uint32_t property) override {
        const auto header=access.read(0x1be5d7,0x50eef0,4);
        const auto rows=access.read(0x1be5dd,header+0x44u,4);
        property_record=rows+(property&0xffffu)*0x3cu;
        return access.read(0x1be5e9,property_record+0x14u,4);
    }
    bool bind_query(std::uint32_t query) override {
        const auto header=access.read(0x1be5f4,0x50eeec,4);
        const auto rows=access.read(0x1be606,header+0x44u,4);
        const auto record=rows+(query&0xffffu)*0x124u;
        if(record==0) return false;
        query_payload=record+0x70u;
        return query_payload!=0;
    }
    std::int16_t property_state() override {
        return std::bit_cast<std::int16_t>(std::uint16_t(
            access.read(0x1be610,property_record+0x24u,2)));
    }
    std::uint8_t query_blocked() override {
        return std::uint8_t(access.read(0x1be617,query_payload+0x69u,1));
    }
};
struct ResetAccess final : NativeCampaignAiResetAccessTu15 {
    Access &access;
    std::uint32_t record;
    ResetAccess(Access &value,std::uint32_t address) : access(value),record(address) {}
    bool marked() override { return (access.read(0x1bae4c,record+0x17u,1)&0x80u)!=0; }
    void reset() override {
        const auto value=access.read(0x1bae52,0x47d6c0,4);
        access.write(0x1bae58,record+0x14u,value);
    }
};
}
bool native_campaign_ai_queries_tu15_owns_entry(std::uint32_t entry) {
    return entry==0x1be5b0u || entry==0x1bae40u;
}
void native_campaign_ai_queries_tu15_run(SourcePhysicsContactMemory &memory,
    std::uint32_t entry, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &stores,
    const NativeCampaignAiReadObserverTu15 &reads) {
    if(!native_campaign_ai_queries_tu15_owns_entry(entry))
        throw std::invalid_argument("Unowned native campaign AI query");
    Access access{memory,abi,services,stores,reads};
    // Source datums and addresses cross this boundary once. Only public
    // return values and callee argument consumption are encoded on return;
    // dead volatile registers, flags and retired private stack are not a CPU
    // state that the native algorithm must reconstruct.
    if(entry==0x1be5b0u) {
        threat_calls.fetch_add(1,std::memory_order_relaxed);
        const auto actor=access.read(0x1be5b0,abi.gp[4]+4u,4);
        ThreatAccess records(access);
        const auto score=native_campaign_ai_threat_tu15(records,actor);
        access.finish(0x1be624,4);
        abi.gp[0]=score;
    } else {
        reset_calls.fetch_add(1,std::memory_order_relaxed);
        const auto preserve=access.read(0x1bae40,abi.gp[4]+12u,1);
        const auto record_address=access.read(0x1bae46,abi.gp[4]+8u,4);
        ResetAccess record(access,record_address);
        native_campaign_ai_reset_tu15(record,preserve!=0);
        access.finish(0x1bae5b,12);
    }
}
NativeCampaignAiQueryActivityTu15 native_campaign_ai_query_activity_tu15() {
    return {threat_calls.load(std::memory_order_relaxed),reset_calls.load(std::memory_order_relaxed)};
}
}
