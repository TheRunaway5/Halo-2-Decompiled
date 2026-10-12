#include "halo2/source_teleporter.hpp"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json = nlohmann::json;
namespace {
Vec3 vector(const Json &a) {
  return {std::bit_cast<float>(a[0].get<std::uint32_t>()),
          std::bit_cast<float>(a[1].get<std::uint32_t>()),
          std::bit_cast<float>(a[2].get<std::uint32_t>())};
}
Json words(Vec3 v) {
  return Json::array({std::bit_cast<std::uint32_t>(v.x),
                      std::bit_cast<std::uint32_t>(v.y),
                      std::bit_cast<std::uint32_t>(v.z)});
}
void append(Json &row, Vec3 v) {
  for (auto w : words(v))
    row.push_back(w);
}
std::vector<NetgameFlag> flags(const Json &rows) {
  std::vector<NetgameFlag> result;
  for (const auto &row : rows) {
    NetgameFlag f;
    f.source_index = result.size();
    f.position = vector(row["position"]);
    f.facing = std::bit_cast<float>(row["facing"].get<std::uint32_t>());
    f.type = row["type"];
    f.channel = row["channel"];
    f.selector12 = row.value("selector12", -1);
    result.push_back(f);
  }
  return result;
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2 || !std::filesystem::is_regular_file(argv[1]))
      return 77;
    if (std::fesetround(FE_TONEAREST) != 0)
      throw FormatError("Cannot establish source rounding");
    std::ifstream input(argv[1]);
    Json fixture;
    input >> fixture;
    if (fixture.at("schema") != 1)
      throw FormatError("Invalid teleporter fixture");
    unsigned initialized{};
    for (const auto &row : fixture.at("initializer_inputs")) {
      SourceTeleporterBank bank;
      for (auto &p : bank.pairs)
        p = {std::bit_cast<std::int32_t>(0xcdcdcdcdu),
             std::bit_cast<std::int32_t>(0xcdcdcdcdu),
             std::bit_cast<std::int32_t>(0xcdcdcdcdu),
             std::bit_cast<std::int32_t>(0xcdcdcdcdu)};
      source_teleporter_initialize(bank, flags(row));
      Json actual = {{"count", bank.count}, {"pairs", Json::array()}};
      for (const auto &p : bank.pairs)
        actual["pairs"].push_back(
            {p.entrance, p.exit, p.cooldown, p.blocked_notice});
      if (actual != fixture["initialized"][initialized])
        throw FormatError("Original teleporter initialization bank mismatch");
      ++initialized;
    }
    unsigned count{}, traversals{}, queries{}, views{}, blocked{};
    for (const auto &row : fixture.at("inputs")) {
      const auto authored = flags(row["flags"]);
      SourceTeleporterBank bank;
      bank.count = row["count"];
      for (unsigned n = 0; n < unsigned(bank.count); ++n) {
        const auto &r = row["pairs"][n];
        bank.pairs[n] = {r[0], r[1], r[2], r[3]};
      }
      std::vector<SourceTeleporterPlayer> players;
      std::vector<Vec3> roots, forwards;
      std::vector<std::uint32_t> parents;
      for (const auto &p : row["players"]) {
        players.push_back({p["datum"], p["unit"], p["flags"], p["local"]});
        roots.push_back(vector(p["root"]));
        forwards.push_back(vector(p["forward"]));
        parents.push_back(p["parent"]);
      }
      SourceTeleporterContext context{
          row["rate"],
          std::bit_cast<float>(row["seconds"].get<std::uint32_t>()), .25f,
          vector(fixture["default_up"])};
      Json events = Json::array();
      SourceTeleporterServices service;
      service.root = [&](std::uint32_t unit) {
        return roots.at(unit & 0xffff);
      };
      service.forward = [&](std::uint32_t unit) {
        return forwards.at(unit & 0xffff);
      };
      service.parent = [&](std::uint32_t unit) {
        return parents.at(unit & 0xffff);
      };
      service.destination_busy = [&](const SourceTeleporterPair &pair,
                                     std::uint32_t player) {
        const auto n = std::size_t(&pair - bank.pairs.data());
        events.push_back(Json::array({"query", n, player}));
        ++queries;
        return (row["busy"].get<unsigned>() & (1u << n)) != 0;
      };
      service.traversal_event = [&](std::uint32_t p) {
        events.push_back(Json::array({"event", p}));
        ++traversals;
      };
      service.detach_from_parent = [&](std::uint32_t unit) {
        if (parents.at(unit & 0xffff) != null_datum)
          throw FormatError("Attached unit admitted");
        events.push_back(Json::array({"detach", unit}));
      };
      service.set_position = [&](std::uint32_t unit, Vec3 p, Vec3 forward,
                                 Vec3 up) {
        auto e = Json::array({"position", unit});
        append(e, p);
        append(e, forward);
        append(e, up);
        events.push_back(e);
        roots.at(unit & 0xffff) = p;
        forwards.at(unit & 0xffff) = forward;
      };
      service.set_local_view = [&](std::int16_t local, Vec3 forward) {
        auto e = Json::array({"view", local});
        append(e, forward);
        events.push_back(e);
        ++views;
      };
      service.blocked_local_notice = [&](std::uint32_t p) {
        events.push_back(Json::array({"blocked-local", p}));
      };
      service.blocked_unit_notice = [&](std::uint32_t unit) {
        events.push_back(Json::array({"blocked-unit", unit}));
        ++blocked;
      };
      for (unsigned n = 0; n < row["steps"].get<unsigned>(); ++n)
        source_teleporter_step(bank, authored, players, context, service);
      Json actual = {{"events", events},
                     {"pairs", Json::array()},
                     {"flags", Json::array()},
                     {"roots", Json::array()},
                     {"forwards", Json::array()}};
      for (int n = 0; n < bank.count; ++n) {
        const auto &p = bank.pairs[n];
        actual["pairs"].push_back(
            {p.entrance, p.exit, p.cooldown, p.blocked_notice});
      }
      for (unsigned n = 0; n < players.size(); ++n) {
        actual["flags"].push_back(players[n].flags);
        actual["roots"].push_back(words(roots[n]));
        actual["forwards"].push_back(words(forwards[n]));
      }
      if (actual != fixture.at("outputs").at(count)) {
        std::cerr << "case " << count << "\nactual " << actual.dump()
                  << "\noriginal " << fixture["outputs"][count].dump() << '\n';
        throw FormatError(
            "Teleporter source control/float/event order mismatch");
      }
      ++count;
    }
    if (count < 768 || !traversals || !blocked || !views)
      throw FormatError("Missing teleporter branch coverage");
    // Missing dependency services must fail at the reached original boundary.
    auto authored = flags(fixture["inputs"][0]["flags"]);
    SourceTeleporterBank bank;
    source_teleporter_initialize(bank, authored);
    std::array<SourceTeleporterPlayer, 1> players{{{1, 2, 1, -1}}};
    bool rejected{};
    try {
      source_teleporter_step(bank, authored, players,
                             {30, .25f, .25f, vector(fixture["default_up"])},
                             {});
    } catch (const FormatError &) {
      rejected = true;
    }
    if (!rejected)
      throw FormatError("Missing original dependency was fabricated");
    std::cout << "PASS " << initialized << " initializers and " << count
              << " paired-image original control cases; " << queries
              << " queries " << traversals << " traversals " << views
              << " view deliveries " << blocked << " blocked notices\n";
    std::cout << "LIMIT explicit dependency inputs; no live "
                 "query/world/pose/event/network/camera owner integration\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
