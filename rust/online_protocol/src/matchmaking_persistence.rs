//! Rust transaction plans and guards for matchmaking skill and result storage.
//!
//! SQLite executes through a temporary generic IO adapter. All SQL, branching,
//! namespaces, guest exclusions, projections and XP decisions originate here.
//! This does not supply an HTTP service or original Live arbitration.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result, array, flag, n, s};
use crate::matchmaking_skill::{
    self as skill, ComparisonRank, PersistentRawRank, RatingClan, RatingPlayer, Xp,
};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::HashMap;

fn pin(v: &Value) -> String {
    format!("{:x}", Sha256::digest(v.to_string().as_bytes()))
}
fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
fn blob(bytes: &[u8]) -> Value {
    json!({"blob":hex(bytes)})
}
fn stmt(sql: &str, params: Value) -> Value {
    json!({"sql":sql,"params":params})
}
fn need(condition: bool, status: u16, message: &'static str) -> Result<()> {
    if condition {
        Ok(())
    } else {
        Err(Error(status, message))
    }
}
fn rows(v: &Value, limit: usize) -> Result<&[Value]> {
    let values = v["rows"]
        .as_array()
        .ok_or(Error(400, "Invalid database rows"))?;
    m::check(
        values.len() <= limit && values.iter().all(Value::is_array),
        "Database row limit differs",
    )?;
    Ok(values)
}
fn cells(row: &Value, count: usize) -> Result<&[Value]> {
    let cells = row.as_array().ok_or(Error(400, "Invalid database row"))?;
    m::check(cells.len() == count, "Database column count differs")?;
    Ok(cells)
}
fn placeholders(count: usize) -> String {
    std::iter::repeat_n("?", count)
        .collect::<Vec<_>>()
        .join(",")
}

#[derive(Clone)]
enum Mode {
    Schema(String),
    Empty,
    Appearance(String),
    LegacyDisplay {
        owner: String,
        playlist: String,
    },
    Snapshot {
        clan: bool,
        owner: String,
        playlist: String,
        configuration: String,
    },
    Identity(String),
    Commit(Value),
}
#[derive(Clone)]
enum Stage {
    Start,
    AppearanceRead,
    SchemaBase,
    SchemaConsensus,
    SchemaTail,
    HistoryReports,
    HistoryIdentities,
    IdentityDone,
    LegacyDisplayRead,
    LegacyDisplayDone,
    LockSnapshot,
    SnapshotRead,
    SnapshotLegacy,
    SnapshotWritten,
    LockResult,
    ExistingResult,
    ExistingBinding,
    ExistingPublic,
    PlayerSkills,
    ClanSkills,
    Usernames,
    Written,
    Done,
}

#[derive(Clone)]
pub struct Database {
    mode: Mode,
    stage: Stage,
    serial: u64,
    pending: Option<Value>,
    atomic: bool,
    result: Value,
    username: HashMap<String, String>,
    expected_changes: Vec<Option<i64>>,
    display_raw: bool,
}

