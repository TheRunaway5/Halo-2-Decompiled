#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_balance.hpp"
#include "tags/directory.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>

namespace halo2 {
namespace {
constexpr const char* vista_build="11081.07.04.30.0934.main";
struct Directory {
    std::vector<Tag> tags;
    std::vector<TagGroupDefinition> groups;
    std::int64_t magic{};
    std::uint32_t scenario{},globals{};
};


Directory directory(const Bytes& data) {
    data.require(0,2048);
    if(data.u32(0)!=0x68656164 || data.u32(4)!=8 || data.u32(2044)!=0x666f6f74 ||
       data.u32(8)!=data.size() || data.string(300,32)!=vista_build)
        throw FormatError("unsupported or truncated Vista cache");
    const auto index=data.u32(16);
    data.require(index,32);
    const auto count=data.u32(index+24);
    if(!count || count>65536 || count!=data.u32(716))throw FormatError("invalid Vista tag directory count");
    const auto table=TagAddressSpace(data, -std::int64_t(index)).offset(data.u32(index+8), std::size_t(count)*16);
    const auto names=data.u32(720),names_size=data.u32(724),name_table=data.u32(728);
    data.require(names,names_size);data.require(name_table,std::size_t(count)*4);
    Directory out;
    out.groups=read_tag_group_definitions(data,index,-std::int64_t(index));
    out.magic=std::int64_t(data.u32(32))-index;
    out.scenario=data.u32(index+12);out.globals=data.u32(index+16);
    out.tags.resize(count);
    for(std::size_t i=0;i<count;++i) {
        auto& tag=out.tags[i];
        tag.datum=0xffffffffu;
        const auto at=table+i*16;
        const auto datum=data.u32(at+4);
        if(datum==0xffffffffu)continue;
        if((datum&65535)!=i)throw FormatError("Vista datum index disagrees with its directory slot");
        const auto name=data.u32(name_table+i*4);
        if(name>=names_size)throw FormatError("Vista tag path escapes filename table");
        tag={TagGroup(data.u32(at)).string(),data.string(names+name,names_size-name),datum,data.u32(at+8),data.u32(at+12)};
        if(tag.address)TagAddressSpace(data, out.magic).offset(tag.address, tag.size);
    }
    return out;
}
std::vector<std::byte> read_shared(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input || input.tellg()<0 || input.tellg()>1024ll*1024*1024)
        throw FormatError("missing or excessive Vista shared cache: "+path.string());
    std::vector<std::byte> out(std::size_t(input.tellg()));
    input.seekg(0);
    if(!input.read(reinterpret_cast<char*>(out.data()),std::streamsize(out.size())))
        throw FormatError("short Vista shared cache read");
    return out;
}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(i*8))&255);
}
}

