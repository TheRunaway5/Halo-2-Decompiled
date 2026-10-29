"""Public totals from accepted results; exact PCR hit/shot consumers only."""


def totals(db, player):
    row = db.execute("SELECT count(*),coalesce(sum(placement=0),0),coalesce(sum(kills),0),"
                     "coalesce(sum(deaths),0),coalesce(sum(assists),0) FROM match_result_players WHERE player=?",(player,)).fetchone()
    # Only the native counter-verifying protocol earns accuracy coverage.
    # Legacy summary/detailed rows are not retroactively declared verified.
    shots,hits,complete=db.execute('SELECT coalesce(sum(shots),0),coalesce(sum(hits),0),count(*) '
                                  'FROM carnage_player_accuracy WHERE player=?',(player,)).fetchone()
    return dict(matches=row[0],wins=row[1],kills=row[2],deaths=row[3],assists=row[4],
                shots=shots,hits=hits,accuracy_matches=complete,
                accuracy_complete=complete==row[0])


def initialize(db):
    db.executescript("""
    CREATE TABLE IF NOT EXISTS eos_players(player TEXT PRIMARY KEY REFERENCES players(id),
      puid TEXT NOT NULL UNIQUE CHECK(length(puid)=32));
    CREATE TABLE IF NOT EXISTS eos_profile_outbox(player TEXT PRIMARY KEY REFERENCES players(id),
      revision INTEGER NOT NULL);
    """)


def enqueue(db, player):
    db.execute("INSERT INTO eos_profile_outbox VALUES(?,1) ON CONFLICT(player) DO UPDATE SET revision=revision+1",(player,))
