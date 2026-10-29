//! Retail search/gather clocks, completion progress and failed-session memory.
//!
//! Original owners: 70190, 70D60/70EA0, 90F70, 71730, 1931D0,
//! 91290/912D0. The native role adapter, private handle generations and
//! birth/id provider are explicit modern integration, not Xbox Live emulation.
use serde_json::{Value, json};

pub const SEEN_CAPACITY: usize = 150;
pub const QUERY_CAPACITY: usize = 50;
pub const REQUEST_LIMIT: usize = 256 * 1024;
pub const RESPONSE_LIMIT: usize = 64 * 1024;
pub type Result<T> = std::result::Result<T, &'static str>;

pub fn elapsed(now: u32, start: u32) -> i32 {
    now.wrapping_sub(start) as i32
}

pub fn jitter(mut seed: u32, search_spread: i32, gather_spread: i32) -> (u32, i32, i32) {
    let mut values = [0; 2];
    for (output, spread) in values.iter_mut().zip([search_spread, gather_spread]) {
        seed = seed.wrapping_mul(1664525).wrapping_add(1013904223);
        let signed_spread = i32::from(spread as i16);
        let product = (seed >> 16).wrapping_mul(signed_spread as u32);
        *output = i32::from((product >> 16) as i16);
    }
    (seed, values[0], values[1])
}

pub fn role(now: u32, search_start: u32, gather_start: u32, capacity: i32) -> &'static str {
    // The native relay supplies no Xbox NAT kind. Original optional restricted
    // NAT policy is disabled by the executable defaults; do not invent it.
    if search_start == 0
        || elapsed(now, search_start) >= elapsed(now, gather_start)
        || capacity == -1
    {
        "search"
    } else {
        "gather"
    }
}

