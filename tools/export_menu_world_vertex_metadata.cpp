// Explicit offline source maintenance. No runtime code generation dependency.
#include "halo2/menu_world_vertex_catalog.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <fstream>
#include <iostream>
#include <map>

namespace {
using namespace halo2;
using json=nlohmann::json;
json read(const std::filesystem::path& path) {
    std::ifstream stream(path);if(!stream)throw FormatError("Cannot read native metadata source: "+path.string());
    return json::parse(stream);
}
std::vector<std::byte> program(std::string_view text) {
    if(text.size()%2)throw FormatError("Native metadata source program has invalid hex extent");
    const auto digit=[](char value)->unsigned {
        if(value>='0'&&value<='9')return value-'0';
        if(value>='a'&&value<='f')return value-'a'+10;
        throw FormatError("Native metadata source program has invalid hex byte");
    };
    std::vector<std::byte> bytes(text.size()/2);
    for(unsigned index=0;index<bytes.size();++index)bytes[index]=std::byte((digit(text[index*2])<<4)|digit(text[index*2+1]));
    return bytes;
}
std::string hash(std::span<const std::byte> bytes) {
    std::array<unsigned char,32> output{};unsigned size{};
    if(EVP_Digest(bytes.data(),bytes.size(),output.data(),&size,EVP_sha256(),nullptr)!=1 || size!=output.size())
        throw FormatError("Cannot hash native source program");
    constexpr std::string_view digits="0123456789abcdef";std::string result(64,'0');
    for(unsigned index=0;index<output.size();++index){result[index*2]=digits[output[index]>>4];result[index*2+1]=digits[output[index]&15];}
    return result;
}
}
int main(int argc,char** argv)try {
    std::filesystem::path programs,output;std::vector<std::filesystem::path> catalogs;
    for(int index=1;index<argc;++index) {
        const std::string option=argv[index];if(index+1>=argc)throw FormatError("Missing native metadata option value");
        const std::filesystem::path value=argv[++index];
        if(option=="--sources")catalogs.push_back(value);
        else if(option=="--programs"&&programs.empty())programs=value;
        else if(option=="--output"&&output.empty())output=value;
        else throw FormatError("Unknown or duplicate native metadata option");
    }
    if(programs.empty()||catalogs.empty()||output.empty())throw FormatError("Supply --programs --sources --output for offline native metadata export");
    const auto registry=read(programs);
    if(registry.at("schema")!="HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1")throw FormatError("Native metadata source registry schema differs");
    std::map<std::string,std::vector<std::byte>> code;
    for(const auto& row:registry.at("cpu_vertex_programs")) {
        auto bytes=program(row.at("vertex_program_hex").get<std::string>());
        (void)decode_vertex_program(bytes);const auto key=hash(bytes);
        if(!code.emplace(key,std::move(bytes)).second)throw FormatError("Native metadata source duplicates a complete program");
    }
    json rows=json::array();
    for(const auto& path:catalogs) {
        const auto catalog=read(path);const auto abi=catalog.at("abi").get<unsigned>();
        if(catalog.at("schema")!="halo2-native-menu-world-vertex-source-v2" || (abi!=2&&abi!=3&&abi!=4))
            throw FormatError("Native metadata source catalog contract differs");
        for(const auto& row:catalog.at("entries")) {
            const auto key=row.at("program_sha256").get<std::string>();
            const auto& bytes=code.at(key);
            if(bytes.size()!=row.at("program_bytes"))throw FormatError("Native metadata program extent differs from original bytes");
            SourceVertexDeclaration declaration{row.at("class_index"),row.at("semantics").get<std::vector<std::uint16_t>>()};
            const auto source=abi==4?menu_world_renderer_vertex_shader_source(bytes,declaration):abi==3?
                menu_world_material_vertex_shader_source(bytes,declaration):menu_world_vertex_shader_source(bytes,declaration);
            if(json(source.consumed_inputs)!=row.at("consumed_inputs") || json(source.consumed_constants)!=row.at("consumed_constants"))
                throw FormatError("Native offline dependency analysis differs from existing compiled source manifest: "+key);
            rows.push_back({{"class_index",declaration.class_index},{"program_sha256",key},{"program_bytes",bytes.size()},
                {"semantics",declaration.semantics},{"consumed_inputs",source.consumed_inputs},{"consumed_constants",source.consumed_constants},
                {"consumed_input_components",source.consumed_input_components},{"default_texture_w_mask",source.default_texture_w_mask},
                {"relative_constants",decode_vertex_program(bytes).relative_constants}});
        }
    }
    std::filesystem::create_directories(output.parent_path());std::ofstream stream(output);
    if(!stream || !(stream<<json{{"schema","HALO2_RAW_VERTEX_ANALYSIS_V1"},{"entries",rows}}.dump(2)<<'\n'))
        throw FormatError("Cannot write native vertex metadata analysis");
    std::cout<<rows.size()<<" complete raw vertex metadata identities exported offline\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
