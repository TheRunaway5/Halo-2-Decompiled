#include "halo2/cache.hpp"
#include "halo2/mcc_native_library.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{},negatives{};
void check(bool value,const char* message){++checks;if(!value)throw FormatError(message);}
std::vector<std::byte> file(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f || f.tellg()<0)throw FormatError("Missing test source");
    std::vector<std::byte> b(std::size_t(f.tellg()));f.seekg(0);
    if(!b.empty() && !f.read(reinterpret_cast<char*>(b.data()),b.size()))throw FormatError("Truncated test source");
    return b;
}
void write(const std::filesystem::path& path,std::span<const std::byte> b){
    std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());
    if(!f)throw FormatError("Test source write failed");
}
void word(std::vector<std::byte>& b,unsigned at,unsigned value){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(i*8));}
Json policy(){std::ifstream f(std::filesystem::path(__FILE__).parent_path().parent_path()/"tools/data/mcc-native-hud-bootstrap.json");return Json::parse(f);}
struct Temporary {
    std::filesystem::path path;
    Temporary(){path=std::filesystem::temp_directory_path()/std::filesystem::path("h2-hud-bootstrap-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(path);}
    ~Temporary(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Fixture {
    Cache& cache;bool wrong_sid{};std::vector<Tag> directory;Json data=policy();std::map<unsigned,std::vector<std::byte>> edits;
    explicit Fixture(Cache& c):cache(c),directory(c.tags()) {
        for(const auto& r:data.at("owners")){
            auto& t=directory.at(r.at("datum").get<unsigned>()&0xffffu);
            check(t.datum==r.at("datum"),"Actual full-salt source identity changed");
            t.address=r.at("address");t.size=r.at("size");
        }
        const auto found=std::ranges::find_if(directory,[](const Tag& t){return t.group=="vrtx" && t.name=="rasterizer\\vertex_shaders_xbox\\screen";});
        if(found!=directory.end()){
            check(found+1==directory.end(),"Native allocator did not append its real owner");directory.pop_back();
        }
    }
    auto sid(){return [&](unsigned id){return wrong_sid?std::string("custom"):cache.string_id(id);};}
    MccShaderRead read(){return [&](unsigned at,std::size_t size){
        if(auto p=edits.find(at);p!=edits.end())return std::span<const std::byte>(p->second).first(size);
        return cache.read_tag_bytes(at,size);
    };}
};
Json actual(const std::filesystem::path& map){
    Cache c(map);Fixture f(c);const auto root=c.shared_directory().parent_path().parent_path()/"native-shaders";
    Json bank;const auto paths=bootstrap_mcc_native_hud_library(bank,map,f.directory,f.read(),f.sid(),root);
    check(paths.size()==3 && bank.at("shader_tags").size()==3 && !bank.contains("rasterizer"),"Exact minimal native closure not staged");
    const auto native=std::ranges::find_if(c.tags(),[](const Tag& t){return t.group=="vrtx" && t.name=="rasterizer\\vertex_shaders_xbox\\screen";});
    check(native!=c.tags().end() && native->size==156 && native->address && c.is_preserved_native_shader(*native),"Real native vertex owner not published");
    check(native->handle().index()==f.directory.size() && native->datum==(0xd0000000u|unsigned(f.directory.size())),"Existing native allocator ownership differs");
    for(const auto& r:bank.at("shader_tags")){
        const auto* t=c.query_tag({.group=TagGroup::from_string(r.at("group").get<std::string>()),.name=r.at("name").get<std::string>(),.require_address=true});
        check(t && c.is_preserved_native_shader(*t),"Minimal native closure owner not registered");
        auto expected=file(paths.at(r.at("payload").get<std::string>()));
        for(const auto& p:r.at("pointers"))word(expected,p.at(0),t->address+p.at(1).get<unsigned>());
        for(const auto& p:r.at("references")){
            const auto* target=c.query_tag({.group=TagGroup::from_string(p.at(1).at(0).get<std::string>()),.name=p.at(1).at(1).get<std::string>(),.require_address=true});
            check(target,"Typed native dependency absent");word(expected,p.at(0),target->datum);
        }
        const auto got=c.read_tag_bytes(t->address,t->size);
        check(std::ranges::equal(got,expected),"Normal Cache native payload/fixups differ");
    }
    const auto* dx9=c.find_tag({f.data.at("owners").at(0).at("datum").get<unsigned>()});
    check(dx9 && dx9->address==7137892 && dx9->size==1600 && !c.is_preserved_native_shader(*dx9),"MCC DX9 screen owner was replaced or admitted as native");
    const auto* grey=c.query_tag({.group=TagGroup{"shad"},.name="ui\\hud\\shaders\\widget_simple_flash_grey",.require_address=true});
    check(grey && grey->size==676 && c.is_preserved_native_shader(*grey),"Normal Cache grey metadata not projected");
    check(!c.load_material(grey->datum,true).passes.empty(),"Actual grey material has no real native pixel pass");
    Json again;check(bootstrap_mcc_native_hud_library(again,map,c.tags(),[&](unsigned at,std::size_t n){return c.read_tag_bytes(at,n);},[&](unsigned id){return c.string_id(id);},root).empty() && again.is_null(),"Already-published namespace bootstrapped twice");
    auto reject=[&](auto mutate){Fixture n(c);mutate(n);Json out;check(bootstrap_mcc_native_hud_library(out,map,n.directory,n.read(),n.sid(),root).empty() && out.is_null(),"Custom/partial MCC closure admitted");++negatives;};
    for(const auto& owner:f.data.at("owners")){
        const auto datum=owner.at("datum").get<unsigned>(),index=datum&0xffffu;
        reject([&](Fixture& n){n.directory[index].datum^=0x10000u;});
        reject([&](Fixture& n){n.directory[index].address+=4;});
        reject([&](Fixture& n){--n.directory[index].size;});
        reject([&](Fixture& n){n.directory[index].group="bitm";});
        reject([&](Fixture& n){n.directory[index].name+="_custom";});
        reject([&](Fixture& n){const auto bytes=c.read_tag_bytes(owner.at("address"),owner.at("size"));auto& edit=n.edits[owner.at("address")];edit.assign(bytes.begin(),bytes.end());edit.back()^=std::byte{1};});
    }
    reject([](Fixture& n){n.directory.push_back(n.directory.at(3821225142u&0xffffu));});
    reject([](Fixture& n){auto t=n.directory.at(3821225142u&0xffffu);t.datum^=0x10000u;n.directory.push_back(t);});
    reject([](Fixture& n){std::swap(n.directory.at(3821225142u&0xffffu),n.directory.at(3821290679u&0xffffu));});
    reject([](Fixture& n){n.directory.push_back({"vrtx","rasterizer\\vertex_shaders_xbox\\screen",0xd0000000u|unsigned(n.directory.size()),1,156});});
    reject([](Fixture& n){n.wrong_sid=true;});
    for(const auto& region:f.data.at("grey_borrowed_regions")){
        const auto at=region.at("address").get<unsigned>(),size=region.at("bytes").get<unsigned>();
        reject([&](Fixture& n){const auto bytes=c.read_tag_bytes(at,size);auto& edit=n.edits[at];edit.assign(bytes.begin(),bytes.end());edit.back()^=std::byte{1};});
    }
    Temporary temp;
    auto changed=file(map);changed.back()^=std::byte{1};const auto changed_map=temp.path/map.filename();write(changed_map,changed);
    Json out;check(bootstrap_mcc_native_hud_library(out,changed_map,f.directory,f.read(),f.sid(),root).empty() && out.is_null(),"Customized compressed source map admitted");++negatives;
    changed.pop_back();write(changed_map,changed);check(bootstrap_mcc_native_hud_library(out,changed_map,f.directory,f.read(),f.sid(),root).empty() && out.is_null(),"Wrong compressed map extent admitted");++negatives;
    std::filesystem::create_directories(temp.path/c.name());std::ofstream(temp.path/c.name()/"library.json")<<"{\"custom\":true}";
    check(bootstrap_mcc_native_hud_library(out,map,f.directory,f.read(),f.sid(),temp.path).empty() && out.is_null(),"Existing custom manifest was supplemented");++negatives;
    std::filesystem::remove(temp.path/c.name()/"library.json");
    auto bad_source=[&](bool altered_json,bool altered_payload){
        const auto dest=temp.path/"01b_spacestation";std::filesystem::create_directories(dest/"payloads");
        auto source=file(root/"01b_spacestation/library.json");
        if(altered_json){auto j=Json::parse(source);j["custom"]=true;std::ofstream(dest/"library.json")<<j.dump();}
        else write(dest/"library.json",source);
        for(const auto& [name,path]:paths){auto b=file(path);if(altered_payload && name==paths.begin()->first)b.back()^=std::byte{1};write(dest/"payloads"/name,b);}
        Json result;bool threw=false;try{(void)bootstrap_mcc_native_hud_library(result,map,f.directory,f.read(),f.sid(),temp.path);}catch(const FormatError&){threw=true;}
        check(threw && result.is_null(),"Untrusted retained source/program published partial closure");++negatives;
    };
    bad_source(true,false);bad_source(false,true);
    return {{"map",c.name()},{"native_records",3},{"new_real_vertex_owner",native->datum},{"grey_bytes",grey->size},{"original_dx9_preserved",true},{"complete_native_payloads_equal",true}};
}
}
int main(int argc,char** argv)try{
    Json untouched;unsigned reads{};const auto read=[&](unsigned,std::size_t)->std::span<const std::byte>{++reads;throw FormatError("Unexpected read");};
    check(bootstrap_mcc_native_hud_library(untouched,"/absent/custom.map",{},read,[](unsigned){return std::string{};},"/absent").empty() && untouched.is_null() && !reads,"Unknown map accessed source/native files");
    untouched={{"custom",true}};const auto saved=untouched;
    check(bootstrap_mcc_native_hud_library(untouched,"/absent/needle.map",{},read,[](unsigned){return std::string{};},"/absent").empty() && untouched==saved && !reads,"Existing library modified");
    Json result;
    if(argc>1){if(std::string(argv[1]).empty() || !std::filesystem::is_regular_file(argv[1]))return 77;result=actual(argv[1]);}
    std::cout<<Json{{"checks",checks},{"negative_cases",negatives},{"actual",result}}.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
