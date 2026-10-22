#include "halo2/source_live_transparent_groups_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <cstring>
#include <cmath>
namespace halo2 {
namespace {
std::vector<std::array<std::uint32_t,6>> ordered_group_layer(PhysicsLiveWorldTu15 &world,
    std::uint32_t layer,std::uint32_t first,std::uint32_t end) {
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto put=[&](std::uint32_t p,std::uint32_t v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};
    if(word(0x4ce274u)!=first||word(0x4ce26cu)!=end)
        throw FormatError("TU1.5 group sort lost its actual command range");
    // Same authenticated3BC20→3BAA0 native caller frame used by the shadow
    // adapter. The group callback selects ordinary bank0/category0. Preserve
    // descriptor residue; do not create a zero-filled expected output bank.
    auto caller=world.prepare_source_call(std::array{0u});const auto stack=caller.gp[4];
    if((stack&3u)||stack<0xf01cu||stack>0xfffffff7u)
        throw FormatError("TU1.5 group sort has no complete original caller frame");
    (void)memory.view(stack-0xf01cu,0xf024u);
    const auto output=stack-0xf000u,callee=stack-0xf01cu;
    const auto returned=word(stack);put(stack-0xf008u,caller.gp[5]);
    caller.gp[0]=output;caller.gp[3]=layer;caller.gp[5]=0;caller.gp[7]=0x4ce268u;caller.gp[4]=callee;
    put(callee,returned);const std::array args{0x4ce268u,0u,layer,output};
    for(unsigned n=0;n<args.size();++n)put(callee+4u+n*4u,args[n]);
    world.run_actual(0x3baa0u,caller);
    const auto count=caller.gp[0];
    if(caller.gp[4]!=stack-0xf008u||count>2560u||count>end-first)
        throw FormatError("TU1.5 group sort escaped its source frame/command range");
    std::vector<std::array<std::uint32_t,6>> result(count);
    if(count)std::memcpy(result.data(),memory.view(output,std::size_t(count)*24u).data(),std::size_t(count)*24u);
    for(const auto &d:result)if((d[0]&65535u)<first||(d[0]&65535u)>=end)
        throw FormatError("TU1.5 group descriptor lost its actual command ordinal");
    return result;
}
}
std::vector<SourceTransparentCommand> source_live_transparent_queue_tu15(PhysicsLiveWorldTu15 &world) {
    const auto &memory=world.memory();
    const auto count=Bytes(memory.view(0x4c2aa0u,4)).u32(0);
    const auto pointer=Bytes(memory.view(0x4c2aa8u,4)).u32(0);
    if(count>1024 || (count&&!pointer))throw FormatError("TU1.5 transparent snapshot lost its actual complete queue");
    std::vector<SourceTransparentCommand> result(count);
    if(count)std::memcpy(result.data(),memory.view(pointer,count*sizeof(SourceTransparentCommand)).data(),count*sizeof(SourceTransparentCommand));
    return result;
}
std::vector<SourceLiveTransparentGroupTu15> source_live_transparent_groups_tu15(PhysicsLiveWorldTu15 &world) {
    auto &memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto put=[&](std::uint32_t p,std::uint32_t v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};
    const auto count=word(0x4c2aa0u),pointer=word(0x4c2aa8u);
    if(count>1024 || (count&&!pointer))throw FormatError("TU1.5 group collector lost its actual transparent queue");
    std::vector<SourceLiveTransparentGroupTu15> groups;
    // Snapshot queue requests before the collector can publish any commands.
    for(unsigned n=0;n<count;++n) {
        SourceTransparentCommand c;
        std::memcpy(&c,memory.view(pointer+n*sizeof(c),sizeof(c)).data(),sizeof(c));
        if(c.callback!=0x423b0u)continue;
        SourceLiveTransparentGroupTu15 g;g.queue_command=c;
        std::memcpy(&g.section,c.payload.data(),4);std::memcpy(&g.flags,c.payload.data()+4,4);
        const auto highwater=word(0x4ce1ccu);
        if(highwater>32 || g.section>=31 || g.section>=highwater || word(0x4cdd4cu+g.section*36u+32u)!=3)
            throw FormatError("TU1.5 group request lost its actual kind3 sort section");
        groups.push_back(std::move(g));
    }
    if(groups.empty())return groups;
    const auto snapshot_geometry=[&] {
        const auto context=word(0x4cd3a0u),prefix=word(0x4cd39cu),unit_count=word(0x4cd398u);
        if(!context || context>0xfffff59fu || prefix!=unit_count || unit_count>850u)
            throw FormatError("TU1.5 group geometry lacks its completed native main prefix/context");
        auto geometry=std::make_shared<SourceLiveTransparentGroupGeometryTu15>();
        geometry->units.resize(prefix);
        if(prefix)std::memcpy(geometry->units.data(),memory.view(0x4c6958u,std::size_t(prefix)*32u).data(),std::size_t(prefix)*32u);
        geometry->arena_address=context+0x25cu;
        geometry->arena.used=Bytes(memory.view(context+0xa5cu,2)).u16(0);
        if(geometry->arena.used>geometry->arena.words.size())
            throw FormatError("TU1.5 group geometry mask arena exceeds its source capacity");
        std::memcpy(geometry->arena.words.data(),memory.view(geometry->arena_address,sizeof(geometry->arena.words)).data(),sizeof(geometry->arena.words));
        return geometry;
    };
    for(auto &g:groups) {
        std::array<std::uint32_t,6> before;
        std::memcpy(before.data(),memory.view(0x4ce268u,sizeof(before)).data(),sizeof(before));
        if(!before[0]||before[1]>before[0]||before[3]||!before[5])
            throw FormatError("TU1.5 group requires its constructed ordinary command bank: capacity="+
                std::to_string(before[0])+" count="+std::to_string(before[1])+" start="+std::to_string(before[3]));
        (void)memory.view(before[5],std::size_t(before[0])*24u);
        // Exact423B0 pre-collection stores. Header+0C is the original range
        // start and+10 retains the previous layer mask. Count/data survive.
        put(0x4ce278u,before[2]);put(0x4ce270u,0);put(0x4ce274u,before[1]);
        const std::array arguments{0u,g.flags|0xffffffcfu,g.section,0xffffffffu,0u,0xffffffffu,0u,1u};
        auto caller=world.prepare_source_call(arguments);const auto stack=caller.gp[4];
        world.run_actual(0x44c80u,caller);
        if(caller.gp[4]!=stack+36u || word(0x4ce268u)!=before[0] || word(0x4ce27cu)!=before[5] ||
           word(0x4ce274u)!=before[1] || word(0x4ce278u)!=before[2])
            throw FormatError("TU1.5 group collection changed its actual caller/bank identity");
        g.first_command=before[1];g.end_command=word(0x4ce26cu);g.layers=word(0x4ce270u);
        if(g.end_command<g.first_command||g.end_command>before[0])
            throw FormatError("TU1.5 group collection escaped its original command capacity");
        g.commands.resize(g.end_command-g.first_command);
        if(!g.commands.empty())std::memcpy(g.commands.data(),memory.view(before[5]+g.first_command*24u,g.commands.size()*24u).data(),g.commands.size()*24u);
        std::memcpy(&g.shader_clock,memory.view(0x4920c0u,8).data(),8);
        if(!std::isfinite(g.shader_clock))throw FormatError("TU1.5 group lost its published shader clock");
        for(const auto layer:std::array{1u,3u,8u,7u,2u,16u,15u,21u}) {
            if(!(g.layers&(1u<<layer)))continue;
            if(layer==8u&&memory.view(0x4c6842u,1)[0]==std::byte{})continue;
            if(layer==2u&&memory.view(0x4c6843u,1)[0]==std::byte{})continue;
            g.ordered_layers.push_back({layer,ordered_group_layer(world,layer,g.first_command,g.end_command)});
        }
        g.geometry=snapshot_geometry();
        // Collection-only caller restoration. A failing original callee keeps
        // its actual partial writes and throws; no stale group is rendered.
        put(0x4ce274u,0);put(0x4ce270u,before[2]);
    }
    return groups;
}
}
