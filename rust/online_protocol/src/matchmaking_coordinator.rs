//! Authoritative native queue/ticket/proposal owner.
//!
//! Reuses the original Rust assignment, raw-rating and hopper-draw owners.
//! Leases, reciprocal legacy stages, bounded central subset assembly and route
//! host ranking are explicitly native replacement policies, not recovered Live.
use crate::matchmaking_catalog::Catalog;
use crate::matchmaking_model::{self as m, Error, Result, array, flag, n, s};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeSet, HashMap, HashSet};

#[path = "matchmaking_gatherings.rs"]
mod gatherings;
#[path = "matchmaking_parties.rs"]
mod parties;
#[path = "matchmaking_queue_api.rs"]
mod queue_api;
#[path = "matchmaking_results.rs"]
mod results;
#[path = "matchmaking_room_setup.rs"]
mod room_setup;
#[path = "matchmaking_search_qos.rs"]
mod search_qos;
#[path = "matchmaking_sessions.rs"]
mod sessions;

pub const REQUEST_LIMIT: usize = 8 * 1024 * 1024;
pub(crate) fn known_variant(name: &str) -> Result<Option<(u32, bool)>> {
    results::legacy_variant(name)
}
pub const RESPONSE_LIMIT: usize = 2 * 1024 * 1024;
pub type WireValue = Value;
pub fn envelope(result: Result<Value>) -> Value {
    match result {
        Ok(value) => json!({"ok":true,"value":value}),
        Err(Error(status, error)) => json!({"ok":false,"status":status,"error":error}),
    }
}

pub(crate) fn id() -> Result<String> {
    let mut bytes = [0u8; 16];
    getrandom::getrandom(&mut bytes).map_err(|_| Error(503, "Identity entropy unavailable"))?;
    bytes[6] = (bytes[6] & 15) | 0x40;
    bytes[8] = (bytes[8] & 63) | 0x80;
    let hex = bytes.iter().map(|b| format!("{b:02x}")).collect::<String>();
    Ok(format!(
        "{}-{}-{}-{}-{}",
        &hex[..8],
        &hex[8..12],
        &hex[12..16],
        &hex[16..20],
        &hex[20..]
    ))
}
fn fingerprint(value: &Value) -> String {
    format!("{:x}", Sha256::digest(value.to_string().as_bytes()))
}
fn strings(value: &Value) -> Result<Vec<String>> {
    value
        .as_array()
        .ok_or(Error(400, "Invalid identity list"))?
        .iter()
        .map(|v| Ok(m::text(v, 64)?.to_owned()))
        .collect()
}
fn optional_string(v: &Value) -> Result<Option<&str>> {
    if v.is_null() {
        Ok(None)
    } else {
        Ok(Some(m::text(v, 64)?))
    }
}
fn reject(condition: bool, status: u16, message: &'static str) -> Result<()> {
    if condition {
        Ok(())
    } else {
        Err(Error(status, message))
    }
}

pub struct Coordinator {
    search_qos: search_qos::Registry,
    setup: room_setup::Registry,
    api: queue_api::Registry,
    results: results::Registry,
    sessions: sessions::Registry,
    parties: parties::Registry,
    ticket_parties: HashMap<String, Value>,
    gatherings: gatherings::Registry,
    playlists: Vec<Value>,
    catalog: Option<Catalog>,
    policy_pins: HashMap<String, String>,
    tickets: Vec<Value>,
    proposals: Vec<Value>,
    ready: HashMap<String, BTreeSet<String>>,
    connections: HashMap<(String, String), (i64, f64)>,
    ticket_profiles: HashMap<String, String>,
    proposal_bindings: HashMap<String, Value>,
    next_anchor: HashMap<String, String>,
    clock: f64,
    lease: i64,
    proposal_lease: i64,
    qos_lease: i64,
    capacity: i64,
    search_budget: i64,
    draw_seed: u32,
    native_seed: u32,
}

