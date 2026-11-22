#pragma once

#include "halo2/physics_contact_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace halo2::native_receiver_transfers {
inline constexpr std::uint32_t secondary_view=0x5598f8u;
inline constexpr std::uint32_t camera_entry=0xc3dc0u;
inline constexpr std::uint32_t camera_words=111u;
inline constexpr std::uint32_t camera_bytes=camera_words*4u;

inline bool reference() {
    static const bool enabled=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_RECEIVER_TRANSFERS");
        return value && *value;
    }();
    return enabled;
}

// Only the World's admitted secondary receiver installs this synchronous
// scope. The bank, original services and exact write counter belong to that
// executor for the entire call. Every transfer obtains its own policy lease.
class Scope final {
public:
    enum class Root { View,Camera };

    Scope(SourceRuntimeMemory &bank,const SourceRuntimeWriteSerial &serial,
        const SourcePhysicsContactRuntimeServices &services,std::uint32_t root_stack,Root root=Root::View)
        :bank_(bank),serial_(serial),services_(services),root_stack_(root_stack),root_(root),previous_(active_),
         fill_(&fill_retained),copy_(&copy_camera_retained) {
        active_=this;
    }
    ~Scope() { active_=previous_; }
    Scope(const Scope &)=delete;
    Scope &operator=(const Scope &)=delete;

    static const Scope *current(const SourcePhysicsContactMemory &bank,
        const SourcePhysicsContactRuntimeServices &services) {
        const auto *scope=active_;
        return scope && &scope->bank_==&bank && &scope->services_==&services ? scope : nullptr;
    }

    static bool owns_camera_root(const SourceRuntimeMemory &bank,std::uint32_t stack) {
        const auto frame=bank.try_read_view(stack,5u*sizeof(std::uint32_t));
        if(!frame)return false;
        std::array<std::uint32_t,5> args;
        std::memcpy(args.data(),frame->data(),frame->size());
        // Source caller prepares shape, output and count in separate slots
        // above its root. The natural 32812 caller uses different storage.
        return args[0]==0x60300000u && args[2]==stack+0x2f00u &&
            args[3]==stack+0x1000u && args[4]==stack+0x2900u;
    }

    bool fill(std::uint32_t stack,std::uint32_t site,std::uint32_t address,
        std::uint32_t count,std::uint32_t value) const {
        // Parent's three saved registers and saved EDI, return address,
        // then the three registers saved by 1373F0.
        return root_==Root::View && stack==root_stack_-0x20u && fill_(*this,site,address,count,value);
    }

    bool copy_camera(std::uint32_t stack,std::uint32_t source,std::uint32_t destination,std::uint32_t count) const {
        // Same saved registers, nine arguments and return address, then
        // 135690's three saved registers. The camera is the root's scratch.
        return root_==Root::View && stack==root_stack_-0x44u && source==root_stack_+0x1000u &&
            copy_(*this,source,destination,count);
    }

    bool clear_camera(std::uint32_t stack,std::uint32_t site,std::uint32_t address,
        std::uint32_t count,std::uint32_t value) const {
        // SUB ESP,138h and four saved registers. C3E51 follows EBP through
        // up to six original output rows. C45D4 revisits only the first row.
        if(root_!=Root::Camera || stack!=root_stack_-0x148u || count!=camera_words || value!=0)return false;
        const auto offset=address-(root_stack_+0x1000u);
        if(site==0xc3e51u && offset<6u*camera_bytes && offset%camera_bytes==0)
            return fill_(*this,site,address,count,value);
        return site==0xc45d4u && offset==0 && fill_(*this,site,address,count,value);
    }

private:
    struct Receipt {
        std::uint64_t calls{},words{};
    };
    inline static constexpr std::array sites_{0x13741eu,0x137435u,0x137457u,0x135768u,0xc3e51u,0xc45d4u};
    struct Stats {
        std::array<Receipt,sites_.size()> transfers{};
        ~Stats() {
            for(std::size_t n=0;n<sites_.size();++n)
                std::fprintf(stderr,"Receiver grouped transfer: site=0x%x calls=%llu words=%llu\n",sites_[n],
                    static_cast<unsigned long long>(transfers[n].calls),static_cast<unsigned long long>(transfers[n].words));
        }
    };
    bool commit(unsigned transfer,std::uint32_t address,std::span<const std::uint32_t> words) const {
        if(!source_runtime_try_write_words(bank_,address,words,serial_))return false;
        static const bool report=[] {
            const auto *value=std::getenv("HALO2_RECEIVER_TRANSFER_STATS");
            return value && *value;
        }();
        if(report) {
            thread_local Stats stats;
            auto &receipt=stats.transfers[transfer];
            ++receipt.calls;
            receipt.words+=words.size();
        }
        return true;
    }

