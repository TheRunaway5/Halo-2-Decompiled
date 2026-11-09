#pragma once
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include <algorithm>
#include <stdexcept>
namespace halo2 {
struct NativeTu15ContactPrivateRange {std::uint32_t first{},bytes{};};
// Owned call-local DATA. Bytes have no value until a real semantic producer
// writes them. They are never borrowed from original stack/GP/control images.
// All native providers must use THIS SAME memory view. Raw private preimages
// and private address-fault equivalence are explicitly outside this contract.
class NativeTu15ContactCandidatePrivateData final:public SourcePhysicsContactMemory,
    public NativeTu15RetainedStoreObservationDomain, public NativeTu15RetainedCounterStorage {
    struct Range {NativeTu15ContactPrivateRange extent;std::vector<std::byte>bytes;
        std::vector<bool>defined;};
    SourcePhysicsContactMemory&outer_;std::vector<Range>ranges_;
    const Range*range(std::uint32_t p,std::size_t n)const{
        if(n>0x100000000ull-std::uint64_t(p))throw std::runtime_error("Native private DATA wraps");
        const auto end=std::uint64_t(p)+n;
        for(const auto&r:ranges_){const std::uint64_t first=r.extent.first,last=first+r.extent.bytes;
            if(std::uint64_t(p)<last&&std::uint64_t(first)<end){
                if(p<first||end>last)throw std::runtime_error("Native private DATA crossing operand");
                return &r;
            }}return nullptr;
    }
    static bool retained_publication(std::uint32_t pc){
        // Authenticated2F parent heap/owner fields, never its local packet.
        switch(pc){case 0x2f37e3:case 0x2f37e7:case 0x2f38c1:
        case 0x2f38f9:case 0x2f38fe:case 0x2f3904:case 0x2f3a08:
        case 0x2f3a0d:case 0x2f3a13:case 0x2f3a32:case 0x2f3a9b:
        case 0x2f3aa0:case 0x2f3d4a:case 0x2f3d4d:case 0x2f3d55:
        case 0x2f3d66:case 0x2f3d6b:case 0x2f3d78:return true;default:break;}
        switch(pc){case 0x2f3b9d:case 0x2f3ba2:case 0x2f3bae:
        case 0x2f3c1f:case 0x2f3c36:case 0x2f3c24:case 0x2f3c3b:
        case 0x2f3ce7:case 0x2f3cf0:return true;default:break;}
        // Convex-agent allocation/constructor and actual allocator metadata
        // are retained heap stores; they may never become hidden local DATA.
        if((pc>=0x233250u&&pc<0x233500u)||(pc>=0x2e4eb0u&&pc<0x2e4f20u)||
            (pc>=0x2f8300u&&pc<0x2f8400u))return true;
        // Cache header/tag publications target the retained agent, unlike
        // its separately owned vector workspace.
        switch(pc){case 0x3090e0:case 0x309117:case 0x309120:case 0x309124:
        case 0x30912a:case 0x30912d:case 0x309137:case 0x309142:
        case 0x309162:case 0x309168:case 0x30916e:case 0x309272:
        case 0x309275:case 0x309279:case 0x309285:case 0x309288:return true;
        default:return false;}
    }
public:
    // One operation owns this definition mask. Nested views wrap SourceMemory&;
    // implicit copy/move must never fork the lifetime metadata.
    NativeTu15ContactCandidatePrivateData(const NativeTu15ContactCandidatePrivateData&)=delete;
    NativeTu15ContactCandidatePrivateData&operator=(const NativeTu15ContactCandidatePrivateData&)=delete;
    NativeTu15ContactCandidatePrivateData(NativeTu15ContactCandidatePrivateData&&)=delete;
    NativeTu15ContactCandidatePrivateData&operator=(NativeTu15ContactCandidatePrivateData&&)=delete;
    NativeTu15ContactCandidatePrivateData(SourcePhysicsContactMemory&outer,
        std::span<const NativeTu15ContactPrivateRange>owned,
        std::span<const NativeTu15ContactPrivateRange>retained):outer_(outer){
        std::uint64_t total{};
        for(const auto&e:owned){const auto end=std::uint64_t(e.first)+e.bytes;
            if(!e.bytes||end>0x100000000ull||(total+=e.bytes)>64u*1024u)
                throw std::runtime_error("Native private DATA requires bounded owned ranges");
            for(const auto&r:ranges_)if(std::uint64_t(e.first)<std::uint64_t(r.extent.first)+r.extent.bytes&&std::uint64_t(r.extent.first)<end)
                throw std::runtime_error("Native private DATA ranges overlap");
            for(const auto&r:retained)if(std::uint64_t(e.first)<std::uint64_t(r.first)+r.bytes&&std::uint64_t(r.first)<end)
                throw std::runtime_error("Native private DATA aliases retained input/output");
            ranges_.push_back({e,std::vector<std::byte>(e.bytes),std::vector<bool>(e.bytes,false)});
        }
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(out.empty())return;
        if(const auto*r=range(p,out.size())){const auto at=p-r->extent.first;
            for(std::size_t i=0;i<out.size();++i)if(!r->defined[at+i])
                throw std::runtime_error("Native candidate requires unprovided private DATA at="+std::to_string(p)+" bytes="+std::to_string(out.size()));
            std::copy_n(r->bytes.begin()+at,out.size(),out.begin());return;}
        outer_.read(p,out);
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{
        if(range(p,16)){read(p,out);return;}outer_.read_vector(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override{
        if(in.empty())return;
        if(const auto*found=range(p,in.size())){auto&r=*const_cast<Range*>(found);const auto at=p-r.extent.first;
            std::copy(in.begin(),in.end(),r.bytes.begin()+at);std::fill_n(r.defined.begin()+at,in.size(),true);return;}
        outer_.write(p,in);
    }
    bool observes_retained_store(std::uint32_t pc,std::uint32_t p,std::size_t n)const override{
        if(range(p,n)){if(retained_publication(pc))throw std::runtime_error("Native retained publication aliases private DATA");return false;}
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        return !domain||domain->observes_retained_store(pc,p,n);
    }
    void require_retained_extent(std::uint32_t p,std::size_t n)const override{
        if(n&&range(p,n))throw std::runtime_error("Native allocator output aliases private DATA");
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        if(domain)domain->require_retained_extent(p,n);
    }
    NativeTu15RetainedCounterUpdate adjust_counter(std::uint32_t p, unsigned width, std::uint32_t delta) override {
        require_retained_extent(p,width);
        auto *counter=dynamic_cast<NativeTu15RetainedCounterStorage*>(&outer_);
        if(!counter)throw std::runtime_error("Native retained counter has no checked outer storage");
        return counter->adjust_counter(p,width,delta);
    }
    bool withhold_defined_data(std::uint32_t p,std::size_t n)override{
        if(!n)return true;
        if(const auto*found=range(p,n)){auto&r=*const_cast<Range*>(found);
            std::fill_n(r.defined.begin()+(p-r.extent.first),n,false);return true;}
        auto*domain=dynamic_cast<NativeTu15RetainedStoreObservationDomain*>(&outer_);
        if(domain&&domain->withhold_defined_data(p,n))return true;
        throw std::runtime_error("Native private lifetime cannot invalidate retained DATA");
    }
    bool data_is_defined(std::uint32_t p,std::size_t n)const override{
        if(!n)return true;
        if(const auto*r=range(p,n)){const auto at=p-r->extent.first;
            return std::all_of(r->defined.begin()+at,r->defined.begin()+at+n,[](bool x){return x;});}
        const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&outer_);
        return !domain||domain->data_is_defined(p,n);
    }
    bool copy_defined_data(std::uint32_t pc,std::uint32_t destination,
        std::uint32_t source,std::size_t n,const SourcePhysicsContactRecordObserver&o={})override{
        if(!n||n>16u||(n&(n-1u)))throw std::runtime_error("Native defined DATA copy requires its actual scalar/vector operand granularity");
        const auto*input=range(source,n);const auto*output=range(destination,n);
        if(!input&&!output){auto*domain=dynamic_cast<NativeTu15RetainedStoreObservationDomain*>(&outer_);
            return domain&&domain->copy_defined_data(pc,destination,source,n,o);}
        // Validate retained-publication aliasing before touching any bytes.
        const bool observed=bool(o)&&observes_retained_store(pc,destination,n);
        if(!o)(void)observes_retained_store(pc,destination,n);
        std::array<std::byte,16>values;std::array<bool,16>defined{};
        if(input){const auto at=source-input->extent.first;for(std::size_t i=0;i<n;++i){
            defined[i]=input->defined[at+i];if(defined[i])values[i]=input->bytes[at+i];}}
        else{if(n==16u)outer_.read_vector(source,std::span<std::byte,16>(values));
            else outer_.read(source,std::span(values).first(n));std::fill_n(defined.begin(),n,true);}
        if(output){auto&r=*const_cast<Range*>(output);const auto at=destination-r.extent.first;
            for(std::size_t i=0;i<n;++i){r.defined[at+i]=defined[i];if(defined[i])r.bytes[at+i]=values[i];}return true;}
        if(!std::all_of(defined.begin(),defined.begin()+n,[](bool x){return x;}))
            throw std::runtime_error("Native undefined DATA cannot escape to retained storage");
        SourcePhysicsContactRecordStore event{pc,destination,{},{}};
        if(observed){event.before.resize(n);outer_.read(destination,event.before);event.written.assign(values.begin(),values.begin()+n);}
        outer_.write(destination,std::span(values).first(n));if(observed)o(event);return true;
    }
};
} // namespace halo2
