#include "halo2/mainmenu_portal_state.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
struct Fixture {
    unsigned portals{},associations{},indices{};
    std::uint64_t flags_offset{},header_offset{},scenario_offset{};
    std::vector<std::pair<SourcePortalMaskState,SourcePortalMaskState>> cases;
};
Fixture fixture(const std::filesystem::path &path) {
    std::ifstream file(path);std::string line;Fixture result;
    require(bool(std::getline(file,line)) && line=="H2_MAINMENU_PORTAL_STATE_ORIGINAL_V1",
        "original mainmenu portal fixture unavailable or changed");
    require(bool(std::getline(file,line)),"original mainmenu portal asset row missing");
    std::istringstream asset(line);char kind{};
    asset>>kind>>result.portals>>result.associations>>result.indices>>result.flags_offset
        >>result.header_offset>>result.scenario_offset;
    require(bool(asset) && kind=='A' && result.portals==151 && !result.associations && !result.indices,
        "original mainmenu portal asset bank changed");
    std::string extra;require(!(asset>>extra),"original mainmenu portal asset row has extra fields");
    while(std::getline(file,line)) {
        std::istringstream row(line);row>>kind;require(kind=='R',"unknown original mainmenu portal row");
        auto &entry=result.cases.emplace_back();
        for(auto *state:{&entry.first,&entry.second}) {
            for(auto &word:state->words)row>>word;
            for(auto &word:state->acoustic)row>>word;
        }
        require(bool(row) && !(row>>extra),"original mainmenu portal state row truncated or extended");
    }
    require(result.cases.size()==4,"original mainmenu portal fixture case count changed");return result;
}
void same(const SourcePortalMaskState &actual,const SourcePortalMaskState &expected) {
    require(actual.words==expected.words && actual.acoustic==expected.acoustic,
        "mainmenu portal state differs from unchanged original constructor");
}
template<class Function>void rejects(Function function,SourcePortalMaskState &state,unsigned &count) {
    const auto before=state;bool rejected=false;
    try{function();}catch(const FormatError &){rejected=true;}
    require(rejected,"unsupported mainmenu portal bank silently admitted");
    same(state,before);++count;
}
struct PrivateCache {
    std::filesystem::path folder,path;
    explicit PrivateCache(const std::filesystem::path &source) {
        folder=std::filesystem::temp_directory_path()/
            ("halo2-mainmenu-portal-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        require(std::filesystem::create_directory(folder),"private portal cache directory unavailable");
        try{path=folder/"mainmenu.map";std::filesystem::copy_file(source,path);}
        catch(...){std::filesystem::remove_all(folder);throw;}
    }
    ~PrivateCache(){std::error_code error;std::filesystem::remove_all(folder,error);}
    void word(std::uint64_t offset,std::uint32_t value) {
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
        require(bool(file),"private mainmenu portal cache unavailable");
        const std::array<char,4> bytes{char(value),char(value>>8),char(value>>16),char(value>>24)};
        file.seekp(std::streamoff(offset));file.write(bytes.data(),bytes.size());
        require(bool(file),"private mainmenu portal mutation failed");
    }
};
std::uint32_t word(const std::filesystem::path &path,std::uint64_t offset) {
    std::ifstream file(path,std::ios::binary);file.seekg(std::streamoff(offset));std::array<unsigned char,4> bytes{};
    require(bool(file.read(reinterpret_cast<char *>(bytes.data()),bytes.size())),"source portal word unavailable");
    return std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);
}
}
int main(int argc,char **argv){try {
    require(argc==3,"mainmenu portal test needs mainmenu.map and original fixture");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    const auto expected=fixture(argv[2]);const Cache cache(argv[1]);
    const auto definition=cache.load_source_portal_mask_definition(0);
    require(cache.load_source_geometry_scenario_type()==2 && definition.portal_flags.size()==expected.portals &&
        definition.associations.size()==expected.associations && definition.portal_indices.size()==expected.indices &&
        std::all_of(definition.portal_flags.begin(),definition.portal_flags.end(),[](auto flags){return flags==0;}),
        "actual title Cache/BSP bank differs from original source fixture");
    unsigned words{},rejections{};
    for(const auto &[before,after]:expected.cases) {
        auto pure=before,actual=before;
        initialize_mainmenu_portal_state(pure,definition);
        initialize_mainmenu_portal_state(actual,cache,0);
        same(pure,after);same(actual,after);words+=288;
        initialize_mainmenu_portal_state(actual,cache,0);same(actual,after);
    }
    auto state=expected.cases.front().first;
    auto blocked=definition;blocked.portal_flags[0]|=8;
    rejects([&]{initialize_mainmenu_portal_state(state,blocked);},state,rejections);
    auto owned=definition;owned.associations.emplace_back();
    rejects([&]{initialize_mainmenu_portal_state(state,owned);},state,rejections);
    auto indexed=definition;indexed.portal_indices.push_back(0);
    rejects([&]{initialize_mainmenu_portal_state(state,indexed);},state,rejections);
    auto oversized=definition;oversized.portal_flags.resize(513);
    rejects([&]{initialize_mainmenu_portal_state(state,oversized);},state,rejections);
    rejects([&]{initialize_mainmenu_portal_state(state,cache,cache.bsps().size());},state,rejections);
    const auto &bsp=cache.bsps().at(0);
    require(expected.header_offset>=bsp.file_offset && expected.header_offset+16<=std::uint64_t(bsp.file_offset)+bsp.size,
        "original association header escaped actual BSP ownership");
    const auto header_address=bsp.memory_address+std::uint32_t(expected.header_offset-bsp.file_offset);
    for(unsigned mutation=0;mutation<4;++mutation) {
        PrivateCache altered(argv[1]);
        if(mutation==0)altered.word(expected.flags_offset,word(argv[1],expected.flags_offset)|8u);
        if(mutation==1) {altered.word(expected.header_offset,1);altered.word(expected.header_offset+4,header_address);}
        if(mutation==2) {altered.word(expected.header_offset+8,1);altered.word(expected.header_offset+12,header_address+8);}
        if(mutation==3)altered.word(expected.scenario_offset+0x10,
            (word(argv[1],expected.scenario_offset+0x10)&0xffff0000u)|1u);
        const Cache changed(altered.path);
        rejects([&]{initialize_mainmenu_portal_state(state,changed,0);},state,rejections);
    }
    std::cout<<expected.cases.size()<<" unchanged original title constructor cases, "<<words
        <<" mask/acoustic words, 151 actual portals, "<<rejections<<" rejected banks; no visibility/draw admission\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
