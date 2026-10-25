#include "halo2/source_announcer_events_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>

namespace halo2 {
SourceAnnouncerResponsesTu15 load_source_announcer_responses_tu15(const Cache& cache) {
    if(!cache.is_mcc())throw FormatError("Xbox announcer runtime requires imported MCC resource owners");
    const Tag* globals=nullptr;
    for(const auto& tag:cache.select_tags({.group=tag_groups::globals,.require_address=true})) {
        if(globals)throw FormatError("Xbox announcer has ambiguous selected globals");globals=&tag;
    }
    if(!globals)throw FormatError("Xbox announcer has no selected globals");
    const auto matg=cache.load_source_tag_metadata(globals->datum);
    const auto globals_header=Bytes(matg.bytes_at(matg.source_address(),0x170));
    const auto mulg=cache.load_source_tag_metadata(globals_header.u32(0x16c));
    if(mulg.group()!="mulg")throw FormatError("Xbox announcer runtime globals have wrong group");
    const auto header=Bytes(mulg.bytes_at(mulg.source_address(),16));
    if(header.u32(8)!=1)throw FormatError("Xbox announcer has no unique runtime block");
    const auto runtime=Bytes(mulg.bytes_at(header.u32(12),0xf0));
    // Original1A7A30 jump table. Types5/7 have no response bank.
    constexpr std::array<unsigned,11> offsets{0x98,0xa0,0xa8,0xb0,0xb8,0,0xc8,0,0xd8,0xe0,0xe8};
    SourceAnnouncerResponsesTu15 result;
    for(unsigned type=0;type<offsets.size();++type)if(const auto at=offsets[type]) {
        const auto count=runtime.u32(at);
        if(count>4096)throw FormatError("Excessive Xbox announcer response count");
        if(!count)continue;
        const auto bytes=mulg.bytes_at(runtime.u32(at+4),std::size_t(count)*0xa8);
        auto& bank=result.banks[type];bank.definitions.resize(count);bank.alternatives.resize(count);
        for(unsigned n=0;n<count;++n) {
            auto& definition=bank.definitions[n];
            std::copy_n(bytes.begin()+n*0xa8,0xa8,definition.begin());const Bytes row(definition);
            const auto alternatives=row.u32(0xa0);
            if(alternatives>4096)throw FormatError("Excessive Xbox announcer alternatives");
            if(!alternatives)continue;
            const auto raw=mulg.bytes_at(row.u32(0xa4),std::size_t(alternatives)*0x50);
            for(unsigned i=0;i<alternatives;++i) {
                const Bytes choice(raw.subspan(i*0x50,0x50));SourceAnnouncerAlternativeTu15 value;
                value.flags=std::uint8_t(choice.u16(0));
                for(unsigned language=0;language<9;++language)value.sounds[language]=choice.u32(8+language*8);
                value.weight=choice.f32(0x4c);bank.alternatives[n].push_back(value);
            }
        }
    }
    return result;
}
} // namespace halo2
