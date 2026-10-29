//! Room text protocol, challenge clocks and publications owned by Rust.
//! Modern coordinator transport policy; no original Xbox challenge recovery.
use super::{Coordinator, Setup, Stage, dispatch, m, packets, wire};
use crate::matchmaking_model::{Error, Result};
use crate::matchmaking_packet_budget::Stream;
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};
#[derive(Default)]
pub(super) struct Connection {
    now: f64,
    sent: Option<f64>,
    challenge: Option<String>,
    blocked: u32,
}
impl Connection {
    fn observe(&mut self, now: f64) -> Result<f64> {
        m::check(now.is_finite() && now >= 0., "Invalid room probe clock")?;
        self.now = self.now.max(now);
        Ok(self.now)
    }
    fn expired(&self) -> bool {
        self.challenge.is_some() && self.sent.is_some_and(|sent| self.now - sent > 15.)
    }
    fn tick(&mut self, now: f64, interval: f64) -> Result<Option<String>> {
        self.observe(now)?;
        if self.challenge.is_none() && self.sent.is_none_or(|sent| self.now - sent > interval) {
            let nonce = wire::entropy(8)?;
            self.challenge = Some(nonce.clone());
            self.sent = Some(self.now);
            return Ok(Some(nonce));
        }
        Ok(None)
    }
    fn answer(&mut self, now: f64, value: &Value) -> Result<Option<f64>> {
        self.observe(now)?;
        if self.expired() {
            return Ok(None);
        }
        let value = m::text(value, 16)?;
        let nonce = self
            .challenge
            .as_ref()
            .ok_or(Error(400, "Invalid connection challenge"))?;
        m::check(
            value.len() == 16 && wire::same(nonce, value),
            "Invalid connection challenge",
        )?;
        let elapsed = (self.now - self.sent.unwrap()) * 1000.;
        self.challenge = None;
        Ok(Some(elapsed))
    }
}
fn effect() -> Value {
    json!({"kind":"effects","publications":[],"remove":[],"disconnect":[],"close":null,"commit":false,"continuity":false,"stop":false,"handled":true})
}
fn close(e: &mut Value, scope: &str, reason: &str) {
    e["close"] = json!({"scope":scope,"reason":reason,"code":1008});
    e["stop"] = json!(true);
}
fn failure(row: &Setup, e: &mut Value) {
    close(
        e,
        if row.direct || row.continuity {
            "peer"
        } else {
            "room"
        },
        "Match connection lost",
    );
}
fn invoke(
    row: &mut Setup,
    core: &mut Coordinator,
    r: &Value,
    op: &str,
    args: Value,
) -> Result<Value> {
    let mut n = r.clone();
    n["op"] = json!(op);
    for (k, v) in args.as_object().unwrap() {
        n[k] = v.clone();
    }
    dispatch(row, core, &n)
}
fn connected(r: &Value, who: &str) -> bool {
    r["peers"]
        .as_array()
        .is_some_and(|peers| peers.iter().any(|p| p["player"] == who))
}
fn full(r: &Value, who: &str) -> bool {
    r["full"]
        .as_array()
        .is_some_and(|v| v.iter().any(|v| v == who))
}
fn targets(row: &Setup, r: &Value) -> Vec<String> {
    row.owners
        .iter()
        .filter(|who| row.wire.connection(who).is_some() && connected(r, who))
        .cloned()
        .collect()
}
fn publication(row: &Setup, r: &Value, e: &mut Value, who: &str, packet: Value, immediate: bool) {
    let Some(connection) = row.wire.connection(who) else {
        return;
    };
    if !connected(r, who) {
        return;
    }
    if !immediate && full(r, who) {
        if !e["disconnect"]
            .as_array()
            .unwrap()
            .iter()
            .any(|v| v["player"] == who)
        {
            e["disconnect"].as_array_mut().unwrap().push(json!({"player":who,"connection":connection,"code":1008,"reason":"Match reader stalled"}));
        }
    } else {
        e["publications"].as_array_mut().unwrap().push(
            json!({"player":who,"connection":connection,"packet":packet,"immediate":immediate}),
        );
    }
}
fn broadcast(row: &Setup, r: &Value, e: &mut Value, packet: Value) {
    for who in targets(row, r) {
        publication(row, r, e, &who, packet.clone(), false);
    }
}
fn completed(row: &mut Setup, core: &mut Coordinator, r: &Value, who: &str) -> Result<Value> {
    let state = invoke(row, core, r, "room_state_get", json!({}))?;
    m::check(
        state["phase"] == "postgame",
        "Native result review is not completed",
    )?;
    let result = super::call(core, json!({"op":"result_snapshot","id":r["id"]}))?;
    let mut packet = json!({"completed":result["digest"]});
    if row
        .wire
        .features(who)
        .is_some_and(|f| f["completion_detail"] == true)
    {
        packet["recorded"] = json!(state["result_recorded"] == true);
    }
    Ok(packet)
}
fn voice_packet(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    let connected = targets(row, r).into_iter().collect::<BTreeSet<_>>();
    let mut n = r.clone();
    n["peers"] = json!(
        r["peers"]
            .as_array()
            .ok_or(Error(400, "Room socket observations required"))?
            .iter()
            .filter(|p| p["player"]
                .as_str()
                .is_some_and(|who| connected.contains(who)))
            .cloned()
            .collect::<Vec<_>>()
    );
    invoke(row, core, &n, "room_state_voice", json!({}))
}
fn voice(row: &mut Setup, core: &mut Coordinator, r: &Value, e: &mut Value) -> Result<()> {
    let packet = voice_packet(row, core, r)?;
    broadcast(row, r, e, packet);
    Ok(())
}
fn review(row: &mut Setup, core: &mut Coordinator, r: &Value, e: &mut Value) -> Result<()> {
    let review = invoke(row, core, r, "room_state_review", json!({}))?;
    if review["commit"] == true {
        e["commit"] = json!(true);
    } else if review["completed"] == true {
        for who in targets(row, r) {
            let packet = completed(row, core, r, &who)?;
            publication(row, r, e, &who, packet, false);
        }
        voice(row, core, r, e)?;
    }
    Ok(())
}
fn generations(row: &Setup, r: &Value) -> BTreeMap<String, u64> {
    let mut out = row
        .owners
        .iter()
        .filter_map(|who| row.wire.connection(who).map(|id| (who.clone(), id)))
        .collect::<BTreeMap<_, _>>();
    if let (Some(who), Some(connection)) = (r["player"].as_str(), r["connection"].as_u64()) {
        out.entry(who.to_owned()).or_insert(connection);
    }
    out
}
fn remove(e: &mut Value, who: &str, prior: &BTreeMap<String, u64>, close: bool) {
    if let Some(connection) = prior.get(who) {
        if !e["remove"]
            .as_array()
            .unwrap()
            .iter()
            .any(|v| v["player"] == who)
        {
            e["remove"].as_array_mut().unwrap().push(json!({"player":who,"connection":connection,"close":close,"code":1008,"reason":"Match rebind timed out"}));
        }
    }
}
fn synchronize(row: &mut Setup, core: &mut Coordinator, r: &Value, e: &mut Value) -> Result<()> {
    if row.handoff.is_none() || row.life.closed {
        return Ok(());
    }
    let prior = generations(row, r);
    let change = invoke(row, core, r, "room_state_synchronize", json!({}))?;
    for who in change["retire"].as_array().unwrap() {
        remove(e, who.as_str().unwrap(), &prior, true);
    }
    if let Some(reason) = change["close"].as_str() {
        close(e, "room", reason);
    } else {
        broadcast(row, r, e, row.control());
    }
    Ok(())
}
fn departure(
    row: &mut Setup,
    core: &mut Coordinator,
    r: &Value,
    e: &mut Value,
    socket: bool,
) -> Result<()> {
    let who = m::text(&r["player"], 64)?.to_owned();
    let prior = generations(row, r);
    let change = invoke(
        row,
        core,
        r,
        if socket {
            "room_state_socket_lost"
        } else {
            "room_state_depart"
        },
        json!({}),
    )?;
    if let Some(reason) = change["close"].as_str() {
        close(e, "room", reason);
        return Ok(());
    }
    remove(e, &who, &prior, false);
    for retired in change["retire"].as_array().unwrap() {
        remove(
            e,
            retired.as_str().unwrap(),
            &prior,
            retired.as_str() != Some(who.as_str()),
        );
    }
    // Publications observe the membership after this departure, before IO
    // removes the socket handle. A detached legacy postgame peer is no longer
    // a voice recipient even though its admitted seat remains frozen.
    let removed = std::iter::once(who.clone())
        .chain(
            change["retire"]
                .as_array()
                .unwrap()
                .iter()
                .map(|p| p.as_str().unwrap().to_owned()),
        )
        .collect::<BTreeSet<_>>();
    let mut future = r.clone();
    future["peers"] = json!(
        r["peers"]
            .as_array()
            .ok_or(Error(400, "Room socket observations required"))?
            .iter()
            .filter(|p| p["player"].as_str().is_some_and(|p| !removed.contains(p)))
            .cloned()
            .collect::<Vec<_>>()
    );
    if change["review"] == true {
        review(row, core, &future, e)?;
    }
    if change["control"] == true {
        broadcast(row, &future, e, row.control());
    }
    if !change["notice"].is_null() {
        for who in targets(row, &future) {
            if row
                .wire
                .features(&who)
                .is_some_and(|f| f["departure_game_over"] == true)
            {
                publication(row, &future, e, &who, change["notice"].clone(), false);
            }
        }
    }
    if change["voice"] == true {
        voice(row, core, &future, e)?;
    }
    Ok(())
}
fn advance(row: &mut Setup, core: &mut Coordinator, r: &Value, e: &mut Value) -> Result<()> {
    if row.life.closed || !e["close"].is_null() {
        return Ok(());
    }
    let mut n = r.clone();
    n["active"] = json!(row.life.active);
    n["connected"] = json!(targets(row, r));
    let change = invoke(row, core, &n, "room_setup_advance", json!({}))?;
    if let Some(reason) = change["close"].as_str() {
        close(e, "room", reason);
    } else if !change["transition"].is_null() {
        e["continuity"] = json!(!change["continuity"].is_null());
        broadcast(row, r, e, row.control());
    }
    Ok(())
}
fn world(
    row: &mut Setup,
    core: &mut Coordinator,
    r: &Value,
    e: &mut Value,
    data: &Value,
) -> Result<bool> {
    let change = invoke(
        row,
        core,
        r,
        "room_continuity_message",
        json!({"data":data}),
    )?;
    if !change["broadcast"].is_null() {
        broadcast(row, r, e, change["broadcast"].clone());
    }
    if change["changed"] == true {
        synchronize(row, core, r, e)?;
    }
    Ok(change["handled"] == true)
}
pub(super) fn execute(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    row.life.observe(r, core.clock)?;
    let op = m::text(&r["op"], 64)?;
    let mut e = effect();
    if op == "room_control_presence" {
        let Ok(who) = wire::current(row, r) else {
            return Ok(json!([]));
        };
        let now = m::real(&r["control_now"])?;
        m::check(now >= 0., "Invalid room probe clock")?;
        let c = row.wire.control(&who)?;
        c.observe(now)?;
        let expired = c.expired();
        if expired || row.life.now >= row.life.deadline {
            return Ok(json!([]));
        }
        return Ok(json!(
            row.players
                .iter()
                .filter(|p| p.owner == who)
                .map(|p| json!({"player":p.id,"guest":p.guest}))
                .collect::<Vec<_>>()
        ));
    }
    // These are private IO continuations. Text input cannot select their opcodes.
    match op {
        "room_control_voice" => {
            return voice_packet(row, core, r);
        }
        "room_control_review" => {
            review(row, core, r, &mut e)?;
            return Ok(e);
        }
        "room_control_advance" => {
            advance(row, core, r, &mut e)?;
            return Ok(e);
        }
        "room_control_synchronize" => {
            synchronize(row, core, r, &mut e)?;
            return Ok(e);
        }
        "room_control_depart" => {
            wire::current(row, r)?;
            departure(row, core, r, &mut e, false)?;
            return Ok(e);
        }
        "room_control_socket_lost" => {
            let who = m::text(&r["player"], 64)?;
            row.actor(who)?;
            let Some(connection) = r["connection"].as_u64() else {
                return Ok(e);
            };
            if !row.wire.detached(who, connection) {
                return Ok(e);
            }
            departure(row, core, r, &mut e, true)?;
            row.wire.consume_detach(who, connection);
            return Ok(e);
        }
        "room_control_world" => {
            let handled = world(row, core, r, &mut e, &r["data"])?;
            e["handled"] = json!(handled);
            return Ok(e);
        }
        "room_control_rejected" => {
            let reason = r["reason"]
                .as_str()
                .ok_or(Error(400, "IO failure reason required"))?;
            m::check(reason.len() <= 1024, "Invalid IO failure reason")?;
            if let Err(error) = wire::current(row, r) {
                close(
                    &mut e,
                    if error.0 == 404 { "room" } else { "peer" },
                    if error.0 == 404 {
                        "Match connection or loading timed out. Search again."
                    } else {
                        "Match connection retired"
                    },
                );
                return Ok(e);
            }
            let mode = invoke(row, core, r, "room_state_rejection", json!({}))?;
            if mode == "room" {
                close(&mut e, "room", reason);
                return Ok(e);
            }
            if mode == "depart" {
                let mut n = r.clone();
                n["direct"] = json!(true);
                departure(row, core, &n, &mut e, false)?;
            }
            if e["close"].is_null() {
                close(&mut e, "peer", reason);
            }
            return Ok(e);
        }
        _ => {}
    }
    let who = match wire::current(row, r) {
        Ok(who) => who,
        Err(error)
            if op == "room_control_tick"
                || op == "room_control_io_failure"
                || op == "room_control_stalled" =>
        {
            close(
                &mut e,
                if error.0 == 404 { "room" } else { "peer" },
                if error.0 == 404 {
                    "Match connection or loading timed out. Search again."
                } else {
                    "Match connection retired"
                },
            );
            return Ok(e);
        }
        Err(error) => return Err(error),
    };
    if op == "room_control_completed" {
        return completed(row, core, r, &who);
    }
    if op == "room_control_io_failure" {
        failure(row, &mut e);
        return Ok(e);
    }
    if op == "room_control_stalled" {
        close(&mut e, "peer", "Match reader stalled");
        return Ok(e);
    }
    m::check(
        ["room_control_tick", "room_control_receive"].contains(&op),
        "Unknown native room control operation",
    )?;
    let now = m::real(&r["control_now"])?;
    if op == "room_control_tick" {
        if r["facts"].is_null() {
            return Ok(json!({"kind":"privacy","pairs":packets::pairs(row,&who)?}));
        }
        let mask = packets::mask(row, &who, &r["facts"])?;
        if row.stage == Stage::Loading {
            advance(row, core, r, &mut e)?;
        }
        if !e["close"].is_null() {
            return Ok(e);
        }
        if row
            .handoff
            .as_ref()
            .and_then(|h| h.deadline())
            .is_some_and(|deadline| row.life.now >= deadline)
        {
            synchronize(row, core, r, &mut e)?;
        }
        if !e["close"].is_null() {
            return Ok(e);
        }
        let interval = if row.stage == Stage::Connecting {
            0.2
        } else {
            5.
        };
        let (ping, expired, changed) = {
            let c = row.wire.control(&who)?;
            let ping = c.tick(now, interval)?;
            let changed = c.blocked != mask;
            c.blocked = mask;
            (ping, c.expired(), changed)
        };
        if expired {
            failure(row, &mut e);
            return Ok(e);
        }
        if changed {
            publication(row, r, &mut e, &who, json!({"voice_blocked":mask}), true);
        }
        if let Some(ping) = ping {
            publication(row, r, &mut e, &who, json!({"ping":ping}), true);
        }
        return Ok(e);
    }
    row.wire.control(&who)?.observe(now)?;
    if row.wire.control(&who)?.expired() {
        failure(row, &mut e);
        return Ok(e);
    }
    let packet_now = r.get("packet_now").map(m::real).unwrap_or(Ok(now))?;
    row.wire.budget(&who, Stream::Control, packet_now)?;
    let text = r["text"]
        .as_str()
        .ok_or(Error(400, "Match text control required"))?;
    let data = crate::matchmaking_json::parse(text.as_bytes(), 119000)?;
    let object = data
        .as_object()
        .ok_or(Error(400, "Invalid match control"))?;
    m::check(object.len() == 1, "Invalid match control fields")?;
    let (kind, value) = object.iter().next().unwrap();
    match kind.as_str() {
        "pong" => {
            let Some(milliseconds) = row.wire.control(&who)?.answer(now, value)? else {
                failure(row, &mut e);
                return Ok(e);
            };
            invoke(
                row,
                core,
                r,
                "room_setup_sample",
                json!({"milliseconds":milliseconds}),
            )?;
        }
        "leave" => {
            m::check(row.direct && *value == true, "Invalid match departure")?;
            let mut request = r.clone();
            request["direct"] = json!(true);
            departure(row, core, &request, &mut e, false)?;
            e["stop"] = json!(true);
            return Ok(e);
        }
        "peer_signal" => {
            let signal = invoke(row, core, r, "room_wire_signal", json!({"signal":value}))?;
            if let Some(target) = signal["deliver"].as_str() {
                publication(row, r, &mut e, target, signal["packet"].clone(), false);
            }
        }
        "peer_ready" => {
            invoke(
                row,
                core,
                r,
                "room_setup_direct_report",
                json!({"rows":value}),
            )?;
        }
        "loaded" => {
            invoke(
                row,
                core,
                r,
                "room_setup_loaded",
                json!({"signature":value}),
            )?;
        }
        "postgame" => {
            let submitted = invoke(row, core, r, "room_state_submit", json!({"report":value}))?;
            let packet = json!({"postgame":submitted["report"]});
            if submitted["publish"] == "sender" {
                publication(row, r, &mut e, &who, packet, false);
                return Ok(e);
            } else {
                broadcast(row, r, &mut e, packet);
            }
        }
        "confirm_result" => {
            let confirmed = invoke(row, core, r, "room_state_confirm", json!({"digest":value}))?;
            if confirmed["reply_completed"] == true {
                let packet = completed(row, core, r, &who)?;
                publication(row, r, &mut e, &who, packet, false);
                let packet = voice_packet(row, core, r)?;
                publication(row, r, &mut e, &who, packet, false);
            }
            if confirmed["review"] == true {
                review(row, core, r, &mut e)?;
            }
        }
        _ => {
            m::check(world(row, core, r, &mut e, &data)?, "Unknown match control")?;
        }
    }
    advance(row, core, r, &mut e)?;
    Ok(e)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn source_nonce_is_one_shot_and_probe_deadlines_are_strict_and_monotonic() {
        let mut c = Connection::default();
        let nonce = c.tick(10., 0.2).unwrap().unwrap();
        assert!(c.tick(10.2, 0.2).unwrap().is_none());
        assert!(c.answer(10.1, &json!("bad")).is_err());
        assert!(c.answer(10.1, &json!(nonce)).unwrap().unwrap() >= 100.);
        assert!(c.answer(10.2, &json!(nonce)).is_err());
        assert!(c.tick(10.2, 0.2).unwrap().is_none());
        assert!(c.tick(10.201, 0.2).unwrap().is_some());
        let nonce = c.challenge.clone().unwrap();
        assert_eq!(c.answer(10., &json!(nonce)).unwrap(), Some(0.));
        for now in [f64::NAN, f64::INFINITY, -1.] {
            assert!(c.tick(now, 5.).is_err());
        }
        let mut c = Connection::default();
        let nonce = c.tick(0., 5.).unwrap().unwrap();
        assert_eq!(c.answer(15., &json!(nonce)).unwrap(), Some(15000.));
        let mut c = Connection::default();
        let nonce = c.tick(0., 5.).unwrap().unwrap();
        assert_eq!(c.answer(15.001, &json!(nonce)).unwrap(), None);
        assert!(c.expired());
        assert!(c.tick(0., 5.).unwrap().is_none());
        assert!(c.expired());
    }
}
