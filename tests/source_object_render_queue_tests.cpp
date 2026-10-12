#include "halo2/source_object_render_queue.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string &text){std::istringstream s(text);std::uint32_t word;std::vector<std::uint32_t> out;while(s>>std::hex>>word)out.push_back(word);return out;}
std::vector<std::uint32_t> execute(const std::vector<std::uint32_t> &row){
    std::size_t at=0;const auto take=[&](){check(at<row.size(),"truncated source render queue fixture");return row[at++];};
    const auto version=take();check(version==1 || version==2,"unknown source render queue fixture version");
    const auto nr=take(),no=take(),nd=take(),nm=take(),ng=take();
    check(nr<=256 && no<=256 && nd<=256 && nm<=256 && ng<=256,"source render queue fixture exceeds banks");
    std::vector<SourceBspAdmissionRecord> queue;std::vector<std::pair<std::uint32_t,SourceModelFadeBytes>> fades;
    for(unsigned i=0;i<nr;++i){SourceBspAdmissionRecord r;r.id=take();const auto flags=take(),other=take(),association=take(),first=take(),second=take();
        check(flags<=65535 && association<=65535 && first<=255 && second<=255,"source render queue field exceeds literal width");
        r.flags=std::uint16_t(flags);r.other=other;r.association=std::uint16_t(association);queue.push_back(r);
        fades.push_back({r.id,{std::uint8_t(first),std::uint8_t(second)}});
    }
    std::vector<SourceObjectRenderQueueOwner> owners;for(unsigned i=0;i<no;++i){const auto id=take(),present=take(),definition=take();check(present<=1,"invalid owner pointer presence");owners.push_back({id,bool(present),definition});}
    std::vector<SourceObjectRenderQueueDefinition> definitions;for(unsigned i=0;i<nd;++i){const auto id=take(),present=take(),model=take();check(present<=1,"invalid definition pointer presence");definitions.push_back({id,bool(present),model});}
    std::vector<SourceObjectRenderQueueModel> models;for(unsigned i=0;i<nm;++i){const auto id=take(),present=take(),render=take();check(present<=1,"invalid model pointer presence");models.push_back({id,bool(present),render});}
    std::vector<SourceObjectRenderQueueGeometry> geometry;for(unsigned i=0;i<ng;++i){const auto id=take(),count=take();geometry.push_back({id,std::int32_t(count)});}
    std::vector<SourceModelFadeOwner> fade_owners;std::vector<SourceModelFadeDefinition> fade_defs;std::vector<SourceModelFadeModel> fade_models;
    std::vector<std::pair<std::uint32_t,Vec3>> centers;Vec3 eye;float scale=0;std::uint8_t modifier=0;
    const auto scalar=[&](){return std::bit_cast<float>(take());};
    if(version==2){const auto nf=take(),ndf=take(),nmf=take(),vm=take();check(nf<=256 && ndf<=256 && nmf<=256 && vm<=255,"source live fade bank exceeds bound");modifier=std::uint8_t(vm);
        eye={scalar(),scalar(),scalar()};scale=scalar();
        for(unsigned i=0;i<nf;++i){SourceModelFadeOwner o;o.identity=take();o.flags=take();o.parent=take();o.definition=take();o.radius=scalar();fade_owners.push_back(o);centers.push_back({o.identity,{scalar(),scalar(),scalar()}});}
        for(unsigned i=0;i<ndf;++i){const auto id=take(),model=take();fade_defs.push_back({id,model});}
        for(unsigned i=0;i<nmf;++i){SourceModelFadeModel m;m.identity=take();m.fade_end=scalar();m.fade_start=scalar();for(auto &v:m.render_cutoffs)v=scalar();const auto count=take();check(count<=65535,"source live fade LOD count exceeds width");m.render_lod_count=std::uint16_t(count);fade_models.push_back(m);}
    }
    check(at==row.size(),"trailing source render queue fields");source_object_render_queue_order(queue,owners,definitions,models,geometry);
    std::vector<std::uint32_t> out{nr};
    std::vector<std::uint32_t> live;
    for(auto &r:queue){SourceModelFadeBytes value;
        if(version==2){const auto center=std::find_if(centers.begin(),centers.end(),[&](const auto &entry){return entry.first==r.id;});check(center!=centers.end(),"source live fade center unavailable");
            const float metric=source_geometry_lod_distance(center->second,eye,scale);value=source_model_fade(fade_owners,fade_defs,fade_models,{r.id,metric,modifier});
            live.insert(live.end(),{r.id,std::bit_cast<std::uint32_t>(metric),value.first,value.second});
        }else{const auto f=std::find_if(fades.begin(),fades.end(),[&](const auto &entry){return entry.first==r.id;});check(f!=fades.end(),"queue swap lost fade owner");value=f->second;}
        r.flags=source_object_render_queue_fade_flags(r.flags,value);out.insert(out.end(),{r.id,r.flags,r.other,r.association});}
    if(version==2){out.push_back(nr);out.insert(out.end(),live.begin(),live.end());}
    return out;
}
void controls(){
    constexpr auto none=0xffffffffu;
    const std::vector<SourceBspAdmissionRecord> initial{{0xe0000001,0xfc00,0xffff,0x12345678},{0xe0000002,0xf400,3,0x87654321}};
    std::vector<SourceObjectRenderQueueOwner> o{{0xe0000001,true,0xe0010011},{0xe0000002,true,0xe0010012}};
    std::vector<SourceObjectRenderQueueDefinition> d{{0xe0010011,true,0xe0020021},{0xe0010012,true,0xe0020022}};
    std::vector<SourceObjectRenderQueueModel> m{{0xe0020021,true,0xe0030031},{0xe0020022,true,0xe0030032}};
    std::vector<SourceObjectRenderQueueGeometry> g{{0xe0030031,0},{0xe0030032,1}};
    const auto reject=[&](const auto &owners,const auto &defs,const auto &models,const auto &geo){auto q=initial;bool rejected=false;try{source_object_render_queue_order(q,owners,defs,models,geo);}catch(const FormatError&){rejected=true;}check(rejected,"unresolved render queue accepted");check(q[0].id==initial[0].id && q[1].id==initial[1].id,"unresolved render queue partly mutated");};
    auto oo=o;oo[1].body_present.reset();reject(oo,d,m,g);oo=o;oo[1].definition.reset();reject(oo,d,m,g);
    auto dd=d;dd[1].present.reset();reject(o,dd,m,g);dd=d;dd[1].model.reset();reject(o,dd,m,g);
    auto mm=m;mm[1].present.reset();reject(o,d,mm,g);mm=m;mm[1].render_model.reset();reject(o,d,mm,g);
    auto gg=g;gg[1].prt_info_count.reset();reject(o,d,m,gg);gg=g;gg.pop_back();reject(o,d,m,gg);
    oo=o;oo.push_back(o.back());reject(oo,d,m,g);dd=d;dd.push_back(d.back());reject(o,dd,m,g);
    // Source null-body branch doesn't read a definition; explicit NONE skips
    // lookup entirely. A nonzero first fade never clears an existing mark.
    oo=o;oo[1].body_present=false;oo[1].definition.reset();auto q=initial;source_object_render_queue_order(q,oo,d,m,g);check(q[0].id==initial[0].id,"null body promoted");
    q={{none,0,0xffff,0}};source_object_render_queue_order(q,{}, {}, {},{});
    check(source_object_render_queue_fade_flags(0xfc01,{255,0})==0xfc01,"nonzero fade cleared prior marker");
    check(source_object_render_queue_fade_flags(0xf400,{0,255})==0xf401,"wrong fade byte selected");
    std::cout<<"10 unresolved controls and4 early/byte controls pass\n";
}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line))if(!line.empty()){for(auto v:execute(words(line)))std::cout<<std::hex<<v<<' ';std::cout<<'\n';}return 0;}
    check(argc==1,"expected optional --probe");controls();std::ifstream file(std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_object_render_queue.tsv");check(bool(file),"original render queue fixture missing");
    std::string line;unsigned cases=0,count=0;
    while(std::getline(file,line)){if(line.empty() || line[0]=='#')continue;const auto split=line.find('|');check(split!=std::string::npos,"missing original render queue output");
        const auto expected=words(line.substr(split+1));check(execute(words(line.substr(0,split)))==expected,"original render queue differs");++cases;count+=unsigned(expected.size());}
    check(cases>100,"empty original render queue coverage");std::cout<<cases<<" original134240/163D50/133790 queue cases / "<<count<<" words exact; current-model PRT prefix swaps and first-fade mark\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
