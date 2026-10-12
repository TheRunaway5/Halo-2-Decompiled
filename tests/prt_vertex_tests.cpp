#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <map>
#include <sstream>

namespace {
using namespace halo2;
constexpr std::array<unsigned,7> outputs{0,3,4,9,10,11,12};
struct Fixture {
    std::array<VertexInstruction,136> instructions{};
    std::array<ShaderVector,16> attributes{};
    std::array<std::uint32_t,4> flags{};
    std::array<ShaderVector,192> constants{};
    std::array<ShaderVector,7> inputs{};
};
static_assert(sizeof(Fixture)==5632 && offsetof(Fixture,constants)==2448 && offsetof(Fixture,inputs)==5520);
struct Reference {
    unsigned event{},vertex{},element{};
    std::array<ShaderVector,7> outputs{};
    std::array<ShaderVector,7> gpu_outputs{};
    bool gpu_record{};
};
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
float word(std::istream &stream) {
    std::string text;check(bool(stream>>text) && text.size()==8,"truncated original PRT word");
    std::size_t used=0;
    const auto bits=std::stoul(text,&used,16);
    check(used==8 && bits<=0xfffffffful,"invalid original PRT word");
    return std::bit_cast<float>(std::uint32_t(bits));
}
void compare(const std::vector<ShaderVector> &actual,const std::vector<Reference> &expected,
             const char *producer) {
    check(actual.size()==expected.size()*16,"incorrect PRT output count");
    std::array<float,7> maximum{};
    std::array<ShaderVector,7> component_maximum{};
    std::array<std::array<std::uint32_t,4>,7> ulp_maximum{};
    float projected_error=0;
    std::array<unsigned,7> different{};
    std::string failure;
    const bool gpu=std::string(producer)=="GPU";
    for(unsigned i=0;i<expected.size();++i)for(unsigned o=0;o<outputs.size();++o)
        for(unsigned axis=0;axis<4;++axis) {
            float a=actual[i*16+outputs[o]][axis];
            if(gpu && outputs[o]==0 && axis<2)a=std::trunc(a*16.f)/16.f;
            if(gpu && (outputs[o]==3 || outputs[o]==4))a=std::clamp(a,0.f,1.f);
            const float b=(gpu?expected[i].gpu_outputs:expected[i].outputs)[o][axis];
            const float error=std::abs(a-b);
            check(std::isfinite(a) && std::isfinite(b),"nonfinite original PRT fixture/output");
            maximum[o]=std::max(maximum[o],error);
            component_maximum[o][axis]=std::max(component_maximum[o][axis],error);
            const auto aw=std::bit_cast<std::uint32_t>(a),bw=std::bit_cast<std::uint32_t>(b);
            if((aw>>31)==(bw>>31))ulp_maximum[o][axis]=std::max(ulp_maximum[o][axis],aw>bw?aw-bw:bw-aw);
            if(gpu && outputs[o]==9 && axis<2)
                projected_error=std::max(projected_error,std::abs(a/actual[i*16+9][3]-b/expected[i].gpu_outputs[o][3]));
            different[o]+=std::bit_cast<std::uint32_t>(a)!=std::bit_cast<std::uint32_t>(b);
            // CPU debugger values have RSQ precision differences only in
            // D1/T2. Actual GPU source uses OpDot, whereas the native shader
            // preserves ordered products/sums. Keep coefficient RGB, snapped
            // x/y and authored UVs exact; bound each measured dot/projection
            // result in its own units instead of a broad relative tolerance.
            bool accepted;
            if(!gpu) {
                accepted=outputs[o]==4 || outputs[o]==11?error<=2e-7f:aw==bw;
            } else if(outputs[o]==0) {
                accepted=axis<2?aw==bw:(aw>>31)==(bw>>31) &&
                    (aw>bw?aw-bw:bw-aw)<=(axis==2?2u:1u);
            } else if(outputs[o]==3) {
                accepted=axis<3?aw==bw:(aw>>31)==(bw>>31) && (aw>bw?aw-bw:bw-aw)<=1u;
            } else if(outputs[o]==9) {
                if(axis<2)accepted=std::abs(a/actual[i*16+9][3]-b/expected[i].gpu_outputs[o][3])<=4e-5f;
                else accepted=axis==2?aw==bw:(aw>>31)==(bw>>31) && (aw>bw?aw-bw:bw-aw)<=1u;
            } else if(outputs[o]==4 || outputs[o]==11)accepted=error<=2e-7f;
            else if(outputs[o]==12)accepted=error<=1.1e-7f;
            else accepted=aw==bw;
            if(!accepted && failure.empty())failure=std::string(producer)+" PRT mismatch at original event "+
                std::to_string(expected[i].event)+", vertex "+std::to_string(expected[i].vertex)+
                ", output "+std::to_string(outputs[o])+", component "+std::to_string(axis)+
                ": error "+std::to_string(error);
        }
    for(unsigned o=0;o<outputs.size();++o) {
        std::cout<<producer<<" original PRT o"<<outputs[o]<<": max "<<maximum[o]
                 <<", differing words "<<different[o]<<'\n';
        std::cout<<"  component maxima:";
        for(float value:component_maximum[o])std::cout<<' '<<value;
        std::cout<<'\n';
        std::cout<<"  same-sign ULP maxima:";
        for(auto value:ulp_maximum[o])std::cout<<' '<<value;
        std::cout<<'\n';
    }
    if(gpu)std::cout<<"GPU projected T0 pixel-coordinate max "<<projected_error<<'\n';
    check(failure.empty(),failure);
}
}
int main(int argc,char **argv) {
    try {
        check(argc==1 || (argc==2 && std::string(argv[1])=="--gpu"),"expected optional --gpu");
        const std::filesystem::path source=HALO2_SOURCE_DIR;
        std::ifstream binary(source/"tests/fixtures/retail_prt_rigid_local.vp",std::ios::binary);
        if(!std::filesystem::exists(source/"tests/fixtures/retail_prt_rigid_local.vp"))return 77;
        check(bool(binary),"original PRT program missing");
        const std::string bytes((std::istreambuf_iterator<char>(binary)),{});
        const auto program=decode_vertex_program({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});
        check(program.count==55 && program.relative_constants,"incorrect original compressed-rigid PRT program");
        std::ifstream file(source/"tests/fixtures/retail_prt_rigid_gpu.tsv");
        check(bool(file),"actual original PRT GPU fixture missing");
        std::map<unsigned,Fixture> banks;
        std::map<unsigned,unsigned> registers;
        std::vector<Fixture> fixtures;
        std::vector<Reference> references;
        std::string line;
        while(std::getline(file,line)) {
            if(line.empty() || line[0]=='#')continue;
            std::istringstream fields(line);char kind;unsigned event,index;
            check(bool(fields>>kind>>event>>index),"invalid original PRT record");
            if(kind=='C') {
                check(index==registers[event] && index<192,"missing/duplicate original PRT register");
                auto &bank=banks[event];
                for(auto &value:bank.constants[index])value=word(fields);
                ++registers[event];
            } else if(kind=='V') {
                check(registers[event]==192,"original PRT constant bank incomplete");
                auto fixture=banks.at(event);
                fixture.instructions=program.instructions;fixture.flags[3]=program.count;
                Reference reference;reference.event=event;reference.vertex=index;
                check(bool(fields>>reference.element),"missing original draw index");
                for(auto &input:fixture.inputs)for(auto &value:input)value=word(fields);
                for(auto &output:reference.outputs)for(auto &value:output)value=word(fields);
                fixtures.push_back(fixture);references.push_back(reference);
            } else if(kind=='G') {
                check(!references.empty(),"original PRT GPU record precedes input");
                auto &reference=references.back();unsigned element;
                check(bool(fields>>element) && reference.event==event && reference.vertex==index &&
                      reference.element==element && !reference.gpu_record,"mismatched original PRT GPU record");
                for(auto &output:reference.gpu_outputs)for(auto &value:output)value=word(fields);
                reference.gpu_record=true;
            } else throw std::runtime_error("unknown original PRT fixture record");
            std::string extra;check(!(fields>>extra),"trailing original PRT fields");
        }
        check(banks.size()==9 && fixtures.size()==118,"independent original PRT draw/sample coverage changed");
        check(std::all_of(references.begin(),references.end(),[](const auto &r){return r.gpu_record;}),
              "actual original GPU PRT outputs missing");
        std::vector<ShaderVector> cpu;
        auto direct_program=program;
        for(unsigned i=0;i<direct_program.count;++i)direct_program.instructions[i][3]&=~2u;
        unsigned indirect_color_cases=0;
        for(const auto &fixture:fixtures) {
            VertexProgramState state;state.constants=fixture.constants;
            for(unsigned i=0;i<7;++i)state.attributes[i]=fixture.inputs[i];
            execute_vertex_program(program,state);
            cpu.insert(cpu.end(),state.outputs.begin(),state.outputs.end());
            VertexProgramState direct;direct.constants=fixture.constants;
            for(unsigned i=0;i<7;++i)direct.attributes[i]=fixture.inputs[i];
            execute_vertex_program(direct_program,direct);
            for(unsigned channel=0;channel<3;++channel)
                if(std::abs(state.outputs[3][channel]-direct.outputs[3][channel])>.001f) {
                    ++indirect_color_cases;break;
                }
        }
        check(indirect_color_cases>50,"original PRT fixture does not detect lost relative coefficients");
        compare(cpu,references,"CPU");
        if(argc==2) {
            halo2_test::Compute<Fixture,16> compute(HALO2_PRT_VERTEX_TEST_SHADER);
            const auto gpu=compute.run(fixtures);
            VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(compute.physical,&properties);
            std::cout<<"PRT GPU reference device: "<<properties.deviceName<<'\n';
            compare(gpu,references,"GPU");
        }
        std::cout<<fixtures.size()<<" actual original PRT vertices across "<<banks.size()<<" draws passed\n";
        return 0;
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
