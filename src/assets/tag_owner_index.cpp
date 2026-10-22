#include "halo2/cache.hpp"
#include "assets/tag_owner_index.hpp"
#include <list>
#include <mutex>
namespace halo2 {
std::vector<const Tag*> indexed_tag_owners(const Cache &cache,TagQuery query) {
    // Import may still change directory roots. Index only a completed MCC
    // directory, using the shader-table control block as its lifetime token.
    if(!cache.mcc_ || cache.mcc_native_shaders_.empty() || !cache.function_tables_) {
        std::vector<const Tag*> found;
        for(const auto &tag:cache.select_tags(query))found.push_back(&tag);
        return found;
    }
    struct Entry {
        const Cache *owner;std::weak_ptr<const ShaderFunctionTables> lifetime;
        const Tag *roots;std::size_t size;detail::TagOwnerIndex index;
    };
    struct Bank {std::mutex mutex;std::list<Entry> entries;};
    static Bank bank;std::lock_guard lock(bank.mutex);
    for(auto at=bank.entries.begin();at!=bank.entries.end();) {
        if(at->lifetime.expired()){at=bank.entries.erase(at);continue;}
        if(at->owner==&cache && at->roots==cache.tags_.data() && at->size==cache.tags_.size() &&
           !at->lifetime.owner_before(cache.function_tables_) && !cache.function_tables_.owner_before(at->lifetime)) {
            bank.entries.splice(bank.entries.begin(),bank.entries,at);
            return bank.entries.front().index.select(query);
        }
        ++at;
    }
    bank.entries.push_front({&cache,cache.function_tables_,cache.tags_.data(),cache.tags_.size(),detail::TagOwnerIndex(cache.tags_)});
    while(bank.entries.size()>8)bank.entries.pop_back();
    return bank.entries.front().index.select(query);
}
}
