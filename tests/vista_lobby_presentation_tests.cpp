#include "halo2/vista_lobby_countdown.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json = nlohmann::json;
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "original fixture argument required");
        std::ifstream file(argv[1]); Json rows; file >> rows;
        unsigned checked{};
        for (const auto& row : rows) {
            const auto& q = row.at("input");
            VistaLobbyCountdownState state{q.at("before_stage"), q.at("before_clock")};
            VistaLobbyCountdownServices services;
            Json events = Json::array(); unsigned ui_queries{};
            services.ui_state = [&] {
                const auto value = q.at("ui").at(ui_queries++).get<std::int32_t>();
                events.push_back({"ui", value}); return value;
            };
            services.lifecycle_active = [&] {
                events.push_back({"active", q.at("active")}); return q.at("active").get<std::uint8_t>();
            };
            services.lifecycle_slot = [&] {
                events.push_back({"slot", q.at("slot")}); return q.at("slot").get<std::uint32_t>();
            };
            services.session_stage = [&] {
                events.push_back({"stage", q.at("stage")}); return q.at("stage").get<std::int32_t>();
            };
            services.ui_sound = [&](std::uint32_t slot) {
                events.push_back({"sound", slot, state.stage, state.stage3_clock});
                if (!q.at("sound_mutation").is_null()) state.stage = q.at("sound_mutation");
            };
            services.ui_clock = [&] {
                events.push_back({"clock", q.at("clock")}); return q.at("clock").get<std::uint32_t>();
            };
            services.duration_context = [&] { events.push_back({"duration"}); };
            services.controller_user = [&](std::int32_t controller) {
                events.push_back({"user", controller}); return q.at("user").get<std::int32_t>();
            };
            services.host_permission = [&] {
                events.push_back({"host", q.at("host")}); return q.at("host").get<std::uint8_t>();
            };
            services.write = [&](std::uint8_t active, std::int32_t stage, std::uint8_t reason, std::int32_t user) {
                events.push_back({"write", active, stage, reason, user}); return q.at("write").get<std::uint8_t>();
            };
            Json returned;
            const auto op = q.at("op").get<std::string>();
            if (op == "active") returned = unsigned(vista_lobby_countdown_active(services));
            else if (op == "stage") returned = vista_lobby_countdown_stage(services);
            else if (op == "start") returned = unsigned(vista_lobby_countdown_start_action(q.at("controller"), services));
            else vista_lobby_countdown_update(state, services);
            if (events != row.at("events") || returned != row.at("returned") ||
                state.stage != row.at("stage").get<std::int32_t>() || state.stage3_clock != row.at("clock").get<std::uint32_t>()) {
                std::cerr << "case=" << checked << " input=" << q.dump() << "\nexpected=" << row.dump()
                          << "\nactual=" << events.dump() << " returned=" << returned.dump() << " stage="
                          << state.stage << " clock=" << state.stage3_clock << '\n';
                throw std::runtime_error("original dynamic Vista lobby presentation mismatch");
            }
            ++checked;
        }
        require(checked == 893, "complete original dynamic lobby presentation corpus");
        VistaLobbyCountdownState state{7, 42};
        bool rejected{};
        try { vista_lobby_countdown_update(state, {}); }
        catch (const FormatError&) { rejected = true; }
        require(rejected && state.stage == 7 && state.stage3_clock == 42, "missing readiness service changed the owner");
        VistaLobbyCountdownServices inactive;
        inactive.ui_state = [] { return 10; };
        inactive.lifecycle_active = [] { return std::uint8_t{}; };
        vista_lobby_countdown_update(state, inactive);
        require(state.stage == -1 && state.stage3_clock == 42, "inactive update consumed unavailable services");
        std::cout << "Vista dynamic lobby presentation original cases=" << checked << " PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
