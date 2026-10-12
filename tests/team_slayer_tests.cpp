#include "halo2/slayer.hpp"
#include "halo2/lethal_attribution.hpp"
#include <bit>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
constexpr std::array<unsigned, 3> slots{0, 3, 15};
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t datum(unsigned slot) { return (7 + 5 * slot) << 16 | slot; }
void check(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
template<class T> void read(std::istringstream &row, T &value) {
  if (!(row >> value)) throw std::runtime_error("Malformed original team Slayer fixture");
}
template<class T, std::size_t N> void read(std::istringstream &row, std::array<T, N> &values) {
  for (auto &value : values) read(row, value);
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original team Slayer fixture");
    std::ifstream file(argv[1]);check(bool(file), "Missing original team Slayer fixture");
    unsigned scores = 0, ranks = 0, attributions = 0;
    for (std::string line; std::getline(file, line);) {
      if (line.empty() || line.front() == '#') continue;
      std::istringstream row(line);char type;read(row, type);
      if (type == 'S') {
        int direct, slot, kind, limit, delta;
        read(row, direct);read(row, slot);read(row, kind);read(row, limit);read(row, delta);
        std::array<int, 3> memberships, flags, before, expected;
        std::array<std::int16_t, 8> teams, expected_teams;
        read(row, memberships);read(row, flags);read(row, before);read(row, teams);
        read(row, expected);read(row, expected_teams);
        int expected_changed, expected_end;std::uint32_t expected_winner;
        read(row, expected_changed);read(row, expected_end);read(row, expected_winner);
        std::array<SlayerRankingPlayer, 3> players;
        for (unsigned n = 0; n < players.size(); ++n)
          players[n] = {datum(slots[n]), std::int16_t(before[n]),
                        std::uint16_t(flags[n]), std::uint8_t(memberships[n])};
        int team = slot;
        if (!direct) {
          const auto index = slot == 0 ? 0u : slot == 3 ? 1u : 2u;
          const auto written = slayer_score_write(players[index].score, delta, 0);
          check(written.applied && !written.end_match, "Preceding player score write differs");
          team = memberships[index];
        }
        SlayerTeamScoreResult result;
        if (team != -1) result = slayer_team_score_write(teams.at(unsigned(team)),
            std::int8_t(team), delta, limit, kind, players);
        check(result.changed == bool(expected_changed), "Original team stat-writer admission differs");
        check(result.end_match == bool(expected_end) &&
            (result.end_match ? std::uint32_t(team) : none) == expected_winner,
            "Original team score-limit completion differs in row " + std::to_string(scores));
        check(teams == expected_teams, "Original team score storage differs in row " + std::to_string(scores));
        for (unsigned n = 0; n < players.size(); ++n)
          check(players[n].score == expected[n], "Original player score storage differs");
        ++scores;
      } else if (type == 'R') {
        unsigned mask;std::uint32_t expected;std::array<std::int16_t, 8> teams;
        read(row, mask);read(row, teams);read(row, expected);
        const auto winner = slayer_team_winner(teams, std::uint8_t(mask));
        check(winner && *winner == expected, "Original retained-mask team winner differs");
        ++ranks;
      } else if (type == 'A') {
        LethalAttributionInput input;
        unsigned count;std::uint32_t member;read(row, input.incoming_player);read(row, member);read(row, count);
        check(count == 4, "Original previous history must have four entries");
        input.incoming_designator = std::bit_cast<std::int16_t>(std::uint16_t(member));
        input.tick = 200;input.victim_player = datum(3);input.result_flags = 1;
        input.victim_membership = 0;input.cause = 1;input.mode = 2;
        input.engine_present = input.statistics_valid = true;
        DamageHistory history;
        for (auto &entry : history) {
          std::uint32_t bits;unsigned cause;
          read(row, entry.player);read(row, entry.tick);read(row, bits);read(row, cause);
          entry.amount = std::bit_cast<float>(bits);entry.cause = std::uint8_t(cause);
        }
        std::uint32_t expected_credit;int expected_relation;
        std::array<int, 3> expected_kills, expected_nonenemy, expected_assists, expected_history;
        std::array<std::int16_t, 3> expected_scores;
        std::array<std::int16_t, 8> expected_teams;
        read(row, expected_credit);read(row, expected_relation);read(row, expected_kills);read(row, expected_nonenemy);
        read(row, expected_assists);read(row, expected_history);
        read(row, expected_scores);read(row, expected_teams);
        const std::array<AttributionPlayer, 3> players{{{datum(0), 0}, {datum(3), 0}, {datum(15), 1}}};
        const auto result = lethal_attribution(input, history, players);
        check(result && result->credited_player == expected_credit && result->relationship == bool(expected_relation),
            "Original team lethal credit or betrayal classification differs");
        for (unsigned n = 0; n < players.size(); ++n) {
          const bool selected = result->credited_player == players[n].datum;
          check(int(selected && result->credited_kill) == expected_kills[n] &&
              int(selected && result->nonenemy_credit) == expected_nonenemy[n],
              "Original team kill/nonenemy credit differs in attribution row " +
                  std::to_string(attributions) + " player " + std::to_string(slots[n]) +
                  " kills=" + std::to_string(expected_kills[n]) +
                  " nonenemy=" + std::to_string(expected_nonenemy[n]));
          check(std::count(result->assists.begin(), result->assists.begin() + result->assist_count,
                    players[n].datum) == expected_assists[n] &&
                std::count(result->nonenemy_history.begin(),
                    result->nonenemy_history.begin() + result->nonenemy_history_count,
                    players[n].datum) == expected_history[n],
              "Original team assist or nonenemy history classification differs");
        }
        // Follow the selected credit through the native source death callback,
        // preceding player writer and team writer, using unchanged memberships.
        std::array<SlayerRankingPlayer, 3> ranked{{{datum(0), 0, 1, 0},
            {datum(3), 0, 1, 0}, {datum(15), 0, 1, 1}}};
        std::array<std::int16_t, 8> teams{};
        const auto requests = slayer_death_scores(result->credited_player, datum(3),
            result->relationship, 1, true, false);
        for (unsigned n = 0; n < requests.count; ++n) {
          const auto &request = requests.values[n];
          const auto p = std::find_if(ranked.begin(), ranked.end(),
              [&](const auto &player) { return player.player == request.player; });
          check(p != ranked.end(), "Original team death request has no full native player datum");
          check(slayer_score_write(p->score, request.delta, 0).applied,
              "Native team death player writer rejected the source request");
          check(!slayer_team_score_write(teams[p->membership], std::int8_t(p->membership),
                    request.delta, 0, 0, ranked).end_match,
              "Native unlimited team callback ended the match");
        }
        for (unsigned n = 0; n < ranked.size(); ++n)
          check(ranked[n].score == expected_scores[n], "Original team death callback player score differs");
        check(teams == expected_teams, "Original team death callback aggregate score differs");
        ++attributions;
      } else throw std::runtime_error("Unknown original team fixture row");
      std::string tail;check(!(row >> tail), "Unexpected original team fixture columns");
    }
    check(scores == 104 && ranks == 20 && attributions == 16, "Original team fixture coverage changed");
    const std::array<std::int16_t, 8> empty{};
    check(!slayer_team_winner(empty, 0), "Absent original team mask must stay unresolved");
    std::cout << "Original team Slayer scores=" << scores << " rankings=" << ranks
              << " attribution=" << attributions << '\n';
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n';return 1; }
}
