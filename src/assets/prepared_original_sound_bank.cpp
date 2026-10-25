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
}
std::shared_ptr<const PreparedOriginalSoundBank> PreparedOriginalSoundBank::load(
    const std::filesystem::path& directory,std::string_view expected) {
    require(expected.size()==64,"Prepared sound needs its admitting manifest digest");
    for(char c:expected)(void)digit(c);
    const auto raw=read_file(directory/"manifest.json",1024*1024);
    require(hash(raw)==expected,"Prepared original sound manifest digest differs");
    const auto manifest=Json::parse(reinterpret_cast<const char*>(raw.data()),reinterpret_cast<const char*>(raw.data()+raw.size()));
    const auto schema=manifest.at("schema").get<std::string>();
    require(schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V1"||schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V2",
            "Unsupported prepared original sound schema");
    const auto& file=manifest.at("encoded_file");require(file.at("path")=="resources.bin","Prepared sound resource escapes its declared owner");
    auto out=std::shared_ptr<PreparedOriginalSoundBank>(new PreparedOriginalSoundBank);
    out->sectors_=read_file(directory/"resources.bin",128*1024*1024);
    require(out->sectors_.size()==word(file.at("bytes"))&&hash(out->sectors_)==file.at("sha256").get<std::string>(),"Prepared sound resource digest/extent differs");
    auto& m=out->metadata_;const auto& contract=manifest.at("original_contract");
    m.name=manifest.at("name").get<std::string>();name(m.name);
    m.sound=fixed<20>(manifest.at("literal_sound_hex"));m.scale=fixed<20>(contract.at("scale_hex"));
    m.playback=fixed<56>(contract.at("playback_hex"));m.sound_class=fixed<92>(contract.at("sound_class_hex"));
    m.range=fixed<12>(manifest.at("literal_pitch_range_hex"));
    const Bytes sound(m.sound),range(m.range);
    const unsigned cls=std::to_integer<unsigned>(m.sound[2]),rate=std::to_integer<unsigned>(m.sound[3]);
    const unsigned encoding=std::to_integer<unsigned>(m.sound[4]),compression=std::to_integer<unsigned>(m.sound[5]);
    require(cls<54&&rate<3&&encoding<3&&compression<=4,"Prepared sound has an unsupported original codec/class");
    require(sound.u16(0)==word(contract.at("flags"))&&cls==word(contract.at("sound_class"))&&
            sound.u32(16)==word(contract.at("maximum_play_time")),"Prepared sound literal/normalized root differs");
    require(contract.at("codec")==Json::array({rate,encoding,compression}),"Prepared sound literal/normalized codec differs");
    require(m.sound[10]==std::byte{1}&&m.sound[12]==std::byte{0xff}&&m.sound[13]==std::byte{0xff}&&sound.u16(14)==0xffff,
            "Prepared sound V1 lacks a required pitch/promotion/custom/extra-info owner");
    const auto& ranges=contract.at("pitch_ranges");require(ranges.is_array()&&ranges.size()==1,"Prepared sound V1 requires one complete pitch range");
    m.pitch_name=ranges[0].at("name").get<std::string>();name(m.pitch_name);
    m.interval=fixed<10>(ranges[0].at("interval_hex"));
    const auto& permutations=manifest.at("permutations");const auto& authored=ranges[0].at("permutations");
    require(permutations.is_array()&&permutations.size()>=1&&permutations.size()<=32&&
            permutations.size()==range.u16(10)&&authored.size()==permutations.size(),"Prepared sound permutation interval differs");
    if(schema=="H2_PREPARED_ORIGINAL_SOUND_BANK_V2") {
        const auto used=ranges[0].at("used_flags_hex").get<std::string>();
        require(used.size()==((permutations.size()+7)/8)*2,"Prepared sound initial permutation state has another extent");
        for(unsigned n=0;n<used.size();n+=2)m.used.push_back(std::byte((digit(used[n])<<4)|digit(used[n+1])));
    }
    std::size_t previous_end{};
    for(unsigned n=0;n<permutations.size();++n) {
        const auto& p=permutations[n];const auto& a=authored[n];Permutation meta;
        meta.name=p.at("name").get<std::string>();name(meta.name);meta.literal=fixed<16>(p.at("literal_hex"));
        const Bytes literal(meta.literal);
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
            require(chunk.bytes&&chunk.source_flags<4&&chunk.offset%512==0&&chunk.sector_bytes%512==0&&
                    chunk.bytes<=chunk.sector_bytes&&chunk.sector_bytes-chunk.bytes<512&&
                    chunk.offset==previous_end&&chunk.offset<=out->sectors_.size()&&
                    chunk.sector_bytes<=out->sectors_.size()-chunk.offset,"Prepared sound chunk/sector extent differs");
            const auto sector=std::span(out->sectors_).subspan(chunk.offset,chunk.sector_bytes),data=sector.first(chunk.bytes);
            require(hash(sector)==c.at("sector_sha256").get<std::string>()&&hash(data)==c.at("encoded_sha256").get<std::string>(),"Prepared sound chunk digest differs");
            encoded.data.insert(encoded.data.end(),data.begin(),data.end());meta.chunks.push_back(chunk);
            previous_end=std::size_t(chunk.offset)+chunk.sector_bytes;
        }
        require(encoded.data.size()==word(p.at("encoded_bytes"))&&encoded.data.size()==word(a.at("encoded_bytes"))&&
                hash(encoded.data)==p.at("encoded_sha256").get<std::string>()&&hash(encoded.data)==a.at("encoded_sha256").get<std::string>(),
                "Prepared original sound permutation payload differs");
        m.permutations.push_back(std::move(meta));out->encoded_.push_back(std::move(encoded));
    }
    require(previous_end==out->sectors_.size(),"Prepared sound bank has unowned trailing data");return out;
}
const EncodedSound& PreparedOriginalSoundBank::encoded(unsigned pitch,unsigned permutation) const {
    require(pitch==0&&permutation<encoded_.size(),"Prepared sound pitch/permutation is outside its original bank");
    return encoded_[permutation];
}
}
