#include "halo2/hud.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>
using namespace halo2;
namespace {
void check(bool b, const char *message) {
  if (!b)
    throw std::runtime_error(message);
}
void layout() {
  check(hud_screen_mode(640, 480) == 0 && hud_screen_mode(640, 240) == 1 &&
            hud_screen_mode(320, 240) == 2,
        "Original logical HUD layout selectors");
  const auto full = hud_viewport(1, 0), top = hud_viewport(2, 0),
             bottom = hud_viewport(2, 1), quarter = hud_viewport(4, 3);
  check(full.safe.left == 48 && full.safe.top == 36 && full.safe.right == 592 &&
            full.safe.bottom == 444,
        "Original full safe rectangle");
  check(top.safe.bottom == 236 && bottom.safe.top == 244 &&
            bottom.outer.top == 240,
        "Original horizontal split gutter");
  check(quarter.safe.left == 324 && quarter.safe.top == 244 &&
            quarter.safe.right == 592 && quarter.safe.bottom == 444 &&
            quarter.mode == 2,
        "Original quarter split safe rectangle");
  // Original 2234E0 chooses grid mode 2 for widescreen. 2F600/2F640
  // give two vertical cells and one spanning cell for three participants.
  const auto wide_left = hud_viewport(2, 0, true),
             wide_right = hud_viewport(2, 1, true),
             three_top = hud_viewport(3, 0), three_bottom = hud_viewport(3, 1),
             three_left = hud_viewport(3, 0, true),
             three_right = hud_viewport(3, 2, true);
  check(wide_left.outer.right == 320 && wide_left.outer.bottom == 480 &&
            wide_right.outer.left == 320 && wide_right.safe.left == 324 &&
            wide_left.safe.right == 316,
        "Original widescreen two-player vertical layout");
  check(three_top.outer.right == 640 && three_top.outer.bottom == 240 &&
            three_top.mode == 1 && three_bottom.outer.top == 240 &&
            three_bottom.outer.right == 320 && three_bottom.mode == 2,
        "Original standard three-player spanning layout");
  check(three_left.outer.right == 320 && three_left.outer.bottom == 480 &&
            three_left.mode == 1 && three_right.outer.top == 240 &&
            three_right.outer.left == 320 && three_right.mode == 2,
        "Original widescreen three-player spanning layout");
  check(hud_screen_mode(320, 480, 3, true) == 2 &&
            hud_screen_mode(320, 480, 2, true) == 1 &&
            hud_screen_mode(640, 480, 3, true) == 0,
        "Original widescreen bottom-right half-layout promotion");
  HudWidget w;
  w.yes = {1, 0, 1, 0};
  check(hud_widget_visible(w, {1, 0, 0, 0}) &&
            hud_widget_visible(w, {0, 0, 1, 0}),
        "Original YES masks combine with OR across banks");
  w.no[3] = 4;
  check(!hud_widget_visible(w, {1, 0, 1, 4}),
        "Original NO masks suppress matching YES");
  w.yes = {};
  check(!hud_widget_visible(w, {1, 0, 1, 0}),
        "Empty YES masks must suppress widget");
}
void shield_history() {
  // Expected values from the untouched 13A838/22AFF1 instructions in
  // analysis/multiplayer/hud-instruction-oracle.json.
  HudShieldHistory h{1, -1, 100};
  struct Sample {
    unsigned tick;
    float current, previous, pulse;
    unsigned last;
  };
  for (const auto s :
       std::array<Sample, 8>{{{101, .5f, 1, 0, 101},
                              {115, .5f, 1, 0, 101},
                              {116, .5f, .5f, .5f, 116},
                              {117, .5f, .5f, .5333333611488342f, 117},
                              {118, .75f, .75f, -1, 118},
                              {119, .25f, .75f, 0, 119},
                              {120, .1f, .75f, 0, 119},
                              {134, .1f, .1f, .5f, 134}}}) {
    update_hud_shield(h, s.current, s.tick);
    check(h.previous == s.previous && h.pulse == s.pulse && h.tick == s.last,
          "Original shield hold/recharge history");
  }
  auto full = hud_shield_layer({1, 0, 0}, 1, 0);
  check(full.visible && full.inputs[0] == 1 && full.inputs[1] == 1 &&
            full.colors[0][0] == 0,
        "Undamaged original shield special must bind black custom color one");
  const auto hurt = hud_shield_layer({1, .25f, 0}, .5f, 0);
  check(hurt.inputs[0] == 1 && hurt.inputs[1] == .5f &&
            hurt.colors[0][0] == .75f,
        "Original damaged shield special inputs and pulse");
  check(!hud_shield_layer({0, 0, 0}, 0, 0).visible,
        "Empty shield emitted original meter layer");
  const auto extra = hud_shield_layer({2.5f, 0, 0}, 2.5f, 2);
  check(extra.visible && extra.inputs[0] == .5f && extra.colors[1][1] == 1 &&
            !hud_shield_layer({2.5f, 0, 0}, 2.5f, 3).visible,
        "Original overshield layer fractions/colors");
}
void retail_positions(const HudContent &content,
                      const std::filesystem::path &directory,
                      const std::filesystem::path &fixture, bool elite=false) {
  std::ifstream records(fixture);
  check(bool(records), "Missing original-instruction HUD position fixture");
  MultiplayerState state;
  for (auto &p : state.players) {
    p.connected = true;
    p.loaded = 60;
    p.reserve = 120;
    p.health = p.shield = 1;
    if(elite) {
      p.source_configuration=SourcePlayerConfiguration{};
      (*p.source_configuration)[0x44]=1;
    }
  }
  unsigned checked = 0;
  std::string line;
  while (std::getline(records, line)) {
    if (line.empty() || line[0] == '#' || line.starts_with("widescreen\t"))
      continue;
    std::istringstream row(line);
    unsigned wide{}, players{}, view{};
    int sequence{};
    std::string name;
    UiRect expected;
    check(bool(row >> wide >> players >> view >> name >> sequence >>
               expected.left >> expected.top >> expected.right >>
               expected.bottom),
          "Invalid original-instruction HUD position record");
    const auto widget = std::find_if(
        content.widgets.begin(), content.widgets.end(),
        [&](const auto &w) { return !w.is_text && w.name == name; });
    check(widget != content.widgets.end(), "Original HUD widget missing");
    auto isolated = content;
    isolated.widgets = {*widget};
    Mesh mesh;
    MultiplayerHud hud(std::move(isolated), directory / "fonts", mesh, 4);
    // The source oracle supplies winning input zero. Leave the per-view
    // animation at its initialized zero when comparing geometry.
    // Standard records compare the final native vertices directly with
    // untouched Xbox 22AFF1 output. Widescreen records exercise its grid,
    // anchor-specific slot and original sprite geometry on native 16:9.
    const unsigned width = wide ? 1280 : 640, height = wide ? 720 : 480;
    const auto vertices =
        hud.draw(state, view, view, players, width, height, 25, 14400);
    if (sequence < 0) {
      check(vertices.empty(), "HUD drew a source-hidden split-screen sprite");
      ++checked;
      continue;
    }
    check(!vertices.empty(), "Original HUD position record produced no sprite");
    if (wide) {
      const auto viewport = hud_viewport(players, view, true);
      constexpr float scale = 1.5f;
      const auto &safe = viewport.safe;
      auto shifted_anchor = [&](unsigned anchor) -> Vec2 {
        switch (anchor) {
        case 0:
          return {safe.right, safe.top};
        case 1:
          return {safe.left, safe.top};
        case 2:
          return {safe.left, safe.bottom};
        case 3:
          return {safe.right, safe.bottom};
        default:
          return {std::floor((safe.left + safe.right) * .5f),
                  std::floor((safe.top + safe.bottom) * .5f)};
        }
      };
      const auto original = shifted_anchor(widget->anchor);
      // Native wide output extends the cell horizontally; corner margins
      // and sprite dimensions keep their original uniform pixel scale.
      const auto &outer = viewport.outer;
      float x;
      if (widget->anchor == 0 || widget->anchor == 3)
        x = outer.right * 2 - (outer.right - safe.right) * scale;
      else if (widget->anchor == 1 || widget->anchor == 2)
        x = outer.left * 2 + (safe.left - outer.left) * scale;
      else {
        const float l = outer.left * 2 + (safe.left - outer.left) * scale,
                    r = outer.right * 2 - (outer.right - safe.right) * scale;
        // Match the original integer-center operation in the adapted HUD
        // coordinate system, before the full-surface vertex transform.
        x = std::floor(((l + r) * .5f - 160) / scale) * scale + 160;
      }
      const float dx = x - original.x * scale;
      expected.left = expected.left * scale + dx;
      expected.right = expected.right * scale + dx;
      expected.top *= scale;
      expected.bottom *= scale;
    }
    UiRect actual{vertices[0].position.y * height / 480,
                  vertices[0].position.x * width / 640,
                  vertices[0].position.y * height / 480,
                  vertices[0].position.x * width / 640};
    for (const auto &v : vertices) {
      actual.left = std::min(actual.left, v.position.x * width / 640);
      actual.right = std::max(actual.right, v.position.x * width / 640);
      actual.top = std::min(actual.top, v.position.y * height / 480);
      actual.bottom = std::max(actual.bottom, v.position.y * height / 480);
    }
    auto close = [](float a, float b) { return std::abs(a - b) < .001f; };
    if (!close(actual.left, expected.left) ||
        !close(actual.top, expected.top) ||
        !close(actual.right, expected.right) ||
        !close(actual.bottom, expected.bottom)) {
      std::ostringstream error;
      error << "HUD position mismatch wide=" << wide << " players=" << players
            << " view=" << view << " widget=" << name
            << " expected=" << expected.left << ',' << expected.top << ','
            << expected.right << ',' << expected.bottom
            << " actual=" << actual.left << ',' << actual.top << ','
            << actual.right << ',' << actual.bottom;
      throw std::runtime_error(error.str());
    }
    ++checked;
  }
  check(checked == 160,
        "Incomplete original-instruction HUD position coverage");
  std::cout
      << "Verified " << checked
      << " original-instruction bitmap positions across both display modes\n";
}
void retail_counter_positions(const HudContent &content,
                              const std::filesystem::path &directory,
                              const std::filesystem::path &fixture) {
  // Original 22AA16 first subtracts the outer cell origin, then truncates
  // the justified text rectangle. 13F0E0/13F700 use ascent and bearing.
  // Authored Conduit13 digits: bearing=(1,16), size=(7,21), advance=8.
  // Original measurement adds one to the far ink edge: '120' width=26,
  // '0' width=8. These are source/font facts, not captured native output.
  std::map<std::tuple<bool, unsigned, unsigned, std::string>, UiRect> original;
  if (!fixture.empty()) {
    std::ifstream records(fixture);
    check(bool(records), "Missing original-instruction HUD text fixture");
    std::string line;
    while (std::getline(records, line)) {
      if (line.empty() || line[0] == '#' || line.starts_with("widescreen\t"))
        continue;
      std::istringstream row(line);
      unsigned wide{}, players{}, view{};
      std::string name, text;
      UiRect bounds;
      check(bool(row >> wide >> players >> view >> name >> text >>
                 bounds.left >> bounds.top >> bounds.right >> bounds.bottom),
            "Invalid original-instruction HUD text record");
      original.emplace(std::tuple{bool(wide), players, view, name}, bounds);
    }
    check(original.size() == 60, "Incomplete original HUD text fixture");
  }
  MultiplayerState state;
  for (auto &p : state.players) {
    p.connected = true;
    p.loaded = 60;
    p.reserve = 120;
    p.health = p.shield = 1;
  }
  for (const auto name : {"bullets_right", "frag_hidden", "plasma_hidden"}) {
    const auto widget = std::find_if(
        content.widgets.begin(), content.widgets.end(),
        [&](const auto &w) { return w.is_text && w.name == name; });
    check(widget != content.widgets.end(), "Missing original HUD counter");
    auto isolated = content;
    isolated.widgets = {*widget};
    Mesh mesh;
    MultiplayerHud hud(std::move(isolated), directory / "fonts", mesh, 4);
    for (const bool wide : {false, true})
      for (unsigned players = 1; players <= 4; ++players)
        for (unsigned view = 0; view < players; ++view) {
          const unsigned width = wide ? 1280 : 640, height = wide ? 720 : 480;
          const float scale = wide ? 1.5f : 1;
          const auto viewport = hud_viewport(players, view, wide);
          const auto &outer = viewport.outer;
          const bool ammo = std::string_view(name) == "bullets_right";
          const float local_width =
              (outer.right - outer.left) * width / 640 / scale;
          const float anchor_x =
              ammo ? local_width - (outer.right - viewport.safe.right)
                   : viewport.safe.left - outer.left;
          const float local_x = anchor_x + widget->offset[viewport.mode].x,
                      local_y = viewport.safe.top - outer.top +
                                widget->offset[viewport.mode].y;
          float expected_left =
                    outer.left * width / 640 +
                    (std::trunc(local_x - (ammo ? 13 : 4)) + 1) * scale,
                expected_top =
                    outer.top * height / 480 + std::trunc(local_y) * scale,
                glyph_width = ammo ? 25 : 7, glyph_height = 21;
          if (!original.empty()) {
            const auto &bounds =
                original.at(std::tuple{wide, players, view, std::string(name)});
            glyph_width = bounds.right - bounds.left;
            glyph_height = bounds.bottom - bounds.top;
            // 4:3 output compares the actual glyph vertices directly with
            // original 22AA16/13EA60/13F0E0/13F700 instruction output.
            if (!wide) {
              expected_left = bounds.left;
              expected_top = bounds.top;
            }
          }
          const auto vertices =
              hud.draw(state, view, view, players, width, height, 25, 14400);
          check(vertices.size() == (ammo ? 18u : 6u),
                "Original HUD counter text/glyph selection mismatch");
          const float left = vertices[0].position.x * width / 640,
                      top = vertices[0].position.y * height / 480;
          float right = left, bottom = top;
          for (const auto &v : vertices) {
            right = std::max(right, v.position.x * width / 640);
            bottom = std::max(bottom, v.position.y * height / 480);
          }
          if (std::abs(left - expected_left) > .001f ||
              std::abs(top - expected_top) > .001f ||
              std::abs(right - expected_left - glyph_width * scale) > .001f ||
              std::abs(bottom - expected_top - glyph_height * scale) > .001f) {
            std::ostringstream error;
            error << "Counter position mismatch wide=" << wide
                  << " players=" << players << " view=" << view
                  << " widget=" << name << " expected=" << expected_left << ','
                  << expected_top << " actual=" << left << ',' << top;
            throw std::runtime_error(error.str());
          }
        }
  }
  std::cout << "Verified 60 viewport-local authored counter placements\n";
}
void sampled_layout(const HudContent &content,const std::filesystem::path &fonts,
                    const std::filesystem::path &assets) {
  MultiplayerState state;
  for(auto &p:state.players) {
    p.connected=true;p.loaded=30;p.reserve=120;p.shield=.5f;p.health=1;
  }
  unsigned compared{},changed{};
  for(unsigned players=1;players<=4;++players) {
    Mesh original_mesh,sampled_mesh;
    MultiplayerHud original(content,fonts,original_mesh,players),
        sampled(content,fonts,sampled_mesh,players,"Slayer",{},assets);
    check(original_mesh.materials.size()==sampled_mesh.materials.size(),
          "Vista artwork changed HUD material topology");
    for(unsigned i=0;i<original_mesh.materials.size();++i)
      changed+=original_mesh.materials[i].diffuse.pixels!=sampled_mesh.materials[i].diffuse.pixels;
    for(const auto dimensions:std::array<std::array<int,2>,2>{{{640,480},{1280,720}}})
    for(unsigned view=0;view<players;++view) {
      original.animate(state,view,view,25,14400,0);
      sampled.animate(state,view,view,25,14400,0);
      const auto a=original.draw(state,view,view,players,dimensions[0],dimensions[1],25,14400),
                 b=sampled.draw(state,view,view,players,dimensions[0],dimensions[1],25,14400);
      check(!a.empty() && a.size()==b.size(),"Vista artwork changed visible HUD widgets");
      for(unsigned i=0;i<a.size();++i) {
        check(a[i].position.x==b[i].position.x && a[i].position.y==b[i].position.y &&
              a[i].position.z==b[i].position.z && a[i].material==b[i].material,
              "Vista sampling changed original HUD or text placement");
        ++compared;
      }
      for(unsigned i=0;i<original_mesh.materials.size();++i)
        check(original_mesh.materials[i].forward_program.words==sampled_mesh.materials[i].forward_program.words,
              "Vista sampling changed original HUD material programs or inputs");
    }
  }
  check(changed>0,"HUD sampling test did not exercise Vista pixels/fonts");
  std::cout<<"Vista artwork preserved "<<compared<<" HUD vertex positions across 1-4 players and two aspects\n";
}
void species_sprites(const Cache &cache,const LevelContent &level,std::uint32_t weapon,
                     const std::filesystem::path &directory) {
  unsigned compared{};
  for(unsigned representation:{2u,3u}) {
    const auto content=cache.load_hud(level.player_representations.at(representation).biped.datum,weapon);
    for(const auto &widget:content.widgets) {
      if(widget.is_text || widget.special || (widget.flags&0x1c) || widget.bitmap==null_datum)continue;
      const auto &image=content.images.at(widget.bitmap);
      if(widget.sequence[0]<0 || unsigned(widget.sequence[0])>=image.sequences.size() ||
          image.sequences[widget.sequence[0]].size()<2)continue;
      auto isolated=content;isolated.widgets={widget};Mesh mesh;
      MultiplayerHud hud(std::move(isolated),directory/"fonts",mesh,1);
      for(unsigned appearance:{0u,1u}) {
        MultiplayerState state;auto &p=state.players[0];p.connected=true;p.loaded=60;p.reserve=120;p.shield=p.health=1;
        std::array<std::uint8_t,9> colors{10,0,0,10,std::uint8_t(appearance),0,0,0,0};
        p.source_configuration=native_selected_player_configuration("HUD",colors,-1);
        const auto vertices=hud.draw(state,0,0,1,640,480,25,14400);
        if(vertices.empty())continue;
        const auto &sprite=image.sequences[widget.sequence[0]][appearance];
        check(vertices.size()==6,"Authored species sprite did not produce one quad");
        float left=1,right=0,top=1,bottom=0;
        for(const auto &v:vertices){left=std::min(left,v.uv.x);right=std::max(right,v.uv.x);
          top=std::min(top,v.uv.y);bottom=std::max(bottom,v.uv.y);}
        check(left==sprite.left && right==sprite.right && top==sprite.top && bottom==sprite.bottom,
            "HUD species selector ignored original sprite UVs");
        check(mesh.materials.at(vertices[0].material).diffuse.pixels==image.images.at(sprite.image).pixels,
            "HUD species selector ignored original sprite image");
        ++compared;
      }
    }
  }
  check(compared>0,"Actual HUD test did not exercise species-specific sprite entries");
  std::cout<<"Verified "<<compared<<" actual Spartan/Elite HUD sprite selections\n";
}
void retail(const std::filesystem::path &directory,
            const std::filesystem::path &positions,
            const std::filesystem::path &text_positions,
            const std::filesystem::path &assets={}) {
  Cache cache(directory / "lockout.map");
  const auto level = cache.load_content(0, true);
  check(level.player_screen_offset.x==0 &&
            level.player_screen_offset.y==.16500000655651093f,
        "Original matg camera screen offsets");
  const auto weapon = cache.slayer_starting_weapon();
  species_sprites(cache,level,weapon,directory);
  auto content = cache.load_hud(level.player.datum, weapon);
  if(!assets.empty())sampled_layout(content,directory/"fonts",assets);
  check(content.widgets.size() == 41 && content.clip_capacity == 60,
        "Actual Spartan, SMG and scoreboard HUD selection");
  const auto crosshair =
      std::find_if(content.widgets.begin(), content.widgets.end(),
                   [](const auto &w) { return w.name == "crosshair"; });
  check(crosshair != content.widgets.end() &&
            crosshair->sequence == std::array<int, 3>{0, 23, 23} &&
            crosshair->registration[0].x == .5f,
        "Original split reticle sprite and registration");
  const auto shield =
      std::find_if(content.widgets.begin(), content.widgets.end(),
                   [](const auto &w) { return w.name == "shield_meter"; });
  check(shield != content.widgets.end() && shield->special == 5 &&
            shield->offset[1].y == -76 && shield->sequence[1] == 2,
        "Original split shield meter");
  check(content.strings.at(0x1600126c) == "\xee\x90\xad",
        "HUD locale scoreboard parser token");
  if (!positions.empty())
    retail_positions(content, directory, positions);
  if (!positions.empty())
    retail_positions(cache.load_hud(level.player_representations.at(3).biped.datum,weapon),
                     directory,positions.parent_path()/"retail_elite_hud_positions.tsv",true);
  retail_counter_positions(content, directory, text_positions);
  MultiplayerState state;
  for (unsigned i = 0; i < 4; ++i) {
    auto &p = state.players[i];
    p.connected = true;
    p.loaded = 60;
    p.reserve = 180;
    p.shield = 1;
    p.health = 1;
    p.score = int(i);
  }
  {
    // Original13A163 and13A183 independently set0x1000 and0x2000.
    // The real Spartan and Elite shield rows both require0x1000. Appearance
    // must choose the second sprite without disabling the shield/status row.
    auto isolated=content;
    std::erase_if(isolated.widgets,[](const auto &w){
      return w.name!="shield_meter" && w.name!="shield_mask";
    });
    check(isolated.widgets.size()==2,"Missing authored shield rows");
    Mesh shield_mesh;
    MultiplayerHud shield_hud(std::move(isolated),directory/"fonts",shield_mesh,2,
                             "Slayer",{},assets);
    auto shield_state=state;
    const std::array<std::uint8_t,9> elite_appearance{10,0,0,10,1,0,0,0,0};
    shield_state.players[1].source_configuration=native_selected_player_configuration(
        "Elite HUD",elite_appearance,-1);
    for(const float amount:{1.f,.5f,0.f,.75f,2.5f}) {
      ++shield_state.tick;
      for(unsigned id=0;id<2;++id) {
        shield_state.players[id].shield=amount;
        shield_hud.animate(shield_state,id,id,25,14400,shield_state.tick/30.0);
        const auto drawn=shield_hud.draw(shield_state,id,id,2,640,480,25,14400);
        check(!drawn.empty(),"Spartan/Elite shield status vanished");
        const auto meters=std::count_if(drawn.begin(),drawn.end(),[&](const auto &v){
          return shield_mesh.materials.at(v.material).name.ends_with("shield_test");
        });
        check(amount==0.f || meters>=6,"Elite appearance disabled the source shield meter");
        if(id==1 && meters) {
          const auto first=std::find_if(drawn.begin(),drawn.end(),[&](const auto &v){
            return shield_mesh.materials.at(v.material).name.ends_with("shield_test");
          });
          const auto viewport=hud_viewport(2,id,false);
          const auto mode=hud_screen_mode(unsigned(viewport.outer.right-viewport.outer.left),
              unsigned(viewport.outer.bottom-viewport.outer.top),shield->anchor,false);
          check(shield_mesh.materials.at(first->material).diffuse.pixels==
                    content.images.at(shield->bitmap).images.at(
                      content.images.at(shield->bitmap).sequences.at(shield->sequence[mode]).at(1).image).pixels || !assets.empty(),
                "Elite shield did not select its authored split sprite");
          if(amount==1.f) {
            const auto &m=shield_mesh.materials.at(first->material);
            auto expected=m.forward_program;
            const auto &pass=m.passes.at(m.forward_pass_index);
            std::array<float,256> scalars{};scalars[40]=1;
            std::array<ShaderVector,256> colors{};
            colors[36]={0,0,0,1};
            colors[37]={69.f/255,5.f/255,154.f/255,1};
            colors[38]={156.f/255,70.f/255,193.f/255,1};
            remap_pixel_externs(expected,scalars,colors,pass.pixel_externs,
                               pass.fragments,m.shader_switches);
            check(m.forward_program.words==expected.words,
                  "Live Elite shield still binds Spartan blue instead of original purple colors");
          }
        }
      }
    }
  }
  {
    auto isolated=content;isolated.widgets={*crosshair};
    Mesh reticle_mesh;
    MultiplayerHud reticle_hud(std::move(isolated),directory/"fonts",reticle_mesh,4);
    const auto centered=reticle_hud.draw(state,0,0,4,640,480,25,14400),
      aimed=reticle_hud.draw(state,0,0,4,640,480,25,14400,Vec2{160,142});
    check(centered.size()==6 && aimed.size()==centered.size(),
          "Projected HUD aim fixture has no crosshair");
    for(unsigned i=0;i<aimed.size();++i)
      check(aimed[i].position.x-centered[i].position.x==-22 &&
                aimed[i].position.y-centered[i].position.y==6,
            "Original projected HUD aim was not applied in physical pixels");
  }
  {
    auto voice_content=content;
    std::erase_if(voice_content.widgets,[](const auto &w){return w.name!="talking_player_icon" && w.name!="talking_player";});
    check(voice_content.widgets.size()==2,"Original talking-player rows missing");
    const auto speaker=std::find_if(voice_content.widgets.begin(),voice_content.widgets.end(),[](const auto &w){return !w.is_text;});
    const auto speaker_image=voice_content.images.at(speaker->bitmap).images.at(0);
    Mesh voice_mesh;MultiplayerHud voice_hud(std::move(voice_content),directory/"fonts",voice_mesh,4);
    const auto speaker_icon=[&](const auto &drawn) {
      return std::any_of(drawn.begin(),drawn.end(),[&](const auto &v){
        const auto &texture=voice_mesh.materials.at(v.material).diffuse;
        return texture.width==speaker_image.width && texture.height==speaker_image.height && texture.pixels==speaker_image.pixels;
      });
    };
    check(voice_hud.draw(state,0,0,4,1280,720,25,14400).empty(),"Inaudible voice displayed the source speaker HUD");
    voice_hud.voice_text("Remote Pilot");
    const auto speaking=voice_hud.draw(state,0,0,4,1280,720,25,14400);
    check(speaker_icon(speaking) && speaking.size()>6,"Audible remote name/icon did not bind original voice HUD rows");
    auto binocular=state;binocular.players[0].source_hud_zoom_mask|=0x400;
    check(voice_hud.draw(binocular,0,0,4,1280,720,25,14400).empty(),"Source binocular condition failed to suppress voice HUD");
  }
  Mesh mesh;
  MultiplayerHud hud(std::move(content), directory / "fonts", mesh, 4);
  hud.animate(state, 0, 0, 25, 14400, 0);
  hud.animate(state, 1, 1, 25, 14400, 0);
  const auto before = mesh.materials;
  const auto vertices = hud.draw(state, 0, 0, 4, 1280, 720, 25, 14400);
  check(!vertices.empty(), "Real HUD produced no geometry");
  const auto original_aspect = hud.draw(state, 0, 0, 4, 640, 480, 25, 14400);
  const float physical_scale = 1.5f;
  check(std::abs((vertices[1].position.x - vertices[0].position.x) * 2 /
                     physical_scale -
                 (original_aspect[1].position.x -
                  original_aspect[0].position.x)) < .0001f &&
            std::abs((vertices[2].position.y - vertices[0].position.y) * 1.5f /
                         physical_scale -
                     (original_aspect[2].position.y -
                      original_aspect[0].position.y)) < .0001f,
        "Widescreen changed original HUD sprite proportions");
  for (const auto &v : vertices)
    check(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
              v.material < mesh.materials.size() &&
              mesh.materials[v.material].hud_pass,
          "Invalid authored HUD vertex/material");
  // Quarter-screen SMG meter is deliberately absent; original sprite byte -1.
  check(std::none_of(vertices.begin(), vertices.end(),
                     [&](const auto &v) {
                       return mesh.materials[v.material].name.ends_with(
                           "ammo_meter");
                     }),
        "Quarter view invented an ammo meter");
  state.players[0].loaded = 30;
  state.players[0].shield = .5f;
  state.tick = 1;
  hud.animate(state, 0, 0, 25, 14400, tick_seconds);
  unsigned changed = 0;
  for (unsigned i = 0; i < mesh.materials.size(); ++i)
    if (before[i].forward_program.words !=
        mesh.materials[i].forward_program.words)
      ++changed;
  check(changed > 0,
        "Ammo/shield inputs did not update original shader constants");
  hud.animate(state, 1, 1, 25, 14400, tick_seconds);
  // Capture independent shader constants for the two owners without sharing
  // a mutable widget pass between their viewports.
  const auto player0 = hud.draw(state, 0, 0, 4, 1280, 720, 25, 14400),
             player1 = hud.draw(state, 1, 1, 4, 1280, 720, 25, 14400);
  check(!player0.empty() && !player1.empty() &&
            player0[0].material != player1[0].material,
        "Split players shared mutable HUD material");
  state.players[0].dead = true;
  check(hud.draw(state, 0, 0, 4, 1280, 720, 25, 14400).empty(),
        "Dead participant kept a live first-person HUD");
  std::cout << "Verified 41 authored HUD rows, split sprites/fonts and "
               "independent player inputs\n";
}
} // namespace
int main(int argc, char **argv) {
  try {
    layout();
    shield_history();
    if (argc > 1)
      retail(argv[1], argc > 2 ? argv[2] : std::filesystem::path{},
             argc > 3 ? argv[3] : std::filesystem::path{},
             argc > 4 ? argv[4] : std::filesystem::path{});
    std::cout << "HUD tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
