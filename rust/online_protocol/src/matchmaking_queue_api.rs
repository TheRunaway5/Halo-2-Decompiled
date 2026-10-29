//! Native queue endpoint policy and continuations over typed directory/SQL/room IO.
//! This is modern service authorization; original Live arbitration is not inferred.
use super::{Coordinator, fingerprint, id, m, reject};
use crate::matchmaking_model::{Error, Result, array, flag, s};
use crate::matchmaking_skill::{self as skill, PersistentRawRank};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet, HashMap};

fn call(core: &mut Coordinator, mut request: Value) -> Result<Value> {
    request["now"] = json!(core.clock);
    core.execute(&request)
}
fn ticket(core: &Coordinator, who: &str) -> Option<Value> {
    core.tickets
        .iter()
        .find(|t| array(t, "players").iter().any(|p| p["id"] == who))
        .cloned()
}
fn idle() -> Value {
    json!({"state":"idle","playlist_id":"","queued_players":0,"match":null})
}
fn physical(p: &Value) -> &str {
    p["connection_owner"].as_str().unwrap_or(s(p, "id"))
}
fn room_rows(value: &Value) -> Result<&[Value]> {
    let rows = value
        .as_array()
        .ok_or(Error(400, "Queue room observations required"))?;
    m::check(rows.len() <= 64, "Too many queue room observations")?;
    let mut ids = BTreeSet::new();
    for row in rows {
        m::fields(row, &["id", "closed", "phase", "owners", "active"])?;
        m::check(
            ids.insert(m::text(&row["id"], 64)?),
            "Duplicate room observation",
        )?;
        m::boolean(&row["closed"])?;
        m::text(&row["phase"], 32)?;
        let owners = row["owners"]
            .as_object()
            .ok_or(Error(400, "Room owners required"))?;
        m::check(owners.len() <= 16, "Too many room owners")?;
        for (who, owner) in owners {
            m::text(&json!(who), 64)?;
            m::text(owner, 64)?;
        }
        let active = row["active"]
            .as_array()
            .ok_or(Error(400, "Active room owners required"))?;
        m::check(active.len() <= 16, "Too many active room owners")?;
        for who in active {
            m::text(who, 64)?;
        }
    }
    Ok(rows)
}
fn active(room: &Value, who: &str) -> bool {
    !flag(room, "closed")
        && room["owners"]
            .get(who)
            .is_some_and(|owner| array(room, "active").contains(owner))
}
fn prelaunch(phase: &str) -> bool {
    ["connecting", "loading"].contains(&phase)
}
fn query(value: &Value) -> Result<Value> {
    let map = value
        .as_object()
        .ok_or(Error(400, "Queue query observations required"))?;
    m::check(map.len() <= 64, "Too many queue query fields")?;
    for (key, v) in map {
        m::check(key.len() <= 128, "Queue query key too long")?;
        m::check(
            v.as_str().is_some_and(|v| v.len() <= 1024),
            "Invalid queue query value",
        )?;
    }
    Ok(value.clone())
}
fn option(q: &Value, key: &str) -> bool {
    q[key] == "1"
}
#[derive(Clone)]
struct Enqueue {
    generation: u64,
    body: Value,
    facts: String,
    profile: String,
    selected: Value,
    operations: Vec<Value>,
}
#[derive(Clone)]
struct View {
    generation: u64,
    query: Value,
    advanced: bool,
    start: Option<(String, Value)>,
    appearance: HashMap<String, Value>,
}
#[derive(Clone)]
enum Work {
    Enqueue(Enqueue),
    View(View, Step),
}
#[derive(Clone)]
enum Step {
    Advance,
    Start(String),
    Appearance(Vec<String>),
}
struct Pending {
    expires: f64,
    work: Work,
}
#[derive(Clone)]
struct Admission {
    id: String,
    owners: BTreeMap<String, u64>,
    parties: BTreeMap<String, BTreeSet<String>>,
    cancelled: BTreeSet<String>,
    closed: bool,
    expires: f64,
}
#[derive(Default)]
pub(super) struct Registry {
    pending: HashMap<String, Pending>,
    rooms: Vec<Admission>,
    latest: HashMap<String, (u64, String)>,
    origins: HashMap<String, String>,
}
impl Registry {
    pub(super) fn cleanup(&mut self, now: f64) {
        self.pending.retain(|_, p| p.expires > now);
        self.rooms.retain(|r| !r.closed || r.expires > now);
        while self.rooms.iter().filter(|r| r.closed).count() > 1024 {
            let index = self.rooms.iter().position(|r| r.closed).unwrap();
            self.rooms.remove(index);
        }
        let live = self
            .rooms
            .iter()
            .map(|r| r.id.as_str())
            .collect::<BTreeSet<_>>();
        self.latest.retain(|_, (_, id)| live.contains(id.as_str()));
        self.origins.retain(|_, id| live.contains(id.as_str()));
    }
    fn suspend(
        &mut self,
        core: &Coordinator,
        work: Work,
        action: &str,
        operands: Value,
    ) -> Result<Value> {
        reject(
            self.pending.len() < 1000,
            503,
            "Queue endpoint continuation capacity reached",
        )?;
        let frame = id()?;
        self.pending.insert(
            frame.clone(),
            Pending {
                expires: core.clock + core.proposal_lease as f64,
                work,
            },
        );
        let mut event = operands;
        event["kind"] = json!("io");
        event["action"] = json!(action);
        event["frame"] = json!(frame);
        Ok(event)
    }
    fn done(value: Value) -> Value {
        json!({"kind":"done","value":value})
    }
    fn begin_enqueue(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let generation = m::integer(&r["generation"], 1, i64::MAX)? as u64;
        let check = call(
            core,
            json!({"op":"party_queue_check","generation":generation,"body":r["body"],"rooms":r["match_rooms"]}),
        )?;
        if !check["ticket"].is_null() {
            return Ok(Self::done(Value::Null));
        }
        let selected = call(
            core,
            json!({"op":"party_queue_prepare","generation":generation,"body":r["body"],"facts":r["facts"],"rooms":r["match_rooms"]}),
        )?;
        let playlist = s(&check, "playlist");
        let entry = core
            .catalog
            .as_ref()
            .ok_or(Error(503, "Admitted catalog required"))?
            .entry(playlist)?;
        let mut operations = Vec::new();
        if !selected["clan"].is_null() {
            operations.push(json!({"mode":"clan_snapshot","owner":selected["clan"],"playlist":playlist,"configuration":entry["rank_configuration"]}));
        } else {
            let mut seen = BTreeSet::new();
            for connection in array(&selected, "members") {
                for seat in array(connection, "local_players") {
                    if flag(seat, "guest") || !seen.insert(s(seat, "player")) {
                        continue;
                    }
                    operations.push(if entry["rank_configuration"].is_null(){json!({"mode":"legacy_display","owner":seat["player"],"playlist":playlist})}
                    else{json!({"mode":"player_snapshot","owner":seat["player"],"playlist":playlist,"configuration":entry["rank_configuration"]})});
                }
            }
        }
        let work = Enqueue {
            generation,
            body: r["body"].clone(),
            facts: fingerprint(&r["facts"]),
            profile: core.policy_pins[playlist].clone(),
            selected,
            operations: operations.clone(),
        };
        self.suspend(
            core,
            Work::Enqueue(work),
            "database",
            json!({"operations":operations}),
        )
    }
    fn complete_enqueue(
        &mut self,
        core: &mut Coordinator,
        work: Enqueue,
        r: &Value,
    ) -> Result<Value> {
        reject(
            r["generation"].as_u64() == Some(work.generation),
            403,
            "Queue continuation belongs to another connection",
        )?;
        let playlist = s(&work.body, "playlist_id");
        reject(
            core.policy_pins.get(playlist) == Some(&work.profile)
                && fingerprint(&r["facts"]) == work.facts,
            409,
            "Queue profile or consent changed during skill IO",
        )?;
        call(
            core,
            json!({"op":"party_queue_check","generation":work.generation,"body":work.body,"rooms":r["match_rooms"]}),
        )?;
        let results = r["result"]
            .as_array()
            .ok_or(Error(400, "Queue skill IO results required"))?;
        m::check(
            results.len() == work.operations.len(),
            "Queue skill IO count differs",
        )?;
        let mut skills = HashMap::new();
        let mut levels = HashMap::new();
        let mut clan_rank = Value::Null;
        for (op, result) in work.operations.iter().zip(results) {
            if op["mode"] == "legacy_display" {
                levels.insert(s(op, "owner"), m::integer(result, 1, 50)?);
                continue;
            }
            m::fields(result, &["raw", "xp", "configuration"])?;
            let raw = m::integer(&result["raw"], 0, 127)?;
            m::integer(&result["xp"], 0, skill::MAX_XP as i64)?;
            reject(
                result["configuration"] == op["configuration"],
                409,
                "Stored skill configuration differs",
            )?;
            if op["mode"] == "clan_snapshot" {
                clan_rank = json!(raw);
            } else {
                skills.insert(s(op, "owner"), raw);
            }
        }
        let actor = core.sessions.checked(&json!(work.generation), core.clock)?;
        let mut players = Vec::new();
        let mut generations = Vec::new();
        for connection in array(&work.selected, "members") {
            generations.push(connection["generation"].clone());
            for seat in array(connection, "local_players") {
                let raw = skills
                    .get(if flag(seat, "guest") {
                        s(connection, "player")
                    } else {
                        s(seat, "player")
                    })
                    .copied();
                let level = if let Some(raw) = raw {
                    skill::display(PersistentRawRank(raw as u8)).0 as i64
                } else if flag(seat, "guest") || !work.selected["clan"].is_null() {
                    1
                } else {
                    *levels
                        .get(s(seat, "player"))
                        .ok_or(Error(409, "Queue skill owner missing"))?
                };
                players.push(json!({"id":seat["player"],"screen_name":seat["screen_name"],"rank":level,"raw_rank":raw,
                    "maps":connection["maps"],"guest":seat["guest"],"connection_owner":connection["player"]}));
            }
        }
        let ticket = call(
            core,
            json!({"op":"session_queue_enqueue","generation":work.generation,"body":work.body,"permit":work.selected["permit"],"facts":r["facts"],"members":generations,
            "queue":{"party_id":work.selected["party_id"],"playlist_id":playlist,"compatibility":actor["compatibility"],"players":players,"clan":work.selected["clan"],"clan_rank":clan_rank,"leader":actor["player"],"language":actor["language"]}}),
        )?;
        for connection in array(&work.selected, "members") {
            let who = s(connection, "player");
            call(core, json!({"op":"session_failure_pop","player":who}))?;
            self.latest.remove(who);
            self.origins.remove(who);
        }
        Ok(Self::done(ticket))
    }
    fn begin_view(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let generation = m::integer(&r["generation"], 1, i64::MAX)? as u64;
        let actor = core.sessions.checked(&json!(generation), core.clock)?;
        let work = View {
            generation,
            query: query(&r["query"])?,
            advanced: false,
            start: None,
            appearance: HashMap::new(),
        };
        if m::boolean(&r["heartbeat"])? {
            if let Some(t) = ticket(core, s(&actor, "player")) {
                call(core, json!({"op":"heartbeat","id":t["id"]}))?;
            }
        }
        self.view(core, work, &r["rooms"])
    }
    fn view(
        &mut self,
        core: &mut Coordinator,
        mut work: View,
        observations: &Value,
    ) -> Result<Value> {
        let actor = core.sessions.checked(&json!(work.generation), core.clock)?;
        let who = s(&actor, "player");
        let rooms = room_rows(observations)?;
        if !work.advanced {
            work.advanced = true;
            if let Some(t) = ticket(core, who) {
                let playlist = s(&t, "playlist");
                if !core.policy(playlist)?["candidate_selection"].is_null() {
                    return self.suspend(
                        core,
                        Work::View(work, Step::Advance),
                        "gathering_advance",
                        json!({"playlist":playlist}),
                    );
                }
                core.advance(playlist, true)?;
            }
        }
        let mut response = idle();
        let mut group = Vec::new();
        let mut ordered = Vec::new();
        let mut section = "match";
        if let Some(t) = ticket(core, who) {
            let status = call(core, json!({"op":"status","id":t["id"]}))?;
            response["state"] = status["state"].clone();
            response["playlist_id"] = t["playlist"].clone();
            response["queued_players"] = json!(
                core.tickets
                    .iter()
                    .filter(|q| q["playlist"] == t["playlist"]
                        && q["compatibility"] == t["compatibility"])
                    .map(|q| array(q, "players").len())
                    .sum::<usize>()
            );
            if let Some(pid) = status["proposal_id"].as_str() {
                let p = core.refresh(pid, false, &Value::Null)?;
                response["state"] =
                    call(core, json!({"op":"status","id":t["id"]}))?["state"].clone();
                group = core.group(&super::strings(&p["tickets"])?)?;
                ordered = array(&p, "teams")
                    .iter()
                    .flat_map(|team| team.as_array().unwrap().iter().cloned())
                    .collect();
                let players = Coordinator::players(&group);
                response["match"] = json!({"map":p["game"]["map"],"variant":p["game"]["variant"],"players":array(&p,"teams").iter().enumerate().flat_map(|(team,ids)|ids.as_array().unwrap().iter().map(move|id|(team,id))).map(|(team,id)|{
                    let player=players.iter().find(|player|player["id"]==*id).unwrap();json!({"name":player["screen_name"],"team":team,"self":*id==actor["player"]})}).collect::<Vec<_>>()});
                if crate::matchmaking_catalog::OPTIONS.contains(&s(&actor, "compatibility")) {
                    response["match"]["game_id"] = p["game"]["id"].clone();
                    response["match"]["settings"] = p["game"]["settings"].clone();
                    response["match"]["settings_sha256"] = if p["game"]["settings"].is_null() {
                        Value::Null
                    } else {
                        json!(format!(
                            "{:x}",
                            Sha256::digest(m::hex(&p["game"]["settings"], 304)?)
                        ))
                    };
                }
            }
            if option(&work.query, "progress") {
                response["progress"] = if response["state"] == "searching" {
                    call(
                        core,
                        json!({"op":"gather_progress","ticket":t["id"],"viewer":who}),
                    )?
                } else {
                    Value::Null
                };
                if !response["progress"].is_null() {
                    group = core.gatherings.view_group(core, s(&t, "id"))?;
                    ordered = Coordinator::players(&group)
                        .iter()
                        .map(|p| p["id"].clone())
                        .collect();
                    section = "progress";
                }
            }
        } else if option(&work.query, "progress") {
            response["progress"] = Value::Null;
        }
        if option(&work.query, "failure") {
            let failure = call(core, json!({"op":"session_failure_get","player":who}))?;
            response["failure"] = if failure["generation"] == work.generation {
                failure["code"].clone()
            } else {
                Value::Null
            };
        }
        if option(&work.query, "start") {
            if let Some(room) = rooms
                .iter()
                .find(|room| active(room, who) && room["phase"] == "loading")
            {
                let rid = s(room, "id");
                if work.start.as_ref().is_none_or(|(id, _)| id != rid) {
                    return self.suspend(
                        core,
                        Work::View(work, Step::Start(rid.to_owned())),
                        "room_start",
                        json!({"room":rid}),
                    );
                }
                response["start_countdown_ms"] = work.start.as_ref().unwrap().1.clone();
            } else {
                response["start_countdown_ms"] = Value::Null;
            }
        }
        let ranks = option(&work.query, "ranks");
        let appearance = option(&work.query, "appearance");
        let mut missing = Vec::new();
        if ranks || appearance {
            let rows_key = if section == "progress" {
                "roster"
            } else {
                "players"
            };
            for (index, who) in ordered.iter().enumerate() {
                let identity = who.as_str().unwrap();
                let t = group
                    .iter()
                    .find(|t| array(t, "players").iter().any(|p| p["id"] == *who))
                    .unwrap();
                let p = array(t, "players")
                    .iter()
                    .find(|p| p["id"] == *who)
                    .unwrap();
                if ranks {
                    response[section][rows_key][index]["level"] =
                        if flag(core.policy(s(t, "playlist"))?, "ranked") && !flag(p, "guest") {
                            if let Some(raw) = t["clan_rank"].as_u64() {
                                json!(skill::display(PersistentRawRank(raw as u8)).0)
                            } else {
                                p["rank"].clone()
                            }
                        } else {
                            Value::Null
                        };
                }
                if appearance {
                    let value = if let Some(seat) = core.sessions.appearance(identity, core.clock) {
                        if !seat["appearance"].is_null() {
                            Some(seat["appearance"].clone())
                        } else if flag(&seat, "guest") {
                            Some(Value::Null)
                        } else {
                            work.appearance.get(identity).cloned()
                        }
                    } else {
                        work.appearance.get(identity).cloned()
                    };
                    if let Some(value) = value {
                        response[section][rows_key][index]["appearance"] = value;
                    } else {
                        missing.push(identity.to_owned());
                    }
                }
            }
        }
        if !missing.is_empty() {
            let operations = missing
                .iter()
                .map(|who| json!({"mode":"player_appearance","owner":who}))
                .collect::<Vec<_>>();
            return self.suspend(
                core,
                Work::View(work, Step::Appearance(missing)),
                "database",
                json!({"operations":operations}),
            );
        }
        Ok(Self::done(response))
    }
    fn admit_room(&mut self, core: &mut Coordinator, id: &str) -> Result<Value> {
        if self.rooms.iter().any(|r| r.id == id) {
            return Ok(Value::Null);
        }
        reject(
            self.rooms.iter().filter(|r| !r.closed).count() < 64,
            503,
            "Queue room admission capacity reached",
        )?;
        let proposal = core.proposal(id)?;
        let group = core.group(&super::strings(&proposal["tickets"])?)?;
        let mut owners = BTreeMap::new();
        let mut parties = BTreeMap::new();
        for t in group {
            let party = array(&t, "players")
                .iter()
                .map(|p| physical(p).to_owned())
                .collect::<BTreeSet<_>>();
            for who in &party {
                if let Some(session) = core.sessions.primary(who, core.clock) {
                    owners.insert(who.clone(), session["generation"].as_u64().unwrap());
                }
                parties.insert(who.clone(), party.clone());
            }
        }
        call(core, json!({"op":"room_setup_open","id":id}))?;
        for (who, generation) in &owners {
            self.latest
                .insert(who.clone(), (*generation, id.to_owned()));
        }
        self.rooms.push(Admission {
            id: id.to_owned(),
            owners,
            parties,
            cancelled: BTreeSet::new(),
            closed: false,
            expires: f64::MAX,
        });
        Ok(Value::Null)
    }
    fn mark(
        &mut self,
        core: &mut Coordinator,
        room: &Admission,
        kind: &str,
        actor: Option<&str>,
        preserve: bool,
    ) -> Result<()> {
        m::check(
            [
                "search_failed",
                "party_changed",
                "party_cancelled",
                "missing_content",
            ]
            .contains(&kind),
            "Invalid room failure kind",
        )?;
        for (who, generation) in &room.owners {
            if room.cancelled.contains(who)
                || self.latest.get(who) != Some(&(*generation, room.id.clone()))
                || core
                    .sessions
                    .checked(&json!(generation), core.clock)
                    .is_err()
            {
                continue;
            }
            let current = call(core, json!({"op":"session_failure_get","player":who}))?;
            if preserve && !current.is_null() {
                continue;
            }
            let code = if actor.is_none_or(|actor| {
                room.parties
                    .get(actor)
                    .is_some_and(|party| party.contains(who))
            }) {
                kind
            } else {
                "search_failed"
            };
            call(
                core,
                json!({"op":"session_failure_set","generation":generation,"code":code}),
            )?;
            self.origins.insert(who.clone(), room.id.clone());
        }
        Ok(())
    }
    fn cancel(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let actor = core.sessions.checked(&r["generation"], core.clock)?;
        let who = s(&actor, "player");
        let q = query(&r["query"])?;
        if option(&q, "cancel") {
            for observation in room_rows(&r["rooms"])? {
                if active(observation, who) && prelaunch(s(observation, "phase")) {
                    if let Some(index) = self
                        .rooms
                        .iter()
                        .position(|room| room.id == s(observation, "id"))
                    {
                        self.rooms[index].cancelled.insert(who.to_owned());
                        let room = self.rooms[index].clone();
                        self.mark(core, &room, "party_cancelled", Some(who), false)?;
                    }
                }
            }
        }
        call(core, json!({"op":"session_failure_pop","player":who}))?;
        self.origins.remove(who);
        self.latest.remove(who);
        if let Some(t) = ticket(core, who) {
            core.cancel_ticket(s(&t, "id"))?;
        }
        Ok(idle())
    }
    fn content_failure(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        m::fields(&r["body"], &["match_id", "kind"])?;
        m::check(
            r["body"]["kind"] == "missing_content",
            "Invalid match failure",
        )?;
        let id = m::text(&r["body"]["match_id"], 64)?;
        let actor = core.sessions.checked(&r["generation"], core.clock)?;
        let who = s(&actor, "player");
        let generation = actor["generation"].as_u64().unwrap();
        let room = self
            .rooms
            .iter()
            .find(|room| room.id == id)
            .cloned()
            .ok_or(Error(403, "Match admission required"))?;
        reject(
            room.owners.get(who) == Some(&generation),
            403,
            "Match admission required",
        )?;
        reject(
            self.latest.get(who) == Some(&(generation, id.to_owned()))
                && ticket(core, who).is_none(),
            409,
            "Match content rejection is no longer current",
        )?;
        let observed = room_rows(&r["rooms"])?;
        let live = observed
            .iter()
            .find(|room| s(room, "id") == id && !flag(room, "closed"));
        if let Some(observed) = live {
            reject(
                active(observed, who) && prelaunch(s(observed, "phase")),
                409,
                "Match content rejection is no longer current",
            )?;
            self.mark(core, &room, "missing_content", Some(who), false)?;
            Ok(
                json!({"response":{"accepted":true},"close":id,"reason":"A party member is missing required content."}),
            )
        } else {
            reject(
                room.closed && room.expires > core.clock,
                409,
                "Match content rejection is no longer current",
            )?;
            let current = call(core, json!({"op":"session_failure_get","player":who}))?;
            reject(
                current["generation"] == generation
                    && self.origins.get(who).is_some_and(|source| source == id),
                409,
                "Match content rejection is no longer current",
            )?;
            let mut eligible = room.clone();
            eligible.owners.retain(|who, generation| {
                self.origins.get(who).is_some_and(|source| source == id)
                    && core
                        .sessions
                        .failure(who)
                        .is_some_and(|(g, _)| g == *generation)
            });
            self.mark(core, &eligible, "missing_content", Some(who), false)?;
            Ok(json!({"response":{"accepted":true},"close":null,"reason":null}))
        }
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        self.cleanup(core.clock);
        match m::text(&r["op"], 64)? {
            "api_ticket" => Ok(ticket(core, m::text(&r["player"], 64)?).unwrap_or(Value::Null)),
            "api_advance" => {
                let playlist = m::text(&r["playlist"], 512)?;
                if core.policy(playlist)?["candidate_selection"].is_null() {
                    core.advance(playlist, true)?;
                    Ok(json!({"gathering":null}))
                } else {
                    Ok(json!({"gathering":playlist}))
                }
            }
            "api_player_appearance" => {
                let who = m::text(&r["player"], 64)?;
                if let Some(seat) = core.sessions.appearance(who, core.clock) {
                    if flag(&seat, "guest") || !seat["appearance"].is_null() {
                        return Ok(json!({"value":seat["appearance"],"database":null}));
                    }
                }
                Ok(json!({"value":null,"database":{"mode":"player_appearance","owner":who}}))
            }
            "api_connection_owner" => {
                let who = m::text(&r["player"], 64)?;
                Ok(ticket(core, who)
                    .and_then(|t| {
                        array(&t, "players")
                            .iter()
                            .find(|p| p["id"] == who)
                            .map(|p| json!(physical(p)))
                    })
                    .unwrap_or(json!(who)))
            }
            "api_cleanup_subjects" => {
                core.cleanup();
                Ok(json!(
                    core.tickets
                        .iter()
                        .filter(|t| !t["clan"].is_null())
                        .flat_map(|t| array(t, "players").iter().map(|p| p["id"].clone()))
                        .collect::<Vec<_>>()
                ))
            }
            "api_cleanup" => {
                let facts = r["clans"]
                    .as_object()
                    .ok_or(Error(400, "Current clan facts required"))?;
                m::check(facts.len() <= 16000, "Too many queued clan facts")?;
                for (who, clan) in facts {
                    m::text(&json!(who), 64)?;
                    if !clan.is_null() {
                        m::text(clan, 64)?;
                    }
                }
                let tickets = core
                    .tickets
                    .iter()
                    .filter(|t| !t["clan"].is_null())
                    .cloned()
                    .collect::<Vec<_>>();
                m::check(
                    tickets.iter().all(|t| {
                        array(t, "players")
                            .iter()
                            .all(|p| facts.contains_key(s(p, "id")))
                    }),
                    "Queued clan facts incomplete",
                )?;
                for t in tickets {
                    if array(&t, "players")
                        .iter()
                        .any(|p| facts[s(p, "id")] != t["clan"])
                    {
                        core.cancel_ticket(s(&t, "id"))?;
                    }
                }
                Ok(Value::Null)
            }
            "api_enqueue_begin" => self.begin_enqueue(core, r),
            "api_view_begin" => self.begin_view(core, r),
            "api_resume" => {
                let pending = self
                    .pending
                    .remove(m::text(&r["frame"], 64)?)
                    .ok_or(Error(409, "Queue endpoint continuation retired"))?;
                match pending.work {
                    Work::Enqueue(work) => {
                        let permit = s(&work.selected, "permit").to_owned();
                        let result = self.complete_enqueue(core, work, r);
                        core.parties.abort_permit(&permit);
                        result
                    }
                    Work::View(mut work, step) => {
                        reject(
                            r["generation"].as_u64() == Some(work.generation),
                            403,
                            "Queue continuation belongs to another connection",
                        )?;
                        match step {
                            Step::Advance => {
                                m::check(r["result"].is_null(), "Invalid gathering IO completion")?
                            }
                            Step::Start(id) => {
                                if !r["result"].is_null() {
                                    m::integer(&r["result"], 0, i32::MAX as i64)?;
                                }
                                work.start = Some((id, r["result"].clone()));
                            }
                            Step::Appearance(ids) => {
                                let values = r["result"]
                                    .as_array()
                                    .ok_or(Error(400, "Appearance IO results required"))?;
                                m::check(values.len() == ids.len(), "Appearance IO count differs")?;
                                for (id, value) in ids.into_iter().zip(values) {
                                    if !value.is_null() {
                                        m::hex(value, 9)?;
                                    }
                                    work.appearance.insert(id, value.clone());
                                }
                            }
                        }
                        self.view(core, work, &r["rooms"])
                    }
                }
            }
            "api_abort" => {
                if let Some(Pending {
                    work: Work::Enqueue(work),
                    ..
                }) = self.pending.remove(m::text(&r["frame"], 64)?)
                {
                    core.parties.abort_permit(s(&work.selected, "permit"));
                }
                Ok(Value::Null)
            }
            "api_cancel" => self.cancel(core, r),
            "api_content_failure" => self.content_failure(core, r),
            "api_room_admit" => self.admit_room(core, m::text(&r["id"], 64)?),
            "api_room_failure" | "api_room_close" => {
                let id = m::text(&r["id"], 64)?;
                let phase = m::text(&r["phase"], 32)?;
                let Some(index) = self.rooms.iter().position(|room| room.id == id) else {
                    return Err(Error(409, "Room failure admission retired"));
                };
                let close = r["op"] == "api_room_close";
                if close && self.rooms[index].closed {
                    return Ok(Value::Null);
                }
                if prelaunch(phase) {
                    let room = self.rooms[index].clone();
                    let actor = if close || r["player"].is_null() {
                        None
                    } else {
                        Some(m::text(&r["player"], 64)?)
                    };
                    let code = if close {
                        "search_failed"
                    } else {
                        m::text(&r["kind"], 32)?
                    };
                    self.mark(core, &room, code, actor, close)?;
                }
                if close {
                    self.rooms[index].closed = true;
                    self.rooms[index].expires =
                        core.clock + if prelaunch(phase) { 15. } else { 0. };
                }
                Ok(Value::Null)
            }
            _ => Err(Error(400, "Unknown native queue endpoint operation")),
        }
    }
}
