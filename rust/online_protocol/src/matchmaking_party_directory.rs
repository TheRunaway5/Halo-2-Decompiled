//! Accepted-party directory policy and transactional SQL, not Xbox Live recovery.
//!
//! The continuation journal replays decisions over immutable replies. Each SQL,
//! provider and native-owner operation executes once; a continuation cannot be
//! reordered, replayed, or expanded into an unbounded query/result bank.
use crate::matchmaking_coordinator::{id, value};
use crate::matchmaking_model::{self as m, Error, Result, array};
use serde_json::{Value, json};
use std::collections::BTreeMap;

const MAX_STEPS: usize = 1024;
const MAX_JOURNAL_BYTES: usize = 1024 * 1024;
#[path="matchmaking_clan_directory.rs"]
mod clans;
#[derive(Clone)]
struct Entry {
    request: Value,
    reply: Value,
}
enum FlowError {
    Failed(Error),
    Yield(Value),
}
impl From<Error> for FlowError {
    fn from(value: Error) -> Self {
        Self::Failed(value)
    }
}
type Flow<T> = std::result::Result<T, FlowError>;
fn need(ok: bool, status: u16, message: &'static str) -> Flow<()> {
    if ok {
        Ok(())
    } else {
        Err(Error(status, message).into())
    }
}
fn statement(sql: &str, params: Value, changes: Option<i64>) -> Value {
    json!({"sql":sql,"params":params,"changes":changes})
}
pub struct Directory {
    config: Value,
    source: Option<u64>,
    journal: Vec<Entry>,
    entropy: BTreeMap<String, String>,
    pending: Option<Value>,
    bytes: usize,
    started: bool,
    terminal: bool,
}
struct Cursor<'a> {
    journal: &'a [Entry],
    entropy: &'a mut BTreeMap<String, String>,
    index: usize,
    source: bool,
    clock: f64,
}
impl Directory {
    pub fn new(config: &Value) -> Result<Self> {
        m::fields_optional(
            config,
            &["mode", "source", "player", "wall", "clock"],
            &["body", "features", "party", "target", "admitted", "steam","family","account_source","account_key","account_now"],
        )?;
        m::check(config["family"].is_null() || config["family"]=="party" || config["family"]=="clan","Unknown directory family")?;
        m::check(config["account_source"].is_null()==config["account_key"].is_null(),"Account source/key pair required")?;
        if !config["account_source"].is_null(){
            config["account_source"].as_u64().ok_or(Error(400,"Account source owner required"))?;
            m::hex(&config["account_key"],32)?;m::text(&config["player"],64)?;
            let now=m::real(&config["account_now"])?;
            m::check(now>=0. && (now+900.).is_finite(),"Invalid directory account clock")?;
        }
        if config["family"]=="clan" && ["command","get"].iter().any(|m|config["mode"]==*m){
            m::check(!config["account_source"].is_null(),"Authenticated clan directory source required")?;
        }
        let source = if config["source"].is_null() {
            None
        } else {
            Some(
                config["source"]
                    .as_u64()
                    .ok_or(Error(400, "Party source owner required"))?,
            )
        };
        m::text(&config["mode"], 32)?;
        if !config["player"].is_null() {
            m::text(&config["player"], 64)?;
        }
        m::integer(&config["wall"], 0, 9007199254650000)?;
        let clock = m::real(&config["clock"])?;
        m::check(clock >= 0. && clock.is_finite(), "Invalid party clock")?;
        Ok(Self {
            config: config.clone(),
            source,
            journal: Vec::new(),
            entropy: BTreeMap::new(),
            pending: None,
            bytes: 0,
            started: false,
            terminal: false,
        })
    }
    pub fn source(&self) -> Option<u64> {
        self.source
    }
    pub fn account_binding(&self)->Option<(u64,String,String,f64)>{
        Some((self.config["account_source"].as_u64()?,self.config["account_key"].as_str()?.to_owned(),self.config["player"].as_str()?.to_owned(),self.config["account_now"].as_f64()?))
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        m::check(!self.terminal, "Party transaction retired")?;
        if r["op"] == "begin" {
            m::fields(r, &["op"])?;
            m::check(!self.started, "Party transaction already started")?;
            self.started = true;
        } else {
            let pending = self
                .pending
                .as_ref()
                .ok_or(Error(409, "Party continuation retired"))?;
            m::check(r["op"] == "reply", "Unknown party continuation")?;
            if r["frame"] != pending["frame"] {
                return Err(Error(409, "Stale party continuation"));
            }
            if pending["kind"] == "sql" {
                m::fields(r, &["op", "frame", "rows", "changes"])?;
                let rows = r["rows"]
                    .as_array()
                    .ok_or(Error(400, "Party database rows required"))?;
                m::check(rows.len() <= 128, "Party database result exceeds bounds")?;
                for row in rows {
                    m::check(
                        row.as_array().is_some_and(|v| v.len() <= 32),
                        "Invalid party database row",
                    )?;
                }
                let changes = r["changes"]
                    .as_array()
                    .ok_or(Error(400, "Party database changes required"))?;
                let statements = array(pending, "statements");
                m::check(
                    changes.len() == statements.len(),
                    "Party database result count differs",
                )?;
                for (row, actual) in statements.iter().zip(changes) {
                    m::integer(actual, -1, i64::MAX)?;
                    if !row["changes"].is_null() && row["changes"] != *actual {
                        return Err(Error(409, "Accepted party changed during the transaction"));
                    }
                }
            } else {
                m::fields(r, &["op", "frame", "value"])?;
            }
            let size = pending.to_string().len() + r.to_string().len();
            m::check(
                self.journal.len() < MAX_STEPS && self.bytes + size <= MAX_JOURNAL_BYTES,
                "Party transaction exceeds bounds",
            )?;
            self.bytes += size;
            self.journal.push(Entry {
                request: self.pending.take().unwrap(),
                reply: r.clone(),
            });
        }
        let mut cursor = Cursor {
            journal: &self.journal,
            entropy: &mut self.entropy,
            index: 0,
            source: self.source.is_some(),
            clock: m::real(&self.config["clock"])?,
        };
        match operation(&self.config, &mut cursor) {
            Ok(v) => {
                self.terminal = true;
                let rejection = v.get("_native_rejection").cloned().unwrap_or(Value::Null);
                Ok(
                    json!({"kind":"done","value":if rejection.is_null(){v}else{Value::Null},"rejection":rejection,"atomic":true}),
                )
            }
            Err(FlowError::Yield(v)) => {
                self.pending = Some(v.clone());
                Ok(v)
            }
            Err(FlowError::Failed(e)) => {
                self.terminal = true;
                Err(e)
            }
        }
    }
}
impl Cursor<'_> {
    fn io(&mut self, mut request: Value) -> Flow<Value> {
        need(
            self.index < MAX_STEPS,
            503,
            "Party transaction exceeds bounds",
        )?;
        request["frame"] = json!(self.index + 1);
        request["atomic"] = json!(true);
        if let Some(entry) = self.journal.get(self.index) {
            need(entry.request == request, 409, "Party continuation changed")?;
            self.index += 1;
            Ok(entry.reply.clone())
        } else {
            Err(FlowError::Yield(request))
        }
    }
    fn query(&mut self, sql: &str, params: Value) -> Flow<Vec<Value>> {
        let r = self
            .io(json!({"kind":"sql","mode":"query","statements":[statement(sql,params,None)]}))?;
        Ok(array(&r, "rows").to_vec())
    }
    fn one(&mut self, sql: &str, params: Value) -> Flow<Option<Value>> {
        let rows = self.query(sql, params)?;
        need(rows.len() <= 1, 409, "Party database uniqueness differs")?;
        Ok(rows.into_iter().next())
    }
    fn write(&mut self, sql: &str, params: Value, changes: Option<i64>) -> Flow<i64> {
        let r = self.batch(vec![statement(sql, params, changes)])?;
        Ok(r["changes"][0].as_i64().unwrap())
    }
    fn batch(&mut self, statements: Vec<Value>) -> Flow<Value> {
        need(
            statements.len() <= 2048,
            503,
            "Party SQL batch exceeds bounds",
        )?;
        self.io(json!({"kind":"sql","mode":"batch","statements":statements}))
    }
    fn provider(&mut self, method: &str, args: Value) -> Flow<Value> {
        Ok(self.io(json!({"kind":"provider","method":method,"args":args}))?["value"].clone())
    }
    fn core(&mut self, mut request: Value) -> Flow<Value> {
        need(self.source, 503, "Matchmaking playlists are not configured")?;
        request["now"] = json!(self.clock);
        Ok(self.io(json!({"kind":"core","request":request}))?["value"].clone())
    }
    fn random(&mut self, key: &str, secret: bool) -> Flow<String> {
        if let Some(value) = self.entropy.get(key) {
            return Ok(value.clone());
        }
        need(
            self.entropy.len() < 256,
            503,
            "Party entropy bank exceeds bounds",
        )?;
        let v = if secret {
            let mut bytes = [0; 32];
            getrandom::getrandom(&mut bytes)
                .map_err(|_| Error(503, "Party entropy unavailable"))?;
            let alphabet = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
            let mut out = String::from("h2p1_");
            let mut bits = 0u32;
            let mut count = 0;
            for byte in bytes {
                bits = (bits << 8) | u32::from(byte);
                count += 8;
                while count >= 6 {
                    count -= 6;
                    out.push(alphabet[((bits >> count) & 63) as usize] as char);
                }
            }
            if count > 0 {
                out.push(alphabet[((bits << (6 - count)) & 63) as usize] as char);
            }
            out
        } else {
            id()?
        };
        self.entropy.insert(key.to_owned(), v.clone());
        Ok(v)
    }
    fn cleanup(&mut self, now: i64) -> Flow<()> {
        self.batch(vec![
            statement(
                "DELETE FROM party_invites WHERE expires<=?",
                json!([now]),
                None,
            ),
            statement(
                "DELETE FROM party_requests WHERE expires<=?",
                json!([now]),
                None,
            ),
            statement(
                "DELETE FROM party_activity_links WHERE expires<=?",
                json!([now]),
                None,
            ),
        ])?;
        Ok(())
    }
    fn party(&mut self, who: &str) -> Flow<Option<Value>> {
        self.one("SELECT p.id,p.leader,p.version FROM parties p JOIN party_members m ON m.party=p.id WHERE m.player=?",json!([who]))
    }
    fn party_id(&mut self, pid: &str) -> Flow<Option<Value>> {
        self.one(
            "SELECT id,leader,version FROM parties WHERE id=?",
            json!([pid]),
        )
    }
    fn members(&mut self, pid: &str) -> Flow<Vec<Value>> {
        let rows=self.query("SELECT m.player,m.handle,p.screen_name,m.ready_playlist,m.ready_revision,m.ready_version FROM party_members m JOIN player_profiles p ON p.player=m.player WHERE m.party=? ORDER BY m.rowid LIMIT 17",json!([pid]))?;
        need(rows.len() <= 16, 409, "Invalid accepted party capacity")?;
        Ok(rows)
    }
    fn privacy(&mut self, pid: &str) -> Flow<Value> {
        Ok(self
            .one(
                "SELECT mode,revision FROM party_privacy WHERE party=?",
                json!([pid]),
            )?
            .unwrap_or(json!([1, 1])))
    }
    fn state(&mut self, players: &[Value]) -> Flow<Vec<Value>> {
        need(players.len() <= 16, 400, "Too many party state subjects")?;
        if !self.source {
            return Ok(players
                .iter()
                .map(|p| json!({"player":p,"session":null,"playing":false,"searching":false}))
                .collect());
        }
        let rooms = self.provider("rooms", Value::Null)?;
        let r =
            self.core(json!({"op":"party_directory_context","players":players,"rooms":rooms}))?;
        Ok(r.as_array()
            .ok_or(Error(503, "Native party context required"))?
            .clone())
    }
    fn seat_count(&mut self, who: &str) -> Flow<i64> {
        let state = self.state(&[json!(who)])?;
        Ok(if state[0]["session"].is_null() {
            1
        } else {
            array(&state[0]["session"], "local_players").len() as i64
        })
    }
    fn playing(&mut self, who: &str) -> Flow<bool> {
        Ok(self.state(&[json!(who)])?[0]["playing"] == true)
    }
    fn searching(&mut self, who: &str) -> Flow<bool> {
        Ok(self.state(&[json!(who)])?[0]["searching"] == true)
    }
    fn any_playing(&mut self, members: &[Value]) -> Flow<bool> {
        let who = members.iter().map(|m| m[0].clone()).collect::<Vec<_>>();
        Ok(self.state(&who)?.iter().any(|p| p["playing"] == true))
    }
    fn any_searching(&mut self, members: &[Value]) -> Flow<bool> {
        let who = members.iter().map(|m| m[0].clone()).collect::<Vec<_>>();
        Ok(self.state(&who)?.iter().any(|p| p["searching"] == true))
    }
    fn size(&mut self, members: &[Value]) -> Flow<i64> {
        let who = members.iter().map(|m| m[0].clone()).collect::<Vec<_>>();
        Ok(self
            .state(&who)?
            .iter()
            .map(|p| {
                if p["session"].is_null() {
                    1
                } else {
                    array(&p["session"], "local_players").len() as i64
                }
            })
            .sum())
    }
    fn blocked(&mut self, a: &str, b: &str) -> Flow<bool> {
        Ok(self.one("SELECT 1 FROM player_friend_blocks WHERE (player=? AND target=?) OR (player=? AND target=?) LIMIT 1",json!([a,b,b,a]))?.is_some())
    }
    fn accepted(&mut self, a: &str, b: &str) -> Flow<bool> {
        if self.blocked(a, b)? {
            return Ok(false);
        }
        let (a, b) = if a < b { (a, b) } else { (b, a) };
        Ok(self
            .one(
                "SELECT 1 FROM player_friendships WHERE low=? AND high=? AND accepted=1",
                json!([a, b]),
            )?
            .is_some())
    }
    fn online(&mut self, who: &str) -> Flow<bool> {
        let r = self.provider("online", json!([who]))?;
        Ok(r[0] == true)
    }
    fn facts(&mut self, who: Option<&str>, pid: Option<&str>) -> Flow<Value> {
        let party = if let Some(pid) = pid {
            self.party_id(pid)?
        } else if let Some(who) = who {
            self.party(who)?
        } else {
            None
        };
        let mut records = Vec::new();
        let mut clans = Vec::new();
        let mut owners = Vec::new();
        if let Some(p) = &party {
            for row in self.members(txt(&p[0])?)? {
                owners.push(row[0].clone());
                let consent=self.one("SELECT party,version,handle,playlist,revision,profile,session_key,generation,roster FROM matchmaking_party_consents WHERE player=?",json!([row[0]]))?;
                let consent=consent.map(|v|json!({"party":v[0],"version":v[1],"handle":v[2],"playlist":v[3],"revision":v[4],"profile":v[5],"session_key":v[6],"generation":v[7],"roster":v[8]}));
                records.push(json!({"player":row[0],"handle":row[1],"ready_playlist":row[3],"ready_revision":row[4],"ready_version":row[5],"consent":consent}));
            }
        } else if let Some(who) = who {
            owners.push(json!(who));
        }
        if self.source {
            let seats = self.core(json!({"op":"session_directory_seats","owners":owners}))?;
            for player in seats
                .as_array()
                .ok_or(Error(503, "Native party seats required"))?
            {
                let row = self.one(
                    "SELECT clan FROM clan_members WHERE player=?",
                    json!([player]),
                )?;
                clans.push(json!({"player":player,"clan":row.map(|r|r[0].clone())}));
            }
        }
        Ok(
            json!({"party":party.map(|p|json!({"id":p[0],"leader":p[1],"version":p[2],"members":records})),"clans":clans}),
        )
    }
    fn plan(&mut self, plan: Value) -> Flow<()> {
        self.batch(array(&plan, "statements").to_vec())?;
        if self.source {
            self.core(json!({"op":"party_effects","effects":plan["effects"]}))?;
        }
        Ok(())
    }
    fn changed(&mut self, pid: &str, admitted: bool) -> Flow<()> {
        let facts = self.facts(None, Some(pid))?;
        let admitted = if admitted {
            let members = self.members(pid)?;
            self.size(&members)? < 16
        } else {
            false
        };
        self.plan(value(
            &json!({"op":"party_change_plan","party":pid,"facts":facts,"admitted":admitted}),
        )?)
    }
    fn cancel_queue(&mut self, who: &str) -> Flow<()> {
        if self.source {
            self.core(json!({"op":"party_directory_cancel_queue","player":who}))?;
        }
        Ok(())
    }
    fn admit(&mut self, who: &str, pid: &str) -> Flow<()> {
        self.cancel_queue(who)?;
        let handle = self.random("member", false)?;
        self.write(
            "INSERT INTO party_members(player,party,handle) VALUES(?,?,?)",
            json!([who, pid, handle]),
            Some(1),
        )?;
        self.write(
            "DELETE FROM party_invites WHERE target=?",
            json!([who]),
            None,
        )?;
        self.changed(pid, true)
    }
    fn admission(&mut self, pid: &str, who: &str, invited: bool) -> Flow<()> {
        let privacy = self.privacy(pid)?;
        let mode = m::integer(&privacy[0], 0, 2)? as i32;
        let members = self.members(pid)?;
        let available = (16 - self.size(&members)?).max(0) as i32;
        let count = self.seat_count(who)? as i32;
        need(
            (1..=4).contains(&count),
            409,
            "Invalid admitted local seats",
        )?;
        let status = crate::party_privacy::admission(
            mode,
            invited,
            count,
            if mode == 0 { available } else { 0 },
            if mode == 1 { available } else { 0 },
        );
        need(
            status != 8,
            403,
            if mode == 2 {
                "Party is closed"
            } else {
                "A party invitation is required"
            },
        )?;
        need(status != 7 && members.len() < 16, 409, "Party is full")
    }
}
const SCHEMA: &str = r#"
CREATE TABLE IF NOT EXISTS parties(id TEXT PRIMARY KEY,leader TEXT NOT NULL REFERENCES players(id),version INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS party_members(player TEXT PRIMARY KEY REFERENCES players(id),party TEXT NOT NULL REFERENCES parties(id) ON DELETE CASCADE,handle TEXT NOT NULL UNIQUE,ready_playlist TEXT NOT NULL DEFAULT '',ready_revision TEXT NOT NULL DEFAULT '',ready_version INTEGER NOT NULL DEFAULT 0);
CREATE INDEX IF NOT EXISTS party_roster ON party_members(party);
CREATE TABLE IF NOT EXISTS party_invites(id TEXT PRIMARY KEY,party TEXT NOT NULL REFERENCES parties(id) ON DELETE CASCADE,target TEXT NOT NULL REFERENCES players(id),inviter TEXT NOT NULL REFERENCES players(id),expires INTEGER NOT NULL,UNIQUE(party,target));
CREATE TABLE IF NOT EXISTS party_blocks(player TEXT NOT NULL REFERENCES players(id),inviter TEXT NOT NULL REFERENCES players(id),PRIMARY KEY(player,inviter));
CREATE TABLE IF NOT EXISTS party_requests(player TEXT NOT NULL REFERENCES players(id),request TEXT NOT NULL,payload TEXT NOT NULL,expires INTEGER NOT NULL,PRIMARY KEY(player,request));
CREATE TABLE IF NOT EXISTS party_activity_links(party TEXT PRIMARY KEY REFERENCES parties(id) ON DELETE CASCADE,version INTEGER NOT NULL,alias TEXT NOT NULL UNIQUE,secret TEXT NOT NULL UNIQUE,expires INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS party_privacy(party TEXT PRIMARY KEY REFERENCES parties(id) ON DELETE CASCADE,mode INTEGER NOT NULL CHECK(mode IN (0,1,2)),revision INTEGER NOT NULL CHECK(revision>0));
"#;
fn schema(c: &mut Cursor<'_>) -> Flow<Value> {
    c.batch(
        SCHEMA
            .split(';')
            .map(str::trim)
            .filter(|s| !s.is_empty())
            .map(|s| statement(s, json!([]), None))
            .collect(),
    )?;
    let columns = c
        .query("PRAGMA table_info(matchmaking_party_consents)", json!([]))?
        .iter()
        .map(|r| r[1].clone())
        .collect::<Vec<_>>();
    c.plan(value(
        &json!({"op":"party_consent_schema","columns":columns}),
    )?)?;
    c.write(
        "INSERT OR IGNORE INTO party_privacy SELECT id,1,1 FROM parties",
        json!([]),
        None,
    )?;
    Ok(Value::Null)
}
fn platform<'a>(v: &'a Value, min: usize, max: usize, message: &'static str) -> Result<&'a str> {
    let v = m::text(v, max)?;
    if !(min..=max).contains(&v.len())
        || !matches!(v.as_bytes().first(), Some(b'1'..=b'9'))
        || !v.bytes().all(|b| b.is_ascii_digit())
    {
        return Err(Error(400, message));
    }
    Ok(v)
}
fn friend_joinable(c: &mut Cursor<'_>, who: &str, target: &str) -> Flow<bool> {
    if who == target || !c.accepted(who, target)? || !c.online(target)? {
        return Ok(false);
    }
    let target_party = c.party(target)?;
    if target_party.is_none() || c.party(who)?.is_some() || c.playing(who)? || c.searching(who)? {
        return Ok(false);
    }
    let party = target_party.unwrap();
    let pid = txt(&party[0])?;
    let leader = txt(&party[1])?;
    if c.blocked(who, leader)?
        || c.one(
            "SELECT 1 FROM party_blocks WHERE player=? AND inviter IN (?,?) LIMIT 1",
            json!([who, target, leader]),
        )?
        .is_some()
    {
        return Ok(false);
    }
    let members = c.members(pid)?;
    if c.any_playing(&members)? || c.any_searching(&members)? {
        return Ok(false);
    }
    let mode = m::integer(&c.privacy(pid)?[0], 0, 2)? as i32;
    let available = (16 - c.size(&members)?).max(0) as i32;
    let count = c.seat_count(who)? as i32;
    Ok(members.len() < 16
        && crate::party_privacy::admission(
            mode,
            false,
            count,
            if mode == 0 { available } else { 0 },
            if mode == 1 { available } else { 0 },
        ) == 0)
}
fn activity(c: &mut Cursor<'_>, who: &str, body: &Value, now: i64) -> Flow<Value> {
    m::fields_optional(body, &["version"], &["join_secret"])?;
    let expected = m::integer(&body["version"], 1, i64::MAX)?;
    let previous = if body.get("join_secret").is_some() {
        Some(secret(&body["join_secret"])?)
    } else {
        None
    };
    let party = c
        .party(who)?
        .ok_or(Error(409, "Create or join a party first"))?;
    let pid = txt(&party[0])?;
    need(party[1] == who, 403, "Only the party leader can do that")?;
    need(party[2] == expected, 409, "Party changed; refresh it")?;
    let members = c.members(pid)?;
    let size = c.size(&members)?;
    if size >= 16 {
        // Revocation commits before reporting a full party. It must not be
        // rolled back into a still usable activity capability.
        c.write(
            "DELETE FROM party_activity_links WHERE party=?",
            json!([pid]),
            None,
        )?;
        return Ok(json!({"_native_rejection":{"status":409,"message":"Party is full"}}));
    }
    need(!c.any_playing(&members)?, 409, "Party is in a match")?;
    let mut link = c.one(
        "SELECT version,alias,secret,expires FROM party_activity_links WHERE party=?",
        json!([pid]),
    )?;
    if let Some(previous) = previous {
        need(
            link.as_ref().is_some_and(|r| {
                r[0] == expected && r[2] == previous && r[3].as_i64().is_some_and(|v| v > now)
            }),
            404,
            "Party invitation unavailable",
        )?;
    }
    if link
        .as_ref()
        .is_none_or(|r| r[0] != expected || r[3].as_i64().is_none_or(|v| v <= now))
    {
        link = Some(json!([
            expected,
            c.random("alias", false)?,
            c.random("secret", true)?,
            now + 600
        ]));
        let row = link.as_ref().unwrap();
        c.write(
            "INSERT OR REPLACE INTO party_activity_links VALUES(?,?,?,?,?)",
            json!([pid, row[0], row[1], row[2], row[3]]),
            Some(1),
        )?;
    }
    let row = link.unwrap();
    Ok(
        json!({"party_id":row[1],"version":expected,"current_size":size,"max_size":16,"join_secret":row[2],"expires_in":m::integer(&row[3],0,i64::MAX)?-now}),
    )
}
fn target(c: &mut Cursor<'_>, public: &Value) -> Flow<String> {
    let public = uuid(public)?;
    let row = c
        .one(
            "SELECT player FROM history_players WHERE public_id=?",
            json!([public]),
        )?
        .ok_or(Error(404, "Player not found"))?;
    Ok(txt(&row[0])?.to_owned())
}
fn invite(c: &mut Cursor<'_>, pid: &str, who: &str, target: &str, now: i64) -> Flow<()> {
    need(target != who, 400, "Cannot invite yourself")?;
    need(
        !c.blocked(who, target)?,
        403,
        "Party invitation unavailable",
    )?;
    need(
        c.one(
            "SELECT 1 FROM party_members WHERE party=? AND player=?",
            json!([pid, target]),
        )?
        .is_none(),
        409,
        "Player is already in this party",
    )?;
    need(
        c.one(
            "SELECT 1 FROM party_blocks WHERE player=? AND inviter=?",
            json!([target, who]),
        )?
        .is_none(),
        403,
        "Player does not accept these invitations",
    )?;
    let outgoing = c
        .one(
            "SELECT count(*) FROM party_invites WHERE party=?",
            json!([pid]),
        )?
        .unwrap();
    let incoming = c
        .one(
            "SELECT count(*) FROM party_invites WHERE target=?",
            json!([target]),
        )?
        .unwrap();
    need(
        m::integer(&outgoing[0], 0, i64::MAX)? < 100
            && m::integer(&incoming[0], 0, i64::MAX)? < 100,
        429,
        "Party invitation capacity reached",
    )?;
    let iid = c.random(&format!("invite:{target}"), false)?;
    c.write(
        "INSERT INTO party_invites VALUES(?,?,?,?,?) ON CONFLICT(party,target) DO NOTHING",
        json!([iid, pid, target, who, now + 600]),
        None,
    )?;
    Ok(())
}
fn accept(
    c: &mut Cursor<'_>,
    who: &str,
    current_party: Option<&Value>,
    iid: &str,
    expected: i64,
    activity: bool,
    now: i64,
) -> Flow<()> {
    let invitation = c
        .one(
            "SELECT party,inviter FROM party_invites WHERE id=? AND target=? AND expires>?",
            json!([iid, who, now]),
        )?
        .ok_or(Error(404, "Party invitation not found"))?;
    let pid = txt(&invitation[0])?;
    let inviter = txt(&invitation[1])?;
    need(
        !c.blocked(who, inviter)?,
        403,
        "Party invitation unavailable",
    )?;
    need(
        current_party.is_none() && !c.playing(who)?,
        409,
        "Leave the current party or match first",
    )?;
    let party = c.party_id(pid)?;
    need(
        party.as_ref().is_some_and(|r| r[2] == expected),
        409,
        "Party changed; refresh invitations",
    )?;
    let party = party.unwrap();
    need(
        c.one(
            "SELECT 1 FROM party_blocks WHERE player=? AND inviter=?",
            json!([who, inviter]),
        )?
        .is_none(),
        403,
        "Party invitation unavailable",
    )?;
    if activity {
        let allowed = c.provider("voice_allowed", json!([[who, inviter], [who, party[1]]]))?;
        need(
            allowed[0] == true && allowed[1] == true,
            403,
            "Party invitation unavailable",
        )?;
    }
    let members = c.members(pid)?;
    c.admission(pid, who, true)?;
    need(!c.any_playing(&members)?, 409, "Party is in a match")?;
    c.admit(who, pid)
}
fn command(c: &mut Cursor<'_>, who: &str, body: &Value, steam: &Value, now: i64) -> Flow<()> {
    let action = command_body(body)?;
    let rid = uuid(&body["request_id"])?;
    let payload = ascii_json(body);
    if let Some(old) = c.one(
        "SELECT payload FROM party_requests WHERE player=? AND request=?",
        json!([who, rid]),
    )? {
        need(
            old[0] == payload,
            409,
            "Party request ID already used differently",
        )?;
        return Ok(());
    }
    let count = c
        .one(
            "SELECT count(*) FROM party_requests WHERE player=?",
            json!([who]),
        )?
        .unwrap();
    need(
        m::integer(&count[0], 0, i64::MAX)? < 256,
        429,
        "Party command limit reached",
    )?;
    let party = c.party(who)?;
    match action {
        "create" => {
            need(
                party.is_none() && !c.playing(who)?,
                409,
                "Leave the current party or match first",
            )?;
            let count = c.one("SELECT count(*) FROM parties", json!([]))?.unwrap();
            need(
                m::integer(&count[0], 0, i64::MAX)? < 1000,
                503,
                "Party capacity reached",
            )?;
            need(
                crate::party_privacy::mode(0) == 0,
                503,
                "Invalid native party privacy mode",
            )?;
            let pid = c.random("party", false)?;
            let handle = c.random("member", false)?;
            c.batch(vec![
                statement(
                    "INSERT INTO parties VALUES(?,?,1)",
                    json!([pid, who]),
                    Some(1),
                ),
                statement(
                    "INSERT INTO party_privacy VALUES(?,0,1)",
                    json!([pid]),
                    Some(1),
                ),
                statement(
                    "INSERT INTO party_members(player,party,handle) VALUES(?,?,?)",
                    json!([who, pid, handle]),
                    Some(1),
                ),
            ])?;
            c.cancel_queue(who)?;
        }
        "join_friend" => {
            let target = target(c, &body["target"])?;
            need(
                c.accepted(who, &target)?,
                403,
                "Friendship no longer available",
            )?;
            need(
                party.is_none() && !c.playing(who)?,
                409,
                "Leave the current party or match first",
            )?;
            need(
                !c.searching(who)?,
                409,
                "Cancel matchmaking before joining a party",
            )?;
            need(c.online(&target)?, 409, "Friend is offline")?;
            let p = c.party(&target)?.ok_or(Error(404, "Friend has no party"))?;
            let pid = txt(&p[0])?;
            let leader = txt(&p[1])?;
            let context = c.one(
                "SELECT handle FROM party_members WHERE player=? AND party=?",
                json!([target, pid]),
            )?;
            let expected = uuid(&body["target_context"])?;
            need(
                context.as_ref().is_some_and(|r| r[0] == expected),
                409,
                "Friend's party changed; refresh it",
            )?;
            need(
                !c.blocked(who, leader)?
                    && c.one(
                        "SELECT 1 FROM party_blocks WHERE player=? AND inviter IN (?,?) LIMIT 1",
                        json!([who, target, leader]),
                    )?
                    .is_none(),
                403,
                "Party join unavailable",
            )?;
            c.admission(pid, who, false)?;
            let members = c.members(pid)?;
            need(!c.any_playing(&members)?, 409, "Party is in a match")?;
            need(!c.any_searching(&members)?, 409, "Party is matchmaking")?;
            c.admit(who, pid)?;
        }
        "join_activity" => {
            let secret = secret(&body["join_secret"])?;
            let link = c
                .one(
                    "SELECT party,version FROM party_activity_links WHERE secret=? AND expires>?",
                    json!([secret, now]),
                )?
                .ok_or(Error(404, "Party invitation unavailable"))?;
            let invitation = c
                .one(
                    "SELECT id FROM party_invites WHERE party=? AND target=?",
                    json!([link[0], who]),
                )?
                .ok_or(Error(404, "Party invitation unavailable"))?;
            accept(
                c,
                who,
                party.as_ref(),
                txt(&invitation[0])?,
                m::integer(&link[1], 1, i64::MAX)?,
                true,
                now,
            )?;
        }
        "accept" | "decline" | "block" => {
            let iid = uuid(&body["target"])?;
            let invitation = c
                .one(
                    "SELECT party,inviter FROM party_invites WHERE id=? AND target=?",
                    json!([iid, who]),
                )?
                .ok_or(Error(404, "Party invitation not found"))?;
            if action == "accept" {
                accept(
                    c,
                    who,
                    party.as_ref(),
                    iid,
                    m::integer(&body["version"], 1, i64::MAX)?,
                    false,
                    now,
                )?;
            } else if action == "block" {
                c.write(
                    "INSERT OR IGNORE INTO party_blocks VALUES(?,?)",
                    json!([who, invitation[1]]),
                    None,
                )?;
            }
            c.write("DELETE FROM party_invites WHERE id=?", json!([iid]), None)?;
        }
        _ => {
            let party = party.ok_or(Error(409, "Create or join a party first"))?;
            let pid = txt(&party[0])?;
            let leader = txt(&party[1])?;
            need(
                party[2] == m::integer(&body["version"], 1, i64::MAX)?,
                409,
                "Party changed; refresh it",
            )?;
            let members = c.members(pid)?;
            need(
                !c.any_playing(&members)?,
                409,
                "Leave the current match first",
            )?;
            if ![
                "leave",
                "leave_lobby",
                "ready",
                "unready",
                "countdown_request",
            ]
            .contains(&action)
            {
                need(leader == who, 403, "Only the party leader can do that")?;
            }
            match action {
                "privacy" => {
                    need(
                        crate::party_privacy::can_change(true, leader == who),
                        403,
                        "Only the party leader can do that",
                    )?;
                    let context = c.one(
                        "SELECT handle FROM party_members WHERE party=? AND player=?",
                        json!([pid, who]),
                    )?;
                    let expected = uuid(&body["privacy_context"])?;
                    need(
                        context.as_ref().is_some_and(|r| r[0] == expected),
                        409,
                        "Party changed; refresh it",
                    )?;
                    let mode = m::integer(&body["privacy"], 0, 2)?;
                    let privacy = c.privacy(pid)?;
                    need(
                        privacy[1] == m::integer(&body["privacy_revision"], 1, i64::MAX)?,
                        409,
                        "Party privacy changed; refresh it",
                    )?;
                    let revision = m::integer(&privacy[1], 1, i64::MAX)?;
                    need(revision < i64::MAX, 503, "Party privacy revision exhausted")?;
                    c.write("INSERT INTO party_privacy VALUES(?,?,?) ON CONFLICT(party) DO UPDATE SET mode=excluded.mode,revision=excluded.revision",json!([pid,mode,revision+1]),Some(1))?;
                }
                "invite" => {
                    let name = m::text(&body["username"], 73)?;
                    let target = if name.contains('#') {
                        let r = c.provider("friend_tag", json!(name))?;
                        txt(&r)?.to_owned()
                    } else {
                        let name = name.to_lowercase();
                        need(
                            (2..=32).contains(&name.len())
                                && name.bytes().all(|b| {
                                    b.is_ascii_lowercase()
                                        || b.is_ascii_digit()
                                        || b"_.".contains(&b)
                                }),
                            400,
                            "Enter a friend tag",
                        )?;
                        let row = c
                            .one(
                                "SELECT player FROM player_profiles WHERE discord_username=?",
                                json!([name]),
                            )?
                            .ok_or(Error(404, "Player must sign into ONLINE first"))?;
                        txt(&row[0])?.to_owned()
                    };
                    invite(c, pid, who, &target, now)?;
                }
                "invite_friend" => {
                    let target = target(c, &body["target"])?;
                    need(
                        c.accepted(who, &target)?,
                        403,
                        "Friendship no longer available",
                    )?;
                    invite(c, pid, who, &target, now)?;
                }
                "invite_discord" => {
                    let subject = platform(
                        &body["discord_id"],
                        17,
                        20,
                        "Invalid Discord invitation target",
                    )?;
                    let row=c.one("SELECT id FROM players WHERE provider='discord' AND subject=? UNION ALL SELECT player FROM player_identities WHERE provider='discord' AND subject=? LIMIT 1",json!([subject,subject]))?.ok_or(Error(404,"Player must sign into ONLINE first"))?;
                    invite(c, pid, who, txt(&row[0])?, now)?;
                }
                "invite_steam" => {
                    let subject =
                        platform(&body["steam_id"], 17, 17, "Invalid Steam invitation target")?;
                    need(
                        steam["state"] == "ready"
                            && array(steam, "friends").iter().any(|r| r["id"] == subject),
                        403,
                        "Refresh Steam friends before inviting this player",
                    )?;
                    let row=c.one("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",json!([subject]))?.ok_or(Error(404,"Player must sign into ONLINE first"))?;
                    need(
                        c.one(
                            "SELECT 1 FROM party_blocks WHERE player=? AND inviter=?",
                            json!([who, row[0]]),
                        )?
                        .is_none(),
                        403,
                        "Player is blocked",
                    )?;
                    invite(c, pid, who, txt(&row[0])?, now)?;
                }
                "invite_member" => {
                    let target = uuid(&body["target"])?;
                    let clan = c.one(
                        "SELECT clan,role FROM clan_members WHERE player=?",
                        json!([who]),
                    )?;
                    let member = if let Some(clan) = clan {
                        c.one(
                            "SELECT 1 FROM clan_members WHERE clan=? AND player=?",
                            json!([clan[0], target]),
                        )?
                        .is_some()
                    } else {
                        false
                    };
                    need(member, 403, "Player is no longer in your clan")?;
                    need(c.online(target)?, 409, "Clan member is offline")?;
                    invite(c, pid, who, target, now)?;
                }
                "invite_clan" => {
                    let clan = c.one(
                        "SELECT clan,role FROM clan_members WHERE player=?",
                        json!([who]),
                    )?;
                    need(
                        clan.as_ref()
                            .is_some_and(|r| r[1].as_i64().is_some_and(|v| v >= 1)),
                        403,
                        "Clan recruiting permission required",
                    )?;
                    let clan = clan.unwrap();
                    let mut targets = Vec::new();
                    for row in c.query(
                        "SELECT player FROM clan_members WHERE clan=? LIMIT 129",
                        json!([clan[0]]),
                    )? {
                        let target = txt(&row[0])?;
                        if !members.iter().any(|r| r[0] == target)
                            && c.online(target)?
                            && c.one(
                                "SELECT 1 FROM party_blocks WHERE player=? AND inviter=?",
                                json!([target, who]),
                            )?
                            .is_none()
                        {
                            targets.push(target.to_owned());
                        }
                    }
                    need(
                        !targets.is_empty(),
                        409,
                        "No eligible online clan members to invite",
                    )?;
                    for target in targets {
                        invite(c, pid, who, &target, now)?;
                    }
                }
                "cancel" => {
                    let count = c.write(
                        "DELETE FROM party_invites WHERE id=? AND party=?",
                        json!([uuid(&body["target"])?, pid]),
                        None,
                    )?;
                    need(count != 0, 404, "Party invitation not found")?;
                }
                "leave" | "leave_lobby" | "remove" | "transfer" => {
                    let target = if ["leave", "leave_lobby"].contains(&action) {
                        who.to_owned()
                    } else {
                        let row = c
                            .one(
                                "SELECT player FROM party_members WHERE party=? AND handle=?",
                                json!([pid, uuid(&body["target"])?]),
                            )?
                            .ok_or(Error(404, "Party member not found"))?;
                        let target = txt(&row[0])?.to_owned();
                        need(target != who, 400, "Use Leave Party for yourself")?;
                        target
                    };
                    if action == "transfer" {
                        c.write(
                            "UPDATE parties SET leader=? WHERE id=?",
                            json!([target, pid]),
                            Some(1),
                        )?;
                    } else if action == "leave_lobby" && who == leader && members.len() == 2 {
                        c.write("DELETE FROM parties WHERE id=?", json!([pid]), Some(1))?;
                    } else {
                        c.write(
                            "DELETE FROM party_members WHERE player=?",
                            json!([target]),
                            Some(1),
                        )?;
                        let remaining = c.members(pid)?;
                        if remaining.is_empty() {
                            c.write("DELETE FROM parties WHERE id=?", json!([pid]), Some(1))?;
                        } else if target == leader {
                            c.write(
                                "UPDATE parties SET leader=? WHERE id=?",
                                json!([remaining[0][0], pid]),
                                Some(1),
                            )?;
                        }
                    }
                    c.changed(pid, false)?;
                }
                "ready" => {
                    let facts = c.facts(Some(who), None)?;
                    let rooms = c.provider("rooms", Value::Null)?;
                    let plan=c.core(json!({"op":"party_ready","player":who,"playlist":body["playlist_id"],"revision":body["revision"],"version":body["version"],"facts":facts,"rooms":rooms}))?;
                    c.plan(plan)?;
                }
                "unready" => c.changed(pid, false)?,
                _ => return Err(Error(400, "Invalid party command route").into()),
            }
        }
    }
    c.write(
        "INSERT INTO party_requests VALUES(?,?,?,?)",
        json!([who, rid, payload, now + 86400]),
        Some(1),
    )?;
    Ok(())
}
fn profile(c: &mut Cursor<'_>, player: &str, session: Option<&Value>, guest: bool) -> Flow<Value> {
    if let Some(session) = session {
        if let Some(seat) = array(session, "local_players")
            .iter()
            .find(|s| s["player"] == player)
        {
            if !seat["appearance"].is_null() {
                return Ok(seat["appearance"].clone());
            }
        }
    }
    if guest {
        return Ok(Value::Null);
    }
    Ok(c.one(
        "SELECT lower(hex(appearance)) FROM player_customization WHERE player=?",
        json!([player]),
    )?
    .map(|r| r[0].clone())
    .unwrap_or(Value::Null))
}
fn view(c: &mut Cursor<'_>, who: &str, flags: &Value) -> Flow<Value> {
    let party = c.party(who)?;
    let mut result = Value::Null;
    if let Some(p) = party {
        let pid = txt(&p[0])?;
        let leader = txt(&p[1])?;
        let members = c.members(pid)?;
        let players = members.iter().map(|r| r[0].clone()).collect::<Vec<_>>();
        let online = c.provider("online", json!(players))?;
        need(
            online
                .as_array()
                .is_some_and(|r| r.len() == members.len() && r.iter().all(Value::is_boolean)),
            503,
            "Invalid party presence observation",
        )?;
        let ready = if c.source {
            let facts = c.facts(Some(who), None)?;
            c.core(json!({"op":"party_ready_view","facts":facts}))?
        } else {
            json!(vec![""; members.len()])
        };
        let reviewing = if flags["postgame"] == true {
            c.provider("room_connections", Value::Null)?
        } else {
            json!([])
        };
        need(
            reviewing.as_array().is_some_and(|r| r.len() <= 64),
            503,
            "Too many party room observations",
        )?;
        for room in reviewing.as_array().unwrap() {
            m::fields(room, &["closed", "phase", "peers"])?;
            m::boolean(&room["closed"])?;
            m::text(&room["phase"], 32)?;
            let peers = room["peers"]
                .as_array()
                .ok_or(Error(400, "Party room peers required"))?;
            need(peers.len() <= 16, 400, "Too many party room peers")?;
            for peer in peers {
                m::fields(peer, &["player", "closed"])?;
                m::text(&peer["player"], 64)?;
                m::boolean(&peer["closed"])?;
            }
        }
        let mut rows = Vec::new();
        let states = if flags["appearance"] == true || flags["local_players"] == true {
            c.state(&players)?
        } else {
            Vec::new()
        };
        for (index, stored) in members.iter().enumerate() {
            let player = txt(&stored[0])?;
            let mut row = json!({"handle":stored[1],"name":stored[2],"self":player==who,"leader":player==leader,"online":online[index],"ready_playlist":ready[index]});
            if flags["postgame"] == true {
                row["postgame"] = json!(reviewing.as_array().unwrap().iter().any(|r| {
                    r["closed"] == false
                        && ["reviewing", "postgame"].iter().any(|s| r["phase"] == *s)
                        && array(r, "peers")
                            .iter()
                            .any(|p| p["player"] == player && p["closed"] == false)
                }));
            }
            let session = states
                .get(index)
                .filter(|r| !r["session"].is_null())
                .map(|r| &r["session"]);
            if flags["appearance"] == true {
                row["appearance"] = profile(c, player, session, false)?;
            }
            if flags["local_players"] == true {
                if let Some(session) = session {
                    let seats = array(session, "local_players");
                    if seats.len() > 1 {
                        let mut locals = Vec::new();
                        for seat in &seats[1..] {
                            let mut local =
                                json!({"name":seat["screen_name"],"guest":seat["guest"]});
                            if flags["appearance"] == true {
                                local["appearance"] = profile(
                                    c,
                                    txt(&seat["player"])?,
                                    Some(session),
                                    seat["guest"] == true,
                                )?;
                            }
                            locals.push(local);
                        }
                        row["local_players"] = json!(locals);
                    }
                }
            }
            rows.push(row);
        }
        let mut outgoing = Vec::new();
        if leader == who {
            for r in c.query("SELECT i.id,p.screen_name FROM party_invites i JOIN player_profiles p ON p.player=i.target WHERE i.party=? ORDER BY i.expires,i.id LIMIT 101",json!([pid]))?{outgoing.push(json!({"id":r[0],"name":r[1]}));}
        }
        result = json!({"version":p[2],"leader":leader==who,"members":rows,"outgoing":outgoing});
        if flags["countdown"] == true {
            let countdown = if c.source {
                let facts = c.facts(Some(who), None)?;
                c.core(json!({"op":"party_countdown_view","player":who,"facts":facts}))?["countdown"].clone()
            } else {
                Value::Null
            };
            result["countdown"] = countdown;
        }
        if flags["privacy"] == true {
            let privacy = c.privacy(pid)?;
            let mode = m::integer(&privacy[0], 0, 2)?;
            let handle = members
                .iter()
                .find(|r| r[0] == who)
                .ok_or(Error(409, "Accepted party changed"))?[1]
                .clone();
            result["privacy"] = json!({"mode":crate::party_privacy::mode(mode as i32),"revision":privacy[1],"context":handle});
        }
    }
    let incoming=c.query("SELECT i.id,p.screen_name,g.version FROM party_invites i JOIN parties g ON g.id=i.party JOIN player_profiles p ON p.player=g.leader WHERE i.target=? ORDER BY i.expires,i.id LIMIT 100",json!([who]))?;
    Ok(
        json!({"party":result,"invitations":incoming.iter().map(|r|json!({"id":r[0],"name":r[1],"version":r[2]})).collect::<Vec<_>>()}),
    )
}
fn txt(v: &Value) -> Result<&str> {
    m::text(v, 64)
}
fn uuid(v: &Value) -> Result<&str> {
    let v = m::text(v, 36)?;
    let b = v.as_bytes();
    m::check(
        b.len() == 36
            && b.iter().enumerate().all(|(i, c)| {
                if [8, 13, 18, 23].contains(&i) {
                    *c == b'-'
                } else {
                    c.is_ascii_digit() || (b'a'..=b'f').contains(c)
                }
            }),
        "Invalid UUID",
    )?;
    Ok(v)
}
fn secret(v: &Value) -> Result<&str> {
    let v = m::text(v, 48)?;
    m::check(
        v.len() == 48
            && v.starts_with("h2p1_")
            && v.as_bytes()[5..]
                .iter()
                .all(|c| c.is_ascii_alphanumeric() || *c == b'_' || *c == b'-'),
        "Invalid party join link",
    )?;
    Ok(v)
}
fn ascii_json(v: &Value) -> String {
    let mut out = String::new();
    for ch in v.to_string().chars() {
        if ch.is_ascii() {
            out.push(ch);
        } else {
            let mut units = [0; 2];
            for u in ch.encode_utf16(&mut units) {
                out.push_str(&format!("\\u{u:04x}"));
            }
        }
    }
    out
}
fn command_body(v: &Value) -> Result<&str> {
    let action = m::text(&v["action"], 64)?;
    uuid(&v["request_id"])?;
    let options: &[&str] = match action {
        "create" => &[],
        "invite" => &["version", "username"],
        "invite_friend" | "invite_member" | "accept" | "cancel" | "remove" | "transfer" => {
            &["version", "target"]
        }
        "invite_discord" => &["version", "discord_id"],
        "invite_steam" => &["version", "steam_id"],
        "invite_clan" | "leave" | "leave_lobby" | "unready" | "countdown_cancel"
        | "countdown_request" => &["version"],
        "decline" | "block" => &["target"],
        "ready" | "countdown_start" => &["version", "playlist_id", "revision"],
        "privacy" => &["version", "privacy", "privacy_revision", "privacy_context"],
        "join_friend" => &["target", "target_context"],
        "join_activity" => &["join_secret"],
        _ => return Err(Error(400, "Invalid party command")),
    };
    let mut fields = vec!["action", "request_id"];
    fields.extend(options);
    m::fields(v, &fields)?;
    Ok(action)
}
fn features(v: &Value, command: bool) -> Value {
    let enabled = |key: &str| v[key] == true || v[key] == "1";
    json!({"local_players":!command && enabled("local_players"),"appearance":!command && enabled("appearance"),"postgame":!command && enabled("postgame"),"countdown":enabled("countdown"),"privacy":enabled("privacy")})
}
fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    if r["family"]=="clan"{return clans::operation(r,c);}
    let mode = txt(&r["mode"])?;
    let now = m::integer(&r["wall"], 0, 9007199254650000)?;
    if mode == "schema" {
        return schema(c);
    }
    if mode == "cleanup" {
        c.cleanup(now)?;
        return Ok(Value::Null);
    }
    let who = if r["player"].is_null() {
        ""
    } else {
        txt(&r["player"])?
    };
    match mode {
        "party" => Ok(c.party(who)?.unwrap_or(Value::Null)),
        "members" => Ok(json!(c.members(txt(&r["party"])?)?)),
        "privacy_state" => c.privacy(txt(&r["party"])?),
        "facts" => c.facts((!who.is_empty()).then_some(who), r["party"].as_str()),
        "changed" => {
            c.changed(txt(&r["party"])?, r["admitted"] == true)?;
            Ok(Value::Null)
        }
        "session_lost" => {
            if let Some(p) = c.party(who)? {
                c.changed(txt(&p[0])?, false)?;
            }
            Ok(Value::Null)
        }
        "friend_joinable" => Ok(json!(friend_joinable(c, who, txt(&r["target"])?)?)),
        "friend_context" => {
            let target = txt(&r["target"])?;
            if who == target || !c.accepted(who, target)? || !c.online(target)? {
                return Ok(Value::Null);
            }
            Ok(c.one(
                "SELECT handle FROM party_members WHERE player=?",
                json!([target]),
            )?
            .map(|r| r[0].clone())
            .unwrap_or(Value::Null))
        }
        "activity" => {
            c.cleanup(now)?;
            activity(c, who, &r["body"], now)
        }
        "view" => view(c, who, &features(&r["features"], false)),
        "get" => {
            c.cleanup(now)?;
            view(c, who, &features(&r["features"], false))
        }
        "command_probe" => {
            let body = &r["body"];
            let action = command_body(body)?;
            c.cleanup(now)?;
            let flags = features(&r["features"], true);
            if action.starts_with("countdown_") {
                return Ok(json!({"route":"countdown","body":body,"features":flags}));
            }
            if let Some(old) = c.one(
                "SELECT payload FROM party_requests WHERE player=? AND request=?",
                json!([who, body["request_id"]]),
            )? {
                need(
                    old[0] == ascii_json(body),
                    409,
                    "Party request ID already used differently",
                )?;
                return Ok(json!({"route":"view","features":flags}));
            }
            let subject = if action == "invite_steam" {
                Some(platform(
                    &body["steam_id"],
                    17,
                    17,
                    "Invalid Steam invitation target",
                )?)
            } else {
                None
            };
            Ok(
                json!({"route":if subject.is_some(){"steam"}else{"command"},"subject":subject,"features":flags}),
            )
        }
        "command" => {
            command(c, who, &r["body"], &r["steam"], now)?;
            Ok(Value::Null)
        }
        _ => Err(Error(400, "Unknown party directory operation").into()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn owner(mode: &str) -> Directory {
        Directory::new(&json!({"mode":mode,"source":null,"player":"p","wall":100,"clock":10.}))
            .unwrap()
    }
    #[test]
    fn sql_continuations_are_single_use_and_malformed_replies_cannot_advance() {
        let mut d = owner("cleanup");
        let first = d.execute(&json!({"op":"begin"})).unwrap();
        assert_eq!(array(&first, "statements").len(), 3);
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":2,"rows":[],"changes":[0,0,0]}))
                .unwrap_err()
                .0,
            409
        );
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[0]}))
                .unwrap_err()
                .0,
            400
        );
        assert!(d.journal.is_empty());
        let result = d
            .execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[0,0,0]}))
            .unwrap();
        assert_eq!(result["kind"], "done");
        assert_eq!(
            d.journal.len(),
            1,
            "The recorded write is consumed rather than emitted again"
        );
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[0,0,0]}))
                .unwrap_err()
                .0,
            400
        );
    }
    #[test]
    fn row_and_journal_byte_limits_reject_expansion_before_retaining_it() {
        let mut d = owner("party");
        d.execute(&json!({"op":"begin"})).unwrap();
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":1,"rows":vec![json!([]);129],"changes":[-1]}))
                .unwrap_err()
                .0,
            400
        );
        assert!(d.journal.is_empty());
        d.bytes = MAX_JOURNAL_BYTES - 1;
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[-1]}))
                .unwrap_err()
                .0,
            400
        );
        assert!(d.journal.is_empty());
        d.bytes = 0;
        assert_eq!(
            d.execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[-1]}))
                .unwrap()["kind"],
            "done"
        );
    }
    #[test]
    fn legacy_request_canonicalization_preserves_ascii_unicode_and_surrogate_pairs() {
        let body = json!({"username":"Grüße🚀#12345678","request_id":"00000000-0000-0000-0000-000000000000","action":"invite","version":1});
        assert_eq!(
            ascii_json(&body),
            r#"{"action":"invite","request_id":"00000000-0000-0000-0000-000000000000","username":"Gr\u00fc\u00dfe\ud83d\ude80#12345678","version":1}"#
        );
        assert_eq!(command_body(&body).unwrap(), "invite");
        let mut invalid = body.clone();
        invalid["version"] = json!(true);
        // The typed mutation branch, rather than JSON truthiness, validates
        // this version before SQL can change accepted membership.
        assert!(m::integer(&invalid["version"], 1, i64::MAX).is_err());
    }
}
