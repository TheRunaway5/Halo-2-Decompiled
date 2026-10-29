CREATE TABLE IF NOT EXISTS clan_playlist_skill(
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,
          playlist TEXT NOT NULL, configuration TEXT NOT NULL,
          raw_rank INTEGER NOT NULL CHECK(raw_rank BETWEEN 0 AND 127),
          xp INTEGER NOT NULL CHECK(xp BETWEEN 0 AND 1073741823),
          PRIMARY KEY(clan,playlist));
        CREATE TABLE IF NOT EXISTS match_result_clan_skill(
          match TEXT NOT NULL, team INTEGER NOT NULL, clan TEXT NOT NULL,
          configuration TEXT NOT NULL, table_digest TEXT NOT NULL,
          effective_rank INTEGER NOT NULL, previous_rank INTEGER NOT NULL,
          previous_xp INTEGER NOT NULL, delta INTEGER NOT NULL,
          raw_rank INTEGER NOT NULL, xp INTEGER NOT NULL,
          PRIMARY KEY(match,team),
          FOREIGN KEY(match,team) REFERENCES match_result_teams(match,team));

CREATE TABLE IF NOT EXISTS matchmaking_ledger_lock(id INTEGER PRIMARY KEY CHECK(id=1), token INTEGER NOT NULL);
INSERT OR IGNORE INTO matchmaking_ledger_lock VALUES(1,0);