void Cache::initialize_vista() {
    // Header offsets and tag masks are verified against pinned Assembly /
    // Reclaimer definitions and the supplied Vista caches. Preserve original
    // bytes at file offsets for raw resources; append a host-owned VA view.
    const Bytes input(storage_);
    auto local=directory(input);
    tag_groups_=std::move(local.groups);
    scenario_tag_={local.scenario};globals_tag_={local.globals};
    name_=input.string(420,32);build_=vista_build;scenario_=input.string(456,256);
    {
        const auto manifest=shared_directory_.parent_path()/"asset-donors.tsv";
        std::ifstream file(manifest);
        if(file) {
            std::string schema,row,extra;
            if(!std::getline(file,schema) || schema!="H2_VISTA_DONORS_V1" ||
               !std::getline(file,row) || !row.starts_with("xbox\t") || row.size()==5 ||
               row.find_first_of("\t\r\n",5)!=std::string::npos || std::getline(file,extra))
                throw FormatError("invalid Vista texture donor manifest");
            xbox_donor_root_=row.substr(5);
            if(xbox_donor_root_.is_relative())xbox_donor_root_=manifest.parent_path()/xbox_donor_root_;
            if(!std::filesystem::is_regular_file(xbox_donor_root_/"default.xbe") ||
               !std::filesystem::is_regular_file(xbox_donor_root_/"maps/shared.map"))
                throw FormatError("Vista texture donor source is incomplete");
        }
    }
    std::vector<std::byte> shared_bytes;
    Directory shared;
    if(input.u32(32)) {
        shared_bytes=read_shared(shared_directory_/"shared.map");
        shared=directory(Bytes(shared_bytes));
        for(const auto& group:shared.groups) {
            const auto* local_group=tag_group(group.group);
            if(!local_group || local_group->parent!=group.parent || local_group->grandparent!=group.grandparent)
                throw FormatError("Vista local/shared tag-group hierarchy collision");
        }
    }
    tags_=local.tags;
    const auto shared_owner=merge_shared_tag_directory(tags_,shared.tags,
        SharedTagLoadPolicy::unloaded_or_empty);
    validate_tag_directory();
    std::vector<std::size_t> resident;
    for(std::size_t i=0;i<tags_.size();++i)
        if(tags_[i].datum!=0xffffffffu && tags_[i].address && tags_[i].size)resident.push_back(i);
    if(resident.empty())throw FormatError("Vista cache has no resident metadata");
    std::sort(resident.begin(),resident.end(),[&](auto a,auto b){return tags_[a].address<tags_[b].address;});
    vista_metadata_regions_=tag_metadata_regions(tags_);
    const auto first=vista_metadata_regions_.front().first;
    std::uint64_t end=first;
    for(auto i:resident) {
        const auto& tag=tags_[i];
        if(tag.address<end)throw FormatError("overlapping Vista resident tag allocations");
        end=std::uint64_t(tag.address)+tag.size;
    }
    end=std::max(end,std::uint64_t(vista_metadata_regions_.back().first)+vista_metadata_regions_.back().second);
    if(end>0x100000000ull || end-first>128ull*1024*1024)
        throw FormatError("excessive Vista virtual metadata span");
    const auto base=storage_.size();
    // Copy before resizing: input holds a span into storage_.
    std::vector<std::byte> metadata(std::size_t(end-first));
    for(auto i:resident) {
        const auto& tag=tags_[i];const Bytes data(shared_owner[i]?shared_bytes:storage_);
        const auto at=TagAddressSpace(data, shared_owner[i]?shared.magic:local.magic).offset(tag.address, tag.size);
        const auto bytes=data.slice(at,tag.size);
        std::copy(bytes.begin(),bytes.end(),metadata.begin()+std::ptrdiff_t(tag.address-first));
    }
    storage_.insert(storage_.end(),metadata.begin(),metadata.end());
    metadata_magic_=std::int64_t(first)-std::int64_t(base);
    const auto table=storage_.size();
    storage_.resize(table+tags_.size()*16);
    for(std::size_t i=0;i<tags_.size();++i) {
        word(storage_,table+i*16+4,tags_[i].datum);
        word(storage_,table+i*16+8,tags_[i].address);
        word(storage_,table+i*16+12,tags_[i].size);
    }
    auto tag_for=[&](std::uint32_t id,const char* expected)->const Tag& {
        const auto* tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(expected),
            .require_address=true, .minimum_size=1});
        if(!tag)
            throw FormatError(std::string("missing resident Vista ")+expected+" tag");
        return *tag;
    };
    if(local.scenario==0xffffffffu)return;
    const auto& scenario_tag=tag_for(local.scenario,"scnr");
    auto scnr=metadata_offset(scenario_tag.address,0x218);
    if(original_data_ && original_data_->multiplayer_netgame_postprocess_constants) {
        multiplayer_netgame_writes_=apply_multiplayer_netgame_postprocess(
            {storage_,metadata_magic_,scenario_tag.address},*original_data_->multiplayer_netgame_postprocess_constants,
            original_data_->image==OriginalData::Image::TitleUpdate15?
                MultiplayerNetgamePostprocessImage::TitleUpdate15:MultiplayerNetgamePostprocessImage::MapPack11).stores;
    }
    if(original_data_ && original_data_->multiplayer_balance_constants && Bytes(storage_).u16(scnr+0x10)==1) {
        multiplayer_balance_writes_=apply_multiplayer_balance(
            {storage_,metadata_magic_,table,tags_.size(),scenario_tag.address,tag_for(local.globals,"matg").address},
            *original_data_->multiplayer_balance_constants,
            original_data_->image==OriginalData::Image::TitleUpdate15?0x13e0:0).stores;
    }
    // Each BSP view duplicates only its root at the existing host offset16.
    // Nested data keeps its original virtual pointers and exact Vista bytes.
    // The lightmap is a separate Vista tag; include its real allocation too.
    struct PendingBsp { std::uint32_t datum,lightmap; };
    std::vector<PendingBsp> pending;
    {
        const Bytes data(storage_);
        const auto count=data.u32(scnr+0x210);
        if(count>256)throw FormatError("excessive Vista BSP count");
        if(count) {
            const auto at=metadata_offset(data.u32(scnr+0x214),std::size_t(count)*0x44);
            for(std::size_t i=0;i<count;++i)pending.push_back({data.u32(at+i*0x44+0x14),data.u32(at+i*0x44+0x1c)});
        }
    }
    for(const auto& selected:pending) {
        const auto& bsp=tag_for(selected.datum,"sbsp");
        if(bsp.size<0x23c)throw FormatError("truncated Vista BSP root");
        const Tag* ltmp=selected.lightmap==0xffffffffu?nullptr:&tag_for(selected.lightmap,"ltmp");
        const auto low=ltmp?std::min(bsp.address,ltmp->address):bsp.address;
        const auto high=ltmp?std::max(std::uint64_t(bsp.address)+bsp.size,std::uint64_t(ltmp->address)+ltmp->size):std::uint64_t(bsp.address)+bsp.size;
        if(low<4096 || high-low>128ull*1024*1024)throw FormatError("excessive Vista BSP/lightmap span");
        const auto magic=low-4096;
        std::vector<std::byte> view(std::size_t(high-magic));
        const Bytes data(storage_);
        for(const auto* tag:{&bsp,ltmp})if(tag) {
            const auto bytes=data.slice(metadata_offset(tag->address,tag->size),tag->size);
            std::copy(bytes.begin(),bytes.end(),view.begin()+std::ptrdiff_t(tag->address-magic));
        }
        const auto root=data.slice(metadata_offset(bsp.address,0x23c),0x23c);
        std::copy(root.begin(),root.end(),view.begin()+16);
        word(view,0,std::uint32_t(view.size()));word(view,4,bsp.address);
        word(view,8,ltmp?ltmp->address:0xffffffffu);
        if(storage_.size()+view.size()>1024ull*1024*1024)throw FormatError("excessive owned Vista cache view");
        bsps_.push_back({std::uint32_t(storage_.size()),std::uint32_t(view.size()),magic,bsp.datum,bsp.name});
        storage_.insert(storage_.end(),view.begin(),view.end());
    }
    const Bytes data(storage_);
    scnr=metadata_offset(scenario_tag.address,0x218);
    const auto count=data.u32(scnr+0x100);
    if(count>65536)throw FormatError("excessive Vista player starts");
    if(!count)return;
    const auto starts=metadata_offset(data.u32(scnr+0x104),std::size_t(count)*0x34);
    for(std::size_t i=0;i<count;++i) {
        const auto at=starts+i*0x34;
        Spawn spawn{{data.f32(at),data.f32(at+4),data.f32(at+8)},data.f32(at+12),data.u16(at+0x12)};
        if(!std::isfinite(spawn.position.x) || !std::isfinite(spawn.position.y) || !std::isfinite(spawn.position.z) || !std::isfinite(spawn.facing))
            throw FormatError("nonfinite Vista player start");
        spawn.team=std::int16_t(data.u16(at+0x10));spawn.player_type=std::int16_t(data.u16(at+0x2c));
        for(unsigned j=0;j<4;++j) {
            spawn.game_types[j]=std::int16_t(data.u16(at+0x14+j*2));
            spawn.spawn_types[j]=std::int16_t(data.u16(at+0x1c+j*2));
        }
        spawns_.push_back(spawn);
    }
}

