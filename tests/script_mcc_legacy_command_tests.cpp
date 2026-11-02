#include "halo2/script_mcc_legacy_command.hpp"
#ifdef HALO2_MCC_COMMAND_SIMULATION
#include "halo2/simulation.hpp"
#endif
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
#ifdef HALO2_MCC_COMMAND_SIMULATION
void simulation(const ScriptProgram &program) {
    LevelContent content;content.scripts=program;
    content.player.datum=1;content.player.height=1;content.player.radius=.2f;
    content.player.crouch_height=.6f;content.player.camera_height=.8f;
    content.player.vitality={100,100,2,.1f,0};
    content.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},
        {{-100,-100,0},{100,100,0},{-100,100,0},0,0}};
    content.movement={2,2,2,10,1,1,1,5,2,1};
    Simulation world(content,0,Spawn{});world.start_level();
    require(world.scripts().running()==1,"Simulation did not start the authored command");
    world.step({});
    require(world.scripts().diagnostics().empty() && world.scripts().running()==0,
        "Simulation host did not complete the actual MCC command");
}
#endif
}
int main(int argc,char **argv)try {
    require(argc==2,"Supply the exact installed MCC blur reference");
    std::ifstream file(argv[1]);nlohmann::json reference;file>>reference;
    require(reference.at("schema")=="H2_INSTALLED_MCC_BLUR_CALLS_V1","MCC blur reference schema differs");
    require(!mcc_script_legacy_command("unrelated",{}),"Legacy command captured an unrelated host call");
    for(const auto args:{std::vector<ScriptValue>{},std::vector<ScriptValue>{{7,1,{}},{7,2,{}}}}) {
        bool bad=false;try{mcc_script_legacy_command("rasterizer_blur",args);}catch(const FormatError&){bad=true;}
        require(bad,"MCC legacy command admitted the wrong arity");
    }
    unsigned cases=0;
    for(const auto &map:reference.at("maps"))for(const auto &call:map.at("calls")) {
        ScriptProgram program;program.nodes.resize(map.at("node_count").get<unsigned>());
        program.strings.resize(map.at("strings_bytes").get<unsigned>());
        for(const auto &row:call.at("rows")) {
            const auto index=row.at("index").get<unsigned>();const auto hex=row.at("raw").get<std::string>();
            std::array<std::byte,20> raw{};require(hex.size()==40 && index<program.nodes.size(),"Installed blur row escapes its extent");
            for(unsigned n=0;n<20;++n)raw[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
            const Bytes bytes(raw);program.nodes[index]={bytes.u16(0),bytes.u16(2),bytes.u16(4),bytes.u16(6),
                bytes.u32(8),bytes.u32(12),bytes.u32(16)};
        }
        constexpr std::string_view command_name="rasterizer_blur";
        const auto text_offset=call.at("function_text_offset").get<unsigned>();
        require(text_offset<program.strings.size() && command_name.size()+1<=program.strings.size()-text_offset,
            "Installed blur name escapes its string bank");
        program.strings.replace(text_offset,command_name.size(),command_name);
        const auto root=call.at("datum").get<unsigned>();
        // Isolate this command as a standalone startup entry. Its authored
        // next sibling belongs to the surrounding campaign expression and
        // is outside this three-record reference. Callee/argument records,
        // salts, opcode, context, text offset and literal remain unchanged.
        program.nodes[root&65535u].next=null_datum;
        program.scripts.push_back({"installed_blur",0,4,root});
        unsigned host_calls=0;
        ScriptVm vm(program,[&](const auto &name,const auto &args) {
            require(name=="rasterizer_blur" && args.size()==1 && args[0].type==7 &&
                args[0].bits==call.at("argument").at(6).get<unsigned>(),"ScriptVm changed the exact authored SHORT argument");
            const auto command=mcc_script_legacy_command(name,args);
            require(command && *command==ScriptValue{},"MCC command did not return its declared VOID zero");
            ++host_calls;return *command;
        });
        vm.start_level();const auto checkpoint=vm.snapshot();
        vm.tick(0);require(vm.diagnostics().empty() && vm.running()==0 && host_calls==1,
            "ScriptVm did not finish the actual authored command");
        vm.restore(checkpoint);vm.tick(0);
        require(vm.diagnostics().empty() && vm.running()==0 && host_calls==2,"Restored ScriptVm command did not execute");
#ifdef HALO2_MCC_COMMAND_SIMULATION
        simulation(program);
#endif
        ++cases;
    }
    require(cases==7,"MCC blur reference no longer covers all seven installed calls");
    std::cout<<"PASS seven exact installed C++ ScriptVm blur commands, checkpoint retries, typed VOID completion and arity\n";
#ifdef HALO2_MCC_COMMAND_SIMULATION
    std::cout<<"PASS seven actual Simulation host startup/completion paths\n";
#endif
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
