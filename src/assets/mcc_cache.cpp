#include "halo2/mcc_cache.hpp"
#include "halo2/mcc_animation.hpp"
#include "halo2/mcc_physics.hpp"
#include "halo2/mcc_gameplay_scalar.hpp"
#include "halo2/mcc_gameplay_reference.hpp"
#include "halo2/mcc_map_gameplay_vanilla.hpp"
#include "halo2/mcc_bsp_collision_reference.hpp"
#include "halo2/mcc_bsp_instance_bounds.hpp"
#include "halo2/mcc_hud_reference.hpp"
#include "halo2/mcc_gameplay_unit_scalar.hpp"
#include "halo2/mcc_destruction_model_scalar.hpp"
#include "halo2/mcc_destruction_model_contract.hpp"
#include "halo2/mcc_destruction_model_reference.hpp"
#include "halo2/mcc_cairo_vanilla.hpp"
#include "halo2/mcc_campaign_vanilla.hpp"
#include "halo2/cache.hpp"
#include "halo2/mcc_prepared_map_owner.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_balance.hpp"
#include "halo2/texture.hpp"
#include "tags/directory.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <zlib.h>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

namespace halo2 {
namespace {
// A content key includes every stored byte, so identical partitions may
// share decode work across files while edits and malformed inputs stay visible.
// This owns no Cache/Texture ABI state and never changes returned value ownership.
class TexturePartitionResidency {
    struct Entry {
        std::size_t hash;
        std::vector<std::byte> stored;
        std::shared_ptr<const std::vector<std::byte>> decoded;
    };
    static constexpr std::size_t budget=128ull*1024*1024,maximum_entries=512;
    std::mutex mutex_;
    std::list<Entry> entries_;
    std::size_t bytes_{};
    static std::size_t hash(std::span<const std::byte> stored) {
        return std::hash<std::string_view>{}({reinterpret_cast<const char*>(stored.data()),stored.size()});
    }
public:
    std::shared_ptr<const std::vector<std::byte>> find(std::span<const std::byte> stored) {
        const auto key=hash(stored);std::lock_guard lock(mutex_);
        for(auto it=entries_.begin();it!=entries_.end();++it)
            if(it->hash==key && std::ranges::equal(it->stored,stored)) {
                auto result=it->decoded;entries_.splice(entries_.begin(),entries_,it);return result;
            }
        return {};
    }
    void admit(std::span<const std::byte> stored,const std::vector<std::byte>& decoded) {
        if(decoded.size()>budget || stored.size()>budget-decoded.size())return;
        // Residency is optional; a successful decode remains usable if its
        // additional cache allocation cannot be made.
        try {
            Entry entry{hash(stored),{stored.begin(),stored.end()},
                std::make_shared<const std::vector<std::byte>>(decoded)};
            const auto size=entry.stored.size()+entry.decoded->size();
            std::lock_guard lock(mutex_);
            for(const auto& current:entries_)
                if(current.hash==entry.hash && current.stored==entry.stored)return;
            // Allocate before eviction so allocation failure preserves residency.
            entries_.push_front(std::move(entry));bytes_+=size;
            while(bytes_>budget || entries_.size()>maximum_entries) {
                const auto& last=entries_.back();bytes_-=last.stored.size()+last.decoded->size();entries_.pop_back();
            }
        } catch(const std::bad_alloc&) {}
    }
};
constexpr std::size_t header_size=896,maximum_cache=1024ull*1024*1024;
std::vector<std::byte> read_range(const std::filesystem::path& path,std::uint64_t at,
    std::uint64_t count=0,bool whole=false) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<0)throw FormatError("Cannot read MCC data: "+path.string());
    const auto size=std::uint64_t(file.tellg());
    if(whole)count=size;
    if(count>maximum_cache || at>size || count>size-at)throw FormatError("MCC file extent is invalid or excessive");
    std::vector<std::byte> result(static_cast<std::size_t>(count));
    file.seekg(std::streamoff(at));
    if(count && !file.read(reinterpret_cast<char*>(result.data()),std::streamsize(count)))
        throw FormatError("Short MCC data read");
    return result;
}
std::vector<std::byte> inflate(std::span<const std::byte> input,std::size_t maximum) {
    if(!maximum || maximum>maximum_cache || input.size()>maximum_cache)
        throw FormatError("Invalid MCC zlib bound");
    std::vector<std::byte> result(std::min<std::size_t>(maximum,65536));
    z_stream stream{};
    stream.next_in=reinterpret_cast<Bytef*>(const_cast<std::byte*>(input.data()));
    stream.avail_in=uInt(input.size());
    if(inflateInit(&stream)!=Z_OK)throw FormatError("MCC zlib initialization failed");
    struct End {z_stream* stream;~End(){inflateEnd(stream);}} end{&stream};
    for(;;) {
        if(stream.total_out==result.size()) {
            if(result.size()==maximum) {
                // A stream can consume its trailer after filling the exact
                // declared output extent; one extra byte detects a bomb.
                result.resize(maximum+1);
            } else result.resize(std::min(maximum,result.size()*2));
        }
        stream.next_out=reinterpret_cast<Bytef*>(result.data()+stream.total_out);
        stream.avail_out=uInt(result.size()-stream.total_out);
        const auto status=::inflate(&stream,Z_NO_FLUSH);
        if(stream.total_out>maximum)throw FormatError("MCC zlib output exceeds declared extent");
        if(status==Z_STREAM_END) {
            if(stream.avail_in || !stream.total_out)throw FormatError("Trailing or empty MCC zlib stream");
            result.resize(stream.total_out);return result;
        }
        if(status!=Z_OK || (!stream.avail_in && stream.avail_out))
            throw FormatError("Invalid or truncated MCC zlib stream");
    }
}
void word(std::vector<std::byte>& b,std::size_t at,std::uint32_t value) {
    Bytes(b).require(at,4);
    for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(i*8));
}
std::size_t mip_bytes(unsigned w,unsigned h,unsigned format) {
    if(format>=14 && format<=16)return std::size_t((w+3)/4)*((h+3)/4)*(format==14?8:16);
    const auto bytes=format<=2?1u:(format==3 || format==6 || format==8 || format==9 || format==22 || format==23)?2u:(format==10 || format==11)?4u:0u;
    if(!bytes)throw FormatError("Unsupported MCC bitmap storage format");
    return std::size_t(w)*h*bytes;
}
Texture cube(const Bytes& source,unsigned width,unsigned format,bool swizzled,unsigned mips) {
    if(!width || width>4096 || mips>=unsigned(std::bit_width(width)))throw FormatError("Invalid MCC cube dimensions/mips");
    // Halo2 PC's Fragmented cube layout stores all six faces of mip N before
    // mip N+1. Reclaimer's Halo2VistaCubeLayout gives Right/Back/Left/Front/
    // Top/Bottom; the original Xbox/native order is Right/Left/Back/Front/Top/Bottom.
    constexpr std::array<unsigned,6> target_face{0,2,1,3,4,5};
    std::array<std::vector<std::byte>,6> faces;
    std::size_t at=0;
    for(unsigned level=0,w=width;level<=mips;++level,w=std::max(1u,w/2)) {
        const auto size=mip_bytes(w,w,format);
        for(unsigned face=0;face<6;++face) {
            const auto raw=source.slice(at,size);at+=size;
            auto& out=faces[target_face[face]];out.insert(out.end(),raw.begin(),raw.end());
        }
    }
    if(at!=source.size())throw FormatError("MCC cube has an unrecognized face/mip extent");
    Texture result{width,width,std::uint16_t(format),{},{},6};
    for(const auto& face:faces) {
        auto image=prepare_texture(Bytes(face),width,width,format,swizzled,mips);
        result.format=image.format;
        for(auto mip:image.mips){mip.offset+=result.pixels.size();result.mips.push_back(mip);}
        result.pixels.insert(result.pixels.end(),image.pixels.begin(),image.pixels.end());
    }
    return result;
}
Texture rgba_volume(const Bytes& source,unsigned w,unsigned h,unsigned d,unsigned format,unsigned mips) {
    if(!w || !h || !d || w>4096 || h>4096 || d>4096 || std::uint64_t(w)*h*d>16777216 ||
        mips>=unsigned(std::bit_width(std::max({w,h,d}))))throw FormatError("Invalid MCC RGBA volume dimensions/mips");
    Texture result{w,h,11,{},{},1,d};std::size_t at=0;
    for(unsigned level=0;level<=mips;++level) {
        const auto size=std::size_t(w)*h*d*4;const auto raw=source.slice(at,size);
        const auto base=result.pixels.size();result.pixels.resize(base+size);
        for(std::size_t p=0;p<size;p+=4) {
            result.pixels[base+p]=raw[p+2];result.pixels[base+p+1]=raw[p+1];result.pixels[base+p+2]=raw[p];
            result.pixels[base+p+3]=format==11?raw[p+3]:std::byte{255};
        }
        result.mips.push_back({w,h,base,size,d});at+=size;
        w=std::max(1u,w/2);h=std::max(1u,h/2);d=std::max(1u,d/2);
    }
    if(at!=source.size())throw FormatError("MCC volume has an unrecognized mip extent");
    return result;
}
struct Directory {
    std::vector<Tag> tags;
    std::vector<TagGroupDefinition> groups;
    std::int64_t magic{};
    std::uint32_t scenario{},globals{};
};
const Tag& required_tag(const Cache& cache,std::uint32_t datum,const char* group) {
    const auto* selected=cache.query_tag({.handle=TagHandle{datum},.group=TagGroup::from_string(group),.require_address=true,.minimum_size=1});
    if(!selected)throw FormatError(std::string("Missing resident MCC ")+group+" tag");
    return *selected;
}
Directory directory(const Bytes& data) {
    data.require(0,header_size);
    if(data.u32(0)!=0x68656164 || data.u32(4)!=13 || data.u32(8)!=data.size())
        throw FormatError("Invalid decoded MCC v13 cache");
    const auto index=data.u32(16);
    data.require(index,32);
    const auto count=data.u32(index+24);
    if(!count || count>65536 || count!=data.u32(32))throw FormatError("Invalid MCC tag directory count");
    const auto header_magic=std::int64_t(data.u32(index))-index-32;
    const auto table=TagAddressSpace(data,header_magic).offset(data.u32(index+8),std::size_t(count)*16);
    const auto names=data.u32(36),size=data.u32(40),offsets=data.u32(44);
    data.require(names,size);data.require(offsets,std::size_t(count)*4);
    Directory out;
    out.magic=std::int64_t(data.u32(720))-index;
    out.scenario=data.u32(index+12);out.globals=data.u32(index+16);
    out.groups=read_tag_group_definitions(data,index,header_magic);
    out.tags.resize(count,Tag{"","",0xffffffffu,0,0});
    for(std::size_t i=0;i<count;++i) {
        const auto at=table+i*16;
        const auto datum=data.u32(at+4);
        if(datum==0xffffffffu)continue;
        const auto name=data.u32(offsets+i*4);
        if((datum&65535)!=i || name>=size)throw FormatError("MCC tag salt/name disagrees with its directory");
        auto& tag=out.tags[i];
        tag={TagGroup(data.u32(at)).string(),data.string(names+name,size-name),datum,data.u32(at+8),data.u32(at+12)};
        if(tag.address)TagAddressSpace(data,out.magic).offset(tag.address,tag.size);
    }
    return out;
}
}

