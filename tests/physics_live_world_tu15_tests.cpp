#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/model.hpp"
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
        if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
        halo2::Cache cache(argv[2]);
        auto name=(std::filesystem::temp_directory_path()/"halo2-live-tu15-XXXXXX").string();
        const auto p=::mkdtemp(name.data());if(!p)throw std::runtime_error("Native temporary directory failed");files=p;
        {
            halo2::PhysicsLiveWorldTu15 world(cache,argv[1],files);
            const auto startup_word=[&](std::uint32_t p) {
                const auto bytes=world.memory().view(p,4);std::uint32_t value{};
                for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
                return value;
            };
            const auto sound_arena=startup_word(0x51db00u),sound=startup_word(0x4f2c50u);
            if(!sound_arena || sound<sound_arena || sound+0x20cu>sound_arena+0x23000u ||
                world.memory().view(sound+0x78u,3)[0]!=std::byte{0} ||
                world.memory().view(sound+0x78u,3)[1]!=std::byte{1} ||
                world.memory().view(sound+0x78u,3)[2]!=std::byte{0})
                throw std::runtime_error("Source sound storage lost its actual arena or unactivated constructor flags");
            world.memory().view(sound_arena,0x23000u);
            for(const auto &[slot,stride]:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{
                    {0x4f2c4cu,0xbcu},{0x52d668u,0x1cu},{0x50ebe8u,0xd4u},
                    {0x52d670u,0x14u},{0x50ebecu,0x6cu}}}) {
                const auto pool=startup_word(slot);
                if(!pool || startup_word(pool+0x24u)!=stride ||
                    world.memory().view(pool+0x29u,1)[0]!=std::byte{1})
                    throw std::runtime_error("Source sound storage lacks a complete original active datum pool");
            }
            std::cout<<"Fresh TU1.5 same-bank renderer, arena and early initializer returned\n";
            const auto sound_bits=startup_word(0x52be90u),sound_arena_bits=startup_word(0x52be94u);
            if(!sound_bits || sound_bits!=startup_word(0x52be98u) || !sound_arena_bits ||
                sound_bits==sound_arena_bits || sound_arena_bits<startup_word(0x4f2950u) ||
                sound_arena_bits+0x1880u>startup_word(0x4f2950u)+startup_word(0x4f2954u))
                throw std::runtime_error("Source sound bit bank lost its original arena or allocated backing");
            for(const auto value:world.memory().view(sound_bits,0x1880u))
                if(value!=std::byte{0})throw std::runtime_error("Source sound bit bank lacks zero-filled native pages");
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
            if(world.world().is_null())throw std::runtime_error("Source world constructor returned without a world");
            std::cout<<"Fresh TU1.5 constructed world "<<std::hex<<world.world().value<<'\n';
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
            if(std::getenv("HALO2_TU15_TEST_NETGAME") || std::getenv("HALO2_TU15_TEST_PICKUPS")) {
                const auto word=[&](std::uint32_t p) {
                    const auto bytes=world.memory().view(p,4);std::uint32_t v=0;
                    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
                    return v;
                };
                const auto scenario=word(0x4ecc20u),rows=word(scenario+0x124u);
                unsigned vehicles=0,pickups=0;
                for(unsigned row=0;row<word(scenario+0x120u);++row) {
                    const auto classification=std::to_integer<unsigned>(world.memory().view(rows+row*0x90u+0x14u,1)[0]);
                    const bool pickup=classification==0 && std::getenv("HALO2_TU15_TEST_PICKUPS");
                    if(!pickup && (classification<1 || classification>7 || !std::getenv("HALO2_TU15_TEST_NETGAME")))continue;
                    std::uint32_t object;
                    try {object=world.create_netgame_object(row);}
                    catch(const std::exception &e) {
                        throw std::runtime_error("Source netgame row "+std::to_string(row)+": "+e.what());
                    }
                    if(object==0xffffffffu)continue;
                    const auto body=world.object_body(object);
                    if(!pickup && world.memory().view(body+0xaau,1)[0]!=std::byte{1})
                        throw std::runtime_error("Source vehicle row constructed another object type");
                    if(pickup)++pickups;else ++vehicles;
                }
                std::cout<<"Fresh TU1.5 netgame factory completed "<<std::dec<<vehicles<<" actual vehicle births\n";
                std::cout<<"Fresh TU1.5 netgame factory completed "<<pickups<<" actual pickup births\n";
                if(std::getenv("HALO2_TU15_TEST_NETGAME") && !vehicles)throw std::runtime_error("No source netgame vehicle bodies were constructed");
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
            std::vector<std::pair<std::uint32_t,std::uint32_t>> peers;
            std::array<std::byte,0x5c> second_command;halo2::initialize_source_neutral_command(second_command);
            unsigned participants=std::getenv("HALO2_TU15_TEST_TWO_PLAYERS")?2u:1u;
            if(const auto count=std::getenv("HALO2_TU15_TEST_PLAYER_COUNT"))participants=std::stoul(count);
            if(!participants || participants>unsigned(players.capacity()))
                throw std::runtime_error("Source multiplayer participant probe exceeds its actual player pool");
            for(unsigned slot=1;slot<participants;++slot) {
                const auto position=std::getenv("HALO2_TU15_TEST_PLAYER_COUNT")?
                    cache.spawns()[slot%cache.spawns().size()].position:spawn_position;
                const auto unit=world.create_biped(tag->datum,position,{1,0,0});
                const auto identity=players.allocate_at(std::int32_t(slot));
                if(unit==0xffffffffu || identity==halo2::DatumPool::none)
                    throw std::runtime_error("Additional actual source multiplayer body/player birth failed");
                world.construct_player(identity,slot,halo2::source_default_player_configuration(),machine);
                world.bind_player(identity,unit);world.initialize_player_equipment(identity,unit);
                peers.emplace_back(identity,unit);
                std::cout<<"Actual source player slot "<<slot<<" and body bound at the declared multiplayer location\n";
            }
            if(std::getenv("HALO2_TU15_TEST_DEATH")) {
                if(std::getenv("HALO2_TU15_TEST_DEATH_TICKS")) {
                    std::array<std::byte,0x5c> neutral;halo2::initialize_source_neutral_command(neutral);
                    for(unsigned tick=0;tick<10;++tick) {
                        if(tick==8) {
                            const auto forward=std::bit_cast<std::uint32_t>(1.f);
                            for(unsigned n=0;n<4;++n)neutral[0xcu+n]=std::byte(forward>>(8*n));
                        }
                        world.deliver_command(player,body,neutral);world.step_objects();
                    }
                    std::cout<<"Fresh TU1.5 death victim completed ten alive world ticks\n";
                }
                constexpr std::uint32_t damage=0x60040000u;
                const std::array record_args{body,damage};world.call_actual(0xd8c20u,record_args);
                const std::array death_args{body,damage,1u,0u};world.call_actual(0xd8d50u,death_args);
                const bool forced_animation=std::getenv("HALO2_TU15_TEST_LANDING_DEATH")!=nullptr;
                const std::array cleanup_args{body,0xffffffffu,0u,forced_animation?1u:0u,0u};world.call_actual(0xccf40u,cleanup_args);
                const auto read=[&](auto p) {
                    const auto bytes=world.memory().view(p,4);std::uint32_t value{};
                    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
                    return value;
                };
                const auto player_row=read(world.player_pool()+0x44u)+(player&65535u)*read(world.player_pool()+0x24u);
                const auto object=world.object_body(body);
                if(read(player_row+0x2cu)!=0xffffffffu || read(player_row+0x30u)!=body ||
                    !(std::to_integer<unsigned>(world.memory().view(object+0x10au,1)[0])&4u) ||
                    world.memory().view(object+0x3dcu,1)[0]!=std::byte{3})
                    throw std::runtime_error("Original fatal cleanup lost dead state, physical transition or player detachment");
                std::cout<<"Fresh TU1.5 ordinary fatal cleanup returned\n";
                if(forced_animation) {
                    if(!(std::to_integer<unsigned>(world.memory().view(object+0x349u,1)[0])&4u))
                        throw std::runtime_error("Explicit original forwarded flag did not issue the authored landing_dead request");
                    const auto manager=object+std::uint16_t(read(object+0x12au));
                    const auto packed=read(manager+4u);
                    if(read(manager+8u)!=0x0c000043u || std::uint16_t(packed)!=0xffffu)
                        throw std::runtime_error("Explicit source death request did not install its local authored channel");
                    const auto clip=cache.load_animation(read(manager),packed>>16);
                    if(cache.string_id(read(manager+8u))!="landing_dead" || clip.frame_count!=42)
                        throw std::runtime_error("Source-installed death channel: "+clip.name+", "+
                            std::to_string(clip.frame_count)+" frames");
                    std::cout<<"PASS explicit source flag installed authored landing_dead, "<<clip.frame_count
                        <<" frames; its natural producer is not supplied by this probe\n";
                }
                if(std::getenv("HALO2_TU15_TEST_DEATH_TICKS")) {
                    const auto initial_component=read(object+0xb4u),initial_pool=read(0x52be5cu);
                    const auto initial_record=read(initial_pool+0x44u)+(initial_component&65535u)*read(initial_pool+0x24u);
                    std::cout<<"Before death ticks bodies="<<read(initial_record+0x74u)
                        <<" constraints="<<read(initial_record+0x80u)<<'\n';
                    const auto node_bytes=read(object+0x114u)&65535u;
                    const auto node_offset=read(object+0x116u)&65535u;
                    const auto initial=world.memory().view(object+node_offset,node_bytes);
                    const std::vector<std::byte> before(initial.begin(),initial.end());
                    bool ragdoll{},moved{};
                    for(unsigned tick=0;tick<90;++tick) {
                        world.step_objects();
                        const auto component=read(object+0xb4u),pool=read(0x52be5cu);
                        if(component==0xffffffffu)throw std::runtime_error("Death lost its source physical component");
                        const auto record=read(pool+0x44u)+(component&65535u)*read(pool+0x24u);
                        const auto bodies=read(record+0x74u),constraints=read(record+0x80u);
                        ragdoll|=bodies==10 && constraints==9;
                        const auto now=world.memory().view(object+node_offset,node_bytes);
                        moved|=!std::ranges::equal(before,now);
                        for(unsigned n=0;n<node_bytes;n+=4)
                            if(!std::isfinite(std::bit_cast<float>(read(object+node_offset+n))))
                                throw std::runtime_error("Death world produced a non-finite WORLD node");
                        std::cout<<"Death world tick "<<tick+1<<" bodies="<<bodies<<" constraints="<<constraints<<'\n';
                    }
                    if(!ragdoll || !moved)throw std::runtime_error("Continuous death ticks did not produce the authored ragdoll and changed WORLD nodes");
                    std::cout<<"After death ticks current="<<std::hex<<read(player_row+0x2cu)
                        <<" previous="<<read(player_row+0x30u)<<" countdown="<<read(player_row+0x164u)<<std::dec<<'\n';
                    std::cout<<"PASS fresh TU1.5 death entered ten-body/nine-constraint ragdoll with moving source WORLD nodes\n";
                }
                // This mode is the confirmed-fatal admission check. Full
                // world ticks, corpse constraints and cameras are independent.
                std::filesystem::remove_all(files);return 0;
            }
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
                try {
                    world.deliver_command(player,body,command);
                    for(const auto &[identity,unit]:peers)world.deliver_command(identity,unit,second_command);
                    world.step_objects();
                }
                catch(const std::exception &e) {
                    if(const auto path=std::getenv("HALO2_TU15_FAILURE_BANK")) {
                        std::ofstream output(path,std::ios::binary);
                        world.memory().visit_committed([&](auto p,auto bytes) {
                            const auto n=std::uint32_t(bytes.size());
                            output.write(reinterpret_cast<const char*>(&p),4);
                            output.write(reinterpret_cast<const char*>(&n),4);
                            output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
                        });
                        if(!output)throw std::runtime_error("Source failure-bank export failed");
                    }
                    throw std::runtime_error("Source controller tick "+std::to_string(tick)+": "+e.what());
                }
                peak_z=std::max(peak_z,position().z);
                const auto velocity=world.memory().view(world.object_body(body)+0x90u,4);
                std::uint32_t velocity_bits=0;
                for(unsigned k=0;k<4;++k)velocity_bits|=std::to_integer<std::uint32_t>(velocity[k])<<(8*k);
                maximum_upward_velocity=std::max(maximum_upward_velocity,std::bit_cast<float>(velocity_bits));
                if(tick==0u || (tick+1u)%15u==0u) {
                    const auto p=position();
                    std::cout<<"Source controller tick "<<tick+1u<<" position="<<p.x<<','<<p.y<<','<<p.z
                        <<" maximum_upward_velocity="<<maximum_upward_velocity<<'\n';
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
