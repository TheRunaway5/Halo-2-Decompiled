//! Native authenticated matchmaking sessions and their queued request state.
//!
//! Provider grants, canonical account and party/room facts are trusted IO from
//! the identity/directory owners. This is native policy, not Xbox Live auth.
use super::{Coordinator, m, reject};
use crate::matchmaking_catalog as catalog;
use crate::matchmaking_model::{Error, Result, array, flag, s};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeSet, HashMap, HashSet, VecDeque};

fn hash(text: &str) -> String {
    format!("{:x}", Sha256::digest(text.as_bytes()))
}
fn entropy(bytes: usize) -> Result<String> {
    let mut out = vec![0; bytes];
    getrandom::getrandom(&mut out).map_err(|_| Error(503, "Session entropy unavailable"))?;
    Ok(out.iter().map(|b| format!("{b:02x}")).collect())
}
fn optional_text(v: &Value, limit: usize) -> Result<Option<&str>> {
    if v.is_null() {
        Ok(None)
    } else {
        Ok(Some(m::text(v, limit)?))
    }
}
fn name(v: &Value) -> Result<&str> {
    let name = m::text(v, 64)?;
    m::check(
        name.encode_utf16().count() <= 31,
        "Screen name must fit 31 UTF16 characters",
    )?;
    Ok(name)
}
fn appearance(v: &Value) -> Result<Value> {
    let raw = m::hex(v, 9)?;
    m::check(raw[4] <= 1, "Player species must be Spartan or Elite")?;
    Ok(v.clone())
}
fn appearance_field(row: &Value) -> Result<Value> {
    row.get("appearance")
        .map(appearance)
        .unwrap_or(Ok(Value::Null))
}
pub(super) fn uuid(v: &Value) -> Result<&str> {
    let id = m::text(v, 36)?;
    m::check(
        id.len() == 36
            && id.bytes().enumerate().all(|(i, b)| {
                if [8, 13, 18, 23].contains(&i) {
                    b == b'-'
                } else {
                    b.is_ascii_digit() || (b'a'..=b'f').contains(&b)
                }
            }),
        "Invalid request UUID",
    )?;
    Ok(id)
}
fn queue_input(body: &Value) -> Result<Value> {
    m::fields(body, &["playlist_id", "revision", "request_id"])?;
    m::text(&body["playlist_id"], 64)?;
    m::text(&body["revision"], 64)?;
    uuid(&body["request_id"])?;
    Ok(body.clone())
}
pub(super) fn input(v: &Value) -> Result<Value> {
    m::fields_optional(
        v,
        &["screen_name", "maps", "compatibility"],
        &["language", "local_players", "renew_session", "appearance"],
    )?;
    name(&v["screen_name"])?;
    let compatibility = m::text(&v["compatibility"], 64)?;
    reject(
        !compatibility.starts_with("halo2-native-queue-")
            || compatibility == "halo2-native-queue-v7"
            || catalog::GATHERING.contains(&compatibility),
        409,
        "Update Halo 2 before using ONLINE matchmaking",
    )?;
    m::check(
        !catalog::GATHERING.contains(&compatibility) || v.get("language").is_some(),
        "Native title language required",
    )?;
    let language = m::integer(v.get("language").unwrap_or(&json!(0)), 0, 8)?;
    let maps = v["maps"]
        .as_array()
        .ok_or(Error(400, "Invalid map capabilities"))?;
    m::check((1..=256).contains(&maps.len()), "Invalid map capabilities")?;
    let mut unique = BTreeSet::new();
    for map in maps {
        m::check(
            unique.insert(m::text(map, 64)?.to_owned()),
            "Duplicate map capability",
        )?;
    }
    let locals = if let Some(rows) = v.get("local_players") {
        rows.as_array()
            .ok_or(Error(400, "Invalid local players"))?
            .clone()
    } else {
        Vec::new()
    };
    m::check(
        locals.len() <= 3,
        "ONLINE supports at most four local players",
    )?;
    reject(
        locals.is_empty() || catalog::GATHERING[1..].contains(&compatibility),
        409,
        "Update Halo 2 for ONLINE split-screen",
    )?;
    let mut rows = Vec::new();
    for local in locals {
        m::fields_optional(
            &local,
            &["screen_name"],
            &["discord_token", "identity_token", "appearance"],
        )?;
        name(&local["screen_name"])?;
        m::check(
            !(local.get("discord_token").is_some() && local.get("identity_token").is_some()),
            "Supply one identity proof per couch player",
        )?;
        let proof = local
            .get("identity_token")
            .or_else(|| local.get("discord_token"));
        if let Some(proof) = proof {
            m::text(proof, 2048)?;
        }
        rows.push(json!({"screen_name":local["screen_name"],"proof":proof,"appearance":appearance_field(&local)?}));
    }
    let renewal = if let Some(token) = v.get("renew_session") {
        json!(m::text(token, 128)?)
    } else {
        Value::Null
    };
    Ok(
        json!({"screen_name":v["screen_name"],"maps":unique,"compatibility":compatibility,"language":language,
        "locals":rows,"renew_session":renewal,"appearance":appearance_field(v)?}),
    )
}
fn identity(v: &Value) -> Result<(&str, &str)> {
    let owner = v
        .as_array()
        .ok_or(Error(401, "Verified account identity required"))?;
    reject(owner.len() == 2, 401, "Verified account identity required")?;
    let provider = m::text(&owner[0], 64)?;
    let subject = m::text(&owner[1], 4096)?;
    reject(
        matches!(provider, "discord" | "steam"),
        401,
        "Verified account identity required",
    )?;
    Ok((provider, subject))
}
#[derive(Clone)]
struct Session {
    key: String,
    generation: u64,
    data: Value,
}
impl Session {
    fn snapshot(&self) -> Value {
        let mut row = self.data.clone();
        row["generation"] = json!(self.generation);
        row["key"] = json!(self.key);
        row
    }
}
#[derive(Default)]
pub(super) struct Registry {
    serial: u64,
    sessions: Vec<Session>,
    retired: VecDeque<Value>,
    requests: HashMap<(String, String), String>,
    failures: HashMap<String, (u64, String)>,
}
impl Registry {
    pub(super) fn qos_binding(&self, generation: u64, key: &str, who: &str, now: f64) -> bool {
        self.active(generation, now).is_ok_and(|record| {
            record.key == key
                && record.data["player"] == who
                && catalog::GATHERING.contains(&s(&record.data, "compatibility"))
        })
    }
    pub(super) fn appearance(&self, who: &str, now: f64) -> Option<Value> {
        self.sessions
            .iter()
            .filter(|s| s.data["expires"].as_f64().unwrap() > now)
            .flat_map(|s| array(&s.data, "local_players").iter())
            .find(|p| p["player"] == who)
            .cloned()
    }
    pub(super) fn failure(&self, who: &str) -> Option<(u64, &str)> {
        self.failures.get(who).map(|(g, code)| (*g, code.as_str()))
    }
    fn by_generation(&self, g: u64) -> Result<&Session> {
        self.sessions
            .iter()
            .find(|s| s.generation == g)
            .ok_or(Error(401, "Matchmaking session expired; reconnect"))
    }
    fn active(&self, g: u64, now: f64) -> Result<&Session> {
        let value = self.by_generation(g)?;
        reject(
            value.data["expires"].as_f64().unwrap() > now,
            401,
            "Matchmaking session expired; reconnect",
        )?;
        Ok(value)
    }
    fn generation(r: &Value) -> Result<u64> {
        r["generation"]
            .as_u64()
            .ok_or(Error(400, "Session generation required"))
    }
    pub(super) fn primary(&self, who: &str, now: f64) -> Option<Value> {
        self.sessions
            .iter()
            .find(|record| {
                record.data["player"] == who && record.data["expires"].as_f64().unwrap() > now
            })
            .map(Session::snapshot)
    }
    pub(super) fn checked(&self, generation: &Value, now: f64) -> Result<Value> {
        self.active(
            generation
                .as_u64()
                .ok_or(Error(400, "Session generation required"))?,
            now,
        )
        .map(Session::snapshot)
    }
    pub(super) fn group_snapshots(
        &self,
        core: &Coordinator,
        generation: u64,
        playlist: &str,
        members: &Value,
    ) -> Result<Vec<Value>> {
        self.group(
            core,
            &json!({"generation":generation,"members":members}),
            &json!({"playlist":playlist}),
        )
        .map(|(_, rows)| rows.iter().map(Session::snapshot).collect())
    }
    fn revoke(&mut self, core: &mut Coordinator, key: &str) -> Result<Value> {
        let Some(index) = self.sessions.iter().position(|s| s.key == key) else {
            return Ok(Value::Null);
        };
        let record = self.sessions.remove(index);
        core.parties.retire_generation(record.generation);
        let who = s(&record.data, "player").to_owned();
        core.search_qos.retire(&who);
        self.requests.retain(|(player, _), _| *player != who);
        self.failures.remove(&who);
        let ids = core
            .tickets
            .iter()
            .filter(|t| array(t, "players").iter().any(|p| p["id"] == who))
            .map(|t| s(t, "id").to_owned())
            .collect::<Vec<_>>();
        for id in ids {
            // Cancelling one ticket also expires any others whose leases have
            // elapsed. A later collected ID may therefore already be retired.
            if core.ticket(&id).is_ok() {
                core.cancel_ticket(&id)?;
            }
        }
        // Retire cached gathering/query/IO state in the same native command;
        // socket and persistent-party notifications are the remaining IO.
        let mut gatherings = std::mem::take(&mut core.gatherings);
        gatherings.cleanup(core);
        core.gatherings = gatherings;
        self.retired
            .push_back(json!({"key":key,"player":who,"generation":record.generation}));
        Ok(record.snapshot())
    }
    fn cleanup(&mut self, core: &mut Coordinator) -> Result<()> {
        let keys = self
            .sessions
            .iter()
            .filter(|s| s.data["expires"].as_f64().unwrap() <= core.clock)
            .map(|s| s.key.clone())
            .collect::<Vec<_>>();
        for key in keys {
            self.revoke(core, &key)?;
        }
        Ok(())
    }
    fn check(&self, core: &Coordinator, r: &Value) -> Result<Value> {
        let body = input(&r["body"])?;
        let facts = r["identities"]
            .as_array()
            .ok_or(Error(400, "Verified identity facts required"))?;
        m::check(
            facts.len() == 1 + array(&body, "locals").len(),
            "Verified local identity count differs",
        )?;
        let mut canonical = HashSet::new();
        for (slot, fact) in facts.iter().enumerate() {
            let signed = slot == 0 || !body["locals"][slot - 1]["proof"].is_null();
            if !signed {
                m::check(fact.is_null(), "Guests cannot have stored identities")?;
                continue;
            }
            m::fields(fact, &["owner", "player", "separate_party"])?;
            let (provider, subject) = identity(&fact["owner"])?;
            let existing = optional_text(&fact["player"], 64)?;
            m::boolean(&fact["separate_party"])?;
            let key = if let Some(player) = existing {
                format!("player:{player}")
            } else {
                format!("provider:{provider}:{subject}")
            };
            m::check(
                canonical.insert(key),
                "Each signed-in couch player needs a different account",
            )?;
        }
        let primary = facts[0]["player"].as_str();
        if let Some(token) = body["renew_session"].as_str() {
            let old = self
                .sessions
                .iter()
                .find(|s| s.key == hash(token) && s.data["expires"].as_f64().unwrap() > core.clock)
                .ok_or(Error(401, "Matchmaking session expired; reconnect"))?;
            reject(
                primary.is_some_and(|p| old.data["player"] == p),
                403,
                "Matchmaking session belongs to another account",
            )?;
        }
        let rooms = r["rooms"]
            .as_array()
            .ok_or(Error(400, "Current room facts required"))?;
        m::check(rooms.len() <= 64, "Room facts exceed service capacity")?;
        for room in rooms {
            m::fields(room, &["closed", "phase", "owners", "active"])?;
            m::boolean(&room["closed"])?;
            m::text(&room["phase"], 32)?;
            let owners = room["owners"]
                .as_object()
                .ok_or(Error(400, "Room owner facts required"))?;
            m::check(owners.len() <= 16, "Room roster exceeds capacity")?;
            for (player, owner) in owners {
                m::text(&json!(player), 64)?;
                m::text(owner, 64)?;
            }
            let active = room["active"]
                .as_array()
                .ok_or(Error(400, "Room active owners required"))?;
            m::check(active.len() <= 16, "Room active owners exceed capacity")?;
            for owner in active {
                m::text(owner, 64)?;
            }
        }
        for (slot, fact) in facts.iter().enumerate() {
            if fact.is_null() {
                continue;
            }
            let Some(existing) = fact["player"].as_str() else {
                continue;
            };
            reject(
                !self.sessions.iter().any(|record| {
                    Some(s(&record.data, "player")) != primary
                        && array(&record.data, "local_players")
                            .iter()
                            .any(|p| p["player"] == existing)
                }),
                409,
                "An account is already seated on another PC",
            )?;
            reject(
                !rooms.iter().any(|room| {
                    !flag(room, "closed")
                        && room["phase"] != "postgame"
                        && room["owners"].get(existing).is_some_and(|owner| {
                            Some(owner.as_str().unwrap()) != primary
                                && array(room, "active").contains(owner)
                        })
                }),
                409,
                "This account is already playing on another PC",
            )?;
            reject(
                slot == 0 || !flag(fact, "separate_party"),
                409,
                "Leave this player's separate party before adding their local profile",
            )?;
        }
        reject(
            self.sessions.len() < 1000
                || self
                    .sessions
                    .iter()
                    .any(|record| Some(s(&record.data, "player")) == primary),
            503,
            "Session capacity reached",
        )?;
        reject(
            self.retired.len() < 2048,
            503,
            "Session retirement IO must be drained",
        )?;
        Ok(body)
    }
    fn register(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        self.cleanup(core)?;
        let body = self.check(core, r)?;
        let facts = array(r, "identities");
        let ids = r["resolved"]
            .as_array()
            .ok_or(Error(400, "Resolved account IO required"))?;
        m::check(ids.len() == facts.len(), "Resolved account count differs")?;
        let mut unique = HashSet::new();
        for (fact, id) in facts.iter().zip(ids) {
            if fact.is_null() {
                m::check(id.is_null(), "Guest identity must remain transient")?;
                continue;
            }
            let id = m::text(id, 64)?;
            m::check(unique.insert(id), "Canonical local accounts differ")?;
            reject(
                fact["player"].is_null() || fact["player"] == id,
                409,
                "Resolved account changed during admission",
            )?;
        }
        let who = m::text(&ids[0], 64)?;
        // Recheck canonical ownership after identity IO, including linked aliases.
        let mut checked = r.clone();
        for (i, id) in ids.iter().enumerate() {
            if !id.is_null() {
                checked["identities"][i]["player"] = id.clone();
            }
        }
        self.check(core, &checked)?;
        if let Some(token) = body["renew_session"].as_str() {
            let key = hash(token);
            let index = self.sessions.iter().position(|s| s.key == key).unwrap();
            let old = &self.sessions[index].data;
            let same = old["screen_name"] == body["screen_name"]
                && old["maps"] == body["maps"]
                && old["compatibility"] == body["compatibility"]
                && old["language"] == body["language"]
                && array(old, "local_players").len() == facts.len()
                && array(&body, "locals").iter().enumerate().all(|(i, p)| {
                    let old = &old["local_players"][i + 1];
                    old["screen_name"] == p["screen_name"]
                        && flag(old, "guest") == ids[i + 1].is_null()
                        && (ids[i + 1].is_null() || old["player"] == ids[i + 1])
                });
            if same {
                let record = &mut self.sessions[index];
                record.data["seen"] = json!(core.clock);
                record.data["expires"] = json!(core.clock + 900.);
                record.data["appearance"] = body["appearance"].clone();
                record.data["local_players"][0]["appearance"] = body["appearance"].clone();
                for (i, p) in array(&body, "locals").iter().enumerate() {
                    record.data["local_players"][i + 1]["appearance"] = p["appearance"].clone();
                }
                return Ok(
                    json!({"response":{"token":token,"expires_in":900},"session":record.snapshot()}),
                );
            }
        }
        reject(
            self.sessions.len() < 1000 || self.sessions.iter().any(|s| s.data["player"] == who),
            503,
            "Session capacity reached",
        )?;
        let generation = self
            .serial
            .checked_add(1)
            .ok_or(Error(503, "Session generation exhausted"))?;
        let token = entropy(32)?;
        let key = hash(&token);
        let mut seats = vec![
            json!({"player":who,"screen_name":body["screen_name"],"guest":false,"appearance":body["appearance"]}),
        ];
        for (i, local) in array(&body, "locals").iter().enumerate() {
            seats.push(json!({"player":if ids[i+1].is_null(){format!("guest-{}",entropy(16)?)}else{ids[i+1].as_str().unwrap().to_owned()},
            "screen_name":local["screen_name"],"guest":ids[i+1].is_null(),"appearance":local["appearance"]}));
        }
        let keys = self
            .sessions
            .iter()
            .filter(|s| s.data["player"] == who)
            .map(|s| s.key.clone())
            .collect::<Vec<_>>();
        for key in keys {
            self.revoke(core, &key)?;
        }
        self.serial = generation;
        let record = Session {
            key,
            generation,
            data: json!({"player":who,"screen_name":body["screen_name"],"maps":body["maps"],"compatibility":body["compatibility"],
            "expires":core.clock+900.,"seen":core.clock,"language":body["language"],"local_players":seats,"appearance":body["appearance"]}),
        };
        let result =
            json!({"response":{"token":token,"expires_in":900},"session":record.snapshot()});
        self.sessions.push(record);
        Ok(result)
    }
    pub(super) fn queue_check(&self, core: &Coordinator, r: &Value) -> Result<Value> {
        let record = self.active(Self::generation(r)?, core.clock)?;
        let body = queue_input(&r["body"])?;
        let pid = m::text(&body["playlist_id"], 64)?;
        let revision = m::text(&body["revision"], 64)?;
        let request = uuid(&body["request_id"])?;
        let p = core.policy(pid)?;
        reject(
            p["revision"] == revision,
            409,
            "Playlist changed; refresh playlists",
        )?;
        if let Some(c) = &core.catalog {
            let e = c.entry(pid)?;
            reject(
                e["required_compatibility"].as_str().is_none_or(|required| {
                    catalog::supports(s(&record.data, "compatibility"), required)
                }),
                409,
                "Update Halo 2 for this playlist",
            )?;
        }
        let who = s(&record.data, "player");
        if let Some(id) = self.requests.get(&(who.to_owned(), request.to_owned())) {
            let ticket = core
                .ticket(id)
                .map_err(|_| Error(409, "Queue request already completed or cancelled"))?;
            reject(
                ticket["playlist"] == pid && core.current_profile(p, ticket),
                409,
                "Queue request already completed or cancelled",
            )?;
            return Ok(json!({"playlist":pid,"ticket":id}));
        }
        reject(
            self.requests.keys().filter(|(p, _)| p == who).count() < 64,
            429,
            "Reconnect before more queue requests",
        )?;
        reject(
            !core.tickets.iter().any(|t| {
                array(t, "players").iter().any(|p| p["id"] == who) && t["playlist"] != pid
            }),
            409,
            "Already searching another playlist",
        )?;
        Ok(json!({"playlist":pid,"ticket":null}))
    }
    fn group(
        &self,
        core: &Coordinator,
        r: &Value,
        check: &Value,
    ) -> Result<(Session, Vec<Session>)> {
        let record = self.active(Self::generation(r)?, core.clock)?.clone();
        let members = r["members"]
            .as_array()
            .ok_or(Error(400, "Admitted party connections required"))?;
        m::check(
            (1..=16).contains(&members.len()),
            "Invalid party connections",
        )?;
        let mut seats = Vec::new();
        let mut seen = HashSet::new();
        let mut versions = Vec::new();
        let mut connections = Vec::new();
        let policy = core.policy(s(check, "playlist"))?;
        for member in members {
            let generation = member
                .as_u64()
                .ok_or(Error(400, "Invalid party session generation"))?;
            m::check(seen.insert(generation), "Duplicate party connection")?;
            let connection = self.active(generation, core.clock)?;
            let version = s(&connection.data, "compatibility");
            versions.push(version);
            reject(
                policy["candidate_selection"].is_null() || catalog::GATHERING.contains(&version),
                409,
                "Every party member must update Halo 2 for gathering matchmaking",
            )?;
            if let Some(c) = &core.catalog {
                let e = c.entry(s(check, "playlist"))?;
                reject(
                    e["required_compatibility"]
                        .as_str()
                        .is_none_or(|required| catalog::supports(version, required)),
                    409,
                    "Every party member must update Halo 2 for this playlist",
                )?;
            }
            connections.push(connection.clone());
            for seat in array(&connection.data, "local_players") {
                seats.push((seat, connection));
            }
        }
        reject(
            catalog::same_continuity(&versions),
            409,
            "Every party member must use the same host-continuation protocol",
        )?;
        reject(
            members
                .iter()
                .any(|m| m.as_u64() == Some(record.generation)),
            403,
            "Search owner is not a party connection",
        )?;
        reject(
            !(flag(policy, "ranked") || !policy["clan_match"].is_null())
                || !seats.iter().any(|(seat, _)| flag(seat, "guest")),
            409,
            "Guests may use casual playlists only",
        )?;
        m::check(
            seats.len() <= m::n(policy, "maximum_party") as usize,
            "Party not eligible",
        )?;
        m::check(
            array(policy, "games").iter().any(|game| {
                connections
                    .iter()
                    .all(|connection| array(&connection.data, "maps").contains(&game["map"]))
            }),
            "Party has no common map in this playlist",
        )?;
        Ok((record, connections))
    }
    fn enqueue(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let check = self.queue_check(core, r)?;
        if let Some(id) = check["ticket"].as_str() {
            return Ok(core.ticket(id)?.clone());
        }
        let (record, connections) = self.group(core, r, &check)?;
        let binding = core.parties.permit(core, r)?;
        if let Some(ticket) = core
            .tickets
            .iter()
            .find(|t| t["party_id"] == r["queue"]["party_id"])
        {
            reject(
                core.ticket_parties.get(s(ticket, "id")) == Some(&binding),
                409,
                "Party admission changed; requeue",
            )?;
        }
        let seats = connections
            .iter()
            .flat_map(|connection| {
                array(&connection.data, "local_players")
                    .iter()
                    .map(move |seat| (seat, connection))
            })
            .collect::<Vec<_>>();
        let players = r["queue"]["players"]
            .as_array()
            .ok_or(Error(400, "Trusted queue players required"))?;
        reject(
            players.len() == seats.len(),
            409,
            "Queue roster differs from admitted sessions",
        )?;
        for (p, (seat, connection)) in players.iter().zip(&seats) {
            m::player(p)?;
            reject(
                p["id"] == seat["player"]
                    && p["screen_name"] == seat["screen_name"]
                    && p["guest"] == seat["guest"]
                    && p["connection_owner"] == connection.data["player"]
                    && p["maps"] == connection.data["maps"],
                409,
                "Queue seat facts differ from admitted sessions",
            )?;
        }
        let q = &r["queue"];
        reject(
            q["playlist_id"] == check["playlist"]
                && q["compatibility"] == record.data["compatibility"]
                && q["leader"] == record.data["player"]
                && q["language"] == record.data["language"],
            409,
            "Search owner differs from admitted session",
        )?;
        let ticket = core.enqueue(q)?;
        core.ticket_parties
            .insert(s(&ticket, "id").to_owned(), binding);
        core.parties.consume(r);
        self.requests.insert(
            (
                s(&record.data, "player").to_owned(),
                s(&r["body"], "request_id").to_owned(),
            ),
            s(&ticket, "id").to_owned(),
        );
        Ok(ticket)
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        match m::text(&r["op"], 64)? {
            "session_check" => {
                self.cleanup(core)?;
                self.check(core, r)
            }
            "session_register" => self.register(core, r),
            "session_auth" => {
                let header = r["header"]
                    .as_str()
                    .ok_or(Error(401, "Matchmaking session required"))?;
                reject(
                    header.starts_with("Bearer ") && header.len() <= 256,
                    401,
                    "Matchmaking session required",
                )?;
                let key = hash(&header[7..]);
                let index = self
                    .sessions
                    .iter()
                    .position(|s| s.key == key && s.data["expires"].as_f64().unwrap() > core.clock)
                    .ok_or(Error(401, "Matchmaking session expired; reconnect"))?;
                self.sessions[index].data["seen"] = json!(core.clock);
                Ok(self.sessions[index].snapshot())
            }
            "session_get" => Ok(self
                .sessions
                .iter()
                .find(|s| s.key == r["key"])
                .map(Session::snapshot)
                .unwrap_or(Value::Null)),
            "session_generation" => Ok(self
                .sessions
                .iter()
                .find(|s| Some(s.generation) == r["generation"].as_u64())
                .map(Session::snapshot)
                .unwrap_or(Value::Null)),
            "session_directory_seats" => {
                let owners = r["owners"]
                    .as_array()
                    .ok_or(Error(400, "Directory connection subjects required"))?;
                m::check(owners.len() <= 16, "Too many directory connection subjects")?;
                let mut seen = HashSet::new();
                let mut players = Vec::new();
                for owner in owners {
                    let who = m::text(owner, 64)?;
                    m::check(seen.insert(who), "Duplicate directory connection subject")?;
                    if let Some(connection) = self.primary(who, core.clock) {
                        players.extend(
                            array(&connection, "local_players")
                                .iter()
                                .filter(|p| !flag(p, "guest"))
                                .map(|p| p["player"].clone()),
                        );
                    }
                }
                Ok(json!(players))
            }
            "session_keys" => Ok(json!(
                self.sessions
                    .iter()
                    .map(|s| s.key.clone())
                    .collect::<Vec<_>>()
            )),
            "session_cleanup" => {
                self.cleanup(core)?;
                Ok(Value::Null)
            }
            "session_revoke" => self.revoke(core, m::text(&r["key"], 64)?),
            "session_events" => Ok(json!(self.retired.drain(..).collect::<Vec<_>>())),
            "session_queue_check" => self.queue_check(core, r),
            "session_queue_plan" => {
                let check = self.queue_check(core, r)?;
                let (_, connections) = self.group(core, r, &check)?;
                Ok(json!({"members":connections.iter().map(Session::snapshot).collect::<Vec<_>>()}))
            }
            "session_queue_enqueue" => self.enqueue(core, r),
            "session_request_items" => Ok(json!(
                self.requests
                    .iter()
                    .map(|((p, r), t)| json!([p, r, t]))
                    .collect::<Vec<_>>()
            )),
            "session_failure_set" => {
                let g = Self::generation(r)?;
                let record = self.by_generation(g)?;
                let who = s(&record.data, "player").to_owned();
                let code = m::text(&r["code"], 32)?;
                m::check(
                    [
                        "search_failed",
                        "party_changed",
                        "party_cancelled",
                        "missing_content",
                    ]
                    .contains(&code),
                    "Invalid native search failure",
                )?;
                self.failures.insert(who, (g, code.to_owned()));
                Ok(Value::Null)
            }
            "session_failure_get" => {
                let who = m::text(&r["player"], 64)?;
                Ok(self
                    .failures
                    .get(who)
                    .map(|(g, c)| json!({"generation":g,"code":c}))
                    .unwrap_or(Value::Null))
            }
            "session_failure_keys" => Ok(json!(self.failures.keys().collect::<Vec<_>>())),
            "session_failure_pop" => {
                let who = m::text(&r["player"], 64)?;
                Ok(self
                    .failures
                    .remove(who)
                    .map(|(g, c)| json!({"generation":g,"code":c}))
                    .unwrap_or(Value::Null))
            }
            #[cfg(feature = "matchmaking-test-inputs")]
            "session_fixture_expires" => {
                let g = Self::generation(r)?;
                let index = self
                    .sessions
                    .iter()
                    .position(|s| s.generation == g)
                    .ok_or(Error(404, "Fixture session retired"))?;
                self.sessions[index].data["expires"] = json!(m::real(&r["expires"])?);
                Ok(self.sessions[index].snapshot())
            }
            _ => Err(Error(400, "Unknown native session operation")),
        }
    }
}
pub(super) fn value(r: &Value) -> Result<Value> {
    match m::text(&r["op"], 64)? {
        "session_body" => input(&r["body"]),
        "session_queue_body" => queue_input(&r["body"]),
        "session_verified_identity" => {
            let (p, s) = identity(&r["owner"])?;
            Ok(json!([p, s]))
        }
        _ => Err(Error(400, "Unknown session value operation")),
    }
}
