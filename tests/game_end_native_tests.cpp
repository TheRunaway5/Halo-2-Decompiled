#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/original_data.hpp"
#include "halo2/hud.hpp"
#include "halo2/audio.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
void compare(const MultiplayerState& source,const MultiplayerState& peer) {
  check(peer.tick==source.tick && peer.finished==source.finished && peer.results_finalized==source.results_finalized &&
      peer.game_over_countdown==source.game_over_countdown && peer.game_over_fade_started==source.game_over_fade_started &&
      peer.score_limit==source.score_limit && peer.team_total_scores==source.team_total_scores &&
      peer.team_wins==source.team_wins,"UDP final match records differ");
  for(unsigned i=0;i<16;++i) {
    const auto& a=source.players[i];const auto& b=peer.players[i];
    check(a.connected==b.connected && (!a.connected || (a.source_player==b.source_player &&
        a.score==b.score && a.total_score==b.total_score && a.wins==b.wins &&
        a.kills==b.kills && a.deaths==b.deaths && a.assists==b.assists && a.suicides==b.suicides &&
        a.source_respawn_countdown==b.source_respawn_countdown)),"UDP final player records differ");
  }
}
void compare_results(const MultiplayerState& ended,const MultiplayerState& current) {
  check(ended.finished && current.finished && current.results_finalized && ended.winner==current.winner &&
      ended.winner_team==current.winner_team && ended.clock_seconds==current.clock_seconds && ended.team_scores==current.team_scores &&
      ended.team_total_scores==current.team_total_scores && ended.team_wins==current.team_wins,
      "Ended team/winner bank changed during postgame world updates");
  for(unsigned i=0;i<16;++i)if(ended.players[i].connected) {
    const auto& a=ended.players[i];const auto& b=current.players[i];
    check(a.score==b.score && a.total_score==b.total_score && a.wins==b.wins &&
        a.kills==b.kills && a.deaths==b.deaths && a.assists==b.assists && a.suicides==b.suicides &&
        a.source_respawn_countdown==b.source_respawn_countdown,
        "Final player bank or inactive respawn timer changed during postgame updates");
  }
}
void verify_recorded(const MultiplayerMatch& match) {
  const auto* retained=match.recorded_statistics();
  check(retained && match.result_recording().enabled && match.result_recording().frozen,
      "Ended native match did not retain/freeze original statistics");
  const auto state=match.state();const auto& bank=match.source_match_statistics();
  const auto& round=match.source_round_lifecycle();
  check(round.phase==2 && round.status==2 && round.index==0 && round.countdown==150 &&
      !(bank.header&255),"Native final world pass did not apply original round phase/validity/countdown");
  for(unsigned i=0;i<16;++i)if(state.players[i].connected) {
    const auto& p=state.players[i];
    check(bank.players[i].fields[2]==p.kills && bank.players[i].fields[3]==p.deaths &&
        bank.players[i].fields[4]==p.suicides && bank.players[i].fields[6]==p.assists,
        "Native combat counters did not enter original match stat fields");
    check(retained->players[i][7]==p.kills && retained->players[i][9]==p.deaths &&
        retained->players[i][11]==p.suicides && retained->players[i][8]==p.assists &&
        retained->players[i][6]==p.wins,"Native combat/win events did not publish retained source stats");
  }
  if(state.team_mode)for(unsigned i=0;i<8;++i)
    check(retained->teams[i][7]==bank.teams[i].fields[2] &&
        retained->teams[i][9]==bank.teams[i].fields[3] &&
        retained->teams[i][11]==bank.teams[i].fields[4] &&
        retained->teams[i][8]==bank.teams[i].fields[6] &&
        retained->teams[i][6]==bank.teams[i].fields[7],"Native team events did not publish retained source stats");
}
void verify_board(const Cache& cache,const std::filesystem::path& map,const UiVariant& variant,
    const MultiplayerState& state,std::span<const unsigned> ids) {
  Mesh mesh;MultiplayerScoreboard board(cache,map.parent_path()/"fonts",mesh,variant);
  check(board.available(),"Original final-scoreboard providers unavailable");
  // No held input. All local bindings execute in original view order.
  for(unsigned n=0;n<40;++n)for(unsigned view=0;view<ids.size();++view)board.step(view,false,state,ids[view]);
  for(unsigned view=0;view<ids.size();++view) {
    const auto draw=board.draw(state,ids,view,1280,720);
    check(!draw.normal_hud && draw.layout.status==FullScoreboardStatus::Ready &&
        !draw.paint.draw.vertices.empty(),"Final results did not open automatically");
    for(const auto& row:draw.layout.rows) {
      const auto& player=state.players[row.datum&65535];
      const auto text=std::to_string(player.total_score);
      check(row.widgets[4].text==std::u16string(text.begin(),text.end()),"Final roster used current score instead of total");
    }
    for(const auto& row:draw.layout.team_rows) {
      const auto text=std::to_string(state.team_total_scores[row.team]);
      check(row.widgets[3].text==std::u16string(text.begin(),text.end()),"Final team total differs");
    }
  }
}
}
int main(int argc,char**argv) {
  try {
    if(argc!=3)throw FormatError("Supply original MAP and update XBE");
    const std::filesystem::path map=argv[1];
    Cache cache(map,{},OriginalData::load_xbe(argv[2]));const auto original=cache.load_content(0,true);
    const auto& end_constants=*cache.original_data()->game_over_constants;
    check(end_constants.seconds==7 && end_constants.fade_threshold_seconds==2 &&
        std::bit_cast<std::uint32_t>(end_constants.ambient_linear_gain)==0x3e4ccccd && end_constants.fade_seconds==1,
        "Imported original postgame operands differ");
    const std::array<unsigned,4> ids{0,1,2,3};
    for(bool team:{false,true})for(int limit:{1,2})for(bool timed:{false,true}) {
      if(timed && limit!=2)continue;
      UiVariant variant{"Ending",0,{{2,timed?32767:limit},{10,timed?1:0},{89,5},{91,team?1:0}}};
      auto rules=slayer_rules(variant,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
      auto content=original;
      // Score-expiry cases declare the existing death API boundary. Timed
      // cases retain the complete actual Lockout collision/kill-volume data.
      if(!timed){content.collision.clear();content.kill_triggers.clear();}
      MultiplayerMatch match(content,0,cache.spawns(),rules);
      check(match.source_round_lifecycle().phase==1 && match.source_round_lifecycle().status==1 &&
          match.source_round_lifecycle().index==0 && match.source_round_lifecycle().start_tick==0 &&
          (match.source_match_statistics().header&255)==1 &&
          match.state().clock_seconds==std::int16_t(rules.time_ticks/30),
          "Native initial phase did not bind original validity/configured clock before the first world tick");
      match.join("Host",team?0:-1);match.join("Other local",team?0:-1);
      LanHost host(match,0,0x15f450);const std::array<std::int8_t,2> choices{7,7};
      LanClient peer("127.0.0.1:"+std::to_string(host.port()),0x15f450,2,
          team?std::span<const std::int8_t>(choices):std::span<const std::int8_t>{});
      std::array<Controls,16> controls{};
      for(unsigned n=0;n<64 && !peer.ready();++n) {
        peer.send(std::array<Controls,2>{});host.receive(controls);host.broadcast();peer.receive();
      }
      check(peer.ready() && match.player(3).connected,"Actual native ending participants did not join");
      const auto hud=cache.load_hud(content.player.datum,rules.weapon);
      check(multiplayer_hud_score_display(match.state(),0,rules.score_limit,rules.time_ticks,hud).score==0,
          "Active native HUD did not consume the zero initial win/score bank");
      if(!timed)for(unsigned victim:{2u,3u}) {
        auto& unit=const_cast<Unit&>(match.unit(victim));unit.vitality.dead=true;unit.last_damager=-1;
        match.step(controls);host.broadcast();peer.receive();compare(match.state(),peer.state());
        if(match.finished())break;
      }
      for(unsigned n=0;n<40 && !match.finished();++n) {
        peer.send(std::array<Controls,2>{});host.receive(controls);match.step(controls);
        host.broadcast();peer.receive();compare(match.state(),peer.state());
      }
      const auto result=match.state();
      check(result.finished && result.results_finalized,"Score/time expiry did not finalize native results");
      check(result.game_over_countdown==209 && !result.game_over_fade_started && !result.game_over_complete(),
          "Ending world pass did not decrement original seven-second countdown");
      verify_recorded(match);const auto retained_at_end=*match.recorded_statistics();
      if(timed) {
        check(result.tick==31 && result.winner==-1 && result.winner_team==-1 &&
            std::all_of(result.players.begin(),result.players.end(),[](const auto& p){return !p.connected || p.wins==0;}),
            "Actual Lockout tied-time ending awarded a win");
        for(unsigned i:ids)check(match.recorded_statistics()->players[i][13]==1,
            "Lifetime sampled the expiry clock after the original time owner made the engine inactive");
      }
      else {
        check(result.players[0].score==limit && result.players[0].total_score==limit && result.players[0].wins==1,
            "Winning player total/win not finalized");
        check(result.players[1].wins==(team?1:0) && (!team || (result.team_wins[0]==1 && result.team_total_scores[0]==limit)),
            "Source team final score or teammate win differs");
        for(const auto* snapshot:{&result,&peer.state()})
          check(multiplayer_hud_score_display(*snapshot,0,limit,rules.time_ticks,hud).score==limit,
              "Live/replicated source limit-one HUD did not consume the awarded win");
      }
      const auto living=timed?0u:1u;auto position=match.unit(living).body.position;
      bool moved=false;unsigned fade_requests=0;
      for(unsigned n=1;n<=225;++n) {
        controls[living].forward=n<160?1:0;
        controls[0].fire=!timed && n==160;
        // Score cases fire a real production SMG projectile after finalization
        // at explicitly positioned/vitality-adjusted bodies. Timed cases retain
        // Lockout geometry and use the separately declared fatal API boundary.
        if(n==160) {
          auto& unit=const_cast<Unit&>(match.unit(living));
          if(timed) {unit.vitality.dead=true;unit.last_damager=-1;}
          else {
            auto& shooter=const_cast<Unit&>(match.unit(0));
            shooter.body.position={0,0,0};shooter.body.velocity={};shooter.yaw=shooter.pitch=0;
            unit.body.position={20,0,0};unit.body.velocity={};unit.body.radius=1;
            unit.vitality.body=.001f;unit.vitality.shield=0;
            unit.vitality.body_stun=unit.vitality.shield_stun=32767;
          }
        }
        const bool fade_before=match.state().game_over_fade_started;
        match.step(controls);host.broadcast();peer.receive();const auto now=match.state();
        check(match.source_round_lifecycle().countdown==150,
            "Final game-over incorrectly advanced the non-game-over round countdown");
        compare_results(result,now);compare(now,peer.state());
        check(std::memcmp(&retained_at_end,match.recorded_statistics(),sizeof(retained_at_end))==0,
            "Postgame world/SMG hit modified frozen retained statistics");
        check(now.tick==result.tick+n,"Original postgame world clock froze");
        check(now.game_over_countdown==std::max(209-int(n),0) && now.game_over_complete()==(n>=209),
            "Original postgame completion/countdown phase differs");
        check(now.game_over_fade_started==(n>=149),"Original two-second fade boundary differs");
        fade_requests+=!fade_before && now.game_over_fade_started;
        if(n<160) {
          const auto p=match.unit(living).body.position;
          moved|=p.x!=position.x || p.y!=position.y || p.z!=position.z;
        }
        if(n==149) {
          const auto& fades=match.simulation().sound_classes().state();
          check(fades[4].target==-64 && fades[33].target==sound_linear_to_db(end_constants.ambient_linear_gain) &&
              fades[34].target==fades[33].target && fades[35].target==0 && fades[4].current==0 &&
              fades[4].remaining==1,"Live original class fade requests differ");
          match.advance_audio(.25f);
          check(match.simulation().sound_classes().state()[4].current==-16 &&
              match.simulation().sound_classes().state()[4].remaining==.75f,
              "Audio wall time did not advance saved class envelope separately");
          SDL_setenv("SDL_AUDIODRIVER","dummy",1);
          AudioEngine mixer(false);
          auto pcm=std::make_shared<PcmSound>(decode_sound(cache.load_sound(content.weapons.at(rules.weapon).sound)));
          check(pcm->sound_class==4 && pcm->frames()>256,"Actual SMG sound did not retain original weapon-fire class");
          mixer.play(pcm);const auto baseline=mixer.render(256);mixer.stop();
          mixer.class_gains(match.simulation().sound_classes().gains());mixer.play(pcm);
          const auto faded=mixer.render(256);bool audible=false;
          for(unsigned sample=0;sample<baseline.size();++sample) {
            audible|=baseline[sample]!=0;
            check(std::abs(faded[sample]-baseline[sample]*sound_db_to_linear(-16))<1e-6f,
                "Actual decoded weapon sound did not consume postgame class gain at mix time");
          }
          check(audible,"Original SMG sample window was all silent");
        }
        if(n>176)check(now.players[living].source_unit==null_datum,"Postgame death did not detach ownership or respawn gate failed");
      }
      check(moved && fade_requests==1,"Postgame movement froze or audio fade repeated");
      if(!timed)check(match.simulation().shots()==1 && match.simulation().hits()==1 && match.unit(living).vitality.dead,
          "Postgame production SMG impact did not reach the actual damage/ownership path");
      verify_board(cache,map,variant,result,ids);verify_board(cache,map,variant,peer.state(),peer.slots());
      if(!timed) {
        auto display=result;display.players[0].score=-23;
        if(team)display.team_scores[0]=-23;
        verify_board(cache,map,variant,display,ids); // final display reads accumulated stats
      }
    }
    // Winning hits travel through real production projectiles/attribution and
    // the score callback, including the victim-stat writes after game-over
    // begins. Position, vitality and absent arena collision are declared inputs.
    for(bool team:{false,true}) {
      UiVariant variant{"Recorded ending",0,{{2,1},{10,0},{89,5},{91,team?1:0}}};
      auto rules=slayer_rules(variant,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
      auto content=original;content.collision.clear();content.kill_triggers.clear();
      MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Shooter",team?0:-1);
      LanHost host(match,0,0x19b2b0);const std::array<std::int8_t,1> choice{7};
      LanClient peer("127.0.0.1:"+std::to_string(host.port()),0x19b2b0,1,
          team?std::span<const std::int8_t>(choice):std::span<const std::int8_t>{});
      std::array<Controls,16> controls{};
      for(unsigned n=0;n<64 && !peer.ready();++n) {
        peer.send(std::array<Controls,1>{});host.receive(controls);host.broadcast();peer.receive();
      }
      check(peer.ready() && match.player(1).connected,"Recorded projectile participant did not join native UDP");
      auto& shooter=const_cast<Unit&>(match.unit(0));auto& victim=const_cast<Unit&>(match.unit(1));
      shooter.body.position={0,0,0};shooter.body.velocity={};shooter.yaw=shooter.pitch=0;
      victim.body.position={20,0,0};victim.body.velocity={};victim.body.radius=1;
      victim.vitality.body=.001f;victim.vitality.shield=0;
      victim.vitality.body_stun=victim.vitality.shield_stun=32767;
      controls[0].fire=true;
      for(unsigned n=0;n<40 && !match.finished();++n) {
        match.step(controls);controls[0].fire=false;host.broadcast();peer.receive();
      }
      check(match.finished() && match.state().results_finalized && match.simulation().shots()==1 &&
          match.simulation().hits()==1 && match.state().players[0].kills==1 &&
          match.state().players[1].deaths==1,"Winning production SMG timeline did not reach source result writers");
      verify_recorded(match);compare(match.state(),peer.state());
      check(match.recorded_statistics()->players[1][9]==1,
          "Final winning hit froze recording before its victim-death writer");
      check(match.recorded_statistics()->players[0][12]==1 &&
          match.recorded_statistics()->players[0][13]==1 && match.recorded_statistics()->players[1][13]==1 &&
          match.recorded_statistics()->players[0][78+8*8]==1 &&
          match.recorded_statistics()->players[1][78+8*8+1]==1,
          "Winning SMG did not publish original best-spree/lifetime/weapon result records");
    }
    for(bool team:{false,true}) {
      UiVariant variant{"Capture producers",0,{{2,0},{10,0},{89,5},{91,team?1:0}}};
      auto rules=slayer_rules(variant,true);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
      auto content=original;content.collision.clear();content.kill_triggers.clear();
      MultiplayerMatch match(content,0,cache.spawns(),rules);
      match.join("Shooter",team?0:-1);match.join("Second",team?7:-1);
      LanHost host(match,0,0x1f2890);const std::array<std::int8_t,1> choice{7};
      LanClient peer("127.0.0.1:"+std::to_string(host.port()),0x1f2890,1,
          team?std::span<const std::int8_t>(choice):std::span<const std::int8_t>{});
      std::array<Controls,16> controls{};
      for(unsigned n=0;n<64 && !peer.ready();++n) {
        peer.send(std::array<Controls,1>{});host.receive(controls);host.broadcast();peer.receive();
      }
      check(peer.ready() && match.player(2).connected,"Capture producer peer did not join native UDP");
      auto step=[&] {
        match.step(controls);host.broadcast();peer.receive();compare(match.state(),peer.state());
        check(!match.finished() && !match.result_recording().frozen,"Unlimited capture match finalized unexpectedly");
      };
      auto shoot=[&](unsigned shooter_slot,unsigned victim_slot) {
        // Release through an actual weapon/world tick before a new pulse.
        // The preceding projectile can kill while its barrel is in recovery.
        controls={};step();
        check(!match.unit(shooter_slot).vitality.dead && !match.unit(victim_slot).vitality.dead,"Capture shot participant did not respawn");
        auto& shooter=const_cast<Unit&>(match.unit(shooter_slot));auto& victim=const_cast<Unit&>(match.unit(victim_slot));
        // Keep the unrelated living slot outside the measured projectile ray.
        for(unsigned i=0;i<3;++i)if(i!=shooter_slot && i!=victim_slot) {
          auto& other=const_cast<Unit&>(match.unit(i));other.body.position={0,40,0};other.body.velocity={};
        }
        shooter.body.position={0,0,0};shooter.body.velocity={};shooter.yaw=shooter.pitch=0;
        victim.body.position={20,0,0};victim.body.velocity={};victim.body.radius=1;
        victim.vitality.body=.001f;victim.vitality.shield=0;
        victim.vitality.body_stun=victim.vitality.shield_stun=32767;
        controls={};controls[shooter_slot].fire=true;
        const auto shots=match.simulation().shots();
        for(unsigned n=0;n<40 && !match.unit(victim_slot).vitality.dead;++n) {
          step();controls[shooter_slot].fire=false;
        }
        if(!match.unit(victim_slot).vitality.dead || match.simulation().shots()!=shots+1)
          throw FormatError("Capture SMG timeline failed: team="+std::to_string(team)+
              " shooter="+std::to_string(shooter_slot)+" victim="+std::to_string(victim_slot)+
              " tick="+std::to_string(match.state().tick)+" shots="+std::to_string(match.simulation().shots()-shots)+
              " hits="+std::to_string(match.simulation().hits())+
              " body="+std::to_string(match.unit(victim_slot).vitality.body));
        check(match.source_match_statistics().players[shooter_slot].last_kill_tick+1==match.state().tick,
            "Credited kill did not capture the original pre-increment world clock");
      };
      shoot(0,1);const auto first_tick=match.source_match_statistics().players[0].last_kill_tick;
      check(match.source_match_statistics().players[0].fields[9]==1 &&
          match.source_match_statistics().players[0].fields[10]==1 &&
          match.recorded_statistics()->players[0][12]==1,"First actual kill did not establish original streak clocks");
      shoot(0,2);
      check(match.source_match_statistics().players[0].last_kill_tick-first_tick<120 &&
          match.source_match_statistics().players[0].fields[9]==2 &&
          match.source_match_statistics().players[0].fields[10]==2 &&
          match.recorded_statistics()->players[0][12]==2 &&
          match.recorded_statistics()->players[0][78+8*8]==2,
          "Second actual kill within original4-second window did not extend spree/multikill/weapon totals");
      for(unsigned n=0;n<220 && match.player(1).source_current_object==null_datum;++n)step();
      check(match.player(1).source_current_object!=null_datum,"Actual stock respawn did not rebind the second participant");
      shoot(1,0);
      check(match.source_match_statistics().players[0].fields[9]==0 &&
          match.source_match_statistics().players[0].fields[10]==0 &&
          match.source_match_statistics().players[0].last_kill_tick==null_datum &&
          match.recorded_statistics()->players[0][12]==2,"Actual victim death did not reset running streak while retaining best spree");
      const auto lifetime=match.recorded_statistics()->players[0][13];
      for(unsigned n=0;n<30;++n)step();
      check(match.player(0).source_current_object==null_datum && match.recorded_statistics()->players[0][13]==lifetime,
          "Lifetime continued sampling after actual death ownership was detached");
      for(unsigned n=0;n<220 && match.player(0).source_current_object==null_datum;++n)step();
      check(match.player(0).source_current_object!=null_datum,"Actual stock respawn did not rebind the shooter");
      shoot(0,2);
      check(match.source_match_statistics().players[0].fields[9]==1 &&
          match.source_match_statistics().players[0].fields[10]==1 &&
          match.recorded_statistics()->players[0][12]==2 &&
          match.recorded_statistics()->players[0][78+8*8]==3,
          "Actual respawn kill did not restart running streak and preserve retained best-spree/weapon totals");
      for(unsigned n=0;n<30;++n)step();
      check(match.recorded_statistics()->players[0][13]>lifetime,"Lifetime did not resume after actual unit rebinding");
      if(team)check(match.recorded_statistics()->teams[0][12]==2,"Original team maximum spree was not retained");
    }
    std::cout<<"PASS six local/UDP FFA+Team time/score endings, limits1/2 with actual HUD providers, once-only totals/wins and automatic result scoreboards\n";
    std::cout<<"PASS continuing postgame world, inactive respawn/stat gates, seven-second countdown, once-only class fades and225 UDP postgame ticks per case\n";
    std::cout<<"PASS four postgame lethal production SMG timelines and actual decoded weapon-fire class gain consumed by SDL mixer\n";
    std::cout<<"PASS original retained player/team counters and win awards, frozen after final-hit death publication; two winning production SMG/UDP timelines\n";
    std::cout<<"PASS weapon/best-spree/lifetime records through two actual FFA/team SMG/UDP kill, multikill, death and stock-respawn timelines\n";
    std::cout<<"LIMIT score/time-death cases use declared fatal stimuli and projectile cases use controlled geometry/vitality; original world/session teardown, native lobby return, physical audio and two-computer LAN remain separate\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
