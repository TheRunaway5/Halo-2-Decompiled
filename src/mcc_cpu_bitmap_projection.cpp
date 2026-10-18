#include "halo2/mcc_cpu_bitmap_projection.hpp"
#include "mcc_cpu_bitmap_backing.hpp"
#include "halo2/source_lens_draw_tu15.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <thread>

namespace halo2::private_cpu_bitmap {
namespace {
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
unsigned word(const SourceRuntimeMemory& bank,unsigned at){return Bytes(bank.view(at,4)).u32(0);}
std::vector<std::byte> image_bytes(const SourceRuntimeCacheTagImage& image,unsigned at,std::size_t bytes){
    for(const auto& region:image.regions)if(at>=region.address &&
        std::uint64_t(at)+bytes<=std::uint64_t(region.address)+region.bytes.size()) {
        const auto view=std::span(region.bytes).subspan(at-region.address,bytes);
        return {view.begin(),view.end()};
    }
    throw FormatError("Private CPU bitmap escaped its complete canonical delivery image");
}
struct DescriptorRead final:SourcePhysicsContactMemory {
    SourceRuntimeMemory& bank;
    unsigned address,value;
    DescriptorRead(SourceRuntimeMemory& b,unsigned p,unsigned v):bank(b),address(p),value(v){}
    bool overlaps(unsigned at,std::size_t bytes)const {
        return std::uint64_t(at)<std::uint64_t(address)+4 && std::uint64_t(address)<std::uint64_t(at)+bytes;
    }
    void read(unsigned at,std::span<std::byte> out)const override {
        if(!overlaps(at,out.size())){bank.read(at,out);return;}
        require(at==address && out.size()==4,"Private CPU bitmap descriptor read differs from complete original13A030");
        for(unsigned n=0;n<4;++n)out[n]=std::byte(value>>(8*n));
    }
    void write(unsigned at,std::span<const std::byte> in)override {
        require(!overlaps(at,in.size()),"Private CPU bitmap lookup attempted to overwrite its canonical descriptor");
        bank.write(at,in);
    }
};
}
struct WorldProjection::Impl {
    struct Bank {
        Tag tag;
        unsigned root,first,count;
        std::vector<std::byte> source_root_bytes,root_bytes,rows_bytes;
        Backing::SparseRows rows;
    };
    struct Frame {Owner source;Backing::RetainedFrame retained;};
    const Cache& cache;
    SourceRuntimeMemory& bank;
    const MccRuntimeCacheDelivery& canonical;
    std::unique_ptr<Backing> backing;
    std::map<unsigned,Bank> banks;
    std::map<std::pair<unsigned,unsigned>,Frame> frames;
    // Retain every attempt before publication: a completed failed stage may
    // revoke use while exact allocation receipts still permit prior recovery.
    bool attempted=false;
    const std::thread::id producer=std::this_thread::get_id();
    void lifecycle_current(bool recovery=false)const {
        // Check the thread before reading Cache, canonical publication or bank.
        require(std::this_thread::get_id()==producer,
            "Private CPU bitmap lifecycle requires its actual producer thread");
        require(recovery || !attempted || (backing && !banks.empty() && !frames.empty()),
            "Private CPU bitmap lifecycle lacks complete attempted publication");
        for(const auto& [handle,selected]:banks) {
            current(selected);
            require(recovery || std::ranges::any_of(frames,[&](const auto& item){
                return item.first.first==handle;
            }),"Private CPU bitmap lifecycle has a bank without a complete frame");
        }
    }
    Impl(const Cache& c,SourceRuntimeMemory& b,const MccRuntimeCacheDelivery& d):cache(c),bank(b),canonical(d){}
    void current(const Bank& selected)const {
        const auto* tag=cache.query_tag({.handle=selected.tag.handle(),.group=tag_groups::bitmap,.require_address=true});
        require(tag && tag->handle()==selected.tag.handle() && tag->name==selected.tag.name &&
            tag->address==selected.tag.address && tag->size==selected.tag.size,
            "Private CPU bitmap lost its actual full-salt Cache source");
        require(std::ranges::equal(cache.read_tag_bytes(tag->address,tag->size),selected.source_root_bytes) &&
            std::ranges::equal(cache.read_tag_bytes(Bytes(selected.source_root_bytes).u32(0x48),selected.rows_bytes.size()),selected.rows_bytes),
            "Private CPU bitmap complete Cache source preimage changed");
        const auto table=canonical.index().table;
        require(word(bank,0x4f0414u)==table && (tag->handle().value&65535u)<canonical.index().count,
            "Private CPU bitmap lost its canonical directory owner");
        const auto entry=table+(tag->handle().value&65535u)*16u;
        require(word(bank,entry)==tag_groups::bitmap.value && word(bank,entry+4u)==tag->handle().value &&
            word(bank,entry+8u)==selected.root && word(bank,entry+12u)==tag->size,
            "Private CPU bitmap canonical typed root identity differs");
        require(canonical.runtime_address(tag->address,tag->size)==selected.root &&
            canonical.runtime_address(Bytes(cache.read_tag_bytes(tag->address,tag->size)).u32(0x48),selected.rows_bytes.size())==selected.first,
            "Private CPU bitmap ordinary relocation owner differs");
        require(std::ranges::equal(bank.view(selected.root,selected.root_bytes.size()),selected.root_bytes) &&
            std::ranges::equal(bank.view(selected.first,selected.rows_bytes.size()),selected.rows_bytes),
            "Private CPU bitmap canonical root or complete168-byte bank changed");
    }
    Bank qualified(TagHandle handle)const {
        require(cache.is_mcc(),"Private CPU bitmap projection requires actual MCC content");
        const auto* found=cache.query_tag({.handle=handle,.group=tag_groups::bitmap,.require_address=true});
        require(found && found->handle()==handle && found->size>=0x4cu && !found->name.empty(),
            "Private CPU bitmap full-salt typed source is absent");
        const Tag tag=*found;
        const auto source_root=cache.read_tag_bytes(tag.address,tag.size);
        const auto count=Bytes(source_root).u32(0x44),source_first=Bytes(source_root).u32(0x48);
        require(count && count<=65536 && count==cache.bitmap_count(handle.value),
            "Private CPU bitmap source count differs from its complete Cache bank");
        const auto root=canonical.runtime_address(tag.address,tag.size);
        const auto first=canonical.runtime_address(source_first,std::size_t(count)*168);
        const auto image=canonical.tag_image();
        auto expected_root=image_bytes(image,root,tag.size);
        auto expected_rows=image_bytes(image,first,std::size_t(count)*168);
        require(Bytes(expected_root).u32(0x44)==count && Bytes(expected_root).u32(0x48)==first,
            "Private CPU bitmap canonical descriptor differs from its ordinary relocation");
        require(std::ranges::equal(expected_rows,cache.read_tag_bytes(source_first,expected_rows.size())),
            "Private CPU bitmap168-byte rows differ from their complete source preimage");
        Bank result{tag,root,first,count,{source_root.begin(),source_root.end()},std::move(expected_root),std::move(expected_rows),{}};
        current(result);return result;
    }
};
WorldProjection::WorldProjection(const Cache& cache,SourceRuntimeMemory& bank,const MccRuntimeCacheDelivery& canonical)
    :impl_(std::make_unique<Impl>(cache,bank,canonical)){}
WorldProjection::~WorldProjection()=default;
bool WorldProjection::has_resources()const noexcept{return impl_->attempted;}
SourceRuntimeMemory::Checkpoint WorldProjection::checkpoint_without_resources()const {
    impl_->lifecycle_current();
    auto result=impl_->backing?impl_->backing->checkpoint_without_resources():impl_->bank.checkpoint();
    impl_->lifecycle_current();
    return result;
}
bool WorldProjection::matches_read_only_checkpoint_without_resources(
    const SourceRuntimeMemory::Checkpoint& incoming)const noexcept {
    try {
        impl_->lifecycle_current();
        SourceRuntimeMemory::validate_checkpoint(incoming);
        const bool same=impl_->backing?
            impl_->backing->matches_read_only_checkpoint_without_resources(incoming):
            impl_->bank.matches_read_only_checkpoint(incoming);
        impl_->lifecycle_current();
        return same;
    }catch(...){return false;}
}
bool WorldProjection::matches_read_only_checkpoint_for_restore(
    const SourceRuntimeMemory::Checkpoint& incoming)const noexcept {
    try {
        // Completed faults revoke use, not independently authenticated ownership.
        // World still owns the exclusive/no-active-call/no-borrow restore gate.
        impl_->lifecycle_current(true);
        SourceRuntimeMemory::validate_checkpoint(incoming);
        const bool same=impl_->backing?
            impl_->backing->matches_read_only_checkpoint_for_restore(incoming):
            impl_->bank.matches_read_only_checkpoint(incoming);
        impl_->lifecycle_current(true);
        return same;
    }catch(...){return false;}
}
unsigned WorldProjection::first_row_for_canonical_root(TagHandle bitmap,unsigned canonical_root){
    auto& state=*impl_;
    require(!state.backing || !state.backing->poisoned(),"Private inline CPU bitmap resource publication previously failed");
    auto found=state.banks.find(bitmap.value);
    auto staged=found==state.banks.end()?std::optional<Impl::Bank>(state.qualified(bitmap)):std::nullopt;
    auto& selected=staged?*staged:found->second;
    state.current(selected);
    require(selected.root==canonical_root,"Private inline CPU bitmap actual requested full salt has a different canonical root");
    // D5610 constructs through 1D2D0; only the DXT5 constructed-row path
    // is proved. Format11 remains valid for the separate direct D5506 path.
    require(Bytes(selected.rows_bytes).u16(12)==16,"Private inline CPU bitmap constructor requires proved DXT5 format16");
    const auto key=std::pair{bitmap.value,0u};
    auto known=state.frames.find(key);
    if(known==state.frames.end()){
        auto source=load(state.cache,bitmap,0u);
        require(source.count==selected.count && source.bitmap.handle()==bitmap &&
            source.bitmap.address==selected.tag.address && source.bitmap.size==selected.tag.size &&
            source.bitmap.name==selected.tag.name && std::ranges::equal(source.rows,selected.rows_bytes),
            "Private inline CPU bitmap resource differs from its actual canonical source");
        state.current(selected);
        state.attempted=true;
        if(!state.backing)state.backing=std::make_unique<Backing>(state.bank);
        if(staged){
            staged->rows=state.backing->reserve_sparse_rows(staged->count);
            found=state.banks.emplace(bitmap.value,std::move(*staged)).first;
            staged.reset();
        }
        auto retained=state.backing->stage(source,found->second.rows);
        known=state.frames.emplace(key,Impl::Frame{std::move(source),std::move(retained)}).first;
    }
    auto& active=state.banks.at(bitmap.value);
    state.current(active);
    require(active.root==canonical_root,"Private inline CPU bitmap canonical root changed");
    known->second.retained.validate(state.bank);
    (void)state.backing->refresh(known->second.source,known->second.retained);
    // Refresh writes invoke the real bank's observers; requalify the source
    // after that publication before returning this actual owned frame0 row.
    state.current(active);
    require(known->second.retained.row()==active.rows.base(),"Private inline CPU bitmap first frame differs from its sparse owner");
    return known->second.retained.row();
}
void WorldProjection::lookup(SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe){
    auto& state=*impl_;
    require(!state.backing || !state.backing->poisoned(),"Private CPU bitmap resource publication previously failed");
    const TagHandle bitmap{abi.gp[0]};
    auto found=state.banks.find(bitmap.value);
    auto staged=found==state.banks.end()?std::optional<Impl::Bank>(state.qualified(bitmap)):std::nullopt;
    auto& selected=staged?*staged:found->second;
    state.current(selected);
    const auto frame=std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[1]));
    if(frame<0 || unsigned(frame)>=selected.count){
        // Keep the actual ECX/GP state: Original13A030 owns signed-low16
        // rejection and its complete return/flags/stack behavior.
        source_run_lens_draw_tu15(state.bank,0x13a030u,abi,fp,services,observe);return;
    }
    const auto key=std::pair{bitmap.value,unsigned(frame)};
    auto known=state.frames.find(key);
    if(known==state.frames.end()){
        auto source=load(state.cache,bitmap,unsigned(frame));
        require(source.count==selected.count && source.bitmap.address==selected.tag.address &&
            source.bitmap.size==selected.tag.size && source.bitmap.name==selected.tag.name &&
            std::ranges::equal(source.rows,selected.rows_bytes),
            "Private CPU bitmap resource no longer matches its admitted canonical source");
        state.current(selected);
        state.attempted=true;
        if(!state.backing)state.backing=std::make_unique<Backing>(state.bank);
        if(staged){
            staged->rows=state.backing->reserve_sparse_rows(staged->count);
            found=state.banks.emplace(bitmap.value,std::move(*staged)).first;
            staged.reset();
        }
        auto retained=state.backing->stage(source,found->second.rows);
        known=state.frames.emplace(key,Impl::Frame{std::move(source),std::move(retained)}).first;
    }
    auto& active=state.banks.at(bitmap.value);
    state.current(active);
    known->second.retained.validate(state.bank);
    (void)state.backing->refresh(known->second.source,known->second.retained);
    // The stamp write runs the real bank's guards/observers. Admit the full
    // canonical source again after that publication, before original lookup.
    state.current(active);
    DescriptorRead memory(state.bank,active.root+0x48u,active.rows.base());
    source_run_lens_draw_tu15(memory,0x13a030u,abi,fp,services,observe);
    require(abi.gp[0]==known->second.retained.row(),
        "Original13A030 did not return its actual owned116-byte CPU row");
}
}
