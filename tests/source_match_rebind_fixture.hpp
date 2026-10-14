#pragma once
#include "halo2/source_match_authority.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/original_data.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &e){
    failed=true;std::cout<<"Rejected: "<<e.what()<<'\n';}check(failed,"Invalid local-view authority preparation accepted");}
struct Directory {
    std::filesystem::path path;
    Directory(){auto p=(std::filesystem::temp_directory_path()/"halo2-local-rebind-XXXXXX").string();
        check(::mkdtemp(p.data()),"Cannot create private rebind files");path=p;}
    ~Directory(){std::error_code ec;std::filesystem::remove_all(path,ec);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
constexpr unsigned player_count=8;
constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x4c}};
struct Owner {
    Cache cache;LevelContent content;Directory files;std::shared_ptr<Clock> clock=std::make_shared<Clock>();
    std::unique_ptr<MultiplayerObjectRuntime> world;std::unique_ptr<MultiplayerMatch> match;
    std::unique_ptr<SourceMatchAuthority> authority;FirstPersonBindingDefinition fp;
    Owner(const char *xbe,const char *map,std::span<const unsigned> locals,std::uint32_t time,
        std::span<const OnlineAdmission> roster={})
        :cache(map,{},OriginalData::load_xbe(xbe)),content(cache.load_content(0,true)){
        clock->time=time;world=std::make_unique<MultiplayerObjectRuntime>(xbe,cache,0,files.path,clock);
        UiVariant variant{"Slayer",0,{{0x59,5}}};auto rules=slayer_rules(variant);
        rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
        world->configure_session(source_game_variant(variant),1,30,rules.random_seed);
        match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules,MultiplayerKingServices{},
            [this]{return 100000u+(match?unsigned(match->simulation().ticks()/30):0);},world.get());
        match->configure_weapon_pickups();
        const auto borrowed=std::shared_ptr<const Cache>(&cache,[](const Cache*){});
        match->configure_grenades(borrowed,*cache.original_data()->grenade_constants);
        match->configure_melee_animation(borrowed,*cache.original_data()->melee_constants);
        for(unsigned id=0;id<player_count;++id){const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
            std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
            check(roster.empty()||(roster.size()==player_count&&roster[id].slot==id),"Source fixture admission slots differ");
            const auto name=roster.empty()?"Rebind seat "+std::to_string(id):roster[id].name;
            check(match->join(name,-1,native_selected_player_configuration(name,appearance,-1))==id,"Source roster admission failed");}
        fp=cache.load_first_person_binding(rules.weapon,2);
        match->configure_replicated_first_person(fp);match->configure_local_views(locals,fp);
        authority=std::make_unique<SourceMatchAuthority>(*match,*world);
    }
    void local_views(std::span<const unsigned> ids){match->configure_local_views(ids,fp);}
    void step(const std::array<Controls,16> &input){clock->time+=33;match->step(input);}
};
std::uint32_t word(const SourceRuntimeMemory &bank,std::uint32_t at,unsigned bytes=4){
    const auto value=bank.view(at,bytes);std::uint32_t v{};
    for(unsigned i=0;i<bytes;++i)v|=std::to_integer<std::uint32_t>(value[i])<<(8*i);return v;
}
SourceRawDatumPoolServices storage(const SourceRuntimeMemory &bank){return {.read=[&bank](auto at,auto n){return bank.view(at,n);}};}
bool bits(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool bits(Vec3 a,Vec3 b){return bits(a.x,b.x)&&bits(a.y,b.y)&&bits(a.z,b.z);}
void local_views(Owner &owner,std::span<const unsigned> locals){
    const auto &bank=owner.world->memory();const auto census=word(bank,0x4f54f0u),pool=word(bank,0x4f54f4u);
    for(unsigned view=0;view<4;++view){auto expected=null_datum;
        if(view<locals.size()&&!(owner.match->player(locals[view]).source_flags&2u))expected=owner.match->player(locals[view]).source_datum;
        check(word(bank,census+0xcu+view*4u)==expected,"Original local-view map did not rebind");
        const auto unit=expected==null_datum?null_datum:owner.match->player(locals[view]).source_current_object;
        check(owner.world->first_person_view(view).unit==unit,"Original first-person unit did not follow the new local player");
    }
    for(unsigned id=0;id<player_count;++id){const auto &p=owner.match->player(id);
        const auto row=source_raw_datum_lookup(pool,p.source_datum,storage(bank));const auto at=std::ranges::find(locals,id);
        const auto expected=at==locals.end()||(p.source_flags&2u)?std::int16_t(-1):std::int16_t(at-locals.begin());
        check(std::bit_cast<std::int16_t>(std::uint16_t(word(bank,row+0x28u,2)))==expected,"Original player local index did not rebind");
    }
}
void same_gameplay(Owner &a,Owner &b){
    const auto x=a.authority->checkpoint(binding),y=b.authority->checkpoint(binding);
    check(x.match.simulation.ticks==y.match.simulation.ticks&&x.match.simulation.random_state==y.match.simulation.random_state&&
        x.match.simulation.source_lifetimes==y.match.simulation.source_lifetimes&&x.match.simulation.scripts==y.match.simulation.scripts,
        "Rebinding changed native clocks/RNG/complete source lifetime history");
    check(x.match.retained_stats.players==y.match.retained_stats.players&&x.match.retained_stats.pvp==y.match.retained_stats.pvp&&
        x.match.retained_stats.teams==y.match.retained_stats.teams&&x.match.result_events.events==y.match.result_events.events,
        "Rebinding changed retained statistics or result events");
    const auto &am=a.world->memory(),&bm=b.world->memory();
    for(unsigned id=0;id<player_count;++id){const auto &p=a.match->player(id),&q=b.match->player(id);const auto &u=a.match->unit(id),&v=b.match->unit(id);
        check(p.source_datum==q.source_datum&&p.name==q.name&&p.source_flags==q.source_flags&&p.source_configuration==q.source_configuration&&
            p.source_profile_request==q.source_profile_request&&p.source_current_object==q.source_current_object&&p.source_previous_unit==q.source_previous_unit&&
            p.score==q.score&&p.kills==q.kills&&p.deaths==q.deaths&&p.respawn.countdown==q.respawn.countdown,
            "Rebinding changed the admitted roster/profile/current unit or scores");
        check(u.source_datum==v.source_datum&&u.source_player==v.source_player&&u.exists==v.exists&&u.vitality.dead==v.vitality.dead&&
            bits(u.body.position,v.body.position)&&bits(u.body.velocity,v.body.velocity)&&bits(u.vitality.body,v.vitality.body)&&bits(u.vitality.shield,v.vitality.shield)&&
            u.weapon.loaded==v.weapon.loaded&&u.weapon.reserve==v.weapon.reserve&&u.source_inventory.slots==v.source_inventory.slots&&
            u.source_inventory.current==v.source_inventory.current&&u.grenades==v.grenades&&u.grenade_action.held()==v.grenade_action.held()&&
            u.grenade_action.phase()==v.grenade_action.phase()&&u.grenade_action.counter()==v.grenade_action.counter(),
            "Rebinding changed live source movement/vitality/inventory/grenade continuation");
        const auto ar=source_raw_datum_lookup(word(am,0x4f54f4u),p.source_datum,storage(am));
        const auto br=source_raw_datum_lookup(word(bm,0x4f54f4u),q.source_datum,storage(bm));
        const auto ab=am.view(ar,0x21c),bb=bm.view(br,0x21c);
        for(unsigned n=0;n<ab.size();++n)if(n!=0x28&&n!=0x29&&ab[n]!=bb[n])
            throw FormatError("Original player byte differs outside local index: slot "+std::to_string(id)+" offset "+std::to_string(n));
    }
    const auto ao=a.world->live_objects(),bo=b.world->live_objects();check(ao.size()==bo.size(),"Rebinding changed live object census");
    for(unsigned i=0;i<ao.size();++i){check(ao[i].object==bo[i].object&&ao[i].definition==bo[i].definition&&ao[i].parent==bo[i].parent,
            "Rebinding changed a complete original object generation");
        const auto ah=source_raw_datum_lookup(a.world->source_object_pool(),ao[i].object,storage(am));
        const auto bh=source_raw_datum_lookup(b.world->source_object_pool(),bo[i].object,storage(bm));
        const auto an=word(am,ah+6u,2),bn=word(bm,bh+6u,2);check(an==bn,"Rebinding changed original object body size");
        const auto ab=am.view(word(am,ah+8u),an),bb=bm.view(word(bm,bh+8u),bn);
        for(unsigned n=0;n<an;++n)if(ab[n]!=bb[n])throw FormatError("Original live object body differs: handle "+std::to_string(ao[i].object)+" offset "+std::to_string(n));
    }
}
void alter_identity(SourceMatchAuthority::Checkpoint &saved,std::uint32_t address){
    for(auto &r:saved.world.world.bank.regions)if(address>=r.address&&std::uint64_t(address)<std::uint64_t(r.address)+r.bytes.size()){
        check(r.access==SourceRuntimeMemory::Access::ReadWrite,"Identity fixture selected immutable storage");
        r.bytes[address-r.address]^=std::byte{1};return;}
    throw FormatError("Identity fixture has no retained source player byte");
}
std::vector<SourceMatchAuthority::LocalPlayerAdmission> admissions(Owner &accepted,std::span<const unsigned> locals){
    std::vector<SourceMatchAuthority::LocalPlayerAdmission> result;const auto &bank=accepted.world->memory();
    for(auto slot:locals){SourceMatchAuthority::LocalPlayerAdmission entry{slot,accepted.match->player(slot).source_datum};
        const auto row=source_raw_datum_lookup(word(bank,0x4f54f4u),entry.player,storage(bank));
        bank.read(row+4u,std::as_writable_bytes(std::span(entry.identity)));result.push_back(entry);}
    return result;
}
}
