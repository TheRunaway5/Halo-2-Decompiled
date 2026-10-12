#include "halo2/source_object_vector_contact.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
Words parse(std::string_view text) {
    std::istringstream in{std::string(text)};Words words;std::uint32_t value;
    while(in>>std::hex>>value)words.push_back(value);
    if(!in.eof())throw FormatError("invalid object-vector fixture word");
    return words;
}
struct Reader {
    const Words& words;std::size_t at{};
    std::uint32_t take() {
        if(at==words.size())throw FormatError("short object-vector fixture");
        return words[at++];
    }
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 vector(){const auto x=scalar(),y=scalar(),z=scalar();return {x,y,z};}
    void done() const {if(at!=words.size())throw FormatError("extra object-vector fixture words");}
};
Words packet(const PosedCollisionHit& hit) {
    return {std::bit_cast<std::uint32_t>(hit.fraction),
        std::bit_cast<std::uint32_t>(hit.plane.normal.x),std::bit_cast<std::uint32_t>(hit.plane.normal.y),
        std::bit_cast<std::uint32_t>(hit.plane.normal.z),std::bit_cast<std::uint32_t>(hit.plane.distance),
        hit.packed_bsp,std::uint32_t(hit.leaf),std::uint32_t(hit.surface),std::uint32_t(hit.signed_plane),
        std::uint32_t(hit.region),std::uint32_t(hit.node),std::uint32_t(hit.model_material),
        std::uint32_t(hit.global_material),hit.flags,hit.breakable};
}
// Only fraction and packed-BSP below are original target observations. The
// other fields are explicit native copy controls for the opaque target packet.
PosedCollisionHit target_packet(std::uint32_t fraction,std::uint32_t bsp) {
    return {std::bit_cast<float>(fraction),{{1.f,-0.f,3.f},7.f},bsp,-17,5,2,9,4,6,11,0xab,0xcd};
}
void malformed_controls() {
    SourceObjectVectorOwner owner;owner.full_handle=0x12340001;
    SourceObjectVectorInput input;input.flags=0x0481fffdu;
    auto miss=[](std::uint32_t,const SourceObjectVectorOwner&,const SourceObjectVectorInput&){return std::optional<PosedCollisionHit>{};};
    auto reject=[&](const auto& operation) {
        try{operation();}catch(const FormatError&){return;}
        throw FormatError("malformed native object-vector bank admitted");
    };
    reject([&]{source_object_vector_contact(std::span(&owner,1),owner.full_handle,true,input,1.f,{});});
    reject([&]{source_object_vector_contact(std::span(&owner,1),0x12340002,true,input,1.f,miss);});
    const std::array duplicate{owner,owner};
    reject([&]{source_object_vector_contact(duplicate,owner.full_handle,true,input,1.f,miss);});
    owner.sibling=owner.full_handle;
    reject([&]{source_object_vector_contact(std::span(&owner,1),owner.full_handle,true,input,1.f,miss);});
    owner.sibling=0xffffffffu;owner.child_head=owner.full_handle;
    reject([&]{source_object_vector_contact(std::span(&owner,1),owner.full_handle,true,input,1.f,miss);});
    owner.child_head=0xffffffffu;owner.full_handle=0x123407ff;
    unsigned called=0;
    const auto last=source_object_vector_contact(std::span(&owner,1),owner.full_handle,true,input,1.f,
        [&](std::uint32_t requested,const SourceObjectVectorOwner& actual,const SourceObjectVectorInput&) {
            if(requested!=owner.full_handle || actual.full_handle!=owner.full_handle)
                throw FormatError("last original object slot lookup differs");
            ++called;return std::optional<PosedCollisionHit>{};
        });
    if(called!=1 || last.targets!=std::vector{owner.full_handle})
        throw FormatError("last original object slot rejected");
    reject([&]{source_object_vector_contact(std::span(&owner,1),0x12340800,true,input,1.f,miss);});
    owner.full_handle=0x12340800;
    reject([&]{source_object_vector_contact(std::span(&owner,1),owner.full_handle,true,input,1.f,miss);});
}
}
int main(int argc,char**argv) {try {
    if(argc!=2)throw FormatError("object-vector test requires original fixture");
    std::ifstream file(argv[1]);if(!file)throw FormatError("object-vector fixture missing");
    std::vector<SourceObjectVectorOwner> owners;std::unordered_map<std::uint16_t,std::uint32_t> slots;
    std::string line;std::size_t cases=0,words=0,targets=0,hits=0,fraction_controls=0,salt_controls=0;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        if(line.starts_with("O ")) {
            const auto values=parse(std::string_view(line).substr(2));Reader r{values};
            SourceObjectVectorOwner owner;owner.full_handle=r.take();owner.flags=r.take();
            owner.header_flags=std::uint8_t(r.take());owner.header_type=std::uint8_t(r.take());
            owner.sibling=r.take();owner.child_head=r.take();owner.unit_flags_10a=std::uint8_t(r.take());
            owner.biped_flags_348=std::uint8_t(r.take());owner.prop_flags_12c=r.take();
            owner.primitive_sphere.center=r.vector();owner.primitive_sphere.radius=r.scalar();r.done();
            if(!slots.emplace(std::uint16_t(owner.full_handle),owner.full_handle).second)
                throw FormatError("duplicate frozen object-vector slot");
            owners.push_back(owner);continue;
        }
        const auto tab=line.find('\t'),a=line.find('|',tab),b=line.find('|',a+1);
        if(tab==std::string::npos || a==std::string::npos || b==std::string::npos)
            throw FormatError("object-vector fixture separators missing");
        const auto label=line.substr(0,tab);
        const auto inputs=parse(std::string_view(line).substr(tab+1,a-tab-1));Reader r{inputs};
        const auto first=r.take();const bool admitted=r.take()!=0;
        SourceObjectVectorInput input;input.flags=r.take();input.lower_flags=r.take();
        input.excluded_a=r.take();input.excluded_b=r.take();
        const auto maximum=r.scalar();input.origin=r.vector();input.delta=r.vector();r.done();
        const auto services=parse(std::string_view(line).substr(a+1,b-a-1));Reader service{services};
        const auto count=service.take();std::optional<PosedCollisionHit> winning_packet;
        float best=maximum;std::uint32_t consumed=0;
        const auto result=source_object_vector_contact(owners,first,admitted,input,maximum,
            [&](std::uint32_t requested,const SourceObjectVectorOwner& actual,const SourceObjectVectorInput& forwarded)->std::optional<PosedCollisionHit> {
                if(consumed++>=count || requested!=service.take())
                    throw FormatError(label+": original target order differs");
                if(slots.at(std::uint16_t(requested))!=actual.full_handle)
                    throw FormatError(label+": requested salt replaced actual low16 lookup");
                if(&forwarded!=&input || forwarded.lower_flags!=9)
                    throw FormatError(label+": source target query context changed");
                if(!service.take())return {};
                const auto fraction=service.take(),bsp=service.take();auto hit=target_packet(fraction,bsp);
                if(hit.fraction<best){best=hit.fraction;winning_packet=hit;}
                return hit;
            });
        service.done();if(consumed!=count)throw FormatError(label+": original target list not consumed");
        Words actual{std::uint32_t(bool(result.contact))};
        if(result.contact) {
            actual.insert(actual.end(),{result.contact->object,std::bit_cast<std::uint32_t>(result.contact->target.fraction),result.contact->target.packed_bsp});
            if(!winning_packet || packet(result.contact->target)!=packet(*winning_packet))
                throw FormatError(label+": supplied complete target packet changed");
        }
        actual.push_back(std::uint32_t(result.targets.size()));actual.insert(actual.end(),result.targets.begin(),result.targets.end());
        const auto expected=parse(std::string_view(line).substr(b+1));
        if(actual!=expected)throw FormatError(label+": original contact or order differs");
        ++cases;words+=expected.size();targets+=count;hits+=bool(result.contact);
        fraction_controls+=label.starts_with("exact_fraction_");salt_controls+=label.starts_with("salt_alias_");
    }
    if(!file.eof() || owners.size()!=202 || cases!=5066 || targets!=3782 || hits!=1531 || words!=18507 || salt_controls!=68 || fraction_controls!=102)
        throw FormatError("incomplete original object-vector fixture");
    malformed_controls();
    std::cout<<owners.size()<<" literal owners; "<<cases<<" unchanged-source traversal cases; "<<targets
        <<" original posed-target outcomes; "<<hits<<" strict replacements; "<<words<<" exact source output words; "
        <<fraction_controls<<" exact-fraction controls; "<<salt_controls<<" salt-alias controls\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
