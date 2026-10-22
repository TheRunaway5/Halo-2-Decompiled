#include "decoder_observer.hpp"
#include "halo2/cache.hpp"
#include "halo2/mcc_cache.hpp"
#include <libdeflate.h>
#include <nlohmann/json.hpp>
#include <zlib.h>
#include <dlfcn.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/evp.h>
#endif

namespace halo2 {std::vector<std::byte> decode_mcc_cache_reference(std::span<const std::byte>);}
namespace {
using Buffer=std::vector<std::byte>;
void require(bool value,const char* why) {if(!value)throw std::runtime_error(why);}
void word(Buffer& b,std::size_t at,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(8*i));
}
std::string digest(std::span<const std::byte> data) {
    require(data.size()<=1024ull*1024*1024,"Digest input exceeds the map bound");
    std::array<unsigned char,32> bytes{};
#if defined(__APPLE__)
    require(CC_SHA256(data.data(),CC_LONG(data.size()),bytes.data()),"SHA256 unavailable");
#else
    unsigned count{};
    require(EVP_Digest(data.data(),data.size(),bytes.data(),&count,EVP_sha256(),nullptr)==1&&count==bytes.size(),
        "SHA256 unavailable");
#endif
    std::ostringstream out;for(auto b:bytes)out<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(b);return out.str();
}
Buffer fixture() {
    Buffer raw(8192);word(raw,0,0x68656164);word(raw,4,13);word(raw,8,raw.size());
    word(raw,28,1);word(raw,776,raw.size()-896);word(raw,784,896);word(raw,788,1);
    Buffer payload(raw.size()-896);for(unsigned i=0;i<payload.size();++i)payload[i]=std::byte((i*37+13)%251);
    uLongf size=compressBound(payload.size());Buffer compressed(size);
    require(compress2(reinterpret_cast<Bytef*>(compressed.data()),&size,
        reinterpret_cast<const Bytef*>(payload.data()),payload.size(),Z_BEST_SPEED)==Z_OK,"Fixture compression failed");
    compressed.resize(size);raw.resize(904);word(raw,896,size);word(raw,900,904);
    raw.insert(raw.end(),compressed.begin(),compressed.end());return raw;
}
nlohmann::json compare(std::span<const std::byte> encoded) {
    const auto start=std::chrono::steady_clock::now();auto original=halo2::decode_mcc_cache_reference(encoded);
    const auto middle=std::chrono::steady_clock::now();auto decoded=halo2::decode_mcc_cache(encoded);
    const auto finish=std::chrono::steady_clock::now();
    require(original==decoded,"Production MCC decoder differs from its retained serial reference");
    return {{"encoded_bytes",encoded.size()},{"decoded_bytes",decoded.size()},
        {"encoded_sha256",digest(encoded)},{"decoded_sha256",digest(decoded)},
        {"serial_seconds",std::chrono::duration<double>(middle-start).count()},
        {"accelerated_seconds",std::chrono::duration<double>(finish-middle).count()},
        {"all_bytes_equal",true}};
}
}
int main(int argc,char** argv)try {
    require(argc==2||argc==3,"Supply the library directory and optional genuine MCC map");
    nlohmann::json report{{"schema","H2_MACOS_LIBDEFLATE_DECODER_V1"},{"header_version",LIBDEFLATE_VERSION_STRING}};
#if defined(__aarch64__) || defined(__arm64__)
    report["architecture"]="arm64";
#elif defined(__x86_64__)
    report["architecture"]="x86_64";
#endif
    Dl_info info{};
    require(dladdr(reinterpret_cast<const void*>(&libdeflate_zlib_decompress_ex),&info)&&info.dli_fname,
        "Cannot identify the actual libdeflate image");
    const auto loaded=std::filesystem::canonical(info.dli_fname);
    require(loaded.parent_path()==std::filesystem::canonical(argv[1]),"Decoder borrowed an unexpected library");
    report["loaded_libdeflate"]=loaded.string();report["fixture"]=compare(fixture());
    const auto observed=decoder_observation();
    require(observed.calls&&observed.successes&&observed.output_bytes==8192-896,
        "Production fixture never completed an actual accelerated decode");
    report["fixture_accelerator"]={{"calls",observed.calls},{"successes",observed.successes},
        {"input_bytes",observed.input_bytes},{"output_bytes",observed.output_bytes}};
    if(argc==3&&*argv[2]) {
        std::ifstream file(argv[2],std::ios::binary|std::ios::ate);
        require(bool(file),"Cannot open owned MCC map");const auto count=file.tellg();
        require(count>=896&&count<=1024ll*1024*1024,"Invalid owned MCC map extent");
        Buffer encoded(static_cast<std::size_t>(count));file.seekg(0);
        require(bool(file.read(reinterpret_cast<char*>(encoded.data()),encoded.size())),"Short owned MCC map read");
        report["owned_map"]=compare(encoded);
    }
    const auto total=decoder_observation();
    report["accelerator"]={{"calls",total.calls},{"successes",total.successes},
        {"input_bytes",total.input_bytes},{"output_bytes",total.output_bytes}};
    report["passed"]=true;std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