Texture prepare_mcc_cubemap(const Bytes& source,unsigned width,unsigned format,bool swizzled,unsigned mips) {
    return cube(source,width,format,swizzled,mips);
}
std::vector<std::byte> read_mcc_cache(const std::filesystem::path& path) {
    return decode_mcc_cache(read_range(path,0,0,true));
}
std::vector<std::byte> read_mcc_texture_partition(const std::filesystem::path& path,
    std::uint32_t pointer,std::uint32_t stored_size) {
    constexpr std::size_t maximum=256ull*1024*1024;
    if(stored_size<8 || stored_size>maximum)throw FormatError("Invalid MCC texture partition size");
    const auto raw=read_range(path,pointer&0x3fffffffu,stored_size);
    const Bytes input(raw);
    const auto count=input.u32(0);
    if(!count || count>4096)throw FormatError("Invalid MCC texture segment count");
    input.require(4,std::size_t(count)*4);
    std::size_t at=4+std::size_t(count)*4,total=at;bool compressed=false;
    for(std::size_t i=0;i<count;++i) {
        const auto size=std::bit_cast<std::int32_t>(input.u32(4+i*4));
        if(!size)throw FormatError("Empty MCC texture segment");
        compressed|=size>0;
        const auto extent=std::uint64_t(size<0?-std::int64_t(size):size);
        if(extent>raw.size()-total)throw FormatError("MCC texture segment escapes partition");
        total+=std::size_t(extent);
    }
    if(total!=raw.size())throw FormatError("MCC texture segment extent differs from descriptor");
    static TexturePartitionResidency residency;
    if(compressed)if(const auto resident=residency.find(raw))return *resident;
    std::vector<std::byte> result;
    for(std::size_t i=0;i<count;++i) {
        const auto size=std::bit_cast<std::int32_t>(input.u32(4+i*4));
        const auto extent=std::size_t(size<0?-std::int64_t(size):size);
        const auto part=input.slice(at,extent);at+=extent;
        if(size<0) {
            if(extent>maximum-result.size())throw FormatError("Excessive raw MCC texture");
            result.insert(result.end(),part.begin(),part.end());
        } else {
            const auto decoded=inflate(part,maximum-result.size());
            result.insert(result.end(),decoded.begin(),decoded.end());
        }
    }
    if(compressed)residency.admit(raw,result);
    return result;
}

