#include "halo2/mcc_physics.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::size_t maximum=128ull*1024*1024,header_size=0x110;
enum class Record { Copy, Root, NodeEdge, Region, Permutation, Curve,
    Body, Sphere, MultiSphere, Pill, Box, Triangle, Polyhedron, List, ListChild, Mopp, Phantom };
struct Block {unsigned offset,source_stride,target_stride;Record record;bool data=false;};
// Assembly fd14ef3c46135e9422966c6a64cd926fb0bd23fb Halo2MCC/Halo2 phmo;
// original TU1.5 1F2C70 / 1F31E0 establish the pre-bind runtime slots.
constexpr std::array root_blocks{
    Block{0x28,0x68,0x68,Record::Copy},Block{0x30,0x18,0x18,Record::NodeEdge},
    Block{0x38,0xa0,0x90,Record::Body},Block{0x40,0xc,0xc,Record::Copy},
    Block{0x48,0xa0,0x80,Record::Sphere},Block{0x50,0xc0,0xb0,Record::MultiSphere},
    Block{0x58,0x60,0x50,Record::Pill},Block{0x60,0xb0,0x90,Record::Box},
    Block{0x68,0x70,0x60,Record::Triangle},Block{0x70,0x120,0x100,Record::Polyhedron},
    Block{0x78,0x30,0x30,Record::Copy},Block{0x80,0x10,0x10,Record::Copy},
    Block{0x88,0x40,0x40,Record::Copy},Block{0x90,0x68,0x38,Record::List},
    Block{0x98,0x10,0x8,Record::ListChild},Block{0xa0,0x28,0x14,Record::Mopp},
    Block{0xa8,1,1,Record::Copy,true},Block{0xb0,0x78,0x78,Record::Copy},
    Block{0xb8,0x94,0x94,Record::Copy},Block{0xc0,0xc,0xc,Record::Region},
    Block{0xc8,0xc,0xc,Record::Copy},Block{0xe0,0x10,0x10,Record::Curve},
    Block{0xe8,0x84,0x84,Record::Copy},Block{0xf0,0x78,0x78,Record::Copy},
    Block{0xf8,0x7c,0x7c,Record::Copy},Block{0x100,0x84,0x84,Record::Copy},
    Block{0x108,0x40,0x20,Record::Phantom}};