pub fn search_expired(now: u32, start: u32, offset: i32) -> bool {
    elapsed(now, start) > 240000_i32.wrapping_add(offset)
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SearchProgress {
    pub maximum: i32,
    pub ranked: bool,
    pub radius: i32,
    pub total: i32,
    pub active: bool,
}

impl SearchProgress {
    pub fn new(maximum: i32, ranked: bool, radius: i32, total: i32, active: bool) -> Result<Self> {
        if !(0..=127).contains(&maximum)
            || radius < -1
            || radius > maximum
            || (ranked && radius == -1)
        {
            return Err("invalid search progress");
        }
        Ok(Self {
            maximum,
            ranked,
            radius,
            total,
            active,
        })
    }

    pub fn complete(&mut self, count: i32, pending: [bool; 4]) -> Result<bool> {
        if !self.active || pending.into_iter().any(|n| n) {
            return Ok(false);
        }
        if !(0..=QUERY_CAPACITY as i32).contains(&count) {
            return Err("invalid query count");
        }
        self.total = self.total.wrapping_add(count);
        let mut changed = false;
        if self.total < SEEN_CAPACITY as i32 {
            if self.radius != -1 {
                if self.radius < self.maximum {
                    self.radius += 1;
                    changed = true;
                } else if self.radius == self.maximum && !self.ranked {
                    self.radius = -1;
                    changed = true;
                }
            }
            if count == QUERY_CAPACITY as i32 || changed {
                return Ok(true);
            }
        }
        self.active = false;
        Ok(false)
    }

    pub fn snapshot(&self) -> Value {
        json!({"maximum":self.maximum,"ranked":self.ranked,"radius":self.radius,"total":self.total,"active":self.active})
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SeenSessions<T> {
    values: Vec<T>,
}

impl<T: PartialEq> Default for SeenSessions<T> {
    fn default() -> Self {
        Self { values: Vec::new() }
    }
}

impl<T: PartialEq> SeenSessions<T> {
    pub fn reject(&mut self, identity: T) {
        if self.values.len() < SEEN_CAPACITY && !self.values.contains(&identity) {
            self.values.push(identity);
        }
    }
    pub fn contains(&self, identity: &T) -> bool {
        self.values.contains(identity)
    }
    pub fn clear(&mut self) {
        self.values.clear();
    }
    pub fn values(&self) -> &[T] {
        &self.values
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GatheringConfiguration {
    pub countdown_ms: [i32; 16],
    pub timeout_ms: i32,
    pub provenance: String,
}

impl GatheringConfiguration {
    pub fn validate(&self) -> Result<()> {
        if self.countdown_ms.iter().any(|&n| n < 0)
            || !(10000..=3600000).contains(&self.timeout_ms)
            || self.provenance.is_empty()
            || self.provenance.chars().count() > 512
        {
            return Err("invalid explicit gathering configuration");
        }
        Ok(())
    }

    pub fn from_json(value: &Value) -> Result<Self> {
        fields(value, &["countdown_ms", "timeout_ms", "provenance"])?;
        let values = value["countdown_ms"]
            .as_array()
            .ok_or("invalid countdown carrier")?;
        if values.len() != 16 {
            return Err("sixteen countdown values required");
        }
        let mut countdown_ms = [0; 16];
        for (output, input) in countdown_ms.iter_mut().zip(values) {
            *output = integer(input)?;
        }
        let config = Self {
            countdown_ms,
            timeout_ms: i32_field(value, "timeout_ms")?,
            provenance: string(value, "provenance", 512)?.to_owned(),
        };
        config.validate()?;
        Ok(config)
    }

    pub fn delay(&self, players: i32, minimum: i32, maximum: i32) -> Result<i32> {
        if !(1..=16).contains(&minimum) || !(minimum..=16).contains(&maximum) {
            return Err("invalid population bounds");
        }
        Ok(if players < minimum || players > maximum {
            i32::MAX
        } else if players == maximum {
            0
        } else {
            self.countdown_ms[players as usize - 1]
        })
    }

    pub fn snapshot(&self) -> Value {
        json!({"countdown_ms":self.countdown_ms,"timeout_ms":self.timeout_ms,"provenance":self.provenance})
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GatheringClock {
    pub started: u32,
    pub growth: u32,
    pub peak: i32,
}

impl GatheringClock {
    pub fn new(started: u32, growth: u32, peak: i32) -> Result<Self> {
        if !(0..=16).contains(&peak) {
            return Err("invalid historical peak");
        }
        Ok(Self {
            started,
            growth,
            peak,
        })
    }
    pub fn observe(&mut self, now: u32, players: i32) -> Result<i32> {
        if !(0..=16).contains(&players) {
            return Err("invalid admitted player count");
        }
        if players > self.peak {
            self.peak = players;
            self.growth = now;
        }
        Ok(elapsed(now, self.growth))
    }
    pub fn expired(&self, now: u32, timeout_ms: i32, offset: i32) -> bool {
        elapsed(now, self.growth) >= timeout_ms.wrapping_add(offset)
    }
    pub fn remaining(
        &self,
        now: u32,
        config: &GatheringConfiguration,
        players: i32,
        minimum: i32,
        maximum: i32,
    ) -> Result<Option<i64>> {
        let delay = config.delay(players, minimum, maximum)?;
        if players < minimum || players > maximum {
            return Ok(None);
        }
        let remaining = i64::from(delay) - i64::from(elapsed(now, self.growth));
        Ok((remaining > 0).then_some(remaining))
    }
    pub fn ready(
        &mut self,
        now: u32,
        config: &GatheringConfiguration,
        players: i32,
        minimum: i32,
        maximum: i32,
    ) -> Result<bool> {
        // The INTMAX invalid-population sentinel is an explicit original gate.
        let delay = config.delay(players, minimum, maximum)?;
        let waited = self.observe(now, players)?;
        Ok(delay != i32::MAX && waited >= delay)
    }
    pub fn snapshot(&self) -> Value {
        json!({"started":self.started,"growth":self.growth,"peak":self.peak})
    }
}

#[derive(Clone, Debug)]
pub struct PartyState {
    mode: &'static str,
    search_start: u32,
    gather_start: u32,
    search_offset: i32,
    gather_offset: i32,
    generation: u64,
    progress: Option<SearchProgress>,
    seen: SeenSessions<String>,
}

impl PartyState {
    pub fn new(search_offset: i32, gather_offset: i32) -> Self {
        Self {
            mode: "choose",
            search_start: 0,
            gather_start: 0,
            search_offset,
            gather_offset,
            generation: 0,
            progress: None,
            seen: SeenSessions::default(),
        }
    }
    pub fn start_search(&mut self, now: u32, maximum: i32, ranked: bool) -> Result<()> {
        let progress = SearchProgress::new(maximum, ranked, 0, 0, true)?;
        let generation = self
            .generation
            .checked_add(1)
            .ok_or("search generation exhausted")?;
        self.mode = "search";
        self.search_start = now;
        self.progress = Some(progress);
        self.generation = generation;
        self.seen.clear();
        Ok(())
    }
    pub fn start_gather(&mut self, now: u32) {
        self.mode = "gather";
        self.gather_start = now;
    }
    pub fn mode(&self) -> &'static str {
        self.mode
    }
    pub fn progress(&self) -> Option<&SearchProgress> {
        self.progress.as_ref()
    }
    pub fn seen(&self) -> &SeenSessions<String> {
        &self.seen
    }
    pub fn reject(&mut self, id: String) {
        self.seen.reject(id);
    }
    pub fn choose(&mut self) {
        self.mode = "choose";
    }
    pub fn joined(&mut self) {
        self.mode = "joined";
    }
    pub fn gather_offset(&self) -> i32 {
        self.gather_offset
    }
    pub fn role(&self, now: u32, capacity: i32) -> &'static str {
        role(now, self.search_start, self.gather_start, capacity)
    }
    pub fn search_expired(&self, now: u32) -> bool {
        search_expired(now, self.search_start, self.search_offset)
    }
    pub fn complete_query(&mut self, count: i32) -> Result<bool> {
        let again = self
            .progress
            .as_mut()
            .ok_or("search not initialized")?
            .complete(count, [false; 4])?;
        if !again {
            self.choose();
        }
        Ok(again)
    }
    pub fn snapshot(&self) -> Value {
        json!({"mode":self.mode,"search_start":self.search_start,"gather_start":self.gather_start,
            "search_offset":self.search_offset,"gather_offset":self.gather_offset,"generation":self.generation,
            "progress":self.progress.as_ref().map(SearchProgress::snapshot),"seen":self.seen.values()})
    }
}

/// Explicit modern replacement for an unrecovered original Live procedure.
/// The caller supplies structurally admitted sessions; this boundary owns
/// birth/private-ID order and the native client's 50-record bank truncation.
pub fn native_birth_id_order(mut rows: Vec<(i64, String)>) -> Result<Vec<String>> {
    if rows.len() > 1000 || rows.iter().any(|(_, id)| id.is_empty() || id.len() > 128) {
        return Err("invalid native provider rows");
    }
    let mut identities = std::collections::HashSet::new();
    if rows.iter().any(|(_, id)| !identities.insert(id.clone())) {
        return Err("duplicate native provider identity");
    }
    rows.sort_by(|a, b| a.0.cmp(&b.0).then_with(|| a.1.cmp(&b.1)));
    Ok(rows
        .into_iter()
        .take(QUERY_CAPACITY)
        .map(|(_, id)| id)
        .collect())
}

fn fields(value: &Value, keys: &[&str]) -> Result<()> {
    let object = value.as_object().ok_or("object carrier required")?;
    if object.len() != keys.len() || keys.iter().any(|key| !object.contains_key(*key)) {
        return Err("invalid carrier fields");
    }
    Ok(())
}
fn integer(value: &Value) -> Result<i32> {
    i32::try_from(value.as_i64().ok_or("integer required")?).map_err(|_| "integer out of range")
}
fn i32_field(value: &Value, key: &str) -> Result<i32> {
    integer(&value[key])
}
fn u32_field(value: &Value, key: &str) -> Result<u32> {
    u32::try_from(value[key].as_u64().ok_or("tick/seed required")?)
        .map_err(|_| "tick/seed out of range")
}
fn boolean(value: &Value, key: &str) -> Result<bool> {
    value[key].as_bool().ok_or("boolean required")
}
fn string<'a>(value: &'a Value, key: &str, maximum: usize) -> Result<&'a str> {
    let text = value[key].as_str().ok_or("string required")?;
    if text.is_empty() || text.chars().count() > maximum {
        return Err("invalid bounded string");
    }
    Ok(text)
}
pub fn decode(data: &[u8]) -> Result<Value> {
    if data.len() > REQUEST_LIMIT {
        return Err("oversize private carrier");
    }
    serde_json::from_slice(data).map_err(|_| "invalid private JSON")
}

pub fn value(request: &Value) -> Result<Value> {
    match string(request, "op", 64)? {
        "elapsed" => {
            fields(request, &["op", "now", "start"])?;
            Ok(json!(elapsed(
                u32_field(request, "now")?,
                u32_field(request, "start")?
            )))
        }
        "jitter" => {
            fields(request, &["op", "seed", "search_spread", "gather_spread"])?;
            let (seed, search, gather) = jitter(
                u32_field(request, "seed")?,
                if request["search_spread"].is_null() {
                    60000
                } else {
                    i32_field(request, "search_spread")?
                },
                if request["gather_spread"].is_null() {
                    60000
                } else {
                    i32_field(request, "gather_spread")?
                },
            );
            Ok(json!([seed, search, gather]))
        }
        "role" => {
            fields(
                request,
                &["op", "now", "search_start", "gather_start", "capacity"],
            )?;
            Ok(json!(role(
                u32_field(request, "now")?,
                u32_field(request, "search_start")?,
                u32_field(request, "gather_start")?,
                i32_field(request, "capacity")?
            )))
        }
        "config_valid" => {
            fields(request, &["op", "config"])?;
            GatheringConfiguration::from_json(&request["config"])?;
            Ok(json!(true))
        }
        "config_delay" => {
            fields(request, &["op", "config", "players", "minimum", "maximum"])?;
            let config = GatheringConfiguration::from_json(&request["config"])?;
            Ok(json!(config.delay(
                i32_field(request, "players")?,
                i32_field(request, "minimum")?,
                i32_field(request, "maximum")?
            )?))
        }
        "retail_configuration" => {
            fields(request, &["op", "index"])?;
            let index = usize::try_from(request["index"].as_u64().ok_or("invalid retail index")?)
                .map_err(|_| "invalid retail index")?;
            let original: Value =
                serde_json::from_str(include_str!("../data/retail_gathering.json"))
                    .map_err(|_| "invalid embedded retail metadata")?;
            let row = original["hoppers"]
                .get(index)
                .ok_or("unknown retail index")?;
            let config = json!({"countdown_ms":row["countdown_ms"],"timeout_ms":row["timeout_ms"],
                "provenance":format!("Retail launch2004 {} countdown/timeout;0x193C70/1931D0",string(row,"name",128)?)});
            GatheringConfiguration::from_json(&config)?;
            Ok(json!([row["minimum"], row["maximum"], config]))
        }
        "native_birth_id_order" => {
            fields(request, &["op", "rows"])?;
            let input = request["rows"].as_array().ok_or("invalid provider rows")?;
            if input.len() > 1000 {
                return Err("too many native provider rows");
            }
            let mut rows = Vec::with_capacity(input.len());
            for row in input {
                fields(row, &["born", "id"])?;
                rows.push((
                    row["born"].as_i64().ok_or("invalid native birth clock")?,
                    string(row, "id", 128)?.to_owned(),
                ));
            }
            Ok(json!(native_birth_id_order(rows)?))
        }
        _ => Err("unknown lifecycle value operation"),
    }
}

#[derive(Clone)]
pub enum Object {
    Progress(SearchProgress),
    Seen(SeenSessions<String>),
    Clock(GatheringClock),
    Jitter(u32),
    Party(PartyState),
}

impl Object {
    pub fn new(request: &Value) -> Result<Self> {
        match string(request, "kind", 32)? {
            "progress" => {
                fields(
                    request,
                    &["kind", "maximum", "ranked", "radius", "total", "active"],
                )?;
                Ok(Self::Progress(SearchProgress::new(
                    i32_field(request, "maximum")?,
                    boolean(request, "ranked")?,
                    i32_field(request, "radius")?,
                    i32_field(request, "total")?,
                    boolean(request, "active")?,
                )?))
            }
            "seen" => {
                fields(request, &["kind"])?;
                Ok(Self::Seen(SeenSessions::default()))
            }
            "clock" => {
                fields(request, &["kind", "started", "growth", "peak"])?;
                Ok(Self::Clock(GatheringClock::new(
                    u32_field(request, "started")?,
                    u32_field(request, "growth")?,
                    i32_field(request, "peak")?,
                )?))
            }
            "jitter" => {
                fields(request, &["kind", "seed"])?;
                Ok(Self::Jitter(u32_field(request, "seed")?))
            }
            "party" => {
                fields(request, &["kind", "search_offset", "gather_offset"])?;
                Ok(Self::Party(PartyState::new(
                    i32_field(request, "search_offset")?,
                    i32_field(request, "gather_offset")?,
                )))
            }
            _ => Err("unknown lifecycle object"),
        }
    }

    // Private views must belong to the current new-search generation. These
    // generations protect the native adapter; they are not Xbox protocol data.
    fn generation(&self, request: &Value) -> Result<()> {
        let expected = request["generation"]
            .as_u64()
            .ok_or("generation required")?;
        let current = if let Self::Party(party) = self {
            party.generation
        } else {
            0
        };
        if expected != current {
            return Err("stale search generation");
        }
        Ok(())
    }
    fn progress(&mut self) -> Result<&mut SearchProgress> {
        match self {
            Self::Progress(value) => Ok(value),
            Self::Party(value) => value.progress.as_mut().ok_or("search not initialized"),
            _ => Err("wrong progress owner"),
        }
    }
    fn seen(&mut self) -> Result<&mut SeenSessions<String>> {
        match self {
            Self::Seen(value) => Ok(value),
            Self::Party(value) => Ok(&mut value.seen),
            _ => Err("wrong seen owner"),
        }
    }

    /// Produces a candidate next state; the ABI commits only after checking
    /// response capacity. Invalid commands cannot partly mutate live state.
    pub fn command(&self, request: &Value) -> Result<(Self, Value)> {
        let mut next = self.clone();
        let result = next.apply(request)?;
        Ok((next, result))
    }

    fn apply(&mut self, request: &Value) -> Result<Value> {
        let op = string(request, "op", 64)?;
        if op.starts_with("progress_") || op.starts_with("seen_") {
            self.generation(request)?;
        }
        match op {
            "snapshot" => {
                fields(request, &["op"])?;
                Ok(match self {
                    Self::Progress(value) => value.snapshot(),
                    Self::Seen(value) => json!(value.values()),
                    Self::Clock(value) => value.snapshot(),
                    Self::Jitter(seed) => json!(seed),
                    Self::Party(value) => value.snapshot(),
                })
            }
            "progress_snapshot" => {
                fields(request, &["op", "generation"])?;
                Ok(self.progress()?.snapshot())
            }
            "progress_complete" => {
                fields(
                    request,
                    &[
                        "op",
                        "generation",
                        "count",
                        "query_pending",
                        "full_pending",
                        "brief_pending",
                        "selected",
                    ],
                )?;
                let count = i32_field(request, "count")?;
                let pending = [
                    boolean(request, "query_pending")?,
                    boolean(request, "full_pending")?,
                    boolean(request, "brief_pending")?,
                    boolean(request, "selected")?,
                ];
                Ok(json!(self.progress()?.complete(count, pending)?))
            }
            "seen_snapshot" => {
                fields(request, &["op", "generation"])?;
                Ok(json!(self.seen()?.values()))
            }
            "seen_clear" => {
                fields(request, &["op", "generation"])?;
                self.seen()?.clear();
                Ok(json!(true))
            }
            "seen_reject" | "seen_contains" => {
                fields(request, &["op", "generation", "identity"])?;
                let identity = string(request, "identity", 128)?.to_owned();
                if !identity.is_ascii() || identity.bytes().any(|n| n < 0x20) {
                    return Err("invalid native opaque session identity");
                }
                let seen = self.seen()?;
                if op == "seen_reject" {
                    seen.reject(identity);
                    Ok(json!(true))
                } else {
                    Ok(json!(seen.contains(&identity)))
                }
            }
            "jitter_next" => {
                fields(request, &["op"])?;
                let Self::Jitter(seed) = self else {
                    return Err("wrong jitter owner");
                };
                let (next, search, gather) = jitter(*seed, 60000, 60000);
                *seed = next;
                Ok(json!([search, gather]))
            }
            "clock_observe" => {
                fields(request, &["op", "now", "players"])?;
                let Self::Clock(clock) = self else {
                    return Err("wrong clock owner");
                };
                Ok(json!(clock.observe(
                    u32_field(request, "now")?,
                    i32_field(request, "players")?
                )?))
            }
            "clock_growth" => {
                fields(request, &["op", "growth"])?;
                let Self::Clock(clock) = self else {
                    return Err("wrong clock owner");
                };
                clock.growth = u32_field(request, "growth")?;
                Ok(json!(true))
            }
            "clock_expired" => {
                fields(request, &["op", "now", "config", "offset"])?;
                let config = GatheringConfiguration::from_json(&request["config"])?;
                let Self::Clock(clock) = self else {
                    return Err("wrong clock owner");
                };
                Ok(json!(clock.expired(
                    u32_field(request, "now")?,
                    config.timeout_ms,
                    i32_field(request, "offset")?
                )))
            }
            "clock_ready" | "clock_remaining" => {
                fields(
                    request,
                    &["op", "now", "config", "players", "minimum", "maximum"],
                )?;
                let config = GatheringConfiguration::from_json(&request["config"])?;
                let (now, players, minimum, maximum) = (
                    u32_field(request, "now")?,
                    i32_field(request, "players")?,
                    i32_field(request, "minimum")?,
                    i32_field(request, "maximum")?,
                );
                let Self::Clock(clock) = self else {
                    return Err("wrong clock owner");
                };
                if op == "clock_ready" {
                    Ok(json!(clock.ready(now, &config, players, minimum, maximum)?))
                } else {
                    Ok(json!(
                        clock.remaining(now, &config, players, minimum, maximum)?
                    ))
                }
            }
            "party_start_search" => {
                fields(request, &["op", "now", "maximum", "ranked"])?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                party.start_search(
                    u32_field(request, "now")?,
                    i32_field(request, "maximum")?,
                    boolean(request, "ranked")?,
                )?;
                Ok(json!(true))
            }
            "party_start_gather" => {
                fields(request, &["op", "now"])?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                party.start_gather(u32_field(request, "now")?);
                Ok(json!(true))
            }
            "party_choose" | "party_joined" => {
                fields(request, &["op"])?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                party.mode = if op == "party_choose" {
                    "choose"
                } else {
                    "joined"
                };
                Ok(json!(true))
            }
            "party_role" => {
                fields(request, &["op", "now", "capacity"])?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                Ok(json!(role(
                    u32_field(request, "now")?,
                    party.search_start,
                    party.gather_start,
                    i32_field(request, "capacity")?
                )))
            }
            "party_search_expired" => {
                fields(request, &["op", "now"])?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                Ok(json!(search_expired(
                    u32_field(request, "now")?,
                    party.search_start,
                    party.search_offset
                )))
            }
            "party_complete_query" => {
                fields(request, &["op", "generation", "count"])?;
                self.generation(request)?;
                let Self::Party(party) = self else {
                    return Err("wrong party owner");
                };
                let again = party
                    .progress
                    .as_mut()
                    .ok_or("search not initialized")?
                    .complete(i32_field(request, "count")?, [false; 4])?;
                if !again {
                    party.mode = "choose";
                }
                Ok(json!(again))
            }
            _ => Err("unknown lifecycle command"),
        }
    }
}
