#pragma once
#include "halo2/simulation.hpp"

namespace halo2 {
struct Simulation::BspWorld {
  std::shared_ptr<const std::vector<NavigationSector>> sectors;
  std::shared_ptr<const CollisionWorld> collision;
  NavigationWorld navigation;
  explicit BspWorld(BspContent value)
      :sectors(std::make_shared<const std::vector<NavigationSector>>(std::move(value.navigation))),
       collision(std::make_shared<const CollisionWorld>(std::move(value.collision))),navigation(*sectors){}
  BspWorld(std::shared_ptr<const std::vector<NavigationSector>> sectors,std::shared_ptr<const CollisionWorld> collision)
      :sectors(std::move(sectors)),collision(std::move(collision)),navigation(*this->sectors){}
};
}