std::uint32_t Cache::mcc_sound_class_tag(bool remastered) const {
    const Bytes data(cache_bytes());
    const auto* globals=query_tag({.handle=globals_tag_.is_null()?std::nullopt:std::optional<TagHandle>{globals_tag_},
        .group=TagGroup::from_string("matg"),.require_address=true});
    if(!globals)throw FormatError("Missing MCC globals for sound class ownership");
    const auto rows=metadata_block(metadata_offset(globals->address,0xc8)+0xc0,0x2c);
    if(rows.count!=1)throw FormatError("Invalid MCC sound globals count");
    return data.u32(rows.offset+(remastered?4:12));
}

EncodedSound Cache::load_mcc_sound(std::uint32_t datum,unsigned pitch,unsigned permutation) const {
    const Bytes data(cache_bytes());
    const auto& sound=required_tag(*this,datum,"snd!");
    const auto& gestalt=mcc_sound_gestalt(datum);
    const auto s=metadata_offset(sound.address,0x1c),g=metadata_offset(gestalt.address,0x78);
    auto entry=[&](std::size_t header,std::size_t stride,std::size_t index) {
        const auto rows=metadata_block(header,stride);
        if(index>=rows.count)throw FormatError("MCC sound index escapes its resident table");
        return rows.offset+index*stride;
    };
    EncodedSound out;out.name=sound.name;out.sound_class=std::to_integer<unsigned>(data.slice(s+2,1)[0]);
    if(out.sound_class>=54)throw FormatError("Invalid MCC sound class");
    const auto codec=entry(g,3,std::to_integer<unsigned>(data.slice(s+3,1)[0]));
    const auto rate=std::to_integer<unsigned>(data.slice(codec,1)[0]);
    const auto encoding=std::to_integer<unsigned>(data.slice(codec+1,1)[0]);
    constexpr unsigned rates[]{22050,44100,32000,48000},channels[]{1,2,2,4};
    if(rate>=std::size(rates) || encoding>=std::size(channels))throw FormatError("Invalid MCC sound codec format");
    out.sample_rate=rates[rate];out.channels=channels[encoding];
    out.codec=encoding==2?4:std::to_integer<unsigned>(data.slice(codec+2,1)[0]);
    if(out.codec>5)throw FormatError("Unknown MCC sound compression");
    if(pitch>=std::to_integer<unsigned>(data.slice(s+10,1)[0]))throw FormatError("MCC sound pitch outside ranges");
    const auto range=entry(g+0x28,12,std::size_t(data.u16(s+8))+pitch);
    if(permutation>=data.u16(range+10))throw FormatError("MCC sound permutation outside range");
    const auto perm=entry(g+0x30,16,std::size_t(data.u16(range+8))+permutation);
    // English is language zero. 6CCBB0 falls back to its zero row only when
    // the selected language row is entirely empty; no bank/offset probing.
    const auto info=entry(perm+8,8,0);
    const auto pcm_bytes=data.u32(info);
    const auto first=data.u16(info+4),count=data.u16(info+6);
    if(!pcm_bytes || !count || (out.codec!=4 && pcm_bytes%(out.channels*2)))throw FormatError("Invalid MCC sound PCM/chunk extent");
    // WMA is a serialized DirectSound container; this word is its serialized
    // byte extent, not a PCM sample count. The container owns its audio format.
    out.samples=out.codec==4?0:pcm_bytes/(out.channels*2);
    if(out.samples>out.sample_rate*600ull)throw FormatError("MCC sound exceeds duration limit");
    const auto language=entry(g+0x60,8,0);
    const auto chunks=metadata_block(language,16);
    if(std::size_t(first)+count>chunks.count)throw FormatError("MCC sound chunks escape language table");
    const bool remastered=(data.u16(s)&0x800)!=0;
    const auto class_id=mcc_sound_class_tag(remastered);
    const auto& classes=required_tag(*this,class_id,"sncl");
    const auto cls=entry(metadata_offset(classes.address,8),0x78,out.sound_class);
    // Original MCC 6C9170 selects bank 5 for multilingual classes, otherwise
    // bank 6 for remastered-only sounds and bank 4 for Classic neutral audio.
    const auto bank=(data.u16(cls+8)&0x80)?"sounds_en.dat":remastered?"sounds_remastered.dat":"sounds_neutral.dat";
    for(unsigned n=0;n<count;++n) {
        const auto c=chunks.offset+(std::size_t(first)+n)*16;
        const auto size=data.u32(c+4)&0x1fffffffu;
        if(!size || size>128*1024*1024-out.data.size())throw FormatError("Invalid MCC sound chunk size");
        const auto bytes=read_range(shared_directory_/bank,data.u32(c),size);
        out.data.insert(out.data.end(),bytes.begin(),bytes.end());
        out.packet_chunks.push_back({size,data.u32(c+8),(data.u32(c+4)&0x20000000u)!=0});
    }
    return out;
}

