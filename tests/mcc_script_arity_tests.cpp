#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void put(std::span<std::byte> out,unsigned at,unsigned value,unsigned size=4) {
    for(unsigned n=0;n<size;++n)out[at+n]=std::byte(value>>(n*8));
}
void node(std::span<std::byte> out,unsigned index,unsigned opcode,unsigned type,unsigned flags,
          unsigned next,unsigned value) {
    const auto at=index*20;
    put(out,at,0x9000+index,2);put(out,at+2,opcode,2);put(out,at+4,type,2);
    put(out,at+6,flags,2);put(out,at+8,next);put(out,at+12,0);put(out,at+16,value);
}
}
int main(int argc,char** argv)try {
    require(argc==2,"Expected authenticated eight-argument declarations");
    std::ifstream file(argv[1]);nlohmann::json reference;file>>reference;
    require(reference.at("mcc_sha256")=="de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946",
        "Eight-argument reference is not the authenticated MCC image");
    unsigned cases=0;
    for(const auto& declaration:reference.at("declarations")) {
        const auto opcode=declaration.at("mcc_opcode").get<unsigned>();
        const auto types=declaration.at("arguments").get<std::vector<std::uint16_t>>();
        auto name=declaration.at("name").get<std::string>();name.push_back(0);
        const auto signature=mcc_script_builtin_signature(opcode);
        require(types.size()==8 && signature && signature->name==std::string_view(name.data(),name.size()-1) &&
            signature->declared_return_type==declaration.at("return_type") && !signature->xbox_opcode &&
            std::ranges::equal(signature->argument_types,types),"Native metadata lost an authenticated argument or declaration");
        for(const unsigned count:{7u,8u,9u}) {
            std::vector<std::byte> raw((count+2)*20);
            node(raw,0,opcode,signature->declared_return_type,8,0xffffffff,0x90010001);
            node(raw,1,2,2,9,0x90020002,0);
            for(unsigned n=0;n<count;++n)node(raw,n+2,types[n%8],types[n%8],9,
                n+1<count?((0x9000+n+3)<<16)|(n+3):0xffffffff,0x12340000+n);
            const auto before=raw;
            const auto projected=project_mcc_script_layout(raw,std::as_bytes(std::span(name)),{});
            require(raw==before && projected.nodes==before && projected.builtin_calls==1 &&
                projected.converted_calls==0 && projected.requirements.size()==1,
                "Eight-argument metadata changed authored records or admitted an unowned command");
            require(projected.requirements[0].need==(count==8?MccScriptLayoutNeed::NativeBuiltin:MccScriptLayoutNeed::ArgumentCount),
                "Eight-argument metadata confused arity validation with native effect ownership");
            bool rejected=false;try{projected.require_native_ready();}catch(const FormatError&){rejected=true;}
            require(rejected,"Test-only eight-argument declaration became runnable without an owner");++cases;
        }
    }
    require(cases==6,"Eight-argument fixture lost installed registry coverage");
    std::cout<<"PASS both original eight-argument declarations, full signatures and retained AST; seven/nine rejected and effects remain unowned\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
