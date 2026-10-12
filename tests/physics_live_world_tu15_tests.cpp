#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shadow.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <bit>
#include <cstdlib>
#include <cmath>
int main(int argc,char**argv) {
    std::cout<<std::unitbuf;
    std::filesystem::path files;
    try {
        if(argc!=3 && argc!=4)throw std::runtime_error("Supply the owned English TU1.5 executable and Xbox map, optionally a diagnostic directory");
        halo2::Cache cache(argv[2]);
        auto name=(std::filesystem::temp_directory_path()/"halo2-live-tu15-XXXXXX").string();
        const auto p=::mkdtemp(name.data());if(!p)throw std::runtime_error("Native temporary directory failed");files=p;
        {
            halo2::PhysicsLiveWorldTu15 world(cache,argv[1],files);
            std::cout<<"Fresh TU1.5 same-bank renderer, arena and early initializer returned\n";
            for(auto entry:{0x37f110u,0x37f130u,0x37f150u,0x37f1e0u,0x37f240u,
                    0x37fc70u,0x37fc90u,0x37fcb0u})world.call(entry);
            world.call_actual(0xb8a00u);
            const auto dump=[&](const char*name) {
                std::ofstream output(std::filesystem::path(argv[3])/name,std::ios::binary);
                world.memory().visit_committed([&](auto p,auto bytes) {
                    const auto n=std::uint32_t(bytes.size());
                    output.write(reinterpret_cast<const char*>(&p),4);
                    output.write(reinterpret_cast<const char*>(&n),4);
                    output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
                });
                if(!output)throw std::runtime_error("Source bank diagnostic export failed");
            };
            if(argc==4) {
                std::filesystem::create_directories(argv[3]);dump("before.bin");
                std::ofstream stores(std::filesystem::path(argv[3])/"native-stores.bin",std::ios::binary);
                halo2::SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=0x600f0000u;
                a.fs_base=0x60200000u;a.eflags=2;a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
                try {
                    world.run(0x1c5ff0,a,[&](const auto&s) {
                        const auto n=std::uint32_t(s.written.size());
                        stores.write(reinterpret_cast<const char*>(&s.instruction),4);
                        stores.write(reinterpret_cast<const char*>(&s.address),4);
                        stores.write(reinterpret_cast<const char*>(&n),4);
                        stores.write(reinterpret_cast<const char*>(s.before.data()),n);
                        stores.write(reinterpret_cast<const char*>(s.written.data()),n);
                    });
                }catch(...) {dump("after.bin");throw;}
                dump("after.bin");
                std::ofstream exit(std::filesystem::path(argv[3])/"native-exit.bin",std::ios::binary);
                exit.write(reinterpret_cast<const char*>(a.gp.data()),sizeof(a.gp));
                exit.write(reinterpret_cast<const char*>(a.xmm.data()),sizeof(a.xmm));
                exit.write(reinterpret_cast<const char*>(&a.eflags),sizeof(a.eflags));
                exit.write(reinterpret_cast<const char*>(&a.fs_base),sizeof(a.fs_base));
                exit.write(reinterpret_cast<const char*>(&a.x87_depth),sizeof(a.x87_depth));
                exit.write(reinterpret_cast<const char*>(&a.x87_status),sizeof(a.x87_status));
                const auto control=a.floating_point.value();
                exit.write(reinterpret_cast<const char*>(&control.x87_control_word),sizeof(control.x87_control_word));
                exit.write(reinterpret_cast<const char*>(&control.mxcsr),sizeof(control.mxcsr));
                if(!exit)throw std::runtime_error("World exit ABI diagnostic export failed");
            }else world.call(0x1c5ff0);
            if(!world.world())throw std::runtime_error("Source world constructor returned without a world");
            std::cout<<"Fresh TU1.5 constructed world "<<std::hex<<world.world()<<'\n';
            world.call_actual(0xb8be0u);
            auto variant=halo2::source_game_variant_default(0);
            halo2::source_game_variant_set(variant,0x59,5);
            world.configure_session(variant,1,30,0x12345678u);
            if(std::getenv("HALO2_TU15_TEST_SCENE")) {
                // Exercise complete source births using original placement
                // bytes. This diagnostic broadens factory coverage; visible
                // acceptance still owns real automatic-placement admission.
                const auto word=[&](std::uint32_t p) {
                    const auto b=world.memory().view(p,4);std::uint32_t v=0;
                    for(unsigned k=0;k<4;++k)v|=std::to_integer<std::uint32_t>(b[k])<<(8*k);
                    return v;
                };
                const auto short_at=[&](std::uint32_t p) {
                    const auto b=world.memory().view(p,2);
                    return std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(b[0])|
                        (std::to_integer<unsigned>(b[1])<<8)));
                };
                const auto scenario=word(0x4ecc20u);
                unsigned born=0;
                for(unsigned type=0;type<13;++type) {
                    const auto description=word(0x474b90u+type*4u);
                    if(!description)continue;
                    const auto placements=short_at(description+0xau),palette=short_at(description+0xcu),stride=short_at(description+0xeu);
                    if(placements<0 || palette<0 || stride<=0)continue;
                    const auto count=word(scenario+placements),rows=word(scenario+placements+4u);
                    for(unsigned n=0;n<count;++n) {
                        const auto row=rows+n*stride;const auto choice=short_at(row);
                        if(choice<0 || (word(row+4u)&1u))continue;
                        if(unsigned(choice)>=word(scenario+palette))throw std::runtime_error("Source scene diagnostic palette ordinal escapes cache");
                        halo2::ScenarioObject placement;placement.type=int(type);placement.placement=int(n);
                        placement.definition=word(word(scenario+palette+4u)+unsigned(choice)*0x28u+4u);
                        if(placement.definition==0xffffffffu)continue;
                        std::uint32_t object;
                        try {object=world.create_scenario_object(placement);}
                        catch(const std::exception &e) {
                            throw std::runtime_error("Source scene type "+std::to_string(type)+
                                " placement "+std::to_string(n)+": "+e.what());
                        }
                        if(object!=0xffffffffu) {world.object_body(object);++born;}
                    }
                }
                std::cout<<"Fresh TU1.5 source scene factory completed "<<std::dec<<born<<" actual body births\n";
            }
            if(std::getenv("HALO2_TU15_TEST_NETGAME")) {
                const auto word=[&](std::uint32_t p) {
                    const auto bytes=world.memory().view(p,4);std::uint32_t v=0;
                    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
                    return v;
                };
                const auto scenario=word(0x4ecc20u),rows=word(scenario+0x124u);
                unsigned vehicles=0;
                for(unsigned row=0;row<word(scenario+0x120u);++row) {
                    const auto classification=std::to_integer<unsigned>(world.memory().view(rows+row*0x90u+0x14u,1)[0]);
                    if(classification<1 || classification>7)continue;
                    std::uint32_t object;
                    try {object=world.create_netgame_object(row);}
                    catch(const std::exception &e) {
                        throw std::runtime_error("Source netgame vehicle row "+std::to_string(row)+": "+e.what());
                    }
                    if(object==0xffffffffu)continue;
                    const auto body=world.object_body(object);
                    if(world.memory().view(body+0xaau,1)[0]!=std::byte{1})
                        throw std::runtime_error("Source vehicle row constructed another object type");
                    ++vehicles;
                }
                std::cout<<"Fresh TU1.5 netgame factory completed "<<std::dec<<vehicles<<" actual vehicle births\n";
                if(!vehicles)throw std::runtime_error("No source netgame vehicle bodies were constructed");
            }
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
                return t.group=="bipd" && t.name=="objects\\characters\\masterchief\\masterchief_mp";
            });
            if(tag==cache.tags().end() || cache.spawns().empty())throw std::runtime_error("Actual multiplayer biped or spawn missing");
            auto spawn_position=cache.spawns().front().position;
            const bool lift=std::getenv("HALO2_TU15_TEST_LIFT")!=nullptr;
            if(lift) {
                const auto content=cache.load_content(0,true);
                const auto definition=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){
                    return t.group=="bloc" && t.name=="scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube";
                });
                if(definition==cache.tags().end())throw std::runtime_error("Actual Lockout lift definition missing");
                const auto placement=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){return o.definition==definition->datum;});
                if(placement==content.objects.end())throw std::runtime_error("Actual Lockout lift placement missing");
                const auto object=cache.load_source_tag_metadata(placement->definition);
                const auto model=cache.load_source_tag_metadata(halo2::Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
                const auto physics=cache.load_source_tag_metadata(halo2::Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
                const halo2::Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
                if(p.u32(0x70)!=2 || p.u32(0x108)!=2)throw std::runtime_error("Actual Lockout lift phantom layout changed");
                const halo2::Bytes hull(physics.bytes_at(p.u32(0x74),0x100));
                halo2::Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
                const auto basis=halo2::authored_basis(placement->rotation);
                spawn_position=halo2::compose_transform(halo2::source_object_transform(placement->position,basis.forward,basis.up,placement->scale,false),local).position;
                std::cout<<"Actual authored lift entrance="<<spawn_position.x<<','<<spawn_position.y<<','<<spawn_position.z<<'\n';
            }
            const auto body=world.create_biped(tag->datum,spawn_position,{1,0,0});
            if(body==0xffffffffu)throw std::runtime_error("TU1.5 source biped birth was rejected");
            std::cout<<"Fresh TU1.5 biped "<<body<<'\n';
            halo2::SourceMachineIdentity machine{};bool machine_found=false;
            ifaddrs *interfaces=nullptr;
            if(::getifaddrs(&interfaces))throw std::runtime_error("Native machine interface inventory failed");
            for(auto *i=interfaces;i;i=i->ifa_next)if(i->ifa_addr && i->ifa_addr->sa_family==AF_PACKET &&
                (i->ifa_flags&IFF_UP) && !(i->ifa_flags&IFF_LOOPBACK)) {
                const auto *link=reinterpret_cast<const sockaddr_ll*>(i->ifa_addr);
                if(link->sll_halen==machine.size()) {
                    std::copy_n(link->sll_addr,machine.size(),machine.begin());machine_found=true;break;
                }
            }
            ::freeifaddrs(interfaces);
            if(!machine_found)throw std::runtime_error("Native machine identity unavailable");
            halo2::DatumPool players(world.memory(),world.player_pool());
            const auto player=players.allocate_at(0);
            if(player==halo2::DatumPool::none)throw std::runtime_error("Source player allocation failed");
            world.construct_player(player,0,halo2::source_default_player_configuration(),machine);
            world.bind_player(player,body);
            std::cout<<"Fresh TU1.5 player bound to its actual source biped\n";
            const auto equipment=world.initialize_player_equipment(player,body);
            const auto weapon_count=std::count_if(equipment.weapons.begin(),equipment.weapons.end(),[](auto p){return p!=0xffffffffu;});
            std::cout<<"Fresh TU1.5 initial equipment completed with "<<std::dec<<weapon_count<<" actual source weapons\n";
            std::array<std::byte,0x5c> command;halo2::initialize_source_neutral_command(command);
            const auto position=[&] {
                const auto p=world.object_body(body)+0x64u;
                std::array<float,3> v;
                for(unsigned n=0;n<3;++n) {
                    const auto b=world.memory().view(p+n*4u,4);std::uint32_t bits=0;
                    for(unsigned k=0;k<4;++k)bits|=std::to_integer<std::uint32_t>(b[k])<<(8*k);
                    v[n]=std::bit_cast<float>(bits);
                }
                return halo2::Vec3{v[0],v[1],v[2]};
            };
            const auto initial_position=position();
            float peak_z=initial_position.z,maximum_upward_velocity=0;
            const auto step=[&](unsigned tick) {
                try {world.deliver_command(player,body,command);world.step_objects();}
                catch(const std::exception &e) {
                    throw std::runtime_error("Source controller tick "+std::to_string(tick)+": "+e.what());
                }
                peak_z=std::max(peak_z,position().z);
                const auto velocity=world.memory().view(world.object_body(body)+0x90u,4);
                std::uint32_t velocity_bits=0;
                for(unsigned k=0;k<4;++k)velocity_bits|=std::to_integer<std::uint32_t>(velocity[k])<<(8*k);
                maximum_upward_velocity=std::max(maximum_upward_velocity,std::bit_cast<float>(velocity_bits));
                if((tick+1u)%60u==0u) {
                    const auto p=position();
                    std::cout<<"Source controller tick "<<tick+1u<<" position="<<p.x<<','<<p.y<<','<<p.z<<'\n';
                }
            };
            for(unsigned tick=0;tick<120;++tick) {
                step(tick);
            }
            std::cout<<"Fresh TU1.5 biped completed 120 same-bank command and world ticks\n";
            const auto neutral_position=position();
            const auto forward=std::bit_cast<std::uint32_t>(1.f);
            for(unsigned k=0;k<4;++k)command[0xc+k]=std::byte(forward>>(8*k));
            for(unsigned tick=0;tick<60;++tick) {
                step(tick+120u);
            }
            const auto moved_position=position();
            std::cout<<std::dec<<"Source body positions initial="<<initial_position.x<<','<<initial_position.y<<','<<initial_position.z
                <<" neutral="<<neutral_position.x<<','<<neutral_position.y<<','<<neutral_position.z
                <<" forward="<<moved_position.x<<','<<moved_position.y<<','<<moved_position.z<<'\n';
            for(const auto& p:{initial_position,neutral_position,moved_position})
                if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                    throw std::runtime_error("TU1.5 source controller produced a non-finite body position");
            if(moved_position.x==neutral_position.x && moved_position.y==neutral_position.y)
                throw std::runtime_error("TU1.5 bound controller completed forward ticks but produced no horizontal movement");
            if(lift) {
                std::cout<<"Source lift peak_z="<<peak_z<<" maximum_upward_velocity="<<maximum_upward_velocity<<'\n';
                if(!(peak_z>initial_position.z && maximum_upward_velocity>0))
                    throw std::runtime_error("TU1.5 authored lift produced no upward body travel");
            }
        }
        std::filesystem::remove_all(files);return 0;
    }catch(const std::exception&e) {
        if(!files.empty())std::filesystem::remove_all(files);
        std::cerr<<e.what()<<'\n';return 1;
    }
}