const Tag& Cache::mcc_sound_gestalt(std::uint32_t datum) const {
    (void)required_tag(*this,datum,"snd!");
    const auto index=datum&0xffffu;
    if(index>=mcc_sound_gestalts_.size() || mcc_sound_gestalts_[index]==0xffffffffu)
        throw FormatError("MCC sound has no exact directory-owner gestalt");
    return required_tag(*this,mcc_sound_gestalts_[index],"ugh!");
}

void Cache::initialize_mcc(const MccPreparedMapOwner* owner) {
    const auto tag=[&](std::uint32_t datum,const char* group)->const Tag& {return required_tag(*this,datum,group);};
    const Bytes input(storage_);
    auto local=directory(input);
    tag_groups_=local.groups;scenario_tag_={local.scenario};globals_tag_={local.globals};
    name_=path_.stem().string();build_=input.string(144,32);scenario_=input.string(208,256);
    std::vector<std::byte> shared_bytes;
    Directory shared;
    const bool resources=owner && owner->binding
        ? owner->binding->role==MccCacheRole::Resources : scenario_tag_.is_null();
    const bool needs_shared=!resources && std::ranges::any_of(local.tags,[](const Tag& t){return t.datum!=0xffffffffu && !t.address;});
    if(needs_shared) {
        shared_bytes=read_mcc_cache(shared_directory_/"shared.map");shared=directory(Bytes(shared_bytes));
        for(const auto& group:shared.groups) {
            const auto* owned=tag_group(group.group);
            if(!owned || owned->parent!=group.parent || owned->grandparent!=group.grandparent)
                throw FormatError("MCC shared tag-group hierarchy collision");
        }
    }
    tags_=local.tags;
    const auto shared_owner=merge_shared_tag_directory(tags_,shared.tags,
        SharedTagLoadPolicy::unloaded_only);
    std::array<std::uint32_t,2> sound_gestalt{0xffffffffu,0xffffffffu};
    for(std::size_t i=0;i<tags_.size();++i)if(tags_[i].group=="ugh!" && tags_[i].address) {
        auto& owner=sound_gestalt[shared_owner[i]?1:0];
        if(owner!=0xffffffffu)throw FormatError("Ambiguous MCC sound gestalt directory ownership");
        owner=tags_[i].datum;
    }
    mcc_sound_gestalts_.resize(tags_.size(),0xffffffffu);
    for(std::size_t i=0;i<tags_.size();++i)if(tags_[i].group=="snd!" && tags_[i].address)
        mcc_sound_gestalts_[i]=sound_gestalt[shared_owner[i]?1:0];
    validate_tag_directory();
    std::vector<std::size_t> resident;
    for(std::size_t i=0;i<tags_.size();++i)if(tags_[i].datum!=0xffffffffu && tags_[i].address && tags_[i].size)resident.push_back(i);
    if(resident.empty())throw FormatError("MCC cache has no resident metadata");
    std::ranges::sort(resident,[&](auto a,auto b){return tags_[a].address<tags_[b].address;});
    vista_metadata_regions_=tag_metadata_regions(tags_);
    const auto first=vista_metadata_regions_.front().first;
    std::uint64_t end=first;
    for(auto i:resident) {
        const auto& t=tags_[i];
        if(t.address<end)throw FormatError("Overlapping MCC resident tag allocations");
        end=std::uint64_t(t.address)+t.size;
    }
    end=std::max(end,std::uint64_t(vista_metadata_regions_.back().first)+vista_metadata_regions_.back().second);
    if(end>0x100000000ull || end-first>128ull*1024*1024)throw FormatError("Excessive MCC metadata span");
    std::vector<std::byte> metadata(std::size_t(end-first));
    for(auto i:resident) {
        const auto& t=tags_[i];const Bytes source(shared_owner[i]?shared_bytes:storage_);
        const auto at=TagAddressSpace(source,shared_owner[i]?shared.magic:local.magic).offset(t.address,t.size);
        const auto bytes=source.slice(at,t.size);
        std::copy(bytes.begin(),bytes.end(),metadata.begin()+std::ptrdiff_t(t.address-first));
    }
    metadata_magic_=std::int64_t(first)-std::int64_t(storage_.size());
    storage_.insert(storage_.end(),metadata.begin(),metadata.end());
    // The donor format ends here for animation metadata. Publish ordinary
    // Xbox-shaped graph payloads to the existing tag directory and providers;
    // source paths and full salted handles remain unchanged.
    for(auto& t:tags_)if(t.group=="jmad" && t.address && t.size) {
        const auto next=(std::uint64_t(std::int64_t(storage_.size())+metadata_magic_)+15)&~std::uint64_t(15);
        const auto padding=next-std::uint64_t(std::int64_t(storage_.size())+metadata_magic_);
        const auto file_offset=storage_.size()+std::size_t(padding);
        if(next>0xffffffffu || file_offset>0x3fffffffu)throw FormatError("Canonical MCC animation address overflow");
        const auto original=Bytes(storage_).slice(metadata_offset(t.address,t.size),t.size);
        const auto converted=normalize_mcc_animation_graph(original,t.address,std::uint32_t(next),std::uint32_t(file_offset),
            [&](std::uint32_t address,std::size_t size){return Bytes(storage_).slice(metadata_offset(address,size),size);});
        if(next+converted.size()-first>128ull*1024*1024 || file_offset+converted.size()>maximum_cache)
            throw FormatError("Excessive canonical animation metadata span");
        storage_.resize(file_offset);
        storage_.insert(storage_.end(),converted.begin(),converted.end());
        // Other MCC definitions can deduplicate authored byte banks into the
        // original animation allocation. Keep that exact typed owner alive;
        // the directory still publishes only the normalized vanilla graph.
        retained_tag_allocations_.push_back(t);
        t.address=std::uint32_t(next);t.size=std::uint32_t(converted.size());
    }
    // Authored phmo values enter the same ordinary directory with checked
    // original consumer layouts. Runtime-only Havok/tool pointers are rebound
    // by the native TU1.5 constructor, never truncated from MCC host pointers.
    for(const auto* group:{"phmo","coll"})for(auto& t:tags_)if(t.group==group && t.address && t.size) {
        const auto next=(std::uint64_t(std::int64_t(storage_.size())+metadata_magic_)+15)&~std::uint64_t(15);
        const auto padding=next-std::uint64_t(std::int64_t(storage_.size())+metadata_magic_);
        const auto file_offset=storage_.size()+std::size_t(padding);
        if(next>0xffffffffu)throw FormatError("Canonical MCC physics address overflow");
        const auto original=Bytes(storage_).slice(metadata_offset(t.address,t.size),t.size);
        const auto normalize=t.group=="coll"?normalize_mcc_collision_model:normalize_mcc_physics_model;
        const auto converted=normalize(original,t.address,std::uint32_t(next),
            [&](std::uint32_t address,std::size_t size){return Bytes(storage_).slice(metadata_offset(address,size),size);});
        if(next+converted.size()-first>128ull*1024*1024 || file_offset+converted.size()>maximum_cache)
            throw FormatError("Excessive canonical physics metadata span");
        storage_.resize(file_offset);storage_.insert(storage_.end(),converted.begin(),converted.end());
        t.address=std::uint32_t(next);t.size=std::uint32_t(converted.size());
    }
    // Instanced collision physics changes layout while the surrounding BSP's
    // render/resource data retains its existing mapping and byte contents.
    for(const auto& t:tags_)if(t.group=="sbsp" && t.address && t.size) {
        const auto at=metadata_offset(t.address,t.size);
        const auto converted=normalize_mcc_structure_bsp_physics(Bytes(storage_).slice(at,t.size),t.address);
        if(converted.size()!=t.size)throw FormatError("MCC instanced physics changed its BSP allocation extent");
        std::copy(converted.begin(),converted.end(),storage_.begin()+std::ptrdiff_t(at));
    }
    for(const auto& t:tags_)if((t.group=="bipd" || t.group=="crea" || t.group=="vehi") && t.address && t.size) {
        const auto at=metadata_offset(t.address,t.size);
        const auto kind=t.group=="bipd"?MccUnitPhysicsKind::Biped:
            t.group=="crea"?MccUnitPhysicsKind::Creature:MccUnitPhysicsKind::Vehicle;
        const auto converted=normalize_mcc_unit_physics(Bytes(storage_).slice(at,t.size),t.address,kind);
        std::copy(converted.begin(),converted.end(),storage_.begin()+std::ptrdiff_t(at));
    }
    std::vector<std::pair<std::uint32_t,std::uint32_t>> retained_regions;
    for(const auto& allocation:retained_tag_allocations_)
        retained_regions.emplace_back(allocation.address,allocation.size);
    vista_metadata_regions_=tag_metadata_regions(tags_,retained_regions);
    // This private load phase owns an immutable directory and stable storage.
    // Qualify map-specific authoring before the generic converters below alter
    // its complete donor owners. Converted bytes enter the ordinary directory.
    const MccMapGameplaySource map_source{
        .scenario_path=scenario_,.scenario=scenario_tag_,.globals=globals_tag_,
        .resolve=[&](TagHandle handle)->std::optional<MccMapGameplayIdentity> {
            const auto* t=find_tag(handle);
            if(!t || !t->address || !t->size)return {};
            return MccMapGameplayIdentity{t->handle(),t->type(),t->name,t->address,t->size};
        },
        .find=[&](TagGroup group,std::string_view name)->std::optional<MccMapGameplayIdentity> {
            const Tag* found=nullptr;
            for(const auto& t:tags_)if(!t.handle().is_null() && t.address && t.size &&
                t.type()==group && t.name==name) {
                if(found)return {};
                found=&t;
            }
            if(!found)return {};
            return MccMapGameplayIdentity{found->handle(),found->type(),found->name,found->address,found->size};
        },
        .read=[&](std::uint32_t address,std::size_t bytes) {return read_tag_bytes(address,bytes);}
    };
    const auto* material_globals=query_tag({.group=tag_groups::globals});
    const std::array map_authoring{plan_mcc_map_gameplay_vanilla(map_source),
        plan_mcc_teleporter_gameplay_vanilla(map_source),
        plan_mcc_material_gameplay_vanilla(map_source),
        plan_mcc_foot_gameplay_vanilla(map_source,material_globals?material_globals->handle():TagHandle{})};
    for(const auto& plan:map_authoring)if(plan.status==MccMapGameplayStatus::InvalidMetadata)
        throw FormatError("MCC map gameplay authoring has invalid metadata: "+plan.reason);
    std::vector<std::pair<std::span<std::byte>,const MccMapGameplayWrite*>> destinations;
    for(const auto& plan:map_authoring)if(plan.status==MccMapGameplayStatus::Ready)
        for(const auto& write:plan.writes)
            destinations.emplace_back(std::span(storage_).subspan(
                metadata_offset(write.source_token,write.after.size()),write.after.size()),&write);
    preflight_mcc_map_gameplay_vanilla(map_authoring,map_source.read);
    // Every component, destination and guard is checked before the first
    // bounded nonthrowing copy. Directory and storage ownership stay stable.
    for(const auto& [destination,write]:destinations)
        std::copy(write->after.begin(),write->after.end(),destination.begin());
    // Exact named retail scalar tuples enter the ordinary internal directory
    // before gameplay or resource-only cache consumers. Unknown authored
    // tuples remain untouched; the pure converter exposes their mismatch
    // receipt for import/audit callers. This does not certify full parity.
    for(const auto& t:tags_)if(t.address && t.size &&
        (t.type()==tag_groups::weapon || t.type()==tag_groups::projectile || t.type()==tag_groups::effect ||
            t.type()==tag_groups::damage_effect)) {
        const auto at=metadata_offset(t.address,t.size);
        (void)convert_mcc_gameplay_scalar(t.type(),t.name,
            std::span(storage_).subspan(at,t.size));
    }
    const MccGameplayReferenceAccess reference_access{
        .bytes=[&](std::uint32_t address,std::size_t size) {
            return std::span(storage_).subspan(metadata_offset(address,size),size);
        },
        .resolve=[&](TagHandle handle)->std::optional<MccGameplayReferenceIdentity> {
            if(handle.is_null() || handle.index()>=tags_.size())return {};
            const auto& t=tags_[handle.index()];
            if(t.handle()!=handle || !t.address || !t.size)return {};
            return MccGameplayReferenceIdentity{handle,t.type(),t.name};
        },
        .find=[&](TagGroup group,std::string_view name)->std::optional<MccGameplayReferenceIdentity> {
            const Tag* found=nullptr;
            for(const auto& t:tags_)if(!t.handle().is_null() && t.address && t.size &&
                t.type()==group && t.name==name) {
                if(found)return {}; // Ambiguous authored target is not a local identity proof.
                found=&t;
            }
            if(!found)return {};
            return MccGameplayReferenceIdentity{found->handle(),found->type(),found->name};
        }
    };
    for(const auto& t:tags_)if(t.address && t.size &&
        (t.type()==tag_groups::weapon || t.type()==tag_groups::projectile))
        // Missing local dependencies stay unchanged and produce an explicit
        // requirement receipt for callers of the same pure profile converter.
        (void)convert_mcc_gameplay_references(t.type(),t.name,t.address,reference_access);
    for(const auto& t:tags_)if(t.address && t.size &&
        (t.type()==tag_groups::biped || t.type()==tag_groups::vehicle))
        (void)convert_mcc_gameplay_unit_scalars(t.type(),t.name,t.address,reference_access.bytes);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::model)
        (void)convert_mcc_destruction_model_scalars(t.type(),t.name,t.address,reference_access.bytes);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::model)
        (void)convert_mcc_destruction_model_contract(t.type(),t.name,t.address,reference_access.bytes);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::model)
        (void)convert_mcc_destruction_model_references(t.type(),t.name,t.address,reference_access);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::multiplayer_globals)
        (void)convert_mcc_multiplayer_weapon_table(t.type(),t.name,t.address,reference_access);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::multiplayer_globals)
        (void)convert_mcc_multiplayer_vehicle_table(t.type(),t.name,t.address,reference_access);
    for(const auto& t:tags_)if(t.address && t.size && t.type()==tag_groups::scenario_structure_bsp) {
        (void)convert_mcc_bsp_instance_transform(t.type(),t.name,t.address,reference_access.bytes);
        (void)convert_mcc_bsp_collision_reference(t.type(),t.name,t.address,reference_access.bytes);
        (void)convert_mcc_bsp_instance_bounds(t,reference_access.bytes);
    }
    const MccHudReferenceAccess hud_reference_access{
        .read=[&](std::uint32_t address,std::size_t size) {
            return Bytes(storage_).slice(metadata_offset(address,size),size);
        },
        .resolve=[&](TagHandle handle)->std::optional<Tag> {
            if(handle.is_null() || handle.index()>=tags_.size())return {};
            const auto& t=tags_[handle.index()];
            if(t.handle()!=handle || !t.address || !t.size)return {};
            return t;
        },
        .string_id=[&](std::uint32_t id) {return string_id(id);},
        .directory=tags_
    };
    const auto publish_hud=[&](Tag& t,MccOwnedHudProjection projection) {
        // A complete authenticated owner is cloned because other definitions
        // can share its source curves. The ordinary directory publishes only
        // the converted owner; its physical layout still uses the nhdt schema.
        auto& converted=projection.bytes;
        if(converted.empty() || converted.size()>65536)
            throw FormatError("Invalid canonical MCC HUD allocation extent");
        const auto current=std::uint64_t(std::int64_t(storage_.size())+metadata_magic_);
        const auto next=(current+15)&~std::uint64_t(15);
        const auto file_offset=storage_.size()+std::size_t(next-current);
        if(next>0xffffffffu || converted.size()>0x100000000ull-next ||
            next+converted.size()-first>128ull*1024*1024 ||
            file_offset>maximum_cache || converted.size()>maximum_cache-file_offset)
            throw FormatError("Excessive canonical MCC HUD metadata span");
        auto pointers=projection.pointers;
        std::ranges::sort(pointers);
        std::uint64_t previous_end=0;
        for(const auto [field,target]:pointers) {
            if(field<previous_end || field>converted.size() || converted.size()-field<4 ||
                target>=converted.size() || Bytes(converted).u32(field)!=target)
                throw FormatError("Invalid canonical MCC HUD pointer ownership");
            previous_end=std::uint64_t(field)+4;
        }
        for(const auto [field,target]:pointers)
            word(converted,field,std::uint32_t(next+target));
        storage_.resize(file_offset);
        storage_.insert(storage_.end(),converted.begin(),converted.end());
        retained_tag_allocations_.push_back(t);
        retained_regions.emplace_back(t.address,t.size);
        t.address=std::uint32_t(next);t.size=std::uint32_t(converted.size());
    };
    // Preserve the existing scoreboard allocation before shader globals are
    // cloned: authenticated sound metadata retains that established namespace.
    for(auto& t:tags_)if(t.group=="nhdt" && t.name=="ui\\hud\\scoreboard" && t.address && t.size) {
        auto projection=project_mcc_scoreboard_hud_reference(t,hud_reference_access);
        if(projection && projection->status==MccGameplayReferenceStatus::Converted)
            publish_hud(t,std::move(*projection));
    }
    struct PendingHud {
        Tag source;
        std::vector<std::byte> before;
        MccOwnedHudProjection projection;
    };
    std::vector<PendingHud> pending_huds;
    // Qualify against retained MCC bitmap metadata before the existing shader
    // owner can project its bitmap layouts or expand the directory. Only owned
    // snapshots survive that producer; no borrowed directory view escapes it.
    for(const auto& t:tags_)if(t.group=="nhdt" && t.name!="ui\\hud\\scoreboard" && t.address && t.size) {
        auto projection=project_mcc_hud_reference(t,hud_reference_access);
        if(!projection || projection->status!=MccGameplayReferenceStatus::Converted)continue;
        const auto before=Bytes(storage_).slice(metadata_offset(t.address,t.size),t.size);
        pending_huds.push_back({t,{before.begin(),before.end()},std::move(*projection)});
    }
    vista_metadata_regions_=tag_metadata_regions(tags_,retained_regions);
    initialize_mcc_native_shaders(owner);
    // New geometry owners append after the existing globals/shader producers.
    // Their names, full salted handles, complete source allocations and live
    // directory identities must still match the exact admitted beforeimage.
    for(auto& pending:pending_huds) {
        const auto index=pending.source.handle().index();
        if(index>=tags_.size())throw FormatError("Canonical MCC HUD owner disappeared before publication");
        auto& t=tags_[index];
        if(t.handle()!=pending.source.handle() || t.group!=pending.source.group ||
            t.name!=pending.source.name || t.address!=pending.source.address || t.size!=pending.source.size ||
            !std::ranges::equal(pending.before,Bytes(storage_).slice(metadata_offset(t.address,t.size),t.size)))
            throw FormatError("Canonical MCC HUD owner changed before publication");
        publish_hud(t,std::move(pending.projection));
    }
    retained_regions.clear();
    for(const auto& allocation:retained_tag_allocations_)
        retained_regions.emplace_back(allocation.address,allocation.size);
    vista_metadata_regions_=tag_metadata_regions(tags_,retained_regions);
    const auto table=storage_.size();storage_.resize(table+tags_.size()*16);
    for(std::size_t i=0;i<tags_.size();++i) {
        word(storage_,table+i*16+4,tags_[i].datum);word(storage_,table+i*16+8,tags_[i].address);word(storage_,table+i*16+12,tags_[i].size);
    }
    if(resources || scenario_tag_.is_null())return;
    const auto& scenario=tag(local.scenario,"scnr");
    const auto scnr=metadata_offset(scenario.address,0x218);
    if(original_data_ && original_data_->multiplayer_netgame_postprocess_constants)
        multiplayer_netgame_writes_=apply_multiplayer_netgame_postprocess(
            {storage_,metadata_magic_,scenario.address},*original_data_->multiplayer_netgame_postprocess_constants,
            original_data_->image==OriginalData::Image::TitleUpdate15?MultiplayerNetgamePostprocessImage::TitleUpdate15:MultiplayerNetgamePostprocessImage::MapPack11).stores;
    if(original_data_ && original_data_->multiplayer_balance_constants && Bytes(storage_).u16(scnr+0x10)==1)
        multiplayer_balance_writes_=apply_multiplayer_balance(
            {storage_,metadata_magic_,table,tags_.size(),scenario.address,tag(local.globals,"matg").address},
            *original_data_->multiplayer_balance_constants,original_data_->image==OriginalData::Image::TitleUpdate15?0x13e0:0).stores;
    struct Pending {std::uint32_t bsp,lightmap;};std::vector<Pending> pending;
    {
        const Bytes data(storage_);
        const auto bsps=metadata_block(scnr+0x210,0x44,256);
        for(std::size_t i=0;i<bsps.count;++i)pending.push_back({data.u32(bsps.offset+i*0x44+0x14),data.u32(bsps.offset+i*0x44+0x1c)});
    }
    for(const auto& row:pending) {
        const auto& bsp=tag(row.bsp,"sbsp");const auto* lightmap=row.lightmap==0xffffffffu?nullptr:&tag(row.lightmap,"ltmp");
        const auto low=lightmap?std::min(bsp.address,lightmap->address):bsp.address;
        const auto high=lightmap?std::max(std::uint64_t(bsp.address)+bsp.size,std::uint64_t(lightmap->address)+lightmap->size):std::uint64_t(bsp.address)+bsp.size;
        if(bsp.size<0x23c || low<4096 || high-low>128ull*1024*1024)throw FormatError("Invalid MCC BSP/lightmap allocation");
        const auto magic=low-4096;std::vector<std::byte> view(std::size_t(high-magic));
        const Bytes data(storage_);
        for(const auto* t:{&bsp,lightmap})if(t) {
            const auto bytes=data.slice(metadata_offset(t->address,t->size),t->size);
            std::copy(bytes.begin(),bytes.end(),view.begin()+std::ptrdiff_t(t->address-magic));
        }
        const auto root=data.slice(metadata_offset(bsp.address,0x23c),0x23c);
        std::copy(root.begin(),root.end(),view.begin()+16);
        word(view,0,std::uint32_t(view.size()));word(view,4,bsp.address);word(view,8,lightmap?lightmap->address:0xffffffffu);
        if(storage_.size()+view.size()>maximum_cache)throw FormatError("Excessive MCC owned metadata view");
        bsps_.push_back({std::uint32_t(storage_.size()),std::uint32_t(view.size()),magic,bsp.datum,bsp.name});
        storage_.insert(storage_.end(),view.begin(),view.end());
    }
    const Bytes data(storage_);const auto starts=metadata_block(scnr+0x100,0x34);
    for(std::size_t i=0;i<starts.count;++i) {
        const auto at=starts.offset+i*0x34;
        Spawn spawn{{data.f32(at),data.f32(at+4),data.f32(at+8)},data.f32(at+12),data.u16(at+0x12)};
        if(!std::isfinite(spawn.position.x) || !std::isfinite(spawn.position.y) || !std::isfinite(spawn.position.z) || !std::isfinite(spawn.facing))throw FormatError("Nonfinite MCC player start");
        spawn.team=std::int16_t(data.u16(at+0x10));spawn.player_type=std::int16_t(data.u16(at+0x2c));
        for(unsigned j=0;j<4;++j) {spawn.game_types[j]=std::int16_t(data.u16(at+0x14+j*2));spawn.spawn_types[j]=std::int16_t(data.u16(at+0x1c+j*2));}
        spawns_.push_back(spawn);
    }
    // Authenticated authoring differences become ordinary internal data
    // before any content, script or native gameplay consumer sees this cache.
    // Prepare every checked owner and original-byte comparison before the
    // first mutation; a stale or overlapping plan cannot partially convert it.
    const auto commit_authoring=[&](const auto& writes,const auto& source_token) {
        struct PreparedWrite {std::size_t offset;std::span<const std::byte> after;};
        std::vector<PreparedWrite> prepared;
        prepared.reserve(writes.size());
        for(const auto& write:writes) {
            if(write.before.empty() || write.before.size()!=write.after.size())
                throw FormatError("MCC vanilla authoring write has an invalid extent");
            const auto at=metadata_offset(source_token(write),write.before.size());
            if(!std::ranges::equal(std::span<const std::byte>(storage_).subspan(at,write.before.size()),write.before))
                throw FormatError("MCC vanilla authoring changed before publication");
            prepared.push_back({at,write.after});
        }
        std::ranges::sort(prepared,{},&PreparedWrite::offset);
        std::size_t end=0;
        for(const auto& target:prepared) {
            if(target.offset<end)throw FormatError("MCC vanilla authoring owners overlap");
            end=target.offset+target.after.size();
        }
        for(const auto& target:prepared)
            std::copy(target.after.begin(),target.after.end(),
                storage_.begin()+std::ptrdiff_t(target.offset));
    };
    const auto vanilla=plan_mcc_cairo_vanilla(*this);
    if(vanilla.matched)commit_authoring(vanilla.writes,[](const auto& write){return write.source_token;});
    const auto campaign=plan_mcc_campaign_vanilla({
        .is_mcc=true,.scenario_path=scenario.name,.scenario_address=scenario.address,
        .read=[&](auto address,auto size){return read_tag_bytes(address,size);},
        .resolve_tag=[&](std::uint32_t handle)->std::optional<MccCampaignTagIdentity> {
            const auto* selected=find_tag(TagHandle{handle});
            if(!selected)return {};
            return MccCampaignTagIdentity{selected->group,selected->name};
        },
        .resolve_string_id=[&](std::uint32_t id){return string_id(id);}
    });
    if(campaign.matched)commit_authoring(campaign.writes,[](const auto& write){return write.address;});
}

