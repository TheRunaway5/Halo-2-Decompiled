#include "halo2/audio.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <span>

using nlohmann::json;
using namespace halo2;
namespace {
std::string digest(std::span<const std::byte> bytes) {
    unsigned char value[EVP_MAX_MD_SIZE];unsigned size{};
    if(!EVP_Digest(bytes.data(),bytes.size(),value,&size,EVP_sha256(),nullptr) || size!=32)
        throw std::runtime_error("Payload digest failed");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(unsigned i=0;i<size;++i){result+=hex[value[i]>>4];result+=hex[value[i]&15];}
    return result;
}
EncodedSound load(const json& row) {
    EncodedSound s;s.name=row.at("name");s.codec=row.at("codec");s.channels=row.at("channels");
    s.sample_rate=row.at("rate");s.samples=row.at("samples");s.sound_class=row.at("sound_class");
    for(const auto& part:row.at("chunks")) {
        const auto bytes=part.at("bytes").get<std::size_t>();
        if(!bytes || bytes>128*1024*1024-s.data.size())throw std::runtime_error("Unbounded payload extent");
        std::ifstream file(part.at("path").get<std::string>(),std::ios::binary);
        file.seekg(part.at("offset").get<std::uint64_t>());
        const auto at=s.data.size();s.data.resize(at+bytes);
        if(!file.read(reinterpret_cast<char*>(s.data.data()+at),std::streamsize(bytes)))
            throw std::runtime_error("Short payload read");
        if(part.contains("pcm_bytes"))s.packet_chunks.push_back({unsigned(bytes),part.at("pcm_bytes"),part.at("reset_after")});
    }
    return s;
}
json compare(const json& item) {
    const auto a=load(item.at("original")),b=load(item.at("mcc"));
    const auto x=decode_sound(a),y=decode_sound(b);
    if(x.stereo.empty() || y.stereo.empty() || x.rate!=48000 || y.rate!=48000)
        throw std::runtime_error("Empty/incorrect-rate decoded payload");
    const auto overlap=std::min(x.stereo.size(),y.stereo.size());
    double xx{},yy{},xy{},error{},peak{};std::size_t mismatches{};
    for(std::size_t i=0;i<overlap;++i) {
        const double v=x.stereo[i],w=y.stereo[i],d=v-w;
        if(!std::isfinite(v) || !std::isfinite(w))throw std::runtime_error("Nonfinite waveform");
        xx+=v*v;yy+=w*w;xy+=v*w;error+=d*d;peak=std::max(peak,std::abs(d));mismatches+=v!=w;
    }
    for(const auto* pcm:{&x,&y})for(float value:pcm->stereo)
        if(!std::isfinite(value))throw std::runtime_error("Nonfinite waveform tail");
    const bool format=a.codec==b.codec && a.sample_rate==b.sample_rate && a.channels==b.channels;
    return {{"key",item.at("key")},{"original_codec",a.codec},{"mcc_codec",b.codec},
        {"original_rate",a.sample_rate},{"mcc_rate",b.sample_rate},
        {"original_channels",a.channels},{"mcc_channels",b.channels},
        {"same_encoded_format",format},{"same_encoded_payload",a.data==b.data},
        {"original_encoded_sha256",digest(a.data)},{"mcc_encoded_sha256",digest(b.data)},
        {"original_pcm_sha256",digest(std::as_bytes(std::span(x.stereo)))},
        {"mcc_pcm_sha256",digest(std::as_bytes(std::span(y.stereo)))},
        // Xbox ugh! permutation+8 equals compressed byte extent in this
        // corpus. Preserve the word without labeling it a PCM frame count.
        {"original_ugh_sample_size_word",a.samples},{"mcc_authored_samples",b.samples},
        {"original_decoded_frames",x.frames()},{"mcc_decoded_frames",y.frames()},
        {"exact_same_pc_decoded_pcm",x.stereo==y.stereo},
        {"overlap_stereo_samples",overlap},{"overlap_mismatched_samples",mismatches},
        {"overlap_rms_original",std::sqrt(xx/double(overlap))},
        {"overlap_rms_mcc",std::sqrt(yy/double(overlap))},
        {"overlap_rms_error",std::sqrt(error/double(overlap))},{"overlap_peak_error",peak},
        {"overlap_zero_lag_normalized_correlation",xx>0 && yy>0?json(xy/std::sqrt(xx*yy)):json(nullptr)}};
}
}
int main(int argc,char** argv) {try {
    if(argc!=3)throw std::runtime_error("Expected owned manifest and small report path");
    std::ifstream input(argv[1]);const auto manifest=json::parse(input);
    const auto& cases=manifest.at("cases");if(cases.empty() || cases.size()>4096)throw std::runtime_error("Invalid case extent");
    json rows=json::array();unsigned exact{},identical{};
    for(const auto& item:cases) {
        auto row=compare(item);exact+=row.at("exact_same_pc_decoded_pcm").get<bool>();
        identical+=row.at("same_encoded_format").get<bool>() && row.at("same_encoded_payload").get<bool>();
        rows.push_back(std::move(row));
    }
    json result={{"passed",true},{"scope","Named Xbox/MCC resources decoded by the same current PC decoder; no Xbox hardware, DSP, timing or audible-delivery proof"},
        {"pairs",rows.size()},{"exact_same_pc_decoded_pcm_pairs",exact},{"identical_format_payload_pairs",identical},
        {"xbox_pcm_equality_verified",false},{"audible_delivery_verified",false},{"cases",rows}};
    std::ofstream out(argv[2]);out<<result.dump(2)<<'\n';if(!out)throw std::runtime_error("Report write failed");
    std::cout<<json{{"passed",true},{"pairs",rows.size()},{"exact_same_pc_decoded_pcm_pairs",exact},
        {"identical_format_payload_pairs",identical}}<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
