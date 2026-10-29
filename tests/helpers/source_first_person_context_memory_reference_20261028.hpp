#pragma once
// Literal pre-optimization ContextMemory class; only its enclosing namespace differs.
// Class source SHA-256: babc4557bd64d1912bd15f066ae5c4d8a97382d434976fd87fa017b0dcb7d9dc
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bitset>
#include <cstring>
#include <limits>
#include <map>
namespace halo2::first_person_context_reference {
class ContextMemory final:public SourcePhysicsContactMemory {
    static constexpr std::uint32_t page_bytes=4096;
    struct Page {std::array<std::byte,page_bytes> bytes;std::bitset<page_bytes> written;};
    const SourcePhysicsContactMemory *source_{};
    std::map<std::uint32_t,Page> pages_;
    static void extent(std::uint32_t at,std::size_t bytes) {
        if(bytes>std::uint64_t(std::numeric_limits<std::uint32_t>::max())+1u-at)
            throw FormatError("Native FP pose operand wraps its input address space");
    }
public:
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
                source_->read(address,bytes.subspan(n,count));
                n+=count;continue;
            }
            const auto &owned=found->second;
            std::size_t done{};
            while(done<count) {
                if(owned.written[offset+done]) {
                    bytes[n+done]=owned.bytes[offset+done];++done;continue;
                }
                const auto begin=done++;
                while(done<count && !owned.written[offset+done])++done;
                source_->read(address+std::uint32_t(begin),bytes.subspan(n+begin,done-begin));
            }
            n+=count;
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        extent(at,bytes.size());
        std::size_t n{};
        while(n<bytes.size()) {
            const auto address=at+std::uint32_t(n),offset=address%page_bytes;
            const auto count=std::min<std::size_t>(bytes.size()-n,page_bytes-offset);
            auto &owned=pages_[address/page_bytes];
            std::memcpy(owned.bytes.data()+offset,bytes.data()+n,count);
            for(std::size_t k=0;k<count;++k)owned.written.set(offset+k);
            n+=count;
        }
    }
    bool owns(std::uint32_t at)const {
        const auto page=pages_.find(at/page_bytes);
        return page!=pages_.end() && page->second.written[at%page_bytes];
    }
    void relinquish(std::uint32_t at) {
        const auto page=pages_.find(at/page_bytes);
        if(page==pages_.end())return;
        page->second.written.reset(at%page_bytes);
        if(page->second.written.none())pages_.erase(page);
    }
    std::vector<SourceFirstPersonContextTu15::PrivateRange> ranges()const {
        std::vector<SourceFirstPersonContextTu15::PrivateRange> result;
        for(const auto &[page,owned]:pages_)for(std::uint32_t n=0;n<page_bytes;) {
            if(!owned.written[n]){++n;continue;}
            const auto begin=n++;while(n<page_bytes && owned.written[n])++n;
            const auto address=page*page_bytes+begin;
            if(!result.empty() && std::uint64_t(result.back().address)+result.back().bytes.size()==address)
                result.back().bytes.insert(result.back().bytes.end(),owned.bytes.begin()+begin,owned.bytes.begin()+n);
            else result.push_back({address,std::vector<std::byte>(owned.bytes.begin()+begin,owned.bytes.begin()+n)});
        }
        return result;
    }
    std::uint32_t word(std::uint32_t at)const {
        std::array<std::byte,4> bytes;read(at,bytes);return Bytes(bytes).u32(0);
    }
    void put(std::uint32_t at,std::uint32_t value) {
        write(at,std::as_bytes(std::span(&value,1)));
    }
};
}
