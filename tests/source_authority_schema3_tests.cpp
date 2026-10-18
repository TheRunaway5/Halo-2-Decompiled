#include "halo2/source_authority_codec.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <zlib.h>

using namespace halo2;
namespace {
using Buffer=std::vector<std::byte>;
void require(bool value,const char *why){if(!value)throw FormatError(why);}
unsigned checks{};
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
    require(rejected,"Invalid schema3 authority archive was accepted");++checks;}
std::uint64_t get(std::span<const std::byte> b,std::size_t at,unsigned count){
    require(at<=b.size()&&count<=b.size()-at,"Invalid private wire fixture range");std::uint64_t value{};
    for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint64_t>(b[at+i])<<(8*i);return value;}
void put(Buffer &b,std::size_t at,std::uint64_t value,unsigned count){
    require(at<=b.size()&&count<=b.size()-at,"Invalid private wire fixture store");
    for(unsigned i=0;i<count;++i)b[at+i]=std::byte(value>>(8*i));}
Buffer unpack(const Buffer &frame){
    const auto size=get(frame,16,8);Buffer result(static_cast<std::size_t>(size));uLongf n=result.size();uLong used=frame.size()-64;
    require(uncompress2(reinterpret_cast<Bytef*>(result.data()),&n,reinterpret_cast<const Bytef*>(frame.data()+64),&used)==Z_OK&&n==size&&used==frame.size()-64,
        "Cannot expand private authority fixture");return result;}
