#include "halo2/source_prt.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F f) {
    try {f();}catch(const FormatError &) {return;}
    throw std::runtime_error("Malformed MCC PRT input was admitted");
}
void word(std::vector<std::byte> &out,unsigned offset,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)out.at(offset+n)=std::byte(value>>(n*8));
}
void admission() {
    // Actual Scarab row0, independently read in v1089/v1090 from the
    // original and prepared MCC maps. Its nonzero cluster detects treating
    // the expanded field as a short or reading weights at the old offset.
    SourcePrtDefinition d;d.stream_descriptor[0]=std::byte{56};d.stream_descriptor[1]=std::byte{12};
    d.lods.push_back({0,{{3,5}}});
    const std::array<std::uint8_t,12> row{0,0,0xc0,0x40,0xb3,0x15,0x4f,0xd9,0x63,5,0x2e,0xf7};
    d.stream.resize(24);for(unsigned n=0;n<row.size();++n)d.stream[n+12]=std::byte(row[n]);
    const auto value=source_prt_vertex_stream(d,5,3,1);
    check(value && value->size()==1 && value->front().cluster==6.f,"MCC PRT lost its original five-short LOD offset");
    check(value->front().weights[0]>0 && value->front().weights[1]<0 &&
        value->front().weights[2]>0 && value->front().weights[3]<0,"MCC PRT weight origin differs");
    check(!source_prt_vertex_stream(d,5,4,1),"Absent PRT section acquired a fabricated stream");
    auto bad=d;bad.lods.front().sections.front().offset=6;
    rejects([&]{source_prt_vertex_stream(bad,5,3,1);});
    bad=d;bad.stream_descriptor[2]=std::byte{1};rejects([&]{source_prt_vertex_stream(bad,5,3,1);});
    for(auto bits:{0x3f000000u,0x7fc00000u,0x7f800000u,0x47000000u,0x80000000u}) {
        bad=d;word(bad.stream,12,bits);rejects([&]{source_prt_vertex_stream(bad,5,3,1);});
    }
    bad=d;bad.stream.pop_back();rejects([&]{source_prt_vertex_stream(bad,5,3,1);});
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply actual prepared MCC maps and offline original map directories");admission();
    const auto authority=OriginalData::native_tu15();unsigned models=0,streams=0,vertices=0;
    for(const auto name:{"03b_newmombasa.map","04b_floodlab.map","05b_deltatowers.map",
            "06a_sentinelwalls.map","06b_floodzone.map","07b_forerunnership.map"}) {
        Cache mcc(std::filesystem::path(argv[1])/name,argv[1],authority);
        Cache original(std::filesystem::path(argv[2])/name,argv[2],authority);
        check(mcc.is_mcc() && !original.is_mcc(),"PRT comparison lost its named platform inputs");
        for(const auto &tag:mcc.select_tags({.group=tag_groups::render_model,.require_address=true})) {
            const auto a=mcc.load_source_prt_definition(tag.datum);if(!a)continue;
            const auto *reference=original.query_tag({.group=tag_groups::render_model,.name=tag.name,.require_address=true});
            check(reference,"MCC PRT model lacks its independently authored original identity");
            const auto b=original.load_source_prt_definition(reference->datum);
            check(b && !a->lods.empty() && a->order==b->order && a->clusters==b->clusters && a->pca_vectors==b->pca_vectors &&
                a->lods.size()==b->lods.size(),"PRT authored cluster/LOD contract differs");
            check(a->stream_descriptor[0]==std::byte{56} && a->stream_descriptor[1]==std::byte{12} &&
                b->stream_descriptor[0]==std::byte{57} && b->stream_descriptor[1]==std::byte{10},
                "Named actual PRT declaration profile differs from its original/MCC audit");
            for(unsigned lod=0;lod<6;++lod) {
                const auto &selected=a->lods[std::min<std::size_t>(a->lods.size()-1,5-lod)];
                for(const auto &section:selected.sections) {
                    const auto current=mcc.load_source_prt_vertex_stream(tag.datum,lod,section.section);
                    const auto expected=original.load_source_prt_vertex_stream(reference->datum,lod,section.section);
                    check(current && expected && current->size()==expected->size(),"PRT selected section extents differ");
                    for(unsigned n=0;n<current->size();++n) {
                        const auto &x=(*current)[n],&y=(*expected)[n];
                        check(std::bit_cast<std::uint32_t>(x.cluster)==std::bit_cast<std::uint32_t>(y.cluster),
                            "Delivered MCC PRT cluster differs from the actual original stream");
                        for(unsigned k=0;k<4;++k)check(std::bit_cast<std::uint32_t>(x.weights[k])==
                            std::bit_cast<std::uint32_t>(y.weights[k]),"Delivered MCC PRT weight differs from the actual original stream");
                    }
                    ++streams;vertices+=current->size();
                }
            }
            ++models;
        }
    }
    check(models==15 && streams>15 && vertices>=98719,"PRT comparison omitted its authored coverage");
    std::cout<<"Original/MCC PRT: "<<models<<" models, "<<streams<<" selected streams, "<<vertices<<" exact vertex comparisons\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
