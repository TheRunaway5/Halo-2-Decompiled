#include "halo2/prepared_original_sound_bank.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace halo2 {
namespace {
using Json=nlohmann::json;
void require(bool yes,const char* message){if(!yes)throw FormatError(message);}
std::vector<std::byte> read_file(const std::filesystem::path& path,std::size_t limit) {
    require(std::filesystem::is_regular_file(std::filesystem::symlink_status(path)),"Prepared sound input is not a regular owned file");
    const auto size=std::filesystem::file_size(path);require(size&&size<=limit,"Prepared sound input exceeds its format bound");
    std::ifstream stream(path,std::ios::binary);std::vector<std::byte> bytes(std::size_t(size),std::byte{});
    require(bool(stream.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))),"Prepared sound input is truncated");
    return bytes;
}
std::string hash(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> digest{};unsigned count{};
    require(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr)==1&&count==digest.size(),
            "Cannot authenticate prepared original sound data");
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:digest){out+=digits[b>>4];out+=digits[b&15];}return out;
}
unsigned digit(char c){
    if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;
    throw FormatError("Prepared sound hex record is not canonical");
}
template<std::size_t N>std::array<std::byte,N> fixed(const Json& value) {
    const auto raw=value.get<std::string>();require(raw.size()==N*2,"Prepared sound literal record has another stride");
    std::array<std::byte,N> result;
    for(unsigned n=0;n<N;++n)result[n]=std::byte((digit(raw[n*2])<<4)|digit(raw[n*2+1]));return result;
}
std::uint32_t word(const Json& value){
    require(value.is_number_integer()&&!value.is_number_float(),"Prepared sound extent is not an integer");
    const auto number=value.get<std::int64_t>();require(number>=0&&std::uint64_t(number)<=0xffffffffu,"Prepared sound extent wraps");
    return std::uint32_t(number);
}
void name(const std::string& value){require(!value.empty()&&value.size()<=4096&&value.find('\0')==std::string::npos,"Prepared sound name is invalid");}
TagHandle datum(const Json& value){
    const auto text=value.get<std::string>();require(text.size()==8,"Prepared original tag identity has another width");
    std::uint32_t result{};for(char c:text)result=(result<<4)|digit(c);
    require(result!=0xffffffffu,"Prepared original tag identity is NONE");return {result};
}
}
std::shared_ptr<const PreparedOriginalSoundBank> PreparedOriginalSoundBank::load(
    const std::filesystem::path& directory,std::string_view expected) {
    require(expected.size()==64,"Prepared sound needs its admitting manifest digest");
    for(char c:expected)(void)digit(c);
    const auto raw=read_file(directory/"manifest.json",1024*1024);
    require(hash(raw)==expected,"Prepared original sound manifest digest differs");
    const auto manifest=Json::parse(reinterpret_cast<const char*>(raw.data()),reinterpret_cast<const char*>(raw.data()+raw.size()));
    const auto schema=manifest.at("schema").get<std::string>();
    const bool v3=schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V3";
    require(v3||schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V1"||schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V2",
            "Unsupported prepared original sound schema");
    const auto& file=manifest.at("encoded_file");require(file.at("path")=="resources.bin","Prepared sound resource escapes its declared owner");
    auto out=std::shared_ptr<PreparedOriginalSoundBank>(new PreparedOriginalSoundBank);
    out->sectors_=read_file(directory/"resources.bin",128*1024*1024);
    require(out->sectors_.size()==word(file.at("bytes"))&&hash(out->sectors_)==file.at("sha256").get<std::string>(),"Prepared sound resource digest/extent differs");
    auto& m=out->metadata_;const auto& contract=manifest.at("original_contract");
    m.name=manifest.at("name").get<std::string>();name(m.name);
    m.sound=fixed<20>(manifest.at("literal_sound_hex"));m.scale=fixed<20>(contract.at("scale_hex"));
    m.playback=fixed<56>(contract.at("playback_hex"));m.sound_class=fixed<92>(contract.at("sound_class_hex"));
    if(v3&&(contract.contains("sound_classes_hex")||
            (manifest.contains("provenance")&&manifest.at("provenance").contains("map_sha256")))){
        const auto& provenance=manifest.at("provenance");auto& origin=m.origin.emplace();
        origin.map=provenance.at("map").get<std::string>();name(origin.map);
        origin.map_sha256=provenance.at("map_sha256").get<std::string>();
        require(origin.map_sha256.size()==64,"Prepared original map digest has another width");for(char c:origin.map_sha256)(void)digit(c);
        origin.sound=datum(provenance.at("sound_datum"));origin.gestalt=datum(provenance.at("gestalt_datum"));origin.classes=datum(provenance.at("class_datum"));
        const auto& rows=contract.at("sound_classes_hex");require(rows.is_array()&&rows.size()==54,"Prepared original class owner is incomplete");
        auto& classes=m.sound_classes.emplace();for(unsigned n=0;n<classes.size();++n)classes[n]=fixed<92>(rows[n]);
        const auto cls=std::to_integer<unsigned>(m.sound[2]);require(cls<classes.size()&&classes[cls]==m.sound_class,"Prepared original selected class differs from its complete owner");
    }
    const Bytes sound(m.sound);
    const unsigned cls=std::to_integer<unsigned>(m.sound[2]),rate=std::to_integer<unsigned>(m.sound[3]);
    const unsigned encoding=std::to_integer<unsigned>(m.sound[4]),compression=std::to_integer<unsigned>(m.sound[5]);
    require(cls<54&&rate<3&&encoding<3&&compression<=4,"Prepared sound has an unsupported original codec/class");
    require(sound.u16(0)==word(contract.at("flags"))&&cls==word(contract.at("sound_class"))&&
            sound.u32(16)==word(contract.at("maximum_play_time")),"Prepared sound literal/normalized root differs");
    require(contract.at("codec")==Json::array({rate,encoding,compression}),"Prepared sound literal/normalized codec differs");
    const auto pitch_count=std::to_integer<unsigned>(m.sound[10]);
    require(pitch_count>0&&sound.u16(6)<0x8000u&&sound.u16(8)<0x8000u&&
            std::to_integer<unsigned>(m.sound[11])<0x80u&&m.sound[13]==std::byte{0xff}&&sound.u16(14)==0xffff,
            "Prepared sound lacks a required pitch/playback/scale/custom/extra-info owner");
    require(v3||(pitch_count==1&&m.sound[12]==std::byte{0xff}),"Legacy prepared sound requires one pitch and no promotion owner");
    const auto& ranges=contract.at("pitch_ranges");
    require(ranges.is_array()&&ranges.size()==pitch_count,"Prepared sound pitch count differs from its literal root");
    if(v3)require(manifest.at("pitch_ranges").is_array()&&manifest.at("pitch_ranges").size()==pitch_count,"Prepared sound omitted literal pitch ranges");
    std::size_t previous_end{};
    for(unsigned pitch=0;pitch<pitch_count;++pitch) {
        auto& range_meta=m.pitch_ranges.emplace_back();
        const auto& normalized=ranges[pitch];
        range_meta.literal=fixed<12>(v3?manifest.at("pitch_ranges")[pitch].at("literal_hex"):manifest.at("literal_pitch_range_hex"));
        const Bytes range(range_meta.literal);
        require(range.u16(0)<0x8000u&&range.u16(2)<0x8000u&&range.u16(8)<0x8000u,"Prepared sound has an invalid signed pitch index");
        range_meta.name=normalized.at("name").get<std::string>();
        if(v3)range_meta.import_name_id=word(normalized.at("import_name_id"));
        if(!range_meta.name.empty()||!v3||range_meta.import_name_id!=0u)name(range_meta.name);
        range_meta.interval=fixed<10>(normalized.at("interval_hex"));
        const auto& permutations=v3?manifest.at("pitch_ranges")[pitch].at("permutations"):manifest.at("permutations");
        const auto& authored=normalized.at("permutations");
        require(permutations.is_array()&&permutations.size()>=1&&permutations.size()<=32&&
                permutations.size()==range.u16(10)&&authored.size()==permutations.size(),"Prepared sound permutation interval differs");
        if(v3||schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V2") {
            const auto& storage=normalized.at("used_flags_hex");
            if(v3&&storage.is_null()) {
                require(permutations.size()==1||(std::to_integer<unsigned>(m.sound_class[10])&0x20u),
                        "Prepared sound omitted required initial permutation storage");
            }else{
            const auto used=storage.get<std::string>();
            require(used.size()==((permutations.size()+7)/8)*2,"Prepared sound initial permutation state has another extent");
            range_meta.used.emplace();
            for(unsigned n=0;n<used.size();n+=2)range_meta.used->push_back(std::byte((digit(used[n])<<4)|digit(used[n+1])));
            }
        }
        auto& encoded_range=out->encoded_.emplace_back();
        for(unsigned n=0;n<permutations.size();++n) {
            const auto& p=permutations[n];const auto& a=authored[n];Permutation meta;
            meta.name=p.at("name").get<std::string>();meta.literal=fixed<16>(p.at("literal_hex"));
            if(v3){meta.import_name_id=word(p.at("import_name_id"));require(*meta.import_name_id==word(a.at("import_name_id")),"Prepared sound import-name identity differs");}
            if(!meta.name.empty()||!v3||meta.import_name_id!=0u)name(meta.name);
            const Bytes literal(meta.literal);
            require(literal.u16(0)<0x8000u&&literal.u16(12)<0x8000u,"Prepared sound has an invalid signed permutation index");
            require(word(p.at("ordinal"))==n&&word(a.at("ordinal"))==n&&a.at("name")==meta.name&&
                    literal.u32(8)==word(p.at("original_sample_size"))&&
                    literal.u32(8)==word(a.at("original_sample_size")),"Prepared sound permutation identity/size differs");
            const auto chance=fixed<6>(p.at("chance_and_copy_hex"));
            require(std::ranges::equal(chance,std::span(meta.literal).subspan(2,6))&&
                    chance==fixed<6>(a.at("chance_and_copy_hex")),"Prepared sound authored permutation controls differ");
            const auto& chunks=p.at("chunks");require(chunks.is_array()&&!chunks.empty()&&chunks.size()==literal.u16(14),"Prepared sound chunk count differs");
            EncodedSound encoded;encoded.name=m.name;encoded.sound_class=cls;
            constexpr unsigned rates[]{22050,44100,32000};encoded.sample_rate=rates[rate];
            encoded.channels=encoding==2?2:encoding+1;encoded.codec=encoding==2?4:compression;encoded.samples=literal.u32(8);
            for(const auto& c:chunks) {
                Chunk chunk{word(c.at("offset")),word(c.at("bytes")),word(c.at("sector_bytes")),word(c.at("source_flags"))};
                if(v3){
                    chunk.literal=fixed<12>(c.at("literal_hex"));const Bytes original(*chunk.literal);
                    require((original.u32(4)&0x3fffffffu)==chunk.bytes&&(original.u32(4)>>30u)==chunk.source_flags&&
                            (original.u32(0)&0x3fffffffu)%512u==0,"Prepared sound literal chunk descriptor differs");
                }
                require(chunk.bytes&&chunk.source_flags<4&&chunk.offset%512==0&&chunk.sector_bytes%512==0&&
                        chunk.bytes<=chunk.sector_bytes&&chunk.sector_bytes-chunk.bytes<512&&
                        chunk.offset==previous_end&&chunk.offset<=out->sectors_.size()&&
                        chunk.sector_bytes<=out->sectors_.size()-chunk.offset,"Prepared sound chunk/sector extent differs");
                const auto sector=std::span(out->sectors_).subspan(chunk.offset,chunk.sector_bytes),data=sector.first(chunk.bytes);
                require(hash(sector)==c.at("sector_sha256").get<std::string>()&&hash(data)==c.at("encoded_sha256").get<std::string>(),"Prepared sound chunk digest differs");
                encoded.data.insert(encoded.data.end(),data.begin(),data.end());
                encoded.packet_chunks.push_back({chunk.bytes,0,false});meta.chunks.push_back(chunk);
                previous_end=std::size_t(chunk.offset)+chunk.sector_bytes;
            }
            require(encoded.data.size()==word(p.at("encoded_bytes"))&&encoded.data.size()==word(a.at("encoded_bytes"))&&
                    hash(encoded.data)==p.at("encoded_sha256").get<std::string>()&&hash(encoded.data)==a.at("encoded_sha256").get<std::string>(),
                    "Prepared original sound permutation payload differs");
            range_meta.permutations.push_back(std::move(meta));encoded_range.push_back(std::move(encoded));
        }
    }
    if(v3) {
        const auto& promotion=contract.at("promotion");const auto index=std::to_integer<unsigned>(m.sound[12]);
        if(index==0xffu)require(promotion.is_null(),"Prepared sound invented a promotion owner");
        else{
            require(index<0x80u&&promotion.is_object(),"Prepared sound lost its promotion owner");
            auto& p=m.promotion.emplace();p.literal=fixed<28>(promotion.at("literal_hex"));
            const auto& rules=promotion.at("rules_hex");const auto& counters=promotion.at("counters_hex");const Bytes row(p.literal);
            require(rules.is_array()&&counters.is_array()&&!rules.empty()&&rules.size()<=65536&&
                    counters.size()>=rules.size()&&counters.size()<=65536&&
                    row.u32(0)==rules.size()&&row.u32(8)==counters.size(),"Prepared sound promotion block extents differ");
            for(const auto& raw:rules){auto rule=fixed<16>(raw);require(Bytes(rule).u16(0)<pitch_count,"Prepared sound promotion rule escapes pitch ranges");p.rules.push_back(rule);}
            for(const auto& raw:counters)p.counters.push_back(fixed<4>(raw));
        }
    }
    require(previous_end==out->sectors_.size(),"Prepared sound bank has unowned trailing data");return out;
}
const EncodedSound& PreparedOriginalSoundBank::encoded(unsigned pitch,unsigned permutation) const {
    require(pitch<encoded_.size()&&permutation<encoded_[pitch].size(),"Prepared sound pitch/permutation is outside its original bank");
    return encoded_[pitch][permutation];
}
}
