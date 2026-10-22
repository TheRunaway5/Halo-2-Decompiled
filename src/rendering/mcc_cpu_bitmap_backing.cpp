#include "rendering/mcc_cpu_bitmap_backing.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <thread>
#include <utility>
#include <vector>

namespace halo2::private_cpu_bitmap {
namespace {
constexpr std::uint64_t low=0x80000000ull,limit=0x90000000ull,page=4096;
unsigned rounded(std::size_t bytes,unsigned alignment) {
    if(!bytes || bytes>std::numeric_limits<unsigned>::max() || !alignment || (alignment&(alignment-1)))
        throw FormatError("Private CPU bitmap allocation has invalid extent/alignment");
    const auto result=(std::uint64_t(bytes)+alignment-1)&~std::uint64_t(alignment-1);
    if(result>std::numeric_limits<unsigned>::max())throw FormatError("Private CPU bitmap allocation wraps");
    return unsigned(result);
}
unsigned hole(const SourceRuntimeMemory&bank,std::size_t bytes,unsigned alignment) {
    const auto extent=rounded(bytes,alignment);
    const auto at=bank.find_unowned_range(unsigned(low),limit,extent,alignment);
    if(!at)throw FormatError("Private CPU bitmap alias namespace exhausted");
    return *at;
}
std::array<std::byte,4> word(unsigned value) {
    return {std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
}
std::array<std::byte,20> header_bytes(const std::array<unsigned,5>&words) {
    std::array<std::byte,20> result{};
    for(unsigned n=0;n<5;++n){const auto w=word(words[n]);std::copy(w.begin(),w.end(),result.begin()+n*4);}
    return result;
}
bool same(const Owner&a,const Owner&b) {
    const auto&x=a.metadata;const auto&y=b.metadata;
    return a.bitmap.group==b.bitmap.group && a.bitmap.name==b.bitmap.name &&
        a.bitmap.datum==b.bitmap.datum && a.bitmap.address==b.bitmap.address && a.bitmap.size==b.bitmap.size &&
        a.frame==b.frame && a.count==b.count && a.source_first==b.source_first &&
        a.root==b.root && a.rows==b.rows && a.encoded==b.encoded && a.common==b.common &&
        x.width==y.width && x.height==y.height && x.format==y.format && x.mipmap_count==y.mipmap_count &&
        x.depth==y.depth && x.type==y.type && x.flags==y.flags;
}
void equal(const SourceRuntimeMemory&bank,unsigned at,std::span<const std::byte> bytes) {
    if(!std::ranges::equal(bank.view(at,bytes.size()),bytes))
        throw FormatError("Private CPU bitmap retained bytes changed");
}
}
struct Backing::Rows {
    unsigned base{},count{};
    NativeRuntimeAddress native_base;
    bool reserved{};
    std::vector<std::uint8_t> committed;
};
struct Backing::Frame {
    Owner owner;
    std::shared_ptr<Rows> rows;
    unsigned row{},encoded{},encoded_pages{},header{};
    NativeRuntimeAddress native_encoded,native_header,native_row;
    std::array<std::byte,20> header_bytes;
    std::shared_ptr<SourceRuntimeMemoryBacking> header_backing;
    unsigned cached_until{};
    bool protected_pages{};
};
struct Backing::State {
    Impl* impl{};
    bool stale{},internal{},busy{},poisoned{},swap_seen{};
    bool expected_topology{};
    unsigned expected_address{},seen_topology{};
    std::size_t expected_bytes{};
    const std::thread::id thread=std::this_thread::get_id();
};
struct Backing::Impl {
    SourceRuntimeMemory&bank;
    SourceRuntimeHostPhysicalMemory physical;
    std::shared_ptr<State> state=std::make_shared<State>();
    std::vector<std::shared_ptr<Rows>> rows;
    std::vector<std::shared_ptr<Frame>> frames;
    std::vector<std::shared_ptr<Frame>> pending;
    struct Busy {
        State&s;
        explicit Busy(State&state,bool recovery=false):s(state) {
            if(std::this_thread::get_id()!=s.thread)throw FormatError("Private CPU bitmap owner is producer-thread-only");
            if(!s.impl || s.stale || (!recovery && s.poisoned) || s.busy)throw FormatError("Private CPU bitmap owner is stale, poisoned or reentered");
            s.busy=true;
        }
        ~Busy(){s.busy=false;}
    };
    struct Internal {State&s;explicit Internal(State&v):s(v){s.internal=true;}~Internal(){s.internal=false;}};
    struct ExpectedTopology {
        State& s;
        ExpectedTopology(State& state,unsigned at,std::size_t bytes):s(state) {
            if(!s.internal || s.expected_topology)throw FormatError("CPU topology receipt operation reentered");
            s.expected_address=at;s.expected_bytes=bytes;s.seen_topology=0;s.expected_topology=true;
        }
        ~ExpectedTopology(){s.expected_topology=false;}
    };
    explicit Impl(SourceRuntimeMemory&b):bank(b),physical(b) {
        state->impl=this;
        const auto weak=std::weak_ptr<State>(state);
        source_runtime_register_topology_observer(bank,this,[weak](unsigned at,std::size_t size){
            const auto s=weak.lock();if(!s || !s->impl)return;
            if(s->internal) {
                if(!s->expected_topology || at!=s->expected_address || size!=s->expected_bytes || s->seen_topology++)s->stale=true;
                return;
            }
            if(!at && !size){s->swap_seen=true;s->stale=true;return;}
            const auto&i=*s->impl;
            const auto overlap=[&](unsigned p,std::size_t n){return (!at&&!size) || (std::uint64_t(at)<std::uint64_t(p)+n && std::uint64_t(p)<std::uint64_t(at)+size);};
            for(const auto&r:i.rows)if(overlap(r->base,std::size_t(r->count)*116)){s->stale=true;return;}
            for(const auto&f:i.frames)if(overlap(f->encoded,f->encoded_pages)||overlap(f->header,20)){s->stale=true;return;}
        });
    }
    ~Impl() {
        state->impl=nullptr;
        source_runtime_unregister_topology_observer(bank,this);
        for(auto it=frames.rbegin();it!=frames.rend();++it) {
            const auto&f=**it;
            try{if(bank.has_backing(f.header,f.header_backing))bank.release(f.header);}catch(...){}
            try{physical.release(f.encoded);}catch(...){}
        }
        // Sparse reservation identity has no public backing lease. An external
        // topology change invalidates our proof, so never release its possible
        // replacement; the enclosing bank safely owns any remaining bytes.
        if(!state->stale)for(auto it=rows.rbegin();it!=rows.rend();++it) {
            try{bank.release((*it)->base);}catch(...){}
        }
    }
    void validate(const Frame&f,bool stamp_writable=true)const {
        if(bank.native_address(f.encoded,f.owner.encoded.size())!=f.native_encoded ||
           bank.source_address(f.native_encoded,f.owner.encoded.size())!=f.encoded ||
           bank.native_address(f.header,20)!=f.native_header || bank.source_address(f.native_header,20)!=f.header ||
           bank.native_address(f.row,24)!=f.native_row || bank.source_address(f.native_row,24)!=f.row ||
           !bank.has_backing(f.header,f.header_backing) || !bank.is_read_only(f.header,20))
            throw FormatError("Private CPU bitmap actual aliases were replaced");
        equal(bank,f.encoded,f.owner.encoded);
        const auto padding_bytes=f.encoded_pages-f.owner.encoded.size();
        if(padding_bytes && std::ranges::any_of(bank.view(f.encoded+unsigned(f.owner.encoded.size()),padding_bytes),[](std::byte b){return b!=std::byte{};}))
            throw FormatError("Private CPU bitmap physical page padding changed");
        equal(bank,f.header,f.header_bytes);
        equal(bank,f.row,f.owner.common);
        const auto n=word(unsigned(f.owner.encoded.size())),h=word(f.header),e=word(f.encoded),c=word(f.cached_until);
        equal(bank,f.row+0x34,n);equal(bank,f.row+0x50,h);equal(bank,f.row+0x54,e);equal(bank,f.row+0x70,c);
        // Actual physical owner retains page protection. The region itself is
        // RW because protection is owned by SourceRuntimeHostPhysicalMemory.
        if(stamp_writable)bank.validate_mutable_view(f.row+0x70,4);
    }
};
unsigned Backing::SparseRows::base()const noexcept{return rows_?rows_->base:0;}
unsigned Backing::SparseRows::count()const noexcept{return rows_?rows_->count:0;}
unsigned Backing::RetainedFrame::row()const noexcept{return frame_?frame_->row:0;}
unsigned Backing::RetainedFrame::encoded()const noexcept{return frame_?frame_->encoded:0;}
unsigned Backing::RetainedFrame::header()const noexcept{return frame_?frame_->header:0;}
NativeRuntimeAddress Backing::RetainedFrame::native_encoded()const noexcept{return frame_?frame_->native_encoded:NativeRuntimeAddress{};}
NativeRuntimeAddress Backing::RetainedFrame::native_header()const noexcept{return frame_?frame_->native_header:NativeRuntimeAddress{};}
void Backing::RetainedFrame::validate(const SourceRuntimeMemory&bank)const {
    const auto s=state_.lock();
    if(!s || std::this_thread::get_id()!=s->thread)throw FormatError("Private CPU bitmap frame is producer-thread-only");
    if(!s->impl || !frame_ || &s->impl->bank!=&bank)throw FormatError("Private CPU bitmap frame has no current actual bank owner");
    const Impl::Busy guard(*s);s->impl->validate(*frame_);
}
Backing::Backing(SourceRuntimeMemory&bank):impl_(std::make_unique<Impl>(bank)){}
Backing::~Backing()=default;
bool Backing::poisoned()const noexcept{return impl_->state->poisoned || impl_->state->stale;}
Backing::SparseRows Backing::reserve_sparse_rows(unsigned count) {
    Impl::Busy guard(*impl_->state);
    if(!count || count>65536)throw FormatError("Private CPU bitmap row count is outside the authenticated source shape");
    auto row=std::make_shared<Rows>();row->count=count;
    impl_->rows.reserve(impl_->rows.size()+1);
    row->committed.resize(std::size_t(count)*116/4);
    row->base=hole(impl_->bank,std::size_t(count)*116,4);
    Impl::Internal mutation(*impl_->state);
    impl_->rows.push_back(row); // retain attempt before any topology callback
    try {
        Impl::ExpectedTopology expected(*impl_->state,row->base,std::size_t(count)*116);
        impl_->bank.reserve(row->base,std::size_t(count)*116,4,"private owned sparse CPU bitmap delivery rows");
        row->native_base=impl_->bank.native_address(row->base,0);row->reserved=true;
        if(impl_->state->stale)throw FormatError("CPU sparse reservation had foreign nested topology");
    }
    catch(...) { impl_->state->poisoned=true;throw; }
    SparseRows result;result.rows_=std::move(row);result.state_=impl_->state;return result;
}
Backing::RetainedFrame Backing::stage(const Owner&owner,const SparseRows&rows) {
    Impl::Busy guard(*impl_->state);
    if(rows.state_.lock()!=impl_->state || !rows.rows_ || rows.rows_->count!=owner.count || owner.frame>=owner.count || owner.frame>0x7fff || owner.encoded.empty())
        throw FormatError("Private CPU bitmap stage has foreign rows/frame ownership");
    for(const auto&f:impl_->frames)if(f->rows==rows.rows_ && f->owner.frame==owner.frame)
        throw FormatError("Private CPU bitmap frame is already staged; refresh its retained owner");
    auto f=std::make_shared<Frame>();f->owner=owner;f->rows=rows.rows_;
    const auto&captured=f->owner;
    f->row=f->rows->base+captured.frame*116;
    f->encoded_pages=rounded(captured.encoded.size(),unsigned(page));
    impl_->frames.reserve(impl_->frames.size()+1);
    impl_->pending.reserve(impl_->pending.size()+1);
    impl_->pending.push_back(f); // pending leases survive partial publication/rollback

    bool encoded=false,header=false,committed=false;
    Impl::Internal mutation(*impl_->state);
    try {
        const auto chosen=hole(impl_->bank,f->encoded_pages,unsigned(page));
        {
            Impl::ExpectedTopology expected(*impl_->state,chosen,f->encoded_pages);
            f->encoded=impl_->physical.allocate({unsigned(captured.encoded.size()),chosen&0x0fffffffu,
                unsigned(std::uint64_t(chosen&0x0fffffffu)+f->encoded_pages-1),unsigned(page),4});
        }
        if(f->encoded)encoded=true;
        if(f->encoded!=chosen)throw FormatError("Private CPU bitmap physical allocator returned a different exact token");
        f->native_encoded=impl_->bank.native_address(f->encoded,captured.encoded.size());
        impl_->bank.write(f->encoded,captured.encoded);
        if(impl_->bank.source_address(f->native_encoded,captured.encoded.size())!=chosen)
            throw FormatError("Private CPU bitmap physical alias failed actual native roundtrip");
        equal(impl_->bank,f->encoded,captured.encoded);
        impl_->physical.protect(f->encoded,f->encoded_pages,2);f->protected_pages=true;
        f->header_bytes=header_bytes(captured.header(f->encoded));
        auto bytes=std::make_shared<std::array<std::byte,20>>(f->header_bytes);
        f->header_backing=std::make_shared<SourceRuntimeMemoryBacking>(bytes,std::span(*bytes));
        f->header=hole(impl_->bank,20,4);
        {
            Impl::ExpectedTopology expected(*impl_->state,f->header,20);
            impl_->bank.mount(f->header,f->header_backing,SourceRuntimeMemory::Access::ReadOnly,"private exact CPU bitmap metadata header");header=true;
        }
        f->native_header=impl_->bank.native_address(f->header,20);
        const auto plan=delivery(captured,impl_->bank,f->native_encoded,f->native_header);
        const auto n=word(plan.encoded_bytes),h=word(plan.header),e=word(plan.encoded),c=word(plan.cached_until);
        // New rows are sparse and never re-used. SourceRuntimeMemory preserves
        // already-committed pages, so only refresh() writes an existing stamp.
        const auto commit=[&](unsigned offset,std::span<const std::byte> value) {
            Impl::ExpectedTopology expected(*impl_->state,f->row+offset,value.size());
            impl_->bank.commit(f->row+offset,value);
            std::fill_n(f->rows->committed.begin()+(std::size_t(captured.frame)*116+offset)/4,value.size()/4,1);
        };
        commit(0,plan.common);committed=true;
        commit(0x34,n);commit(0x50,h);commit(0x54,e);commit(0x70,c);
        f->cached_until=plan.cached_until;
        f->native_row=impl_->bank.native_address(f->row,24);
        impl_->validate(*f);
        if(impl_->state->stale)throw FormatError("CPU stage had foreign nested topology");
        impl_->frames.push_back(f);
        RetainedFrame result;result.frame_=std::move(f);result.state_=impl_->state;return result;
    } catch(...) {
        // An allocation may notify topology before returning/throwing; never
        // reuse this owner after any attempted stage failed, even if rollback
        // released every tracked alias. Partial sparse commits cannot undo.
        (void)committed;impl_->state->poisoned=true;
        if(header)try{if(impl_->bank.has_backing(f->header,f->header_backing)) {
            Impl::ExpectedTopology expected(*impl_->state,f->header,20);impl_->bank.release(f->header);
        }}catch(...){impl_->state->poisoned=true;}
        if(encoded)try{
            Impl::ExpectedTopology expected(*impl_->state,f->encoded,f->encoded_pages);impl_->physical.release(f->encoded);
        }catch(...){impl_->state->poisoned=true;}
        throw;
    }
}
Delivery Backing::refresh(const Owner&owner,const RetainedFrame&frame) {
    Impl::Busy guard(*impl_->state);
    if(frame.state_.lock()!=impl_->state || !frame.frame_ || !same(owner,frame.frame_->owner))
        throw FormatError("Private CPU bitmap refresh differs from its complete retained source owner");
    auto&f=*frame.frame_;impl_->validate(f);
    const auto plan=delivery(f.owner,impl_->bank,f.native_encoded,f.native_header);
    const auto stamp=word(plan.cached_until);
    try {
        impl_->bank.write(f.row+0x70,stamp);
        f.cached_until=plan.cached_until;
        impl_->validate(f);
    } catch(...) {impl_->state->poisoned=true;throw;}
    return plan;
}
SourceRuntimeMemory::Checkpoint Backing::checkpoint_without_resources() const {
    Impl::Busy guard(*impl_->state);
    // Only immutable captures kept by this actual owner identify exclusions.
    // Do not accept caller-supplied ranges, names, or numeric native aliases.
    const auto physical=impl_->physical.ownership_checkpoint();
    if(physical.version!=1 || physical.allocations.size()!=impl_->frames.size())
        throw FormatError("CPU checkpoint physical ownership is incomplete");
    for(const auto&f:impl_->frames) {
        // Match the constructor/sample domain already admitted by this owner.
        // Actual masterchief/antenna DXT3 has complete original1301E0/D5610
        // header/ABI and64-sample proof; its aliases also stay on dead stack.
        const auto format=f->owner.metadata.format;
        if(format!=10 && format!=11 && (format<14 || format>16))
            throw FormatError("CPU checkpoint source closure lacks its format proof");
        impl_->validate(*f,false);
        const auto allocation=std::ranges::find(physical.allocations,f->encoded,
            &SourceRuntimeHostPhysicalMemory::AllocationCheckpoint::base);
        if(allocation==physical.allocations.end() || allocation->bytes!=f->encoded_pages ||
            allocation->read_only.size()!=f->encoded_pages/page ||
            std::ranges::any_of(allocation->read_only,[](auto value){return value!=1;}))
            throw FormatError("CPU checkpoint physical pages/protection changed");
    }
    if(impl_->state->stale || impl_->state->poisoned)
        throw FormatError("CPU checkpoint owner changed during validation");
    auto saved=impl_->bank.checkpoint();
    SourceRuntimeMemory::validate_checkpoint(saved);
    std::vector<unsigned> exclusions;
    exclusions.reserve(impl_->rows.size()+impl_->frames.size()*2);
    const auto region=[&](unsigned base,std::size_t size,SourceRuntimeMemory::Access access,
                          const char*owner,std::size_t pages,
                          std::span<const std::byte> bytes,std::span<const std::uint8_t> committed)
                          ->const SourceRuntimeMemory::CheckpointRegion& {
        const auto at=std::ranges::find(saved.regions,base,&SourceRuntimeMemory::CheckpointRegion::address);
        if(at==saved.regions.end() || at->bytes.size()!=size || at->access!=access ||
            at->owner!=owner || at->page_bytes!=pages ||
            !std::ranges::equal(at->committed,committed) || !std::ranges::equal(at->bytes,bytes))
            throw FormatError("CPU checkpoint complete derived region differs");
        exclusions.push_back(base);return *at;
    };
    for(const auto&r:impl_->rows) {
        std::vector<std::byte> expected(std::size_t(r->count)*116);
        std::vector<std::uint8_t> committed(expected.size()/4);
        const Owner*namespace_owner{};
        for(const auto&f:impl_->frames)if(f->rows==r) {
            const auto&o=f->owner;
            if(namespace_owner && (o.bitmap.datum!=namespace_owner->bitmap.datum ||
                o.bitmap.group!=namespace_owner->bitmap.group || o.bitmap.name!=namespace_owner->bitmap.name ||
                o.bitmap.address!=namespace_owner->bitmap.address || o.bitmap.size!=namespace_owner->bitmap.size ||
                o.count!=namespace_owner->count || o.source_first!=namespace_owner->source_first ||
                o.root!=namespace_owner->root || o.rows!=namespace_owner->rows))
                throw FormatError("CPU checkpoint sparse bank mixes source namespaces");
            namespace_owner=&o;
            if(o.count!=r->count || o.frame>=r->count || f->row!=r->base+o.frame*116)
                throw FormatError("CPU checkpoint sparse ordinal ownership changed");
            const auto offset=std::size_t(o.frame)*116;
            const auto put=[&](unsigned delta,std::span<const std::byte> bytes) {
                std::copy(bytes.begin(),bytes.end(),expected.begin()+offset+delta);
                std::fill_n(committed.begin()+(offset+delta)/4,bytes.size()/4,1);
            };
            put(0,o.common);const auto n=word(unsigned(o.encoded.size())),h=word(f->header),e=word(f->encoded),c=word(f->cached_until);
            put(0x34,n);put(0x50,h);put(0x54,e);put(0x70,c);
        }
        region(r->base,expected.size(),SourceRuntimeMemory::Access::ReadWrite,
            "private owned sparse CPU bitmap delivery rows",4,expected,committed);
    }
    for(const auto&f:impl_->frames) {
        region(f->header,20,SourceRuntimeMemory::Access::ReadOnly,
            "private exact CPU bitmap metadata header",0,f->header_bytes,{});
        // Snapshot comparisons include the actual allocator's zero padding;
        // these bytes are owned backing, not published software row fields.
        std::vector<std::byte> expected(f->encoded_pages);
        std::copy(f->owner.encoded.begin(),f->owner.encoded.end(),expected.begin());
        region(f->encoded,expected.size(),SourceRuntimeMemory::Access::ReadWrite,
            "actual host physical allocation pages",0,expected,{});
    }
    std::sort(exclusions.begin(),exclusions.end());
    if(std::adjacent_find(exclusions.begin(),exclusions.end())!=exclusions.end())
        throw FormatError("CPU checkpoint derived regions alias");
    // Every guard and complete preimage passed before changing this local
    // value. No writes, releases or topology notifications touch the live bank.
    if(impl_->state->stale || impl_->state->poisoned)
        throw FormatError("CPU checkpoint owner changed before copy publication");
    std::erase_if(saved.regions,[&](const auto&r){return std::binary_search(exclusions.begin(),exclusions.end(),r.address);});
    return saved;
}
bool Backing::matches_read_only_checkpoint_without_resources(
    const SourceRuntimeMemory::Checkpoint&incoming)const noexcept {
    try {
        if(std::this_thread::get_id()!=impl_->state->thread)return false;
        SourceRuntimeMemory::validate_checkpoint(incoming);
        const auto live=checkpoint_without_resources();
        auto a=live.regions.begin(),b=incoming.regions.begin();
        for(;;) {
            while(a!=live.regions.end() && a->access!=SourceRuntimeMemory::Access::ReadOnly)++a;
            while(b!=incoming.regions.end() && b->access!=SourceRuntimeMemory::Access::ReadOnly)++b;
            if(a==live.regions.end() || b==incoming.regions.end())return a==live.regions.end() && b==incoming.regions.end();
            if(*a!=*b)return false;
            ++a;++b;
        }
    }catch(...){return false;}
}
bool Backing::swap_invalidated()const noexcept {
    const auto&s=*impl_->state;
    return std::this_thread::get_id()==s.thread && s.swap_seen && s.stale;
}

SourceRuntimeMemory::Checkpoint Backing::checkpoint_for_restore()const {
    // A completed producer failure can revoke use without revoking ownership.
    // Busy/stale/foreign-thread owners are still forbidden. Never use this
    // result for export: mutable delivery postimages may be incomplete.
    Impl::Busy guard(*impl_->state,true);
    auto saved=impl_->bank.checkpoint();
    SourceRuntimeMemory::validate_checkpoint(saved);
    const auto physical=impl_->physical.ownership_checkpoint();
    if(physical.version!=1)throw FormatError("CPU restore physical receipt version differs");
    std::vector<unsigned> exclusions;
    const auto find=[&](unsigned at) {
        return std::ranges::find(saved.regions,at,&SourceRuntimeMemory::CheckpointRegion::address);
    };
    const auto region=[&](unsigned at,std::size_t bytes,SourceRuntimeMemory::Access access,
                          const char* owner,std::size_t pages,
                          std::span<const std::uint8_t> committed) {
        const auto got=find(at);
        if(got==saved.regions.end() || got->bytes.size()!=bytes || got->access!=access ||
            got->owner!=owner || got->page_bytes!=pages || !std::ranges::equal(got->committed,committed))
            throw FormatError("CPU restore complete derived topology differs");
        exclusions.push_back(at);
    };
    for(const auto&rows:impl_->rows) {
        if(!rows->reserved) {
            if(find(rows->base)!=saved.regions.end())
                throw FormatError("CPU restore reservation attempt lacks its actual native receipt");
            continue;
        }
        if(impl_->bank.native_address(rows->base,0)!=rows->native_base ||
            impl_->bank.source_address(rows->native_base,0)!=rows->base)
            throw FormatError("CPU restore sparse native owner was replaced");
        region(rows->base,std::size_t(rows->count)*116,SourceRuntimeMemory::Access::ReadWrite,
            "private owned sparse CPU bitmap delivery rows",4,rows->committed);
    }
    for(const auto&allocation:physical.allocations) {
        const auto frame=std::ranges::find_if(impl_->pending,[&](const auto&f){return f->encoded==allocation.base;});
        if(frame==impl_->pending.end())throw FormatError("CPU restore physical owner has no pending receipt");
        const auto&f=**frame;
        if(!f.native_encoded.value || allocation.bytes!=f.encoded_pages ||
            allocation.read_only.size()!=f.encoded_pages/page ||
            std::ranges::any_of(allocation.read_only,[&](auto value){return value!=unsigned(f.protected_pages);}) ||
            impl_->bank.native_address(f.encoded,f.owner.encoded.size())!=f.native_encoded ||
            impl_->bank.source_address(f.native_encoded,f.owner.encoded.size())!=f.encoded)
            throw FormatError("CPU restore physical lease/protection differs");
        region(f.encoded,f.encoded_pages,SourceRuntimeMemory::Access::ReadWrite,
            "actual host physical allocation pages",0,{});
    }
    for(const auto&f:impl_->pending) {
        if(f->encoded && std::ranges::find(physical.allocations,f->encoded,
            &SourceRuntimeHostPhysicalMemory::AllocationCheckpoint::base)==physical.allocations.end() &&
            find(f->encoded)!=saved.regions.end())
            throw FormatError("CPU restore encoded region lost its physical owner");
        if(!impl_->bank.has_backing(f->header,f->header_backing))continue;
        if(!f->native_header.value || impl_->bank.native_address(f->header,20)!=f->native_header ||
            impl_->bank.source_address(f->native_header,20)!=f->header)
            throw FormatError("CPU restore immutable header lease differs");
        // Header bytes are immutable and source-derived. Only mutable rows
        // and encoded allocations are permitted an unusable postimage here.
        equal(impl_->bank,f->header,f->header_bytes);
        region(f->header,20,SourceRuntimeMemory::Access::ReadOnly,
            "private exact CPU bitmap metadata header",0,{});
    }
    std::sort(exclusions.begin(),exclusions.end());
    if(std::adjacent_find(exclusions.begin(),exclusions.end())!=exclusions.end() || impl_->state->stale)
        throw FormatError("CPU restore derived receipts alias or changed");
    std::erase_if(saved.regions,[&](const auto&r){return std::binary_search(exclusions.begin(),exclusions.end(),r.address);});
    return saved;
}
bool Backing::matches_read_only_checkpoint_for_restore(
    const SourceRuntimeMemory::Checkpoint&incoming)const noexcept {
    try {
        SourceRuntimeMemory::validate_checkpoint(incoming);
        const auto live=checkpoint_for_restore();
        auto a=live.regions.begin(),b=incoming.regions.begin();
        for(;;) {
            while(a!=live.regions.end() && a->access!=SourceRuntimeMemory::Access::ReadOnly)++a;
            while(b!=incoming.regions.end() && b->access!=SourceRuntimeMemory::Access::ReadOnly)++b;
            if(a==live.regions.end() || b==incoming.regions.end())return a==live.regions.end()&&b==incoming.regions.end();
            if(*a!=*b)return false;
            ++a;++b;
        }
    }catch(...){return false;}
}

}