constexpr std::array node_edge_blocks{Block{8,0xc,0xc,Record::Copy}};
constexpr std::array region_blocks{Block{4,0xc,0xc,Record::Permutation}};
constexpr std::array permutation_blocks{Block{4,2,2,Record::Copy}};
constexpr std::array curve_blocks{Block{8,0x10,0x10,Record::Copy}};
std::span<const Block> blocks(Record record) {
    switch(record) {
    case Record::Root:return root_blocks;
    case Record::NodeEdge:return node_edge_blocks;
    case Record::Region:return region_blocks;
    case Record::Permutation:return permutation_blocks;
    case Record::Curve:return curve_blocks;
    default:return {};
    }
}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
void pack(Record kind,std::span<const std::byte> input,std::span<std::byte> out) {
    const Bytes source(input);
    const auto copy=[&](unsigned from,unsigned to,unsigned count) {
        if(to>out.size() || count>out.size()-to)throw FormatError("Physics pack escapes its output record");
        const auto bytes=source.slice(from,count);std::copy(bytes.begin(),bytes.end(),out.begin()+to);
    };
    const auto selector=[&](unsigned from,unsigned to) {
        if(source.u32(from+4))throw FormatError("MCC physics selector is already a bound 64-bit pointer");
        copy(from,to,4);
    };
    // 1F2C70 replaces vtables/user-data/child pointers before consuming them.
    // Size is retained. The same constructor sets the reference-count flags.
    const auto shape_header=[&](unsigned from,unsigned to){copy(from+8,to+4,4);};
    if(kind==Record::Copy || kind==Record::Root || kind==Record::NodeEdge ||
       kind==Record::Region || kind==Record::Permutation || kind==Record::Curve) {
        if(input.size()!=out.size())throw FormatError("Physics copy record layout differs");
        std::copy(input.begin(),input.end(),out.begin());return;
    }
    if(kind==Record::Body) {
        copy(0,0,0x38);selector(0x38,0x38);copy(0x40,0x3c,4);copy(0x50,0x40,0x50);return;
    }
    if(kind==Record::ListChild) {selector(0,0);copy(8,4,4);return;}
    if(kind==Record::List) {
        shape_header(0,0);copy(0x20,0x10,8);
        // An external list's inline storage is not an authored child bank.
        if((source.u32(0x24)&0x7fffffffu)<=4) {
            const auto count=source.u32(0x20);
            if(count>4)throw FormatError("MCC inline physics list exceeds four child slots");
            for(unsigned i=0;i<count;++i){selector(0x28+i*16,0x18+i*8);copy(0x30+i*16,0x1c+i*8,4);}
        }
        return;
    }
    if(kind==Record::Mopp) {shape_header(0,0);selector(0x18,0xc);return;}
    if(kind==Record::Phantom) {
        shape_header(0,0);selector(0x18,0xc);copy(0x20,0x10,4);shape_header(0x28,0x14);
        // Unlike the other user-data slots, 1F2C70 explicitly preserves this
        // authored phantom value. Reject an unrepresentable value atomically.
        if(source.u32(0x3c))throw FormatError("MCC phantom user data exceeds its verified original value width");
        copy(0x38,0x1c,4);return;
    }
    copy(0,0,0x20);shape_header(0x20,0x20);copy(0x38,0x2c,4);
    switch(kind) {
    case Record::Sphere:shape_header(0x40,0x30);copy(0x60,0x40,0x40);break;
    case Record::MultiSphere:copy(0x40,0x30,0x80);break;
    case Record::Pill:copy(0x40,0x30,0x20);break;
    case Record::Box:copy(0x40,0x30,0x10);shape_header(0x50,0x40);copy(0x70,0x50,0x40);break;
    case Record::Triangle:copy(0x40,0x30,0x30);break;
    case Record::Polyhedron:
        copy(0x40,0x30,0x20);copy(0x68,0x54,8);copy(0x70,0x60,0x90);
        copy(0x100,0xf0,4);copy(0x110,0xf8,8);break;
    default:throw FormatError("Unknown MCC physics record conversion");
    }
}
}
std::vector<std::byte> normalize_mcc_physics_model(std::span<const std::byte> payload,
    std::uint32_t source_address,std::uint32_t target_address,
    const std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>& resident_read) {
    const Bytes source(payload);source.require(0,header_size);
    if(payload.size()>maximum || std::uint64_t(source_address)+payload.size()>0x100000000ull)
        throw FormatError("Excessive MCC physics source allocation");
    if(source.u32(0xd0) || source.u32(0xd8))
        throw FormatError("MCC physics import/error blocks lack a verified compiled consumer layout");
    const auto read=[&](std::uint32_t address,std::size_t size) {
        if(address>=source_address && std::uint64_t(address)-source_address<=payload.size() &&
           size<=payload.size()-(std::uint64_t(address)-source_address))
            return source.slice(address-source_address,size);
        if(resident_read)return resident_read(address,size);
        throw FormatError("MCC physics pointer escapes its resident owner");
    };
    std::vector<std::byte> result(payload.begin(),payload.begin()+header_size);
    word(result,0xd4,0);word(result,0xdc,0);
    using Key=std::tuple<std::uint32_t,std::uint32_t,unsigned,unsigned,Record,bool>;
    std::map<Key,std::size_t> copied;
    const auto relocate=[&](auto&& self,Record kind,std::uint32_t address,std::size_t output,unsigned depth)->void {
        if(depth>8 || copied.size()>1000000)throw FormatError("Excessive MCC physics pointer graph");
        for(const auto& block:blocks(kind)) {
            if(std::uint64_t(address)+block.offset+8>0x100000000ull)
                throw FormatError("MCC physics descriptor wraps its source address");
            const Bytes descriptor(read(address+block.offset,8));
            const auto count=descriptor.u32(0),pointer=descriptor.u32(4);
            if(!count){word(result,output+block.offset+4,0);continue;}
            if((!block.data && count>65536) || std::uint64_t(count)*block.source_stride>maximum ||
               std::uint64_t(pointer)+std::uint64_t(count)*block.source_stride>0x100000000ull)
                throw FormatError("Excessive MCC physics block extent");
            const Key key{pointer,count,block.source_stride,block.target_stride,block.record,block.data};
            auto found=copied.find(key);
            if(found==copied.end()) {
                const auto bytes=read(pointer,std::size_t(count)*block.source_stride);
                const auto at=(result.size()+15)&~std::size_t(15);
                const auto extent=at+std::size_t(count)*block.target_stride;
                if(extent>maximum || std::uint64_t(target_address)+extent>0x100000000ull)
                    throw FormatError("Canonical physics allocation exceeds its serialized address space");
                result.resize(extent);found=copied.emplace(key,at).first;
                if(block.record==Record::Copy) {
                    std::copy(bytes.begin(),bytes.end(),result.begin()+std::ptrdiff_t(at));
                }else for(std::size_t i=0;i<count;++i) {
                    pack(block.record,bytes.subspan(i*block.source_stride,block.source_stride),
                        std::span(result).subspan(at+i*block.target_stride,block.target_stride));
                    self(self,block.record,pointer+std::uint32_t(i*block.source_stride),at+i*block.target_stride,depth+1);
                }
            }
            word(result,output+block.offset+4,target_address+std::uint32_t(found->second));
        }
    };
    relocate(relocate,Record::Root,source_address,0,0);
    return result;
}
}
