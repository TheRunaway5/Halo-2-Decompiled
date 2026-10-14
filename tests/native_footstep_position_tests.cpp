#include "halo2/native_locomotion.hpp"
#include "halo2/model_markers.hpp"
#include <iostream>

int main() try {
  using namespace halo2;
  const Vec3 root{1.25f, -2.5f, 3.75f};
  const auto check = [&](Vec3 position) {
    if (position.x != root.x || position.y != root.y || position.z != root.z)
      throw FormatError("Footstep fallback lost the unit's world position");
  };
  const std::vector<std::vector<ObjectAttachmentMarker>> bank(2);
  for (unsigned marker : {2u, 3u, 0xffffu})
    check(authored_footstep_position(bank, {}, marker, root));
  check(authored_footstep_position(bank, {}, 0, root));
  ObjectAttachmentMarker marker;
  marker.local.position = {.25f, .5f, .75f};
  Transform node;
  node.position = {10, 20, 30};
  const std::vector<std::vector<ObjectAttachmentMarker>> posed_bank{{marker}};
  const auto posed = authored_footstep_position(posed_bank, std::span(&node, 1), 0, root);
  if (posed.x != 10.25f || posed.y != 20.5f || posed.z != 30.75f)
    throw FormatError("Valid footstep marker stopped using the current node pose");
  std::cout << "Footstep missing ordinal and empty lookup retain unit world position\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
