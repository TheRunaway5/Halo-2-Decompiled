#include "halo2/cache.hpp"
#include <iostream>
#include <thread>
#include <utility>
using namespace halo2;
static_assert(std::random_access_iterator<decltype(Material{}.passes.begin())>);
static_assert(std::random_access_iterator<decltype(std::declval<const Material&>().passes.begin())>);
namespace {
void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
const MaterialPass* pass(const Material& m,unsigned n){return &m.passes.at(n);}
}
int main()try {
    Material decoded;decoded.passes.resize(96);
    for(unsigned n=0;n<decoded.passes.size();++n){auto& p=decoded.passes[n];
        p.name="authored pass "+std::to_string(n);p.template_pass=n;p.implementation=n%4;
        p.constants[0][3]={float(n),2,3,4};p.program.words[5]=n;
        p.vertex_code.assign(1024,std::byte(n));p.pixel_remaps.push_back({std::uint16_t(n),4,21});}
    const Material authored=decoded;
    std::vector<Material> lights(24,authored);
    for(const auto& light:lights)for(unsigned n=0;n<96;++n)
        check(pass(light,n)==pass(authored,n),"Light fan-out duplicated an unchanged authored shader pass");
    auto& changed=lights[0].passes[17];changed.program.words[5]=12345;changed.constants[0][3][0]=-1;
    changed.vertex_code[0]=std::byte{0x55};changed.pixel_remaps[0].source=22;
    check(pass(lights[0],16)==pass(authored,16) && pass(lights[0],18)==pass(authored,18),
        "Editing one pass copied unrelated pass records");
    check(pass(authored,17)->program.words[5]==17 && pass(authored,17)->constants[0][3][0]==17 &&
        pass(authored,17)->vertex_code[0]==std::byte{17} && pass(authored,17)->pixel_remaps[0].source==21,
        "Editing a light changed an authored program, constant, bytecode or remap");
    const Material snapshot=lights[0];changed.program.words[5]=54321;
    check(pass(snapshot,17)->program.words[5]==12345,"A pre-copy pass reference changed the copied snapshot");
    check(pass(snapshot,16)==pass(authored,16),"Escaped selected-pass view duplicated an unrelated pass");
    Material writable=authored;auto it=writable.passes.begin()+3;auto& escaped=*it;
    Material moved=std::move(writable);const Material moved_snapshot=moved;escaped.name="edited";
    check(pass(moved_snapshot,3)->name=="authored pass 3","Moving a pass bank lost the mutable-reference boundary");
    check(moved.passes.end()-moved.passes.begin()==96,"Pass-bank iterator distance changed");
    Material constructed;constructed.passes.emplace_back();constructed.passes.front().name="one";
    constructed.passes.push_back(*pass(authored,9));constructed.passes.reserve(8);
    check(constructed.passes.size()==2 && pass(constructed,1)->template_pass==9,"Pass-bank construction changed values");
    std::array<std::thread,4> threads;std::array<bool,4> okay{};
    for(unsigned t=0;t<4;++t)threads[t]=std::thread([&,t]{okay[t]=true;for(unsigned n=0;n<50;++n){
        Material copy=authored;okay[t]=okay[t] && pass(copy,25)==pass(authored,25);
        copy.passes[t].constants[0][0][0]=float(n);okay[t]=okay[t] && pass(authored,t)->constants[0][0][0]==0;}});
    for(auto& thread:threads)thread.join();for(bool result:okay)check(result,"Concurrent pass copies lost independent values");
    std::cout<<"PASS light Material fan-out shares unchanged pass records and detaches selected edits; constants/bytecode/remaps and escaped references preserved\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
