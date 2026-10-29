#include "halo2/controller_settings.hpp"
#include <iostream>
#include <stdexcept>
void check(bool v) {
  if (!v)
    throw std::runtime_error("Controller setting regression");
}
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
int main(int argc, char **argv) {
  using namespace halo2;
  ControllerSample s;
  ControllerSettings c;
  s.hardware[16] = 1;
  s.hardware[23] = 1;
  auto a = controller_input(s, c);
  check(a.forward == 1 && a.right == 0 && a.look_yaw == 1);
  c.sticks = 1;
  a = controller_input(s, c);
  check(a.forward == 0 && a.right == 1 && a.look_pitch == 1);
  c.sticks = 2;
  a = controller_input(s, c);
  check(a.forward == 1 && a.right == 1 && a.look_yaw == 0);
  c.sticks = 3;
  a = controller_input(s, c);
  check(a.forward == 0 && a.right == 0 && a.look_pitch == 1 && a.look_yaw == 1);
  s = {};
  s.hardware[0] = 1;
  c.buttons = 0;
  check(controller_input(s, c).grenade && !controller_input(s, c).fire);
  c.buttons = 1;
  check(controller_input(s, c).fire && !controller_input(s, c).grenade);
  c.buttons = 2;
  check(controller_input(s, c).melee && !controller_input(s, c).grenade);
  s = {};
  s.hardware[9] = 1;
  c.buttons = 3;
  check(controller_input(s, c).melee && !controller_input(s, c).zoom);
  // TU1.5 127710 writes the sixth digital held counter at processed+1D;
  // 15E70D reads that Back counter directly, outside gameplay remapping.
  for(unsigned sticks=0;sticks<4;++sticks)for(unsigned buttons=0;buttons<4;++buttons) {
    ControllerSettings settings;settings.sticks=sticks;settings.buttons=buttons;
    for(unsigned hardware=0;hardware<24;++hardware) {
      ControllerSample sample;sample.hardware[hardware]=1;
      if(controller_input(sample,settings).scoreboard!=(hardware==7))
        throw std::runtime_error("TU1.5 Back scoreboard hold missing or remapped");
    }
    check(!controller_input({},settings).scoreboard);
  }
  a = {};
  a.look_pitch = 1;
  a.look_yaw = 1;
  float y = 0, p = 0;
  c.sensitivity = 3;
  c.invert = false;
  controller_look(a, c, .1f, y, p);
  check(std::abs(y + .20943952f) < 1e-6 && std::abs(p - .10471976f) < 1e-6);
  c.invert = true;
  y = p = 0;
  controller_look(a, c, .1f, y, p);
  check(p < 0);
  c.invert=false;y=p=0;
  controller_look(a,c,5.f,y,p,false);
  check(p>3.f); // Source command owner constrains this accumulated native input.
  y=p=0;controller_look(a,c,5.f,y,p);
  check(p<=1.4922565f);
  AutoCenterState center;
  p = .6f;
  for (unsigned i = 0; i < 3; ++i)
    auto_center_look(center, true, 1, 0, false, 4, 1.f / 30, 1, 2, p);
  check(center.active && p == .6f);
  auto_center_look(center, true, 1, 0, false, 4, 1.f / 30, 1, 2, p);
  check(p < .6f);
  const float moving = p;
  auto_center_look(center, false, 1, 0, false, 4, 1, 1, 2, p);
  check(p == moving && !center.active);
  auto_center_look(center, true, 1, .1f, false, 4, 1, 1, 2, p);
  check(!center.active);
  if (argc == 2) {
    std::ifstream file(argv[1]);
    nlohmann::json rows;
    file >> rows;
    for (const auto &row : rows)
      if (row.at("keyboard") == 4) {
        ControllerSettings settings;
        settings.buttons = row.at("button");
        settings.sticks = row.at("stick");
        for (unsigned hardware = 0; hardware < 24; ++hardware) {
          ControllerSample sample;
          sample.hardware[hardware] = 1;
          const auto native = controller_input(sample, settings);
          const auto source = [&](unsigned action) {
            for (const auto &b : row.at("actions").at(action))
              if (b[0] == 2 && b[1] == hardware)
                return true;
            return false;
          };
          check(native.jump == source(0) && native.crouch == source(13) &&
                native.fire == source(10) && native.secondary == source(23));
          check(native.grenade == source(7) && native.melee == source(5) &&
                native.zoom == source(14));
          check(native.reload == source(43) && native.pickup == source(37) &&
                native.dual_wield == source(39) &&
                native.switch_weapon == source(4));
          check(native.flashlight == source(6) &&
                native.switch_grenade == (source(3) || (settings.buttons==0 && hardware==4)) &&
                native.accept == source(19) && native.cancel == source(20));
          check(native.look_yaw == float(source(54)) - float(source(53)) &&
                native.look_pitch == float(source(55)) - float(source(56)));
          check(native.forward == float(source(45)) - float(source(46)) &&
                native.right == float(source(48)) - float(source(47)));
        }
      }
  }
  std::cout << "Controller layouts, fixed Back scoreboard hold, sensitivity, inversion and tag-driven "
               "auto-centering passed\n";
}
