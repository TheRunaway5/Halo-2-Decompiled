//! Rust-owned native gathering registry, query lifecycle and IO continuations.
//!
//! The retired Live procedure and XNet quality producers are not recovered.
//! The explicitly native birth/id provider and relay probe remain separate
//! boundaries. Python executes requested transport IO; it cannot choose a
//! candidate, complete a query, edit a registry or advance a party role.
use super::{Coordinator, id, m, reject, strings};
use crate::matchmaking_model::{Error, Result, array, flag, n, s};
use crate::matchmaking_ratings::{self as ratings, RankMap, RawStatistics, SearchEnvelope};
use crate::search_lifecycle::{self as life, GatheringClock, GatheringConfiguration, PartyState};
use crate::session_candidates::{
    self as candidates, BANK_SIZE, Candidates, Configuration, STRIDE, Search,
};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{HashMap, HashSet};

fn original<T>(value: std::result::Result<T, &'static str>) -> Result<T> {
    value.map_err(|_| Error(400, "Invalid gathering lifecycle operands"))
}
pub(super) fn tick(value: f64) -> Result<u32> {
    let value = (value * 1000.).trunc();
    m::check(value.is_finite(), "Invalid gathering tick")?;
    Ok(value.rem_euclid(4294967296.) as u32)
}
fn wall(value: &Value) -> Result<i64> {
    let value = m::real(value)?.trunc();
    m::check(
        value >= i64::MIN as f64 && value < -(i64::MIN as f64),
        "Invalid gathering wall clock",
    )?;
    Ok(value as i64)
}
fn leader(ticket: &Value) -> &str {
    ticket["leader"]
        .as_str()
        .unwrap_or_else(|| s(&array(ticket, "players")[0], "id"))
}
fn clan(value: &Value) -> Option<[u8; 12]> {
    value
        .as_str()
        .map(|name| Sha256::digest(name.as_bytes())[..12].try_into().unwrap())
}
fn rank_map(policy: &Value) -> Result<RankMap> {
    if policy["hopper"].is_null() {
        Ok(RankMap::retail_launch())
    } else {
        RankMap::from_header(&m::hex(&policy["hopper"], 0x614)?)
            .ok_or(Error(400, "Invalid admitted rank table"))
    }
}
fn statistics(policy: &Value, group: &[Value]) -> Result<RawStatistics> {
    let raw = group
        .iter()
        .flat_map(|t| {
            array(t, "players").iter().map(|p| {
                if policy["clan_match"].is_null() {
                    m::search_rank(p)
                } else {
                    n(t, "clan_rank") as i32
                }
            })
        })
        .collect::<Vec<_>>();
    let flags = group
        .iter()
        .flat_map(|t| {
            array(t, "players")
                .iter()
                .map(|p| u32::from(flag(p, "guest")))
        })
        .collect::<Vec<_>>();
    let stats = ratings::raw_statistics(&raw, &flags)
        .ok_or(Error(400, "Invalid gathering rating roster"))?;
    m::check(
        stats.rated_count > 0,
        "A gathering needs a signed-in player",
    )?;
    Ok(stats)
}
fn envelope(policy: &Value, ticket: &Value) -> Result<SearchEnvelope> {
    let stats = statistics(policy, std::slice::from_ref(ticket))?;
    SearchEnvelope::new(stats.minimum, stats.maximum, &rank_map(policy)?)
        .ok_or(Error(400, "Invalid gathering rank envelope"))
}
fn search(policy: &Value, ticket: &Value, radius: i32) -> Result<(Search, i32)> {
    let stats = statistics(policy, std::slice::from_ref(ticket))?;
    let envelope = envelope(policy, ticket)?;
    let bounds = if radius == -1 {
        None
    } else {
        Some(
            envelope
                .bounds(radius)
                .ok_or(Error(400, "Invalid search radius"))?,
        )
    };
    let search = Search {
        player_count: array(ticket, "players").len() as i32,
        average_rank: stats.average,
        language: n(ticket, "language") as i32,
        quality_first: 0,
        quality_second: -1,
        minimum_rank: bounds.map(|b| b.0),
        maximum_rank: bounds.map(|b| b.1),
        own_address: None,
        clan: clan(&ticket["clan"]),
    };
    let threshold =
        candidates::threshold(radius, envelope.maximum_radius, &Configuration::default())
            .map_err(|_| Error(400, "Invalid candidate threshold"))?;
    Ok((search, threshold))
}
fn search_json(search: &Search) -> Value {
    json!({"player_count":search.player_count,"average_rank":search.average_rank,"language":search.language,
        "quality_first":search.quality_first,"quality_second":search.quality_second,"minimum_rank":search.minimum_rank,
        "maximum_rank":search.maximum_rank,"own_address":null,
        "clan":search.clan.map(|c|c.iter().map(|b|format!("{b:02x}")).collect::<String>())})
}

