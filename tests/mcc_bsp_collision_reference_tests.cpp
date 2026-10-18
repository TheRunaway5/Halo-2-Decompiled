#include "halo2/mcc_bsp_collision_reference.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>

namespace {
using namespace halo2;
constexpr std::string_view name="scenarios\\solo\\03a_oldmombasa\\earthcity_3";
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
void put(std::span<std::byte> data,std::size_t at,std::uint32_t value) {
    Bytes(data).require(at,4);for(unsigned i=0;i<4;++i)data[at+i]=std::byte(value>>(8*i));
}
struct Fixture {
    std::uint32_t base=0x1000,root=0x1000;
    std::vector<std::byte> data=std::vector<std::byte>(0x10000);
    Fixture() {
        put(data,8,0x12130f01);put(data,0x14,1);put(data,0x18,0x1400);
        put(data,0x138,43);put(data,0x13c,0x1600);
        put(data,0x140,243);put(data,0x144,0x4000);
        put(data,0x600+0x5c,0x36138b96);
    }
    std::size_t offset(std::uint32_t address,std::size_t size=4)const {
        require(address>=base,"Fixture owner underflow");
        Bytes(data).require(address-base,size);return address-base;
    }
    auto access() {
        return [this](std::uint32_t address,std::size_t size)->std::span<std::byte> {
            return std::span(data).subspan(offset(address,size),size);
        };
    }
    auto convert(){return convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,name,root,access());}
    std::uint32_t u32(std::uint32_t address)const{return Bytes(data).u32(offset(address));}
    void word(std::uint32_t address,std::uint32_t value){put(data,offset(address),value);}
};
unsigned portable() {
    unsigned guards=0;
    Fixture custom;const auto original=custom.data;bool called=false;
    auto unused=[&](std::uint32_t,std::size_t)->std::span<std::byte>{called=true;throw FormatError("Unexpected owner read");};
    require(!convert_mcc_bsp_collision_reference(tag_groups::weapon,name,custom.root,unused) &&
        !convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,
            "scenarios\\custom\\earthcity_3",custom.root,unused) && !called && custom.data==original,
        "Unrecognized type/path accessed metadata");guards+=2;
    const auto unchanged=[&](Fixture bad) {
        const auto before=bad.data;const auto result=bad.convert();
        require(result && result->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && before==bad.data,
            "Unknown collision identity was partly restored");++guards;
    };
    unchanged(Fixture{}); // No synthetic geometry can authenticate the full source digest.
    for(unsigned at:{8u,0x14u,0x138u,0x140u,0x65cu}) {
        Fixture bad;bad.data[at]^=std::byte{1};unchanged(std::move(bad));
    }
    const auto throws_unchanged=[&](Fixture bad,auto access) {
        const auto before=bad.data;bool rejected=false;
        try{(void)convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,name,bad.root,access(bad));}
        catch(const FormatError&){rejected=true;}
        require(rejected && bad.data==before,"Invalid owner failed after a partial write");++guards;
    };
    throws_unchanged(Fixture{},[](Fixture&){return std::function<std::span<std::byte>(std::uint32_t,std::size_t)>{};});
    throws_unchanged(Fixture{},[](Fixture& f){return [&f](std::uint32_t a,std::size_t n){return f.access()(a,n-1);};});
    for(unsigned at:{0x18u,0x13cu,0x144u})for(std::uint32_t token:{0u,0xffffffffu,0xfffffff0u}) {
        Fixture bad;put(bad.data,at,token);throws_unchanged(std::move(bad),[](Fixture& f){return f.access();});
    }
    for(std::uint32_t token:{0u,0xffffffffu,0xfffffff0u}) {
        Fixture bad;bad.root=token;throws_unchanged(std::move(bad),[](Fixture& f){return f.access();});
    }
    for(std::size_t size:{std::size_t(0x23b),std::size_t(0x43f),std::size_t(0x2797),std::size_t(0x8387)}) {
        Fixture bad;bad.data.resize(size);throws_unchanged(std::move(bad),[](Fixture& f){return f.access();});
    }
    return guards;
}
Fixture fixture(const Cache& cache) {
    require(cache.bsps().size()==4 && cache.bsps()[2].name==name,"Actual selected BSP identity differs");
    const auto& bsp=cache.bsps()[2];const auto storage=cache.source_runtime_cache_bytes();
    const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
    const auto* tag=cache.find_tag({bsp.datum},tag_groups::scenario_structure_bsp);
    require(scenario && scenario->name=="scenarios\\solo\\03a_oldmombasa\\03a_oldmombasa" &&
        tag && tag->datum==bsp.datum && tag->type()==tag_groups::scenario_structure_bsp && tag->name==name &&
        !cache.find_tag({bsp.datum^0x10000u},tag_groups::scenario_structure_bsp),
        "Actual scenario/BSP full salted typed identity failed to round trip");
    const auto bytes=Bytes(storage).slice(bsp.file_offset,bsp.size);
    Fixture result;result.data.assign(bytes.begin(),bytes.end());
    result.base=bsp.memory_address;result.root=bsp.memory_address+16;return result;
}
struct Location {unsigned owner,field,component;std::uint32_t donor;};
constexpr std::array<Location,17> locations{{
    {0,8,3,0x43361d71u},{0,32,0,0x3f6893b3u},{0,32,1,0xbed5f3f4u},{0,32,2,0x43ecbe56u},
    {0,56,0,0xc4201873u},{0,56,1,0x436f1fbeu},{0,56,2,0xc14446dcu},
    {1,8,0,0x3efc6111u},{1,8,1,0x3f5ebceau},{1,8,2,0xb845de6cu},{1,8,3,0xbe314db6u},
    {1,32,0,0x3f66ac73u},{1,32,1,0xbede0ac6u},{1,32,2,0xbef57e46u},
    {1,56,0,0x3de7ae9bu},{1,56,1,0xbe86ab58u},{1,56,2,0x3fa284e0u}
}};
std::array<std::uint32_t,44> owners(const Fixture& f) {
    std::array<std::uint32_t,44> result{};result[0]=f.u32(f.root+0x18);
    const auto defs=f.u32(f.root+0x13c);
    for(unsigned i=1;i<result.size();++i)result[i]=defs+(i-1)*200+0x70;
    return result;
}
std::array<std::uint32_t,17> addresses(const Fixture& f) {
    const auto records=owners(f);std::array<std::uint32_t,17> result{};
    for(unsigned i=0;i<locations.size();++i) {
        const auto& l=locations[i];result[i]=f.u32(records[l.owner]+l.field+4)+l.component*4;
    }
    return result;
}
constexpr std::array<unsigned,8> strides{8,16,4,4,16,8,12,16};
void exact_payloads(const Fixture& a,const Fixture& b) {
    const auto aa=owners(a),bb=owners(b);
    for(unsigned owner=0;owner<44;++owner)for(unsigned field=0;field<8;++field) {
        const auto n=a.u32(aa[owner]+field*8);
        require(n==b.u32(bb[owner]+field*8),"Actual original collision count differs");
        if(!n)continue;
        const auto size=n*strides[field];
        const auto av=std::span(a.data).subspan(a.offset(a.u32(aa[owner]+field*8+4),size),size);
        const auto bv=std::span(b.data).subspan(b.offset(b.u32(bb[owner]+field*8+4),size),size);
        require(std::ranges::equal(av,bv),"Complete converted collision payload differs from original");
    }
}
unsigned actual_guards(Fixture donor,const Fixture& original) {
    const auto at=addresses(donor),og=addresses(original);unsigned guards=0;
    // Ordinary Cache may already apply this converter. Restore only the known
    // 17-word preimage in this private fixture to exercise the actual module.
    bool pre=true,post=true;
    for(unsigned i=0;i<at.size();++i){pre&=donor.u32(at[i])==locations[i].donor;post&=donor.u32(at[i])==original.u32(og[i]);}
    require(pre||post,"Actual source collision tuple is neither complete preimage nor postimage");
    Fixture authenticated=donor;const auto admitted=authenticated.convert();
    require(admitted && admitted->field_count==17 && admitted->status==(pre?
        MccGameplayReferenceStatus::Converted:MccGameplayReferenceStatus::AlreadyEquivalent),
        "Actual source is not a complete authenticated collision preimage or postimage");
    for(unsigned i=0;i<at.size();++i) {
        const auto& field=admitted->fields[i];
        require(field.address==at[i] && field.before_bits==locations[i].donor && field.after_bits==original.u32(og[i]),
            "Authenticated original collision scalar identity differs");
        donor.word(field.address,field.before_bits);
    }
    auto target=donor.data;
    for(unsigned i=0;i<at.size();++i)put(target,donor.offset(at[i]),original.u32(og[i]));
    Fixture converted=donor;auto receipt=converted.convert();
    require(receipt && receipt->status==MccGameplayReferenceStatus::Converted && receipt->field_count==17 &&
        converted.data==target,"Actual conversion changed bytes outside the exact 17 scalars");
    for(unsigned i=0;i<at.size();++i)require(receipt->fields[i].address==at[i] &&
        receipt->fields[i].before_bits==locations[i].donor && receipt->fields[i].after_bits==original.u32(og[i]),
        "Actual scalar receipt differs from the original source");
    exact_payloads(converted,original);
    require(converted.convert()->status==MccGameplayReferenceStatus::AlreadyEquivalent && converted.data==target,
        "Complete original collision postimage is not idempotent");
    const auto unchanged=[&](Fixture& bad) {
        const auto before=bad.data;auto result=bad.convert();
        require(result && result->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && before==bad.data,
            "Custom/partial collision was overwritten");++guards;
    };
    for(unsigned i=0;i<at.size();++i) {
        Fixture bad=donor;bad.word(at[i],original.u32(og[i]));unchanged(bad);
        bad=converted;bad.word(at[i],locations[i].donor);unchanged(bad);
        for(unsigned bit=0;bit<32;++bit){bad=donor;bad.word(at[i],locations[i].donor^(1u<<bit));unchanged(bad);}
    }
    // Every owner/block count and every nonempty topology/payload tail is
    // guarded in both preimage and already-equivalent paths.
    for(const auto* source:{&donor,&converted}) {
        const auto records=owners(*source);
        for(unsigned owner=0;owner<44;++owner)for(unsigned field=0;field<8;++field) {
            Fixture bad=*source;const auto descriptor=records[owner]+field*8;
            bad.word(descriptor,bad.u32(descriptor)^1u);unchanged(bad);
            const auto n=source->u32(descriptor);if(!n)continue;
            const auto ptr=source->u32(descriptor+4),size=n*strides[field];
            bad=*source;bad.data[bad.offset(ptr,size)+size-1]^=std::byte{1};unchanged(bad);
        }
        for(auto address:{source->root+8,source->root+0x14,source->root+0x138,source->root+0x140,
            source->u32(source->root+0x13c)+0x5c}) {
            Fixture bad=*source;bad.word(address,bad.u32(address)^1u);unchanged(bad);
        }
    }
    // Different source tokens cannot hide aliases of metadata or other arrays.
    const auto records=owners(donor);const auto plane_pointer=records[0]+12;
    for(auto ptr:{donor.root,donor.u32(donor.root+0x18),donor.u32(donor.root+0x13c),
        donor.u32(donor.root+0x144),donor.u32(records[0]+4)}) {
        Fixture bad=donor;bad.word(plane_pointer,ptr);unchanged(bad);
    }
    Fixture host_alias=donor;const auto before_alias=host_alias.data;
    const auto plane=host_alias.u32(plane_pointer),vertices=host_alias.u32(records[0]+60);
    const auto access=[&](std::uint32_t address,std::size_t size) {
        return host_alias.access()(address==plane?vertices:address,size);
    };
    require(convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,name,host_alias.root,access)->status==
        MccGameplayReferenceStatus::AuthoredTupleUnrecognized && host_alias.data==before_alias,
        "Aliased host payload storage was accepted");++guards;
    // Relocation and unused empty-block pointers do not alter payload identity.
    Fixture relocated=donor;constexpr unsigned delta=0x100000;
    const auto old_records=owners(relocated);
    for(unsigned owner=0;owner<44;++owner)for(unsigned field=0;field<8;++field) {
        const auto descriptor=old_records[owner]+field*8;
        relocated.word(descriptor+4,relocated.u32(descriptor)?relocated.u32(descriptor+4)+delta:0xffffffffu);
    }
    for(unsigned descriptor:{0x14u,0x138u,0x140u}) {
        const auto pointer=relocated.root+descriptor+4;relocated.word(pointer,relocated.u32(pointer)+delta);
    }
    relocated.base+=delta;relocated.root+=delta;
    require(relocated.convert()->status==MccGameplayReferenceStatus::Converted && relocated.data.size()==target.size(),
        "Relocated authentic collision payload was rejected");
    exact_payloads(relocated,original);
    // Instance bounds/name/policy data remain independent of this correction.
    Fixture metadata=donor;const auto instance=metadata.u32(metadata.root+0x144);
    for(unsigned offset:{52u,60u,64u,68u,72u,80u,84u})metadata.data[metadata.offset(instance+offset)]^=std::byte{1};
    auto expected=metadata.data;for(unsigned i=0;i<at.size();++i)put(expected,metadata.offset(at[i]),original.u32(og[i]));
    require(metadata.convert()->status==MccGameplayReferenceStatus::Converted && metadata.data==expected,
        "Collision conversion copied instance spheres, names, or policies");
    return guards;
}
auto bits(Vec3 p){return std::array{std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y),std::bit_cast<std::uint32_t>(p.z)};}
void actual_cache(Cache& mcc,const Cache& xbox) {
    const auto& bsp=mcc.bsps()[2];const auto storage=mcc.source_runtime_cache_bytes();
    const auto before=std::vector<std::byte>(storage.begin(),storage.end());
    const auto access=[&](std::uint32_t address,std::size_t size)->std::span<std::byte> {
        require(address>=bsp.memory_address,"Actual published BSP token underflow");
        const auto offset=address-bsp.memory_address;Bytes(storage.subspan(bsp.file_offset,bsp.size)).require(offset,size);
        return {const_cast<std::byte*>(storage.data())+bsp.file_offset+offset,size};
    };
    const auto root=bsp.memory_address+16;
    const auto transform=convert_mcc_bsp_instance_transform(tag_groups::scenario_structure_bsp,name,root,access);
    const auto receipt=convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,name,root,access);
    const auto recognized=[](auto status){return status==MccGameplayReferenceStatus::Converted || status==MccGameplayReferenceStatus::AlreadyEquivalent;};
    require(transform && recognized(transform->status) && receipt && recognized(receipt->status),
        "Actual published BSP did not admit both independent corrections");
    const auto a=mcc.load_bsp_content(2),b=xbox.load_bsp_content(2);
    require(a.collision.size()==42524 && a.collision.size()==b.collision.size(),"Actual expanded collision triangle count differs");
    unsigned instance0=0;
    for(unsigned i=0;i<a.collision.size();++i) {
        const auto& p=a.collision[i];const auto& q=b.collision[i];instance0+=p.source.instance==0;
        require(bits(p.a)==bits(q.a) && bits(p.b)==bits(q.b) && bits(p.c)==bits(q.c) && bits(p.source_normal)==bits(q.source_normal),
            "Actual expanded collision positions or source normals differ from original");
        require(p.material==q.material && p.flags==q.flags && p.source.bsp==q.source.bsp &&
            p.source.record==q.source.record && p.source.surface==q.source.surface &&
            p.source.instance==q.source.instance && p.source.plane==q.source.plane,
            "Actual expanded collision topology/material identity differs from original");
    }
    require(instance0==186,"Actual instance-zero expansion differs");
    for(std::size_t i=0;i<before.size();++i)if(before[i]!=storage[i]) {
        bool allowed=i>=bsp.file_offset+transform->address-bsp.memory_address &&
            i<bsp.file_offset+transform->address-bsp.memory_address+52;
        for(const auto& field:receipt->fields)allowed|=i>=bsp.file_offset+field.address-bsp.memory_address &&
            i<bsp.file_offset+field.address-bsp.memory_address+4;
        require(allowed,"Actual Cache overlay modified unrelated bytes");
    }
    require(convert_mcc_bsp_collision_reference(tag_groups::scenario_structure_bsp,name,root,access)->status==
        MccGameplayReferenceStatus::AlreadyEquivalent,"Actual Cache postimage is not idempotent");
    std::cout<<"PASS actual Cache: 42,524 triangles, all positions/normals/topology exact; 186 instance-zero triangles; only transform52 + scalar17 fields changed\n";
}
} // namespace
int main(int argc,char** argv)try {
    const auto guards=portable();std::cout<<"PASS portable collision owner/type/path boundaries; "<<guards<<" rejection guards\n";
    if(argc==1)return 0;
    if(argc==3 && std::string_view(argv[1])=="--mcc-only") {
        if(!*argv[2])return 77;
        Cache mcc(argv[2]);require(mcc.is_mcc(),"Expected an owned MCC map");
        auto owned=fixture(mcc);
        const auto collision=owned.convert();
        require(collision && collision->status==MccGameplayReferenceStatus::AlreadyEquivalent,
            "Normal MCC Cache did not publish the complete original collision postimage");
        const auto transform=convert_mcc_bsp_instance_transform(tag_groups::scenario_structure_bsp,name,owned.root,owned.access());
        require(transform && transform->status==MccGameplayReferenceStatus::AlreadyEquivalent,
            "Normal MCC Cache did not publish the original instance transform");
        const auto content=mcc.load_bsp_content(2);require(content.collision.size()==42524,"MCC collision expansion count differs");
        for(const auto& triangle:content.collision)for(const auto p:{triangle.a,triangle.b,triangle.c,triangle.source_normal})
            require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z),"MCC collision expansion is nonfinite");
        std::cout<<"PASS MCC-only normal Cache: complete 44-owner original collision postimage and transform, 42,524 finite expanded triangles\n";
        return 0;
    }
    require(argc==3,"Expected optional MCC/original Old Mombasa map paths");
    if(!*argv[1] || !*argv[2]){std::cout<<"SKIP paired owned map paths are not configured\n";return 77;}
    Cache mcc(argv[1]);const Cache xbox(argv[2]);
    require(mcc.is_mcc() && !xbox.is_mcc() && !xbox.is_vista(),"Expected actual paired MCC/original map formats");
    const auto mutations=actual_guards(fixture(mcc),fixture(xbox));
    std::cout<<"PASS actual collision: exact 17-word delta, complete 44-owner original postimage, relocation/idempotence; "<<mutations<<" mutation/alias guards\n";
    actual_cache(mcc,xbox);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
