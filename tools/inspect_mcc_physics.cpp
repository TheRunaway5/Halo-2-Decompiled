#include "halo2/cache.hpp"
#include "halo2/mcc_physics.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc,char** argv)try {
    if(argc!=3 && argc!=4)throw halo2::FormatError("usage: inspect_mcc_physics MCC_MAP NEW_REFERENCE_PACKET_DIRECTORY [--collision|--bsp]");
    std::string group="phmo";
    if(argc==4) {
        if(std::string(argv[3])=="--collision")group="coll";
        else if(std::string(argv[3])=="--bsp")group="sbsp";
        else throw halo2::FormatError("Unknown physics packet option");
    }
    const halo2::Cache cache(argv[1]);
    if(!cache.is_mcc())throw halo2::FormatError("Expected an MCC cache");
    const std::filesystem::path output=argv[2];
    std::filesystem::create_directories(output.parent_path());
    if(!std::filesystem::create_directory(output))throw halo2::FormatError("Reference packet output must be a new directory");
    nlohmann::json rows=nlohmann::json::array();std::size_t bytes=0;
    for(const auto& tag:cache.tags())if(tag.group==group && tag.address && tag.size) {
        // Export the exact canonical definition published by normal Cache
        // loading. A second conversion would reinterpret an original layout
        // as an MCC donor layout and invalidate the comparison.
        auto packet=cache.tag_payload({tag.datum});
        std::vector<std::byte> converted;
        if(group=="sbsp") {
            converted=halo2::normalize_mcc_structure_bsp_physics(packet,tag.address);
            packet=converted;
        }
        std::ostringstream filename;filename<<std::hex<<std::setfill('0')<<std::setw(8)<<tag.datum<<".bin";
        std::ofstream file(output/filename.str(),std::ios::binary);
        file.write(reinterpret_cast<const char*>(packet.data()),std::streamsize(packet.size()));
        if(!file)throw halo2::FormatError("Could not write owned physics reference packet");
        rows.push_back({{"datum",tag.datum},{"path",tag.name},{"file",filename.str()},
            {"address",tag.address},{"bytes",packet.size()}});bytes+=packet.size();
    }
    std::ofstream manifest(output/"packets.json");
    manifest<<nlohmann::json{{"schema","H2_MCC_PHYSICS_REFERENCE_PACKETS_V1"},
        {"map",cache.path().string()},{"role","Offline constructor comparison; not a native-world snapshot"},
        {"group",group},
        {"packets",rows}}.dump(2)<<'\n';
    if(!manifest)throw halo2::FormatError("Could not write physics reference packet manifest");
    std::cout<<"Normalized "<<group<<" packets: "<<rows.size()<<" bytes: "<<bytes<<"; no Xbox executable input\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
