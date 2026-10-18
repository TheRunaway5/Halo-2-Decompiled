#include "halo2/mcc_gameplay_unit_scalar.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <iostream>

namespace {
using namespace halo2;
struct Field {
    std::uint32_t descriptor,count,index,stride,offset;
    std::vector<std::uint32_t> donor,target;
};
struct Case {TagGroup group;std::string_view name;std::vector<Field> fields;};
const std::array cases{
    Case{tag_groups::biped,"objects\\characters\\masterchief\\masterchief",{
        {0x284,3,2,0x80,0x2c,{0x3e9851ec},{0x3e9851eb}},
        {0x284,3,2,0x80,0x70,{0,0,0x3e9851ec},{0,0,0x3e9851eb}}}},
    Case{tag_groups::biped,"objects\\characters\\grunt\\grunt",{
        {0x284,3,1,0x80,0x2c,{0x3e4f5c2a},{0x3e4f5c29}},
        {0x284,3,1,0x80,0x70,{0,0,0x3e4f5c2a},{0,0,0x3e4f5c29}},
        {0x284,3,2,0x80,0x2c,{0x3e828f5d},{0x3e828f5c}},
        {0x284,3,2,0x80,0x70,{0,0,0x3e828f5d},{0,0,0x3e828f5c}}}},
    Case{tag_groups::vehicle,"objects\\vehicles\\warthog\\turrets\\gauss\\gauss",{
        {0xffffffffu,0,0,0,0x2e4,{0x3ebc385c},{0x3ebc385b}},
        {0x1c8,2,0,0xb0,0,{0x00001098},{0x00001198}}}},
    Case{tag_groups::vehicle,"objects\\vehicles\\scorpion\\scorpion",{
        {0xffffffffu,0,0,0,0x2c0,{0x3effffff},{0x3f000000}}}},
    Case{tag_groups::vehicle,"objects\\vehicles\\banshee\\banshee",{
        {0xffffffffu,0,0,0,0x2e4,{0x3fd71be0},{0x3fd71be1}}}}
};
void require(bool value,const char* why){if(!value)throw FormatError(why);}
void put(std::span<std::byte> bytes,std::uint32_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));
}
std::uint32_t bank(const Field& f){return f.descriptor==0x284?0x2000:0x2400;}
std::uint32_t address(const Field& f){return (f.descriptor==0xffffffffu?0x1000:bank(f)+f.index*f.stride)+f.offset;}
struct Fixture {
    std::vector<std::byte> bytes=std::vector<std::byte>(0x2000);
    explicit Fixture(const Case& c) {
        for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(n*17u+0x47u);
        for(const auto& f:c.fields) {
            if(f.descriptor!=0xffffffffu){put(bytes,f.descriptor,f.count);put(bytes,f.descriptor+4,bank(f));}
            for(unsigned w=0;w<f.donor.size();++w)put(bytes,address(f)-0x1000+w*4,f.donor[w]);
        }
    }
    auto reader() {
        return [this](std::uint32_t at,std::size_t size) {
            if(at<0x1000)throw FormatError("Fixture address underflow");
            Bytes(bytes).require(at-0x1000,size);return std::span(bytes).subspan(at-0x1000,size);
        };
    }
};
void synthetic() {
    for(const auto& c:cases) {
        Fixture f(c);auto expected=f.bytes;
        for(const auto& field:c.fields)for(unsigned w=0;w<field.target.size();++w)
            put(expected,address(field)-0x1000+w*4,field.target[w]);
        const auto receipt=convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,f.reader());
        require(receipt && receipt->status==MccGameplayUnitScalarStatus::Converted && f.bytes==expected,
            "Known normalized unit tuple changed another authored byte or missed exact target bits");
        require(receipt->field_count==c.fields.size(),"Unit receipt omitted a tuple guard field");
        for(unsigned n=0;n<c.fields.size();++n) {
            const auto& actual=receipt->fields[n];const auto& field=c.fields[n];
            require(actual.address==address(field) && actual.words==field.donor.size(),"Unit receipt lost source address or vector width");
            for(unsigned w=0;w<field.donor.size();++w)require(actual.before[w]==field.donor[w] &&
                actual.after[w]==field.target[w],"Unit receipt changed exact authored words");
        }
        const auto repeat=convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,f.reader());
        require(repeat && repeat->status==MccGameplayUnitScalarStatus::AlreadyEquivalent && f.bytes==expected,
            "Already-target unit tuple was not idempotent");
        for(const auto& field:c.fields)for(unsigned w=0;w<field.donor.size();++w) {
            Fixture authored(c);put(authored.bytes,address(field)-0x1000+w*4,field.donor[w]^0x1000u);
            const auto before=authored.bytes;const auto mismatch=convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,authored.reader());
            require(mismatch && mismatch->status==MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized &&
                authored.bytes==before,"Mod-authored scalar/vector component was overwritten");
        }
        Fixture custom(c);const auto before_custom=custom.bytes;
        require(!convert_mcc_gameplay_unit_scalars(c.group,std::string(c.name)+"_custom",0x1000,custom.reader()) &&
            !convert_mcc_gameplay_unit_scalars(tag_groups::weapon,c.name,0x1000,custom.reader()) && custom.bytes==before_custom,
            "Unit tuple profile reinterpreted a custom path or another group");
        Fixture short_owner(c);const auto& last=c.fields.back();short_owner.bytes.resize(address(last)-0x1000+last.donor.size()*4-1);
        const auto before_short=short_owner.bytes;bool rejected=false;
        try{(void)convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,short_owner.reader());}catch(const FormatError&){rejected=true;}
        require(rejected && short_owner.bytes==before_short,"A short metadata owner was partially written");
        const auto nested=std::ranges::find_if(c.fields,[](const auto& field){return field.descriptor!=0xffffffffu;});
        if(nested!=c.fields.end()) {
            Fixture shape(c);put(shape.bytes,nested->descriptor,nested->count+1);const auto before=shape.bytes;
            const auto mismatch=convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,shape.reader());
            require(mismatch && mismatch->status==MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized && shape.bytes==before,
                "A custom block shape was converted, including an earlier root field");
            Fixture missing(c);put(missing.bytes,nested->descriptor+4,0);const auto original=missing.bytes;rejected=false;
            try{(void)convert_mcc_gameplay_unit_scalars(c.group,c.name,0x1000,missing.reader());}catch(const FormatError&){rejected=true;}
            require(rejected && missing.bytes==original,"A missing nested owner was partly changed");
        }
    }
    Fixture partial(cases[1]);const auto& first=cases[1].fields[0];put(partial.bytes,address(first)-0x1000,first.target[0]);
    const auto before=partial.bytes;const auto receipt=convert_mcc_gameplay_unit_scalars(cases[1].group,cases[1].name,0x1000,partial.reader());
    require(receipt && receipt->status==MccGameplayUnitScalarStatus::AuthoredTupleUnrecognized && partial.bytes==before,
        "A partly authored multi-shape tuple was completed without evidence");
    Fixture incomplete_bank(cases[2]);incomplete_bank.bytes.resize(0x1404);
    const auto original_bank=incomplete_bank.bytes;bool rejected=false;
    try{(void)convert_mcc_gameplay_unit_scalars(cases[2].group,cases[2].name,0x1000,incomplete_bank.reader());}
    catch(const FormatError&){rejected=true;}
    require(rejected && incomplete_bank.bytes==original_bank,
        "A truncated declared bank was accepted because its selected field still fitted");
    std::cout<<"PASS 5 normalized unit fixtures: exact scalar/vector words, all-or-none guards, ownership, custom blocks and idempotence\n";
}
std::uint32_t actual_address(const Cache& cache,const Tag& tag,const Field& f) {
    if(f.descriptor==0xffffffffu)return tag.address+f.offset;
    const Bytes block(cache.read_tag_bytes(tag.address+f.descriptor,8));
    require(block.u32(0)==f.count,"Actual unit definition differs from reviewed block shape");
    return block.u32(4)+f.index*f.stride+f.offset;
}
void actual(const std::filesystem::path& original,const std::filesystem::path& donor) {
    const Cache xbox(original,{},OriginalData::native_tu15()),mcc(donor);unsigned tags{},fields{};
    require(!xbox.is_mcc() && mcc.is_mcc(),"Unit integration requires actual Xbox and MCC maps");
    for(const auto& c:cases) {
        const auto* a=xbox.query_tag({.group=c.group,.name=c.name,.require_address=true,.minimum_size=1});
        const auto* b=mcc.query_tag({.group=c.group,.name=c.name,.require_address=true,.minimum_size=1});
        if(!a || !b)continue;
        for(const auto& field:c.fields) {
            const Bytes expected(xbox.read_tag_bytes(actual_address(xbox,*a,field),field.target.size()*4));
            const Bytes converted(mcc.read_tag_bytes(actual_address(mcc,*b,field),field.target.size()*4));
            for(unsigned w=0;w<field.target.size();++w)require(expected.u32(w*4)==field.target[w] &&
                converted.u32(w*4)==expected.u32(w*4),"Cache did not publish the exact original unit scalar/vector words");
            ++fields;
        }
        ++tags;
    }
    require(tags!=0,"No reviewed unit profile exists in actual pair");
    std::cout<<"PASS "<<mcc.name()<<": "<<tags<<" actual vanilla unit definitions / "<<fields<<" exact named fields\n";
}
}
int main(int argc,char** argv)try {
    synthetic();if(argc==3)actual(argv[1],argv[2]);
    else if(argc!=1)throw halo2::FormatError("usage: mcc_gameplay_unit_scalar_tests [XBOX.map MCC.map]");
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
