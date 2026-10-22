"""Public totals from accepted results; exact PCR hit/shot consumers only."""


def totals(db, player):
    row = db.execute("SELECT count(*),coalesce(sum(placement=0),0),coalesce(sum(kills),0),"
                     "coalesce(sum(deaths),0),coalesce(sum(assists),0) FROM match_result_players WHERE player=?",(player,)).fetchone()
    # vista_pcr_results.cpp: weapons5..38, category4 -> shots, category5 -> hits.
    # Do not label unknown general-stat indices as accuracy counters.
    weapons = dict(db.execute("SELECT field,sum(value) FROM carnage_weapon_stats WHERE player=? "
                             "AND weapon BETWEEN 5 AND 38 AND field IN (4,5) GROUP BY field",(player,)))
    complete = db.execute("SELECT count(*) FROM carnage_players WHERE player=?",(player,)).fetchone()[0]
    return dict(matches=row[0],wins=row[1],kills=row[2],deaths=row[3],assists=row[4],
                shots=weapons.get(4,0),hits=weapons.get(5,0),accuracy_matches=complete,
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