impl Coordinator {
    pub fn result_commit_context(&self, id: &str) -> Result<Value> {
        let mut context = self.results.commit_context(id, 0)?;
        let required = context["admission"]["compatibility"] == "halo2-native-queue-v11";
        context["authority_epoch"] = json!(self.setup.authority_epoch(id, required)?);
        Ok(context)
    }
    pub fn public_rank_setup(
        &self,
        playlist: &str,
        player: &str,
        clan: Option<&str>,
    ) -> Result<Value> {
        let entry = self
            .catalog
            .as_ref()
            .ok_or(Error(503, "Admitted service catalog required"))?
            .entry(playlist)?;
        if !entry["policy"]["clan_match"].is_null() {
            return Ok(clan
                .map(|clan| {
                    json!({"mode":"clan_snapshot","owner":clan,"playlist":playlist,
                "configuration":entry["rank_configuration"]})
                })
                .unwrap_or(Value::Null));
        }
        Ok(if entry["rank_configuration"].is_null() {
            json!({"mode":"legacy_display","owner":player,"playlist":playlist})
        } else {
            json!({"mode":"player_snapshot","owner":player,"playlist":playlist,"configuration":entry["rank_configuration"]})
        })
    }
    pub fn new(v: &Value) -> Result<Self> {
        m::fields_optional(
            v,
            &[
                "playlists",
                "lease",
                "proposal_lease",
                "qos_lease",
                "capacity",
                "search_budget",
                "draw_seed",
                "native_seed",
            ],
            &["catalog"],
        )?;
        let policies = v["playlists"]
            .as_array()
            .ok_or(Error(400, "Invalid playlist catalog"))?;
        m::check(
            (1..=64).contains(&policies.len()),
            "Invalid playlist catalog",
        )?;
        let mut ids = HashSet::new();
        for policy in policies {
            m::playlist(policy)?;
            m::check(ids.insert(s(policy, "id")), "Duplicate playlist")?;
        }
        let catalog = if v["catalog"].is_null() {
            None
        } else {
            Some(Catalog::new(&v["catalog"])?)
        };
        if let Some(catalog) = &catalog {
            m::check(
                catalog.policies() == *policies,
                "Catalog policy facts differ from queue setup",
            )?;
        }
        let policy_pins = policies
            .iter()
            .map(|p| {
                let profile = if let Some(c) = &catalog {
                    c.profile(p)?
                } else {
                    p.clone()
                };
                Ok((s(p, "id").to_owned(), fingerprint(&profile)))
            })
            .collect::<Result<HashMap<_, _>>>()?;
        Ok(Self {
            search_qos: search_qos::Registry::default(),
            setup: room_setup::Registry::default(),
            api: queue_api::Registry::default(),
            results: results::Registry::default(),
            sessions: sessions::Registry::default(),
            parties: parties::Registry::default(),
            ticket_parties: HashMap::new(),
            gatherings: gatherings::Registry::default(),
            playlists: policies.clone(),
            policy_pins,
            catalog,
            tickets: Vec::new(),
            proposals: Vec::new(),
            ready: HashMap::new(),
            connections: HashMap::new(),
            ticket_profiles: HashMap::new(),
            proposal_bindings: HashMap::new(),
            next_anchor: HashMap::new(),
            clock: 0.,
            lease: m::integer(&v["lease"], 1, 300)?,
            proposal_lease: m::integer(&v["proposal_lease"], 1, 120)?,
            qos_lease: m::integer(&v["qos_lease"], 1, 120)?,
            capacity: m::integer(&v["capacity"], 1, 1000)?,
            search_budget: m::integer(&v["search_budget"], 1, 100000)?,
            draw_seed: m::integer(&v["draw_seed"], 0, u32::MAX as i64)? as u32,
            native_seed: m::integer(&v["native_seed"], 0, u32::MAX as i64)? as u32,
        })
    }
    fn profile_value(&self, p: &Value) -> Result<Value> {
        if let Some(catalog) = &self.catalog {
            catalog.profile(p)
        } else {
            Ok(p.clone())
        }
    }
    fn replace_policies(&mut self, list: &[Value], catalog: Option<Catalog>) -> Result<()> {
        let pins = list
            .iter()
            .map(|p| {
                let profile = if let Some(c) = &catalog {
                    c.profile(p)?
                } else {
                    p.clone()
                };
                Ok((s(p, "id").to_owned(), fingerprint(&profile)))
            })
            .collect::<Result<HashMap<_, _>>>()?;
        self.playlists = list.to_vec();
        self.catalog = catalog;
        self.policy_pins = pins;
        self.cleanup();
        Ok(())
    }
    fn policy(&self, id: &str) -> Result<&Value> {
        self.playlists
            .iter()
            .find(|p| s(p, "id") == id)
            .ok_or(Error(404, "Unknown playlist"))
    }
    fn ticket(&self, id: &str) -> Result<&Value> {
        self.tickets
            .iter()
            .find(|p| s(p, "id") == id)
            .ok_or(Error(404, "Ticket not found"))
    }
    fn proposal(&self, id: &str) -> Result<&Value> {
        self.proposals
            .iter()
            .find(|p| s(p, "id") == id)
            .ok_or(Error(404, "Proposal not found"))
    }
    fn group(&self, ids: &[String]) -> Result<Vec<Value>> {
        m::check(ids.len() <= 16, "Invalid admission group bounds")?;
        m::check(
            ids.iter().collect::<HashSet<_>>().len() == ids.len(),
            "Duplicate admission ticket",
        )?;
        ids.iter().map(|id| Ok(self.ticket(id)?.clone())).collect()
    }
    fn players(group: &[Value]) -> Vec<Value> {
        group
            .iter()
            .flat_map(|t| array(t, "players").iter().cloned())
            .collect()
    }
    fn remove_proposal(&mut self, id: &str) {
        self.proposals.retain(|p| s(p, "id") != id);
        self.ready.remove(id);
        self.proposal_bindings.remove(id);
    }
    fn cancel_ticket(&mut self, ident: &str) -> Result<()> {
        self.ticket(ident)?;
        let remove = self
            .proposals
            .iter()
            .filter(|p| array(p, "tickets").iter().any(|t| t == ident))
            .map(|p| s(p, "id").to_owned())
            .collect::<Vec<_>>();
        for id in remove {
            self.remove_proposal(&id);
        }
        self.tickets.retain(|t| s(t, "id") != ident);
        self.ticket_profiles.remove(ident);
        self.ticket_parties.remove(ident);
        self.cleanup();
        Ok(())
    }
    fn cleanup(&mut self) {
        self.api.cleanup(self.clock);
        self.parties.cleanup(self.clock, &self.policy_pins);
        let expired = self
            .tickets
            .iter()
            .filter(|t| t["expires_at"].as_f64().unwrap() <= self.clock)
            .map(|t| s(t, "id").to_owned())
            .collect::<HashSet<_>>();
        let proposals = self
            .proposals
            .iter()
            .filter(|p| {
                p["expires_at"].as_f64().unwrap() <= self.clock
                    || array(p, "tickets")
                        .iter()
                        .any(|id| expired.contains(id.as_str().unwrap()))
            })
            .map(|p| s(p, "id").to_owned())
            .collect::<Vec<_>>();
        for id in proposals {
            self.remove_proposal(&id);
        }
        self.tickets.retain(|t| !expired.contains(s(t, "id")));
        self.ticket_profiles.retain(|id, _| !expired.contains(id));
        let live = self
            .tickets
            .iter()
            .map(|t| s(t, "id"))
            .collect::<HashSet<_>>();
        self.ticket_parties
            .retain(|id, _| live.contains(id.as_str()));
        let active = self
            .tickets
            .iter()
            .flat_map(|t| array(t, "players").iter().map(|p| s(p, "id").to_owned()))
            .collect::<HashSet<_>>();
        self.connections.retain(|(a, b), (_, end)| {
            *end > self.clock && active.contains(a) && active.contains(b)
        });
        // Teardown retires the actual Rust states, including saved view
        // generations and in-flight query work, at the queue owner boundary.
        let mut registry = std::mem::take(&mut self.gatherings);
        registry.cleanup(self);
        self.gatherings = registry;
    }
    fn random_index(&mut self, count: usize) -> usize {
        // Native scheduler/host RNG stream v2. This is separate from the
        // original hopper draw stream; upstream original ownership is open.
        self.native_seed = self
            .native_seed
            .wrapping_mul(1664525)
            .wrapping_add(1013904223);
        ((u64::from(self.native_seed) * count as u64) >> 32) as usize
    }
    fn stage<'a>(&self, policy: &'a Value, ticket: &Value) -> &'a Value {
        let waited = self.clock - ticket["joined_at"].as_f64().unwrap();
        array(policy, "stages")
            .iter()
            .rev()
            .find(|r| waited >= n(r, "after_seconds") as f64)
            .unwrap_or(&array(policy, "stages")[0])
    }
    fn radius(policy: &Value, rank: i32, raw: bool) -> Result<i32> {
        if flag(policy, "use_2007_rank_limits") {
            m::rank_radius(
                if raw { rank.min(49) + 1 } else { rank },
                policy["high_rank_radius"].as_i64().map(|n| n as i32),
            )
        } else {
            Ok(n(policy, "custom_rank_radius") as i32)
        }
    }
    fn current_profile(&self, policy: &Value, ticket: &Value) -> bool {
        self.ticket_profiles
            .get(s(ticket, "id"))
            .is_some_and(|pin| {
                self.policy_pins
                    .get(s(policy, "id"))
                    .is_some_and(|current| pin == current)
            })
    }
    fn compatible(&self, policy: &Value, a: &Value, b: &Value) -> Result<bool> {
        if ["playlist", "revision", "compatibility"]
            .iter()
            .any(|k| a[*k] != b[*k])
            || !self.current_profile(policy, a)
            || !self.current_profile(policy, b)
        {
            return Ok(false);
        }
        let raw = !array(a, "players")[0]["raw_rank"].is_null();
        if raw == array(b, "players")[0]["raw_rank"].is_null() {
            return Ok(false);
        }
        let players = Self::players(&[a.clone(), b.clone()]);
        if m::available(array(policy, "games"), &players).is_empty() {
            return Ok(false);
        }
        if !policy["clan_match"].is_null()
            && (a["clan"].is_null()
                || b["clan"].is_null()
                || a["clan"] == b["clan"]
                || (array(a, "players").len() as i64 - array(b, "players").len() as i64).abs()
                    > n(&policy["clan_match"], "imbalance"))
        {
            return Ok(false);
        }
        if !policy["candidate_selection"].is_null() {
            return Ok(true);
        }
        let (x, y) = if !policy["clan_match"].is_null() {
            (n(a, "clan_rank") as i32, n(b, "clan_rank") as i32)
        } else {
            (
                m::party_rank(array(a, "players"))?,
                m::party_rank(array(b, "players"))?,
            )
        };
        for (ticket, rank) in [(a, x), (b, y)] {
            let stage = self.stage(policy, ticket);
            if flag(stage, "unbounded_skill") {
                continue;
            }
            let f = &stage["rank_fraction"];
            if i128::from((x - y).abs()) * i128::from(f[1].as_i64().unwrap())
                > i128::from(Self::radius(policy, rank, raw)?) * i128::from(f[0].as_i64().unwrap())
            {
                return Ok(false);
            }
        }
        Ok(true)
    }
    fn teams(&self, policy: &Value, group: &[Value]) -> Result<Option<Vec<Vec<String>>>> {
        if group.iter().any(|t| !self.current_profile(policy, t)) {
            return Ok(None);
        }
        if !policy["clan_match"].is_null()
            && (group.len() != 2 || !self.compatible(policy, &group[0], &group[1])?)
        {
            return Ok(None);
        }
        let players = Self::players(group);
        if players.is_empty() || players.len() > 16 {
            return Ok(None);
        }
        let parties = group
            .iter()
            .enumerate()
            .flat_map(|(i, t)| std::iter::repeat_n(i as i32, array(t, "players").len()))
            .collect::<Vec<_>>();
        let weights = group
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
        let assignment = if !policy["hopper"].is_null() {
            let bytes = m::hex(&policy["hopper"], 0x614)?;
            crate::team_assignment::assign(
                crate::team_assignment::Hopper::from_header(&bytes).unwrap(),
                &parties,
                &weights,
            )
        } else {
            let c = &policy["clan_match"];
            let b = &policy["party_balance"];
            crate::team_assignment::NativePolicy {
                ranked: flag(policy, "ranked"),
                team_count: n(policy, "team_count") as u32,
                team_size: n(policy, "team_size") as u32,
                clan_minimum: c["minimum"].as_u64().unwrap_or(0) as u32,
                clan_imbalance: c["imbalance"].as_u64().unwrap_or(0) as u32,
                significant_minimum: b["minimum_large"].as_u64().unwrap_or(0) as u32,
                significant_maximum: b["maximum"].as_u64().unwrap_or(0) as u32,
                significant_spread: b["imbalance"].as_u64().unwrap_or(0) as u32,
            }
            .assign(&parties, &weights)
        };
        Ok(assignment.map(|a| {
            (0..if n(policy, "team_size") == 0 {
                players.len()
            } else {
                n(policy, "team_count") as usize
            })
                .map(|team| {
                    players
                        .iter()
                        .zip(&a.teams)
                        .filter(|(_, t)| **t == team as i32)
                        .map(|(p, _)| s(p, "id").to_owned())
                        .collect()
                })
                .collect()
        }))
    }
    fn draw(&mut self, games: &[Value], players: &[Value]) -> Result<Option<usize>> {
        let available = m::available(games, players);
        let rows = games
            .iter()
            .enumerate()
            .map(|(index, g)| crate::hopper_selection::Row {
                weight: n(g, "weight") as i32,
                available: available.contains(&index),
            })
            .collect::<Vec<_>>();
        let draw = crate::hopper_selection::choose(self.draw_seed, &rows)
            .map_err(|_| Error(400, "Invalid hopper weight domain"))?;
        Ok(draw.map(|d| {
            self.draw_seed = d.seed;
            d.index
        }))
    }
    fn host(
        &mut self,
        policy: &Value,
        group: &[Value],
        preferred: Option<&str>,
        required: Option<&str>,
        quality: &Value,
    ) -> Result<Option<String>> {
        if group
            .iter()
            .any(|t| t["expires_at"].as_f64().unwrap() <= self.clock)
        {
            return Ok(None);
        }
        let players = Self::players(group);
        let mut minimum = HashMap::new();
        for t in group {
            let threshold = n(self.stage(policy, t), "minimum_connection");
            for p in array(t, "players") {
                minimum.insert(s(p, "id").to_owned(), threshold);
            }
        }
        let mut hosts = Vec::new();
        let measured = if quality.is_null() {
            None
        } else {
            Some(
                quality
                    .as_object()
                    .ok_or(Error(400, "Invalid measured host quality"))?,
            )
        };
        for p in &players {
            let host = s(p, "id");
            if required.is_some_and(|r| r != host)
                || measured.is_some_and(|q| !q.contains_key(host))
            {
                continue;
            }
            let mut scores = Vec::new();
            let mut valid = true;
            for other in &players {
                let other = s(other, "id");
                if other == host {
                    continue;
                }
                if let Some(&(score, expiry)) =
                    self.connections.get(&(host.to_owned(), other.to_owned()))
                {
                    if expiry > self.clock && score >= minimum[other] {
                        scores.push(score);
                        continue;
                    }
                }
                valid = false;
                break;
            }
            if valid && let Some(worst) = scores.into_iter().min() {
                hosts.push((worst, host.to_owned()));
            }
        }
        if hosts.is_empty() {
            return Ok(None);
        }
        if let Some(quality) = measured {
            let mut ranked = Vec::new();
            for (_, host) in hosts {
                let key = quality[&host]
                    .as_array()
                    .ok_or(Error(400, "Invalid measured host quality"))?
                    .iter()
                    .map(|n| m::integer(n, i64::MIN, i64::MAX))
                    .collect::<Result<Vec<_>>>()?;
                ranked.push((key, host));
            }
            // Stable ties retain admitted player order, as the native adapter.
            return Ok(ranked
                .into_iter()
                .min_by(|a, b| a.0.cmp(&b.0))
                .map(|(_, id)| id));
        }
        let best = hosts.iter().map(|(score, _)| *score).max().unwrap();
        let eligible = hosts
            .into_iter()
            .filter(|(score, _)| *score == best)
            .map(|(_, id)| id)
            .collect::<Vec<_>>();
        if let Some(p) = preferred
            && eligible.iter().any(|id| id == p)
        {
            return Ok(Some(p.to_owned()));
        }
        Ok(Some(eligible[self.random_index(eligible.len())].clone()))
    }
    fn enqueue(&mut self, r: &Value) -> Result<Value> {
        let party = m::text(&r["party_id"], 64)?;
        let pid = m::text(&r["playlist_id"], 512)?;
        let compatibility = m::text(&r["compatibility"], 64)?;
        let p = self.policy(pid)?.clone();
        if let Some(catalog) = &self.catalog {
            let entry = catalog.entry(pid)?;
            reject(
                entry["required_compatibility"]
                    .as_str()
                    .is_none_or(|required| {
                        crate::matchmaking_catalog::supports(compatibility, required)
                    }),
                409,
                "Update Halo 2 for this playlist",
            )?;
            reject(
                p["candidate_selection"].is_null()
                    || crate::matchmaking_catalog::GATHERING.contains(&compatibility),
                409,
                "Update Halo 2 for gathering matchmaking",
            )?;
        }
        let players = r["players"]
            .as_array()
            .ok_or(Error(400, "Trusted player records required"))?;
        m::check(
            !players.is_empty() && players.len() <= n(&p, "maximum_party") as usize,
            "Party not eligible",
        )?;
        let mut ids = HashSet::new();
        let mut units = HashSet::new();
        for player in players {
            m::player(player)?;
            m::check(ids.insert(s(player, "id")), "Duplicate party member")?;
            units.insert(player["raw_rank"].is_null());
        }
        m::check(units.len() == 1, "Mixed search skill units")?;
        reject(
            !(flag(&p, "ranked") || !p["clan_match"].is_null())
                || !players.iter().any(|p| flag(p, "guest")),
            409,
            "Guests may use casual playlists only",
        )?;
        let leader = optional_string(&r["leader"])?;
        m::check(
            leader.is_none_or(|id| ids.contains(id)),
            "Party leader is not a member",
        )?;
        let language = m::integer(&r["language"], 0, 8)?;
        if p["clan_match"].is_null() {
            m::check(
                r["clan"].is_null() && r["clan_rank"].is_null(),
                "Clan ratings require a clan playlist",
            )?;
        } else {
            m::text(&r["clan"], 64)?;
            m::integer(&r["clan_rank"], 0, 127)?;
            m::check(
                (n(&p["clan_match"], "minimum")..=n(&p["clan_match"], "maximum"))
                    .contains(&(players.len() as i64)),
                "Party size does not meet the clan playlist limits",
            )?;
        }
        if flag(&p, "use_2007_rank_limits") {
            for player in players {
                m::rank_radius(
                    n(player, "rank") as i32,
                    p["high_rank_radius"].as_i64().map(|v| v as i32),
                )?;
            }
        }
        m::check(
            !m::available(array(&p, "games"), players).is_empty(),
            "Party has no common map in this playlist",
        )?;
        for t in &self.tickets {
            if s(t, "party_id") == party {
                reject(
                    t["playlist"] == pid
                        && t["compatibility"] == compatibility
                        && t["players"] == r["players"]
                        && self.current_profile(&p, t),
                    409,
                    "Party already queued differently",
                )?;
                reject(
                    t["clan"] == r["clan"] && t["clan_rank"] == r["clan_rank"],
                    409,
                    "Clan search changed",
                )?;
                reject(
                    t["leader"] == r["leader"] && t["language"] == r["language"],
                    409,
                    "Search owner changed",
                )?;
                return Ok(t.clone());
            }
            reject(
                r["clan"].is_null() || t["clan"] != r["clan"] || t["playlist"] != pid,
                409,
                "This clan is already searching this playlist",
            )?;
            reject(
                !array(t, "players").iter().any(|p| ids.contains(s(p, "id"))),
                409,
                "Player already queued",
            )?;
        }
        reject(
            self.tickets.len() < self.capacity as usize,
            503,
            "Queue capacity reached",
        )?;
        let tid = id()?;
        let ticket = json!({"id":tid,"party_id":party,"playlist":pid,"revision":p["revision"],"compatibility":compatibility,
            "players":players,"joined_at":self.clock,"expires_at":self.clock+self.lease as f64,"clan":r["clan"],"clan_rank":r["clan_rank"],"leader":leader,"language":language});
        self.ticket_profiles
            .insert(tid, self.policy_pins[pid].clone());
        self.tickets.push(ticket.clone());
        Ok(ticket)
    }
    fn form(
        &mut self,
        policy: &Value,
        ids: &[String],
        game: Option<&Value>,
        host: Option<String>,
        gathering_host: Option<&str>,
    ) -> Result<Value> {
        m::check(
            !ids.is_empty()
                && ids.len() <= 16
                && ids.iter().collect::<HashSet<_>>().len() == ids.len(),
            "Invalid proposal ticket set",
        )?;
        let group = self.group(ids)?;
        let players = Self::players(&group);
        let count = players.len() as i64;
        if !(n(policy, "minimum_players")..=n(policy, "maximum_players")).contains(&count) {
            return Ok(Value::Null);
        }
        reject(
            group.iter().all(|t| {
                t["playlist"] == policy["id"]
                    && t["expires_at"].as_f64().unwrap() > self.clock
                    && self.current_profile(policy, t)
            }),
            409,
            "Admission profile or ticket changed",
        )?;
        reject(
            !self.proposals.iter().any(|p| {
                array(p, "tickets")
                    .iter()
                    .any(|t| ids.iter().any(|id| t == id))
            }),
            409,
            "Ticket already reserved",
        )?;
        for (index, ticket) in group.iter().enumerate() {
            for other in &group[..index] {
                reject(
                    self.compatible(policy, other, ticket)?,
                    409,
                    "Incompatible proposal admissions",
                )?;
            }
        }
        for owner in [host.as_deref(), gathering_host].into_iter().flatten() {
            m::check(
                players.iter().any(|p| s(p, "id") == owner),
                "Host is not an admitted member",
            )?;
        }
        let Some(teams) = self.teams(policy, &group)? else {
            return Ok(Value::Null);
        };
        let game = if let Some(game) = game {
            reject(
                array(policy, "games").contains(game),
                409,
                "Selected game differs from admitted catalog",
            )?;
            game.clone()
        } else {
            let Some(index) = self.draw(array(policy, "games"), &players)? else {
                return Ok(Value::Null);
            };
            array(policy, "games")[index].clone()
        };
        if m::available(std::slice::from_ref(&game), &players).is_empty() {
            return Ok(Value::Null);
        }
        let ident = id()?;
        let proposal = json!({"id":ident,"playlist":policy["id"],"revision":policy["revision"],"tickets":ids,
            "teams":teams,"host":host,"game":game,"expires_at":self.clock+self.proposal_lease as f64,"gathering_host":gathering_host});
        let binding = json!({"profile_sha256":self.policy_pins[s(policy,"id")],"profile":self.profile_value(policy)?,
            "catalog_sha256":self.catalog.as_ref().map(|c|fingerprint(&json!(c.entries))),"tickets":group,"game":game,
            "party_bindings":ids.iter().map(|id|self.ticket_parties.get(id).cloned().unwrap_or(Value::Null)).collect::<Vec<_>>() });
        self.proposal_bindings.insert(
            ident.clone(),
            json!({"digest":fingerprint(&binding),"bound":binding}),
        );
        self.ready.insert(ident, BTreeSet::new());
        self.proposals.push(proposal.clone());
        Ok(proposal)
    }
    fn refresh(&mut self, ident: &str, optimize: bool, quality: &Value) -> Result<Value> {
        let proposal = self.proposal(ident)?.clone();
        let ids = strings(&proposal["tickets"])?;
        let group = self.group(&ids)?;
        let policy = self.policy(s(&proposal, "playlist"))?.clone();
        reject(
            group.iter().all(|t| self.current_profile(&policy, t)),
            409,
            "Admission profile changed; requeue",
        )?;
        let host = self.host(
            &policy,
            &group,
            proposal["host"].as_str(),
            if optimize {
                None
            } else {
                proposal["gathering_host"].as_str()
            },
            quality,
        )?;
        let index = self
            .proposals
            .iter()
            .position(|p| s(p, "id") == ident)
            .unwrap();
        if proposal["host"] != json!(host) {
            self.proposals[index]["host"] = json!(host);
            self.ready.get_mut(ident).unwrap().clear();
        }
        Ok(self.proposals[index].clone())
    }
    fn subsets(
        &self,
        policy: &Value,
        pool: &[Value],
        group: Vec<Value>,
        start: usize,
        count: usize,
        budget: &mut usize,
        local: &mut usize,
        output: &mut Vec<Vec<Value>>,
    ) -> Result<()> {
        if *budget == 0 || *local == 0 {
            return Ok(());
        }
        *budget -= 1;
        *local -= 1;
        output.push(group.clone());
        if count >= n(policy, "maximum_players") as usize {
            return Ok(());
        }
        for i in start..pool.len() {
            if *budget == 0 || *local == 0 {
                break;
            }
            let ticket = &pool[i];
            let added = array(ticket, "players").len();
            if count + added <= n(policy, "maximum_players") as usize
                && group
                    .iter()
                    .map(|t| self.compatible(policy, t, ticket))
                    .collect::<Result<Vec<_>>>()?
                    .iter()
                    .all(|v| *v)
            {
                let mut next = group.clone();
                next.push(ticket.clone());
                self.subsets(
                    policy,
                    pool,
                    next,
                    i + 1,
                    count + added,
                    budget,
                    local,
                    output,
                )?;
            }
        }
        Ok(())
    }
    fn advance(&mut self, playlist: &str, pending: bool) -> Result<Value> {
        let p = self.policy(playlist)?.clone();
        m::check(
            p["candidate_selection"].is_null(),
            "Use the gathering-session owner for this playlist",
        )?;
        let reserved = self
            .proposals
            .iter()
            .flat_map(|p| {
                array(p, "tickets")
                    .iter()
                    .map(|t| t.as_str().unwrap().to_owned())
            })
            .collect::<HashSet<_>>();
        let mut queue = self
            .tickets
            .iter()
            .filter(|t| t["playlist"] == playlist && !reserved.contains(s(t, "id")))
            .cloned()
            .collect::<Vec<_>>();
        queue.sort_by(|a, b| {
            a["joined_at"]
                .as_f64()
                .unwrap()
                .total_cmp(&b["joined_at"].as_f64().unwrap())
                .then_with(|| s(a, "id").cmp(s(b, "id")))
        });
        if queue.is_empty() {
            return Ok(Value::Null);
        }
        let start = queue
            .iter()
            .position(|t| {
                self.next_anchor
                    .get(playlist)
                    .is_some_and(|id| id == s(t, "id"))
            })
            .unwrap_or(0);
        queue.rotate_left(start);
        let mut budget = self.search_budget as usize;
        for (index, anchor) in queue.iter().enumerate() {
            self.next_anchor.insert(
                playlist.to_owned(),
                s(&queue[(index + 1) % queue.len()], "id").to_owned(),
            );
            let mut pool = Vec::new();
            for t in &queue {
                if t["id"] != anchor["id"] && self.compatible(&p, anchor, t)? {
                    pool.push(t.clone());
                }
            }
            let size = pool.len().min(49);
            for i in 0..size {
                let selected = i + self.random_index(pool.len() - i);
                pool.swap(i, selected);
            }
            pool.truncate(size);
            let mut local = budget.min(512);
            let mut groups = Vec::new();
            self.subsets(
                &p,
                &pool,
                vec![anchor.clone()],
                0,
                array(anchor, "players").len(),
                &mut budget,
                &mut local,
                &mut groups,
            )?;
            for group in groups {
                let count = Self::players(&group).len() as i64;
                if !(n(&p, "minimum_players")..=n(&p, "maximum_players")).contains(&count) {
                    continue;
                }
                if self.teams(&p, &group)?.is_none() {
                    continue;
                }
                let host = self.host(&p, &group, None, None, &Value::Null)?;
                if host.is_some() || pending {
                    let ids = group
                        .iter()
                        .map(|t| s(t, "id").to_owned())
                        .collect::<Vec<_>>();
                    let proposal = self.form(&p, &ids, None, host, None)?;
                    if !proposal.is_null() {
                        return Ok(proposal);
                    }
                }
            }
            if budget == 0 {
                break;
            }
        }
        Ok(Value::Null)
    }

    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        self.clock = self.clock.max(m::real(&r["now"])?);
        let op = m::text(&r["op"], 64)?;
        if op.starts_with("search_qos_") {
            let mut owner = std::mem::take(&mut self.search_qos);
            let result = owner.execute(self, r);
            self.search_qos = owner;
            return result;
        }
        if op.starts_with("room_setup_")
            || op.starts_with("room_continuity_")
            || op.starts_with("room_state_")
            || op.starts_with("room_wire_")
            || op.starts_with("room_packets_")
            || op.starts_with("room_control_")
        {
            let mut registry = std::mem::take(&mut self.setup);
            let result = registry.execute(self, r);
            self.setup = registry;
            return result;
        }
        if op.starts_with("api_") {
            let mut registry = std::mem::take(&mut self.api);
            let result = registry.execute(self, r);
            self.api = registry;
            return result;
        }
        if op.starts_with("party_") {
            let mut registry = std::mem::take(&mut self.parties);
            let result = registry.execute(self, r);
            self.parties = registry;
            return result;
        }
        if op.starts_with("session_") {
            let mut registry = std::mem::take(&mut self.sessions);
            let result = registry.execute(self, r);
            self.sessions = registry;
            return result;
        }
        if op.starts_with("result_") {
            let mut registry = std::mem::take(&mut self.results);
            let result = registry.execute(self, r);
            self.results = registry;
            return result;
        }
        if op.starts_with("gather_") {
            if [
                "gather_advance",
                "gather_query",
                "gather_found",
                "gather_cleanup",
                "gather_resume",
            ]
            .contains(&op)
            {
                self.cleanup();
            }
            let mut registry = std::mem::take(&mut self.gatherings);
            let result = registry.execute(self, r);
            self.gatherings = registry;
            return result;
        }
        if ![
            "now",
            "keys",
            "len",
            "get",
            "setting_get",
            "setting_set",
            "policy_put",
            "policies_set",
            "catalog_set",
            "catalog_get",
            "catalog_display_requests",
            "catalog_public_view",
            "catalog_eligible",
            "catalog_clan_group",
            "draw",
            "teams",
            "compatible",
            "fits",
            "stage",
            "host",
            "admission",
        ]
        .contains(&op)
        {
            self.cleanup();
        }
        match op {
            "now" => Ok(json!(self.clock)),
            "cleanup" => Ok(Value::Null),
            "enqueue" => self.enqueue(r),
            "heartbeat" => {
                let ident = m::text(&r["id"], 64)?;
                let ticket = self.ticket(ident)?;
                reject(
                    self.current_profile(self.policy(s(ticket, "playlist"))?, ticket),
                    409,
                    "Admission profile changed; requeue",
                )?;
                let index = self
                    .tickets
                    .iter()
                    .position(|t| s(t, "id") == ident)
                    .unwrap();
                self.tickets[index]["expires_at"] = json!(self.clock + self.lease as f64);
                Ok(Value::Null)
            }
            "cancel" => {
                self.cancel_ticket(m::text(&r["id"], 64)?)?;
                Ok(Value::Null)
            }
            "connection" => {
                let host = m::text(&r["host"], 64)?.to_owned();
                let player = m::text(&r["player"], 64)?.to_owned();
                let score = m::integer(&r["score"], 0, 100)?;
                let active = self
                    .tickets
                    .iter()
                    .flat_map(|t| array(t, "players").iter().map(|p| s(p, "id")))
                    .collect::<HashSet<_>>();
                reject(
                    active.contains(host.as_str()) && active.contains(player.as_str()),
                    404,
                    "QoS identity is not queued",
                )?;
                reject(
                    self.connections
                        .contains_key(&(host.clone(), player.clone()))
                        || self.connections.len() < (self.capacity * 16 * 50) as usize,
                    503,
                    "Connection measurement capacity reached",
                )?;
                self.connections
                    .insert((host, player), (score, self.clock + self.qos_lease as f64));
                Ok(Value::Null)
            }
            "compatible" => {
                let p = self.policy(m::text(&r["playlist"], 512)?)?;
                Ok(json!(self.compatible(
                    p,
                    self.ticket(m::text(&r["a"], 64)?)?,
                    self.ticket(m::text(&r["b"], 64)?)?
                )?))
            }
            "teams" => {
                let group = self.group(&strings(&r["tickets"])?)?;
                Ok(json!(self.teams(
                    self.policy(m::text(&r["playlist"], 512)?)?,
                    &group
                )?))
            }
            "stage" => Ok(self
                .stage(
                    self.policy(m::text(&r["playlist"], 512)?)?,
                    self.ticket(m::text(&r["ticket"], 64)?)?,
                )
                .clone()),
            "host" => {
                let group = self.group(&strings(&r["tickets"])?)?;
                let p = self.policy(m::text(&r["playlist"], 512)?)?.clone();
                Ok(json!(self.host(
                    &p,
                    &group,
                    optional_string(&r["preferred"])?,
                    optional_string(&r["required"])?,
                    &r["quality"]
                )?))
            }
            "draw" => {
                let games = r["games"]
                    .as_array()
                    .ok_or(Error(400, "Invalid hopper rows"))?;
                let players = r["players"]
                    .as_array()
                    .ok_or(Error(400, "Invalid draw players"))?;
                m::check(
                    (1..=100).contains(&games.len()) && players.len() <= 16,
                    "Invalid draw bounds",
                )?;
                for g in games {
                    m::integer(&g["weight"], i32::MIN as i64, i32::MAX as i64)?;
                    m::text(&g["map"], 64)?;
                }
                for p in players {
                    m::player(p)?;
                }
                Ok(json!(self.draw(games, players)?))
            }
            "form" => {
                let p = self.policy(m::text(&r["playlist"], 512)?)?.clone();
                self.form(
                    &p,
                    &strings(&r["tickets"])?,
                    if r["game"].is_null() {
                        None
                    } else {
                        Some(&r["game"])
                    },
                    None,
                    optional_string(&r["gathering_host"])?,
                )
            }
            "advance" => self.advance(m::text(&r["playlist"], 512)?, m::boolean(&r["pending"])?),
            "refresh" => self.refresh(
                m::text(&r["id"], 64)?,
                m::boolean(&r["optimize"])?,
                &r["quality"],
            ),
            "acknowledge" => {
                let ident = m::text(&r["id"], 64)?;
                let player = m::text(&r["player"], 64)?;
                let ids = strings(&self.proposal(ident)?["tickets"])?;
                let group = self.group(&ids)?;
                let members = Self::players(&group)
                    .iter()
                    .map(|p| s(p, "id").to_owned())
                    .collect::<BTreeSet<_>>();
                reject(
                    members.contains(player),
                    403,
                    "Player is not in this proposal",
                )?;
                // Authentication precedes refresh: an outsider must not draw
                // a host, clear readiness or otherwise mutate a reservation.
                let proposal = self.refresh(ident, m::boolean(&r["optimize"])?, &r["quality"])?;
                reject(
                    !proposal["host"].is_null(),
                    409,
                    "Connectivity has not been verified",
                )?;
                let ready = self.ready.get_mut(ident).unwrap();
                ready.insert(player.to_owned());
                if *ready != members {
                    return Ok(Value::Null);
                }
                self.tickets
                    .retain(|t| !ids.iter().any(|id| id == s(t, "id")));
                for id in ids {
                    self.ticket_profiles.remove(&id);
                    self.ticket_parties.remove(&id);
                }
                self.remove_proposal(ident);
                self.cleanup();
                Ok(proposal)
            }
            "status" => {
                let ident = m::text(&r["id"], 64)?;
                self.ticket(ident)?;
                if let Some(p) = self
                    .proposals
                    .iter()
                    .find(|p| array(p, "tickets").iter().any(|t| t == ident))
                {
                    Ok(
                        json!({"state":if p["host"].is_null(){"checking_connections"}else{"awaiting_transport"},"proposal_id":p["id"],"expires_in":(p["expires_at"].as_f64().unwrap()-self.clock).ceil().max(0.) as u64}),
                    )
                } else {
                    Ok(json!({"state":"searching"}))
                }
            }
            "keys" => self.keys(
                m::text(&r["map"], 64)?,
                m::integer(&r["offset"], 0, 1_000_000)? as usize,
                m::integer(&r["limit"], 1, 1000)? as usize,
            ),
            "len" => self.length(m::text(&r["map"], 64)?),
            "get" => self.get(m::text(&r["map"], 64)?, &r["key"]),
            "connection_delete" => {
                let pair = strings(&r["key"])?;
                m::check(pair.len() == 2, "Invalid connection key")?;
                Ok(json!(
                    self.connections.remove(&(pair[0].clone(), pair[1].clone()))
                ))
            }
            "catalog_get" => {
                let catalog = self
                    .catalog
                    .as_ref()
                    .ok_or(Error(503, "Admitted service catalog required"))?;
                if r["id"].is_null() {
                    Ok(json!(catalog.entries))
                } else {
                    Ok(catalog.entry(m::text(&r["id"], 64)?)?.clone())
                }
            }
            "catalog_set" => {
                let catalog = Catalog::new(&r["entries"])?;
                self.replace_policies(&catalog.policies(), Some(catalog))?;
                Ok(Value::Null)
            }
            "catalog_display_requests" => Ok(json!(
                self.catalog
                    .as_ref()
                    .ok_or(Error(503, "Admitted service catalog required"))?
                    .display_requests()
            )),
            "catalog_public_view" => self
                .catalog
                .as_ref()
                .ok_or(Error(503, "Admitted service catalog required"))?
                .public_view(
                    m::text(&r["compatibility"], 64)?,
                    &r["connections"],
                    &r["levels"],
                ),
            "catalog_eligible" => {
                let catalog = self
                    .catalog
                    .as_ref()
                    .ok_or(Error(503, "Admitted service catalog required"))?;
                let entry = catalog.entry(m::text(&r["id"], 64)?)?;
                Ok(json!(crate::matchmaking_catalog::eligible(
                    entry,
                    m::text(&r["compatibility"], 64)?,
                    &r["connections"]
                )?))
            }
            "catalog_clan_group" => {
                let p = self.policy(m::text(&r["id"], 64)?)?;
                let seats = r["seats"]
                    .as_array()
                    .ok_or(Error(400, "Clan seats required"))?;
                m::check(seats.len() <= 16, "Invalid clan seat count")?;
                crate::matchmaking_catalog::clan_group(&p["clan_match"], seats)
            }
            "policy_put" => {
                let p = &r["value"];
                m::playlist(p)?;
                let key = m::text(&r["key"], 512)?;
                m::check(s(p, "id") == key, "Policy identity differs")?;
                let mut policies = self.playlists.clone();
                if let Some(index) = policies.iter().position(|p| s(p, "id") == key) {
                    policies[index] = p.clone();
                } else {
                    m::check(policies.len() < 64, "Catalog capacity reached")?;
                    policies.push(p.clone());
                }
                let catalog = self
                    .catalog
                    .as_ref()
                    .map(|c| c.with_policies(&policies))
                    .transpose()?;
                self.replace_policies(&policies, catalog)?;
                Ok(Value::Null)
            }
            "policies_set" => {
                let list = r["values"]
                    .as_array()
                    .ok_or(Error(400, "Invalid catalog"))?;
                m::check((1..=64).contains(&list.len()), "Invalid catalog")?;
                let mut ids = HashSet::new();
                for p in list {
                    m::playlist(p)?;
                    m::check(ids.insert(s(p, "id")), "Duplicate playlist")?;
                }
                let catalog = self
                    .catalog
                    .as_ref()
                    .map(|c| c.with_policies(list))
                    .transpose()?;
                self.replace_policies(list, catalog)?;
                Ok(Value::Null)
            }
            "setting_get" => self.setting(m::text(&r["name"], 64)?),
            "setting_set" => {
                let name = m::text(&r["name"], 64)?;
                let (low, high) = match name {
                    "lease" => (1, 300),
                    "proposal_lease" | "qos_lease" => (1, 120),
                    "capacity" => (1, 1000),
                    "search_budget" => (1, 100000),
                    "draw_seed" => (0, u32::MAX as i64),
                    _ => return Err(Error(400, "Unknown coordinator setting")),
                };
                let v = m::integer(&r["value"], low, high)?;
                match name {
                    "lease" => self.lease = v,
                    "proposal_lease" => self.proposal_lease = v,
                    "qos_lease" => self.qos_lease = v,
                    "capacity" => self.capacity = v,
                    "search_budget" => self.search_budget = v,
                    "draw_seed" => self.draw_seed = v as u32,
                    _ => unreachable!(),
                }
                Ok(Value::Null)
            }
            "admission" => self
                .proposal_bindings
                .get(m::text(&r["id"], 64)?)
                .cloned()
                .ok_or(Error(404, "Proposal not found")),
            "fits" => self.fits(r),
            _ => Err(Error(400, "Unknown coordinator operation")),
        }
    }
    fn setting(&self, name: &str) -> Result<Value> {
        Ok(match name {
            "lease" => json!(self.lease),
            "proposal_lease" => json!(self.proposal_lease),
            "qos_lease" => json!(self.qos_lease),
            "capacity" => json!(self.capacity),
            "search_budget" => json!(self.search_budget),
            "draw_seed" => json!(self.draw_seed),
            _ => return Err(Error(400, "Unknown coordinator setting")),
        })
    }
    fn length(&self, map: &str) -> Result<Value> {
        Ok(json!(match map {
            "playlists" => self.playlists.len(),
            "tickets" => self.tickets.len(),
            "proposals" | "ready" => self.proposals.len(),
            "connections" => self.connections.len(),
            "next_anchor" => self.next_anchor.len(),
            _ => return Err(Error(400, "Unknown coordinator map")),
        }))
    }
    fn keys(&self, map: &str, offset: usize, limit: usize) -> Result<Value> {
        Ok(match map {
            "playlists" => json!(
                self.playlists
                    .iter()
                    .map(|p| s(p, "id"))
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            "tickets" => json!(
                self.tickets
                    .iter()
                    .map(|p| s(p, "id"))
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            "proposals" => json!(
                self.proposals
                    .iter()
                    .map(|p| s(p, "id"))
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            "ready" => json!(
                self.proposals
                    .iter()
                    .map(|p| s(p, "id"))
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            "connections" => json!(
                self.connections
                    .keys()
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            "next_anchor" => json!(
                self.next_anchor
                    .keys()
                    .skip(offset)
                    .take(limit)
                    .collect::<Vec<_>>()
            ),
            _ => return Err(Error(400, "Unknown coordinator map")),
        })
    }
    fn get(&self, map: &str, key: &Value) -> Result<Value> {
        let missing = Error(404, "Coordinator key not found");
        Ok(match map {
            "playlists" => self.policy(m::text(key, 512)?)?.clone(),
            "tickets" => self.ticket(m::text(key, 64)?)?.clone(),
            "proposals" => self.proposal(m::text(key, 64)?)?.clone(),
            "ready" => json!(self.ready.get(m::text(key, 64)?).ok_or(missing)?),
            "connections" => {
                let pair = strings(key)?;
                m::check(pair.len() == 2, "Invalid connection key")?;
                json!(
                    self.connections
                        .get(&(pair[0].clone(), pair[1].clone()))
                        .ok_or(missing)?
                )
            }
            "next_anchor" => json!(self.next_anchor.get(m::text(key, 512)?).ok_or(missing)?),
            _ => return Err(Error(400, "Unknown coordinator map")),
        })
    }
    fn fits(&self, r: &Value) -> Result<Value> {
        let p = self.policy(m::text(&r["playlist"], 512)?)?;
        if !r["game"].is_null() {
            m::game(&r["game"], n(p, "team_size") > 0)?;
            if !array(p, "games").contains(&r["game"]) {
                return Ok(json!(false));
            }
        }
        let group = self.group(&strings(&r["tickets"])?)?;
        let ticket = self.ticket(m::text(&r["ticket"], 64)?)?;
        for other in &group {
            if !self.compatible(p, other, ticket)? {
                return Ok(json!(false));
            }
        }
        let mut all = group;
        all.push(ticket.clone());
        let players = Self::players(&all);
        if m::available(array(p, "games"), &players).is_empty()
            || players.len() > n(p, "maximum_players") as usize
        {
            return Ok(json!(false));
        }
        if !r["game"].is_null()
            && m::available(std::slice::from_ref(&r["game"]), array(ticket, "players")).is_empty()
        {
            return Ok(json!(false));
        }
        if !p["clan_match"].is_null() {
            return Ok(json!(all.len() == 2));
        }
        if n(p, "team_size") == 0 {
            return Ok(json!(true));
        }
        let size = n(p, "team_size") as usize;
        let count = n(p, "team_count") as usize;
        if count > 2 {
            let parties = all
                .iter()
                .enumerate()
                .flat_map(|(i, t)| std::iter::repeat_n(i as i32, array(t, "players").len()))
                .collect::<Vec<_>>();
            return Ok(json!([true, false].into_iter().any(|preserve| {
                crate::team_balance::can_add(
                    crate::team_balance::Options {
                        teams: count,
                        maximum_team_size: size,
                        flag_parties: false,
                        flag_minimum: 0,
                        flag_maximum: 0,
                        use_parties: preserve,
                        optimize: false,
                    },
                    &parties,
                    0,
                    0,
                )
            })));
        }
        let mut possible = 1_u32;
        for t in &all {
            possible |= possible << array(t, "players").len();
        }
        Ok(json!(
            !flag(p, "ranked")
                || (0..=size).any(|left| possible & (1 << left) != 0
                    && players.len().saturating_sub(left) <= size)
        ))
    }
}

pub fn value(r: &Value) -> Result<Value> {
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("party_") && op != "party_rank")
    {
        return parties::value(r);
    }

    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("session_"))
    {
        return sessions::value(r);
    }

    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("catalog_"))
    {
        return crate::matchmaking_catalog::value(r);
    }
    if r["op"].as_str().is_some_and(|op| op.starts_with("result_")) {
        return results::value(r);
    }
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("rank_") && op != "rank_radius")
    {
        return crate::matchmaking_skill::value(r);
    }
    match m::text(&r["op"], 64)? {
        "defaults" => Ok(m::defaults()),
        "gather_tick" => Ok(json!(gatherings::tick(m::real(&r["value"])?)?)),
        "validate_owner" => {
            Coordinator::new(&r["value"])?;
            Ok(json!(true))
        }
        "search_rank" => {
            m::player(&r["player"])?;
            Ok(json!(m::search_rank(&r["player"])))
        }
        "validate" => {
            match m::text(&r["kind"], 64)? {
                "playlist" => m::playlist(&r["value"])?,
                "player" => m::player(&r["value"])?,
                _ => return Err(Error(400, "Unknown matchmaking model")),
            };
            Ok(json!(true))
        }
        "rank_radius" => Ok(json!(m::rank_radius(
            m::integer(&r["rank"], 1, 50)? as i32,
            if r["high"].is_null() {
                None
            } else {
                Some(m::integer(&r["high"], 0, 49)? as i32)
            }
        )?)),
        "radius" => {
            m::playlist(&r["policy"])?;
            Ok(json!(Coordinator::radius(
                &r["policy"],
                m::integer(&r["rank"], 0, 127)? as i32,
                m::boolean(&r["raw"])?
            )?))
        }
        "party_rank" | "effective_ranks" => {
            m::playlist(&r["policy"])?;
            let players = r["players"]
                .as_array()
                .ok_or(Error(400, "Invalid players"))?;
            for p in players {
                m::player(p)?;
            }
            if r["op"] == "party_rank" {
                return Ok(json!(m::party_rank(players)?));
            }
            let mut ranks = players.iter().map(m::search_rank).collect::<Vec<_>>();
            if flag(&r["policy"], "mixed_party_floor") && !ranks.is_empty() {
                let high = *ranks.iter().max().unwrap();
                let displayed = if players[0]["raw_rank"].is_null() {
                    high
                } else {
                    high.min(49) + 1
                };
                let floor = high
                    - m::rank_radius(
                        displayed,
                        r["policy"]["high_rank_radius"].as_i64().map(|v| v as i32),
                    )?;
                for rank in &mut ranks {
                    *rank = (*rank).max(floor);
                }
            }
            Ok(json!(ranks))
        }
        "available" => {
            let games = r["game_maps"]
                .as_array()
                .ok_or(Error(400, "Invalid maps"))?;
            let players = r["maps"].as_array().ok_or(Error(400, "Invalid players"))?;
            m::check(
                games.len() <= 100 && players.len() <= 16,
                "Invalid map filter bounds",
            )?;
            for g in games {
                m::text(g, 64)?;
            }
            for p in players {
                let maps = p.as_array().ok_or(Error(400, "Invalid map set"))?;
                m::check(maps.len() <= 256, "Invalid map set")?;
                for map in maps {
                    m::text(map, 64)?;
                }
            }
            Ok(json!(
                games
                    .iter()
                    .enumerate()
                    .filter_map(|(i, g)| (!players.is_empty()
                        && players.iter().all(|p| p.as_array().unwrap().contains(g)))
                    .then_some(i))
                    .collect::<Vec<_>>()
            ))
        }
        _ => Err(Error(400, "Unknown matchmaking value operation")),
    }
}

pub fn decode(data: &[u8]) -> Result<Value> {
    m::check(data.len() <= REQUEST_LIMIT, "Oversize matchmaking carrier")?;
    crate::matchmaking_json::parse(data, REQUEST_LIMIT)
}
