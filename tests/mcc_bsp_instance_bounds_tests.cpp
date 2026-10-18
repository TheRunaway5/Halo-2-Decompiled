#include "halo2/mcc_bsp_instance_bounds.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <set>

namespace {
using namespace halo2;
constexpr std::string_view path="scenarios\\solo\\03a_oldmombasa\\earthcity_3";
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
void word(std::span<std::byte> data,unsigned at,std::uint32_t value){Bytes(data).require(at,4);for(unsigned i=0;i<4;++i)data[at+i]=std::byte(value>>(8*i));}
struct Fixture {
    Tag tag{"sbsp",std::string(path),0x85b42367u,34215616u,2489464u};
    std::uint32_t definitions=36228336u,instances=36458072u;
    std::vector<std::byte> root=std::vector<std::byte>(0x23c),defs=std::vector<std::byte>(43*200),rows=std::vector<std::byte>(243*88);
    Fixture(){word(root,8,0x12130f01);word(root,0x138,43);word(root,0x13c,definitions);word(root,0x140,243);word(root,0x144,instances);word(defs,0x5c,0x36138b96);}
    auto access(){return [this](std::uint32_t at,std::size_t size)->std::span<std::byte>{
        for(auto p:{std::pair{tag.address,&root},std::pair{definitions,&defs},std::pair{instances,&rows}})
            if(at>=p.first && std::uint64_t(at)-p.first+size<=p.second->size())return std::span(*p.second).subspan(at-p.first,size);
        throw FormatError("Fixture request escaped an exact metadata owner");};}
    auto convert(){return convert_mcc_bsp_instance_bounds(tag,access());}
    bool same(const Fixture& other)const{return root==other.root&&defs==other.defs&&rows==other.rows;}
};
unsigned portable(){
    unsigned guards=0;Fixture f;bool read=false;
    const auto unused=[&](std::uint32_t,std::size_t)->std::span<std::byte>{read=true;throw FormatError("Unexpected metadata access");};
    for(unsigned kind=0;kind<5;++kind){auto tag=f.tag;if(kind==0)tag.group="weap";if(kind==1)tag.name="scenarios\\custom\\earthcity_3";if(kind==2)tag.datum^=0x10000;if(kind==3)++tag.address;if(kind==4)--tag.size;
        const auto r=convert_mcc_bsp_instance_bounds(tag,unused);check(!read && (!r||r->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized),"Unknown owner entered finite sphere profile");++guards;}
    const auto before=f;check(f.convert()->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized&&f.same(before),"Synthetic bank authenticated or changed");++guards;
    bool threw=false;try{(void)convert_mcc_bsp_instance_bounds(f.tag,{});}catch(const FormatError&){threw=true;}
    check(threw&&f.same(before),"Missing checked access did not fail atomically");++guards;
    for(unsigned n:{0u,0x23bu}){threw=false;try{(void)convert_mcc_bsp_instance_bounds(f.tag,[&](std::uint32_t,std::size_t){return std::span(f.root).first(n);});}catch(const FormatError&){threw=true;}check(threw&&f.same(before),"Short root view was accepted or changed");++guards;}
    return guards;
}
Fixture fixture(const Cache& cache){
    check(cache.bsps().size()==4&&cache.bsps()[2].name==path,"Selected original BSP path differs");
    const auto* t=cache.find_tag({cache.bsps()[2].datum},tag_groups::scenario_structure_bsp);
    const auto* s=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
    check(t&&t->name==path&&s&&s->name=="scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa"&&
        !cache.find_tag({t->datum^0x10000u},tag_groups::scenario_structure_bsp),"Actual full-salt BSP namespace did not round trip");
    const auto& bsp=cache.bsps()[2];const auto all=cache.source_runtime_cache_bytes();const Bytes bytes(all.subspan(bsp.file_offset,bsp.size));
    const Bytes header(bytes.slice(16,0x23c));check(header.u32(0x138)==43&&header.u32(0x140)==243,"Actual bank counts differ");
    const auto read=[&](std::uint32_t at,std::size_t size){check(at>=bsp.memory_address,"Actual token underflow");return bytes.slice(at-bsp.memory_address,size);};
    Fixture f;f.tag=*t;f.definitions=header.u32(0x13c);f.instances=header.u32(0x144);
    const auto root=cache.is_mcc()?read(t->address,0x23c):bytes.slice(16,0x23c);
    const auto defs=read(f.definitions,43*200),rows=read(f.instances,243*88);
    f.root.assign(root.begin(),root.end());f.defs.assign(defs.begin(),defs.end());f.rows.assign(rows.begin(),rows.end());return f;
}
constexpr std::array<std::uint32_t,13> transform_before{0x3f800000,0xb56fb6a1,0x3f800000,0,0xbf800000,0xb56fb6a1,0,0,0,0x3f800000,0xc41ef873,0x436f6625,0xc16ba993};
constexpr std::array<std::uint32_t,13> transform_after{0x3f800000,0xb56fb6a7,0x3f7fffff,0,0xbf7fffff,0xb56fb6a7,0,0,0,0x3f800000,0xc41ef86b,0x436f6616,0xc16ba9a4};
void transform(Fixture& f,bool after){for(unsigned i=0;i<13;++i)word(f.rows,i*4,(after?transform_after:transform_before)[i]);}
unsigned actual(Fixture donor,const Fixture& original){
    // A normal Cache can already publish the complete postimage. Authenticate
    // that entire bank first, then reconstruct only the admitted retail scalar
    // preimages in this private fixture so conversion tests never become vacuous.
    auto authenticated=donor;const auto admission=authenticated.convert();
    check(admission&&admission->field_count==49&&(admission->status==MccGameplayReferenceStatus::Converted||
        admission->status==MccGameplayReferenceStatus::AlreadyEquivalent),"Actual Cache sphere bank is not a complete authenticated preimage/postimage");
    for(const auto& field:admission->fields){const auto at=field.instance*88+60+field.component*4;
        check(field.address==donor.instances+at&&field.after_bits==Bytes(original.rows).u32(at)&&
            Bytes(donor.rows).u32(at)==(admission->status==MccGameplayReferenceStatus::Converted?field.before_bits:field.after_bits),
            "Authenticated source/reference scalar identity differs");}
    for(const auto& field:admission->fields)word(donor.rows,field.address-donor.instances,field.before_bits);
    auto converted=donor;const auto receipt=converted.convert();
    check(receipt&&receipt->status==MccGameplayReferenceStatus::Converted&&receipt->field_count==49,"Actual stored-sphere conversion did not restore 49 authenticated words");
    auto expected=donor;unsigned words=0,changed=0;std::set<unsigned> spheres,definitions;
    for(unsigned i=0;i<243;++i)for(unsigned c=0;c<4;++c){const auto at=i*88+60+c*4;const auto a=Bytes(donor.rows).u32(at),b=Bytes(original.rows).u32(at);
        if(a!=b){word(expected.rows,at,b);++words;spheres.insert(i);definitions.insert(Bytes(donor.rows).u16(i*88+52));}}
    for(unsigned i=0;i<donor.rows.size();++i)changed+=donor.rows[i]!=expected.rows[i];
    check(words==49&&changed==51&&spheres.size()==35&&definitions.size()==12&&converted.same(expected),"Conversion exceeded exact original49-word/51-byte sphere delta");
    for(unsigned i=0;i<243;++i)check(std::ranges::equal(std::span(converted.rows).subspan(i*88+60,16),std::span(original.rows).subspan(i*88+60,16)),"Retained stored sphere differs from original");
    std::set<std::uint32_t> addresses;
    for(const auto& field:receipt->fields){const auto at=field.instance*88+60+field.component*4;check(field.address==donor.instances+at&&field.before_bits==Bytes(donor.rows).u32(at)&&field.after_bits==Bytes(original.rows).u32(at)&&field.before_bits!=field.after_bits&&addresses.insert(field.address).second,"Scalar receipt differs from original field identity");}
    unsigned guards=0;
    for(bool after:{false,true}){auto pre=donor,post=expected;transform(pre,after);transform(post,after);const auto a=pre.convert();check(a&&a->status==MccGameplayReferenceStatus::Converted&&pre.same(post),"Exact transform order changed sphere admission or transform bytes");
        const auto stable=pre;check(pre.convert()->status==MccGameplayReferenceStatus::AlreadyEquivalent&&pre.same(stable),"Full postimage is not idempotent");}
    const auto unchanged=[&](Fixture bad){const auto prior=bad;auto r=bad.convert();check(r&&r->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized&&bad.same(prior),"Custom/partial metadata was partly overwritten");++guards;};
    for(const auto* source:{&donor,&expected}){
        for(const auto& field:receipt->fields){auto bad=*source;const auto at=field.address-donor.instances;word(bad.rows,at,source==&donor?field.after_bits:field.before_bits);unchanged(std::move(bad));}
        // Every retained instance row, every complete definition row and every
        // fixed root word is covered on both preimage and postimage paths.
        for(unsigned row=0;row<243;++row){auto bad=*source;bad.rows[row*88+87]^=std::byte{1};unchanged(std::move(bad));}
        for(unsigned row=0;row<43;++row)for(unsigned at:{0u,0x5cu,0xc4u}){auto bad=*source;bad.defs[row*200+at]^=std::byte{1};unchanged(std::move(bad));}
        for(unsigned at=0;at+4<=0x23c;at+=4){auto bad=*source;bad.root[at]^=std::byte{1};unchanged(std::move(bad));}
        for(unsigned at=0;at<52;++at){auto bad=*source;bad.rows[at]^=std::byte{1};unchanged(std::move(bad));}
        for(unsigned at:{52u,54u,56u,76u,80u,84u}){auto bad=*source;bad.rows[at]^=std::byte{1};unchanged(std::move(bad));}
    }
    for(unsigned row=0;row<243;++row){auto bad=donor;bad.rows[row*88+40]^=std::byte{1};unchanged(std::move(bad));}
    // Valid source tokens cannot conceal overlapping actual host backing.
    auto alias=donor;const auto prior=alias;
    const auto access=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{if(at==alias.definitions)return std::span(alias.rows).first(n);return alias.access()(at,n);};
    check(convert_mcc_bsp_instance_bounds(alias.tag,access)->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized&&alias.same(prior),"Aliased host banks were accepted");++guards;
    for(unsigned which:{0u,1u}){auto short_view=donor;bool threw=false;try{(void)convert_mcc_bsp_instance_bounds(short_view.tag,[&](std::uint32_t at,std::size_t n){auto b=short_view.access()(at,n);return (at==(which?short_view.instances:short_view.definitions))?b.first(n-1):b;});}catch(const FormatError&){threw=true;}check(threw&&short_view.same(donor),"Short child bank caused a partial conversion");++guards;}
    std::cout<<"PASS actual spheres: 35 instances / 12 definitions / 49 words / 51 bytes; "<<guards<<" independent custom/mixed/owner guards\n";return guards;
}
auto bits(Vec3 p){return std::array{std::bit_cast<unsigned>(p.x),std::bit_cast<unsigned>(p.y),std::bit_cast<unsigned>(p.z)};}
void same_collision(const BspContent& a,const BspContent& b,bool navigation=true){check(a.collision.size()==b.collision.size(),"Expanded collision counts changed");if(navigation)check(a.navigation.size()==b.navigation.size(),"Expanded navigation counts changed");
    for(unsigned i=0;i<a.collision.size();++i){const auto&x=a.collision[i];const auto&y=b.collision[i];check(bits(x.a)==bits(y.a)&&bits(x.b)==bits(y.b)&&bits(x.c)==bits(y.c)&&bits(x.source_normal)==bits(y.source_normal)&&x.material==y.material&&x.flags==y.flags&&x.source.bsp==y.source.bsp&&x.source.record==y.source.record&&x.source.surface==y.source.surface&&x.source.instance==y.source.instance&&x.source.plane==y.source.plane,"Expanded collision values or topology changed");}
    if(navigation)for(unsigned i=0;i<a.navigation.size();++i){const auto&x=a.navigation[i];const auto&y=b.navigation[i];check(x.flags==y.flags&&x.polygon.size()==y.polygon.size()&&x.portals.size()==y.portals.size(),"Navigation ownership changed");for(unsigned p=0;p<x.polygon.size();++p)check(bits(x.polygon[p])==bits(y.polygon[p]),"Navigation polygon changed");for(unsigned p=0;p<x.portals.size();++p)check(x.portals[p].sector==y.portals[p].sector&&x.portals[p].flags==y.portals[p].flags&&bits(x.portals[p].midpoint)==bits(y.portals[p].midpoint),"Navigation portal changed");}}
void actual_cache(Cache& mcc,const Cache& original){
    const auto& bsp=mcc.bsps()[2];auto raw=mcc.source_runtime_cache_bytes();
    const auto* tag=mcc.find_tag({bsp.datum},tag_groups::scenario_structure_bsp);check(tag,"Actual BSP Tag missing");
    const auto access=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{check(at>=bsp.memory_address,"Actual mutable token underflow");Bytes(raw.subspan(bsp.file_offset,bsp.size)).require(at-bsp.memory_address,n);return {const_cast<std::byte*>(raw.data())+bsp.file_offset+at-bsp.memory_address,n};};
    auto authorized=fixture(mcc);const auto admission=authorized.convert();const auto reference=fixture(original);
    check(admission&&admission->field_count==49&&(admission->status==MccGameplayReferenceStatus::Converted||
        admission->status==MccGameplayReferenceStatus::AlreadyEquivalent),"Published Cache sphere bank failed complete authentication");
    const bool was_postimage=admission->status==MccGameplayReferenceStatus::AlreadyEquivalent;
    for(const auto& field:admission->fields){const auto at=field.instance*88+60+field.component*4;
        check(field.address==authorized.instances+at&&field.after_bits==Bytes(reference.rows).u32(at)&&
            Bytes(access(field.address,4)).u32(0)==(was_postimage?field.after_bits:field.before_bits),
            "Published Cache preimage reconstruction is not exact and reference-bound");}
    // Only the already-authenticated 49 scalar words may be restored, and only
    // within this test's owned Cache copy. No transform or other byte is reset.
    for(const auto& field:admission->fields)word(access(field.address,4),0,field.before_bits);
    const auto prior_collision=mcc.load_bsp_content(2);const auto prior_mesh=mcc.load_bsp(2,false);
    const auto before=std::vector<std::byte>(raw.begin(),raw.end());
    const auto r=convert_mcc_bsp_instance_bounds(*tag,access);check(r&&r->status==MccGameplayReferenceStatus::Converted,"Actual published Cache bounds did not convert");
    const auto collision=mcc.load_bsp_content(2);const auto mesh=mcc.load_bsp(2,false);same_collision(prior_collision,collision);const auto original_collision=original.load_bsp_content(2);same_collision(collision,original_collision,false);
    // Current retained Vertex is a complete 152-byte, four-byte-aligned bank:
    // all float attributes and all integer provenance fields are initialized.
    static_assert(sizeof(Vertex)==152);static_assert(offsetof(Vertex,source_part_flags)==150);
    check(mesh.vertices.size()==prior_mesh.vertices.size()&&mesh.clusters==prior_mesh.clusters&&mesh.instances==prior_mesh.instances&&bits(mesh.minimum)==bits(prior_mesh.minimum)&&bits(mesh.maximum)==bits(prior_mesh.maximum)&&mesh.materials.size()==prior_mesh.materials.size()&&mesh.lightmaps.size()==prior_mesh.lightmaps.size()&&mesh.material_warnings==prior_mesh.material_warnings&&mesh.resident_material_lightmaps==prior_mesh.resident_material_lightmaps,"Expanded render metadata changed");
    for(unsigned i=0;i<mesh.vertices.size();++i)check(std::memcmp(&mesh.vertices[i],&prior_mesh.vertices[i],sizeof(Vertex))==0,"Expanded render attribute or source provenance changed");
    unsigned changed=0;for(std::size_t i=0;i<before.size();++i)if(before[i]!=raw[i]){++changed;bool allowed=false;for(const auto& field:r->fields){const auto at=bsp.file_offset+field.address-bsp.memory_address;allowed|=i>=at&&i<at+4;}check(allowed,"Cache overlay changed unrelated metadata");}
    check(changed==51&&collision.collision.size()==42524,"Actual Cache delta or expanded collision census differs");
    const auto stable=std::vector<std::byte>(raw.begin(),raw.end());check(convert_mcc_bsp_instance_bounds(*tag,access)->status==MccGameplayReferenceStatus::AlreadyEquivalent&&std::ranges::equal(stable,raw),"Actual Cache postimage is not idempotent");
    std::cout<<"PASS actual Cache: "<<collision.collision.size()<<" original-equivalent collision triangles, "<<mesh.vertices.size()<<" unchanged expanded render vertices, "<<collision.navigation.size()<<" unchanged MCC navigation sectors (original "<<original_collision.navigation.size()<<"); exact51-byte delta; initial_postimage="<<was_postimage<<"\n";
}
} // namespace
int main(int argc,char**argv)try{
    std::cout<<"PASS portable bounds ownership: "<<portable()<<" guards\n";
    if(argc==1)return 0;
    if(argc==3&&std::string_view(argv[1])=="--mcc-only"){
        if(!*argv[2])return 77;
        Cache mcc(argv[2]);check(mcc.is_mcc(),"Expected an owned MCC map");
        const auto raw=mcc.source_runtime_cache_bytes();const auto before=std::vector<std::byte>(raw.begin(),raw.end());
        const auto* owner=mcc.find_tag({mcc.bsps().at(2).datum},tag_groups::scenario_structure_bsp);
        check(owner&&owner->name==path,"Normal Cache lost the typed BSP identity");
        const auto read=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{const auto b=mcc.read_tag_bytes(at,n);return {const_cast<std::byte*>(b.data()),b.size()};};
        const auto admitted=convert_mcc_bsp_instance_bounds(*owner,read);
        auto published=fixture(mcc);const auto copy=published.convert();
        check(admitted&&copy&&admitted->field_count==49&&copy->field_count==49&&
            admitted->status==MccGameplayReferenceStatus::AlreadyEquivalent&&copy->status==MccGameplayReferenceStatus::AlreadyEquivalent,
            "Normal MCC-only Cache did not publish both complete bounds postimages");
        for(unsigned i=0;i<49;++i){const auto&a=admitted->fields[i];const auto&b=copy->fields[i];
            check(a.address==b.address&&a.instance==b.instance&&a.component==b.component&&a.before_bits==b.before_bits&&a.after_bits==b.after_bits&&
                Bytes(mcc.read_tag_bytes(a.address,4)).u32(0)==a.after_bits&&Bytes(published.rows).u32(a.address-published.instances)==a.after_bits,
                "Normal Cache metadata/published49word postimages disagree");}
        const auto collision=mcc.load_bsp_content(2);const auto mesh=mcc.load_bsp(2,false);
        check(collision.collision.size()==42524&&mesh.vertices.size()==136515&&std::ranges::equal(before,raw),
            "Normal MCC-only Cache expansion count or immutable backing changed");
        std::cout<<"PASS normal MCC-only Cache: metadata + published BSP already-equivalent, all49 postimage words, 42524 collision triangles / 136515 render vertices, Cache backing unchanged\n";
        return 0;
    }
    check(argc==3,"Expected optional actual MCC/original Old Mombasa paths");if(!*argv[1]||!*argv[2])return 77;
    Cache mcc(argv[1]);const Cache original(argv[2]);check(mcc.is_mcc()&&!original.is_mcc()&&!original.is_vista(),"Expected MCC and original Xbox maps");
    actual(fixture(mcc),fixture(original));actual_cache(mcc,original);actual_cache(mcc,original);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
