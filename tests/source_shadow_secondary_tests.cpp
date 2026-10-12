#include "halo2/source_shadow_secondary.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
using namespace halo2;using Words=std::vector<std::uint32_t>;
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
struct Fixture {
    SourceShadowSecondaryClassification input;
    std::vector<SourceShadowSecondaryOwner> owners;
    std::vector<SourceShadowSecondaryDefinition> definitions;
    std::vector<SourceShadowSecondaryModel> models;
    std::array<std::vector<SourceBspAdmissionRecord>,3> storage;
    SourceShadowSecondaryRows rows;
};
Fixture load(const Words& row){
    std::size_t at=0;auto take=[&](){require(at<row.size(),"truncated source secondary fixture");return row[at++];};
    require(take()==1,"unknown source secondary fixture");Fixture f;
    f.input={take(),std::uint8_t(take()),take()};const auto no=take(),nd=take(),nm=take();
    require(no<=2048&&nd<=2048&&nm<=2048,"source secondary metadata count");
    std::array<unsigned,3> sizes{take(),take(),take()};std::array<std::int16_t,3> counts{};
    for(auto& count:counts)count=std::int16_t(take());
    for(unsigned i=0;i<no;++i){const auto id=take(),definition=take(),flags=take(),type=take();require(type<=255,"source secondary type");f.owners.push_back({id,definition,flags,std::uint8_t(type)});}
    for(unsigned i=0;i<nd;++i){const auto id=take(),model=take();f.definitions.push_back({id,model});}
    for(unsigned i=0;i<nm;++i){const auto id=take(),mode=take();f.models.push_back({id,mode});}
    for(unsigned j=0;j<3;++j){require(sizes[j]<=4096,"source secondary storage count");
        for(unsigned i=0;i<sizes[j];++i){SourceBspAdmissionRecord record;record.id=take();const auto flags=take();record.other=take();const auto association=take();require(flags<=65535&&association<=65535,"source secondary row width");record.flags=std::uint16_t(flags);record.association=std::uint16_t(association);f.storage[j].push_back(record);}}
    require(at==row.size(),"extra source secondary input words");
    f.rows={{counts[0],f.storage[0]},{counts[1],f.storage[1]},{counts[2],f.storage[2]}};return f;
}
Words run(Fixture& f){
    source_shadow_secondary_classify(f.rows,f.owners,f.definitions,f.models,f.input);
    Words out{std::uint16_t(f.rows.clusters.count),std::uint16_t(f.rows.objects.count),std::uint16_t(f.rows.instances.count)};
    for(const auto& storage:f.storage)for(const auto& r:storage)out.insert(out.end(),{r.id,r.flags,r.other,r.association});return out;
}
Words parse(const std::string& line){std::istringstream in(line);std::uint32_t v;Words out;while(in>>std::hex>>v)out.push_back(v);return out;}
Words evaluate(const Words& row){auto f=load(row);return run(f);}
void print(const Words& out){for(unsigned i=0;i<out.size();++i)std::cout<<(i?" ":"")<<std::hex<<out[i];std::cout<<'\n';}
void controls(const Words& baseline){
    auto reject=[&](Fixture& f,const char* message){bool error=false;try{run(f);}catch(const FormatError&){error=true;}require(error,message);};
    auto f=load(baseline);f.owners.clear();reject(f,"unresolved root lookup accepted");
    f=load(baseline);f.owners.front().definition.reset();reject(f,"unresolved current definition accepted");
    f=load(baseline);f.definitions.clear();reject(f,"unresolved current definition lookup accepted");
    f=load(baseline);f.definitions.front().model.reset();reject(f,"unresolved current model datum accepted");
    f=load(baseline);f.models.clear();reject(f,"unresolved root render model accepted");
    f=load(baseline);f.models.front().render_model.reset();reject(f,"unresolved mode datum accepted");
    f=load(baseline);f.rows.objects.count=2;reject(f,"queue count beyond supplied rows accepted");
    f=load(baseline);f.owners.front().flags.reset();f.owners.front().type.reset();run(f);
    f=load(baseline);f.rows.objects.count=-1;f.models.clear();run(f);
    //No root row means hlmt+4 is unread. A type-mask hit skips object flags.
    f=load(baseline);f.storage[1][0].id=0xe0000140u;
    f.owners.push_back({0xe0000140u,std::nullopt,std::nullopt,std::uint8_t(8)});
    f.models.clear();f.input.receiver_type_mask=1u<<8;run(f);
    require(f.storage[1][0].flags==0xd000u,"type-mask hit/rootless queue differs");
    f.storage[1][0].flags=0xf000u;f.input.receiver_type_mask=0;
    reject(f,"unresolved nonroot flag31 accepted");
    f.input.receiver_type_mask=0xffffffffu;f.owners.back().type.reset();
    reject(f,"unresolved nonroot type accepted");
    //Root NONE must read no lookup/queue field. ModelNONE writes only count.
    f=load(baseline);f.input.root=0xffffffffu;f.owners.clear();f.definitions.clear();f.models.clear();f.rows.objects.count=32767;run(f);
    f=load(baseline);f.definitions.front().model=0xffffffffu;f.models.clear();f.rows.objects.count=32767;
    const auto before=f.storage;run(f);require(f.rows.clusters.count==0&&f.rows.objects.count==32767,"modelNONE count/early branch differs");
    for(unsigned j=0;j<3;++j)for(unsigned i=0;i<before[j].size();++i)
        require(before[j][i].flags==f.storage[j][i].flags,"modelNONE changed retained storage");
}
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string_view(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line))if(!line.empty())print(evaluate(parse(line)));return 0;}
    const auto path=argc==2?std::filesystem::path(argv[1]):std::filesystem::path(HALO2_SOURCE_DIR)/"tests/fixtures/retail_source_shadow_secondary.tsv";
    std::ifstream file(path);require(bool(file),"source secondary fixture unavailable");
    std::size_t cases=0,checked=0,changed=0;Words first;
    std::string line;while(std::getline(file,line)){
        if(line.empty()||line[0]=='#')continue;const auto sep=line.find('|');require(sep!=line.npos,"secondary fixture separator");
        const auto input=parse(line.substr(0,sep)),expected=parse(line.substr(sep+1));auto f=load(input);const auto initial=f.storage;const auto actual=run(f);
        if(actual!=expected){std::cerr<<"source secondary case "<<cases<<" mismatch\n";throw FormatError("secondary classification differs from original");}
        if(first.empty())first=input;
        for(unsigned j=0;j<3;++j)for(unsigned i=0;i<initial[j].size();++i)changed+=initial[j][i].flags!=f.storage[j][i].flags;
        ++cases;checked+=actual.size();
    }
    require(cases>=1000&&changed>0,"secondary fixture has inadequate branch coverage");controls(first);
    std::cout<<cases<<" original secondary classifications / "<<checked<<" exact words; "<<changed
        <<" changed flags; retained root row and 9 rejection/5 unread-field controls pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
