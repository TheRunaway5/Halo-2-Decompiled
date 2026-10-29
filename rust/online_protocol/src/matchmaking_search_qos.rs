//! Native relay-cycle QoS owner, distinct from original XNet/service recovery.
//! Physical session/socket generations bind every leg, nonce, sample and query.
use super::{Coordinator, m, reject};
use crate::matchmaking_model::{Error, Result, array, s};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet, VecDeque};
type Pair = (String, String);
const ROW_CAP: usize = 50000;
const PROBE_CAP: usize = 1000;
const PEER_PROBES: usize = 4;
#[derive(Clone)]
struct Budget {
    credits: f64,
    at: f64,
}
impl Budget {
    fn new(credits: f64, now: f64) -> Self {
        Self { credits, at: now }
    }
    fn refill(&mut self, now: f64, rate: f64, burst: f64) {
        let now = now.max(self.at);
        self.credits = burst.min(self.credits + (now - self.at) * rate);
        self.at = now;
    }
}
struct Leg {
    pair: Pair,
    nonce: String,
}
struct Peer {
    connection: u64,
    generation: u64,
    key: String,
    queue: VecDeque<Leg>,
    ping: Option<String>,
    sent: Option<f64>,
    control: Budget,
    reserve: Budget,
}
struct Probe {
    nonce: String,
    expires: f64,
    waiting: String,
    started: Option<f64>,
    second: bool,
    sent: bool,
}
struct Measurement {
    milliseconds: i64,
    expires: f64,
}
struct Row {
    generations: [u64; 2],
    probe: Option<Probe>,
    samples: Vec<i64>,
    sample_expires: Option<f64>,
    measurement: Option<Measurement>,
}
pub(super) struct Registry {
    now: f64,
    cleanup_at: f64,
    lease: f64,
    timeout: f64,
    next: u64,
    peers: BTreeMap<String, Peer>,
    rows: BTreeMap<Pair, Row>,
    retry: BTreeSet<Pair>,
    wakes: BTreeMap<String, u64>,
}
impl Default for Registry {
    fn default() -> Self {
        Self {
            now: 0.,
            cleanup_at: 0.,
            lease: 30.,
            timeout: 4.,
            next: 0,
            peers: BTreeMap::new(),
            rows: BTreeMap::new(),
            retry: BTreeSet::new(),
            wakes: BTreeMap::new(),
        }
    }
}
fn nonce() -> Result<String> {
    let mut b = [0; 16];
    getrandom::getrandom(&mut b).map_err(|_| Error(503, "QoS entropy unavailable"))?;
    Ok(b.iter().map(|v| format!("{v:02x}")).collect())
}
fn pair(a: String, b: String) -> Pair {
    if a <= b { (a, b) } else { (b, a) }
}
fn physical(core: &Coordinator, who: &str) -> String {
    core.tickets
        .iter()
        .filter(|t| t["expires_at"].as_f64().is_some_and(|v| v > core.clock))
        .flat_map(|t| array(t, "players").iter())
        .find(|p| p["id"] == who)
        .map(|p| {
            p["connection_owner"]
                .as_str()
                .unwrap_or(s(p, "id"))
                .to_owned()
        })
        .unwrap_or(who.to_owned())
}
fn queued(core: &Coordinator, who: &str) -> bool {
    core.tickets.iter().any(|t| {
        t["expires_at"].as_f64().is_some_and(|v| v > core.clock)
            && array(t, "players")
                .iter()
                .any(|p| p["id"] == who || p["connection_owner"] == who)
    })
}
fn observed(r: &Value, who: &str, connection: u64) -> bool {
    r["io_peers"].as_array().is_some_and(|v| {
        v.iter().any(|p| {
            p["player"] == who
                && p["connection"].as_u64() == Some(connection)
                && p["closed"] == false
        })
    })
}
fn effect() -> Value {
    json!({"messages":[],"close":false})
}
impl Registry {
    fn observe(&mut self, r: &Value) -> Result<()> {
        let now = m::real(&r["qos_now"])?;
        m::check(now >= 0. && (now + 360.).is_finite(), "Invalid QoS clock")?;
        self.now = self.now.max(now);
        Ok(())
    }
    fn session(core: &Coordinator, r: &Value) -> Result<Value> {
        let row = core.sessions.checked(&r["generation"], core.clock)?;
        reject(row["key"] == r["key"], 401, "Search authorization changed")?;
        reject(
            crate::matchmaking_catalog::GATHERING.contains(&s(&row, "compatibility")),
            409,
            "Update Halo 2 for gathering QoS",
        )?;
        Ok(row)
    }
    fn valid(&self, core: &Coordinator, r: &Value, who: &str) -> bool {
        let Some(p) = self.peers.get(who) else {
            return false;
        };
        observed(r, who, p.connection)
            && !(p.ping.is_some() && p.sent.is_some_and(|sent| self.now - sent > 15.))
            && core
                .sessions
                .qos_binding(p.generation, &p.key, who, core.clock)
    }
    fn active(&self, core: &Coordinator, r: &Value, key: &Pair, row: &Row) -> bool {
        queued(core, &key.0)
            && queued(core, &key.1)
            && self.valid(core, r, &key.0)
            && self.valid(core, r, &key.1)
            && self
                .peers
                .get(&key.0)
                .is_some_and(|p| p.connection == row.generations[0])
            && self
                .peers
                .get(&key.1)
                .is_some_and(|p| p.connection == row.generations[1])
    }
    fn remove(&mut self, key: &Pair) {
        self.rows.remove(key);
        self.retry.remove(key);
        for who in [&key.0, &key.1] {
            if let Some(peer) = self.peers.get_mut(who) {
                peer.queue.retain(|leg| leg.pair != *key);
            }
        }
    }
    fn wake(&mut self, who: &str) {
        if let Some(peer) = self.peers.get(who) {
            self.wakes.insert(who.to_owned(), peer.connection);
        }
    }
    pub(super) fn retire(&mut self, who: &str) {
        self.peers.remove(who);
        self.wakes.remove(who);
        let keys = self
            .rows
            .keys()
            .filter(|k| k.0 == who || k.1 == who)
            .cloned()
            .collect::<Vec<_>>();
        for key in keys {
            self.remove(&key);
        }
    }
    fn cleanup(&mut self, core: &Coordinator, r: &Value, force: bool) {
        if !force && self.now < self.cleanup_at {
            return;
        }
        self.cleanup_at = self.now + 0.25;
        // Snapshot eligibility once, rather than cloning a session/profile
        // for each of the bounded bank's up to50000 pair records.
        let live = self
            .peers
            .keys()
            .filter(|who| self.valid(core, r, who))
            .cloned()
            .collect::<BTreeSet<_>>();
        let queued = core
            .tickets
            .iter()
            .filter(|t| t["expires_at"].as_f64().is_some_and(|v| v > core.clock))
            .flat_map(|t| array(t, "players").iter())
            .map(|p| {
                p["connection_owner"]
                    .as_str()
                    .unwrap_or(s(p, "id"))
                    .to_owned()
            })
            .collect::<BTreeSet<_>>();
        let dead = self
            .rows
            .iter()
            .filter(|(key, row)| {
                !live.contains(&key.0)
                    || !live.contains(&key.1)
                    || !queued.contains(&key.0)
                    || !queued.contains(&key.1)
                    || self
                        .peers
                        .get(&key.0)
                        .is_none_or(|p| p.connection != row.generations[0])
                    || self
                        .peers
                        .get(&key.1)
                        .is_none_or(|p| p.connection != row.generations[1])
                    || row.probe.as_ref().is_some_and(|p| p.expires <= self.now)
                    || row.sample_expires.is_some_and(|v| v <= self.now)
                    || row
                        .measurement
                        .as_ref()
                        .is_some_and(|v| v.expires <= self.now)
            })
            .map(|(key, _)| key.clone())
            .collect::<Vec<_>>();
        for key in dead {
            self.remove(&key);
        }
    }
    fn canonical(&self, core: &Coordinator, r: &Value) -> Result<Pair> {
        Ok(pair(
            physical(core, m::text(&r["a"], 64)?),
            physical(core, m::text(&r["b"], 64)?),
        ))
    }
    fn current(&self, core: &Coordinator, r: &Value) -> Result<String> {
        let who = m::text(&r["player"], 64)?;
        reject(
            self.valid(core, r, who),
            401,
            "Search authorization expired",
        )?;
        reject(
            self.peers[who].connection
                == r["connection"]
                    .as_u64()
                    .ok_or(Error(400, "QoS connection required"))?,
            409,
            "Search connection replaced",
        )?;
        Ok(who.to_owned())
    }
    fn allocate(&mut self, core: &Coordinator, r: &Value, key: &Pair) -> Result<bool> {
        if key.0 == key.1
            || !queued(core, &key.0)
            || !queued(core, &key.1)
            || !self.valid(core, r, &key.0)
            || !self.valid(core, r, &key.1)
        {
            return Ok(false);
        }
        if let Some(row) = self.rows.get(key) {
            if row.probe.is_some() || row.measurement.is_some() {
                return Ok(true);
            }
        }
        if (!self.rows.contains_key(key) && self.rows.len() >= ROW_CAP)
            || self.rows.values().filter(|row| row.probe.is_some()).count() >= PROBE_CAP
            || [&key.0, &key.1].iter().any(|who| {
                self.rows
                    .iter()
                    .filter(|(k, row)| {
                        row.probe.is_some()
                            && (k.0.as_str() == who.as_str() || k.1.as_str() == who.as_str())
                    })
                    .count()
                    >= PEER_PROBES
            })
        {
            return Ok(false);
        }
        if self.peers[&key.0].queue.len() >= 8 {
            return Ok(false);
        }
        for who in [&key.0, &key.1] {
            self.peers
                .get_mut(who)
                .unwrap()
                .reserve
                .refill(self.now, 32., 16.);
        }
        if [&key.0, &key.1]
            .iter()
            .any(|who| self.peers[*who].reserve.credits < 1.)
        {
            return Ok(false);
        }
        let nonce = nonce()?;
        let generations = [self.peers[&key.0].connection, self.peers[&key.1].connection];
        for who in [&key.0, &key.1] {
            self.peers.get_mut(who).unwrap().reserve.credits -= 1.;
        }
        let row = self.rows.entry(key.clone()).or_insert(Row {
            generations,
            probe: None,
            samples: Vec::new(),
            sample_expires: None,
            measurement: None,
        });
        row.probe = Some(Probe {
            nonce: nonce.clone(),
            expires: self.now + self.timeout,
            waiting: key.0.clone(),
            started: None,
            second: false,
            sent: false,
        });
        self.retry.remove(key);
        self.peers.get_mut(&key.0).unwrap().queue.push_back(Leg {
            pair: key.clone(),
            nonce,
        });
        self.wake(&key.0);
        Ok(true)
    }
    fn echo(&mut self, core: &Coordinator, r: &Value, who: &str, echo: &str) -> Result<()> {
        let key = self
            .rows
            .iter()
            .find(|(_, row)| {
                row.probe
                    .as_ref()
                    .is_some_and(|p| p.waiting == who && p.nonce == echo)
            })
            .map(|(key, _)| key.clone())
            .ok_or(Error(400, "Unexpected route probe"))?;
        let row = &self.rows[&key];
        let probe = row.probe.as_ref().unwrap();
        reject(
            self.active(core, r, &key, row)
                && probe.started.is_some()
                && probe.sent
                && probe.expires > self.now,
            400,
            "Unexpected route probe",
        )?;
        if !probe.second {
            if !self.valid(core, r, &key.1) || self.peers[&key.1].queue.len() >= 8 {
                self.remove(&key);
                return Ok(());
            }
            let nonce = nonce()?;
            let p = self.rows.get_mut(&key).unwrap().probe.as_mut().unwrap();
            p.second = true;
            p.waiting = key.1.clone();
            p.nonce = nonce.clone();
            p.sent = false;
            self.peers.get_mut(&key.1).unwrap().queue.push_back(Leg {
                pair: key.clone(),
                nonce,
            });
            self.wake(&key.1);
            return Ok(());
        }
        let elapsed = ((self.now - probe.started.unwrap()).max(0.) * 1000.) as i64;
        let row = self.rows.get_mut(&key).unwrap();
        row.probe = None;
        row.samples.push(elapsed);
        if row.samples.len() == 3 {
            row.samples.sort_unstable();
            row.measurement = Some(Measurement {
                milliseconds: row.samples[1],
                expires: self.now + self.lease,
            });
            row.samples.clear();
            row.sample_expires = None;
            self.retry.remove(&key);
        } else {
            if row.sample_expires.is_none() {
                row.sample_expires = Some(self.now + 3. * self.timeout);
            }
            self.retry.insert(key.clone());
            self.allocate(core, r, &key)?;
        }
        Ok(())
    }
    fn tick(&mut self, core: &Coordinator, r: &Value, who: &str) -> Result<Value> {
        let mut e = effect();
        let p = self.peers.get_mut(who).unwrap();
        if p.ping.is_some() && p.sent.is_some_and(|sent| self.now - sent > 15.) {
            e["close"] = json!(true);
            return Ok(e);
        }
        if p.ping.is_none() && p.sent.is_none_or(|sent| self.now - sent >= 5.) {
            let ping = nonce()?;
            p.ping = Some(ping.clone());
            p.sent = Some(self.now);
            e["messages"]
                .as_array_mut()
                .unwrap()
                .push(json!({"ping":ping}));
            // Let IO finish this heartbeat before timestamping a route leg.
            // A queued leg wakes the writer immediately after that write.
            if !p.queue.is_empty() {
                self.wake(who);
            }
            return Ok(e);
        }
        let retry = self
            .retry
            .range((who.to_owned(), String::new())..)
            .take_while(|k| k.0 == who)
            .take(4)
            .cloned()
            .collect::<Vec<_>>();
        for key in retry {
            self.allocate(core, r, &key)?;
        }
        for _ in 0..8 {
            let Some(leg) = self.peers.get_mut(who).unwrap().queue.pop_front() else {
                break;
            };
            let Some(row) = self.rows.get_mut(&leg.pair) else {
                continue;
            };
            let Some(p) = row.probe.as_mut() else {
                continue;
            };
            if p.waiting != who || p.nonce != leg.nonce || p.sent {
                continue;
            }
            if !p.second {
                p.started = Some(self.now);
            }
            p.sent = true;
            e["messages"]
                .as_array_mut()
                .unwrap()
                .push(json!({"probe":p.nonce}));
            break;
        }
        if !self.peers[who].queue.is_empty() {
            self.wake(who);
        }
        Ok(e)
    }
    fn matches(row: &Row, table: &str) -> bool {
        match table {
            "probes" => row.probe.is_some(),
            "samples" => !row.samples.is_empty(),
            "measurements" => row.measurement.is_some(),
            _ => false,
        }
    }
    fn view(key: &Pair, row: &Row, table: &str) -> Value {
        match table {
            "probes" => {
                let p = row.probe.as_ref().unwrap();
                json!({"pair":[key.0,key.1],"nonce":p.nonce,"expires":p.expires,"waiting":p.waiting,"started":p.started,"second":p.second})
            }
            "samples" => json!({"expires":row.sample_expires,"values":row.samples}),
            "measurements" => {
                let m = row.measurement.as_ref().unwrap();
                json!({"milliseconds":m.milliseconds,"expires":m.expires})
            }
            _ => Value::Null,
        }
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 64)?;
        // This is a bounded drain of typed IO destinations. Reading it cannot
        // advance the measurement clock or install/modify a probe.
        if op == "search_qos_wakeups" {
            let rows = self
                .wakes
                .iter()
                .take(32)
                .map(|(who, connection)| (who.clone(), *connection))
                .collect::<Vec<_>>();
            for (who, _) in &rows {
                self.wakes.remove(who);
            }
            return Ok(json!(
                rows.into_iter()
                    .map(|(player, connection)| json!({"player":player,"connection":connection}))
                    .collect::<Vec<_>>()
            ));
        }
        if op == "search_qos_init" {
            m::check(
                self.peers.is_empty() && self.rows.is_empty(),
                "Cannot configure active QoS",
            )?;
            let lease = m::real(&r["lease"])?;
            let timeout = m::real(&r["timeout"])?;
            m::check(
                lease > 0. && lease <= 120. && timeout > 0. && timeout <= 120.,
                "Invalid native QoS timing",
            )?;
            self.lease = lease;
            self.timeout = timeout;
            return Ok(Value::Null);
        }
        self.observe(r)?;
        self.cleanup(core, r, false);
        match op {
            "search_qos_authorize" | "search_qos_attach" => {
                let session = Self::session(core, r)?;
                let who = s(&session, "player").to_owned();
                if !r["prior"].is_null() {
                    reject(
                        r["prior"]
                            == json!({"player":who,"generation":session["generation"],"key":session["key"]}),
                        401,
                        "Search authorization changed",
                    )?;
                }
                // Missing IO publication is not permission for a second
                // preflight to replace a binding which already attached.
                let busy = self.peers.get(&who).is_some_and(|p| {
                    let session = core
                        .sessions
                        .qos_binding(p.generation, &p.key, &who, core.clock);
                    let expired =
                        p.ping.is_some() && p.sent.is_some_and(|sent| self.now - sent > 15.);
                    let closed = r["io_peers"].as_array().is_some_and(|v| {
                        v.iter().any(|v| {
                            v["player"] == who
                                && v["connection"].as_u64() == Some(p.connection)
                                && v["closed"] == true
                        })
                    });
                    session && !expired && !closed
                });
                reject(!busy, 409, "Search connection already open")?;
                let admission =
                    json!({"player":who,"generation":session["generation"],"key":session["key"]});
                if op == "search_qos_authorize" {
                    return Ok(admission);
                }
                let connection = self
                    .next
                    .checked_add(1)
                    .ok_or(Error(503, "QoS connection generation exhausted"))?;
                self.retire(&who);
                self.next = connection;
                self.peers.insert(
                    who.clone(),
                    Peer {
                        connection,
                        generation: session["generation"].as_u64().unwrap(),
                        key: s(&session, "key").to_owned(),
                        queue: VecDeque::new(),
                        ping: None,
                        sent: None,
                        control: Budget::new(20., self.now),
                        reserve: Budget::new(16., self.now),
                    },
                );
                Ok(json!({"player":who,"connection":connection}))
            }
            "search_qos_detach" => {
                let who = m::text(&r["player"], 64)?;
                let current = self
                    .peers
                    .get(who)
                    .is_some_and(|p| Some(p.connection) == r["connection"].as_u64());
                if current {
                    self.retire(who);
                }
                Ok(json!(current))
            }
            "search_qos_discard" => {
                let who = m::text(&r["player"], 64)?;
                if let Some(g) = r.get("generation").filter(|v| !v.is_null()) {
                    let g = g
                        .as_u64()
                        .ok_or(Error(400, "Session generation required"))?;
                    if self.peers.get(who).is_some_and(|p| p.generation != g) {
                        return Ok(Value::Null);
                    }
                }
                self.retire(who);
                Ok(Value::Null)
            }
            "search_qos_cleanup" => {
                self.cleanup(core, r, true);
                Ok(Value::Null)
            }
            "search_qos_ready" => Ok(json!(self.valid(
                core,
                r,
                &physical(core, m::text(&r["player"], 64)?)
            ))),
            "search_qos_request" | "search_qos_pending" | "search_qos_measurement" => {
                let a = m::text(&r["a"], 64)?;
                let b = m::text(&r["b"], 64)?;
                let key = self.canonical(core, r)?;
                if !queued(core, a)
                    || !queued(core, b)
                    || !self.valid(core, r, &key.0)
                    || !self.valid(core, r, &key.1)
                {
                    return Ok(if op == "search_qos_measurement" {
                        Value::Null
                    } else {
                        json!(false)
                    });
                }
                if op == "search_qos_request" {
                    return Ok(json!(self.allocate(core, r, &key)?));
                }
                if key.0 == key.1 {
                    return Ok(if op == "search_qos_measurement" {
                        json!({"milliseconds":0,"expires":self.now+self.lease})
                    } else {
                        json!(false)
                    });
                }
                let row = self
                    .rows
                    .get(&key)
                    .filter(|row| self.active(core, r, &key, row));
                if op == "search_qos_pending" {
                    return Ok(json!(row.is_some_and(
                        |row| row.probe.is_some() || self.retry.contains(&key)
                    )));
                }
                Ok(row
                    .and_then(|row| row.measurement.as_ref())
                    .filter(|v| v.expires > self.now)
                    .map(|v| json!({"milliseconds":v.milliseconds,"expires":v.expires}))
                    .unwrap_or(Value::Null))
            }
            "search_qos_tick" => {
                let who = match self.current(core, r) {
                    Ok(who) => who,
                    Err(_) => return Ok(json!({"messages":[],"close":true})),
                };
                self.tick(core, r, &who)
            }
            "search_qos_receive" => {
                let who = self.current(core, r)?;
                let p = self.peers.get_mut(&who).unwrap();
                p.control.refill(self.now, 40., 20.);
                reject(p.control.credits >= 1., 429, "Search control flood")?;
                p.control.credits -= 1.;
                m::check(r["kind"] == "text", "Invalid search control")?;
                let text = r["text"]
                    .as_str()
                    .ok_or(Error(400, "Search text required"))?;
                let body = crate::matchmaking_json::parse(text.as_bytes(), 256)?;
                let object = body
                    .as_object()
                    .ok_or(Error(400, "Search control object required"))?;
                m::check(object.len() == 1, "Invalid search control")?;
                if let Some(pong) = object.get("pong") {
                    let value = m::text(pong, 32)?;
                    m::check(
                        p.ping.as_deref() == Some(value),
                        "Unexpected search heartbeat",
                    )?;
                    p.ping = None;
                } else {
                    m::fields(&body, &["echo"])?;
                    let echo = m::text(&body["echo"], 32)?;
                    m::check(echo.len() == 32, "Invalid route challenge")?;
                    self.echo(core, r, &who, echo)?;
                }
                Ok(effect())
            }
            "search_qos_stats" => {
                self.cleanup(core, r, true);
                Ok(
                    json!({"peers":self.peers.len(),"rows":self.rows.len(),"probes":self.rows.values().filter(|row|row.probe.is_some()).count(),"samples":self.rows.values().filter(|row|!row.samples.is_empty()).count(),"measurements":self.rows.values().filter(|row|row.measurement.is_some()).count(),"lease":self.lease,"timeout":self.timeout}),
                )
            }
            "search_qos_keys" | "search_qos_get" => {
                self.cleanup(core, r, true);
                let table = m::text(&r["table"], 16)?;
                m::check(
                    ["probes", "samples", "measurements"].contains(&table),
                    "Invalid QoS view",
                )?;
                if op == "search_qos_keys" {
                    let offset = m::integer(&r["offset"], 0, 50000)? as usize;
                    return Ok(json!(
                        self.rows
                            .iter()
                            .filter(|(_, row)| Self::matches(row, table))
                            .skip(offset)
                            .take(128)
                            .map(|(key, _)| json!([key.0, key.1]))
                            .collect::<Vec<_>>()
                    ));
                }
                let key = self.canonical(core, r)?;
                Ok(self
                    .rows
                    .get(&key)
                    .filter(|row| Self::matches(row, table))
                    .map(|row| Self::view(&key, row, table))
                    .unwrap_or(Value::Null))
            }
            _ => Err(Error(400, "Unknown native QoS operation")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
        r["now"] = json!(100.);
        if r.get("qos_now").is_none() {
            r["qos_now"] = json!(10.);
        }
        c.execute(&r)
    }
    fn setup(count: usize) -> (Coordinator, Vec<Value>, Vec<Value>, Value) {
        let entries=crate::matchmaking_catalog::parse(json!({"version":1,"playlists":[{"id":"q","revision":"v1","name":"QoS fixture","description":"Native transport fixture","ranked":false,"quickmatch":true,
            "team_size":0,"minimum_players":2,"maximum_players":16,"maximum_party":4,"games":[{"id":"s","map":"lockout","variant":"Slayer","weight":1}],
            "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],"skill":{"kind":"custom","radius":6},"provenance":"Declared native QoS test, not original XNet"}]}).to_string().as_bytes()).unwrap();
        let mut c=Coordinator::new(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),"lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,"draw_seed":1,"native_seed":2})).unwrap();
        let mut sessions = Vec::new();
        let mut attached = Vec::new();
        let mut io = json!([]);
        for i in 0..count {
            let who = format!("p{i:02}");
            let admitted=send(&mut c,json!({"op":"session_register","body":{"screen_name":"Pilot","maps":["lockout"],"compatibility":"halo2-native-queue-v9","language":0},
                "identities":[{"owner":["discord",who],"player":who,"separate_party":false}],"resolved":[who],"rooms":[]})).unwrap()["session"].clone();
            send(&mut c,json!({"op":"enqueue","party_id":who,"playlist_id":"q","compatibility":"halo2-native-queue-v9","clan":null,"clan_rank":null,"leader":who,"language":0,
                "players":[{"id":who,"screen_name":"Pilot","rank":1,"raw_rank":null,"maps":["lockout"],"guest":false,"connection_owner":who}]})).unwrap();
            let a=send(&mut c,json!({"op":"search_qos_attach","generation":admitted["generation"],"key":admitted["key"],"prior":null,"io_peers":io})).unwrap();
            io.as_array_mut()
                .unwrap()
                .push(json!({"player":a["player"],"connection":a["connection"],"closed":false}));
            sessions.push(admitted);
            attached.push(a);
        }
        (c, sessions, attached, io)
    }
    fn request(io: &Value, a: &Value, b: &Value, op: &str, now: f64) -> Value {
        json!({"op":op,"a":a["player"],"b":b["player"],"qos_now":now,"io_peers":io})
    }
    fn actor(io: &Value, a: &Value, op: &str, now: f64) -> Value {
        json!({"op":op,"player":a["player"],"connection":a["connection"],"qos_now":now,"io_peers":io})
    }
    fn frame(c: &mut Coordinator, io: &Value, a: &Value, now: f64) -> String {
        for _ in 0..2 {
            let value = send(c, actor(io, a, "search_qos_tick", now)).unwrap();
            for message in value["messages"].as_array().unwrap() {
                if !message["ping"].is_null() {
                    let mut r = actor(io, a, "search_qos_receive", now);
                    r["kind"] = json!("text");
                    r["text"] = json!(json!({"pong":message["ping"]}).to_string());
                    send(c, r).unwrap();
                }
                if let Some(probe) = message["probe"].as_str() {
                    return probe.to_owned();
                }
            }
        }
        panic!("No dispatched route leg");
    }
    fn echo(c: &mut Coordinator, io: &Value, a: &Value, nonce: &str, now: f64) -> Result<Value> {
        let mut r = actor(io, a, "search_qos_receive", now);
        r["kind"] = json!("text");
        r["text"] = json!(json!({"echo":nonce}).to_string());
        send(c, r)
    }
    #[test]
    fn reservation_depletion_is_atomic_and_tick_recovers_real_partial_work() {
        let (mut c, _, a, io) = setup(2);
        c.search_qos.peers.get_mut("p00").unwrap().reserve = Budget::new(0., 10.);
        assert_eq!(
            send(
                &mut c,
                request(&io, &a[0], &a[1], "search_qos_request", 10.)
            )
            .unwrap(),
            false
        );
        assert!(c.search_qos.rows.is_empty());
        assert_eq!(c.search_qos.peers["p01"].reserve.credits, 16.);
        assert_eq!(
            send(
                &mut c,
                request(&io, &a[0], &a[1], "search_qos_request", 10.1)
            )
            .unwrap(),
            true
        );
        let before = c.search_qos.peers["p00"].reserve.credits;
        assert_eq!(
            send(
                &mut c,
                request(&io, &a[0], &a[1], "search_qos_request", 10.1)
            )
            .unwrap(),
            true
        );
        assert_eq!(c.search_qos.peers["p00"].reserve.credits, before);
        let first = frame(&mut c, &io, &a[0], 10.1);
        echo(&mut c, &io, &a[0], &first, 10.11).unwrap();
        let second = frame(&mut c, &io, &a[1], 10.11);
        for who in ["p00", "p01"] {
            c.search_qos.peers.get_mut(who).unwrap().reserve = Budget::new(0., 10.12);
        }
        echo(&mut c, &io, &a[1], &second, 10.12).unwrap();
        assert!(c.search_qos.rows.values().all(|r| r.probe.is_none()));
        assert_eq!(c.search_qos.retry.len(), 1);
        assert_eq!(
            send(
                &mut c,
                request(&io, &a[0], &a[1], "search_qos_pending", 10.12)
            )
            .unwrap(),
            true
        );
        frame(&mut c, &io, &a[0], 10.2);
        assert!(c.search_qos.retry.is_empty());
        assert!(c.search_qos.rows.values().all(|r| r.probe.is_some()));
    }
    #[test]
    fn legs_must_be_dispatched_and_three_complete_cycles_own_the_median() {
        let (mut c, _, a, io) = setup(2);
        let mut now = 10.;
        send(
            &mut c,
            request(&io, &a[0], &a[1], "search_qos_request", now),
        )
        .unwrap();
        let early = c
            .search_qos
            .rows
            .values()
            .next()
            .unwrap()
            .probe
            .as_ref()
            .unwrap()
            .nonce
            .clone();
        assert_eq!(echo(&mut c, &io, &a[0], &early, now).unwrap_err().0, 400);
        for duration in [0.0301, 0.0901, 0.0501] {
            let start = now;
            let first = frame(&mut c, &io, &a[0], now);
            now += 0.01;
            echo(&mut c, &io, &a[0], &first, now).unwrap();
            let early = c
                .search_qos
                .rows
                .values()
                .next()
                .unwrap()
                .probe
                .as_ref()
                .unwrap()
                .nonce
                .clone();
            assert_eq!(echo(&mut c, &io, &a[1], &early, now).unwrap_err().0, 400);
            let second = frame(&mut c, &io, &a[1], now);
            now = start + duration;
            echo(&mut c, &io, &a[1], &second, now).unwrap();
            assert_eq!(echo(&mut c, &io, &a[1], &second, now).unwrap_err().0, 400);
        }
        let result = send(
            &mut c,
            request(&io, &a[0], &a[1], "search_qos_measurement", now),
        )
        .unwrap();
        assert_eq!(result["milliseconds"], 50);
        assert_eq!(
            send(
                &mut c,
                request(&io, &a[1], &a[0], "search_qos_measurement", now - 1.)
            )
            .unwrap(),
            result,
            "Regressing IO cannot change or refill a measurement"
        );
    }
    #[test]
    fn two_preflights_and_stale_retirement_cannot_replace_or_end_new_generation() {
        let (mut c, sessions, a, io) = setup(2);
        let duplicate = json!({"op":"search_qos_attach","generation":sessions[0]["generation"],"key":sessions[0]["key"],"prior":null,"io_peers":[]});
        assert_eq!(
            send(&mut c, duplicate).unwrap_err().0,
            409,
            "Unpublished IO is not a free native attachment"
        );
        send(
            &mut c,
            json!({"op":"session_revoke","key":sessions[0]["key"]}),
        )
        .unwrap();
        assert!(!c.search_qos.peers.contains_key("p00"));
        let admitted=send(&mut c,json!({"op":"session_register","body":{"screen_name":"Pilot","maps":["lockout"],"compatibility":"halo2-native-queue-v9","language":0},
            "identities":[{"owner":["discord","p00"],"player":"p00","separate_party":false}],"resolved":["p00"],"rooms":[]})).unwrap()["session"].clone();
        let next=send(&mut c,json!({"op":"search_qos_attach","generation":admitted["generation"],"key":admitted["key"],"prior":null,"io_peers":io})).unwrap();
        assert_ne!(next["connection"], a[0]["connection"]);
        send(&mut c,json!({"op":"search_qos_discard","player":"p00","generation":sessions[0]["generation"]})).unwrap();
        assert_eq!(
            send(&mut c, actor(&io, &a[0], "search_qos_detach", 10.)).unwrap(),
            false
        );
        assert_eq!(
            c.search_qos.peers["p00"].connection,
            next["connection"].as_u64().unwrap()
        );
        for op in [
            "search_qos_restore",
            "search_qos_install",
            "search_qos_measurement_set",
        ] {
            assert_eq!(
                send(&mut c, json!({"op":op,"io_peers":io})).unwrap_err().0,
                400
            );
        }
    }
    #[test]
    fn wake_destinations_are_consumed_once_and_retire_with_the_connection() {
        let (mut c, _, a, io) = setup(2);
        send(
            &mut c,
            request(&io, &a[0], &a[1], "search_qos_request", 10.),
        )
        .unwrap();
        let wakes = |c: &mut Coordinator| {
            send(c, json!({"op":"search_qos_wakeups","qos_now":999.})).unwrap()
        };
        assert_eq!(
            wakes(&mut c),
            json!([{"player":a[0]["player"],"connection":a[0]["connection"]}])
        );
        assert_eq!(
            c.search_qos.now, 10.,
            "Draining IO cannot advance the measurement clock"
        );
        assert_eq!(wakes(&mut c), json!([]));
        let heartbeat = send(&mut c, actor(&io, &a[0], "search_qos_tick", 10.)).unwrap();
        assert_eq!(heartbeat["messages"].as_array().unwrap().len(), 1);
        assert!(heartbeat["messages"][0]["ping"].is_string());
        assert!(
            c.search_qos
                .rows
                .values()
                .next()
                .unwrap()
                .probe
                .as_ref()
                .unwrap()
                .started
                .is_none()
        );
        assert_eq!(
            wakes(&mut c),
            json!([{"player":a[0]["player"],"connection":a[0]["connection"]}])
        );
        let first = frame(&mut c, &io, &a[0], 10.1);
        assert_eq!(
            c.search_qos
                .rows
                .values()
                .next()
                .unwrap()
                .probe
                .as_ref()
                .unwrap()
                .started,
            Some(10.1)
        );
        echo(&mut c, &io, &a[0], &first, 10.11).unwrap();
        assert_eq!(
            wakes(&mut c),
            json!([{"player":a[1]["player"],"connection":a[1]["connection"]}])
        );
        let second = frame(&mut c, &io, &a[1], 10.11);
        assert_eq!(
            wakes(&mut c),
            json!([{"player":a[1]["player"],"connection":a[1]["connection"]}]),
            "IO consumes the wake emitted after the second peer's heartbeat"
        );
        echo(&mut c, &io, &a[1], &second, 10.12).unwrap();
        assert_eq!(c.search_qos.wakes.len(), 1);
        c.search_qos.retire(s(&a[0], "player"));
        assert_eq!(wakes(&mut c), json!([]));
    }
    #[test]
    fn whole_pair_capacity_and_metadata_pages_are_bounded_including_partial_collectors() {
        let (mut c, _, a, io) = setup(17);
        for i in 0..17 {
            for j in i + 1..17 {
                let key = pair(s(&a[i], "player").to_owned(), s(&a[j], "player").to_owned());
                c.search_qos.rows.insert(
                    key,
                    Row {
                        generations: [
                            a[i]["connection"].as_u64().unwrap(),
                            a[j]["connection"].as_u64().unwrap(),
                        ],
                        probe: None,
                        samples: vec![10],
                        sample_expires: Some(20.),
                        measurement: None,
                    },
                );
            }
        }
        let one = send(
            &mut c,
            json!({"op":"search_qos_keys","table":"samples","offset":0,"io_peers":io}),
        )
        .unwrap();
        let two = send(
            &mut c,
            json!({"op":"search_qos_keys","table":"samples","offset":128,"io_peers":io}),
        )
        .unwrap();
        assert_eq!(one.as_array().unwrap().len(), 128);
        assert_eq!(two.as_array().unwrap().len(), 8);
        assert!(one.to_string().len() < 32768);
        // The same bounded row bank counts unfinished collectors as well as
        // finished measurements. Capacity cannot be bypassed with partials.
        for i in c.search_qos.rows.len()..ROW_CAP {
            c.search_qos.rows.insert(
                (format!("unused{i:05}"), "z".into()),
                Row {
                    generations: [0, 0],
                    probe: None,
                    samples: vec![1],
                    sample_expires: Some(20.),
                    measurement: None,
                },
            );
        }
        let fresh = pair("p00".into(), "p01".into());
        c.search_qos.rows.remove(&fresh);
        c.search_qos.rows.insert(
            ("capacity".into(), "z".into()),
            Row {
                generations: [0, 0],
                probe: None,
                samples: vec![1],
                sample_expires: Some(20.),
                measurement: None,
            },
        );
        let mut owner = std::mem::take(&mut c.search_qos);
        assert_eq!(owner.rows.len(), ROW_CAP);
        assert!(!owner.allocate(&c, &json!({"io_peers":io}), &fresh).unwrap());
        owner.rows.clear();
        c.search_qos = owner;
    }
}