BitmapResource Cache::load_mcc_bitmap_resource(std::uint32_t datum,unsigned index) const {
    const auto& selected=required_tag(*this,datum,"bitm");
    if(selected.size<76)throw FormatError("Truncated MCC bitmap tag");
    const auto base=metadata_offset(selected.address,selected.size);
    const auto records=metadata_block(base+68,168);
    if(index>=records.count)throw FormatError("MCC bitmap frame outside bank");
    const auto at=records.offset+std::size_t(index)*168;
    const Bytes data(cache_bytes());
    // MCC splits the old depth word into depth8 and creation/sampling flags8.
    BitmapResource out{data.u16(at+4),data.u16(at+6),std::to_integer<std::uint16_t>(data.slice(at+8,1)[0]),data.u16(at+10),data.u16(at+12),{}};
    for(unsigned i=0;i<3;++i) {
        const auto size=data.u32(at+80+i*4);
        if(size)out.levels[i]=read_mcc_texture_partition(shared_directory_/"textures.dat",data.u32(at+56+i*4),size);
    }
    return out;
}
Texture Cache::load_mcc_bitmap(std::uint32_t datum,unsigned index,std::span<const std::byte> palette) const {
    const auto& selected=required_tag(*this,datum,"bitm");
    const auto base=metadata_offset(selected.address,selected.size);
    const auto records=metadata_block(base+68,168);
    if(index>=records.count)throw FormatError("MCC bitmap frame outside bank");
    const auto at=records.offset+std::size_t(index)*168;
    const Bytes data(cache_bytes());
    const auto resource=load_mcc_bitmap_resource(datum,index);
    if(resource.levels[0].empty())throw FormatError("MCC bitmap has no highest-detail partition");
    const auto flags=data.u16(at+14),mips=data.u16(at+20);
    const auto& raw=resource.levels[0];
    if(resource.type==0 && resource.depth==1) {
        if(resource.format==18)return prepare_palettized_texture(Bytes(raw),resource.width,resource.height,flags&8,palette,mips);
        if(resource.format==17)return prepare_palettized_texture(Bytes(raw),resource.width,resource.height,flags&8,tu15_bump_palette(),mips);
        return prepare_texture(Bytes(raw),resource.width,resource.height,resource.format,flags&8,mips);
    }
    if(resource.type==1) {
        if(resource.format==10 || resource.format==11) {
            if(flags&8)throw FormatError("Unverified swizzled MCC RGBA volume layout");
            return rgba_volume(Bytes(raw),resource.width,resource.height,resource.depth,resource.format,mips);
        }
        return prepare_volume_texture(Bytes(raw),resource.width,resource.height,resource.depth,resource.format,flags&8,mips);
    }
    if(resource.type==2 && resource.depth==1 && resource.width==resource.height)
        return prepare_mcc_cubemap(Bytes(raw),resource.width,resource.format,flags&8,mips);
    throw FormatError("MCC bitmap has unsupported array/dimensionality");
}
}
