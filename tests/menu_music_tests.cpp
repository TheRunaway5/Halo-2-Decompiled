#include "halo2/menu_music.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_timing.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
using Row = std::vector<std::string>;
void check(bool value, const std::string &why) {
  if (!value) throw FormatError("Menu music differs: " + why);
}
std::uint32_t hex(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::uint16_t> chances(const std::string &s) {
  std::istringstream input(s); std::vector<std::uint16_t> result;
  for (std::string value; std::getline(input,value,',');)
    result.push_back(std::uint16_t(std::stoul(value)));
  return result;
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply maps and original music selector fixture");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::exists(maps / "mainmenu.map") ||
        !std::filesystem::exists(argv[2])) return 77;
    const auto scheduler_path = std::filesystem::path(argv[2]).parent_path() /
                                "menu-music-scheduler-original.tsv";
    if (!std::filesystem::exists(scheduler_path)) return 77;
    std::ifstream scheduler(scheduler_path);
    unsigned scheduler_cases = 0;
    for (std::string line; std::getline(scheduler, line);) {
      std::istringstream input(line); Row row;
      for (std::string value; input >> value;) row.push_back(value);
      check(row.size() == 8, "original scheduler fixture shape");
      FrontendMusicTransitions state(bool(std::stoi(row[1])), hex(row[2]));
      const auto action = state.request(bool(std::stoi(row[3])), hex(row[4]));
      check((action ? int(*action) : -1) == std::stoi(row[5]) &&
                state.playing() == bool(std::stoi(row[6])) &&
                state.last_change() == hex(row[7]),
            row[0] + " original playing state/time/backend transition");
      ++scheduler_cases;
    }
    check(scheduler_cases == 1036, "complete original scheduler suite");
    std::ifstream file(argv[2]); std::vector<Row> calls, play, initializations, fades;
    std::map<std::string,std::vector<SoundRngStore>> expected_stores;
    for (std::string line; std::getline(file,line);) {
      std::istringstream input(line); Row row;
      for (std::string value; input >> value;) row.push_back(value);
      if (row.empty()) continue;
      if (row[0] == "C") {
        calls.push_back(row);
        if (row[1].starts_with("play-")) play.push_back(row);
      } else if (row[0] == "R") {
        expected_stores[row.at(1)].push_back({hex(row.at(2)),hex(row.at(3)),hex(row.at(4))});
      } else if(row[0]=="I") initializations.push_back(row);
      else if(row[0]=="F") fades.push_back(row);
      else throw FormatError("Unknown selector fixture row");
    }
    unsigned draws = 0;
    for (const auto &c : calls) {
      const auto input = chances(c.at(3));
      check(input.size() == std::stoul(c.at(2)), c[1]+" input count");
      LoopingSoundSelection state{hex(c.at(4)),std::int16_t(std::stoi(c.at(5)))};
      auto seed = hex(c.at(6)); bool all = true; std::vector<SoundRngStore> actual;
      const auto selected = source_looping_sound_permutation(input,state,seed,
          std::stoul(c.at(7)) != 0,&all,&actual);
      check(selected == std::stoi(c.at(8)) && state.previous == selected,
            c[1]+" selected/previous");
      check(state.used == hex(c.at(9)) && seed == hex(c.at(10)) &&
            all == bool(std::stoul(c.at(11))),c[1]+" mask, seed, all-used");
      const auto &expected = expected_stores[c[1]];
      check(actual.size() == expected.size(),c[1]+" ordered draw count");
      for (unsigned i=0;i<actual.size();++i)
        check(actual[i].instruction == expected[i].instruction &&
              actual[i].before == expected[i].before && actual[i].after == expected[i].after,
              c[1]+" ordered LCG store "+std::to_string(i));
      draws += actual.size();
    }
    check(calls.size() == 1281 && play.size() == 128,"complete original selector suite");
    for(const auto &r:initializations) {
      auto state=hex(r[1]);const auto draw=source_crt_random(state);
      check(draw==std::stoul(r[4]) && state==hex(r[5]) &&
            source_secondary_random_seed(hex(r[2]),hex(r[3]),draw)==hex(r[6]),
            "original CRT/secondary-seed initialization");
    }
    for(const auto &r:fades) {
      const auto gain_hex=r[4].substr(6,2)+r[4].substr(4,2)+r[4].substr(2,2)+r[4].substr(0,2);
      SoundPlaybackFade fade{std::uint16_t(std::stoul(r[5])),std::bit_cast<float>(hex(gain_hex)),
          std::stoi(r[2]),std::stoi(r[3]),bool(std::stoi(r[6]))};
      const auto actual=source_sound_playback_fade(fade,std::stoi(r[7]));
      check(std::bit_cast<std::uint32_t>(actual)==hex(r[8]) && fade.start==std::stoi(r[9]) &&
            fade.end==std::stoi(r[10]),r[1]+" original fade gain/times");
    }
    check(initializations.size()==128 && fades.size()==6027,"complete source initialization/fade suite");
    std::uint32_t crt = 1;
    check(source_crt_random(crt) == 41 && crt == 0x29e2c0,
          "original CRT first draw");
    check(source_secondary_random_seed(0x11223344,0x55667788,41) ==
          (0x11223344u ^ 0x55667788u ^ 41u),"source initialization XOR");
    {
      DesktopSession desktop;
      auto &auxiliary=desktop.auxiliary_random();
      auto *identity=&auxiliary;
      auxiliary=0x84172a3b;
      (void)desktop.fresh_secondary_random();
      check(&desktop.auxiliary_random()==identity && auxiliary==0x84172a3b,
          "private secondary initialization replaced the application auxiliary word");
    }

    Cache cache(maps / "mainmenu.map");
    const auto content = cache.load_frontend();
    MenuMusic music(cache,content.music,0x146240);
    const auto &bank = music.loops(0);
    check(bank.size() == 4,"every authored loop decoded");
    SDL_setenv("SDL_AUDIODRIVER","dummy",1); AudioEngine mixer(false);
    const auto voices = music.play(mixer);
    check(voices.size() == 1,"one authored menu music voice");
    const auto definition = cache.load_looping_sound(content.music);
    const auto intro = std::make_shared<PcmSound>(decode_sound(cache.load_sound(definition.tracks[0].start)));
    AudioEngine reference(false);
    const auto gain = std::pow(10.f,definition.tracks[0].gain_db / 20.f);
    auto expected_mix = [&](const std::shared_ptr<const PcmSound> &pcm) {
      reference.play(pcm,gain);
      return reference.render(pcm->frames());
    };
    check(mixer.render(intro->frames()) == expected_mix(intro),"complete retail intro mixed");
    std::set<unsigned> visited;
    // Cross four complete retail song boundaries, then one cycle boundary.
    // Compare every decoded/mixed stereo sample to the permutation selected
    // by unchanged original instructions, including late portions of each song.
    for (unsigned i=0;i<5;++i) {
      const auto permutation = unsigned(std::stoi(play[i].at(8)));
      const auto &pcm = bank.at(permutation);
      check(music.seed() == hex(play[i].at(6)),"before playback boundary seed");
      const auto samples = mixer.render(pcm->frames());
      check(samples == expected_mix(pcm),"full permutation "+std::to_string(permutation)+" mixed unchanged");
      check(music.seed() == hex(play[i].at(10)),"after playback boundary seed");
      if (i<4) visited.insert(permutation);
      std::cout << "loop=" << permutation << " frames=" << pcm->frames()
                << " seconds=" << double(pcm->frames())/48000 << '\n';
    }
    check(visited.size() == 4,"all four retail loop permutations played before repeat");
    music.stop(mixer,voices);
    const auto fading=mixer.render(1024);
    check(std::any_of(fading.begin(),fading.end(),[](float value){return value!=0;}),
          "stop request preserves music while source fade begins");
    check(mixer.active_voices()==1,"fade retains the voice");
    mixer.render(48000*6);
    check(mixer.render(1024) == std::vector<float>(2048),"completed fade cannot restart music callback");
    check(mixer.active_voices() == 0,"stopped voice retired");
    AudioEngine timed_fade(false);
    auto tone=std::make_shared<PcmSound>();tone->stereo.assign(128,.25f);
    const auto fading_voice=timed_fade.play(tone,1,0,true);
    timed_fade.fade_out(fading_voice,5,1);
    const auto first_five_seconds=timed_fade.render(240000);
    check(first_five_seconds[0]==.25f && first_five_seconds.back()>0 &&
          timed_fade.active_voices()==1,"source fade retains audio through first5000ms");
    const auto last_millisecond=timed_fade.render(48);
    check(last_millisecond.back()>0 && timed_fade.active_voices()==1,
          "relative fade times include original+1ms anchor");
    check(timed_fade.render(48)==std::vector<float>(96) && timed_fade.active_voices()==0,
          "voice retires at original5001ms endpoint");
    AudioEngine volume(false);
    volume.play(tone,1,0,true);volume.master_volume(.5f);
    const auto half=volume.render(48);
    check(std::all_of(half.begin(),half.end(),[](float x){return x==.125f;}),"native master volume did not scale mixer output");
    volume.master_volume(0);
    check(volume.render(48)==std::vector<float>(96) && volume.active_voices()==1,"native mute changed playback lifetime or left audible PCM");
    volume.master_volume(1);
    check(volume.render(48).front()==.25f,"unmute did not restore ongoing playback");
    std::cout << calls.size() << " original selections / " << draws
              << " ordered RNG stores; complete intro and five real loop permutations mixed\n";
    std::cout<<initializations.size()<<" original seed initializations; "<<fades.size()
             <<" exact original fade gains/times; authored5s fade preserved music then retired\n";
    std::cout << scheduler_cases
              << " original music transitions including initial start and early movie skip\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
