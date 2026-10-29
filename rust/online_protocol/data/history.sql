CREATE TABLE IF NOT EXISTS report_schema(version INTEGER PRIMARY KEY);
        INSERT OR IGNORE INTO report_schema VALUES(1);
        CREATE TABLE IF NOT EXISTS history_players(
          player TEXT PRIMARY KEY REFERENCES players(id), public_id TEXT NOT NULL UNIQUE);
        CREATE TABLE IF NOT EXISTS carnage_reports(
          match TEXT PRIMARY KEY REFERENCES match_results(id), public_id TEXT NOT NULL UNIQUE,
          format TEXT NOT NULL, admitted_at INTEGER, admitted_players INTEGER,
          published_players INTEGER NOT NULL, variant_bytes BLOB);
        CREATE INDEX IF NOT EXISTS completed_match_history ON match_results(completed DESC,id);
        CREATE TABLE IF NOT EXISTS carnage_players(
          match TEXT NOT NULL, player TEXT NOT NULL, slot INTEGER NOT NULL CHECK(slot BETWEEN 0 AND 15),
          screen_name TEXT NOT NULL, discord_username_at_match TEXT NOT NULL,
          appearance BLOB NOT NULL CHECK(length(appearance)=16),
          PRIMARY KEY(match,player), UNIQUE(match,slot),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player));
        CREATE INDEX IF NOT EXISTS career_report_players ON carnage_players(player);
        CREATE TABLE IF NOT EXISTS carnage_player_summaries(
          match TEXT NOT NULL, player TEXT NOT NULL, slot INTEGER NOT NULL,
          screen_name TEXT NOT NULL, discord_username_at_match TEXT NOT NULL,
          PRIMARY KEY(match,player),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player));
        CREATE TABLE IF NOT EXISTS carnage_general_stats(
          match TEXT NOT NULL, player TEXT NOT NULL, stat INTEGER NOT NULL CHECK(stat BETWEEN 0 AND 44),
          value INTEGER NOT NULL CHECK(value BETWEEN 1 AND 32767), PRIMARY KEY(match,player,stat),
          FOREIGN KEY(match,player) REFERENCES carnage_players(match,player));
        CREATE TABLE IF NOT EXISTS carnage_medals(
          match TEXT NOT NULL, player TEXT NOT NULL, medal INTEGER NOT NULL CHECK(medal BETWEEN 0 AND 31),
          count INTEGER NOT NULL CHECK(count BETWEEN 1 AND 32767), PRIMARY KEY(match,player,medal),
          FOREIGN KEY(match,player) REFERENCES carnage_players(match,player));
        CREATE TABLE IF NOT EXISTS carnage_weapon_stats(
          match TEXT NOT NULL, player TEXT NOT NULL, weapon INTEGER NOT NULL CHECK(weapon BETWEEN 0 AND 44),
          field INTEGER NOT NULL CHECK(field BETWEEN 0 AND 7),
          value INTEGER NOT NULL CHECK(value BETWEEN 1 AND 32767),
          PRIMARY KEY(match,player,weapon,field),
          FOREIGN KEY(match,player) REFERENCES carnage_players(match,player));
        CREATE INDEX IF NOT EXISTS career_weapon_accuracy ON carnage_weapon_stats(player,field,value)
          WHERE weapon BETWEEN 5 AND 38 AND field IN (4,5);
        CREATE TABLE IF NOT EXISTS carnage_player_accuracy(
          match TEXT NOT NULL, player TEXT NOT NULL,
          shots INTEGER NOT NULL CHECK(shots BETWEEN 0 AND 1114078),
          hits INTEGER NOT NULL CHECK(hits BETWEEN 0 AND 1114078),
          PRIMARY KEY(match,player),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player));
        CREATE INDEX IF NOT EXISTS career_verified_accuracy ON carnage_player_accuracy(player,shots,hits);
        CREATE TABLE IF NOT EXISTS carnage_weapon_accuracy(
          match TEXT NOT NULL, player TEXT NOT NULL, weapon INTEGER NOT NULL CHECK(weapon BETWEEN 5 AND 38),
          shots INTEGER NOT NULL CHECK(shots BETWEEN 0 AND 32767),
          hits INTEGER NOT NULL CHECK(hits BETWEEN 0 AND 32767),
          PRIMARY KEY(match,player,weapon),
          FOREIGN KEY(match,player) REFERENCES carnage_player_accuracy(match,player));
        CREATE TABLE IF NOT EXISTS carnage_opponents(
          match TEXT NOT NULL, player TEXT NOT NULL, opponent TEXT NOT NULL,
          field0 INTEGER NOT NULL CHECK(field0 BETWEEN 0 AND 32767),
          field1 INTEGER NOT NULL CHECK(field1 BETWEEN 0 AND 32767),
          PRIMARY KEY(match,player,opponent),
          FOREIGN KEY(match,player) REFERENCES carnage_players(match,player),
          FOREIGN KEY(match,opponent) REFERENCES carnage_players(match,player));
        CREATE TABLE IF NOT EXISTS carnage_team_stats(
          match TEXT NOT NULL, team INTEGER NOT NULL, stat INTEGER NOT NULL CHECK(stat BETWEEN 0 AND 44),
          value INTEGER NOT NULL CHECK(value BETWEEN 1 AND 32767), PRIMARY KEY(match,team,stat),
          FOREIGN KEY(match,team) REFERENCES match_result_teams(match,team));
