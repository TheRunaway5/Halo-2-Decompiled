#include "source_match_rebind_fixture.hpp"
namespace {
void run(const char *xbe,const char *map){
    const std::array<unsigned,4> al{0,1,2,3},bl{4,5,6,7};Owner a(xbe,map,al,1000),b(xbe,map,bl,900000),workspace(xbe,map,al,500000);
    for(unsigned tick=0;tick<20;++tick){std::array<Controls,16> input{};for(unsigned id=0;id<player_count;++id)input[id].yaw=a.match->unit(id).yaw;
        input[0].fire=tick<12;input[0].reload=tick==12;input[0].grenade=tick>=17;a.step(input);}
    check(a.match->unit(0).weapon.loaded==54&&a.match->unit(0).grenade_action.held()!=null_datum,"Rebind fixture omitted active weapon/grenade state");
    // Declared fixture admission comes from the already accepted source owner.
    // Production must obtain these through authenticated session admission.
    const auto aa=admissions(a,al),ba=admissions(a,bl);
    auto prototype=b.authority->checkpoint(binding);const auto &bm=b.world->memory();
    const auto prototype_row=source_raw_datum_lookup(word(bm,0x4f54f4u),b.match->player(bl.front()).source_datum,storage(bm));
    // Explicitly different local prototype metadata, not a second physical PC
    // or an injected original constructor return. It must not replace admission.
    alter_identity(prototype,prototype_row+4u);
    auto prototype_stage=b.authority->prepare(prototype,binding);
    check(b.authority->commit(std::move(prototype_stage)),"Declared alternate prototype identity was rejected");
    const auto saved=a.authority->checkpoint(binding),before=b.authority->checkpoint(binding);
    rejects([&]{b.authority->prepare(saved,binding);});
    rejects([&]{b.authority->prepare_with_local_views(saved,binding,ba,*b.authority);});
    auto bad=saved;bad.match.players[bl.front()].source_datum^=0x10000u;
    rejects([&]{b.authority->prepare_with_local_views(bad,binding,ba,*workspace.authority);});
    bad=saved;const auto &am=a.world->memory();const auto row=source_raw_datum_lookup(word(am,0x4f54f4u),a.match->player(bl.front()).source_datum,storage(am));
    alter_identity(bad,row+4u);rejects([&]{b.authority->prepare_with_local_views(bad,binding,ba,*workspace.authority);});
    auto wrong_admission=ba;wrong_admission[0].identity[0]^=1;
    rejects([&]{b.authority->prepare_with_local_views(saved,binding,wrong_admission,*workspace.authority);});
    wrong_admission=ba;wrong_admission[0].slot=bl[1];
    rejects([&]{b.authority->prepare_with_local_views(saved,binding,wrong_admission,*workspace.authority);});
    {
        auto cancelled=b.authority->prepare_with_local_views(saved,binding,ba,*workspace.authority);
        rejects([&]{b.match->step({});});rejects([&]{b.world->step_objects();});rejects([&]{b.authority->checkpoint(binding);});
    }
    const auto after=b.authority->checkpoint(binding);
    check(before.world.world.bank==after.world.world.bank&&before.match.simulation.ticks==after.match.simulation.ticks&&
        before.match.configuration_binding==after.match.configuration_binding,"Cancelled rebind changed the live replacement world");
    local_views(b,bl);workspace.local_views(al);
    auto staged=b.authority->prepare_with_local_views(saved,binding,ba,*workspace.authority);
    check(!a.authority->commit(std::move(staged))&&b.authority->commit(std::move(staged)),"Foreign guard or new-local-view publication failed");
    local_views(a,al);local_views(b,bl);same_gameplay(a,b);
    for(unsigned tick=0;tick<40;++tick){std::array<Controls,16> input{};for(unsigned id=0;id<player_count;++id){input[id].yaw=a.match->unit(id).yaw;input[id].forward=id&1?0:.1f;}
        a.step(input);b.step(input);same_gameplay(a,b);local_views(b,bl);}
    staged=a.authority->prepare_with_local_views(b.authority->checkpoint(binding),binding,aa,*workspace.authority);
    check(a.authority->commit(std::move(staged)),"Repeated rebind to the original PC views failed");same_gameplay(a,b);local_views(a,al);
    check(a.match->depart(0),"Actual local player departure failed");
    const auto pending=a.authority->checkpoint(binding);
    rejects([&]{a.authority->prepare_with_local_views(pending,binding,aa,*workspace.authority);});
    staged=b.authority->prepare_with_local_views(pending,binding,ba,*workspace.authority);
    check(b.authority->commit(std::move(staged)),"Pending departure did not rebind to surviving local players");
    same_gameplay(a,b);local_views(a,al);local_views(b,bl);
    for(unsigned tick=0;tick<5;++tick){std::array<Controls,16> input{};for(unsigned id=0;id<player_count;++id)input[id].yaw=a.match->unit(id).yaw;
        a.step(input);b.step(input);same_gameplay(a,b);}
    check(a.match->unit(0).vitality.dead&&b.match->unit(0).vitality.dead&&!b.match->finished(),"Rebound pending departure did not retain a surviving match");
    std::cout<<"PASS actual TU1.5 eight-player Match rebind four local views 0/1/2/3 -> 4/5/6/7 -> 0/1/2/3, explicit admitted source identities independent of prototype identities, cancellation without publication, forty future matching complete live object bodies, original player bytes except local index, and pending local departure continued for five source ticks\n";
}
}
int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    run(argv[1],argv[2]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
