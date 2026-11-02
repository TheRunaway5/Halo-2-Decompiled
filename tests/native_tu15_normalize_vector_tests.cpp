#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_normalize_vector_bridge.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#ifdef HALO2_NORMALIZE_LITERAL_VARIANTS
#include "normalize_literal_variants.hpp"
#endif

namespace {
using namespace halo2;using J=nlohmann::json;
std::vector<std::byte> bytes(const std::string &s) {
    std::vector<std::byte> out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;
}
std::string hex(std::span<const std::byte> b) {
    const char *digits="0123456789abcdef";std::string out;for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    SourceDirectPageIndex pages;
    mutable J reads=J::array();J writes=J::array();bool recording{},constant_views{},constant_view_alias{},short_constant_view{};
    bool writable_views{},writable_view_alias{},short_writable_view{};int reject=-1;mutable int attempts{};
    auto locate(std::uint32_t p,std::size_t n)const {
        auto it=banks.upper_bound(p);if(it==banks.begin())throw std::runtime_error("Missing normalize bank");--it;
        if(std::uint64_t(p-it->first)+n>it->second.size())throw std::runtime_error("Missing normalize extent");return it;
    }
    void read(std::uint32_t p,std::span<std::byte> b)const override {
        if(recording && attempts++==reject)throw std::runtime_error("Declared normalize read refusal");
        const auto it=locate(p,b.size());std::copy_n(it->second.begin()+p-it->first,b.size(),b.begin());
        if(recording)reads.push_back(J::array({p,b.size()}));
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override {
        const auto it=locate(p,b.size());std::copy(b.begin(),b.end(),banks.at(it->first).begin()+p-it->first);
        if(recording)writes.push_back(J::array({p,hex(b)}));
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t p,std::size_t n)const noexcept override {
        if(!constant_views||n!=4||(p!=0x46a0ec&&p!=0x46a0d0&&p!=0x46504c&&p!=0x465030))return std::nullopt;
        if(constant_view_alias)return std::span<const std::byte>(banks.at(0x62000000).data()+0x7fc,4);
        const auto it=banks.find(p);if(it==banks.end()||it->second.size()!=4)return std::nullopt;
        return std::span<const std::byte>(it->second).first(short_constant_view?3:4);
    }
    std::optional<std::span<const std::byte>> try_write_extent(std::uint32_t p,std::size_t n)const noexcept override {
        if(!writable_views||!n)return std::nullopt;
        if(writable_view_alias&&p<0x62000000)p=0x620007fc;
        auto it=banks.upper_bound(p);if(it==banks.begin())return std::nullopt;--it;
        const auto offset=std::uint64_t(p)-it->first;
        if(offset>=it->second.size()||n>it->second.size()-offset)return std::nullopt;
        return std::span<const std::byte>(it->second).subspan(std::size_t(offset),short_writable_view?n-1:n);
    }
    void enable_direct() {
        direct_pages_=&pages;
        for(auto &[p,b]:banks)if(!(p&4095u))for(std::size_t n=0;n+4096<=b.size();n+=4096) {
            const auto address=p+std::uint32_t(n);auto &block=pages.blocks[address>>22];
            if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(address>>12)&1023u]={b.data()+n,true};
        }
    }
    void page(std::uint32_t address,bool present,bool writable) {
        auto &block=pages.blocks[address>>22];if(!block)return;
        auto &p=(*block)[(address>>12)&1023u];if(!present)p.bytes=nullptr;p.writable=writable;
    }
    void alias_page(std::uint32_t address,std::uint32_t backing) {
        (*pages.blocks[address>>22])[(address>>12)&1023u].bytes=
            (*pages.blocks[backing>>22])[(backing>>12)&1023u].bytes;
    }
};
J run(const J &row) {
    Memory memory;for(const auto &b:row.at("banks"))memory.banks.emplace(b.at("address"),bytes(b.at("before")));
    memory.constant_views=row.value("constant_views",false);memory.constant_view_alias=row.value("constant_view_alias",false);
    memory.short_constant_view=row.value("short_constant_view",false);
    memory.writable_views=row.value("writable_views",false);memory.writable_view_alias=row.value("writable_view_alias",false);
    memory.short_writable_view=row.value("short_writable_view",false);
    if(row.value("direct",false))memory.enable_direct();
    if(row.value("alias_stack_page",false))memory.alias_page(0x62000000,0x1000000);
    for(auto p:row.value("read_only_pages",std::vector<std::uint32_t>{}))memory.page(p,true,false);
    for(auto p:row.value("missing_pages",std::vector<std::uint32_t>{}))memory.page(p,false,false);
    SourcePhysicsContactRuntimeAbi entry{};entry.gp=row.at("gp");entry.xmm=row.at("xmm");entry.eflags=row.at("eflags");
    entry.fs_base=0x7ffdf000;entry.x87_depth=row.at("live_x87").size();entry.x87_status=row.at("before_fp")[1];
    for(unsigned n=0;n<entry.x87_depth;++n){const auto b=bytes(row.at("live_x87")[n]);std::memcpy(&entry.x87[n],b.data(),10);}
    entry.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
    const auto image=row.at("image")=="mp11"?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15;
    NativeTu15NormalizeStats stats{};
    SourcePhysicsContactRuntimeServices services{},underlying{};SourcePhysicsContactRecordObserver observe{};
    J events=J::array();if(row.value("observe",false))observe=[&](const auto&e){
        events.push_back(J::array({e.instruction,e.address,hex(e.before),hex(e.written)}));};
    if(row.value("kernel",false))services.normalize_vector_30bd0=native_tu15_normalize_vector_30bd0_resident;
    if(row.value("guarded",false)) {
        services.normalize_vector_30bd0=nullptr;
        services.normalize_vector_30bd0_guarded=[&](auto &m,auto v,auto sp,const auto&o,auto constants)
            ->std::optional<NativeTu15NormalizeOutcome> {
            if(row.value("deny_policy",false))return std::nullopt;
            const auto result=native_tu15_normalize_vector_30bd0_resident(m,v,o,constants);
            std::array<std::byte,4> b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(result.magnitude_bits>>(n*8));
            m.write(sp-4,b);return result;
        };
    }
    services.normalize_vector_30bd0_stats=&stats;
    std::uint32_t position=row.value("position",0x11223344u);unsigned instruction_calls{},owner_calls{};bool first_trace=false,second_trace=false;
    if(row.value("required",false))services.required_owner=[&](auto&,const auto&,auto&,const auto&) {
        ++owner_calls;throw std::runtime_error("Unexpected external normalizer owner");};
    const auto instruction=row.value("instruction",std::string{});
    const std::array<std::uint32_t,3> sites{0x38846e,0x21f590,0x33b595};
    if(!instruction.empty()) {
        services.instruction=[&](auto pc){position=pc;++instruction_calls;};
        if(instruction!="arbitrary")services.instruction_cursor={&position,&first_trace,&second_trace,
            sites,std::this_thread::get_id(),&services.instruction.target_type(),true};
        if(row.value("null_position",false))services.instruction_cursor.position=nullptr;
    }
    std::vector<PhysicsInstructionRemapTu15::Binding> remap;
    if(instruction=="remap"||instruction=="uncovered-remap") {
        underlying=services;
        for(auto pc:native_tu15_normalize_vector_sites)remap.push_back({pc,pc});
        if(instruction=="uncovered-remap")remap.pop_back();
        services.instruction=PhysicsInstructionRemapTu15{&underlying,remap};
    }
    SourcePhysicsContactRuntimeAbi after{};bool rejected{},taken{};std::uint16_t host_sw{};
    {
        physics_step_detail::FpScope environment(*entry.floating_point);
        auto execute=[&](auto &s,const auto &actual_entry) {
        s.load(actual_entry);
        if constexpr(requires{s.begin_mmx();})if(row.value("active_mmx",false))s.begin_mmx();
        first_trace=row.value("trace",false);
        memory.recording=true;memory.reject=row.value("reject_read",-1);
        const auto profile=row.value("profile",std::string{})=="tu15"?NativeTu15NormalizeProfile::Tu15Literal:
            row.value("profile",std::string{})=="mp11"?NativeTu15NormalizeProfile::Mp11Literal:NativeTu15NormalizeProfile::ImageSelected;
        try {
#ifdef HALO2_NORMALIZE_LITERAL_VARIANTS
            if(row.value("literal",false)||row.value("bound",false))
                normalize_literals::run(row.at("variant"),s,row.value("bound",false));
            else
#endif
            if(row.value("required",false)) {
                s.required(0x324a1,0x30bd0,false);taken=stats.admitted!=0;
            }else
            if(row.value("gate",false))taken=native_tu15_normalize_vector_try(s,profile);
            else native_tu15_normalize_vector_state(s,native_tu15_normalize_vector_30bd0_resident,profile);
        }
        catch(const std::runtime_error&){rejected=true;}
        asm volatile("fnstsw %0":"=m"(host_sw));
        // No pending real x87 values may survive a rejected memory read.
        after=s.save();
        };
#ifdef HALO2_NORMALIZE_LITERAL_VARIANTS
        if(row.value("variant",-1)==0) {
            SourceUnitAnimationRuntimeServices owners{};
            normalize_literals::animation::State s{memory,observe,services,owners};
            SourceUnitAnimationRuntimeAbi actual{};
            static_cast<SourcePhysicsContactRuntimeAbi&>(actual)=entry;execute(s,actual);
        }else if(row.value("variant",-1)==2) {
            physics_vehicle_motion_detail::MotionState s{memory,observe,services,image};execute(s,entry);
        }else
#endif
        {physics_world_runtime_detail::WorldState s{memory,observe,services,image};execute(s,entry);}
    }
    J banks=J::array();for(const auto &[p,b]:memory.banks)banks.push_back(J{{"address",p},{"after",hex(b)}});
    J live=J::array();for(unsigned n=0;n<after.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&after.x87[n]),10)));
    J renderer_sites=J::array();for(std::size_t i=0;i<stats.renderer_sites.size();++i){
        const auto &site=stats.renderer_sites[i];renderer_sites.push_back(J{{"pc",native_tu15_normalize_renderer_sites[i]},
            {"admitted",site.admitted},{"refused",site.refused},{"last_vector",site.last_refused_vector},
            {"last_stack",site.last_refused_stack},{"last_refusal",unsigned(site.last_refusal)},
            {"first",site.first_admitted_ordinal},{"last",site.last_admitted_ordinal}});}
    return J{{"gp",after.gp},{"xmm",after.xmm},{"eflags",after.eflags},{"fs_base",after.fs_base},
        {"fp",J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})},
        {"live_x87",live},{"banks",banks},{"reads",memory.reads},{"writes",memory.writes},
        {"rejected",rejected},{"host_x87_top",(host_sw>>11u)&7u},{"observer_events",events},
        {"gate_taken",taken},{"admitted",stats.admitted},{"refused",stats.refused},
        {"attempts",stats.attempts},
        {"renderer_admitted",stats.renderer_admitted},{"last_admitted_site",stats.last_admitted_site},
        {"renderer_sites",renderer_sites},
        {"position",position},{"instruction_calls",instruction_calls},{"owner_calls",owner_calls}};
}
}
int main()try {
    std::string line;while(std::getline(std::cin,line)){std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;}return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
