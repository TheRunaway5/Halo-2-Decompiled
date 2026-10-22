#include "halo2/mcc_ordinary_pointer.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <string_view>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::uint64_t address_end=0x100000000ull;
constexpr std::uint64_t maximum_bytes=128ull*1024*1024;
enum class OrdinaryKind { Block, Data, Runtime64, Runtime32, Unknown32, LocaleRows, LocaleData };
struct OrdinaryNode {unsigned first,count,extent;};
struct OrdinaryField {unsigned offset,stride,child;OrdinaryKind kind;const char* name;};
struct OrdinarySchema {std::string_view group;MccOrdinaryPointerFlavor flavor;unsigned root;};
#include "assets/mcc/mcc_ordinary_pointer_layout.inc"
std::uint32_t add(std::uint32_t address,std::uint64_t offset) {
    if(offset>=address_end || std::uint64_t(address)+offset>=address_end)
        throw FormatError("Ordinary metadata field address wraps");
    return std::uint32_t(std::uint64_t(address)+offset);
}
}

std::optional<std::vector<MccOrdinaryPointerField>> ordinary_mcc_pointer_fields(
    TagGroup group,std::uint32_t root,const MccOrdinaryPointerRead& read,MccOrdinaryPointerFlavor flavor,
    const MccOrdinaryClosedNativeRoot& closed_native_root) {
    if(flavor!=MccOrdinaryPointerFlavor::Mcc && flavor!=MccOrdinaryPointerFlavor::Xbox)
        throw FormatError("Unknown ordinary metadata schema flavor");
    const auto name=group.string();
    const auto schema=std::ranges::find_if(ordinary_schemas,[&](const auto& candidate) {
        return candidate.group==name && candidate.flavor==flavor;
    });
    if(schema==ordinary_schemas.end())return std::nullopt;
    if(!root || root==0xffffffffu || !read)throw FormatError("Ordinary metadata root owner is missing");
    std::vector<MccOrdinaryPointerField> result;
    std::set<std::pair<unsigned,std::uint32_t>> visited;
    const auto checked_read=[&](std::uint32_t address,std::size_t size) {
        if(!address || address==0xffffffffu || size>maximum_bytes || size>address_end-address)
            throw FormatError("Ordinary metadata reference is null, excessive, or wraps");
        std::span<const std::byte> bytes;
        try{bytes=read(address,size);}
        catch(const FormatError& e){throw FormatError("Ordinary metadata read at "+std::to_string(address)+
            " for "+std::to_string(size)+" bytes: "+e.what());}
        if(bytes.size()!=size)throw FormatError("Ordinary metadata owner returned a different extent");
        return bytes;
    };
    auto walk=[&](auto&& self,unsigned node,std::uint32_t address,unsigned depth,unsigned record_size)->void {
        if(depth>64 || visited.size()>=1000000 || result.size()>=2000000)
            throw FormatError("Excessive ordinary metadata pointer graph");
        if(!visited.emplace(node,address).second)return;
        const auto layout=ordinary_nodes.at(node);
        if(record_size && layout.extent>record_size)
            throw FormatError("Pinned ordinary metadata fields exceed their record stride");
        if(layout.extent)(void)checked_read(address,layout.extent);
        for(unsigned i=0;i<layout.count;++i) {
            const auto& field=ordinary_fields.at(layout.first+i);
            const auto at=add(address,field.offset);
            const auto failure=[&](const char* reason) {
                throw FormatError("Ordinary "+name+" field "+field.name+": "+reason);
            };
            if(field.kind==OrdinaryKind::Runtime64 || field.kind==OrdinaryKind::Runtime32 || field.kind==OrdinaryKind::Unknown32) {
                const Bytes value(checked_read(at,field.kind==OrdinaryKind::Runtime64?8:4));
                if(value.u32(0) || (field.kind==OrdinaryKind::Runtime64 && value.u32(4)))
                    failure(field.kind==OrdinaryKind::Unknown32?"nonempty owner has no declared typed schema":
                        "already-bound runtime slot requires a genuine native rebinder");
                continue;
            }
            const bool locale=field.kind==OrdinaryKind::LocaleRows || field.kind==OrdinaryKind::LocaleData;
            const Bytes descriptor(checked_read(at,locale?12:8));
            const auto count=descriptor.u32(locale?8:0),pointer=descriptor.u32(locale?0:4);
            // Tagblock counts are signed 32-bit values, not tag datums. In
            // particular, campaign BSP surface-reference banks legitimately
            // exceed 65,536 records. Bound by declared bytes and traversal
            // budget rather than imposing a 16-bit handle limit on arrays.
            if((field.kind==OrdinaryKind::Block && count>0x7fffffffu) ||
               (field.kind==OrdinaryKind::LocaleRows && count>65536))
                failure("record count exceeds its declared format");
            if(count && !field.stride)failure("nonempty zero-stride bank has no declared typed schema");
            const auto extent=std::uint64_t(count)*field.stride;
            if(extent>maximum_bytes || extent>std::numeric_limits<std::uint32_t>::max())
                failure("bank extent is excessive");
            if(!pointer || pointer==0xffffffffu) {
                // Locale counts/sizes describe logical file data even while
                // the runtime pointers are unloaded. The existing locale
                // reader uses separate Cache Offset fields in that state.
                // This visitor does not admit locale activation or interpret
                // runtime flag/padding words as metadata descriptors.
                if(count && !locale)failure("nonempty bank has no metadata owner");
                continue;
            }
            // Even a stale pointer in a zero-count descriptor must have an
            // explicit relocation owner. Leaving it unchanged would publish a
            // donor address in the canonical vanilla bank.
            if(extent)(void)checked_read(pointer,std::size_t(extent));
            result.push_back({add(at,locale?0:4),pointer,std::uint32_t(extent)});
            if(field.kind==OrdinaryKind::Block && count && ordinary_nodes.at(field.child).count)
                for(std::uint32_t n=0;n<count;++n) {
                    const auto child_address=add(pointer,std::uint64_t(n)*field.stride);
                    if(node==schema->root && group==tag_groups::globals && field.offset==0x108 &&
                       closed_native_root && closed_native_root(child_address,field.stride))continue;
                    self(self,field.child,child_address,depth+1,field.stride);
                }
        }
    };
    walk(walk,schema->root,root,0,0);
    std::ranges::sort(result,{},&MccOrdinaryPointerField::field_address);
    for(std::size_t n=1;n<result.size();++n)
        if(result[n].field_address==result[n-1].field_address &&
           (result[n].address!=result[n-1].address || result[n].bytes!=result[n-1].bytes))
            throw FormatError("Ordinary metadata aliases disagree about the same pointer field");
    const auto duplicate=std::ranges::unique(result,{},&MccOrdinaryPointerField::field_address);
    result.erase(duplicate.begin(),duplicate.end());
    return result;
}
} // namespace halo2
