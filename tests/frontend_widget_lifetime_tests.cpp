#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <string>

using namespace halo2;
namespace {
void require(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
void same_lifetime(const MenuFrame &actual, const MenuFrame &expected,
                   const std::string &where) {
  require(actual.instance == expected.instance &&
              actual.pane_generation == expected.pane_generation &&
              actual.pane_created_clock == expected.pane_created_clock,
          where + ": retained text widget lifetime changed");
}
}

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("maps directory required");
    const std::filesystem::path maps(argv[1]);
    if (!std::filesystem::exists(maps / "mainmenu.map")) return 77;
    Cache cache(maps / "mainmenu.map");
    const auto content = cache.load_frontend();
    constexpr std::array<unsigned, 5> supported{12, 19, 38, 178, 210};
    for (const auto &[screen, definition] : content.screens) {
      const bool expected = std::find(supported.begin(), supported.end(), screen) != supported.end();
      require(frontend_text_widgets_recreated_on_focus(screen) == expected,
              "source-bounded pane recreation policy changed for screen " + std::to_string(screen));
    }
    // Separate diagnostic states can share one painter. Reused screen numbers
    //must not accidentally share the original allocation owner's text cursor.
    MenuState first(content), second(content);
    require(first.frame().instance && second.frame().instance &&
                first.frame().instance != second.frame().instance,
            "separate MenuStates reused a screen allocation identity");
    std::set<std::uint64_t> owners{first.frame().instance, second.frame().instance};
    for (const auto screen : supported) {
      MenuState state(content);
      const auto context = "screen " + std::to_string(screen);
      require(owners.insert(state.frame().instance).second, context + ": initial owner reused");
      state.ui_clock(100);
      state.open(screen);
      const auto initial = state.frame();
      require(initial.instance && owners.insert(initial.instance).second &&
                  initial.pane_generation == 0 && initial.pane_created_clock == 100,
              context + ": fresh screen did not use its construction clock");
      state.ui_clock(125);
      same_lifetime(state.frame(), initial, context + " clock input without navigation");
      if (screen != 210) {
        require(state.frame().entries.size() >= 2 &&
                    content.screens.at(screen).panes.size() >= 2,
                context + ": authored test route no longer has two selectable panes");
        const auto original_selection = initial.selected;
        const auto other_selection = (initial.selected + 1) % unsigned(initial.entries.size());
        state.ui_clock(150);
        state.select(other_selection);
        require(state.frame().instance == initial.instance &&
                    state.frame().pane_generation == 1 && state.frame().pane_created_clock == 150,
                context + ": changed focus did not recreate pane text at the event clock");
        const auto selected = state.frame();
        state.ui_clock(175);
        state.select(other_selection);
        same_lifetime(state.frame(), selected, context + " same selection");
        state.move(0);
        same_lifetime(state.frame(), selected, context + " zero move");
        state.move(int(state.frame().entries.size()));
        same_lifetime(state.frame(), selected, context + " full-cycle no-op");
        // There is intentionally no painter call between these focus events.
        //An eventual return to the same pane still owns newly built text.
        state.ui_clock(201);
        state.select(original_selection);
        state.ui_clock(202);
        state.select(other_selection);
        state.ui_clock(203);
        state.select(original_selection);
        require(state.frame().instance == initial.instance &&
                    state.frame().pane_generation == 4 && state.frame().pane_created_clock == 203,
                context + ": multiple focus changes before drawing lost pane reconstruction");
        state.ui_clock(220);
        state.move(1);
        require(state.frame().selected == other_selection && state.frame().pane_generation == 5 &&
                    state.frame().pane_created_clock == 220,
                context + ": directional focus did not recreate pane text");
        state.ui_clock(221);
        state.move(-1);
        require(state.frame().selected == original_selection && state.frame().pane_generation == 6 &&
                    state.frame().pane_created_clock == 221,
                context + ": reverse focus did not recreate pane text");
      } else {
        // Original 210's list data is supplied by matchmaking. This native
        //runtime has no service feed; do not invent a selectable network row.
        require(state.frame().entries.empty(), "screen 210 now needs actual service-feed focus coverage");
        state.ui_clock(250);
        state.move(1);
        state.select(1);
        same_lifetime(state.frame(), initial, context + " absent service selection");
      }
      const auto retained = state.frame();
      state.ui_clock(300);
      state.open(193);
      require(state.frame().instance != retained.instance && state.frame().pane_created_clock == 300,
              context + ": child reused its parent's text lifetime");
      state.ui_clock(350);
      state.back();
      require(state.frame().screen == screen, context + ": child Back missed retained parent");
      same_lifetime(state.frame(), retained, context + " child Back");
      state.ui_clock(400);
      state.open(screen);
      require(owners.insert(state.frame().instance).second &&
                  state.frame().instance != retained.instance && state.frame().pane_generation == 0 &&
                  state.frame().pane_created_clock == 400,
              context + ": same-number reentry did not create a fresh text lifetime");
      state.back();
      same_lifetime(state.frame(), retained, context + " nested same-screen Back");
    }
    // Unproved difficulty_lobby1 is deliberately outside the shared policy.
    MenuState unresolved(content);
    unresolved.ui_clock(450);
    unresolved.open(1);
    const auto unproved = unresolved.frame();
    unresolved.ui_clock(500);
    unresolved.select((unproved.selected + 1) % unsigned(unproved.entries.size()));
    same_lifetime(unresolved.frame(), unproved, "unproved screen 1 focus");

    // The ordinary profile/Split Screen/Setup/Map path retains the outgoing
    //Setup allocation as a transient channel, with its original clocks.
    MenuState lobby(content);
    lobby.accept(); lobby.accept(); lobby.text("Widget Lifetime"); lobby.commit_text(); lobby.back();
    lobby.select(2); lobby.accept(); lobby.accept();
    require(lobby.frame().screen == 14, "ordinary Split Screen route missed lobby");
    lobby.ui_clock(600);
    lobby.select(1); lobby.accept();
    require(lobby.frame().screen == 23, "ordinary lobby route missed Setup");
    const auto setup = lobby.frame();
    lobby.ui_clock(650);
    lobby.accept();
    require(lobby.frame().screen == 206 && lobby.frame().previous_lobby_setup &&
                lobby.frame().instance != setup.instance && lobby.frame().pane_created_clock == 650,
            "Map did not create its own owner beside the outgoing Setup channel");
    same_lifetime(*lobby.frame().previous_lobby_setup, setup, "retained outgoing Setup");
    std::cout << "five original pane-owner lifetimes and ordinary Setup/Map retention passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
