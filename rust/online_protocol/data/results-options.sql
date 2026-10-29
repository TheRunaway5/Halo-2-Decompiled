CREATE TABLE IF NOT EXISTS match_result_options(
  match TEXT PRIMARY KEY REFERENCES match_results(id), game_id TEXT NOT NULL,
  settings BLOB NOT NULL CHECK(length(settings)=304), sha256 TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS match_result_admission(
  match TEXT PRIMARY KEY REFERENCES match_results(id), namespace TEXT NOT NULL CHECK(namespace='native-matchmaking-v1'),
  admission_sha256 TEXT NOT NULL CHECK(length(admission_sha256)=64),
  envelope_sha256 TEXT NOT NULL CHECK(length(envelope_sha256)=64), metadata TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS eos_players(player TEXT PRIMARY KEY REFERENCES players(id),
  puid TEXT NOT NULL UNIQUE CHECK(length(puid)=32));
CREATE TABLE IF NOT EXISTS eos_profile_outbox(player TEXT PRIMARY KEY REFERENCES players(id), revision INTEGER NOT NULL);
