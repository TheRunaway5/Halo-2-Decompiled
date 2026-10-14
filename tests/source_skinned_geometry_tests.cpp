#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
}
int main(int argc,char**argv)try{
    require(argc==3 || argc==4,"supply Xbox map, original TU1.5 declaration oracle and optional compression oracle");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    Cache cache(argv[1]);std::ifstream file(argv[2]);nlohmann::json original;file>>original;
    require(original.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("intercepted_instructions")==0,"skinned declarations lack unchanged TU1.5 authority");
    nlohmann::json compression;
    if(argc==4){std::ifstream input(argv[3]);input>>compression;
        require(compression.at("original_xbe_sha256")==original.at("original_xbe_sha256") &&
            compression.at("intercepted_instructions")==0,"compression oracle lacks TU1.5 authority");}
    unsigned sections=0,registers=0,compression_words=0;std::set<unsigned> classes,formats;
    for(const auto&tag:cache.tags())if(tag.group=="mode"){
        const auto metadata=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
        const auto count=root.u32(0x24),address=root.u32(0x28);
        for(unsigned section=0;section<count;++section){
            const Bytes definition(metadata.bytes_at(address+section*0x5c,0x5c));
            const auto classification=definition.u16(0);
            if(classification!=2 && classification!=3)continue;
            if(!definition.u16(4))continue;
            const auto geometry=load_menu_world_source_model_section(cache,tag.datum,section);
            if(argc==4){
                const auto&rows=compression.at("rows");
                const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto&r){return r.at("model")==tag.datum && r.at("section")==section;});
                require(row!=rows.end(),"authored skin section missing original compression bank");
                for(unsigned i=0;i<12;++i){require(std::bit_cast<std::uint32_t>(geometry.compression_rows()[i])==row->at("constants")[i],
                    "source skinned compression differs from unchanged TU1.5 arithmetic");++compression_words;}
            }
            require(geometry.model_geometry_class()==classification,"raw geometry lost its authored class");
            const auto type=std::to_integer<unsigned>(geometry.descriptors().at(0)[0]);
            const auto stride=std::to_integer<unsigned>(geometry.descriptors().at(0)[1]);
            const auto row=std::find_if(original.at("rows").begin(),original.at("rows").end(),[&](const auto&r){
                return r.at("stream_types")[0]==type;
            });
            require(row!=original.at("rows").end(),"authored skinned stream lacks an original declaration case");
            const auto semantics=row->at("semantics").get<std::vector<unsigned>>();
            for(unsigned reg=0;reg<semantics.size();++reg){
                const auto semantic=SourceVertexSemantic(semantics[reg]);
                const auto stream=std::find_if(geometry.streams().begin(),geometry.streams().end(),[&](const auto&s){return s.semantic==semantic;});
                require(stream!=geometry.streams().end(),"source skinned stream semantic missing");
                const auto expected=row->at("registers")[reg].get<std::array<unsigned,4>>();
                const auto&resource=geometry.resources().at(stream->resource);
                require(resource.type==56 && resource.channel==expected[0] && stream->byte_offset==expected[1],
                    "source skinned stream differs from original register channel/offset");
                require(stream->byte_stride==std::to_integer<unsigned>(geometry.descriptors().at(expected[0])[1]) &&
                    resource.bytes.size()==geometry.vertex_count()*stream->byte_stride,
                    "source skinned stream extent differs from actual descriptor");
                // Original NV2A format words from whole1C310, independently
                // retained in the fixture. Indices and weights are byte UNORM.
                switch(expected[2]){
                case 0x31:require(stream->format==MenuWorldSourceFormat::snorm16x3,"SHORT3 fetch changed");break;
                case 0x32:require(stream->format==MenuWorldSourceFormat::float32x3,"FLOAT3 fetch changed");break;
                case 0x21:require(stream->format==MenuWorldSourceFormat::snorm16x2,"SHORT2 fetch changed");break;
                case 0x16:require(stream->format==MenuWorldSourceFormat::packed_signed_11_11_10,"CMP fetch changed");break;
                case 0x14:case 0x24:case 0x34:case 0x44:
                    require(stream->format==std::array{MenuWorldSourceFormat::unorm8x1,MenuWorldSourceFormat::unorm8x2,
                        MenuWorldSourceFormat::unorm8x3,MenuWorldSourceFormat::unorm8x4}.at((expected[2]>>4)-1),
                        "byte UNORM influence width changed");
                    require(stream->byte_stride==stride,"skin influences escaped position stream");break;
                default:throw FormatError("unverified original skinned fetch format");
                }
                ++registers;
            }
            classes.insert(classification);formats.insert(type);++sections;
        }
    }
    require(classes==std::set<unsigned>{2,3} && sections>0,"no actual rigid-boned and skinned source sections exercised");
    std::cout<<"PASS "<<sections<<" authored sections / "<<registers<<" original register bindings / "<<compression_words<<" original compression words / "<<formats.size()
        <<" position layouts; palette production and GPU shader consumption are separate\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
