#include "halo2/mcc_gameplay_reference.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
constexpr std::string_view name="scenarios\\solo\\03a_oldmombasa\\earthcity_3";
constexpr std::string_view donor_hex="0000803fa1b66fb50000803f00000000000080bfa1b66fb50000000000000000000000000000803f73f81ec425666f4393a96bc10000000000000000d9fd1ec4dd9d6f430fc558c1b9882940817e8af3d341000b00000000";
constexpr std::string_view target_hex="0000803fa7b66fb5ffff7f3f00000000ffff7fbfa7b66fb50000000000000000000000000000803f6bf81ec416666f43a4a96bc1";
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<std::byte> hex(std::string_view value) {
    std::vector<std::byte> result;
    for(std::size_t i=0;i<value.size();i+=2)result.push_back(std::byte(std::stoul(std::string(value.substr(i,2)),nullptr,16)));
    return result;
}
void put(std::span<std::byte> data,unsigned at,std::uint32_t word) {
    Bytes(data).require(at,4);for(unsigned i=0;i<4;++i)data[at+i]=std::byte(word>>(8*i));
}
struct Fixture {
    static constexpr unsigned root=0x1000,definitions=0x1600,instances=0x4000;
    std::vector<std::byte> bytes=std::vector<std::byte>(0x10000);
    Fixture() {
        for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte(i*17u+23u);
        put(bytes,0x138,43);put(bytes,0x13c,definitions);
        put(bytes,0x140,243);put(bytes,0x144,instances);
        put(bytes,definitions-root+0x5c,0x36138b96);
        const auto donor=hex(donor_hex);std::ranges::copy(donor,bytes.begin()+instances-root);
    }
    auto access() {
        return [this](std::uint32_t address,std::size_t size)->std::span<std::byte> {
            if(address<root)throw FormatError("Fixture owner underflow");
            Bytes(bytes).require(address-root,size);return std::span(bytes).subspan(address-root,size);
        };
    }
    auto convert(){return convert_mcc_bsp_instance_transform(tag_groups::scenario_structure_bsp,name,root,access());}
};
void portable() {
    Fixture f;auto expected=f.bytes;const auto target=hex(target_hex);
    std::ranges::copy(target,expected.begin()+Fixture::instances-Fixture::root);
    auto result=f.convert();require(result && result->status==MccGameplayReferenceStatus::Converted &&
        result->address==Fixture::instances && f.bytes==expected,"Instance conversion changed other records or non-transform fields");
    result=f.convert();require(result && result->status==MccGameplayReferenceStatus::AlreadyEquivalent &&
        f.bytes==expected,"Original instance transform is not idempotent");
    unsigned negatives=0;
    const auto untouched=[&](Fixture& bad) {
        const auto before=bad.bytes;const auto result=bad.convert();
        require(result && result->status==MccGameplayReferenceStatus::AuthoredTupleUnrecognized && before==bad.bytes,
            "Changed transform/owner tuple was overwritten");++negatives;
    };
    for(unsigned i=0;i<52;++i){Fixture bad;bad.bytes[Fixture::instances-Fixture::root+i]^=std::byte{1};untouched(bad);}
    for(unsigned at:{0x138u,0x140u,Fixture::definitions-Fixture::root+0x5c,
        Fixture::instances-Fixture::root+52,Fixture::instances-Fixture::root+54,
        Fixture::instances-Fixture::root+56,Fixture::instances-Fixture::root+76,Fixture::instances-Fixture::root+84}) {
        Fixture bad;bad.bytes[at]^=std::byte{1};untouched(bad);
    }
    Fixture namespace_changed;put(namespace_changed.bytes,Fixture::instances-Fixture::root+80,0x0b007777);
    const auto original_name=Bytes(namespace_changed.bytes).u32(Fixture::instances-Fixture::root+80);
    require(namespace_changed.convert()->status==MccGameplayReferenceStatus::Converted &&
        Bytes(namespace_changed.bytes).u32(Fixture::instances-Fixture::root+80)==original_name,
        "Original numeric StringID was copied across the MCC namespace");
    Fixture custom;const auto before=custom.bytes;
    require(!convert_mcc_bsp_instance_transform(tag_groups::scenario_structure_bsp,
        "scenarios\\custom\\earthcity_3",Fixture::root,custom.access()) &&
        !convert_mcc_bsp_instance_transform(tag_groups::weapon,name,Fixture::root,custom.access()) && before==custom.bytes,
        "Custom BSP/other group was treated as the named original instance");
    for(unsigned cutoff:{0x147u,Fixture::definitions-Fixture::root+0x5f,
        Fixture::instances-Fixture::root+87}) {
        Fixture bad;bad.bytes.resize(cutoff);const auto before=bad.bytes;bool rejected=false;
        try{(void)bad.convert();}catch(const FormatError&){rejected=true;}
        require(rejected && before==bad.bytes,"A short metadata owner was partly written");++negatives;
    }
    Fixture overflow;put(overflow.bytes,0x13c,0xfffffff0u);const auto before_overflow=overflow.bytes;bool rejected=false;
    try{(void)overflow.convert();}catch(const FormatError&){rejected=true;}
    require(rejected && before_overflow==overflow.bytes,"An overflowing source token was accepted");++negatives;
    std::cout<<"PASS exact original instance transform, all other bytes/namespace retained, idempotence; "<<negatives<<" mutation guards\n";
}
std::span<const std::byte> instance(const Cache& cache) {
    require(cache.bsps().size()==4 && cache.bsps()[2].name==name,"Missing named actual BSP");
    const auto& bsp=cache.bsps()[2];
    const auto storage=cache.source_runtime_cache_bytes();
    const Bytes metadata(Bytes(storage).slice(bsp.file_offset,bsp.size));
    const Bytes descriptor(metadata.slice(16+0x140,8));
    require(descriptor.u32(0)==243,"Actual instance count differs");
    require(descriptor.u32(4)>=bsp.memory_address,"Instance owner underflow");
    const auto offset=descriptor.u32(4)-bsp.memory_address;
    (void)metadata.slice(offset,88);return storage.subspan(bsp.file_offset+offset,88);
}
}
int main(int argc,char**argv)try {
    portable();if(argc==1)return 0;
    require(argc==3,"Expected optional MCC/original Old Mombasa paths");
    const halo2::Cache mcc(argv[1]),xbox(argv[2]);require(mcc.is_mcc()&&!xbox.is_mcc()&&!xbox.is_vista(),"Expected actual paired formats");
    const auto a=instance(mcc),b=instance(xbox);const auto raw=hex(donor_hex);
    require(std::ranges::equal(a.first(52),b.first(52)),"Ordinary Cache did not publish the exact original transform");
    require(std::ranges::equal(a.subspan(52),std::span(raw).subspan(52)),"Ordinary Cache replaced a non-transform instance field");
    std::cout<<"PASS actual ordinary MCC Cache transform equals original while retaining MCC instance tail\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
