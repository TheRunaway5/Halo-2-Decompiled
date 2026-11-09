//! Room endpoint coordination and typed IO for the existing Rust room owners.
use super::*;
use halo2_online_protocol::matchmaking_model as m;
use room_io::{Binding, Outbound, Sockets};

pub(super) fn facts(owners: &mut Owners, source: u64, clock: &Clock) -> Result<Value> {
    owners.request(
        source,
        &json!({"op":"room_service_observations","now":clock.monotonic,"io_now":clock.monotonic}),
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    fn pair() -> (Runtime, Vec<String>) {
        let supplied: Value = serde_json::from_slice(include_bytes!(
            "../../../multiplayer/master_server/examples/matchmaking-online.json"
        ))
        .unwrap();
        let configuration = supplied["playlists"]
            .as_array()
            .unwrap()
            .iter()
            .find(|p| p["id"] == "team-slayer")
            .unwrap()["rank_configuration"]
            .clone();
        let catalog = json!({"version":1,"playlists":[{"id":"room-unit","name":"Room unit","ranked":false,"team_size":0,
            "minimum_players":2,"maximum_players":2,"maximum_party":1,"revision":"room-unit-v1","description":"Declared transaction fixture",
            "quickmatch":false,"skill":{"kind":"custom","radius":0},"rank_configuration":configuration,
            "required_compatibility":"halo2-native-queue-v10","games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],
            "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],
            "provenance":"Declared legacy transaction/upgrade unit fixture; no original or played-game claim."}]});
        let mut runtime = Runtime::open(
            Path::new(":memory:"),
            true,
            Some(catalog.to_string().as_bytes()),
        )
        .unwrap();
        let mut headers = Vec::new();
        for (i, c) in ['V', 'W'].iter().enumerate() {
            let token = format!("h2s_{}", c.to_string().repeat(43));
            runtime
                .db
                .execute(
                    "INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                    rusqlite::params![
                        authentication::steam_digest(&token).unwrap(),
                        format!("765611979602879{}", 80 + i),
                        "Unit Fixture",
                        database::wall_now().unwrap() + 3600
                    ],
                )
                .unwrap();
            let identity = runtime.steam(&token).unwrap();
            let admitted=runtime.matchmaking_login(json!({"screen_name":format!("Unit Pilot {i}"),"maps":["lockout"],"compatibility":"halo2-native-queue-v10","language":0}),vec![Some(identity)]).unwrap();
            let header = format!("Bearer {}", admitted["token"].as_str().unwrap());
            runtime.ticket(&header,"POST",json!({"playlist_id":"room-unit","revision":"room-unit-v1","request_id":format!("00000000-0000-4000-8000-00000000000{i}")}),json!({})).unwrap();
            headers.push(header);
        }
        (runtime, headers)
    }
    #[test]
    fn rejected_room_commit_publishes_no_native_result_or_setup() {
        let (mut runtime, headers) = pair();
        runtime.db.execute_batch("CREATE TABLE room_parent(id TEXT PRIMARY KEY);CREATE TABLE room_failure(id TEXT REFERENCES room_parent(id) DEFERRABLE INITIALLY DEFERRED);CREATE TRIGGER room_fail AFTER UPDATE ON matchmaking_ledger_lock BEGIN INSERT INTO room_failure VALUES('missing'); END;").unwrap();
        assert_eq!(runtime.room_descriptor(&headers[0]).err().unwrap().0, 500);
        let source = runtime.queue_source().unwrap();
        let clock = runtime.clock().unwrap();
        assert_eq!(
            facts(&mut runtime.owners, source, &clock).unwrap()["queue"],
            json!([])
        );
        assert_eq!(
            runtime
                .db
                .query_row("SELECT count(*) FROM room_failure", [], |r| r
                    .get::<_, i64>(0))
                .unwrap(),
            0
        );
        runtime.db.execute_batch("DROP TRIGGER room_fail").unwrap();
        let first = runtime.room_descriptor(&headers[0]).unwrap();
        assert!(first == runtime.room_descriptor(&headers[0]).unwrap());
        let second = runtime.room_descriptor(&headers[1]).unwrap();
        assert_eq!(first["id"], second["id"]);
        assert!(first["token"] != second["token"]);
    }
    #[test]
    fn upgraded_room_stream_cannot_publish_after_expiry_or_search_cancellation() {
        for cancel in [false, true] {
            let (mut runtime, headers) = pair();
            let descriptor = runtime.room_descriptor(&headers[0]).unwrap();
            let id = descriptor["id"].as_str().unwrap();
            let header = format!("Bearer {}", descriptor["token"].as_str().unwrap());
            let prior = runtime.room_authorize(id, "relay", &header).unwrap();
            if cancel {
                runtime
                    .ticket(&headers[0], "DELETE", Value::Null, json!({"cancel":"1"}))
                    .unwrap();
            } else {
                runtime.started = Instant::now() - std::time::Duration::from_secs(26);
            }
            let error = runtime
                .room_attach(id, "relay", &header, prior, json!({}))
                .err()
                .unwrap();
            assert!([404, 409].contains(&error.0));
            assert_eq!(runtime.room_sockets.peers(id), json!([]));
        }
    }

    #[test]
    fn committed_result_retry_verifies_durable_data_before_room_acknowledgement() {
        let (mut runtime, headers) = pair();
        let first = runtime.room_descriptor(&headers[0]).unwrap();
        let second = runtime.room_descriptor(&headers[1]).unwrap();
        let id = first["id"].as_str().unwrap();
        assert_eq!(first["id"], second["id"]);
        let owners = headers
            .iter()
            .map(|header| runtime.matchmaking_auth(header).unwrap()["player"].clone())
            .collect::<Vec<_>>();
        let mut raw = vec![0; halo2_online_protocol::security_policy::PCR_SIZE];
        raw[0..2].copy_from_slice(&[1, 1]);
        raw[0x10..0x12].copy_from_slice(&[1, 1]);
        raw[0x64] = 2;
        for (slot, row) in m::array(&first, "players").iter().enumerate() {
            let at = 0x370 + slot * 0x94;
            raw[at] = 1;
            raw[at + 2..at + 10].copy_from_slice(&m::hex(&row["identity"], 8).unwrap());
            for (i, code) in row["name"].as_str().unwrap().encode_utf16().enumerate() {
                raw[at + 12 + 2 * i..at + 14 + 2 * i].copy_from_slice(&code.to_le_bytes());
            }
            raw[at + 0x88] = 255;
            raw[at + 0x90] = slot as u8;
        }
        let report = raw
            .iter()
            .map(|byte| format!("{byte:02x}"))
            .collect::<String>();
        let digest = runtime
            .room_request(json!({"op":"result_submit","id":id,"actor":owners[0],"report":report}))
            .unwrap();
        for actor in &owners {
            runtime
                .room_request(json!({"op":"result_confirm","id":id,"actor":actor,"digest":digest}))
                .unwrap();
        }

        // Finish the SQL transaction without delivering its acknowledgement to the room.
        let source = runtime.queue_source().unwrap();
        let clock = runtime.clock().unwrap();
        let observations = runtime.room_sockets.observations(runtime.started);
        let sources = runtime.sources(&observations);
        let committed = database::transaction(&mut runtime.owners, &mut runtime.db, Some(source), |owners, db| {
            let (handle, receipt) = database::drive(owners, db,
                json!({"database":{"mode":"result_commit","source":source,"match":id,"completed":clock.wall}}),
                &clock, sources)?;
            owners.retire(handle);
            Ok(receipt)
        }).unwrap();
        assert_eq!(committed["already_committed"], false);
        runtime
            .db
            .execute(
                "UPDATE match_result_admission SET envelope_sha256=?",
                ["0".repeat(64)],
            )
            .unwrap();
        let error = runtime
            .room_commit(id)
            .expect_err("Invalid durable result was acknowledged");
        assert_eq!(error.0, 409);
        let snapshot = json!({"op":"result_snapshot","id":id});
        assert_eq!(
            runtime.room_request(snapshot.clone()).unwrap()["committed"],
            false
        );

        runtime
            .db
            .execute(
                "UPDATE match_result_admission SET envelope_sha256=?",
                [committed["envelope_sha256"].as_str().unwrap()],
            )
            .unwrap();
        runtime.room_commit(id).unwrap();
        assert_eq!(runtime.room_request(snapshot).unwrap()["committed"], true);
        let rows: (i64, i64, String) = runtime
            .db
            .query_row(
                "SELECT (SELECT count(*) FROM match_results),
                    (SELECT sum(xp) FROM playlist_skill),public_id FROM carnage_reports",
                [],
                |row| Ok((row.get(0)?, row.get(1)?, row.get(2)?)),
            )
            .unwrap();
        assert_eq!((rows.0, rows.1), (1, 100));
        assert_eq!(rows.2, committed["public"].as_str().unwrap());
    }
}
pub(super) fn request(
    owners: &mut Owners,
    source: u64,
    sockets: &Sockets,
    clock: &Clock,
    mut r: Value,
) -> Result<Value> {
    let id = m::text(&r["id"], 64)?;
    let peers = sockets.peers(id);
    let full = sockets.full(id);
    r["now"] = json!(clock.monotonic);
    r["io_now"] = json!(clock.monotonic);
    r["control_now"] = json!(clock.monotonic);
    r["packet_now"] = json!(clock.monotonic);
    r["clock_ms"] = json!((clock.monotonic * 1000.) as i64);
    r["peers"] = peers;
    r["full"] = full;
    owners.request(source, &r)
}
pub struct Reply {
    pub text: Vec<String>,
    pub stop: bool,
}
impl Runtime {
    pub(super) fn room_request(&mut self, r: Value) -> Result<Value> {
        let source = self.queue_source()?;
        let clock = self.clock()?;
        request(&mut self.owners, source, &self.room_sockets, &clock, r)
    }
    pub fn room_descriptor(&mut self, header: &str) -> Result<Value> {
        // Advancing the real queue can request physical search measurements.
        self.ticket(header, "GET", Value::Null, json!({}))?;
        let auth = self.matchmaking_auth(header)?;
        let source = self.queue_source()?;
        let started = self.started;
        let observations = self.room_sockets.observations(started);
        let sources = self.sources(&observations);
        let sockets = &self.room_sockets;
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let clock = Clock {
                    monotonic: started.elapsed().as_secs_f64(),
                    wall: database::wall_now()?,
                };
                let prepared=owners.request(source,&json!({"op":"room_service_prepare","now":clock.monotonic,"generation":auth["generation"],"wall":clock.wall}))?;
                let id = m::text(&prepared["id"], 64)?;
                if prepared["existing"] == false {
                    let (handle, snapshots) = database::drive(
                        owners,
                        db,
                        json!({"database":{"mode":"snapshot_bank","operations":prepared["operations"]}}),
                        &clock,
                        sources,
                    )?;
                    owners.retire(handle);
                    let mut clans = serde_json::Map::new();
                    for who in prepared["clan_players"]
                        .as_object()
                        .ok_or(Error(503, "Native clan subjects required"))?
                        .keys()
                    {
                        let clan = db
                            .query_row("SELECT clan FROM clan_members WHERE player=?", [who], |r| {
                                r.get::<_, String>(0)
                            })
                            .optional()
                            .map_err(database::failure)?;
                        clans.insert(who.clone(), json!(clan));
                    }
                    let current = started.elapsed().as_secs_f64();
                    owners.request(source,&json!({"op":"room_service_admit","now":current,"io_now":current,"generation":auth["generation"],"prepared":prepared,"snapshots":snapshots,"clans":clans}))?;
                }
                let fresh = Clock {
                    monotonic: started.elapsed().as_secs_f64(),
                    wall: database::wall_now()?,
                };
                request(
                    owners,
                    source,
                    sockets,
                    &fresh,
                    json!({"op":"room_wire_descriptor","id":id,"player":auth["player"],"stun_servers":[]}),
                )
            },
        )
    }
    pub fn room_authorize(&mut self, id: &str, route: &str, authorization: &str) -> Result<Value> {
        self.room_maintenance()?;
        self.room_exists(id)?;
        self.room_request(json!({"op":"room_wire_authorize","id":id,"route":route,"authorization":authorization,"prior":null}))
    }
    pub(super) fn room_exists(&mut self, id: &str) -> Result<()> {
        if self.room_request(json!({"op":"room_service_exists","id":id}))? != true {
            return Err(Error(404, "Match no longer exists"));
        }
        Ok(())
    }
    pub fn room_content_failure(&mut self, header: &str, body: Value) -> Result<Value> {
        let auth = self.matchmaking_auth(header)?;
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let effect=self.owners.queue_transaction(source,|owners|{
            let observed=facts(owners,source,&clock)?;
            owners.request(source,&json!({"op":"api_content_failure","now":clock.monotonic,"generation":auth["generation"],"body":body,"rooms":observed["queue"]}))
        })?;
        if let Some(id) = effect["close"].as_str() {
            self.room_close(id, 1000, m::text(&effect["reason"], 1024)?)?;
        }
        Ok(effect["response"].clone())
    }
    pub fn room_attach(
        &mut self,
        id: &str,
        route: &str,
        authorization: &str,
        prior: Value,
        features: Value,
    ) -> Result<Binding> {
        self.room_maintenance()?;
        let admitted=self.room_request(json!({"op":"room_wire_attach","id":id,"route":route,"authorization":authorization,"prior":prior,
            "completion":features["completion"],"native_handoff":features["native_handoff"],"departure_game_over":features["departure_game_over"]}))?;
        let binding = self.room_sockets.attach(id, &admitted)?;
        let mut packets = vec![admitted["voice_auth"].clone()];
        packets.extend(m::array(&admitted, "publications").iter().cloned());
        for packet in packets {
            if !self.room_sockets.send(
                id,
                &binding.player,
                binding.connection,
                Outbound::Text(packet.to_string()),
            ) {
                binding.close();
                return Err(Error(503, "Initial room publication exceeds IO bound"));
            }
        }
        Ok(binding)
    }
    fn room_close(&mut self, id: &str, code: u16, reason: &str) -> Result<()> {
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let sockets = &self.room_sockets;
        let state = match request(
            &mut self.owners,
            source,
            sockets,
            &clock,
            json!({"op":"room_state_get","id":id}),
        ) {
            Ok(state) => state,
            Err(Error(409, _)) => {
                self.room_sockets.close_reason(id, code, reason);
                return Ok(());
            }
            Err(e) => return Err(e),
        };
        self.owners.queue_transaction(source, |owners| {
            for r in [
                json!({"op":"room_state_close","id":id}),
                json!({"op":"result_drop","id":id}),
                json!({"op":"api_room_close","id":id,"phase":state["phase"]}),
                json!({"op":"room_setup_close","id":id}),
            ] {
                request(owners, source, sockets, &clock, r)?;
            }
            Ok(())
        })?;
        self.room_sockets.close_reason(id, code, reason);
        Ok(())
    }
    fn room_commit(&mut self, id: &str) -> Result<()> {
        let source = self.queue_source()?;
        let started = self.started;
        let observations = self.room_sockets.observations(started);
        let sources = self.sources(&observations);
        let value = database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let clock = Clock {
                    monotonic: started.elapsed().as_secs_f64(),
                    wall: database::wall_now()?,
                };
                let (handle, value) = database::drive(
                    owners,
                    db,
                    json!({"database":{"mode":"result_commit","source":source,"match":id,"completed":clock.wall}}),
                    &clock,
                    sources,
                )?;
                owners.retire(handle);
                Ok(value)
            },
        )?;
        self.room_request(json!({"op":"result_committed","id":id,"digest":value["digest"]}))?;
        Ok(())
    }
    fn room_effect(
        &mut self,
        id: &str,
        mut effect: Value,
        actor: Option<&Value>,
        depth: usize,
    ) -> Result<Reply> {
        if depth > 8 {
            return Err(Error(503, "Native room effect continuation exceeds bound"));
        }
        let mut text = Vec::new();
        for item in m::array(&effect, "remove") {
            if item["close"] == true {
                self.room_sockets.disconnect_reason(
                    id,
                    m::text(&item["player"], 64)?,
                    item["connection"].as_u64(),
                    item["code"].as_u64().unwrap_or(1008) as u16,
                    m::text(&item["reason"], 1024)?,
                );
            }
            self.room_sockets.remove(
                id,
                m::text(&item["player"], 64)?,
                item["connection"]
                    .as_u64()
                    .ok_or(Error(503, "Native removed generation required"))?,
            );
        }
        if effect["commit"] == true && self.room_sockets.write_ready(id) {
            // Retain the native unanimous review on an IO COMMIT failure.
            // Only the database write is retried; no consent, outcome or XP
            // decision is supplied by the adapter. The native lease still runs.
            match self.room_commit(id) {
                Ok(()) => self.room_sockets.wrote(id),
                Err(Error(500, _)) => {
                    self.room_sockets.failed_write(id);
                    return Ok(Reply {
                        text,
                        stop: effect["stop"] == true,
                    });
                }
                Err(error) => return Err(error),
            }
            let completed = self.room_request(json!({"op":"room_control_review","id":id}))?;
            if completed["commit"] == true {
                return Err(Error(503, "Native result commit was not consumed"));
            }
            let next = self.room_effect(id, completed, actor, depth + 1)?;
            text.extend(next.text);
            if next.stop {
                effect["stop"] = json!(true)
            }
        }
        for item in m::array(&effect, "disconnect") {
            self.room_sockets.disconnect_reason(
                id,
                m::text(&item["player"], 64)?,
                item["connection"].as_u64(),
                item["code"].as_u64().unwrap_or(1008) as u16,
                m::text(&item["reason"], 1024)?,
            );
        }
        for item in m::array(&effect, "publications") {
            let who = m::text(&item["player"], 64)?;
            let connection = item["connection"]
                .as_u64()
                .ok_or(Error(503, "Native destination generation required"))?;
            if item["immediate"] == true
                && actor.is_some_and(|a| {
                    a["player"] == who && a["connection"].as_u64() == Some(connection)
                })
            {
                text.push(item["packet"].to_string());
            } else if !self.room_sockets.send(
                id,
                who,
                connection,
                Outbound::Text(item["packet"].to_string()),
            ) {
                let failure=self.room_request(json!({"op":"room_control_stalled","id":id,"player":who,"connection":connection}))?;
                self.room_effect(
                    id,
                    failure,
                    Some(&json!({"id":id,"player":who,"connection":connection})),
                    depth + 1,
                )?;
            }
        }
        if let Some(scope) = effect["close"]["scope"].as_str() {
            if scope == "room" {
                self.room_close(
                    id,
                    effect["close"]["code"].as_u64().unwrap_or(1008) as u16,
                    m::text(&effect["close"]["reason"], 1024)?,
                )?;
            } else if let Some(actor) = actor {
                self.room_sockets.disconnect_reason(
                    id,
                    m::text(&actor["player"], 64)?,
                    actor["connection"].as_u64(),
                    effect["close"]["code"].as_u64().unwrap_or(1008) as u16,
                    m::text(&effect["close"]["reason"], 1024)?,
                );
            }
        }
        Ok(Reply {
            text,
            stop: effect["stop"] == true,
        })
    }
    fn room_action(&mut self, mut actor: Value, op: &str, fields: Value) -> Result<Value> {
        actor["op"] = json!(op);
        for (k, v) in fields
            .as_object()
            .ok_or(Error(503, "Native room action fields required"))?
        {
            actor[k] = v.clone();
        }
        let mut effect = self.room_request(actor.clone())?;
        if effect["kind"] == "privacy" {
            let mut facts = Vec::new();
            for pair in m::array(&effect, "pairs") {
                let clock = self.clock()?;
                let setup = self.config(
                    "voice",
                    "allowed",
                    pair[0].as_str(),
                    None,
                    Value::Null,
                    json!({}),
                    &clock,
                );
                let mut setup = setup;
                setup["party_directory"]["target"] = pair[1].clone();
                let allowed = self.plan(setup)?;
                facts.push(json!({"a":pair[0],"b":pair[1],"allowed":allowed}));
            }
            actor["facts"] = json!(facts);
            effect = self.room_request(actor)?;
        }
        Ok(effect)
    }
    pub fn room_tick(&mut self, actor: Value) -> Result<Reply> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let effect = self.room_action(actor.clone(), "room_control_tick", json!({}))?;
        let mut reply = self.room_effect(&id, effect, Some(&actor), 0)?;
        if !reply.stop && self.room_sockets.write_ready(&id) {
            let state = self.room_request(json!({"op":"room_state_get","id":id}))?;
            if state["phase"] == "reviewing" {
                let effect = self.room_request(json!({"op":"room_control_review","id":id}))?;
                let next = self.room_effect(&id, effect, Some(&actor), 0)?;
                reply.text.extend(next.text);
                reply.stop |= next.stop;
            }
        }
        if !reply.stop
            && self.room_sockets.current(
                &id,
                m::text(&actor["player"], 64)?,
                actor["connection"]
                    .as_u64()
                    .ok_or(Error(503, "Native room generation required"))?,
            )
        {
            let mut drain = actor;
            drain["op"] = json!("room_wire_drain");
            let pending = self.room_request(drain)?;
            // The owner drains at most16 frames. Send this batch directly;
            // a full512-frame pending bank never overfills the512-slot IO queue.
            reply.text.extend(
                pending
                    .as_array()
                    .ok_or(Error(503, "Native signal frames required"))?
                    .iter()
                    .map(Value::to_string),
            );
        }
        Ok(reply)
    }
    pub fn room_receive(&mut self, actor: Value, text: String) -> Result<Reply> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let effect =
            self.room_action(actor.clone(), "room_control_receive", json!({"text":text}))?;
        self.room_effect(&id, effect, Some(&actor), 0)
    }
    pub fn room_rejected(&mut self, actor: Value) -> Result<Reply> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let effect = self.room_action(
            actor.clone(),
            "room_control_rejected",
            json!({"reason":"Invalid match control"}),
        )?;
        self.room_effect(&id, effect, Some(&actor), 0)
    }
    pub fn room_io_failure(&mut self, actor: Value) -> Result<Reply> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let effect = self.room_action(actor.clone(), "room_control_io_failure", json!({}))?;
        self.room_effect(&id, effect, Some(&actor), 0)
    }
    pub fn room_packet(&mut self, actor: Value, bytes: Vec<u8>) -> Result<()> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let hex = bytes.iter().map(|b| format!("{b:02x}")).collect::<String>();
        let effect = self.room_action(actor, "room_packets_route", json!({"packet":hex}))?;
        if let Some(who) = effect["close_peer"].as_str() {
            self.room_sockets.disconnect(&id, who, None);
        }
        if let Some(who) = effect["deliver"].as_str() {
            let hex = m::text(&effect["packet"], 2810)?;
            let bytes = m::hex(&json!(hex), hex.len() / 2)?;
            let recipient = effect["recipient_connection"]
                .as_u64()
                .ok_or(Error(503, "Native packet destination required"))?;
            let message = Outbound::Packet {
                bytes,
                target: m::integer(&effect["target"], 0, 15)? as u8,
                origin: effect["origin_connection"]
                    .as_u64()
                    .ok_or(Error(503, "Native packet origin required"))?,
                recipient,
            };
            if !self.room_sockets.send(&id, who, recipient, message) {
                self.room_sockets.disconnect(&id, who, Some(recipient));
            }
        }
        Ok(())
    }
    pub fn room_delivery(&mut self, actor: Value, packet: &Outbound) -> Result<bool> {
        let Outbound::Packet {
            bytes,
            target,
            origin,
            recipient,
        } = packet
        else {
            return Ok(true);
        };
        let effect = self.room_action(
            actor,
            "room_packets_delivery",
            json!({"packet":bytes.iter().map(|b|format!("{b:02x}")).collect::<String>(),
            "target":target,"origin_connection":origin,"recipient_connection":recipient}),
        )?;
        Ok(effect["kind"] == "send")
    }
    pub fn room_detach(&mut self, actor: Value) -> Result<()> {
        let id = m::text(&actor["id"], 64)?.to_owned();
        let mut r = actor.clone();
        r["op"] = json!("room_wire_detach");
        let current = self.room_request(r);
        let outcome: Result<()> = if current.as_ref().is_ok_and(|v| v == true) {
            (|| {
                let effect =
                    self.room_action(actor.clone(), "room_control_socket_lost", json!({}))?;
                self.room_effect(&id, effect, Some(&actor), 0)?;
                Ok(())
            })()
        } else {
            Ok(())
        };
        self.room_sockets.remove(
            &id,
            m::text(&actor["player"], 64)?,
            actor["connection"]
                .as_u64()
                .ok_or(Error(503, "Native detached connection required"))?,
        );
        outcome?;
        match current {
            Ok(_) | Err(Error(409, _)) => Ok(()),
            Err(e) => Err(e),
        }
    }
    pub(super) fn room_maintenance(&mut self) -> Result<()> {
        if self.queue.is_none() {
            return Ok(());
        }
        for actor in self.room_sockets.ended() {
            self.room_detach(actor)?;
        }
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let observed = facts(&mut self.owners, source, &clock)?;
        for row in m::array(&observed, "queue") {
            if self.room_request(json!({"op":"room_state_expired","id":row["id"]}))? == true {
                self.room_close(
                    m::text(&row["id"], 64)?,
                    1000,
                    "Match connection or loading timed out. Search again.",
                )?;
            }
        }
        Ok(())
    }
}