struct Gathering {
    id: String,
    tickets: Vec<String>,
    born: i64,
    host: String,
    language: i32,
    game: Option<Value>,
    clock: Option<GatheringClock>,
    profile: String,
}
impl Gathering {
    fn snapshot(&self) -> Value {
        json!({"id":self.id,"tickets":self.tickets,"born":self.born,"host":self.host,
        "language":self.language,"game":self.game,"clock":self.clock.as_ref().map(GatheringClock::snapshot)})
    }
}
struct Query {
    generation: u64,
    ids: Vec<String>,
    started: f64,
    selector: Candidates,
    attempted: HashSet<String>,
    finished: HashSet<String>,
}
impl Query {
    fn snapshot(&self) -> Value {
        let mut attempted = self.attempted.iter().collect::<Vec<_>>();
        attempted.sort();
        let mut finished = self.finished.iter().collect::<Vec<_>>();
        finished.sort();
        json!({"generation":self.generation,"ids":self.ids,"started":self.started,"attempted":attempted,"finished":finished})
    }
}
#[derive(Clone)]
struct Context {
    playlist: String,
    profile: String,
    now: f64,
    tick: u32,
    wall: i64,
    task: Task,
}
#[derive(Clone)]
enum Task {
    Advance {
        queue: Vec<String>,
        index: usize,
        budget: usize,
    },
    Query,
    Bank,
}
struct Facts {
    ready: Vec<bool>,
    pairs: Vec<PairFact>,
}
struct PairFact {
    milliseconds: Option<i32>,
    pending: bool,
}
impl Facts {
    fn parse(v: &Value, players: usize, pairs: usize) -> Result<Self> {
        m::fields(v, &["ready", "pairs"])?;
        let ready = v["ready"]
            .as_array()
            .ok_or(Error(400, "Invalid readiness facts"))?;
        let quality = v["pairs"]
            .as_array()
            .ok_or(Error(400, "Invalid quality facts"))?;
        m::check(
            ready.len() == players && quality.len() == pairs,
            "Gathering IO fact count differs",
        )?;
        Ok(Self {
            ready: ready.iter().map(m::boolean).collect::<Result<_>>()?,
            pairs: quality
                .iter()
                .map(|p| {
                    m::fields(p, &["milliseconds", "pending"])?;
                    Ok(PairFact {
                        milliseconds: if p["milliseconds"].is_null() {
                            None
                        } else {
                            Some(m::integer(&p["milliseconds"], 0, i32::MAX as i64)? as i32)
                        },
                        pending: m::boolean(&p["pending"])?,
                    })
                })
                .collect::<Result<_>>()?,
        })
    }
}
enum Wait {
    Ready {
        ticket: String,
        players: Vec<String>,
    },
    QueryFacts {
        ticket: String,
        ids: Vec<String>,
        players: Vec<String>,
    },
    Requests {
        ticket: String,
        ids: Vec<String>,
        facts: Facts,
        actions: Vec<(String, String, String)>,
        minimum: i32,
    },
    BankFacts {
        ticket: String,
        ids: Vec<String>,
        excluded: HashSet<String>,
    },
}
struct Flight {
    token: u64,
    context: Context,
    wait: Wait,
}

