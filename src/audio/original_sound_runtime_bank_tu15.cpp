#include "halo2/original_sound_runtime_bank_tu15.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <set>

namespace halo2 {
namespace {
constexpr std::array<unsigned,11> strides{56,20,4,10,12,16,52,1,12,28,44};
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::uint32_t word(std::span<const std::byte> bytes,unsigned at,unsigned width=4){
    Bytes(bytes).require(at,width);std::uint32_t out{};for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(bytes[at+n])<<(n*8u);return out;
}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value,unsigned width=4){
    Bytes(bytes).require(at,width);for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8u));
}
std::vector<std::byte> copy_bytes(std::span<const std::byte> in){return{in.begin(),in.end()};}
}
struct OriginalSoundRuntimeBankTu15::Impl {
    struct Sound {std::uint32_t root{};std::shared_ptr<const PreparedOriginalSoundBank> original;};
    struct Resource {std::uint32_t offset{},bytes{},sector_bytes{},flags{};};
    std::uint32_t base{},budget{},ugh{},classes{},resource_first{};
    std::vector<std::byte> data,resources;
    std::array<TagBlock,11> blocks{};
    std::map<std::uint32_t,Sound> sounds;
    std::vector<Resource> chunks;
    std::shared_ptr<const std::vector<Tag>> tag_owner;
    std::shared_ptr<const void> string_owner,prefix_owner;
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const{
        require(at>=base&&std::uint64_t(at)-base<=data.size()&&n<=data.size()-(std::uint64_t(at)-base),"Original native sound read escapes its real allocation");return std::span(data).subspan(at-base,n);
    }
    std::span<std::byte> memory(std::uint32_t at,std::size_t n){(void)read(at,n);return std::span(data).subspan(at-base,n);}
    TagBlock current(unsigned table)const{
        require(table<11,"Original native sound table selector is invalid");const Bytes root(read(ugh+table*8u,8));
        const auto count=root.u32(0),at=root.u32(4);require(count<=0x100000u&&std::uint64_t(at)+std::uint64_t(count)*strides[table]<=0x100000000ull,"Original native sound descriptor wraps its owner");
        if(count)(void)read(at,std::size_t(count)*strides[table]);return{std::int32_t(count),at};
    }
    std::uint32_t append(std::span<const std::byte> raw){
        const auto offset=(std::uint64_t(data.size())+3u)&~3ull;
        require(offset+raw.size()<=budget&&std::uint64_t(base)+offset+raw.size()<=0x100000000ull,"Original native sound allocation exceeds its reserved range");
        data.resize(std::size_t(offset));const auto address=std::uint32_t(std::uint64_t(base)+offset);data.insert(data.end(),raw.begin(),raw.end());return address;
    }
    std::span<const std::byte> row(unsigned table,unsigned index)const{
        const auto b=current(table);require(index<unsigned(b.count),"Original native sound table index escapes its owner");return read(b.address+index*strides[table],strides[table]);
    }
    std::span<const std::byte> class_row(unsigned index)const{
        const Bytes root(read(classes,8));const auto count=root.u32(0),at=root.u32(4);
        require(count==54&&index<count&&std::uint64_t(at)+std::uint64_t(count)*92u<=0x100000000ull,
            "Original native class descriptor changed extent or wraps its owner");
        (void)read(at,std::size_t(count)*92u);return read(at+index*92u,92);
    }
    const Sound& sound(TagHandle h)const{const auto i=sounds.find(h.value);require(i!=sounds.end(),"Original native sound has no exact full-salt binding");return i->second;}
};
OriginalSoundRuntimeBankTu15::OriginalSoundRuntimeBankTu15(std::unique_ptr<Impl> p):impl_(std::move(p)){}
OriginalSoundRuntimeBankTu15::~OriginalSoundRuntimeBankTu15()=default;
std::shared_ptr<OriginalSoundRuntimeBankTu15> OriginalSoundRuntimeBankTu15::create(
    std::uint32_t base,std::uint32_t budget,std::shared_ptr<const std::vector<Tag>> tags,
    std::span<const OriginalSoundRuntimeBindingTu15> input,const OriginalSoundRuntimeStringsTu15& strings,
    const OriginalSoundRuntimePrefixTu15& prefix){
    require(base&&base!=0xffffffffu&&!(base&3u)&&budget&&std::uint64_t(base)+budget<=0x100000000ull,"Original native sound range is null, unaligned or wraps");
    require(tags&&!input.empty()&&strings.owner&&strings.resolve&&strings.lookup,"Original native sound construction requires real namespace/string owners");
    const std::vector<OriginalSoundRuntimeBindingTu15> bindings(input.begin(),input.end());
    const auto native_tags=tags;const auto string_owner=strings.owner;
    // Retain and copy all prefix views before copying/invoking caller string
    // functions. Prefix nested pointers keep their existing real allocation.
    const auto prefix_owner=prefix.owner;
    std::array<std::vector<std::byte>,11> prefix_rows;
    std::array<std::uint32_t,11> prefix_counts{};
    require(prefix_owner?prefix.sound_classes.size()==54*92u:
        prefix.sound_classes.empty()&&std::ranges::all_of(prefix.tables,[](const auto& r){return r.empty();}),
        "Original native prefix requires its complete retained class/table owner");
    std::uint64_t prefix_size{};
    for(unsigned n=0;n<11;++n){
        prefix_size+=prefix.tables[n].size();require(prefix_size<=budget,
            "Original native prefix tables exceed their reserved metadata budget");
    }
    const auto prefix_classes=copy_bytes(prefix.sound_classes);
    for(unsigned n=0;n<11;++n){
        require(prefix.tables[n].size()%strides[n]==0&&prefix.tables[n].size()/strides[n]<=0x100000u,
            "Original native prefix has an invalid table stride or extent");
        prefix_rows[n]=copy_bytes(prefix.tables[n]);prefix_counts[n]=std::uint32_t(prefix_rows[n].size()/strides[n]);
    }
    std::map<std::uint32_t,Tag> directory;std::set<std::uint16_t> directory_indices;
    for(const auto& tag:*native_tags)require(tag.datum!=0xffffffffu&&directory.emplace(tag.datum,tag).second&&directory_indices.insert(TagHandle{tag.datum}.index()).second,
        "Duplicate or invalid native sound namespace identity/index");
    std::set<std::uint16_t> native_indices;std::set<std::uint32_t> originals;
    const auto& first=bindings.front().bank;require(bool(first),"Original native sound binding has no prepared bank");
    const auto& initial=first->metadata();require(initial.origin&&initial.sound_classes,"Prepared sound lacks its complete original namespace/class owner");
    const auto origin=*initial.origin;const auto original_classes=*initial.sound_classes;auto classes=original_classes;
    if(prefix_owner)for(unsigned n=0;n<54;++n)std::copy_n(prefix_classes.begin()+n*92u,92,classes[n].begin());
    std::array<std::map<std::uint32_t,std::vector<std::byte>>,11> rows;
    struct Chunk {std::vector<std::byte> literal,sector;std::uint32_t size{};};
    std::map<std::uint32_t,Chunk> chunk_rows;
    std::map<std::uint32_t,PreparedOriginalSoundBank::Promotion> promotions;
    std::map<std::uint32_t,bool> used_storage;
    auto insert=[&](unsigned table,std::uint32_t index,std::span<const std::byte> value){
        auto [i,added]=rows[table].emplace(index,copy_bytes(value));require(added||std::ranges::equal(i->second,value),"Conflicting original table alias data");
    };
    // Copy all namespaces/data before invoking caller-owned string operations.
    for(const auto& binding:bindings){
        require(binding.bank&&!binding.sound.is_null()&&native_indices.insert(binding.sound.index()).second,"Original native sound binding has a missing bank or duplicate native index");
        const auto found=directory.find(binding.sound.value);const auto& m=binding.bank->metadata();
        require(found!=directory.end()&&found->second.group=="snd!"&&found->second.address&&found->second.size>=20&&found->second.name==m.name,"Original sound binding has no exact typed native tag owner");
        require(m.origin&&m.sound_classes&&m.origin->map==origin.map&&m.origin->map_sha256==origin.map_sha256&&
            m.origin->gestalt==origin.gestalt&&m.origin->classes==origin.classes&&*m.sound_classes==original_classes,"Original sound bindings mix source namespaces or class owners");
        require(classes[std::to_integer<unsigned>(m.sound[2])]==m.sound_class,
            "Original sound selected class differs from the retained native prefix");
        require(originals.insert(m.origin->sound.value).second,"Original sound identity was rebound to a second native tag");
        insert(0,word(m.sound,6,2),m.playback);insert(1,word(m.sound,11,1),m.scale);
        for(unsigned p=0;p<m.pitch_ranges.size();++p){const auto& range=m.pitch_ranges[p];
            require(range.import_name_id.has_value(),"Original pitch import name has no real owner");
            std::array<std::byte,4> id{};put(id,0,*range.import_name_id);insert(2,word(range.literal,0,2),id);
            insert(3,word(range.literal,2,2),range.interval);insert(4,word(m.sound,8,2)+p,range.literal);
            auto [used,added]=used_storage.emplace(word(m.sound,8,2)+p,range.used.has_value());require(added||used->second==range.used.has_value(),"Conflicting original used-state ownership");
            if(range.used)for(unsigned n=0;n<range.used->size();++n)insert(7,(word(range.literal,6,2)>>3u)+n,std::span(*range.used).subspan(n,1));
            for(unsigned n=0;n<range.permutations.size();++n){const auto& permutation=range.permutations[n];
                require(permutation.import_name_id.has_value(),"Original permutation import name has no real owner");
                put(id,0,*permutation.import_name_id);insert(2,word(permutation.literal,0,2),id);insert(5,word(range.literal,8,2)+n,permutation.literal);
                for(unsigned c=0;c<permutation.chunks.size();++c){const auto& part=permutation.chunks[c];require(part.literal.has_value(),"Original sound chunk lacks its literal source record");
                    const auto sector=binding.bank->sectors().subspan(part.offset,part.sector_bytes);
                    const auto index=word(permutation.literal,12,2)+c;Chunk candidate{copy_bytes(*part.literal),copy_bytes(sector),part.bytes};
                    auto [i,added]=chunk_rows.emplace(index,std::move(candidate));
                    require(added||(i->second.literal==copy_bytes(*part.literal)&&std::ranges::equal(i->second.sector,sector)&&i->second.size==part.bytes),"Conflicting original chunk alias data");
                }
            }
        }
        if(m.promotion){const auto index=word(m.sound,12,1);auto [i,added]=promotions.emplace(index,*m.promotion);
            require(added||(i->second.literal==m.promotion->literal&&i->second.rules==m.promotion->rules&&i->second.counters==m.promotion->counters),"Conflicting original promotion alias data");}
    }
    const auto resolve=strings.resolve;const auto lookup=strings.lookup;
    std::map<std::uint32_t,std::string> import_names;
    auto label=[&](std::uint32_t index,std::string_view name){auto [i,added]=import_names.emplace(index,name);require(added||i->second==name,"Original import index has conflicting names");};
    for(const auto& binding:bindings)for(const auto& p:binding.bank->metadata().pitch_ranges){label(word(p.literal,0,2),p.name);for(const auto& v:p.permutations)label(word(v.literal,0,2),v.name);}
    for(auto& [index,raw]:rows[2]){const auto& name=import_names.at(index);const auto target=resolve(word(raw,0),name);require(lookup(target)==name,"Original sound import has no matching real native StringID");put(raw,0,target);}
    auto out=std::make_unique<Impl>();out->base=base;out->budget=budget;out->tag_owner=native_tags;out->string_owner=string_owner;out->prefix_owner=prefix_owner;out->resource_first=prefix_counts[8];
    std::array<std::map<std::uint32_t,std::uint32_t>,11> ordinal;
    for(unsigned table=0;table<11;++table){unsigned next=prefix_counts[table];for(const auto& [old,raw]:rows[table]){(void)raw;ordinal[table].emplace(old,next++);}}
    unsigned next_chunk=prefix_counts[8];for(auto& [old,chunk]:chunk_rows){
        ordinal[8].emplace(old,next_chunk++);const auto offset=out->resources.size();
        require(offset<=0x3fffffffu&&offset+chunk.sector.size()<=0x40000000ull,"Original native resource file exceeds its locator range");
        put(chunk.literal,0,std::uint32_t(offset));rows[8].emplace(old,chunk.literal);
        out->chunks.push_back({std::uint32_t(offset),chunk.size,std::uint32_t(chunk.sector.size()),word(chunk.literal,4)});
        out->resources.insert(out->resources.end(),chunk.sector.begin(),chunk.sector.end());
    }
    unsigned next_promotion=prefix_counts[9];for(const auto& [old,p]:promotions){(void)p;ordinal[9].emplace(old,next_promotion++);}
    auto mapped=[&](unsigned table,std::uint32_t index,unsigned bits){const auto i=ordinal[table].find(index);require(i!=ordinal[table].end()&&i->second<(1u<<(bits-1)),"Original native table ordinal escapes its signed field");return i->second;};
    auto contiguous=[&](unsigned table,std::uint32_t first,unsigned count){for(unsigned n=0;n<count;++n)require(mapped(table,first+n,16)==mapped(table,first,16)+n,"Original native table compaction broke a contiguous interval");};
    for(auto& [old,raw]:rows[4]){(void)old;put(raw,0,mapped(2,word(raw,0,2),16),2);put(raw,2,mapped(3,word(raw,2,2),16),2);
        const auto first=word(raw,8,2),count=word(raw,10,2);contiguous(5,first,count);put(raw,8,mapped(5,first,16),2);
        const auto original=word(raw,6,2),flag=original>>3u;const auto i=ordinal[7].find(flag);
        if(used_storage.at(old)){require(i!=ordinal[7].end(),"Original native used storage lost its real owner");const auto size=(count+7u)/8u;contiguous(7,flag,size);require(i->second<8192,"Original native used byte index exceeds its bit field");put(raw,6,(i->second<<3u)|(original&7u),2);}
    }
    for(auto& [old,raw]:rows[5]){(void)old;put(raw,0,mapped(2,word(raw,0,2),16),2);const auto first=word(raw,12,2),count=word(raw,14,2);contiguous(8,first,count);put(raw,12,mapped(8,first,16),2);}
    for(const auto& [old,p]:promotions){
        std::vector<std::byte> rules,counters;for(const auto& r:p.rules)rules.insert(rules.end(),r.begin(),r.end());for(const auto& c:p.counters)counters.insert(counters.end(),c.begin(),c.end());
        auto raw=copy_bytes(p.literal);put(raw,4,out->append(rules));put(raw,12,out->append(counters));rows[9].emplace(old,std::move(raw));
    }
    for(unsigned table=0;table<11;++table)if(!rows[table].empty()||!prefix_rows[table].empty()){
        auto raw=std::move(prefix_rows[table]);for(const auto& [old,value]:rows[table]){(void)old;require(value.size()==strides[table],"Original native table has another stride");raw.insert(raw.end(),value.begin(),value.end());}
        require(std::uint64_t(prefix_counts[table])+rows[table].size()<=0x100000u,"Combined native sound table exceeds its count bound");
        out->blocks[table]={std::int32_t(prefix_counts[table]+rows[table].size()),out->append(raw)};
    }
    std::array<std::byte,88> ugh{};for(unsigned n=0;n<11;++n){put(ugh,n*8u,std::uint32_t(out->blocks[n].count));put(ugh,n*8u+4u,out->blocks[n].address);}out->ugh=out->append(ugh);
    std::vector<std::byte> class_bytes;for(const auto& row:classes)class_bytes.insert(class_bytes.end(),row.begin(),row.end());const auto class_rows=out->append(class_bytes);
    std::array<std::byte,8> class_root{};put(class_root,0,54);put(class_root,4,class_rows);out->classes=out->append(class_root);
    for(const auto& binding:bindings){const auto& m=binding.bank->metadata();auto raw=m.sound;
        put(raw,6,mapped(0,word(raw,6,2),16),2);put(raw,11,mapped(1,word(raw,11,1),8),1);
        const auto first=word(raw,8,2),count=word(raw,10,1);contiguous(4,first,count);put(raw,8,mapped(4,first,16),2);
        if(word(raw,12,1)!=255)put(raw,12,mapped(9,word(raw,12,1),8),1);
        out->sounds.emplace(binding.sound.value,Impl::Sound{out->append(raw),binding.bank});
    }
    return std::shared_ptr<OriginalSoundRuntimeBankTu15>(new OriginalSoundRuntimeBankTu15(std::move(out)));
}
std::uint32_t OriginalSoundRuntimeBankTu15::address()const{return impl_->base;}
std::span<const std::byte> OriginalSoundRuntimeBankTu15::bytes()const{return impl_->data;}
std::span<std::byte> OriginalSoundRuntimeBankTu15::memory(std::uint32_t p,std::size_t n){return impl_->memory(p,n);}
std::uint32_t OriginalSoundRuntimeBankTu15::gestalt_root()const{return impl_->ugh;}
std::uint32_t OriginalSoundRuntimeBankTu15::class_root()const{return impl_->classes;}
std::uint32_t OriginalSoundRuntimeBankTu15::sound_root(TagHandle sound)const{return impl_->sound(sound).root;}
TagBlock OriginalSoundRuntimeBankTu15::block(unsigned n)const{return impl_->current(n);}
std::uint32_t OriginalSoundRuntimeBankTu15::original_chunk_first()const{return impl_->resource_first;}
SoundVariationDefinition OriginalSoundRuntimeBankTu15::load(TagHandle sound,unsigned pitch)const{
    SoundVariationDefinition d;d.datum=sound.value;const auto& binding=impl_->sound(sound);
    const auto copy=[](auto& to,std::span<const std::byte> from){require(to.size()==from.size(),"Original native metadata row stride differs");std::ranges::copy(from,to.begin());};
    copy(d.sound,impl_->read(binding.root,20));const Bytes root(d.sound);const auto count=word(d.sound,10,1);
    require(pitch<count,"Original native sound pitch escapes its definition");
    const auto cls=word(d.sound,2,1);require(cls<54,"Original native sound class escapes its real owner");
    copy(d.sound_class,impl_->class_row(cls));
    copy(d.playback,impl_->row(0,root.u16(6)));copy(d.scale,impl_->row(1,word(d.sound,11,1)));d.sample_rate_index=word(d.sound,3,1);
    for(unsigned n=0;n<count;++n){const auto range=impl_->row(4,root.u16(8)+n);SoundPitchRange p;copy(p.interval,impl_->row(3,word(range,2,2)));p.permutations=std::bit_cast<std::int16_t>(std::uint16_t(word(range,10,2)));d.pitch_ranges.push_back(p);}
    copy(d.range,impl_->row(4,root.u16(8)+pitch));const auto first=word(d.range,8,2),permutations=word(d.range,10,2);require(permutations>=1&&permutations<=32,"Original native permutation extent is unsupported");
    for(unsigned n=0;n<permutations;++n){auto& p=d.permutations.emplace_back();copy(p,impl_->row(5,first+n));}
    if(permutations>1&&!(word(d.sound_class,10,2)&0x20u)){const auto first_byte=word(d.range,6,2)>>3u,size=(permutations+7u)/8u;for(unsigned n=0;n<size;++n)d.used.push_back(impl_->row(7,first_byte+n)[0]);}
    return d;
}
EncodedSound OriginalSoundRuntimeBankTu15::encoded(TagHandle sound,unsigned pitch,unsigned permutation)const{
    const auto d=load(sound,pitch);require(permutation<d.permutations.size(),"Original native encoded permutation escapes its range");const auto& p=d.permutations[permutation];
    EncodedSound e;e.name=impl_->sound(sound).original->metadata().name;e.sound_class=word(d.sound,2,1);
    const auto rate=word(d.sound,3,1),encoding=word(d.sound,4,1);require(rate<3&&encoding<3,"Original native encoded format changed to an unsupported owner");
    constexpr unsigned rates[]{22050,44100,32000};e.sample_rate=rates[rate];e.channels=encoding==2?2:encoding+1;e.codec=encoding==2?4:word(d.sound,5,1);e.samples=word(p,8);
    const auto first=word(p,12,2),count=word(p,14,2);require(count>0,"Original native permutation lost its resource chunks");
    for(unsigned n=0;n<count;++n){const auto at=first+n;const auto raw=impl_->row(8,at);const auto current=impl_->current(8),initial=impl_->blocks[8];const auto descriptor=current.address+at*12u;
        const auto first=std::uint64_t(initial.address)+std::uint64_t(impl_->resource_first)*12u;
        require(descriptor>=first&&(descriptor-first)%12u==0&&(descriptor-first)/12u<impl_->chunks.size(),"Original native chunk has no real resource owner");const auto& c=impl_->chunks[(descriptor-first)/12u];
        require(word(raw,0)==c.offset&&word(raw,4)==c.flags,"Original native resource descriptor differs from its admitted immutable route");const auto part=std::span(impl_->resources).subspan(c.offset,c.bytes);e.data.insert(e.data.end(),part.begin(),part.end());
        e.packet_chunks.push_back({c.bytes,0,false});}
    return e;
}
void OriginalSoundRuntimeBankTu15::read_resource(std::uint32_t descriptor,std::size_t offset,std::span<std::byte> destination)const{
    const auto b=impl_->blocks[8];require(descriptor>=b.address&&(descriptor-b.address)%12u==0,"Original native resource request has no exact descriptor owner");const auto index=(descriptor-b.address)/12u;
    require(index>=impl_->resource_first&&index-impl_->resource_first<impl_->chunks.size(),"Native prefix resource belongs to its existing external owner");
    const auto& c=impl_->chunks[index-impl_->resource_first];require(offset<=c.sector_bytes&&destination.size()<=c.sector_bytes-offset,"Original native sector read escapes its resource owner");
    const auto raw=impl_->read(descriptor,12);require(word(raw,0)==c.offset&&word(raw,4)==c.flags,"Original native sector descriptor changed its admitted route");std::ranges::copy(std::span(impl_->resources).subspan(c.offset+offset,destination.size()),destination.begin());
}
std::vector<std::byte> OriginalSoundRuntimeBankTu15::read_cache_resource(const SourceSoundCacheReadTu15& request)const{
    const auto table=impl_->blocks[8];require(request.descriptor>=table.address&&(request.descriptor-table.address)%12u==0,
        "Original sound-cache read lacks its exact physical descriptor");
    const auto index=(request.descriptor-table.address)/12u;
    require(index>=impl_->resource_first&&index-impl_->resource_first<impl_->chunks.size(),
        "Original sound-cache read belongs to a different resource owner");
    const auto& resource=impl_->chunks[index-impl_->resource_first];
    require(request.encoded_offset==resource.offset&&request.size_flags==resource.flags&&request.bytes==resource.sector_bytes,
        "Original sound-cache read changed its admitted locator, flags or sector extent");
    std::vector<std::byte> result(request.bytes);read_resource(request.descriptor,0,result);return result;
}
}
