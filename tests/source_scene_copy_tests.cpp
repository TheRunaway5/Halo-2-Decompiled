#include "halo2/source_scene_copy.hpp"
#include "halo2/cache.hpp"
#include "halo2/vertex_program.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <limits>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const std::string &message) {if(!value)throw FormatError(message);}
Json read(const char *path) {
    std::ifstream file(path);check(bool(file),std::string("Missing original scene-copy evidence: ")+path);
    return Json::parse(file);
}
float number(const Json &word) {return std::bit_cast<float>(word.get<std::uint32_t>());}
void same(const ShaderVector &actual,const Json &expected,const std::string &label) {
    check(expected.size()==4,label+": incomplete original attribute");
    for(unsigned axis=0;axis<4;++axis)
        check(std::bit_cast<std::uint32_t>(actual[axis])==expected.at(axis),label+": word"+std::to_string(axis)+" differs");
}
template<class F>void rejected(F function,const char *label) {
    bool failed=false;try{function();}catch(const FormatError &){failed=true;}check(failed,label);
}
std::vector<std::byte> unhex(const std::string &hex) {
    check(hex.size()%2==0,"Truncated original program bytes");std::vector<std::byte> result;
    for(std::size_t i=0;i<hex.size();i+=2)result.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
void screen_program(const SourceSceneCopyFrame &frame,const VertexProgram &program) {
    check(program.count==7,"Original screen program coverage changed");
    for(const auto &vertex:frame.vertices) {
        VertexProgramState state;state.attributes[0]=vertex.position;
        for(unsigned tap=0;tap<4;++tap)state.attributes[tap+1]=vertex.texture_coordinates[tap];
        state.attributes[5]=vertex.corner;
        // Attribute6 is not supplied by34100. A declared retained value only
        // checks this independent MOV program; no source device default claim.
        state.attributes[6]={.125f,.25f,.5f,1};
        execute_vertex_program(program,state);
        check(state.outputs[0]==vertex.position,"Screen program altered source position");
        for(unsigned tap=0;tap<4;++tap)
            check(state.outputs[tap+9]==vertex.texture_coordinates[tap],"Screen program altered source sample coordinates");
        check(state.outputs[3]==vertex.corner && state.outputs[4]==state.attributes[6],"Screen MOV color routing differs");
    }
}
SourceSceneCopyContext captured_context(const Json &original) {
    const auto &before=original.at("result").at("before");
    SourceSceneCopyContext result{};
    result.camera_rectangle=before.at("camera_rectangle_shorts").get<std::array<std::int16_t,4>>();
    const auto &target=original.at("resources").back();
    result.destination_width=target.at("width");result.destination_height=target.at("height");
    result.near_plane=number(before.at("depth_range_bits").at(0));
    result.far_plane=number(before.at("depth_range_bits").at(1));
    return result;
}
unsigned captured(const Json &original) {
    check(original.at("instruction_patches").empty() && original.at("supplied_returns").empty(),"Original copy evidence substitutes source behavior");
    check(original.at("result").at("complete_original_return")==true,"Original copy did not complete");
    const auto frame=source_scene_copy_frame(captured_context(original));
    const auto bytes=unhex(original.at("screen_vertex_program").at("program_hex"));
    screen_program(frame,decode_vertex_program(bytes));
    unsigned callbacks=0,successful=0,programs=0;
    for(const auto &event:original.at("result").at("events")) {
        if(event.at("entry")==0x3fa3d0) {
            const auto program_bytes=unhex(event.at("program_payload_hex"));
            const Bytes words(program_bytes);
            for(unsigned word=0;word<60;++word)check(frame.program.words[word]==words.u32(word*4),"Scene-copy program differs from whole original352E0");
            ++programs;
        }
        if(event.at("entry")!=0x34100)continue;
        ++callbacks;check(event.at("completed")==true,"Original copy callback did not complete");
        const auto x=number(event.at("corner_bits").at(0)),y=number(event.at("corner_bits").at(1));
        const unsigned corner=x==0?(y==0?0:1):(y==0?3:2);
        const unsigned attribute=event.at("attribute");
        const bool accepted=(event.at("returned_eax").get<std::uint32_t>()&255)!=0;
        check(accepted==(attribute<6),"Original callback attribute admission changed");
        if(!accepted)continue;
        const auto &vertex=frame.vertices[corner];
        same(attribute==0?vertex.position:attribute==5?vertex.corner:vertex.texture_coordinates[attribute-1],
             event.at("output_after_bits"),"Captured corner"+std::to_string(corner)+" attribute"+std::to_string(attribute));
        ++successful;
    }
    check(callbacks==64 && successful==24 && programs==1,"Captured scene-copy coverage changed");
    return successful;
}
unsigned rectangles(const Json &fixture,const Json &original) {
    check(fixture.at("schema")=="original-scene-copy-geometry-v1","Unexpected original rectangle fixture");
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Rectangle evidence substitutes original behavior");
    check(fixture.at("inputs").size()==34 && fixture.at("versions").size()==1,"Original rectangle coverage changed");
    const auto &rows=fixture.at("versions").at("retail").at("results");
    check(rows.size()==fixture.at("inputs").size(),"Original rectangle results missing");
    const auto program_bytes=unhex(original.at("screen_vertex_program").at("program_hex"));
    const auto program=decode_vertex_program(program_bytes);
    unsigned attributes=0;
    for(std::size_t i=0;i<rows.size();++i) {
        const auto &input=fixture.at("inputs").at(i);const auto &row=rows.at(i);
        const std::string label=input.at("name");
        check(row.at("complete_original_return")==true && row.at("stop_before").is_null(),label+": original call did not complete");
        SourceSceneCopyContext context{input.at("camera_rectangle_shorts").get<std::array<std::int16_t,4>>(),
            row.at("destination_dimensions").at(0),row.at("destination_dimensions").at(1),
            number(row.at("near_bits")),number(row.at("far_bits"))};
        const auto frame=source_scene_copy_frame(context);screen_program(frame,program);
        for(unsigned word=0;word<60;++word)
            check(frame.program.words[word]==row.at("pixel_program_words").at(word),label+": pixel program differs");
        check(row.at("vertices").size()==4,label+": incomplete original quad");
        for(unsigned corner=0;corner<4;++corner) {
            const auto &vertex=frame.vertices[corner];const auto &expected=row.at("vertices").at(corner);
            for(unsigned axis=0;axis<2;++axis)
                check(std::bit_cast<std::uint32_t>(vertex.corner[axis])==expected.at("corner_bits").at(axis),label+": original quad order differs");
            check(expected.at("attributes").size()==16,label+": incomplete original callback results");
            for(unsigned attribute=0;attribute<16;++attribute) {
                const auto &value=expected.at("attributes").at(attribute);
                check(value.at("index")==attribute && value.at("returned")==bool(attribute<6),label+": original attribute admission changed");
                if(attribute>=6)continue;
                same(attribute==0?vertex.position:attribute==5?vertex.corner:vertex.texture_coordinates[attribute-1],
                     value.at("value_bits"),label+" corner"+std::to_string(corner)+" attribute"+std::to_string(attribute));
                ++attributes;
            }
        }
    }
    check(attributes==816,"Original rectangle emitted-attribute coverage changed");
    return attributes;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply unchanged whole original scene-copy report and rectangle fixture");
    const auto original=read(argv[1]);unsigned attributes=captured(original);
    attributes+=rectangles(read(argv[2]),original);
    SourceSceneCopyContext context=captured_context(original);
    const auto rounding=std::fegetround();
    for(const auto control:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::fesetround(control)==0,"Cannot set host rounding control");
        rejected([&]{(void)source_scene_copy_frame(context);},"Unadmitted rounding accepted");
    }
    check(std::fesetround(rounding)==0,"Cannot restore host rounding control");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86_FP)
    const auto mxcsr=_mm_getcsr();
    for(const auto control:{mxcsr|0x40u,mxcsr|0x8000u,mxcsr&~0x80u}) {
        _mm_setcsr(control);
        rejected([&]{(void)source_scene_copy_frame(context);},"Unadmitted gradual/mask control accepted");
    }
    _mm_setcsr(mxcsr);
#endif
    context.destination_width=0;
    rejected([&]{(void)source_scene_copy_frame(context);},"Zero destination dimension accepted");
    context=captured_context(original);context.far_plane=std::numeric_limits<float>::infinity();
    rejected([&]{(void)source_scene_copy_frame(context);},"Nonfinite scene-copy input accepted");
    context=captured_context(original);context.far_plane=context.near_plane;
    rejected([&]{(void)source_scene_copy_frame(context);},"Nonfinite scene-copy depth arithmetic accepted");
    std::cout<<attributes<<" complete original emitted attributes and all60 program words across35calls passed; screen MOV and finite host-control admission passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
