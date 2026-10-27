// Explicit offline resource audit for independently proved PC sky material pairs.
#include "halo2/sky_input_identity.hpp"
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using namespace halo2::sky_audit;
int main(int argc,char** argv)try {
    if(argc!=6)throw FormatError("usage: audit_pc_sky_bitmaps PROJECT PREPARED PAIRS_JSON REPORT SCRATCH_EXPORT");
    const std::filesystem::path root=argv[1],prepared=argv[2],pairs_path=argv[3],output=argv[4],exports=argv[5];
    std::filesystem::create_directories(exports);
    std::ifstream source(pairs_path);const auto pairs=json::parse(source);
    if(pairs.at("schema")!="H2_PC_SKY_PAIRED_RESOURCE_INPUTS_V1" || pairs.at("pairs").size()!=19)
        throw FormatError("Unexpected proved PC sky resource pairing cohort");
    json report={{"schema","H2_PC_SKY_RESOURCE_AUDIT_V1"},{"pairs",json::array()}};
    std::map<std::string,std::unique_ptr<Cache>> originals,actuals;unsigned failures=0;
    for(const auto& pair:pairs.at("pairs")) {
        const auto original_name=pair.at("source_map").get<std::string>(),actual_name=pair.at("map").get<std::string>()+".map";
        if(!originals.contains(original_name))originals[original_name]=std::make_unique<Cache>(root/original_name);
        const auto actual_path=prepared/"halo2/h2_maps_win64_dx11"/actual_name;
        if(!actuals.contains(actual_name))actuals[actual_name]=std::make_unique<Cache>(actual_path,actual_path.parent_path(),nullptr,prepared);
        auto& original=*originals.at(original_name);auto& actual=*actuals.at(actual_name);
        const auto find=[](const Cache& cache,const std::string& name)->std::uint32_t {
            std::optional<std::uint32_t> found;
            for(const auto& tag:cache.tags())if(tag.group=="bitm" && tag.name==name && tag.address) {
                if(found)throw FormatError("Ambiguous proved PC sky bitmap name");found=tag.datum;
            }
            if(!found)throw FormatError("Missing proved PC sky bitmap owner");return *found;
        };
        const auto xdatum=find(original,pair.at("original_bitmap")),ydatum=find(actual,pair.at("bitmap"));
        const auto x=original.load_bitmap(xdatum,pair.at("original_frame")),y=actual.load_bitmap(ydatum,pair.at("frame"));
        auto row=pair;row["map"]=actual_name;row["original_datum"]=xdatum;row["prepared_datum"]=ydatum;
        row["original"]=texture(x);row["prepared"]=texture(y);row["equal"]=texture_equal(x,y);
        if(!row["equal"].get<bool>()) {
            ++failures;
            for(const auto& [suffix,image]:std::array<std::pair<std::string,const Texture*>,2>{{{"target",&x},{"donor",&y}}}) {
                const auto file=sha256({image->pixels.data(),image->pixels.size()})+"."+suffix;
                const auto path=exports/file;
                if(!std::filesystem::exists(path)) {
                    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(image->pixels.data()),image->pixels.size());
                    if(!out)throw FormatError("Cannot export exact PC sky correction input");
                }
                row[suffix+"_file"]=file;
            }
        }
        report["pairs"].push_back(row);
    }
    report["failures"]=failures;report["passed"]=failures==0;
    std::ofstream out(output);out<<report.dump(2)<<'\n';if(!out)throw FormatError("Cannot save PC sky resource audit");
    std::cout<<report["pairs"].size()<<" complete PC bitmap pairs, "<<failures<<" differing inputs\n";
    return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
