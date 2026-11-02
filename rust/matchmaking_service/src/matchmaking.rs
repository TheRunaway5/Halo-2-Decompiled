//! Session, accepted-party and catalog coordination inside the Rust runtime.
//! Provider identities are private receipts; HTTP cannot supply canonical facts,
//! session generations, clan membership, consent handles or SQL plans.
use super::*;
use database::{Clock, Sources};
use halo2_online_protocol::{matchmaking_coordinator, matchmaking_model as m};

fn queue(owners: &mut Owners, source: u64, mut request: Value, clock: &Clock) -> Result<Value> {
    request["now"] = json!(clock.monotonic);
    owners.request(source, &request)
}
fn drive(
    owners: &mut Owners,
    db: &Connection,
    setup: Value,
    clock: &Clock,
    sources: Sources,
) -> Result<Value> {
    let (id, value) = database::drive(owners, db, setup, clock, sources)?;
    owners.retire(id);
    Ok(value)
}
fn directory(config: &Value, mode: &str, who: Option<&str>, body: Value) -> Value {
    let mut setup = config.clone();
    let row = &mut setup["party_directory"];
    row["mode"] = json!(mode);
    row["player"] = json!(who);
    row["body"] = body;
    setup
}
fn events(
    owners: &mut Owners,
    db: &Connection,
    source: u64,
    config: &Value,
    clock: &Clock,
    sources: Sources,
) -> Result<()> {
    let retired = queue(owners, source, json!({"op":"session_events"}), clock)?;
    for event in retired
        .as_array()
        .ok_or(Error(503, "Native session retirement events required"))?
    {
        drive(
            owners,
            db,
            directory(
                config,
                "session_lost",
                event["player"].as_str(),
                Value::Null,
            ),
            clock,
            sources,
        )?;
    }
    Ok(())
}
fn tick(owners: &mut Owners, db: &Connection, source: u64, clock: &Clock) -> Result<()> {
    let mut statement = db
        .prepare("SELECT id,version FROM parties LIMIT 1001")
        .map_err(database::failure)?;
    let versions = statement
        .query_map([], |r| {
            Ok(json!({"id":r.get::<_,String>(0)?,"version":r.get::<_,i64>(1)?}))
        })
        .map_err(database::failure)?
        .collect::<std::result::Result<Vec<_>, _>>()
        .map_err(database::failure)?;
    queue(
        owners,
        source,
        json!({"op":"party_countdown_tick","clock_ms":(clock.monotonic*1000.) as i64,"versions":versions}),
        clock,
    )?;
    Ok(())
}
fn gathering(
    owners: &mut Owners,
    source: u64,
    playlist: &Value,
    clock: &Clock,
    sockets: &super::search_qos::Sockets,
) -> Result<()> {
    let mut event = queue(
        owners,
        source,
        json!({"op":"gather_advance","playlist":playlist,"wall":clock.wall}),
        clock,
    )?;
    for _ in 0..16384 {
        if event["kind"] == "done" {
            return Ok(());
        }
        if event["kind"] != "io" {
            return Err(Error(503, "Unknown native gathering response"));
        }
        let result = match event["action"].as_str() {
            Some("facts") => {
                let mut ready = Vec::new();
                let mut pairs = Vec::new();
                for player in m::array(&event, "players") {
                    ready.push(sockets.request(
                        owners,
                        source,
                        json!({"op":"search_qos_ready","player":player}),
                        clock,
                    )?);
                }
                for pair in m::array(&event, "pairs") {
                    let measured = sockets.request(
                        owners,
                        source,
                        json!({"op":"search_qos_measurement","a":pair[0],"b":pair[1]}),
                        clock,
                    )?;
                    let pending = sockets.request(
                        owners,
                        source,
                        json!({"op":"search_qos_pending","a":pair[0],"b":pair[1]}),
                        clock,
                    )?;
                    pairs.push(json!({"milliseconds":measured["milliseconds"],"pending":pending}));
                }
                json!({"ready":ready,"pairs":pairs})
            }
            Some("request") => {
                let mut results = Vec::new();
                for pair in m::array(&event, "pairs") {
                    results.push(sockets.request(
                        owners,
                        source,
                        json!({"op":"search_qos_request","a":pair[0],"b":pair[1]}),
                        clock,
                    )?);
                }
                json!(results)
            }
            _ => return Err(Error(503, "Unknown native gathering IO")),
        };
        event = queue(
            owners,
            source,
            json!({"op":"gather_resume","frame":event["frame"],"result":result}),
            clock,
        )?;
    }
    Err(Error(503, "Native gathering IO exceeds bound"))
}
fn queue_io(
    owners: &mut Owners,
    db: &Connection,
    source: u64,
    config: &Value,
    generation: &Value,
    mut event: Value,
    clock: &Clock,
    sources: Sources,
    sockets: &super::search_qos::Sockets,
) -> Result<Value> {
    for _ in 0..128 {
        if event["kind"] == "done" {
            return Ok(event["value"].clone());
        }
        if event["kind"] != "io" {
            return Err(Error(503, "Unknown native queue response"));
        }
        let result = match event["action"].as_str() {
            Some("database") => drive(
                owners,
                db,
                json!({"database":{"mode":"snapshot_bank","operations":event["operations"]}}),
                clock,
                sources,
            )?,
            Some("gathering_advance") => {
                gathering(owners, source, &event["playlist"], clock, sockets)?;
                Value::Null
            }
            Some("room_start") => {
                let observed = super::rooms::facts(owners, source, clock)?;
                let row = m::array(&observed, "queue")
                    .iter()
                    .find(|r| r["id"] == event["room"])
                    .ok_or(Error(409, "Native start room retired"))?;
                let actual = sources.rooms.facts();
                let connected = actual
                    .as_array()
                    .unwrap()
                    .iter()
                    .find(|r| r["id"] == event["room"])
                    .map(|r| {
                        m::array(r, "peers")
                            .iter()
                            .filter(|p| p["closed"] == false)
                            .map(|p| p["player"].clone())
                            .collect::<Vec<_>>()
                    })
                    .unwrap_or_default();
                let now = sources.rooms.now();
                owners.request(source,&json!({"op":"room_setup_start","id":event["room"],"now":now,"io_now":now,"clock_ms":(now*1000.) as i64,
                    "active":row["active"],"connected":connected}))?
            }
            _ => return Err(Error(503, "Unknown native queue IO")),
        };
        let facts = drive(owners, db, config.clone(), clock, sources)?;
        let rooms = super::rooms::facts(owners, source, clock)?;
        event = queue(
            owners,
            source,
            json!({"op":"api_resume","frame":event["frame"],"generation":generation,
            "result":result,"facts":facts,"match_rooms":rooms["matching"],"rooms":rooms["queue"]}),
            clock,
        )?;
    }
    Err(Error(503, "Native queue IO exceeds bound"))
}
impl Runtime {
    pub(super) fn queue_source(&self) -> Result<u64> {
        self.queue
            .ok_or(Error(503, "Matchmaking playlists are not configured"))
    }
    pub fn matchmaking_body(&self, body: &Value) -> Result<Value> {
        self.queue_source()?;
        matchmaking_coordinator::value(&json!({"op":"session_body","body":body}))
    }
    /// Called only after every primary/signed-local provider proof completed.
    /// All canonical writes and provisional session effects share one commit.
    pub fn matchmaking_login(
        &mut self,
        body: Value,
        identities: Vec<Option<Identity>>,
    ) -> Result<Value> {
        let source = self.queue_source()?;
        let plan = self.matchmaking_body(&body)?;
        if identities.len() != 1 + m::array(&plan, "locals").len() || identities[0].is_none() {
            return Err(Error(503, "Verified session seat receipts required"));
        }
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let identity_config = self.config(
            "identity",
            "admission_identities",
            None,
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        let party_config = self.config(
            "party",
            "session_lost",
            None,
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let mut accounts = Vec::new();
                let mut seats = Vec::new();
                for (index, identity) in identities.iter().enumerate() {
                    let local = (index > 0).then(|| &plan["locals"][index - 1]);
                    if local.is_some_and(|p| p["proof"].is_null() != identity.is_none()) {
                        return Err(Error(503, "Verified local proof receipt differs"));
                    }
                    if let Some(identity) = identity {
                        // The transaction may have waited for another connection.
                        // Read fresh wall time after its write lock is acquired.
                        if let Some(digest) = &identity.steam_digest {
                            let subject=db.query_row("SELECT subject FROM steam_login_sessions WHERE token_hash=? AND expires>?",
                            rusqlite::params![digest,database::wall_now()?],|r|r.get::<_,String>(0)).optional().map_err(database::failure)?;
                            if subject.as_deref() != Some(&identity.subject) {
                                return Err(Error(401, "Steam sign-in expired"));
                            }
                        }
                        let owner = json!([identity.provider, identity.subject]);
                        matchmaking_coordinator::value(
                            &json!({"op":"session_verified_identity","owner":owner}),
                        )?;
                        accounts.push(owner.clone());
                        seats.push(json!({"owner":owner,"screen_name":local.map_or(&plan["screen_name"],|p|&p["screen_name"]),"username":identity.username}));
                    } else {
                        accounts.push(Value::Null);
                        seats.push(Value::Null);
                    }
                }
                queue(owners, source, json!({"op":"session_cleanup"}), &clock)?;
                events(owners, db, source, &party_config, &clock, sources)?;
                let facts = drive(
                    owners,
                    db,
                    directory(
                        &identity_config,
                        "admission_identities",
                        None,
                        json!(accounts),
                    ),
                    &clock,
                    sources,
                )?;
                let rooms = super::rooms::facts(owners, source, &clock)?;
                queue(
                    owners,
                    source,
                    json!({"op":"session_check","body":body,"identities":facts,"rooms":rooms["sessions"]}),
                    &clock,
                )?;
                let resolved = drive(
                    owners,
                    db,
                    directory(&identity_config, "resolve_seats", None, json!(seats)),
                    &clock,
                    sources,
                )?;
                let admitted = queue(
                    owners,
                    source,
                    json!({"op":"session_register","body":body,"identities":facts,"resolved":resolved,"rooms":rooms["sessions"]}),
                    &clock,
                )?;
                events(owners, db, source, &party_config, &clock, sources)?;
                Ok(admitted["response"].clone())
            },
        )
    }
    pub(super) fn matchmaking_auth(&mut self, header: &str) -> Result<Value> {
        self.maintenance()?;
        let source = self.queue_source()?;
        let clock = self.clock()?;
        queue(
            &mut self.owners,
            source,
            json!({"op":"session_auth","header":header}),
            &clock,
        )
    }
    pub fn matchmaking_logout(&mut self, header: &str) -> Result<Value> {
        let auth = self.matchmaking_auth(header)?;
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let config = self.config(
            "party",
            "session_lost",
            None,
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                queue(
                    owners,
                    source,
                    json!({"op":"session_revoke","key":auth["key"]}),
                    &clock,
                )?;
                events(owners, db, source, &config, &clock, sources)?;
                Ok(json!({"state":"signed_out"}))
            },
        )
    }
    pub fn playlists(&mut self, header: &str) -> Result<Value> {
        let auth = self.matchmaking_auth(header)?;
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let who = m::text(&auth["player"], 64)?;
        let setup = self.config(
            "party",
            "facts",
            Some(who),
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let facts = drive(owners, db, setup, &clock, sources)?;
                let players = if facts["party"].is_null() {
                    vec![auth["player"].clone()]
                } else {
                    m::array(&facts["party"], "members")
                        .iter()
                        .map(|p| p["player"].clone())
                        .collect()
                };
                let rooms = super::rooms::facts(owners, source, &clock)?;
                let context = queue(
                    owners,
                    source,
                    json!({"op":"party_directory_context","players":players,"rooms":rooms["matching"]}),
                    &clock,
                )?;
                let connections=m::array(&json!({"rows":context}),"rows").iter().map(|r| {
                let s=&r["session"];
                if s.is_null() { Value::Null } else {
                    let seats=m::array(s,"local_players").iter().map(|p|json!({"id":p["player"],"guest":p["guest"],
                        "clan":m::array(&facts,"clans").iter().find(|c|c["player"]==p["player"]).map(|c|c["clan"].clone())})).collect::<Vec<_>>();
                    json!({"id":s["player"],"compatibility":s["compatibility"],"maps":s["maps"],"seats":seats})
                }
            }).collect::<Vec<_>>();
                let clan = db
                    .query_row("SELECT clan FROM clan_members WHERE player=?", [who], |r| {
                        r.get::<_, String>(0)
                    })
                    .optional()
                    .map_err(database::failure)?;
                let requested = queue(
                    owners,
                    source,
                    json!({"op":"catalog_display_requests"}),
                    &clock,
                )?;
                let mut levels = serde_json::Map::new();
                for playlist in requested
                    .as_array()
                    .ok_or(Error(503, "Catalog display requests required"))?
                {
                    let level = drive(
                        owners,
                        db,
                        json!({"database":{"mode":"public_rank","source":source,"playlist":playlist,"player":who,"clan":clan}}),
                        &clock,
                        sources,
                    )?;
                    levels.insert(m::text(playlist, 64)?.into(), level);
                }
                queue(
                    owners,
                    source,
                    json!({"op":"catalog_public_view","compatibility":auth["compatibility"],"connections":connections,"levels":levels}),
                    &clock,
                )
            },
        )
    }
    pub fn ticket(
        &mut self,
        header: &str,
        method: &str,
        body: Value,
        query: Value,
    ) -> Result<Value> {
        let body = if method == "POST" {
            matchmaking_coordinator::value(&json!({"op":"session_queue_body","body":body}))?
        } else {
            body
        };
        let auth = self.matchmaking_auth(header)?;
        let source = self.queue_source()?;
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let who = m::text(&auth["player"], 64)?;
        let config = self.config(
            "party",
            "facts",
            Some(who),
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        let sockets = &self.qos_sockets;
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let rooms = super::rooms::facts(owners, source, &clock)?;
                if method == "DELETE" {
                    return queue(
                        owners,
                        source,
                        json!({"op":"api_cancel","generation":auth["generation"],"query":query,"rooms":rooms["queue"]}),
                        &clock,
                    );
                }
                if method == "POST" {
                    let facts = drive(owners, db, config.clone(), &clock, sources)?;
                    let event = queue(
                        owners,
                        source,
                        json!({"op":"api_enqueue_begin","generation":auth["generation"],"body":body,"facts":facts,"match_rooms":rooms["matching"]}),
                        &clock,
                    )?;
                    queue_io(
                        owners,
                        db,
                        source,
                        &config,
                        &auth["generation"],
                        event,
                        &clock,
                        sources,
                        sockets,
                    )?;
                }
                let rooms = super::rooms::facts(owners, source, &clock)?;
                let event = queue(
                    owners,
                    source,
                    json!({"op":"api_view_begin","generation":auth["generation"],"query":query,"heartbeat":method=="GET","rooms":rooms["queue"]}),
                    &clock,
                )?;
                queue_io(
                    owners,
                    db,
                    source,
                    &config,
                    &auth["generation"],
                    event,
                    &clock,
                    sources,
                    sockets,
                )
            },
        )
    }
    pub fn party(
        &mut self,
        header: &str,
        mode: &str,
        body: Value,
        features: Value,
    ) -> Result<Value> {
        self.maintenance()?;
        let auth = self.session(header)?;
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let who = m::text(&auth["player"], 64)?;
        let key = m::text(&auth["key"], 128)?;
        let setup = self.config(
            "party",
            mode,
            Some(who),
            Some(key),
            body.clone(),
            features,
            &clock,
        );
        database::transaction(&mut self.owners, &mut self.db, self.queue, |owners, db| {
            if mode != "command" {
                return drive(owners, db, setup, &clock, sources);
            }
            let probe = drive(
                owners,
                db,
                directory(&setup, "command_probe", Some(who), body.clone()),
                &clock,
                sources,
            )?;
            match probe["route"].as_str() {
                Some("command") => {
                    drive(owners, db, setup.clone(), &clock, sources)?;
                }
                Some("view") => {}
                Some("steam") => {
                    return Err(Error(503, "Steam invitation lookup capability unavailable"));
                }
                Some("countdown") => {
                    let source = sources
                        .queue
                        .ok_or(Error(503, "Matchmaking playlists are not configured"))?;
                    let facts = drive(
                        owners,
                        db,
                        directory(&setup, "facts", Some(who), Value::Null),
                        &clock,
                        sources,
                    )?;
                    let request = m::text(&body["request_id"], 36)?;
                    let previous = db
                        .query_row(
                            "SELECT payload FROM party_requests WHERE player=? AND request=?",
                            [who, request],
                            |r| r.get::<_, String>(0),
                        )
                        .optional()
                        .map_err(database::failure)?;
                    let count = db
                        .query_row(
                            "SELECT count(*) FROM party_requests WHERE player=?",
                            [who],
                            |r| r.get::<_, i64>(0),
                        )
                        .map_err(database::failure)?;
                    let rooms = super::rooms::facts(owners, source, &clock)?;
                    let plan = queue(
                        owners,
                        source,
                        json!({"op":"party_countdown_prepare","player":who,"body":body,"facts":facts,"rooms":rooms["matching"],
                        "clock_ms":(clock.monotonic*1000.) as i64,"wall_seconds":clock.wall,"requests":{"payload":previous,"count":count}}),
                        &clock,
                    )?;
                    database::sql(
                        db,
                        &json!({"mode":"batch","statements":plan["statements"],"frame":null}),
                    )?;
                    if !plan["token"].is_null() {
                        queue(
                            owners,
                            source,
                            json!({"op":"party_countdown_commit","token":plan["token"],"facts":facts,"rooms":rooms["matching"]}),
                            &clock,
                        )?;
                    }
                }
                _ => return Err(Error(503, "Unknown native party route")),
            }
            let mut view = directory(&setup, "view", Some(who), Value::Null);
            view["party_directory"]["features"] = probe["features"].clone();
            drive(owners, db, view, &clock, sources)
        })
    }
    pub fn party_countdown(&mut self, header: &str) -> Result<Value> {
        self.maintenance()?;
        let clock = self.clock()?;
        let auth = self.account("auth", json!({"header":header,"rate":false}), &clock)?;
        self.account(
            "budget",
            json!({"player":auth["player"],"bucket":"countdown"}),
            &clock,
        )?;
        let who = m::text(&auth["player"], 64)?;
        let facts = self.directory("party", "facts", Some(who), None, Value::Null, Value::Null)?;
        if let Some(source) = self.queue {
            let clock = self.clock()?;
            queue(
                &mut self.owners,
                source,
                json!({"op":"party_countdown_view","player":who,"facts":facts}),
                &clock,
            )
        } else {
            Ok(json!({"version":facts["party"]["version"],"countdown":null}))
        }
    }
    pub fn maintenance(&mut self) -> Result<()> {
        self.voice_maintenance()?;
        let Some(source) = self.queue else {
            return Ok(());
        };
        let clock = self.clock()?;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let setup = self.config(
            "party",
            "session_lost",
            None,
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        database::transaction(
            &mut self.owners,
            &mut self.db,
            Some(source),
            |owners, db| {
                let subjects = queue(owners, source, json!({"op":"api_cleanup_subjects"}), &clock)?;
                let mut clans = serde_json::Map::new();
                for player in subjects
                    .as_array()
                    .ok_or(Error(503, "Native clan cleanup subjects required"))?
                {
                    let who = m::text(player, 64)?;
                    let clan = db
                        .query_row("SELECT clan FROM clan_members WHERE player=?", [who], |r| {
                            r.get::<_, String>(0)
                        })
                        .optional()
                        .map_err(database::failure)?;
                    clans.insert(who.to_owned(), json!(clan));
                }
                queue(
                    owners,
                    source,
                    json!({"op":"api_cleanup","clans":clans}),
                    &clock,
                )?;
                queue(owners, source, json!({"op":"session_cleanup"}), &clock)?;
                events(owners, db, source, &setup, &clock, sources)?;
                queue(owners, source, json!({"op":"gather_cleanup"}), &clock)?;
                tick(owners, db, source, &clock)
            },
        )?;
        self.room_maintenance()
    }
}
