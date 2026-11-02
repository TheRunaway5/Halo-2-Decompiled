#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include "halo2/mcc_sound_vanilla_reference.hpp"
#include <algorithm>
#include <map>
#include <openssl/evp.h>
#include <set>

namespace halo2 {namespace {
constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){require(ctx&&EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"Original sound publication digest failed");}
    void bytes(std::span<const std::byte> b){require(b.empty()||EVP_DigestUpdate(ctx.get(),b.data(),b.size())==1,"Original sound publication digest failed");}
    void word(std::uint32_t x){std::array<std::byte,4> b;for(unsigned n=0;n<4;++n)b[n]=std::byte(x>>(8*n));bytes(b);}
    void wide(std::uint64_t x){word(std::uint32_t(x));word(std::uint32_t(x>>32));}
    void text(std::string_view x){wide(x.size());bytes(std::as_bytes(std::span(x.data(),x.size())));}
    auto finish(){std::array<std::byte,32> out;unsigned size{};require(EVP_DigestFinal_ex(ctx.get(),reinterpret_cast<unsigned char*>(out.data()),&size)==1&&size==32,"Original sound publication digest failed");return out;}
};
auto digest(const MccSoundRuntimePlan& p){
    Hash h;h.word(p.address);h.wide(p.bytes.size());h.bytes(p.bytes);
    for(const auto& b:p.blocks){h.word(std::uint32_t(b.count));h.word(b.address);}
    h.wide(p.patches.size());for(const auto& x:p.patches){h.word(x.owner.value);h.word(x.source_address);h.wide(x.before.size());h.bytes(x.before);h.wide(x.after.size());h.bytes(x.after);}
    h.wide(p.gestalts.size());for(const auto& g:p.gestalts){h.word(g.owner.value);for(auto x:g.first)h.word(x);for(auto x:g.count)h.word(x);}
    h.wide(p.chunks.size());for(const auto& c:p.chunks){h.word(c.gestalt.value);h.word(c.source_address);h.word(c.native_address);h.word(c.source_index);h.word(c.file_offset);h.word(c.encoded_bytes);h.word(c.decoded_bytes);h.word(c.reset_after);}
    h.wide(p.sounds.size());for(const auto& s:p.sounds){h.word(s.sound.value);h.word(s.gestalt.value);h.word(s.codec.sample_rate);h.word(s.codec.encoding);h.word(s.codec.compression);h.word(unsigned(s.file));h.word(s.remastered_index);h.bytes(s.mcc_extension);h.word(s.source_root);h.word(s.native_root);h.word(s.source_flags);h.word(s.native_flags);}
    h.wide(p.unused_flag_indices.size());for(const auto& u:p.unused_flag_indices){h.word(u.gestalt.value);h.word(u.range_index);h.word(u.encoded_index);h.wide(u.users.size());for(const auto& x:u.users){h.word(x.sound.value);h.word(x.mcc_class.value);h.word(x.native_class.value);h.word(x.class_index);h.word(x.mcc_flags);h.word(x.native_flags);}}
    h.wide(p.native_pcm_duration.size());for(auto x:p.native_pcm_duration)h.word(x.value);
    h.word(bool(p.authored_flags));if(p.authored_flags){h.wide(p.authored_flags->corrections.size());for(const auto& c:p.authored_flags->corrections){h.word(c.sound.value);h.word(c.gestalt.value);h.word(c.source_root);h.word(c.source_flags);h.word(c.native_before);h.word(c.native_after);}h.wide(p.authored_flags->owners.size());for(const auto& o:p.authored_flags->owners){h.word(o.gestalt.value);h.word(unsigned(o.status));}}
    return h.finish();
}
std::span<const std::byte> read(const MccSoundRuntimePlan& p,std::uint32_t at,std::size_t size){
    require(at>=p.address&&std::uint64_t(at)-p.address<=p.bytes.size()&&size<=p.bytes.size()-(std::uint64_t(at)-p.address),"Original sound prefix pointer escapes its actual native plan");return std::span(p.bytes).subspan(at-p.address,size);
}
}
struct MccOriginalSoundRuntimeDeliveryTu15::Impl {
    MccSoundRuntimePlan parent;
    std::shared_ptr<const OriginalSoundRuntimeBankTu15> bank;
    std::vector<OriginalSoundRuntimeBindingTu15> bindings;
    std::array<std::byte,32> seal;
    std::array<std::byte,32> resources;
};
MccOriginalSoundRuntimeDeliveryTu15::MccOriginalSoundRuntimeDeliveryTu15(std::unique_ptr<Impl> p):impl_(std::move(p)){}
MccOriginalSoundRuntimeDeliveryTu15::~MccOriginalSoundRuntimeDeliveryTu15()=default;
const MccSoundRuntimePlan& MccOriginalSoundRuntimeDeliveryTu15::parent()const{return impl_->parent;}
std::shared_ptr<const OriginalSoundRuntimeBankTu15> MccOriginalSoundRuntimeDeliveryTu15::bank()const{return impl_->bank;}
std::span<const OriginalSoundRuntimeBindingTu15> MccOriginalSoundRuntimeDeliveryTu15::bindings()const{return impl_->bindings;}
bool MccOriginalSoundRuntimeDeliveryTu15::contains(TagHandle h)const{return std::ranges::find(impl_->bindings,h,&OriginalSoundRuntimeBindingTu15::sound)!=impl_->bindings.end();}
const std::array<std::byte,32>& MccOriginalSoundRuntimeDeliveryTu15::resource_identity()const{return impl_->resources;}
std::array<std::byte,32> MccOriginalSoundRuntimeDeliveryTu15::bind_resource_identity(const std::array<std::byte,32>& resident)const{
    Hash h;h.text("Halo 2 original sound resident binding v1");h.bytes(resident);h.bytes(impl_->resources);return h.finish();
}
std::optional<std::vector<std::byte>> MccOriginalSoundRuntimeDeliveryTu15::read_cache_resource(const SourceSoundCacheReadTu15& request)const{
    const auto& bank=*impl_->bank;const auto table=bank.block(8);
    require(request.descriptor>=table.address&&(request.descriptor-table.address)%12u==0&&
        (request.descriptor-table.address)/12u<std::uint32_t(table.count),
        "Original sound publication read has no admitted physical descriptor");
    if((request.descriptor-table.address)/12u>=bank.original_chunk_first())return bank.read_cache_resource(request);
    const Bytes row(bank.bytes().subspan(request.descriptor-bank.address(),12));const auto raw=row.u32(4)&0x3fffffffu;
    require(raw&&request.encoded_offset==row.u32(0)&&request.size_flags==row.u32(4)&&request.bytes==((raw+511u)&~511u),
        "Native sound prefix read differs from its admitted external descriptor route");
    return std::nullopt;
}
void MccOriginalSoundRuntimeDeliveryTu15::require_plan(const MccSoundRuntimePlan& p)const{
    require(p.original.get()==this&&p.authored_flags==impl_->parent.authored_flags&&digest(p)==impl_->seal,
        "Original sound publication lineage or immutable native plan changed");
}
MccSoundRuntimePlan stage_mcc_original_sound_runtime_delivery_tu15(const MccSoundRuntimeSource& input,const MccSoundRuntimePlan& supplied,
    const MccOriginalSoundRuntimeInputTu15& original,std::uint32_t available){
    require(!supplied.original&&!original.bindings.empty(),"Original sound publication requires one complete base plan and nonempty bindings");
    require(input.tags.size()<=65536,"Original sound publication directory exceeds its datum space");
    // Own all caller containers before copying any callable wrapper.
    auto state=std::make_unique<MccOriginalSoundRuntimeDeliveryTu15::Impl>();state->parent=supplied;
    state->bindings.assign(original.bindings.begin(),original.bindings.end());
    if(state->parent.authored_flags)state->parent.authored_flags=std::make_shared<const MccSoundVanillaFlagPlan>(*state->parent.authored_flags);
    auto tags=std::make_shared<std::vector<Tag>>(input.tags.begin(),input.tags.end());
    const auto strings=original.strings;auto source=input;source.tags=*tags;auto& parent=state->parent;
    require(source.read&&source.relocate&&source.gestalt_owner&&parent.address&&!(parent.address&3u)&&!parent.bytes.empty()&&
        parent.bytes.size()<=available&&std::uint64_t(parent.address)+available<=0x100000000ull,
        "Original sound publication lacks its real source or reserved native range");
    std::map<std::uint32_t,const Tag*> directory;std::set<std::uint16_t> slots;const Tag* globals{};
    for(const auto& t:*tags)if(!t.handle().is_null()){
        require(directory.emplace(t.datum,&t).second&&slots.insert(t.handle().index()).second,"Original sound publication aliases a full-salt directory slot");
        if(t.address&&t.group=="matg"){require(!globals,"Original sound publication has ambiguous globals");globals=&t;}
    }
    const auto tag=[&](TagHandle h)->const Tag&{const auto i=directory.find(h.value);require(!h.is_null()&&i!=directory.end()&&i->second->address,"Original sound publication lost a resident full-salt owner");return *i->second;};
    std::map<std::uint32_t,std::size_t> patches;std::vector<std::pair<std::uint64_t,std::uint64_t>> roots;
    for(std::size_t n=0;n<parent.patches.size();++n){const auto& p=parent.patches[n];const auto& t=tag(p.owner);
        const auto size=t.group=="snd!"?20u:t.group=="ugh!"?88u:t.group=="sncl"?8u:0u;
        require(size&&t.size>=size&&std::uint64_t(t.address)+size<=0x100000000ull&&p.source_address==t.address&&p.before.size()==size&&p.after.size()==size&&patches.emplace(t.datum,n).second,
            "Original sound publication has an invalid, duplicate or untyped root patch");
        const auto raw=source.read(t.address,size);require(raw.size()==size&&std::ranges::equal(raw,p.before),"Original sound publication raw root preimage changed");
        const auto target=source.relocate(t.address,size);require(target&&std::uint64_t(target)+size<=0x100000000ull&&
            (std::uint64_t(target)+size<=parent.address||std::uint64_t(parent.address)+available<=target),
            "Original sound publication root aliases its reserved child-bank allocation");
        roots.emplace_back(target,std::uint64_t(target)+size);
    }
    std::ranges::sort(roots);for(unsigned n=1;n<roots.size();++n)require(roots[n-1].second<=roots[n].first,"Original sound publication root owners overlap");
    OriginalSoundRuntimePrefixTu15 prefix;
    auto retained=std::make_shared<const MccSoundRuntimePlan>(parent);prefix.owner=retained;
    std::vector<std::pair<std::uint64_t,std::uint64_t>> intervals;
    for(unsigned n=0;n<11;++n){const auto b=parent.blocks[n];require(b.count>=0&&std::uint32_t(b.count)<=0x100000u,"Original sound prefix table count is invalid");
        prefix.tables[n]=read(parent,b.address,std::size_t(b.count)*strides[n]);
        if(b.count)intervals.emplace_back(b.address,std::uint64_t(b.address)+prefix.tables[n].size());
    }
    std::ranges::sort(intervals);for(unsigned n=1;n<intervals.size();++n)require(intervals[n-1].second<=intervals[n].first,"Original sound prefix tables overlap");
    require(globals&&globals->name=="globals\\globals"&&globals->size>=0xc8u&&std::uint64_t(globals->address)+0xc8u<=0x100000000ull,"Original sound publication lacks its actual globals owner");
    const Bytes sg_root(source.read(globals->address+0xc0u,8));require(sg_root.u32(0)==1,"Original sound publication SoundGlobals is not singular");
    const Bytes sg(source.read(sg_root.u32(4),44));const TagHandle legacy{sg.u32(12)};const auto& cls=tag(legacy);
    require(sg.u32(8)==TagGroup{"sncl"}.value&&cls.group=="sncl"&&patches.contains(legacy.value),"Original sound publication lost its typed Legacy class route");
    const Bytes classes(parent.patches[patches.at(legacy.value)].after);require(classes.u32(0)==54,"Original sound publication requires the complete native class bank");
    prefix.sound_classes=read(parent,classes.u32(4),54*92u);
    std::array<std::byte,88> old_root{};
    for(unsigned n=0;n<11;++n)for(unsigned b=0;b<4;++b){old_root[n*8+b]=std::byte(std::uint32_t(parent.blocks[n].count)>>(b*8));old_root[n*8+4+b]=std::byte(parent.blocks[n].address>>(b*8));}
    for(const auto& [id,t]:directory)if(t->address&&(t->group=="ugh!"||t->group=="snd!"||t->group=="sncl")){
        require(patches.contains(id),"Original sound publication omits a resident native sound root");
        if(t->group=="ugh!")require(std::ranges::equal(parent.patches[patches.at(id)].after,old_root),"Original sound prefix gestalt root differs from its complete table bank");
    }
    std::map<std::uint32_t,std::size_t> definitions;
    for(std::size_t n=0;n<parent.sounds.size();++n){const auto& s=parent.sounds[n];const auto& t=tag(s.sound);const auto& g=tag(s.gestalt);const auto& p=parent.patches[patches.at(s.sound.value)];const Bytes native(p.after),raw(p.before);
        require(t.group=="snd!"&&g.group=="ugh!"&&definitions.emplace(s.sound.value,n).second&&s.source_root==t.address&&
            s.native_root==source.relocate(t.address,20)&&source.gestalt_owner(s.sound)==s.gestalt&&s.source_flags==raw.u16(0)&&s.native_flags==native.u16(0)&&
            s.codec.sample_rate==std::to_integer<unsigned>(p.after[3])&&s.codec.encoding==std::to_integer<unsigned>(p.after[4])&&s.codec.compression==std::to_integer<unsigned>(p.after[5])&&s.file!=MccSoundDataFile::Original,
            "Original sound prefix definition has a foreign root, codec, flags or owner");
    }
    for(const auto& [id,t]:directory)if(t->address&&t->group=="snd!")require(definitions.contains(id),"Original sound prefix omits a resident definition");
    auto native_tags=std::make_shared<std::vector<Tag>>();for(const auto& [id,t]:directory)if(t->address&&t->group=="snd!"){
        auto projected=*t;projected.address=source.relocate(t->address,t->size);
        require(projected.address==parent.sounds[definitions.at(id)].native_root,"Original sound full native allocation and root relocation disagree");native_tags->push_back(std::move(projected));}
    for(const auto& b:state->bindings){require(b.bank&&definitions.contains(b.sound.value),"Original sound publication has an unbound target");const auto& t=tag(b.sound);
        require(t.name==b.bank->metadata().name&&!(Bytes(source.read(t.address,20)).u16(0)&0x800u),"Original sound publication has no exact Classic target");}
    const auto offset=(std::uint64_t(parent.bytes.size())+3u)&~3ull;require(offset<available,"Original sound append exhausts its reserved metadata range");
    auto graph=OriginalSoundRuntimeBankTu15::create(std::uint32_t(parent.address+offset),std::uint32_t(available-offset),native_tags,state->bindings,strings,prefix);
    MccSoundRuntimePlan result=parent;result.bytes.resize(std::size_t(offset));result.bytes.insert(result.bytes.end(),graph->bytes().begin(),graph->bytes().end());
    for(unsigned n=0;n<11;++n)result.blocks[n]=graph->block(n);
    const auto root=graph->bytes().subspan(graph->gestalt_root()-graph->address(),88);
    for(auto& p:result.patches)if(tag(p.owner).group=="ugh!")p.after.assign(root.begin(),root.end());
    std::set<std::uint32_t> targets;
    for(const auto& b:state->bindings){targets.insert(b.sound.value);auto& s=result.sounds[definitions.at(b.sound.value)];auto& p=result.patches[patches.at(b.sound.value)];
        const auto bytes=graph->bytes().subspan(graph->sound_root(b.sound)-graph->address(),20);p.after.assign(bytes.begin(),bytes.end());s.native_flags=Bytes(bytes).u16(0);
        s.codec={std::to_integer<std::uint8_t>(bytes[3]),std::to_integer<std::uint8_t>(bytes[4]),std::to_integer<std::uint8_t>(bytes[5])};s.file=MccSoundDataFile::Original;
    }
    std::set<std::uint32_t> chunk_addresses;
    require(parent.chunks.size()==std::size_t(parent.blocks[8].count),"Original sound prefix chunk route closure is incomplete");
    for(auto& c:result.chunks){require(c.native_address>=parent.blocks[8].address&&(c.native_address-parent.blocks[8].address)%12u==0&&
        (c.native_address-parent.blocks[8].address)/12u<std::uint32_t(parent.blocks[8].count)&&chunk_addresses.insert(c.native_address).second,
        "Original sound prefix has a duplicate or foreign physical chunk route");c.native_address=result.blocks[8].address+(c.native_address-parent.blocks[8].address);}
    std::erase_if(result.native_pcm_duration,[&](TagHandle h){return targets.contains(h.value);});
    state->bank=std::move(graph);state->seal=digest(result);
    Hash resources;resources.text("Halo 2 original sound publication resources v1");resources.bytes(state->seal);resources.wide(state->bindings.size());
    for(const auto& binding:state->bindings){const auto& metadata=binding.bank->metadata();
        require(bool(metadata.origin),"Original sound resource identity lost its admitted provenance");const auto& origin=*metadata.origin;
        resources.word(binding.sound.value);resources.text(metadata.name);resources.text(origin.map);resources.text(origin.map_sha256);
        resources.word(origin.sound.value);resources.word(origin.gestalt.value);resources.word(origin.classes.value);
        const auto sectors=binding.bank->sectors();resources.wide(sectors.size());resources.bytes(sectors);
    }
    state->resources=resources.finish();
    result.original=std::shared_ptr<const MccOriginalSoundRuntimeDeliveryTu15>(new MccOriginalSoundRuntimeDeliveryTu15(std::move(state)));
    result.original->require_plan(result);return result;
}
}
