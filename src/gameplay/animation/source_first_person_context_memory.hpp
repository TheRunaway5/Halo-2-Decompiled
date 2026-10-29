#pragma once
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>

namespace halo2::first_person_context_detail {
// Private byte ownership only. Unwritten input reads retain their original
// maximal contiguous runs within each 4KiB page, including fault boundaries.
class ContextMemory final:public SourcePhysicsContactMemory {
    static constexpr std::uint32_t page_bytes=4096,word_bits=64;
    struct Page {
        std::array<std::byte,page_bytes> bytes;
        std::array<std::uint64_t,page_bytes/word_bits> written{};
    };
    const SourcePhysicsContactMemory *source_{};
    std::map<std::uint32_t,Page> pages_;
    bool reference_{};
    static void extent(std::uint32_t at,std::size_t bytes) {
        if(bytes>std::uint64_t(std::numeric_limits<std::uint32_t>::max())+1u-at)
            throw FormatError("Native FP pose operand wraps its input address space");
    }
    static std::uint64_t low_mask(unsigned bits) {
        return bits==word_bits?~std::uint64_t{}:(std::uint64_t{1}<<bits)-1;
    }
    static bool marked(const Page &page,unsigned offset) {
        return (page.written[offset/word_bits]>>(offset%word_bits))&1u;
    }
    static unsigned change(const Page &page,unsigned first,unsigned end,bool value) {
        while(first<end) {
            const auto shift=first%word_bits;
            const auto count=std::min(end-first,word_bits-shift);
            const auto word=page.written[first/word_bits];
            const auto different=((value?~word:word)>>shift)&low_mask(count);
            if(different)return first+unsigned(std::countr_zero(different));
            first+=count;
        }
        return end;
    }
    static bool reference_selected() {
        // Enabled by default after bounded native fidelity/timing qualification.
        // Zero/empty or reference mode also restores allocation/copy in pinning.
        static const bool reference=[] {
            const auto *native=std::getenv("HALO2_NATIVE_FP_CONTEXT_MEMORY");
            const auto *original=std::getenv("HALO2_REFERENCE_FP_CONTEXT_MEMORY");
            return (native&&(!*native||(native[0]=='0'&&!native[1])))||(original&&*original);
        }();
        return reference;
    }
public:
    explicit ContextMemory(bool reference=reference_selected()):reference_(reference) {}
    void borrow(const SourcePhysicsContactMemory *source) {source_=source;}
    void clear() {pages_.clear();}
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        extent(at,bytes.size());
        if(!source_)throw FormatError("Native FP context has no borrowed input owner");
        std::size_t n{};
        while(n<bytes.size()) {
            const auto address=at+std::uint32_t(n),page=address/page_bytes;
            const auto offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes.size()-n,page_bytes-offset);
            const auto found=pages_.find(page);
            if(found==pages_.end()) {
                source_->read(address,bytes.subspan(n,count));n+=count;continue;
            }
            const auto &owned=found->second;
            std::size_t done{};
            while(done<count) {
                if(reference_) {
                    if(marked(owned,offset+unsigned(done))) {
                        bytes[n+done]=owned.bytes[offset+done];++done;continue;
                    }
                    const auto begin=done++;
                    while(done<count&&!marked(owned,offset+unsigned(done)))++done;
                    source_->read(address+std::uint32_t(begin),bytes.subspan(n+begin,done-begin));
                    continue;
                }
                const auto begin=offset+unsigned(done);
                const bool private_bytes=marked(owned,begin);
                const auto end=change(owned,begin,offset+unsigned(count),private_bytes);
                const auto length=end-begin;
                if(private_bytes)std::memcpy(bytes.data()+n+done,owned.bytes.data()+begin,length);
                else source_->read(address+std::uint32_t(done),bytes.subspan(n+done,length));
                done+=length;
            }
            n+=count;
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        extent(at,bytes.size());std::size_t n{};
        while(n<bytes.size()) {
            const auto address=at+std::uint32_t(n),offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes.size()-n,page_bytes-offset);
            auto &owned=pages_[address/page_bytes];
            std::memcpy(owned.bytes.data()+offset,bytes.data()+n,count);
            if(reference_) {
                for(std::size_t k=0;k<count;++k)
                    owned.written[(offset+k)/word_bits]|=std::uint64_t{1}<<((offset+k)%word_bits);
            } else {
                unsigned cursor=offset;
                const auto end=offset+unsigned(count);
                while(cursor<end) {
                    const auto shift=cursor%word_bits,length=std::min(end-cursor,word_bits-shift);
                    owned.written[cursor/word_bits]|=low_mask(length)<<shift;cursor+=length;
                }
            }
            n+=count;
        }
    }
    bool owns(std::uint32_t at)const {
        const auto page=pages_.find(at/page_bytes);
        return page!=pages_.end()&&marked(page->second,at%page_bytes);
    }
    bool fully_owned(std::uint32_t at,std::size_t bytes)const {
        extent(at,bytes);
        if(!source_)throw FormatError("Native FP context has no borrowed input owner");
        std::size_t n{};
        while(n<bytes) {
            const auto address=at+std::uint32_t(n),offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes-n,page_bytes-offset);
            const auto found=pages_.find(address/page_bytes);
            if(found==pages_.end())return false;
            unsigned cursor=offset;const auto end=offset+unsigned(count);
            while(cursor<end) {
                const auto shift=cursor%word_bits,length=std::min(end-cursor,word_bits-shift);
                const auto mask=low_mask(length)<<shift;
                if((found->second.written[cursor/word_bits]&mask)!=mask)return false;
                cursor+=length;
            }
            n+=count;
        }
        return true;
    }
    void pin_range(std::uint32_t at,std::uint32_t bytes,bool force_copy=false) {
        if(!at||!bytes)throw FormatError("Native FP cache owner has an empty native extent");
        if(!reference_&&!force_copy&&fully_owned(at,bytes))return;
        std::vector<std::byte> copy(bytes);read(at,copy);write(at,copy);
    }
    void relinquish(std::uint32_t at) {
        const auto page=pages_.find(at/page_bytes);if(page==pages_.end())return;
        page->second.written[(at%page_bytes)/word_bits]&=~(std::uint64_t{1}<<(at%word_bits));
        if(std::all_of(page->second.written.begin(),page->second.written.end(),[](auto word){return word==0;}))pages_.erase(page);
    }
    std::vector<SourceFirstPersonContextTu15::PrivateRange> ranges()const {
        std::vector<SourceFirstPersonContextTu15::PrivateRange> result;
        for(const auto &[page,owned]:pages_)for(std::uint32_t n=0;n<page_bytes;) {
            if(reference_) {
                if(!marked(owned,n)){++n;continue;}
            } else {
                n=change(owned,n,page_bytes,false);if(n==page_bytes)break;
            }
            const auto begin=n++;
            if(reference_)while(n<page_bytes&&marked(owned,n))++n;
            else n=change(owned,n,page_bytes,true);
            const auto address=page*page_bytes+begin;
            if(!result.empty()&&std::uint64_t(result.back().address)+result.back().bytes.size()==address)
                result.back().bytes.insert(result.back().bytes.end(),owned.bytes.begin()+begin,owned.bytes.begin()+n);
            else result.push_back({address,std::vector<std::byte>(owned.bytes.begin()+begin,owned.bytes.begin()+n)});
        }
        return result;
    }
    std::uint32_t word(std::uint32_t at)const {
        std::array<std::byte,4> bytes;read(at,bytes);return Bytes(bytes).u32(0);
    }
    void put(std::uint32_t at,std::uint32_t value) {write(at,std::as_bytes(std::span(&value,1)));}
};
}