Buffer repack(const Buffer &frame,const Buffer &raw){
    Buffer result(frame.begin(),frame.begin()+64),compressed(compressBound(raw.size()));uLongf size=compressed.size();
    require(compress2(reinterpret_cast<Bytef*>(compressed.data()),&size,reinterpret_cast<const Bytef*>(raw.data()),raw.size(),Z_BEST_SPEED)==Z_OK,
        "Cannot recompress private authority fixture");
    put(result,16,raw.size(),8);put(result,24,size,8);
    require(SHA256(reinterpret_cast<const unsigned char*>(raw.data()),raw.size(),reinterpret_cast<unsigned char*>(result.data()+32)),"Cannot hash private authority fixture");
    result.insert(result.end(),compressed.begin(),compressed.begin()+size);return result;
}
// A private wire walker locates actual named fields. Mutations below keep the
// envelope lengths, compression and checksum valid, so schema guards do the work.
struct Node {
    unsigned tag{};std::size_t begin{},end{},name_begin{};
    std::vector<std::pair<std::string,Node>> fields;
    std::vector<Node> values;
    const Node &field(std::string_view name)const{
        const auto found=std::ranges::find(fields,name,&decltype(fields)::value_type::first);
        require(found!=fields.end(),"Private authority fixture field missing");return found->second;}
};
Node parse(std::span<const std::byte> b,std::size_t &at,unsigned depth=0){
    require(depth<128,"Private authority fixture recursion overflow");Node n;n.begin=at;n.tag=unsigned(get(b,at++,1));
    const auto advance=[&](std::uint64_t bytes){require(bytes<=b.size()-at,"Truncated private authority fixture");at+=std::size_t(bytes);};
    const auto count=[&](unsigned bytes){const auto value=get(b,at,bytes);advance(bytes);return value;};
    switch(n.tag){
    case 0:advance(1);break;
    case 1:case 2:case 4:case 14:advance(8);break;
    case 3:advance(4);break;
    case 5:case 6:{const auto bytes=count(8);advance(bytes);break;}
    case 7:case 8:case 9:case 10:{const auto size=count(8);require(size<=b.size()-at,"Private authority fixture container overflow");
        for(std::uint64_t i=0;i<size;++i){n.values.push_back(parse(b,at,depth+1));if(n.tag==9)n.values.push_back(parse(b,at,depth+1));}break;}
    case 11:case 12:if(count(1))n.values.push_back(parse(b,at,depth+1));break;
    case 13:{const auto size=count(4);require(size<=b.size()-at,"Private authority fixture record overflow");
        for(std::uint64_t i=0;i<size;++i){const auto name_begin=at,length=count(2);require(length<=b.size()-at,"Private authority field name overflow");
            std::string name(reinterpret_cast<const char*>(b.data()+at),std::size_t(length));advance(length);auto child=parse(b,at,depth+1);child.name_begin=name_begin;
            n.fields.emplace_back(std::move(name),std::move(child));}break;}
    default:throw FormatError("Unknown private authority fixture tag");}
    n.end=at;return n;
}
Node tree(const Buffer &raw){std::size_t at{};auto result=parse(raw,at);require(at==raw.size(),"Private authority fixture has trailing bytes");return result;}
SourceMatchAuthority::Checkpoint fixture(const nlohmann::json &original){
    require(original.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&original.at("cases").size()==80,
        "Original TU1.5 BAF10 CPU fixture identity differs");
    const auto named=std::ranges::find_if(original.at("cases"),[](const auto&r){return r.at("named").template get<bool>();});
    require(named!=original.at("cases").end(),"Original named marker fixture missing");const auto &lookups=named->at("lookups");
    SourceMatchAuthority::Checkpoint saved{};saved.match.source_backed=true;saved.match.simulation.source_backed=true;
    saved.match.simulation.object_bodies=SourceObjectBodyStorage(256);
    // These four argument identities come from the genuine BAF10 CPU fixture.
    // Its oracle stops at B9460 before full return; caller_return below is its
    // declared caller sentinel, NOT a completed C0790/World admission receipt.
    SourceObjectAttachmentTu15 event{lookups.at(0).at(0),lookups.at(0).at(1),lookups.at(1).at(0),lookups.at(1).at(1),0x03000000u};
    saved.world.world.model_attachments.emplace(event.child,event);
    saved.world.world.model_collectors.emplace(std::pair{1u,event.parent},SourceObjectDrawCollector{});
    saved.world.world.bank.regions.push_back({0x62000000u,SourceRuntimeMemory::Access::ReadWrite,"declared serializer bytes",0,{std::byte{0x31},std::byte{0x72}}, {}});
    return saved;
}
void run(const nlohmann::json &original){
    auto saved=fixture(original);require(saved.world.world.version==3,"Current World schema is not3");
    const auto encoded=encode_source_authority(saved);require(encoded.size()>=64&&get(encoded,8,4)==1&&get(encoded,12,4)==0,"Archive framing changed");
    constexpr std::string_view magic="H2AUTH01";require(std::ranges::equal(std::span(encoded).first(8),std::as_bytes(std::span(magic))),"Archive magic changed");
    const auto decoded=decode_source_authority(encoded);require(encode_source_authority(decoded)==encoded,"Strict schema3 decode/re-encode changed bytes");
    require(decoded.world.world.model_attachments==saved.world.world.model_attachments&&decoded.world.world.bank==saved.world.world.bank,
        "Attachment fields or independent bank bytes were lost");++checks;
    const auto raw=unpack(encoded);const auto parsed=tree(raw);const auto &world=parsed.field("world").field("world");
    require(world.fields.size()==29,"Current native World named field count differs");const auto &attachments=world.field("model_attachments");
    require(attachments.tag==9&&attachments.values.size()==2,"Actual attachment map shape differs");
    const auto &event=attachments.values.at(1);require(event.fields.size()==5&&event.field("parent").tag==1&&event.field("child_marker").tag==1,
        "Attachment serializer fields differ");++checks;
    for(const unsigned version:{2u,4u}){
        auto wrong=saved;wrong.world.world.version=version;rejects([&]{(void)encode_source_authority(wrong);});
        auto changed=raw;put(changed,world.field("version").begin+1,version,8);rejects([&]{(void)decode_source_authority(repack(encoded,changed));});
    }
    // A real old native record has28fields; a fake current-shaped version2 is
    // tested above separately. No empty attachment map migration is admitted.
    auto legacy=raw;put(legacy,world.field("version").begin+1,2,8);put(legacy,world.begin+1,28,4);
    legacy.erase(legacy.begin()+attachments.name_begin,legacy.begin()+attachments.end);
    rejects([&]{(void)decode_source_authority(repack(encoded,legacy));});
    auto missing=raw;put(missing,world.begin+1,28,4);missing.erase(missing.begin()+attachments.name_begin,missing.begin()+attachments.end);
    rejects([&]{(void)decode_source_authority(repack(encoded,missing));});
    auto unknown=raw;unknown[attachments.name_begin+2]=std::byte{'x'};rejects([&]{(void)decode_source_authority(repack(encoded,unknown));});
    auto duplicate=raw;const auto pair=Buffer(raw.begin()+attachments.values.at(0).begin,raw.begin()+attachments.values.at(1).end);
    duplicate.insert(duplicate.begin()+attachments.end,pair.begin(),pair.end());put(duplicate,attachments.begin+1,2,8);
    rejects([&]{(void)decode_source_authority(repack(encoded,duplicate));});
    auto wrong_type=raw;wrong_type[event.field("parent").begin]=std::byte{2};rejects([&]{(void)decode_source_authority(repack(encoded,wrong_type));});
    auto invalid_enum=raw;const auto &state=parsed.field("match").field("simulation").field("player").field("value").field("state");
    put(invalid_enum,state.begin+1,255,8);rejects([&]{(void)decode_source_authority(repack(encoded,invalid_enum));});
    auto pointer=saved;pointer.match.simulation.player.value.custom_animation=std::make_shared<AnimationClip>();rejects([&]{(void)encode_source_authority(pointer);});
    for(const auto slot:{8u,12u}){auto bad=encoded;bad[slot]=std::byte{2};rejects([&]{(void)decode_source_authority(bad);});}
    auto bad=encoded;bad.pop_back();rejects([&]{(void)decode_source_authority(bad);});
    bad=encoded;bad.push_back(std::byte{});rejects([&]{(void)decode_source_authority(bad);});
    bad=encoded;bad[32]^=std::byte{1};rejects([&]{(void)decode_source_authority(bad);});
    bad=encoded;bad[64]^=std::byte{1};rejects([&]{(void)decode_source_authority(bad);});
    auto trailing=raw;trailing.push_back(std::byte{});rejects([&]{(void)decode_source_authority(repack(encoded,trailing));});
    for(unsigned budget=0;budget<4;++budget){auto limits=SourceAuthorityCodecLimits{};
        if(budget==0)limits.encoded_bytes=63;if(budget==1)limits.decoded_bytes=16;
        if(budget==2)limits.allocated_bytes=1;if(budget==3)limits.container_elements=1;
        rejects([&]{(void)decode_source_authority(encoded,limits);});
    }
    auto limits=SourceAuthorityCodecLimits{};limits.decoded_bytes=16;rejects([&]{(void)encode_source_authority(saved,limits);});
    std::cout<<"PASS "<<checks<<" schema3 attachment archive/strict rejection checks; framing1 unchanged. "
        "BAF10 argument fixture only; actual World attachment admission/restore is a separate integration gate.\n";
}
}
int main(int argc,char **argv)try{
    const auto path=argc==2?std::filesystem::path(argv[1]):std::filesystem::path(__FILE__).parent_path()/"fixtures/campaign_object_attachment_tu15.json";
    std::ifstream input(path);require(bool(input),"Original attachment CPU fixture unavailable");nlohmann::json original;input>>original;run(original);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