    static bool fill_retained(const Scope &scope,std::uint32_t site,std::uint32_t address,
        std::uint32_t count,std::uint32_t value) {
        if(value==0 && count &&
           ((site==0x13741eu && address==secondary_view+0x21cu && count==16u) ||
            (site==0x137435u && address==secondary_view+0x25cu && count<=zeros_.size())))
            return scope.commit(site==0x13741eu?0:1,address,std::span(zeros_).first(count));
        if(site==0x137457u && address==secondary_view+0x1cu && count==none_.size() && value==0xffffffffu)
            return scope.commit(2,address,none_);
        if(site==0xc3e51u || site==0xc45d4u)
            return scope.commit(site==0xc3e51u?4:5,address,std::span(zeros_).first(camera_words));
        return false;
    }

    static bool copy_camera_retained(const Scope &scope,std::uint32_t source,std::uint32_t destination,std::uint32_t count) {
        constexpr std::size_t max_words=6*camera_words;
        if(!count || count>max_words || count%camera_words)return false;
        const auto bytes=std::size_t(count)*sizeof(std::uint32_t);
        const auto input=scope.bank_.try_read_view(source,bytes);
        const auto output=scope.bank_.try_write_extent(destination,bytes);
        if(!input || !output)return false;
        // Two source mappings may name overlapping host storage. Preserve
        // forward REP MOVSD semantics for either kind of overlap.
        const std::less<const std::byte *> before;
        if(before(output->data(),input->data()+bytes) && before(input->data(),output->data()+bytes))return false;
        std::array<std::uint32_t,max_words> words;
        std::memcpy(words.data(),input->data(),bytes);
        return scope.commit(3,destination,std::span(words).first(count));
    }

    inline static thread_local const Scope *active_{};
    inline static constexpr std::array<std::uint32_t,512> zeros_{};
    inline static constexpr auto none_=[] {
        std::array<std::uint32_t,128> words{};
        words.fill(0xffffffffu);
        return words;
    }();
    SourceRuntimeMemory &bank_;
    const SourceRuntimeWriteSerial &serial_;
    const SourcePhysicsContactRuntimeServices &services_;
    std::uint32_t root_stack_;
    Root root_;
    const Scope *previous_;
    // Standalone original-instruction oracles exercise the adapters without
    // a retained bank. Link the commit operations only for a bound scope.
    decltype(&fill_retained) fill_;
    decltype(&copy_camera_retained) copy_;
};

template<class State>
const Scope *current(const State &state) {
    const auto *scope=Scope::current(state.memory,state.services);
    if(!scope || state.observe)return nullptr;
    const auto *cursor=state.instruction_cursor;
    if(!state.literal_service_sites || !cursor || cursor!=&state.services.instruction_cursor || !cursor->position ||
       (cursor->first_trace && *cursor->first_trace) || (cursor->second_trace && *cursor->second_trace))return nullptr;
    return scope;
}

template<class State>
bool clear_camera(State &state,std::uint32_t site) {
    const auto *scope=current(state);
    if(!scope || state.image!=SourcePhysicsContactImage::TitleUpdate15 || (state.other_flags&0x400u) ||
       *state.instruction_cursor->position!=site)return false;
    auto &g=state.gp;
    if(!scope->clear_camera(g[4],site,g[7],g[1],g[0]))return false;
    g[7]+=camera_bytes;
    g[1]=0;
    return true;
}
} // namespace halo2::native_receiver_transfers