/// A bounded group of native snapshot plans shares one SQLite transaction.
/// Child frame numbers remain private; the bank exposes one monotonic sequence.
#[derive(Clone)]
pub struct DatabaseBank {
    children: Vec<Database>,
    index: usize,
    serial: u64,
    child_frame: Option<Value>,
    values: Vec<Value>,
    started: bool,
}
impl DatabaseBank {
    pub fn new(r: &Value) -> Result<Self> {
        m::fields(r, &["mode", "operations"])?;
        m::check(r["mode"] == "snapshot_bank", "Invalid database bank mode")?;
        let operations = r["operations"]
            .as_array()
            .ok_or(Error(400, "Database bank operations required"))?;
        m::check(
            operations.len() <= 16,
            "Database bank exceeds the admitted roster",
        )?;
        let mut children = Vec::new();
        for operation in operations {
            m::check(
                [
                    "player_snapshot",
                    "clan_snapshot",
                    "legacy_display",
                    "player_appearance",
                ]
                .contains(&m::text(&operation["mode"], 64)?),
                "Invalid snapshot bank operation",
            )?;
            children.push(Database::new(operation)?);
        }
        Ok(Self {
            children,
            index: 0,
            serial: 0,
            child_frame: None,
            values: Vec::new(),
            started: false,
        })
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        let mut next = self.clone();
        let result = next.apply(r)?;
        *self = next;
        Ok(result)
    }
    fn apply(&mut self, r: &Value) -> Result<Value> {
        let mut event = if r["op"] == "begin" {
            m::fields(r, &["op"])?;
            need(!self.started, 409, "Database bank already started")?;
            self.started = true;
            if self.children.is_empty() {
                return Ok(json!({"kind":"done","atomic":true,"value":[]}));
            }
            self.children[0].execute(&json!({"op":"begin"}))?
        } else {
            m::fields(r, &["op", "frame", "rows", "changes"])?;
            need(
                r["op"] == "reply"
                    && r["frame"].as_u64() == Some(self.serial)
                    && self.child_frame.is_some(),
                409,
                "Stale database bank continuation",
            )?;
            let mut reply = r.clone();
            reply["frame"] = self.child_frame.take().unwrap();
            self.children[self.index].execute(&reply)?
        };
        loop {
            if event["kind"] == "done" {
                self.values.push(event["value"].clone());
                self.index += 1;
                if self.index == self.children.len() {
                    return Ok(json!({"kind":"done","atomic":true,"value":self.values}));
                }
                event = self.children[self.index].execute(&json!({"op":"begin"}))?;
            } else {
                m::check(
                    event["kind"] == "sql",
                    "Invalid snapshot child continuation",
                )?;
                self.child_frame = Some(event["frame"].clone());
                self.serial = self
                    .serial
                    .checked_add(1)
                    .ok_or(Error(503, "Database bank frame exhausted"))?;
                event["frame"] = json!(self.serial);
                event["atomic"] = json!(true);
                return Ok(event);
            }
        }
    }
}
impl Database {
    pub fn new(r: &Value) -> Result<Self> {
        let mode = m::text(&r["mode"], 64)?;
        let mode = match mode {
            "player_appearance" => {
                m::fields(r, &["mode", "owner"])?;
                Mode::Appearance(m::text(&r["owner"], 64)?.to_owned())
            }
            "player_schema" | "clan_schema" | "result_schema" | "history_schema" => {
                m::fields(r, &["mode"])?;
                Mode::Schema(mode.to_owned())
            }
            "player_snapshot" | "clan_snapshot" => {
                m::fields(r, &["mode", "owner", "playlist", "configuration"])?;
                let configuration = m::text(&r["configuration"], 64)?.to_owned();
                skill::policy(&configuration)?;
                Mode::Snapshot {
                    clan: mode == "clan_snapshot",
                    owner: m::text(&r["owner"], 64)?.to_owned(),
                    playlist: m::text(&r["playlist"], 512)?.to_owned(),
                    configuration,
                }
            }
            "legacy_display" => {
                m::fields(r, &["mode", "owner", "playlist"])?;
                Mode::LegacyDisplay {
                    owner: m::text(&r["owner"], 64)?.to_owned(),
                    playlist: m::text(&r["playlist"], 64)?.to_owned(),
                }
            }
            "history_identity" => {
                m::fields(r, &["mode", "player"])?;
                Mode::Identity(m::text(&r["player"], 64)?.to_owned())
            }
            _ => return Err(Error(400, "Unknown Rust database operation")),
        };
        let atomic = !matches!(mode, Mode::Identity(_) | Mode::Appearance(_));
        Ok(Self {
            mode,
            stage: Stage::Start,
            serial: 0,
            pending: None,
            atomic,
            result: Value::Null,
            username: HashMap::new(),
            expected_changes: Vec::new(),
            display_raw: false,
        })
    }
    /// Setup comes from the actual Rust catalog, never caller-selected rank policy.
    pub fn public_rank(setup: &Value) -> Result<Self> {
        if setup.is_null() {
            let mut state = Self::new(&json!({"mode":"player_schema"}))?;
            state.mode = Mode::Empty;
            state.atomic = false; // No stored clan: preserve the caller's transaction untouched.
            return Ok(state);
        }
        let mut state = Self::new(setup)?;
        state.display_raw = matches!(state.mode, Mode::Snapshot { .. });
        Ok(state)
    }
    /// Only the private ABI can obtain this data from the actual Rust queue
    /// result owner. No arbitrary result payload is accepted by new().
    pub fn result(data: Value, completed: i64) -> Result<Self> {
        let admission = &data["admission"];
        m::check(
            admission["namespace"] == "native-matchmaking-v1",
            "Result namespace is not matchmaking",
        )?;
        let raw = m::hex(&data["report"], crate::security_policy::PCR_SIZE)?;
        need(
            format!("{:x}", Sha256::digest(&raw)) == m::text(&data["digest"], 64)?
                && pin(admission) == m::text(&data["admission_sha256"], 64)?,
            409,
            "Frozen result binding differs",
        )?;
        m::integer(&json!(completed), 0, 9007199254740991)?;
        let mut data = data;
        data["completed"] = json!(completed);
        Ok(Self {
            mode: Mode::Commit(data),
            stage: Stage::Start,
            serial: 0,
            pending: None,
            atomic: true,
            result: Value::Null,
            username: HashMap::new(),
            expected_changes: Vec::new(),
            display_raw: false,
        })
    }
    fn action(&mut self, mode: &str, statements: Value, stage: Stage) -> Result<Value> {
        self.serial = self
            .serial
            .checked_add(1)
            .ok_or(Error(503, "Database continuation exhausted"))?;
        let event = json!({"kind":"sql","frame":self.serial,"mode":mode,"statements":statements,"atomic":self.atomic});
        self.stage = stage;
        self.pending = Some(event.clone());
        Ok(event)
    }
    fn query(&mut self, sql: &str, params: Value, stage: Stage) -> Result<Value> {
        self.action("query", json!([stmt(sql, params)]), stage)
    }
    fn batch(
        &mut self,
        statements: Vec<Value>,
        expected: Vec<Option<i64>>,
        stage: Stage,
    ) -> Result<Value> {
        m::check(
            statements.len() == expected.len() && statements.len() <= 2048,
            "Database write batch exceeds bounds",
        )?;
        self.expected_changes = expected;
        self.action("batch", json!(statements), stage)
    }
    fn script(&mut self, sql: &str, stage: Stage) -> Result<Value> {
        // These embedded schemas contain simple DDL only. Emit individual
        // statements so SQLite's executescript implicit COMMIT cannot split
        // the surrounding atomic migration or an existing service transaction.
        let statements = sql
            .split(';')
            .map(str::trim)
            .filter(|s| !s.is_empty())
            .map(|sql| stmt(sql, json!([])))
            .collect::<Vec<_>>();
        let count = statements.len();
        self.batch(statements, vec![None; count], stage)
    }
    fn done(&mut self, value: Value) -> Result<Value> {
        let value = if self.display_raw {
            json!(skill::display(PersistentRawRank(m::integer(&value["raw"], 0, 127)? as u8)).0)
        } else {
            value
        };
        self.stage = Stage::Done;
        self.pending = None;
        Ok(json!({"kind":"done","value":value,"atomic":self.atomic}))
    }
    fn lock(&mut self, stage: Stage) -> Result<Value> {
        self.batch(
            vec![stmt(
                "UPDATE matchmaking_ledger_lock SET token=token WHERE id=1",
                json!([]),
            )],
            vec![Some(1)],
            stage,
        )
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        let mut next = self.clone();
        let result = next.apply(r)?;
        *self = next;
        Ok(result)
    }
    fn apply(&mut self, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 64)?;
        if op == "begin" {
            m::check(
                matches!(self.stage, Stage::Start),
                "Database work already started",
            )?;
            return self.start();
        }
        m::check(op == "reply", "Unknown database continuation operation")?;
        need(self.pending.is_some(), 409, "Database continuation retired")?;
        need(
            r["frame"].as_u64() == Some(self.serial),
            409,
            "Stale database continuation",
        )?;
        m::fields(r, &["op", "frame", "rows", "changes"])?;
        rows(r, 128)?;
        let changes = r["changes"]
            .as_array()
            .ok_or(Error(400, "Invalid database changes"))?;
        let pending = self.pending.as_ref().unwrap();
        m::check(
            changes.len() == pending["statements"].as_array().unwrap().len(),
            "Database result count differs",
        )?;
        let changes = changes
            .iter()
            .map(|v| m::integer(v, -1, i64::MAX))
            .collect::<Result<Vec<_>>>()?;
        if pending["mode"] == "batch" {
            for (actual, expected) in changes.iter().zip(&self.expected_changes) {
                if let Some(expected) = expected {
                    need(
                        *actual == *expected,
                        409,
                        "Frozen database row changed during finalization",
                    )?;
                }
            }
        }
        self.advance(r)
    }
    fn start(&mut self) -> Result<Value> {
        match &self.mode {
            Mode::Appearance(owner) => self.query(
                "SELECT lower(hex(appearance)) FROM player_customization WHERE player=? LIMIT 1",
                json!([owner]),
                Stage::AppearanceRead,
            ),
            Mode::Empty => self.done(Value::Null),
            Mode::LegacyDisplay { owner, playlist } => self.batch(
                vec![stmt(
                    "INSERT OR IGNORE INTO playlist_ranks VALUES(?,?,1)",
                    json!([owner, playlist]),
                )],
                vec![None],
                Stage::LegacyDisplayRead,
            ),
            Mode::Schema(kind) => match kind.as_str() {
                "player_schema" => {
                    self.script(include_str!("../data/skill-player.sql"), Stage::SchemaTail)
                }
                "clan_schema" => {
                    self.script(include_str!("../data/skill-clan.sql"), Stage::SchemaTail)
                }
                "result_schema" => {
                    self.script(include_str!("../data/results.sql"), Stage::SchemaBase)
                }
                "history_schema" => {
                    self.script(include_str!("../data/history.sql"), Stage::HistoryReports)
                }
                _ => unreachable!(),
            },
            Mode::Snapshot { .. } => self.lock(Stage::LockSnapshot),
            Mode::Identity(player) => {
                let player = player.clone();
                self.batch(
                    vec![stmt(
                        "INSERT OR IGNORE INTO history_players VALUES(?,?)",
                        json!([player, id()?]),
                    )],
                    vec![None],
                    Stage::IdentityDone,
                )
            }
            Mode::Commit(_) => self.lock(Stage::LockResult),
        }
    }
    fn snapshot_read(&mut self) -> Result<Value> {
        let Mode::Snapshot {
            clan,
            owner,
            playlist,
            ..
        } = &self.mode
        else {
            unreachable!()
        };
        let sql = if *clan {
            "SELECT raw_rank,xp,configuration FROM clan_playlist_skill WHERE clan=? AND playlist=? LIMIT 1"
        } else {
            "SELECT raw_rank,xp,configuration FROM playlist_skill WHERE player=? AND playlist=? LIMIT 1"
        };
        self.query(sql, json!([owner, playlist]), Stage::SnapshotRead)
    }
    fn snapshot_insert(&mut self) -> Result<Value> {
        let Mode::Snapshot {
            clan,
            owner,
            playlist,
            configuration,
        } = &self.mode
        else {
            unreachable!()
        };
        let mut statements = vec![stmt(
            if *clan {
                "INSERT INTO clan_playlist_skill VALUES(?,?,?,0,0)"
            } else {
                "INSERT INTO playlist_skill VALUES(?,?,?,0,0)"
            },
            json!([owner, playlist, configuration]),
        )];
        let mut expected = vec![Some(1)];
        if !*clan {
            statements.push(stmt(
                "INSERT OR IGNORE INTO playlist_ranks VALUES(?,?,1)",
                json!([owner, playlist]),
            ));
            expected.push(None);
        }
        self.result = json!({"raw":0,"xp":0,"configuration":configuration});
        self.batch(statements, expected, Stage::SnapshotWritten)
    }
    fn history_reports(&mut self) -> Result<Value> {
        self.query("SELECT m.id,(SELECT count(*) FROM match_result_players WHERE match=m.id) FROM match_results m WHERE m.id NOT IN (SELECT match FROM carnage_reports) ORDER BY m.id LIMIT 128",json!([]),Stage::HistoryReports)
    }
    fn history_identities(&mut self) -> Result<Value> {
        self.query("SELECT DISTINCT player FROM match_result_players WHERE player NOT IN (SELECT player FROM history_players) ORDER BY player LIMIT 128",json!([]),Stage::HistoryIdentities)
    }
    fn commit_data(&self) -> &Value {
        let Mode::Commit(data) = &self.mode else {
            unreachable!()
        };
        data
    }
    fn admission(&self) -> &Value {
        &self.commit_data()["admission"]
    }
    fn players(&self) -> &[Value] {
        array(self.admission(), "players")
    }
    fn check_skills(&self, response: &Value, clan: bool) -> Result<()> {
        let frozen = &self.admission()[if clan { "clan_skills" } else { "skills" }];
        let frozen = frozen.as_object().unwrap();
        let records = rows(response, 16)?;
        need(
            records.len() == frozen.len(),
            409,
            if clan {
                "Clan skill changed during this match"
            } else {
                "Playlist skill changed during this match"
            },
        )?;
        for row in records {
            let c = cells(row, 4)?;
            let key = m::text(&c[0], 64)?;
            let expected = frozen
                .get(key)
                .ok_or(Error(409, "Stored skill owner differs from admission"))?;
            need(
                c[1] == expected["raw"]
                    && c[2] == expected["xp"]
                    && c[3] == expected["configuration"],
                409,
                if clan {
                    "Clan skill changed during this match"
                } else {
                    "Playlist skill changed during this match"
                },
            )?;
        }
        Ok(())
    }
    fn load_player_skills(&mut self) -> Result<Value> {
        let skills = self.admission()["skills"].as_object().unwrap();
        if skills.is_empty() {
            return self.load_clan_skills();
        }
        let mut params = vec![self.admission()["proposal"]["playlist"].clone()];
        params.extend(skills.keys().map(|s| json!(s)));
        self.query(&format!("SELECT player,raw_rank,xp,configuration FROM playlist_skill WHERE playlist=? AND player IN ({})",placeholders(skills.len())),json!(params),Stage::PlayerSkills)
    }
    fn load_clan_skills(&mut self) -> Result<Value> {
        let skills = self.admission()["clan_skills"].as_object().unwrap();
        if skills.is_empty() {
            return self.load_usernames();
        }
        let mut params = vec![self.admission()["proposal"]["playlist"].clone()];
        params.extend(skills.keys().map(|s| json!(s)));
        self.query(&format!("SELECT clan,raw_rank,xp,configuration FROM clan_playlist_skill WHERE playlist=? AND clan IN ({})",placeholders(skills.len())),json!(params),Stage::ClanSkills)
    }
    fn load_usernames(&mut self) -> Result<Value> {
        let players = self
            .players()
            .iter()
            .filter(|p| !flag(p, "guest"))
            .map(|p| p["id"].clone())
            .collect::<Vec<_>>();
        self.query(
            &format!(
                "SELECT player,discord_username FROM player_profiles WHERE player IN ({})",
                placeholders(players.len())
            ),
            json!(players),
            Stage::Usernames,
        )
    }
    fn writes(&mut self) -> Result<Value> {
        let data = self.commit_data();
        let a = self.admission();
        let proposal = &a["proposal"];
        let raw = m::hex(&data["report"], crate::security_policy::PCR_SIZE)?;
        let match_id = &proposal["id"];
        let playlist = &proposal["playlist"];
        let public = id()?;
        let mut out = Vec::new();
        let mut expected = Vec::new();
        macro_rules! write {
            ($sql:expr,[$($params:tt)*]) => {{
                out.push(stmt($sql, json!([$($params)*])));
                expected.push(Some(1));
            }};
        }
        macro_rules! maybe {
            ($sql:expr,[$($params:tt)*]) => {{
                out.push(stmt($sql, json!([$($params)*])));
                expected.push(None);
            }};
        }
        write!(
            "INSERT INTO match_results VALUES(?,?,?,?,?,?,?,?,?)",
            [
                match_id,
                playlist,
                proposal["revision"],
                proposal["game"]["map"],
                proposal["game"]["variant"],
                u8::from(flag(a, "ranked")),
                data["completed"],
                data["digest"],
                "native-unanimous-v1"
            ]
        );
        let envelope = json!({"admission_sha256":data["admission_sha256"],"digest":data["digest"],"confirmations":data["confirmations"],
            "departed_slots":data["departed_slots"],"authority_epoch":data["authority_epoch"],"namespace":a["namespace"]});
        let envelope_pin = pin(&envelope);
        let metadata = json!({"admission":a,"finalization":envelope});
        write!(
            "INSERT INTO match_result_admission VALUES(?,?,?,?,?)",
            [
                match_id,
                a["namespace"],
                data["admission_sha256"],
                envelope_pin,
                metadata.to_string()
            ]
        );
        if !proposal["game"]["settings"].is_null() {
            let options = m::hex(&proposal["game"]["settings"], 304)?;
            write!(
                "INSERT INTO match_result_options VALUES(?,?,?,?)",
                [
                    match_id,
                    proposal["game"]["id"],
                    blob(&options),
                    format!("{:x}", Sha256::digest(&options))
                ]
            );
        }
        if flag(data, "consensus_required") {
            write!(
                "INSERT INTO match_result_consensus VALUES(?,?,?,?,?,?)",
                [
                    match_id,
                    "native-unanimous-v1",
                    array(a, "owners").len(),
                    array(data, "confirmations").len(),
                    data["departed_slots"],
                    data["authority_epoch"]
                ]
            );
        }
        let team_mode = flag(a, "team_mode");
        let teams=(0usize..8).filter(|team| raw[0xcb0+team*0x18]==1)
            .collect::<std::collections::BTreeSet<_>>();
        let signed16 = |at| i16::from_le_bytes(raw[at..at + 2].try_into().unwrap());
        if team_mode {
            for &team in &teams {
                let at = 0xcb0 + team * 0x18;
                write!(
                    "INSERT INTO match_result_teams VALUES(?,?,?,?)",
                    [match_id, team, raw[at + 1], signed16(at + 2)]
                );
            }
        }
        for (slot, p) in self.players().iter().enumerate() {
            if flag(p, "guest") {
                continue;
            }
            let at = 0x370 + slot * 0x94;
            let team = raw[at+0x88] as usize;
            let stat = |index| {
                u16::from_le_bytes(
                    raw[0xe30 + 2 * (slot * 437 + index)..0xe30 + 2 * (slot * 437 + index) + 2]
                        .try_into()
                        .unwrap(),
                ) & 0x7fff
            };
            write!(
                "INSERT INTO match_result_players VALUES(?,?,?,?,?,?,?,?)",
                [
                    match_id,
                    p["id"],
                    if team_mode {
                        raw[0xcb0 + team * 0x18 + 1]
                    } else {
                        raw[at + 0x90]
                    },
                    signed16(at + 0x92),
                    stat(7),
                    stat(9),
                    stat(8),
                    stat(11)
                ]
            );
            if team_mode {
                write!(
                    "INSERT INTO match_result_memberships VALUES(?,?,?,?)",
                    [match_id, p["id"], team, raw[at + 0x90]]
                );
            }
        }
        if let Some(conf) = a["skill_configuration"].as_str() {
            let (tables, digest) = skill::policy(conf)?;
            let mut rated = Vec::new();
            let team_places = if team_mode {
                Some(Value::Object(
                    teams
                        .iter()
                        .map(|team| (team.to_string(), json!(raw[0xcb0 + team * 0x18 + 1])))
                        .collect(),
                ))
            } else {
                None
            };
            for (slot, p) in self.players().iter().enumerate() {
                let frozen = &a["skills"][s(p, "id")];
                rated.push(RatingPlayer {
                    skill: ComparisonRank(if flag(p, "guest") {
                        -1
                    } else {
                        n(frozen, "raw") as i32
                    }),
                    place: raw[0x370 + slot * 0x94 + 0x90] as i32,
                    team: if team_mode {
                        raw[0x370+slot*0x94+0x88] as i32
                    } else {
                        slot as i32
                    },
                    xp: Xp(if flag(p, "guest") {
                        0
                    } else {
                        n(frozen, "xp") as i32
                    }),
                    key: 3,
                    flags: u32::from(flag(p, "guest")),
                    active: true,
                });
            }
            for (slot, p) in self.players().iter().enumerate() {
                if flag(p, "guest") {
                    continue;
                }
                let previous = &a["skills"][s(p, "id")];
                let delta = tables.player(&rated, slot, 3, team_places.as_ref())?;
                let xp = n(previous, "xp") + i64::from(delta);
                m::integer(&json!(xp), 0, skill::MAX_XP as i64)?;
                let rank = tables.rank(Xp(xp as i32), PersistentRawRank(n(previous, "raw") as u8));
                write!(
                    "UPDATE playlist_skill SET raw_rank=?,xp=? WHERE player=? AND playlist=? AND configuration=? AND raw_rank=? AND xp=?",
                    [
                        rank.0,
                        xp,
                        p["id"],
                        playlist,
                        conf,
                        previous["raw"],
                        previous["xp"]
                    ]
                );
                write!(
                    "UPDATE playlist_ranks SET rank=? WHERE player=? AND playlist=?",
                    [skill::display(rank).0, p["id"], playlist]
                );
                write!(
                    "INSERT INTO match_result_skill VALUES(?,?,?,?,?,?,?,?,?,?)",
                    [
                        match_id,
                        p["id"],
                        conf,
                        digest,
                        rated[slot].skill.0,
                        previous["raw"],
                        previous["xp"],
                        delta,
                        rank.0,
                        xp
                    ]
                );
            }
        }
        if let Some(conf) = a["clan_configuration"].as_str() {
            let (tables, digest) = skill::policy(conf)?;
            let mut entries = a["clans"].as_object().unwrap().iter().collect::<Vec<_>>();
            entries.sort_by_key(|(team, _)| team.parse::<usize>().unwrap());
            let rated = entries
                .iter()
                .map(|(team, clan)| {
                    let previous = &a["clan_skills"][clan.as_str().unwrap()];
                    RatingClan {
                        skill: ComparisonRank(n(previous, "raw") as i32),
                        place: raw[0xcb0 + team.parse::<usize>().unwrap() * 0x18 + 1] as i32,
                        xp: Xp(n(previous, "xp") as i32),
                        active: true,
                    }
                })
                .collect::<Vec<_>>();
            for (index, (team, clan)) in entries.iter().enumerate() {
                let previous = &a["clan_skills"][clan.as_str().unwrap()];
                let delta = tables.clan(&rated, index)?;
                let xp = n(previous, "xp") + i64::from(delta);
                m::integer(&json!(xp), 0, skill::MAX_XP as i64)?;
                let rank = tables.rank(Xp(xp as i32), PersistentRawRank(n(previous, "raw") as u8));
                write!(
                    "UPDATE clan_playlist_skill SET raw_rank=?,xp=? WHERE clan=? AND playlist=? AND configuration=? AND raw_rank=? AND xp=?",
                    [
                        rank.0,
                        xp,
                        clan,
                        playlist,
                        conf,
                        previous["raw"],
                        previous["xp"]
                    ]
                );
                write!(
                    "INSERT INTO match_result_clan_skill VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                    [
                        match_id,
                        team.parse::<usize>().unwrap(),
                        clan,
                        conf,
                        digest,
                        previous["raw"],
                        previous["raw"],
                        previous["xp"],
                        delta,
                        rank.0,
                        xp
                    ]
                );
            }
        }
        let accuracy = flag(a, "accuracy");
        let projected = if accuracy {
            crate::security_policy::persistable_accuracy_result(&raw)
        } else {
            crate::security_policy::persistable_result(&raw)
        }
        .ok_or(Error(400, "Incomplete source carnage report"))?;
        let published = self.players().iter().filter(|p| !flag(p, "guest")).count();
        write!(
            "INSERT INTO carnage_reports VALUES(?,?,?,?,?,?,?)",
            [
                match_id,
                public,
                if accuracy {
                    "native-verified-accuracy-v3"
                } else {
                    "native-verified-summary-v2"
                },
                a["created_wall"],
                self.players().len(),
                published,
                blob(&projected[0x20..0x150])
            ]
        );
        for (slot, p) in self.players().iter().enumerate() {
            if flag(p, "guest") {
                continue;
            }
            maybe!(
                "INSERT OR IGNORE INTO history_players VALUES(?,?)",
                [p["id"], id()?]
            );
            write!(
                "INSERT INTO carnage_player_summaries VALUES(?,?,?,?,?)",
                [
                    match_id,
                    p["id"],
                    slot,
                    p["name"],
                    self.username[s(p, "id")]
                ]
            );
            if accuracy {
                let mut weapons = Vec::new();
                for weapon in 5..39 {
                    let at = 0xecc + 2 * (slot * 437 + weapon * 8 + 4);
                    let shots =
                        u16::from_le_bytes(projected[at..at + 2].try_into().unwrap()) & 0x7fff;
                    let hits =
                        u16::from_le_bytes(projected[at + 2..at + 4].try_into().unwrap()) & 0x7fff;
                    if shots != 0 || hits != 0 {
                        weapons.push((weapon, shots, hits));
                    }
                }
                write!(
                    "INSERT INTO carnage_player_accuracy VALUES(?,?,?,?)",
                    [
                        match_id,
                        p["id"],
                        weapons.iter().map(|w| u32::from(w.1)).sum::<u32>(),
                        weapons.iter().map(|w| u32::from(w.2)).sum::<u32>()
                    ]
                );
                for (weapon, shots, hits) in weapons {
                    write!(
                        "INSERT INTO carnage_weapon_accuracy VALUES(?,?,?,?,?)",
                        [match_id, p["id"], weapon, shots, hits]
                    );
                }
            }
            maybe!(
                "INSERT INTO eos_profile_outbox VALUES(?,1) ON CONFLICT(player) DO UPDATE SET revision=revision+1",
                [p["id"]]
            );
        }
        self.result = json!({"public":public,"already_committed":false,"digest":data["digest"],"admission_sha256":data["admission_sha256"],"envelope_sha256":envelope_pin,"published_players":published});
        self.batch(out, expected, Stage::Written)
    }
    fn advance(&mut self, response: &Value) -> Result<Value> {
        match self.stage {
            Stage::AppearanceRead=>{
                let records=rows(response,1)?;
                let value=if let Some(row)=records.first(){let value=&cells(row,1)?[0];m::hex(value,9)?;value.clone()}else{Value::Null};
                self.done(value)
            },
            Stage::SchemaBase=>self.query("SELECT sql FROM sqlite_master WHERE type='table' AND name='match_result_consensus'",json!([]),Stage::SchemaConsensus),
            Stage::SchemaConsensus=>{
                let rows=rows(response,1)?;
                let schema=include_str!("../data/results-consensus.sql");
                if rows.first().is_some_and(|row|row[0].as_str().is_some_and(|s|!s.contains("native-unanimous-v1"))){
                    self.batch(vec![stmt(&schema.replace("match_result_consensus","match_result_consensus_v2"),json!([])),
                        stmt("INSERT INTO match_result_consensus_v2 SELECT * FROM match_result_consensus",json!([])),stmt("DROP TABLE match_result_consensus",json!([])),
                        stmt("ALTER TABLE match_result_consensus_v2 RENAME TO match_result_consensus",json!([]))],vec![None;4],Stage::SchemaTail)
                }else{self.script(schema,Stage::SchemaTail)}
            },
            Stage::SchemaTail=>{
                if matches!(&self.mode,Mode::Schema(name) if name=="result_schema"){
                    self.mode=Mode::Schema("result_options".to_owned());self.script(include_str!("../data/results-options.sql"),Stage::SchemaTail)
                }else{self.done(Value::Null)}
            },
            Stage::HistoryReports=>{
                // Script/batch completions have no rows; query after those.
                if self.pending.as_ref().unwrap()["mode"]!="query"{return self.history_reports();}
                let rows=rows(response,128)?;if rows.is_empty(){return self.history_identities();}
                let mut out=Vec::new();for row in rows{let c=cells(row,2)?;
                    out.push(stmt("INSERT INTO carnage_reports VALUES(?,?,?,NULL,NULL,?,NULL)",json!([m::text(&c[0],64)?,id()?,"legacy-summary-v1",m::integer(&c[1],0,16)?])));}
                let count=out.len();self.batch(out,vec![Some(1);count],Stage::HistoryReports)
            },
            Stage::HistoryIdentities=>{
                if self.pending.as_ref().unwrap()["mode"]!="query"{return self.history_identities();}
                let rows=rows(response,128)?;if rows.is_empty(){return self.done(Value::Null);}
                let mut out=Vec::new();for row in rows{let c=cells(row,1)?;out.push(stmt("INSERT OR IGNORE INTO history_players VALUES(?,?)",json!([m::text(&c[0],64)?,id()?])));}
                let count=out.len();self.batch(out,vec![None;count],Stage::HistoryIdentities)
            },
            Stage::LegacyDisplayRead=>{
                let Mode::LegacyDisplay { owner, playlist } = &self.mode else { unreachable!() };
                self.query("SELECT rank FROM playlist_ranks WHERE player=? AND playlist=? LIMIT 1",json!([owner,playlist]),Stage::LegacyDisplayDone)
            },
            Stage::LegacyDisplayDone=>{
                let records=rows(response,1)?;
                let row=records.first().ok_or(Error(409,"Stored public level missing"))?;
                self.done(json!(m::integer(&cells(row,1)?[0],1,50)?))
            },
            Stage::IdentityDone=>self.done(Value::Null),
            Stage::LockSnapshot=>self.snapshot_read(),
            Stage::SnapshotRead=>{
                let records=rows(response,1)?;
                let Mode::Snapshot{clan,owner,playlist,configuration}=&self.mode else{unreachable!()};
                if let Some(row)=records.first(){let c=cells(row,3)?;need(c[2]==*configuration,409,if *clan{"This clan playlist requires an explicit skill migration"}else{"This playlist requires an explicit skill migration"})?;
                    return self.done(json!({"raw":m::integer(&c[0],0,127)?,"xp":m::integer(&c[1],0,skill::MAX_XP as i64)?,"configuration":configuration}));}
                if *clan{return self.snapshot_insert();}
                self.query("SELECT rank FROM playlist_ranks WHERE player=? AND playlist=? LIMIT 1",json!([owner,playlist]),Stage::SnapshotLegacy)
            },
            Stage::SnapshotLegacy=>{let records=rows(response,1)?;need(records.is_empty()||cells(&records[0],1)?[0]==1,409,"This playlist requires an explicit skill migration")?;self.snapshot_insert()},
            Stage::SnapshotWritten=>self.done(self.result.clone()),
            Stage::LockResult=>{let key=self.admission()["proposal"]["id"].clone();self.query("SELECT digest FROM match_results WHERE id=? LIMIT 1",json!([key]),Stage::ExistingResult)},
            Stage::ExistingResult=>{
                let records=rows(response,1)?;
                if let Some(row)=records.first(){need(cells(row,1)?[0]==self.commit_data()["digest"],409,"Completed result is immutable")?;
                    return self.query("SELECT admission_sha256,envelope_sha256,namespace FROM match_result_admission WHERE match=? LIMIT 1",json!([self.admission()["proposal"]["id"]]),Stage::ExistingBinding);}
                self.load_player_skills()
            },
            Stage::ExistingBinding=>{
                let records=rows(response,1)?;need(records.len()==1,409,"Existing result has no bound admission")?;let c=cells(&records[0],3)?;
                need(c[0]==self.commit_data()["admission_sha256"]&&c[2]==self.admission()["namespace"],409,"Completed result admission is immutable")?;
                self.result=json!({"already_committed":true,"digest":self.commit_data()["digest"],"admission_sha256":c[0],"envelope_sha256":c[1]});
                self.query("SELECT public_id FROM carnage_reports WHERE match=? LIMIT 1",json!([self.admission()["proposal"]["id"]]),Stage::ExistingPublic)
            },
            Stage::ExistingPublic=>{let records=rows(response,1)?;need(records.len()==1,409,"Existing result publication is incomplete")?;self.result["public"]=json!(m::text(&cells(&records[0],1)?[0],64)?);self.done(self.result.clone())},
            Stage::PlayerSkills=>{self.check_skills(response,false)?;self.load_clan_skills()},
            Stage::ClanSkills=>{self.check_skills(response,true)?;self.load_usernames()},
            Stage::Usernames=>{
                let records=rows(response,16)?;let count=self.players().iter().filter(|p|!flag(p,"guest")).count();
                need(records.len()==count,409,"Player profile missing during result publication")?;
                for row in records{let c=cells(row,2)?;let who=m::text(&c[0],64)?.to_owned();let name=c[1].as_str().ok_or(Error(400,"Invalid stored public username"))?;
                    m::check(name.len()<=64,"Invalid stored public username")?;self.username.insert(who,name.to_owned());}
                m::check(self.players().iter().filter(|p|!flag(p,"guest")).all(|p|self.username.contains_key(s(p,"id"))),"Stored public username owner differs")?;
                self.writes()
            },
            Stage::Written=>self.done(self.result.clone()),
            Stage::Start|Stage::Done=>Err(Error(409,"Database continuation is not pending")),
        }
    }
}