#[derive(Default)]
pub(super) struct Registry {
    seed: Option<u32>,
    serial: u64,
    sessions: Vec<Gathering>,
    states: HashMap<String, PartyState>,
    queries: HashMap<String, Query>,
    flight: Option<Flight>,
}
impl Registry {
    pub(super) fn view_group(&self, core: &Coordinator, ticket: &str) -> Result<Vec<Value>> {
        self.sessions
            .iter()
            .find(|g| g.tickets.iter().any(|id| id == ticket))
            .map(|g| core.group(&g.tickets))
            .unwrap_or(Ok(Vec::new()))
    }
    #[cfg(feature = "matchmaking-test-inputs")]
    fn fixture(&mut self, core: &Coordinator, r: &Value) -> Result<Value> {
        reject(
            self.flight.is_none(),
            409,
            "Cannot restore inputs during gathering IO",
        )?;
        match m::text(&r["action"], 64)? {
            "insert" => {
                let v = &r["value"];
                m::fields(
                    v,
                    &["id", "tickets", "born", "host", "language", "game", "clock"],
                )?;
                let key = m::text(&v["id"], 64)?.to_owned();
                let ids = strings(&v["tickets"])?;
                let group = core.group(&ids)?;
                m::check(!group.is_empty(), "Empty gathering fixture")?;
                let policy = core.policy(s(&group[0], "playlist"))?;
                m::check(
                    group
                        .iter()
                        .all(|t| t["playlist"] == policy["id"] && core.current_profile(policy, t)),
                    "Fixture admission differs",
                )?;
                m::check(
                    !self
                        .sessions
                        .iter()
                        .any(|g| g.id != key && g.tickets.iter().any(|id| ids.contains(id))),
                    "Fixture ticket already gathered",
                )?;
                let host = m::text(&v["host"], 64)?.to_owned();
                m::check(
                    Coordinator::players(&group)
                        .iter()
                        .any(|p| s(p, "id") == host),
                    "Fixture host is not admitted",
                )?;
                let clock = if v["clock"].is_null() {
                    None
                } else {
                    let c = &v["clock"];
                    m::fields(c, &["started", "growth", "peak"])?;
                    Some(original(GatheringClock::new(
                        m::integer(&c["started"], 0, u32::MAX as i64)? as u32,
                        m::integer(&c["growth"], 0, u32::MAX as i64)? as u32,
                        m::integer(&c["peak"], 0, 16)? as i32,
                    ))?)
                };
                let game = if v["game"].is_null() {
                    None
                } else {
                    m::check(
                        array(policy, "games").contains(&v["game"]),
                        "Fixture game differs from catalog",
                    )?;
                    Some(v["game"].clone())
                };
                let g = Gathering {
                    id: key.clone(),
                    tickets: ids,
                    born: m::integer(&v["born"], i64::MIN, i64::MAX)?,
                    host,
                    language: m::integer(&v["language"], 0, 8)? as i32,
                    game,
                    clock,
                    profile: core.policy_pins[s(policy, "id")].clone(),
                };
                let result = g.snapshot();
                if let Some(index) = self.sessions.iter().position(|g| g.id == key) {
                    self.sessions[index] = g;
                } else {
                    m::check(self.sessions.len() < 1000, "Fixture registry capacity")?;
                    self.sessions.push(g);
                }
                Ok(result)
            }
            "members" => {
                let key = m::text(&r["id"], 64)?;
                let ids = strings(&r["tickets"])?;
                let group = core.group(&ids)?;
                m::check(
                    !ids.is_empty() && Coordinator::players(&group).len() <= 16,
                    "Invalid fixture membership",
                )?;
                let old = self.session(key)?;
                let policy = core.policy(s(&group[0], "playlist"))?;
                m::check(
                    core.policy_pins.get(s(policy, "id")) == Some(&old.profile)
                        && group.iter().all(|t| core.current_profile(policy, t)),
                    "Fixture profile differs",
                )?;
                m::check(
                    !self
                        .sessions
                        .iter()
                        .any(|g| g.id != key && g.tickets.iter().any(|id| ids.contains(id))),
                    "Fixture duplicate admission",
                )?;
                self.session_mut(key)?.tickets = ids;
                Ok(self.session(key)?.snapshot())
            }
            "clock" => {
                let g = self.session_mut(m::text(&r["id"], 64)?)?;
                let clock = g
                    .clock
                    .as_ref()
                    .ok_or(Error(404, "Clock fixture not initialized"))?;
                let (next, result) =
                    original(life::Object::Clock(clock.clone()).command(&r["command"]))?;
                let life::Object::Clock(next) = next else {
                    unreachable!()
                };
                g.clock = Some(next);
                Ok(result)
            }
            _ => Err(Error(400, "Unknown diagnostic input")),
        }
    }
    fn serial(&mut self) -> Result<u64> {
        self.serial = self
            .serial
            .checked_add(1)
            .ok_or(Error(503, "Gathering generation exhausted"))?;
        Ok(self.serial)
    }
    fn session(&self, id: &str) -> Result<&Gathering> {
        self.sessions
            .iter()
            .find(|g| g.id == id)
            .ok_or(Error(404, "Gathering not found"))
    }
    fn session_mut(&mut self, id: &str) -> Result<&mut Gathering> {
        self.sessions
            .iter_mut()
            .find(|g| g.id == id)
            .ok_or(Error(404, "Gathering not found"))
    }
    fn state(&mut self, core: &Coordinator, id: &str) -> Result<&mut PartyState> {
        let ticket = core.ticket(id)?;
        let policy = core.policy(s(ticket, "playlist"))?;
        reject(
            core.current_profile(policy, ticket),
            409,
            "Admission profile changed; requeue",
        )?;
        if !self.states.contains_key(id) {
            let (next, a, b) = life::jitter(
                self.seed
                    .ok_or(Error(503, "Gathering owner not initialized"))?,
                60000,
                60000,
            );
            self.seed = Some(next);
            self.states.insert(id.to_owned(), PartyState::new(a, b));
        }
        Ok(self.states.get_mut(id).unwrap())
    }
    fn start_search(&mut self, core: &Coordinator, id: &str, tick: u32) -> Result<()> {
        let ticket = core.ticket(id)?;
        let policy = core.policy(s(ticket, "playlist"))?;
        let max = envelope(policy, ticket)?.maximum_radius;
        original(
            self.state(core, id)?
                .start_search(tick, max, flag(policy, "ranked")),
        )?;
        self.queries.remove(id);
        Ok(())
    }
    fn fits(
        &self,
        core: &Coordinator,
        policy: &Value,
        g: &Gathering,
        ticket: &Value,
    ) -> Result<bool> {
        if core.policy_pins.get(s(policy, "id")) != Some(&g.profile) {
            return Ok(false);
        }
        Ok(core.fits(&json!({"playlist":policy["id"],"tickets":g.tickets,"ticket":ticket["id"],"game":g.game}))?.as_bool().unwrap())
    }
    fn withdraw(&mut self, core: &Coordinator, id: &str) -> Result<()> {
        let Some(index) = self.sessions.iter().position(|g| g.id == id) else {
            return Ok(());
        };
        let gathering = self.sessions.remove(index);
        for (index, key) in gathering.tickets.iter().enumerate() {
            self.queries.remove(key);
            if core.ticket(key).is_ok() && self.states.contains_key(key) {
                if index == 0 {
                    self.states.get_mut(key).unwrap().choose();
                } else {
                    self.start_search(core, key, tick(core.clock)?)?;
                }
            }
        }
        Ok(())
    }
    pub(super) fn cleanup(&mut self, core: &Coordinator) {
        let reserved = core
            .proposals
            .iter()
            .flat_map(|p| array(p, "tickets").iter().filter_map(Value::as_str))
            .collect::<HashSet<_>>();
        let available = core
            .tickets
            .iter()
            .filter(|t| {
                !reserved.contains(s(t, "id"))
                    && core
                        .policy(s(t, "playlist"))
                        .is_ok_and(|p| core.current_profile(p, t))
            })
            .map(|t| s(t, "id").to_owned())
            .collect::<HashSet<_>>();
        let remove = self
            .sessions
            .iter()
            .filter(|g| g.tickets.iter().any(|t| !available.contains(t)))
            .map(|g| g.id.clone())
            .collect::<Vec<_>>();
        for id in remove {
            let _ = self.withdraw(core, &id);
        }
        self.states.retain(|id, _| available.contains(id));
        self.queries.retain(|id, _| available.contains(id));
        if self.flight.as_ref().is_some_and(|f| {
            core.policy_pins.get(&f.context.playlist) != Some(&f.context.profile)
                || match &f.wait {
                    Wait::Ready { ticket, .. }
                    | Wait::QueryFacts { ticket, .. }
                    | Wait::Requests { ticket, .. }
                    | Wait::BankFacts { ticket, .. } => !available.contains(ticket),
                }
        }) {
            self.flight = None;
        }
    }
    fn found(
        &mut self,
        core: &mut Coordinator,
        policy: &Value,
        key: &str,
        now: f64,
        wall: i64,
    ) -> Result<Value> {
        let ticket = core.ticket(key)?.clone();
        reject(
            ticket["playlist"] == policy["id"] && core.current_profile(policy, &ticket),
            409,
            "Gathering admission changed",
        )?;
        reject(
            !self
                .sessions
                .iter()
                .any(|g| g.tickets.iter().any(|id| id == key)),
            409,
            "Ticket already gathered",
        )?;
        let Some(index) = core.draw(array(policy, "games"), array(&ticket, "players"))? else {
            return Err(Error(409, "Admitted gathering has no installed hopper map"));
        };
        let tick = tick(now)?;
        let mut clock = original(GatheringClock::new(tick, tick, 0))?;
        original(clock.observe(tick, array(&ticket, "players").len() as i32))?;
        self.state(core, key)?.start_gather(tick);
        let gathering = Gathering {
            id: id()?,
            tickets: vec![key.to_owned()],
            born: wall,
            host: leader(&ticket).to_owned(),
            language: n(&ticket, "language") as i32,
            game: Some(array(policy, "games")[index].clone()),
            clock: Some(clock),
            profile: core.policy_pins[s(policy, "id")].clone(),
        };
        let result = gathering.snapshot();
        self.sessions.push(gathering);
        self.queries.remove(key);
        Ok(result)
    }
    fn proposal(&mut self, core: &mut Coordinator, policy: &Value, id: &str) -> Result<Value> {
        let tick = tick(core.clock)?;
        let group = core.group(&self.session(id)?.tickets)?;
        let count = Coordinator::players(&group).len() as i32;
        let config = original(GatheringConfiguration::from_json(&policy["gathering"]))?;
        let g = self.session_mut(id)?;
        let clock = g
            .clock
            .get_or_insert(original(GatheringClock::new(tick, tick, 0))?);
        if !original(clock.ready(
            tick,
            &config,
            count,
            n(policy, "minimum_players") as i32,
            n(policy, "maximum_players") as i32,
        ))? {
            return Ok(Value::Null);
        }
        let result = core.form(policy, &g.tickets, g.game.as_ref(), None, Some(&g.host))?;
        if !result.is_null() {
            self.sessions.retain(|g| g.id != id);
        }
        Ok(result)
    }
    fn bank(
        &self,
        core: &Coordinator,
        policy: &Value,
        ticket: &Value,
        ids: &[String],
        excluded: &HashSet<String>,
        facts: &Facts,
        wall: i64,
    ) -> Result<Vec<u8>> {
        m::check(
            ids.len() <= 50 && facts.pairs.len() == ids.len(),
            "Invalid bank inputs",
        )?;
        let mut bank = vec![0; BANK_SIZE];
        let map = rank_map(policy)?;
        for (index, id) in ids.iter().enumerate() {
            let g = self.session(id)?;
            let group = core.group(&g.tickets)?;
            let stats = statistics(policy, &group)?;
            let count = Coordinator::players(&group).len() as i32;
            let space = n(policy, "maximum_players") as i32 - count;
            m::check(space >= 0, "Gathering over capacity")?;
            let mask = (1i32 << space.min(n(policy, "maximum_party") as i32)) - 1;
            let age = i32::try_from(
                wall.checked_sub(g.born)
                    .ok_or(Error(400, "Gathering age overflow"))?,
            )
            .map_err(|_| Error(400, "Gathering age out of range"))?;
            let minimum = map
                .advertised_minimum(stats.minimum, stats.maximum)
                .ok_or(Error(400, "Invalid advertised minimum"))?;
            m::check(
                g.id.len() == 36 && g.id.is_ascii(),
                "Invalid private session row identity",
            )?;
            let row = &mut bank[index * STRIDE..(index + 1) * STRIDE];
            row[0] = 1;
            row[1] = u8::from(
                !excluded.contains(id)
                    && !self
                        .states
                        .get(s(ticket, "id"))
                        .is_some_and(|p| p.seen().contains(id)),
            );
            row[0x24..0x48].copy_from_slice(g.id.as_bytes());
            fn put(row: &mut [u8], at: usize, n: i32) {
                row[at..at + 4].copy_from_slice(&n.to_le_bytes());
            }
            for (at, value) in [
                (0x5c, stats.average),
                (0x60, space),
                (0x64, minimum),
                (0x68, stats.maximum),
                (0x6c, age),
                (0x70, mask),
            ] {
                put(row, at, value);
            }
            if let Some(ms) = facts.pairs[index].milliseconds {
                row[0x74] = 1;
                row[0xa4] = 1;
                for (at, value) in [
                    (0x90, ms),
                    (0xac, 4),
                    (0xb0, 0x2651),
                    (0xb4, 0x2651),
                    (0xb8, g.language),
                    (0xf8, 0),
                    (0xfc, -1),
                    (0x10c, space),
                    (0x110, mask),
                    (0x11c, age),
                    (0x120, stats.average),
                    (0x124, stats.maximum),
                    (0x12c, minimum),
                ] {
                    put(row, at, value);
                }
                let mut clans = Vec::new();
                for t in &group {
                    if !t["clan"].is_null() && !clans.contains(&t["clan"]) {
                        clans.push(t["clan"].clone());
                    }
                }
                m::check(clans.len() <= 2, "Invalid gathering clan count")?;
                put(row, 0x130, clans.len() as i32);
                for (i, c) in clans.iter().enumerate() {
                    row[0x134 + i * 12..0x140 + i * 12].copy_from_slice(&clan(c).unwrap());
                }
            }
        }
        Ok(bank)
    }
    fn context(&self, core: &Coordinator, r: &Value, task: Task) -> Result<Context> {
        let policy = core.policy(m::text(&r["playlist"], 512)?)?;
        m::check(
            !policy["gathering"].is_null(),
            "Gathering configuration required",
        )?;
        Ok(Context {
            playlist: s(policy, "id").to_owned(),
            profile: core.policy_pins[s(policy, "id")].clone(),
            now: core.clock,
            tick: tick(core.clock)?,
            wall: wall(&r["wall"])?,
            task,
        })
    }
    fn yield_io(
        &mut self,
        context: Context,
        wait: Wait,
        action: &str,
        players: Vec<String>,
        pairs: Vec<(String, String)>,
    ) -> Result<Value> {
        let token = self.serial()?;
        let result =
            json!({"kind":"io","frame":token,"action":action,"players":players,"pairs":pairs});
        self.flight = Some(Flight {
            token,
            context,
            wait,
        });
        Ok(result)
    }
    fn done(value: Value) -> Value {
        json!({"kind":"done","value":value})
    }
    fn begin_query(&mut self, core: &Coordinator, ctx: Context, key: &str) -> Result<Value> {
        let ticket = core.ticket(key)?;
        let policy = core.policy(&ctx.playlist)?;
        let radius = self
            .state(core, key)?
            .progress()
            .ok_or(Error(409, "Search not initialized"))?
            .radius;
        if !self.queries.contains_key(key) {
            let mut rows = Vec::new();
            for g in &self.sessions {
                if self.fits(core, policy, g, ticket)? {
                    rows.push((g.born, g.id.clone()));
                }
            }
            let ids = original(life::native_birth_id_order(rows))?;
            let selector =
                Candidates::new(search(policy, ticket, radius)?.0, Configuration::default())
                    .map_err(|_| Error(400, "Invalid candidate search"))?;
            let generation = self.serial()?;
            self.queries.insert(
                key.to_owned(),
                Query {
                    generation,
                    ids,
                    started: ctx.now,
                    selector,
                    attempted: HashSet::new(),
                    finished: HashSet::new(),
                },
            );
        }
        let mut ids = Vec::new();
        for id in &self.queries[key].ids {
            if let Ok(g) = self.session(id) {
                if self.fits(core, policy, g, ticket)? {
                    ids.push(id.clone());
                }
            }
        }
        let mut players = Vec::new();
        let mut pairs = Vec::new();
        for id in &ids {
            let g = self.session(id)?;
            players.extend(
                Coordinator::players(&core.group(&g.tickets)?)
                    .iter()
                    .map(|p| s(p, "id").to_owned()),
            );
            pairs.push((leader(ticket).to_owned(), g.host.clone()));
        }
        self.yield_io(
            ctx,
            Wait::QueryFacts {
                ticket: key.to_owned(),
                ids,
                players: players.clone(),
            },
            "facts",
            players,
            pairs,
        )
    }
    fn next(&mut self, core: &mut Coordinator, ctx: Context) -> Result<Value> {
        let Task::Advance {
            queue,
            index,
            budget: _,
        } = &ctx.task
        else {
            return Ok(Self::done(Value::Null));
        };
        if *index >= queue.len() {
            return Ok(Self::done(Value::Null));
        }
        let key = queue[*index].clone();
        if self.sessions.iter().any(|g| g.tickets.contains(&key)) || core.ticket(&key).is_err() {
            let mut ctx = ctx;
            if let Task::Advance { index, .. } = &mut ctx.task {
                *index += 1;
            }
            return self.next(core, ctx);
        }
        let players = array(core.ticket(&key)?, "players")
            .iter()
            .map(|p| s(p, "id").to_owned())
            .collect::<Vec<_>>();
        self.yield_io(
            ctx,
            Wait::Ready {
                ticket: key,
                players: players.clone(),
            },
            "facts",
            players,
            Vec::new(),
        )
    }
    fn after_ticket(&mut self, core: &mut Coordinator, mut ctx: Context) -> Result<Value> {
        if let Task::Advance { index, .. } = &mut ctx.task {
            *index += 1;
        }
        self.next(core, ctx)
    }
    fn process_ticket(
        &mut self,
        core: &mut Coordinator,
        mut ctx: Context,
        key: &str,
    ) -> Result<Value> {
        let policy = core.policy(&ctx.playlist)?.clone();
        self.state(core, key)?;
        loop {
            let cost = self
                .queries
                .get(key)
                .map_or(self.sessions.len().min(50), |q| q.ids.len())
                .max(1);
            if let Task::Advance { budget, .. } = &mut ctx.task {
                if *budget < cost {
                    return self.after_ticket(core, ctx);
                }
                *budget -= cost;
            }
            let state = self.states.get(key).unwrap();
            if state.mode() == "choose" {
                if state.role(ctx.tick, n(&policy, "maximum_players") as i32) == "search" {
                    self.start_search(core, key, ctx.tick)?;
                } else {
                    let g = self.found(core, &policy, key, ctx.now, ctx.wall)?;
                    let proposal = self.proposal(core, &policy, s(&g, "id"))?;
                    if !proposal.is_null() {
                        return Ok(Self::done(proposal));
                    }
                    return self.after_ticket(core, ctx);
                }
            }
            let state = self.states.get(key).unwrap();
            if state.mode() != "search" {
                return self.after_ticket(core, ctx);
            }
            if state.search_expired(ctx.tick) {
                self.queries.remove(key);
                self.states.get_mut(key).unwrap().choose();
                continue;
            }
            return self.begin_query(core, ctx, key);
        }
    }
    fn query_facts(
        &mut self,
        core: &mut Coordinator,
        ctx: Context,
        key: &str,
        ids: Vec<String>,
        facts: Facts,
    ) -> Result<Value> {
        let policy = core.policy(&ctx.playlist)?;
        let ticket = core.ticket(key)?;
        let excluded = &self
            .queries
            .get(key)
            .ok_or(Error(409, "Query retired"))?
            .finished;
        let bank = self.bank(core, policy, ticket, &ids, excluded, &facts, ctx.wall)?;
        self.queries
            .get_mut(key)
            .unwrap()
            .selector
            .update(&bank)
            .map_err(|_| Error(400, "Invalid candidate bank"))?;
        let candidate_bank = *self.queries[key].selector.bank();
        let mut ready = HashMap::new();
        let mut offset = 0;
        for id in &ids {
            let count = Coordinator::players(&core.group(&self.session(id)?.tickets)?).len();
            ready.insert(
                id.clone(),
                facts.ready[offset..offset + count].iter().all(|b| *b),
            );
            offset += count;
        }
        let mut actions = Vec::new();
        for row in candidate_bank.chunks_exact(STRIDE) {
            if row[0] == 0 || row[1] == 0 || row[0x74] != 0 {
                continue;
            }
            let id = std::str::from_utf8(&row[0x24..0x48])
                .map_err(|_| Error(400, "Invalid candidate identity"))?
                .to_owned();
            if self.queries[key].finished.contains(&id) {
                continue;
            }
            let index = ids
                .iter()
                .position(|value| value == &id)
                .ok_or(Error(409, "Candidate retired"))?;
            if !ready[&id]
                || (self.queries[key].attempted.contains(&id) && !facts.pairs[index].pending)
            {
                self.queries
                    .get_mut(key)
                    .unwrap()
                    .finished
                    .insert(id.clone());
                self.state(core, key)?.reject(id);
                continue;
            }
            actions.push((
                id.clone(),
                leader(ticket).to_owned(),
                self.session(&id)?.host.clone(),
            ));
        }
        let radius = self.states[key].progress().unwrap().radius;
        let minimum = search(policy, ticket, radius)?.1;
        if actions.is_empty() {
            return self.query_finish(core, ctx, key, ids, facts, false, minimum);
        }
        let pairs = actions
            .iter()
            .map(|(_, a, b)| (a.clone(), b.clone()))
            .collect();
        self.yield_io(
            ctx,
            Wait::Requests {
                ticket: key.to_owned(),
                ids,
                facts,
                actions,
                minimum,
            },
            "request",
            Vec::new(),
            pairs,
        )
    }
    fn query_finish(
        &mut self,
        core: &mut Coordinator,
        ctx: Context,
        key: &str,
        ids: Vec<String>,
        facts: Facts,
        pending: bool,
        minimum: i32,
    ) -> Result<Value> {
        let policy = core.policy(&ctx.playlist)?.clone();
        let ticket = core.ticket(key)?.clone();
        if !self.queries[key].finished.is_empty() {
            let bank = self.bank(
                core,
                &policy,
                &ticket,
                &ids,
                &self.queries[key].finished,
                &facts,
                ctx.wall,
            )?;
            self.queries
                .get_mut(key)
                .unwrap()
                .selector
                .update(&bank)
                .map_err(|_| Error(400, "Invalid finished bank"))?;
        }
        let elapsed = ((ctx.now - self.queries[key].started) * 1000.).trunc();
        m::check(
            (i32::MIN as f64..=i32::MAX as f64).contains(&elapsed),
            "Invalid query elapsed",
        )?;
        let selected = self.queries[key]
            .selector
            .select(minimum, elapsed as i32, true);
        if let Some(row) = selected {
            let id = std::str::from_utf8(&row[0x24..0x48])
                .map_err(|_| Error(400, "Invalid selected identity"))?
                .to_owned();
            reject(
                self.fits(core, &policy, self.session(&id)?, &ticket)?,
                409,
                "Selected gathering admission changed",
            )?;
            self.session_mut(&id)?.tickets.push(key.to_owned());
            self.state(core, key)?.joined();
            self.queries.remove(key);
            if matches!(ctx.task, Task::Query) {
                return Ok(Self::done(
                    json!({"again":false,"gathering":self.session(&id)?.snapshot()}),
                ));
            }
            let proposal = self.proposal(core, &policy, &id)?;
            if !proposal.is_null() {
                return Ok(Self::done(proposal));
            }
            return self.after_ticket(core, ctx);
        }
        if pending {
            if matches!(ctx.task, Task::Query) {
                return Ok(Self::done(json!({"again":false,"gathering":null})));
            }
            return self.after_ticket(core, ctx);
        }
        let count = self.queries[key].ids.len() as i32;
        original(self.state(core, key)?.complete_query(count))?;
        self.queries.remove(key);
        if matches!(ctx.task, Task::Query) {
            return Ok(Self::done(json!({"again":true,"gathering":null})));
        }
        self.process_ticket(core, ctx, key)
    }
    fn resume(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let token = r["frame"]
            .as_u64()
            .ok_or(Error(400, "Invalid IO generation"))?;
        let flight = self
            .flight
            .as_ref()
            .ok_or(Error(409, "Gathering IO retired"))?;
        reject(token == flight.token, 409, "Stale gathering IO generation")?;
        reject(
            core.policy_pins.get(&flight.context.playlist) == Some(&flight.context.profile),
            409,
            "Gathering profile changed",
        )?;
        // Validate the whole result before taking the continuation. A malformed
        // or replayed reply cannot advance the current Rust query.
        let (facts, results) = match &flight.wait {
            Wait::Ready { players, .. } => (
                Some(Facts::parse(&r["result"], players.len(), 0)?),
                Vec::new(),
            ),
            Wait::QueryFacts { ids, players, .. } => (
                Some(Facts::parse(&r["result"], players.len(), ids.len())?),
                Vec::new(),
            ),
            Wait::BankFacts { ids, .. } => {
                (Some(Facts::parse(&r["result"], 0, ids.len())?), Vec::new())
            }
            Wait::Requests { actions, .. } => {
                let values = r["result"]
                    .as_array()
                    .ok_or(Error(400, "Invalid probe allocation results"))?;
                m::check(
                    values.len() == actions.len(),
                    "Probe allocation result count differs",
                )?;
                (
                    None,
                    values.iter().map(m::boolean).collect::<Result<Vec<_>>>()?,
                )
            }
        };
        let Flight {
            context: ctx, wait, ..
        } = self.flight.take().unwrap();
        match wait {
            Wait::Ready { ticket, .. } => {
                if facts.unwrap().ready.iter().all(|b| *b) {
                    self.process_ticket(core, ctx, &ticket)
                } else {
                    self.after_ticket(core, ctx)
                }
            }
            Wait::QueryFacts { ticket, ids, .. } => {
                self.query_facts(core, ctx, &ticket, ids, facts.unwrap())
            }
            Wait::BankFacts {
                ticket,
                ids,
                excluded,
            } => Ok(Self::done(json!(
                self.bank(
                    core,
                    core.policy(&ctx.playlist)?,
                    core.ticket(&ticket)?,
                    &ids,
                    &excluded,
                    &facts.unwrap(),
                    ctx.wall
                )?
                .iter()
                .map(|b| format!("{b:02x}"))
                .collect::<String>()
            ))),
            Wait::Requests {
                ticket,
                ids,
                facts,
                actions,
                minimum,
            } => {
                let mut pending = false;
                for ((id, _, _), allocated) in actions.into_iter().zip(results) {
                    if allocated {
                        self.queries
                            .get_mut(&ticket)
                            .ok_or(Error(409, "Query retired"))?
                            .attempted
                            .insert(id.clone());
                    }
                    if !self.queries[&ticket].attempted.contains(&id)
                        && ctx.now - self.queries[&ticket].started >= 12.
                    {
                        self.queries
                            .get_mut(&ticket)
                            .unwrap()
                            .finished
                            .insert(id.clone());
                        self.state(core, &ticket)?.reject(id);
                    } else {
                        pending = true;
                    }
                }
                self.query_finish(core, ctx, &ticket, ids, facts, pending, minimum)
            }
        }
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 64)?;
        if self.flight.is_some()
            && [
                "gather_found",
                "gather_start_search",
                "gather_withdraw",
                "gather_proposal",
            ]
            .contains(&op)
        {
            return Err(Error(409, "Gathering IO is active"));
        }
        if self.flight.is_some() && op == "gather_state_command" {
            m::check(
                [
                    "snapshot",
                    "progress_snapshot",
                    "seen_snapshot",
                    "seen_contains",
                    "party_role",
                    "party_search_expired",
                ]
                .contains(&m::text(&r["command"]["op"], 64)?),
                "Cannot mutate a party during gathering IO",
            )?;
        }
        match op {
            "gather_init" => {
                let seed = m::integer(&r["seed"], 0, u32::MAX as i64)? as u32;
                reject(
                    self.seed.is_none(),
                    409,
                    "Gathering owner already initialized",
                )?;
                self.seed = Some(seed);
                Ok(Value::Null)
            }
            "gather_seed" => Ok(json!(
                self.seed.ok_or(Error(503, "Gathering not initialized"))?
            )),
            "gather_tick" => Ok(json!(tick(m::real(&r["value"])?)?)),
            "gather_cleanup" => {
                self.cleanup(core);
                Ok(Value::Null)
            }
            "gather_state" => {
                let key = m::text(&r["ticket"], 64)?;
                let state = self.state(core, key)?;
                Ok(state.snapshot())
            }
            "gather_leader" => Ok(json!(leader(core.ticket(m::text(&r["ticket"], 64)?)?))),
            "gather_group" => Ok(json!(
                core.group(&self.session(m::text(&r["id"], 64)?)?.tickets)?
            )),
            "gather_state_command" => {
                let key = m::text(&r["ticket"], 64)?;
                // Views never recreate a canceled state. Admission/new-state
                // initialization is available only through gather_state.
                core.ticket(key)?;
                let state = self
                    .states
                    .get(key)
                    .ok_or(Error(404, "Gathering state retired"))?;
                let (next, value) =
                    original(life::Object::Party(state.clone()).command(&r["command"]))?;
                let life::Object::Party(next) = next else {
                    unreachable!()
                };
                self.states.insert(key.to_owned(), next);
                Ok(value)
            }
            "gather_start_search" => {
                self.start_search(
                    core,
                    m::text(&r["ticket"], 64)?,
                    m::integer(&r["tick"], 0, u32::MAX as i64)? as u32,
                )?;
                Ok(Value::Null)
            }
            "gather_found" => {
                let policy = core.policy(m::text(&r["playlist"], 512)?)?.clone();
                self.found(
                    core,
                    &policy,
                    m::text(&r["ticket"], 64)?,
                    core.clock,
                    wall(&r["wall"])?,
                )
            }
            "gather_withdraw" => {
                self.withdraw(core, m::text(&r["id"], 64)?)?;
                Ok(Value::Null)
            }
            "gather_proposal" => {
                let policy = core.policy(m::text(&r["playlist"], 512)?)?.clone();
                self.proposal(core, &policy, m::text(&r["id"], 64)?)
            }
            "gather_search" => {
                let ticket = core.ticket(m::text(&r["ticket"], 64)?)?;
                let policy = core.policy(m::text(&r["playlist"], 512)?)?;
                let radius = if r["progress"].is_null() {
                    0
                } else {
                    let p = &r["progress"];
                    original(life::SearchProgress::new(
                        m::integer(&p["maximum"], 0, 127)? as i32,
                        m::boolean(&p["ranked"])?,
                        m::integer(&p["radius"], -1, 127)? as i32,
                        m::integer(&p["total"], i32::MIN as i64, i32::MAX as i64)? as i32,
                        m::boolean(&p["active"])?,
                    ))?
                    .radius
                };
                let (search, threshold) = search(policy, ticket, radius)?;
                Ok(json!({"search":search_json(&search),"threshold":threshold}))
            }
            "gather_progress" => {
                if r["ticket"].is_null() {
                    return Ok(Value::Null);
                }
                let key = m::text(&r["ticket"], 64)?;
                let viewer = m::text(&r["viewer"], 64)?;
                let Some(g) = self
                    .sessions
                    .iter()
                    .find(|g| g.tickets.iter().any(|id| id == key))
                else {
                    return Ok(Value::Null);
                };
                let ticket = core.ticket(key)?;
                let p = core.policy(s(ticket, "playlist"))?;
                let players = Coordinator::players(&core.group(&g.tickets)?);
                let countdown = if let Some(clock) = &g.clock {
                    original(clock.remaining(
                        tick(core.clock)?,
                        &original(GatheringConfiguration::from_json(&p["gathering"]))?,
                        players.len() as i32,
                        n(p, "minimum_players") as i32,
                        n(p, "maximum_players") as i32,
                    ))?
                } else {
                    None
                };
                Ok(
                    json!({"players":players.len(),"minimum_players":p["minimum_players"],"maximum_players":p["maximum_players"],"wait_ms":countdown,
                    "roster":players.iter().map(|p|json!({"name":p["screen_name"],"self":p["id"]==viewer})).collect::<Vec<_>>()}),
                )
            }
            "gather_get" => {
                let key = m::text(&r["key"], 64)?;
                match m::text(&r["map"], 64)? {
                    "sessions" => Ok(self.session(key)?.snapshot()),
                    "states" => Ok(self
                        .states
                        .get(key)
                        .ok_or(Error(404, "State not found"))?
                        .snapshot()),
                    "queries" => Ok(self
                        .queries
                        .get(key)
                        .ok_or(Error(404, "Query not found"))?
                        .snapshot()),
                    _ => Err(Error(400, "Unknown gathering map")),
                }
            }
            "gather_keys" => Ok(match m::text(&r["map"], 64)? {
                "sessions" => json!(self.sessions.iter().map(|g| &g.id).collect::<Vec<_>>()),
                "states" => json!(self.states.keys().collect::<Vec<_>>()),
                "queries" => json!(self.queries.keys().collect::<Vec<_>>()),
                _ => return Err(Error(400, "Unknown gathering map")),
            }),
            "gather_query_bank" => {
                let key = m::text(&r["ticket"], 64)?;
                let query = self.queries.get(key).ok_or(Error(404, "Query not found"))?;
                reject(
                    query.generation
                        == r["generation"]
                            .as_u64()
                            .ok_or(Error(400, "Query generation required"))?,
                    409,
                    "Stale query view",
                )?;
                Ok(json!(
                    query
                        .selector
                        .bank()
                        .iter()
                        .map(|b| format!("{b:02x}"))
                        .collect::<String>()
                ))
            }
            "gather_clock" => {
                let g = self.session(m::text(&r["id"], 64)?)?;
                let clock = g
                    .clock
                    .as_ref()
                    .ok_or(Error(404, "Gathering clock not initialized"))?;
                Ok(clock.snapshot())
            }
            "gather_bank" => {
                reject(self.flight.is_none(), 409, "Gathering IO already active")?;
                let ctx = self.context(core, r, Task::Bank)?;
                let ticket = core.ticket(m::text(&r["ticket"], 64)?)?;
                let ids = strings(&r["ids"])?;
                m::check(ids.len() <= 50, "Bank exceeds original capacity")?;
                let pairs = ids
                    .iter()
                    .map(|id| Ok((leader(ticket).to_owned(), self.session(id)?.host.clone())))
                    .collect::<Result<Vec<_>>>()?;
                let excluded = strings(&r["excluded"])?.into_iter().collect();
                self.yield_io(
                    ctx,
                    Wait::BankFacts {
                        ticket: s(ticket, "id").to_owned(),
                        ids,
                        excluded,
                    },
                    "facts",
                    Vec::new(),
                    pairs,
                )
            }
            "gather_query" => {
                reject(self.flight.is_none(), 409, "Gathering IO already active")?;
                let ctx = self.context(core, r, Task::Query)?;
                self.begin_query(core, ctx, m::text(&r["ticket"], 64)?)
            }
            "gather_resume" => self.resume(core, r),
            "gather_abort" => {
                let token = r["frame"]
                    .as_u64()
                    .ok_or(Error(400, "Invalid abort generation"))?;
                reject(
                    self.flight.as_ref().is_some_and(|f| f.token == token),
                    409,
                    "Stale abort generation",
                )?;
                self.flight = None;
                Ok(Value::Null)
            }
            "gather_advance" => {
                reject(self.flight.is_none(), 409, "Gathering IO already active")?;
                let playlist = m::text(&r["playlist"], 512)?;
                let policy = core.policy(playlist)?.clone();
                let ids = self
                    .sessions
                    .iter()
                    .filter(|g| {
                        core.ticket(&g.tickets[0])
                            .is_ok_and(|t| t["playlist"] == playlist)
                    })
                    .map(|g| g.id.clone())
                    .collect::<Vec<_>>();
                let config = original(GatheringConfiguration::from_json(&policy["gathering"]))?;
                let tick = tick(core.clock)?;
                for id in ids {
                    let proposal = self.proposal(core, &policy, &id)?;
                    if !proposal.is_null() {
                        return Ok(Self::done(proposal));
                    }
                    let founder = self.session(&id)?.tickets[0].clone();
                    let offset = self.state(core, &founder)?.gather_offset();
                    if self.session(&id)?.clock.as_ref().unwrap().expired(
                        tick,
                        config.timeout_ms,
                        offset,
                    ) {
                        self.withdraw(core, &id)?;
                    }
                }
                let reserved = core
                    .proposals
                    .iter()
                    .flat_map(|p| array(p, "tickets").iter().filter_map(Value::as_str))
                    .collect::<HashSet<_>>();
                let mut tickets = core
                    .tickets
                    .iter()
                    .filter(|t| {
                        t["playlist"] == playlist
                            && !reserved.contains(s(t, "id"))
                            && core.current_profile(&policy, t)
                    })
                    .collect::<Vec<_>>();
                tickets.sort_by(|a, b| {
                    a["joined_at"]
                        .as_f64()
                        .unwrap()
                        .total_cmp(&b["joined_at"].as_f64().unwrap())
                        .then_with(|| s(a, "id").cmp(s(b, "id")))
                });
                let queue = tickets.iter().map(|t| s(t, "id").to_owned()).collect();
                let ctx = self.context(
                    core,
                    r,
                    Task::Advance {
                        queue,
                        index: 0,
                        budget: core.search_budget as usize,
                    },
                )?;
                self.next(core, ctx)
            }
            #[cfg(feature = "matchmaking-test-inputs")]
            "gather_fixture" => self.fixture(core, r),
            _ => Err(Error(400, "Unknown gathering operation")),
        }
    }
}
