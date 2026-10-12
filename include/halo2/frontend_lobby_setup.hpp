#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2 {
enum class FrontendLobbySetupAction { Open, SwitchCoop, SwitchCustom, AlternateSession };
struct FrontendLobbySetupRow {
  int semantic{};
  std::uint32_t label{};
  int destination = -1;
  FrontendLobbySetupAction action = FrontendLobbySetupAction::Open;
};
inline std::uint32_t frontend_lobby_setup_label(int semantic) {
  // Literal2BB74F semantic-keyed table, not authored widget/row ordinals.
  switch (semantic) {
  case 0: return 0x0a0005d3;
  case 1: return 0x0e0005d4;
  case 2: return 0x0c0005d5;
  case 3: return 0x110005d6;
  case 4: return 0x0d00042a;
  case 6: return 0x0e0005db;
  case 7: return 0x120005dd;
  case 8: return 0x130005dc;
  case 9: return 0x0d0005d8;
  default: return 0;
  }
}
inline std::vector<FrontendLobbySetupRow> frontend_lobby_setup_rows(int session_enum) {
  std::vector<int> semantics;
  switch (session_enum) {
  case 0: case 2: semantics = {2, 3, 7}; break;
  case 1: case 3: semantics = {0, 1, 4, 6}; break;
  case 4: semantics = {2, 3, 7, 8}; break;
  case 5: semantics = {0, 1, 4, 6, 8}; break;
  case 6: semantics = {9, 6, 7}; break;
  }
  std::vector<FrontendLobbySetupRow> result;
  for (const int semantic : semantics) {
    FrontendLobbySetupRow row{semantic, frontend_lobby_setup_label(semantic)};
    switch (semantic) {
    case 0: row.destination = 206; break;
    case 1: row.destination = 207; break;
    // Alternate11/12 loaders update the session and close their own screen;
    // IDs also used by ordinary campaign launch need caller context.
    case 2: row.destination = 11; break;
    case 3: row.destination = 12; break;
    case 4: row.destination = 222; break;
    case 6: row.destination = 14; row.action = FrontendLobbySetupAction::SwitchCoop; break;
    case 7: row.destination = 14; row.action = FrontendLobbySetupAction::SwitchCustom; break;
    case 8: case 9: row.destination = 237; row.action = FrontendLobbySetupAction::AlternateSession; break;
    }
    result.push_back(row);
  }
  return result;
}
inline int frontend_lobby_setup_focused_semantic(int session_enum, int row) {
  const auto rows = frontend_lobby_setup_rows(session_enum);
  return row >= 0 && std::size_t(row) < rows.size() ? rows[std::size_t(row)].semantic : -1;
}
inline std::uint32_t frontend_lobby_setup_subtitle(int session_enum) {
  return session_enum == 0 || session_enum == 1 ? 0x0b0005f9 : 0x0a0005f8;
}
inline std::optional<int> frontend_lobby_setup_quick_category(int engine) {
  // Original2BB978 calls2C83E6 with these exact category IDs.
  switch (engine) {
  case 1: return 22;
  case 2: return 23;
  case 3: return 24;
  case 4: return 25;
  case 7: return 28;
  case 8: return 29;
  case 9: return 30;
  default: return std::nullopt;
  }
}
enum class FrontendLobbySetupPreview { Authored, Map, CampaignLevel, Engine, Difficulty };
struct FrontendLobbySetupDetailInput {
  int semantic = -1;
  std::optional<int> engine;
  int difficulty{};
  bool settings_valid{}, entry_valid{};
};
struct FrontendLobbySetupDetail {
  // Original type6 ordinals2/3/4 are authored raw text0/1/2 respectively.
  std::array<std::uint32_t, 3> strings{};
  int authored_frame{};
  FrontendLobbySetupPreview preview = FrontendLobbySetupPreview::Authored;
  std::optional<int> descriptor_bank, descriptor_frame;
};
inline FrontendLobbySetupDetail frontend_lobby_setup_detail(const FrontendLobbySetupDetailInput &input) {
  FrontendLobbySetupDetail result;
  // Original2BBBF6 first clears the override pointer. Original2B0B19 still
  // falls back to the authored bitmap; nullptr is not a visibility override.
  switch (input.semantic) {
  case 0:
    result.strings = {0x0f0005e0, 0x0b0005ec, 0x030005f4};
    if (input.settings_valid && input.entry_valid) result.preview = FrontendLobbySetupPreview::Map;
    break;
  case 1:
    result.strings = {0x130005e1, 0x0f0005ed, 0x0700055d};
    if (input.engine) {
      const int engine = *input.engine;
      const int frame = engine == 1 ? 6 : engine == 2 ? 0 : engine == 3 ? 3 :
                        engine == 4 ? 1 : engine == 7 ? 4 : engine == 8 ? 8 :
                        engine == 9 ? 7 : -1;
      if (frame >= 0) {
        result.preview = FrontendLobbySetupPreview::Engine;
        result.descriptor_bank = 1; result.descriptor_frame = frame;
      }
    }
    break;
  case 2:
    result.strings = {0x110005e2, 0x0d0005ee, 0x050005f5};
    if (input.settings_valid && input.entry_valid) result.preview = FrontendLobbySetupPreview::CampaignLevel;
    break;
  case 3:
    result.strings = {0x160005e3, 0x120005ef, 0x0a0005f6};
    if (input.difficulty >= 0 && input.difficulty <= 3) {
      result.preview = FrontendLobbySetupPreview::Difficulty;
      result.descriptor_bank = 0; result.descriptor_frame = input.difficulty;
    }
    break;
  case 4: result.strings = {0x120005df, 0x0f0005ed, 0x0700055d}; result.authored_frame = 2; break;
  case 6: result.strings = {0x130005e8, 0x0e0005db, 0}; result.authored_frame = 7; break;
  case 7: result.strings = {0x1a0005e7, 0x120005dd, 0}; result.authored_frame = 6; break;
  case 8: result.strings = {0x180005e9, 0x130005dc, 0}; break;
  case 9: result.strings = {0x120005e5, 0x0e0005f1, 0x060005f7}; result.authored_frame = 4; break;
  }
  return result;
}
} // namespace halo2
