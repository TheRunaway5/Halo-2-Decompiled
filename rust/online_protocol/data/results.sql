CREATE TABLE IF NOT EXISTS match_results(
          id TEXT PRIMARY KEY, playlist TEXT NOT NULL, revision TEXT NOT NULL,
          map TEXT NOT NULL, variant TEXT NOT NULL, ranked INTEGER NOT NULL,
          completed INTEGER NOT NULL, digest TEXT NOT NULL,
          validation TEXT NOT NULL CHECK(validation='native-unanimous-v1'));
        CREATE TABLE IF NOT EXISTS match_result_players(
          match TEXT NOT NULL REFERENCES match_results(id),
          player TEXT NOT NULL REFERENCES players(id), placement INTEGER NOT NULL,
          score INTEGER NOT NULL, kills INTEGER NOT NULL, deaths INTEGER NOT NULL,
          assists INTEGER NOT NULL, suicides INTEGER NOT NULL,
          PRIMARY KEY(match,player));
        CREATE INDEX IF NOT EXISTS player_match_results ON match_result_players(player,match);
        CREATE TABLE IF NOT EXISTS match_result_teams(
          match TEXT NOT NULL REFERENCES match_results(id), team INTEGER NOT NULL CHECK(team BETWEEN 0 AND 7),
          placement INTEGER NOT NULL, score INTEGER NOT NULL, PRIMARY KEY(match,team));
        CREATE TABLE IF NOT EXISTS match_result_memberships(
          match TEXT NOT NULL, player TEXT NOT NULL, team INTEGER NOT NULL CHECK(team BETWEEN 0 AND 7),
          individual_placement INTEGER NOT NULL, PRIMARY KEY(match,player),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player),
          FOREIGN KEY(match,team) REFERENCES match_result_teams(match,team));

CREATE TABLE IF NOT EXISTS matchmaking_ledger_lock(id INTEGER PRIMARY KEY CHECK(id=1), token INTEGER NOT NULL);
INSERT OR IGNORE INTO matchmaking_ledger_lock VALUES(1,0);
