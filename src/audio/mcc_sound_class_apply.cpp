#include "audio/mcc_sound_class_apply.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <set>
#include <tuple>

namespace halo2::mcc_sound_class_detail {
namespace {
void require(bool yes,const char*why){if(!yes)throw FormatError(why);}
constexpr std::array<unsigned,11> strides{0x38,0x14,4,10,12,16,0x34,1,12,0x1c,0x2c};
struct Interval {std::uint64_t begin,end;};
bool overlaps(Interval a,Interval b){return a.begin<b.end&&b.begin<a.end;}
Interval interval(unsigned p,std::size_t n){require(p&&p!=0xffffffffu&&n&&n<=0x100000000ull-p,"Class stage interval is empty/null/wrapped");return {p,std::uint64_t(p)+n};}
struct Reader {
    MccSoundRuntimeSource source;
    std::map<unsigned,Tag> tags;
    Reader(const MccSoundRuntimeSource&s,std::span<const Tag> owned):source(s){
        source.tags=owned;require(source.read&&source.relocate&&source.gestalt_owner,"Class stage needs typed metadata/relocation owners");
        std::set<unsigned> indices;for(const auto&t:owned)if(!t.handle().is_null())
            require(indices.insert(t.handle().index()).second&&tags.emplace(t.datum,t).second,"Class stage namespace salt/index aliases");
    }
    const Tag&tag(TagHandle h,std::string_view group={})const{const auto i=tags.find(h.value);require(!h.is_null()&&i!=tags.end()&&i->second.address&&(!group.size()||i->second.group==group),"Class stage lost full-salt typed owner");return i->second;}
    std::vector<std::byte>read(unsigned p,std::size_t n)const{(void)interval(p,n);const auto b=source.read(p,n);require(b.size()==n,"Class stage partial raw metadata");return {b.begin(),b.end()};}
};
struct Class {Tag tag;std::array<std::byte,8>root;std::vector<std::byte>raw;std::size_t patch{},offset{};unsigned count{};Interval bank;};
void put(std::vector<std::byte>&b,std::size_t at,unsigned value){Bytes(b).require(at,4);for(unsigned n=0;n<4;++n)b[at+n]=std::byte(value>>(n*8));}
}
Result stage(const MccSoundRuntimeSource&input,const MccSoundRuntimePlan&candidate){
    // Caller-owned containers can be released by a callable copy constructor;
    // own them before copying the wrapper. Wrapper remains valid during copy.
    Result out;out.native=candidate;
    const std::vector<Tag> owned_tags(input.tags.begin(),input.tags.end());
    require(owned_tags.size()<=65536,"Class stage excessive namespace");
    Reader r(input,owned_tags);
    auto&native=out.native;const auto allocation=interval(native.address,native.bytes.size());
    std::vector<Interval> occupied;
    for(unsigned n=0;n<11;++n){const auto&b=native.blocks[n];require(b.count>=0,"Class stage negative native count");if(!b.count)continue;const auto range=interval(b.address,std::size_t(b.count)*strides[n]);require(range.begin>=allocation.begin&&range.end<=allocation.end,"Class stage gestalt bank escapes plan");for(const auto old:occupied)require(!overlaps(range,old),"Class stage overlapping merged gestalt banks");occupied.push_back(range);}
    const Tag*globals{};for(const auto&t:owned_tags)if(t.address&&t.group=="matg"){require(!globals,"Class stage nonunique globals");globals=&t;}
    require(globals&&globals->name=="globals\\globals","Class stage has no actual named globals");
    const auto descriptor=r.read(globals->address+0xc0,8);const Bytes d(descriptor);require(d.u32(0)==1,"Class stage nonsingular SoundGlobals");
    const auto sound_globals=r.read(d.u32(4),44);const Bytes sg(sound_globals);
    require(sg.u32(0)==TagGroup{"sncl"}.value&&sg.u32(8)==TagGroup{"sncl"}.value,"Class stage SoundGlobals typed roles differ");
    const TagHandle remastered{sg.u32(4)},legacy{sg.u32(12)};(void)r.tag(remastered,"sncl");(void)r.tag(legacy,"sncl");
    std::map<unsigned,Class> classes;std::set<unsigned> patch_roots;
    for(std::size_t i=0;i<native.patches.size();++i){const auto&p=native.patches[i];const auto&t=r.tag(p.owner);require(p.source_address==t.address&&patch_roots.insert(p.source_address).second,"Class stage native patch root identity/uniqueness differs");
        const unsigned size=t.group=="snd!"?20u:t.group=="ugh!"?88u:t.group=="sncl"?8u:0u;
        require(size&&p.before.size()==size&&p.after.size()==size,"Class stage foreign native patch extent/group");
        const auto raw=r.read(t.address,size);require(raw==p.before,"Class stage root complete raw preimage differs");
        const auto root=r.source.relocate(t.address,t.size);require(root==r.source.relocate(t.address,size),"Class stage complete native/root relocation disagrees");const auto root_range=interval(root,size);require(!overlaps(root_range,allocation),"Class stage native root aliases merged plan");
        if(t.group!="sncl")continue;
        Class c;c.tag=t;c.patch=i;std::ranges::copy(raw,c.root.begin());const Bytes b(raw);c.count=b.u32(0);require(c.count<=0x100000&&c.count>0,"Class stage source class count invalid");c.raw=r.read(b.u32(4),std::size_t(c.count)*120);
        const auto expected=project_mcc_sound_class_root(raw,Bytes(p.after).u32(4));require(std::ranges::equal(expected,p.after),"Class stage class native descriptor differs from truthful source");
        c.bank=interval(Bytes(p.after).u32(4),std::size_t(c.count)*92);require(c.bank.begin>=allocation.begin&&c.bank.end<=allocation.end,"Class stage class bank escapes merged owner");for(const auto old:occupied)require(!overlaps(c.bank,old),"Class stage class allocation aliases another bank");occupied.push_back(c.bank);c.offset=std::size_t(c.bank.begin-allocation.begin);require(classes.emplace(t.datum,std::move(c)).second,"Class stage class root repeated");
    }
    // Every directory class root must have exactly one independently owned
    // native descriptor, including the preserved Remastered class bank.
    for(const auto&[datum,t]:r.tags)if(t.address&&t.group=="sncl")require(classes.contains(datum),"Class stage omitted native class owner");
    const auto selected=classes.find(legacy.value);require(selected!=classes.end(),"Class stage Legacy native root missing");const auto&cls=selected->second;
    const auto pre=std::span(native.bytes).subspan(cls.offset,std::size_t(cls.count)*92);
    out.reference=project_mcc_sound_class_reference(r.source,legacy,pre);
    if(out.reference.status==MccSoundClassReferenceStatus::UnrecognizedSource)return out;
    require(out.reference.status!=MccSoundClassReferenceStatus::CandidateMismatch,"Class stage canonical complete candidate mismatch");
    require(cls.count==54&&out.reference.owner==legacy&&out.reference.native_records.size()==54*92,"Class stage admitted profile lost complete typed owner");
    auto projected=native.bytes;for(const auto&w:out.reference.words){require(w.class_index<54&&w.offset%4==0&&w.offset+4<=92,"Class stage typed word target invalid");const auto at=cls.offset+std::size_t(w.class_index)*92+w.offset;require(Bytes(native.bytes).u32(at)==(out.reference.status==MccSoundClassReferenceStatus::Converted?w.before_bits:w.after_bits),"Class stage typed word preimage differs");put(projected,at,w.after_bits);}
    require(std::ranges::equal(std::span(projected).subspan(cls.offset,54*92),out.reference.native_records),"Class stage complete projected postimage differs");
    // Re-enumerate every incoming user for the retained unused-flag receipt.
    // No consumer with changed class predicates may escape this all-user check.
    using Key=std::pair<unsigned,unsigned>;std::map<Key,std::vector<MccSoundRuntimeFlagUser>> users;
    std::map<unsigned,std::pair<unsigned,std::vector<std::byte>>> ranges;
    for(const auto&g:native.gestalts){const auto&t=r.tag(g.owner,"ugh!");const auto root=r.read(t.address,120);const Bytes b(root);require(b.u32(0x28)==g.count[4],"Class stage raw/native pitch count differs");ranges.emplace(g.owner.value,std::pair(b.u32(0x2c),r.read(b.u32(0x2c),std::size_t(g.count[4])*12)));}
    std::set<unsigned> definitions;
    for(const auto&s:native.sounds){require(definitions.insert(s.sound.value).second,"Class stage repeated native sound definition");const auto&t=r.tag(s.sound,"snd!");require(t.address==s.source_root&&r.source.gestalt_owner(s.sound)==s.gestalt&&r.source.relocate(t.address,20)==s.native_root,"Class stage sound definition ownership differs");const auto raw=r.read(t.address,28);const Bytes b(raw);const auto index=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[2]));require(index>=0,"Class stage negative source class");const auto mcc=(b.u16(0)&0x800)?remastered:legacy;const auto m=classes.find(mcc.value);require(m!=classes.end()&&unsigned(index)<m->second.count&&unsigned(index)<cls.count,"Class stage class user escapes exact owner");const auto mcc_flags=Bytes(m->second.raw).u16(unsigned(index)*120+10),native_flags=Bytes(projected).u16(cls.offset+unsigned(index)*92+10);require((Bytes(cls.raw).u16(unsigned(index)*120+10)&0x20)==(native_flags&0x20),"Class stage changed persistent-consumer bit5");const int first=std::bit_cast<std::int16_t>(b.u16(8)),count=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[10]));const auto g=std::ranges::find(native.gestalts,s.gestalt,&MccSoundRuntimeGestalt::owner);require(g!=native.gestalts.end()&&count>=0&&(!count||(first>=0&&std::uint64_t(first)+count<=g->count[4])),"Class stage user pitch interval escapes owner");for(int n=0;n<count;++n)users[{s.gestalt.value,unsigned(first+n)}].push_back({s.sound,mcc,legacy,std::uint8_t(index),mcc_flags,native_flags});}
    for(const auto&[datum,t]:r.tags)if(t.address&&t.group=="snd!")require(definitions.contains(datum),"Class stage omitted native sound user");
    std::set<Key>diagnostics;
    for(auto&u:native.unused_flag_indices){require(diagnostics.emplace(u.gestalt.value,u.range_index).second,"Class stage repeated unused-flag diagnostic");const auto all=users.find({u.gestalt.value,u.range_index});require(all!=users.end()&&!all->second.empty()&&u.users.size()==all->second.size(),"Class stage incomplete unused-flag incoming closure");const auto source_range=ranges.find(u.gestalt.value);require(source_range!=ranges.end()&&std::uint64_t(u.range_index)*12+12<=source_range->second.second.size()&&Bytes(source_range->second.second).u16(u.range_index*12+6)==u.encoded_index,"Class stage unused-flag raw range differs");const auto g=std::ranges::find(native.gestalts,u.gestalt,&MccSoundRuntimeGestalt::owner);require(g!=native.gestalts.end(),"Class stage unused range native owner absent");const auto native_range=native.blocks[4].address-native.address+std::size_t(g->first[4]+u.range_index)*12;require(Bytes(native.bytes).u16(native_range+6)==u.encoded_index,"Class stage unused encoded index changed");
        std::set<unsigned>seen;for(auto&old:u.users){const auto it=std::ranges::find(all->second,old.sound,&MccSoundRuntimeFlagUser::sound);require(it!=all->second.end()&&seen.insert(old.sound.value).second&&old.mcc_class==it->mcc_class&&old.native_class==it->native_class&&old.class_index==it->class_index&&old.mcc_flags==it->mcc_flags,"Class stage stale unused-flag user/source receipt");const auto expected_before=Bytes(cls.raw).u16(old.class_index*120+10);require(old.native_flags==expected_before||old.native_flags==it->native_flags,"Class stage stale native class diagnostic");require((it->mcc_flags&0x20)&&(it->native_flags&0x20),"Class stage unused range gained real persistent consumer");old.native_flags=it->native_flags;++out.diagnostic_users;}++out.diagnostic_ranges;}
    // No source or caller mutation occurred. Only the48 typed words change
    // in this private owned result after all providers and guards have passed.
    native.bytes=std::move(projected);return out;
}
} // namespace halo2::mcc_sound_class_detail