std::pair<const Cache*,std::uint32_t> Cache::vista_texture_donor(std::uint32_t datum) const {
    if(!vista_)throw FormatError("Xbox texture donor requested for a non-Vista cache");
    const auto* tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::bitmap, .require_address=true});
    if(!tag)
        throw FormatError("missing Vista bitmap reference for Xbox donor");
    const auto& source=*tag;
    if(xbox_donor_root_.empty())throw FormatError("Vista bitmap requires configured original Xbox texture donor: "+source.name);
    const std::array names{path_.filename(),std::filesystem::path("shared.map"),
        std::filesystem::path("single_player_shared.map"),std::filesystem::path("mainmenu.map")};
    for(const auto& name:names) {
        const auto candidate=xbox_donor_root_/"maps"/name;
        if(!std::filesystem::is_regular_file(candidate))continue;
        auto found=std::find_if(vista_texture_donors_.begin(),vista_texture_donors_.end(),[&](const auto& c){return c->path_==candidate;});
        if(found==vista_texture_donors_.end()) {
            auto cache=std::make_shared<Cache>(candidate);
            if(cache->is_vista())throw FormatError("texture donor must be an original Xbox cache");
            vista_texture_donors_.push_back(std::move(cache));found=std::prev(vista_texture_donors_.end());
        }
        const auto& donor=**found;
        const auto tag=donor.query_tag({.group=tag_groups::bitmap, .name=source.name, .require_address=true, .minimum_size=1});
        if(tag && donor.bitmap_count(tag->datum)==bitmap_count(datum))
            return {&donor,tag->datum};
    }
    throw FormatError("no exact-path/frame-count Xbox texture donor: "+source.name);
}
} // namespace halo2
