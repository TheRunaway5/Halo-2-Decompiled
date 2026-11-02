//! Public client/controller tests. All provider, HTTP, clock and native owners
//! below are declared fixtures; installed input and genuine SDK play are separate.
use halo2_online_protocol::{matchmaking_client::*, matchmaking_readiness};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::sync::{Arc, Mutex};

fn config() -> Config {
    Config {
        input_valid: true,
        endpoint: "https://queue.example".into(),
        url: UrlParts {
            parsed: true,
            normalized: "https://queue.example/".into(),
            scheme: "https".into(),
            host: "queue.example".into(),
            path: "/".into(),
            ..Default::default()
        },
        maps: vec!["lockout".into()],
        compatibility: "halo2-native-queue-v16".into(),
        language: 0,
        poll_ms: 10,
        login_present: true,
        ..Default::default()
    }
}
fn profile() -> Profile {
    Profile {
        name: "Chosen Pilot".into(),
        ..Default::default()
    }
}
fn playlists() -> Value {
    json!({"playlists":[{"id":"ranked","revision":"1","name":"Ranked Slayer","description":"Fixture",
        "ranked":true,"quickmatch":true,"eligible":true,"minimum_players":2,"maximum_players":16,"level":1}]})
}
fn ticket(queued: bool) -> Value {
    json!({"state":if queued {"searching"} else {"idle"},"playlist_id":if queued {"ranked"} else {""},
        "queued_players":if queued {1} else {0},"match":null})
}
fn encoded(bytes: &[u8]) -> String {
    bytes.iter().map(|v| format!("{v:02x}")).collect()
}
fn decoded(s: &str) -> Vec<u8> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}
fn presets() -> Value {
    serde_json::from_str(include_str!("../data/tu15-result-presets.json")).unwrap()
}
fn matched() -> Value {
    let settings = presets()["variants"][0]["settings"]
        .as_str()
        .unwrap()
        .to_owned();
    json!({"state":"checking_connections","playlist_id":"ranked","queued_players":2,"start_countdown_ms":0,
        "match":{"map":"lockout","variant":"Slayer","game_id":"source-game","settings_sha256":encoded(&Sha256::digest(decoded(&settings))),
        "settings":settings,"players":[{"name":"Chosen Pilot","team":0,"self":true,"level":1},
        {"name":"Peer Pilot","team":1,"self":false,"level":2}]}})
}
fn descriptor() -> Value {
    json!({"id":"private-room","token":"private-room-token","slot":0,"local_slots":[0],"team_mode":false,
        "players":[{"name":"Chosen Pilot","team":0,"identity":"0123456789abcdef","peer":0,"guest":false},
        {"name":"Peer Pilot","team":1,"identity":"fedcba9876543210","peer":1,"guest":false}],
        "transport":{"kind":"eos-p2p-v1","socket":"Fixture","peers":[{"slot":1,"key":"private-peer-key"}],
        "session":{"playlist":"ranked","revision":"1","map":"lockout","variant":"Slayer"}}})
}

struct Data {
    now: u64,
    auth: Option<String>,
    locals: Vec<Grant>,
    calls: Vec<Request>,
    queued: bool,
    catalog: String,
    ticket: Option<String>,
    descriptor: Value,
    token: String,
    lease: u32,
    lose_post: bool,
    ticket_error: Option<(i64, String)>,
    inject: Option<(String, Command)>,
    phase: ConnectionPhase,
    installed: Vec<Admission>,
    qos: Vec<String>,
}
struct Fixture {
    client: Arc<Client>,
    data: Mutex<Data>,
}
impl Fixture {
    fn new(c: Config) -> Self {
        Self {
            client: Arc::new(Client::new(c)),
            data: Mutex::new(Data {
                now: 1,
                auth: Some("private-external-grant".into()),
                locals: vec![],
                calls: vec![],
                queued: false,
                catalog: playlists().to_string(),
                ticket: None,
                descriptor: descriptor(),
                token: "s".repeat(43),
                lease: 900,
                lose_post: false,
                ticket_error: None,
                inject: None,
                phase: ConnectionPhase::Missing,
                installed: vec![],
                qos: vec![],
            }),
        }
    }
    fn ready(&self) {
        self.client.command(Command::Refresh(profile()));
        self.client.poll(self);
        assert_eq!(self.client.snapshot().status, Status::Ready);
    }
    fn join(&self) {
        self.client.command(Command::Join("ranked".into()));
        self.client.poll(self);
    }
    fn count(&self, method: &str, path: &str) -> usize {
        self.data
            .lock()
            .unwrap()
            .calls
            .iter()
            .filter(|r| r.method == method && r.url.ends_with(path))
            .count()
    }
    fn start_match(&self) {
        self.ready();
        self.data.lock().unwrap().ticket = Some(matched().to_string());
        self.join();
        assert_eq!(self.data.lock().unwrap().installed.len(), 1);
    }
}
impl Io for Fixture {
    fn now_ms(&self) -> u64 {
        self.data.lock().unwrap().now
    }
    fn cancelled(&self, version: u64) -> bool {
        !self.client.current(version)
    }
    fn identity(&self, _: u64) -> Result<Identity> {
        let d = self.data.lock().unwrap();
        Ok(Identity {
            primary: d.auth.clone(),
            locals: d.locals.clone(),
        })
    }
    fn request(&self, r: &Request) -> Result<Response> {
        let mut d = self.data.lock().unwrap();
        d.calls.push(r.clone());
        assert!(r.url.starts_with("https://queue.example/v1/matchmaking/"));
        let session = r.url.ends_with("/session") && r.method == "POST";
        if session {
            assert_eq!(Some(&r.bearer), d.auth.as_ref());
        } else {
            assert_eq!(
                r.bearer, d.token,
                "External authorization escaped session exchange"
            );
        }
        if d.inject
            .as_ref()
            .is_some_and(|(path, _)| r.url.ends_with(path) && r.method == "POST")
        {
            let (_, command) = d.inject.take().unwrap();
            self.client.command(command);
        }
        let response = if session {
            json!({"token":d.token,"expires_in":d.lease})
        } else if r.method == "DELETE" {
            d.queued = false;
            json!({})
        } else if r.url.ends_with("/playlists") {
            return Ok(Response {
                status: 200,
                body: d.catalog.clone(),
            });
        } else if r.url.ends_with("/transport") {
            d.descriptor.clone()
        } else if r.url.ends_with("/failure") {
            json!({})
        } else {
            if let Some((status, body)) = &d.ticket_error {
                return Ok(Response {
                    status: *status,
                    body: body.clone(),
                });
            }
            if r.method == "POST" {
                d.queued = true;
                if std::mem::take(&mut d.lose_post) {
                    return Err(Error::Unavailable);
                }
            }
            if let Some(body) = &d.ticket {
                return Ok(Response {
                    status: 200,
                    body: body.clone(),
                });
            }
            ticket(d.queued)
        };
        Ok(Response {
            status: if session { 201 } else { 200 },
            body: response.to_string(),
        })
    }
    fn connection(&self, _: u64) -> Result<Connection> {
        Ok(Connection {
            phase: self.data.lock().unwrap().phase,
            error: "Fixture connection refused".into(),
        })
    }
    fn install(&self, version: u64, v: &Admission) -> Result<bool> {
        if !self.client.current(version) {
            return Ok(false);
        }
        let mut d = self.data.lock().unwrap();
        d.installed.push(v.clone());
        d.phase = ConnectionPhase::Connecting;
        Ok(true)
    }
    fn disconnect(&self, version: u64) {
        if self.client.current(version) {
            self.data.lock().unwrap().phase = ConnectionPhase::Missing;
        }
    }
    fn qos(&self, version: u64, _: &str, token: &str, _: bool) -> Result<bool> {
        if !self.client.current(version) {
            return Ok(false);
        }
        self.data.lock().unwrap().qos.push(token.into());
        Ok(true)
    }
}

#[test]
fn uncertain_post_keeps_one_rust_owned_request_key() {
    let f = Fixture::new(config());
    f.ready();
    f.data.lock().unwrap().lose_post = true;
    f.join();
    assert_eq!(f.client.snapshot().status, Status::Failed);
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Searching);
    let d = f.data.lock().unwrap();
    let keys: Vec<_> = d
        .calls
        .iter()
        .filter(|r| r.method == "POST" && r.url.ends_with("/ticket"))
        .map(|r| serde_json::from_str::<Value>(&r.body).unwrap()["request_id"].clone())
        .collect();
    assert_eq!(keys.len(), 2);
    assert_eq!(keys[0], keys[1]);
    assert_eq!(keys[0].as_str().unwrap().len(), 36);
    assert_eq!(keys[0].as_str().unwrap().as_bytes()[14], b'4');
}

#[test]
fn commands_during_http_cannot_publish_or_revive_the_old_queue() {
    for command in [Command::SignOut, Command::Cancel] {
        let f = Fixture::new(config());
        f.ready();
        f.data.lock().unwrap().inject = Some(("/ticket".into(), command));
        f.join();
        assert!(f.client.snapshot().game.is_none());
        assert_ne!(f.client.snapshot().status, Status::Searching);
        f.client.poll(&f);
        assert!(!f.data.lock().unwrap().queued);
        assert!(f.client.snapshot().game.is_none());
        assert_eq!(f.count("POST", "/ticket"), 1);
        assert_eq!(
            f.count("DELETE", "/ticket") + f.count("DELETE", "/ticket?cancel=1"),
            1
        );
    }
}

#[test]
fn late_successful_session_exchange_is_still_owned_for_cleanup() {
    let f = Fixture::new(config());
    f.data.lock().unwrap().inject = Some(("/session".into(), Command::SignOut));
    f.client.command(Command::Refresh(profile()));
    f.client.poll(&f);
    f.client.poll(&f);
    assert_eq!(f.count("DELETE", "/session"), 1);
    assert_eq!(f.count("GET", "/playlists"), 0);
    assert_eq!(f.client.snapshot().status, Status::Idle);
}

#[test]
fn lease_refresh_uses_fresh_external_grants_and_retains_accepted_search() {
    let f = Fixture::new(config());
    f.data.lock().unwrap().lease = 1;
    f.ready();
    f.join();
    let before = f.count("POST", "/ticket");
    {
        let mut d = f.data.lock().unwrap();
        d.now += 750;
        d.auth = Some("fresh-private-external-grant".into());
    }
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Searching);
    assert_eq!(f.count("POST", "/ticket"), before);
    assert_eq!(f.count("DELETE", "/ticket"), 0);
    let d = f.data.lock().unwrap();
    let r = d
        .calls
        .iter()
        .filter(|r| r.method == "POST" && r.url.ends_with("/session"))
        .last()
        .unwrap();
    assert_eq!(
        serde_json::from_str::<Value>(&r.body).unwrap()["renew_session"],
        d.token
    );
}

#[test]
fn passive_members_observe_the_leader_without_enqueuing() {
    let f = Fixture::new(config());
    f.ready();
    f.data.lock().unwrap().queued = true;
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Searching);
    assert_eq!(f.count("POST", "/ticket"), 0);
    f.client.command(Command::Cancel);
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Ready);
    assert_eq!(f.count("DELETE", "/ticket?cancel=1"), 1);
}

#[test]
fn explicit_terminal_failures_fence_retries_until_a_new_command() {
    for (kind, state) in [
        ("search_failed", 1),
        ("party_cancelled", 4),
        ("missing_content", 5),
        ("party_changed", 6),
    ] {
        let f = Fixture::new(config());
        f.ready();
        f.join();
        let mut body = ticket(true);
        body["failure"] = json!(kind);
        f.data.lock().unwrap().ticket = Some(body.to_string());
        f.client.poll(&f);
        assert_eq!(f.client.snapshot().failure_state, Some(state));
        let count = f.data.lock().unwrap().calls.len();
        for _ in 0..3 {
            f.client.poll(&f);
        }
        assert_eq!(f.data.lock().unwrap().calls.len(), count);
        f.data.lock().unwrap().ticket = None;
        f.client.command(Command::Cancel);
        f.client.poll(&f);
        assert_eq!(f.client.snapshot().failure_state, None);
        assert_eq!(f.client.snapshot().status, Status::Ready);
        f.join();
        assert_eq!(f.count("POST", "/ticket"), 2);
    }
}

#[test]
fn failed_requests_clear_presentation_but_keep_transient_retry_identity() {
    let f = Fixture::new(config());
    f.start_match();
    {
        let mut d = f.data.lock().unwrap();
        d.phase = ConnectionPhase::Loading;
        d.ticket_error = Some((
            503,
            json!({"error":"Fixture temporarily unavailable"}).to_string(),
        ));
    }
    f.client.poll(&f);
    assert_eq!(
        f.client.snapshot().message,
        "Fixture temporarily unavailable"
    );
    assert!(f.client.snapshot().game.is_none());
    assert_eq!(f.client.snapshot().start_ms, None);
    f.data.lock().unwrap().ticket_error = None;
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Loading);
    assert!(f.client.snapshot().game.is_some());
    assert_eq!(f.data.lock().unwrap().installed.len(), 1);
    assert_eq!(f.count("POST", "/ticket"), 1);
}

#[test]
fn missing_shared_countdown_refuses_launch_and_stops_automatic_retries() {
    let f = Fixture::new(config());
    f.start_match();
    {
        let mut d = f.data.lock().unwrap();
        d.phase = ConnectionPhase::Loading;
        let mut body = matched();
        body.as_object_mut().unwrap().remove("start_countdown_ms");
        d.ticket = Some(body.to_string());
    }
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Failed);
    assert!(!f.client.snapshot().start_supported);
    let count = f.data.lock().unwrap().calls.len();
    f.client.poll(&f);
    assert_eq!(f.data.lock().unwrap().calls.len(), count);
}

fn launch() -> matchmaking_readiness::NativeLaunchObservation {
    matchmaking_readiness::NativeLaunchObservation {
        session_present: true,
        transferred: false,
        loading: true,
        start_supported: true,
        start_ms: Some(0),
        team_mode: Some(false),
        current_peer: 0,
    }
}

#[test]
fn launch_is_bound_to_the_observed_shared_zero_and_transfers_once() {
    let f = Fixture::new(config());
    f.start_match();
    let mut connecting = launch();
    connecting.loading = false;
    assert!(!f.client.take_launch(connecting, &[]));
    {
        let mut d = f.data.lock().unwrap();
        d.phase = ConnectionPhase::Loading;
        let mut body = matched();
        body["start_countdown_ms"] = json!(1);
        d.ticket = Some(body.to_string());
    }
    f.client.poll(&f);
    assert!(!f.client.take_launch(launch(), &[]));
    f.data.lock().unwrap().ticket = Some(matched().to_string());
    f.client.poll(&f);
    assert!(f.client.take_launch(launch(), &[]));
    assert!(!f.client.take_launch(launch(), &[]));
    let count = f.count("POST", "/ticket");
    f.client.poll(&f);
    assert_eq!(f.count("POST", "/ticket"), count);
}

#[test]
fn actual_content_refusal_notifies_the_bound_room_and_keeps_original_status_five() {
    let f = Fixture::new(config());
    assert!(!f.client.reject_content());
    f.start_match();
    assert!(f.client.reject_content());
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().failure_state, Some(5));
    let d = f.data.lock().unwrap();
    let request = d
        .calls
        .iter()
        .find(|r| r.url.ends_with("/failure"))
        .unwrap();
    assert_eq!(
        serde_json::from_str::<Value>(&request.body).unwrap(),
        json!({"match_id":"private-room","kind":"missing_content"})
    );
}

#[test]
fn configuration_policy_rejects_unsafe_origins_duplicate_maps_and_invalid_clocks() {
    let mut cases = Vec::new();
    let mut c = config();
    c.endpoint = "https://user@queue.example".into();
    cases.push(c);
    let mut c = config();
    c.endpoint += "/path";
    c.url.path = "/path".into();
    cases.push(c);
    let mut c = config();
    c.url.scheme = "http".into();
    cases.push(c);
    let mut c = config();
    c.maps.push(c.maps[0].clone());
    cases.push(c);
    let mut c = config();
    c.maps.clear();
    cases.push(c);
    let mut c = config();
    c.poll_ms = -1;
    cases.push(c);
    let mut c = config();
    c.language = 9;
    cases.push(c);
    let mut c = config();
    c.login_present = false;
    cases.push(c);
    let mut c = config();
    c.url.parsed = false;
    cases.push(c);
    for c in cases {
        let f = Fixture::new(c);
        assert!(!f.client.configured());
        assert_eq!(f.client.snapshot().status, Status::Failed);
    }
    let mut c = config();
    c.endpoint.clear();
    let f = Fixture::new(c);
    assert_eq!(f.client.snapshot().status, Status::Unconfigured);
}

#[test]
fn strict_catalog_does_not_partially_publish_private_or_duplicate_fields() {
    let mut bodies = vec![
        "{\"playlists\":[],\"playlists\":[]}".into(),
        " ".repeat(65537),
        "[".repeat(18) + &"]".repeat(18),
    ];
    let mut v = playlists();
    v["playlists"][0]["private_id"] = json!("private");
    bodies.push(v.to_string());
    let mut v = playlists();
    v["playlists"][0]["level"] = json!(1.5);
    bodies.push(v.to_string());
    for body in bodies {
        let f = Fixture::new(config());
        f.data.lock().unwrap().catalog = body;
        f.client.command(Command::Refresh(profile()));
        f.client.poll(&f);
        assert_eq!(f.client.snapshot().status, Status::Failed);
        assert!(f.client.snapshot().playlists.is_empty());
    }
}

#[test]
fn transport_roster_names_teams_owners_guests_and_session_are_bound_before_installation() {
    for case in 0..11 {
        let f = Fixture::new(config());
        f.ready();
        let mut d = f.data.lock().unwrap();
        d.ticket = Some(matched().to_string());
        match case {
            0 => d.descriptor["players"][0]["name"] = json!("Forged"),
            1 => d.descriptor["players"][0]["team"] = json!(1),
            2 => d.descriptor["players"][0]["identity"] = json!("0123456789abcdeF"),
            3 => d.descriptor["players"][0]["peer"] = json!(1),
            4 => d.descriptor["players"][0]["guest"] = json!(true),
            5 => d.descriptor["local_slots"] = json!([0, 0]),
            6 => d.descriptor["slot"] = json!(1),
            7 => d.descriptor["transport"]["session"]["map"] = json!("coagulation"),
            8 => d.descriptor["transport"]["session"]["playlist"] = json!("other"),
            9 => d.descriptor["players"][1]["peer"] = json!(0),
            _ => d.descriptor["unexpected"] = json!(true),
        }
        drop(d);
        f.join();
        assert_eq!(
            f.client.snapshot().status,
            Status::Failed,
            "descriptor case {case}"
        );
        assert!(f.data.lock().unwrap().installed.is_empty());
    }
}

#[test]
fn every_authored_preset_keeps_its_complete_304_byte_ticket() {
    let presets = presets();
    let rows = presets["variants"].as_array().unwrap();
    assert_eq!(rows.len(), 39);
    for row in rows {
        let f = Fixture::new(config());
        f.ready();
        let mut body = matched();
        let settings = decoded(row["settings"].as_str().unwrap());
        body["match"]["settings"] = row["settings"].clone();
        body["match"]["variant"] = row["name"].clone();
        body["match"]["settings_sha256"] = json!(encoded(&Sha256::digest(&settings)));
        {
            let mut d = f.data.lock().unwrap();
            d.ticket = Some(body.to_string());
            d.descriptor["transport"]["session"]["variant"] = row["name"].clone();
        }
        f.join();
        assert_eq!(
            f.client
                .snapshot()
                .game
                .unwrap()
                .settings
                .unwrap()
                .as_slice(),
            settings.as_slice()
        );
        assert_eq!(f.data.lock().unwrap().installed.len(), 1);
    }
}

#[test]
fn every_settings_byte_is_digest_bound_and_source_invalid_settings_are_refused() {
    let baseline = matched();
    for offset in 0..304 {
        let f = Fixture::new(config());
        f.ready();
        let mut body = baseline.clone();
        let mut bytes = decoded(body["match"]["settings"].as_str().unwrap());
        bytes[offset] ^= 1;
        body["match"]["settings"] = json!(encoded(&bytes));
        f.data.lock().unwrap().ticket = Some(body.to_string());
        f.join();
        assert!(
            f.data.lock().unwrap().installed.is_empty(),
            "digest byte {offset}"
        );
        assert!(f.client.snapshot().game.is_none());
    }
    let f = Fixture::new(config());
    f.ready();
    let mut body = baseline;
    let presets = presets();
    let row = presets["variants"]
        .as_array()
        .unwrap()
        .iter()
        .find(|v| v["engine"] == 4)
        .unwrap();
    let mut bytes = decoded(row["settings"].as_str().unwrap());
    bytes[0xf4..0xf6].copy_from_slice(&[255, 255]);
    body["match"]["settings"] = json!(encoded(&bytes));
    body["match"]["settings_sha256"] = json!(encoded(&Sha256::digest(&bytes)));
    f.data.lock().unwrap().ticket = Some(body.to_string());
    f.join();
    assert!(f.data.lock().unwrap().installed.is_empty());
}

#[test]
fn local_signed_grants_guests_and_appearance_use_only_the_declared_session_body() {
    let f = Fixture::new(config());
    let mut p = profile();
    p.appearance = Some([0, 0, 0, 0, 1, 2, 3, 4, 5]);
    p.locals = vec![
        LocalPlayer {
            name: "Guest".into(),
            signed_in: false,
            appearance: None,
        },
        LocalPlayer {
            name: "Signed".into(),
            signed_in: true,
            appearance: None,
        },
    ];
    f.data.lock().unwrap().locals = vec![
        Grant::default(),
        Grant {
            authorization: Some("private-secondary-grant".into()),
            steam: true,
        },
    ];
    f.client.command(Command::Refresh(p));
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Ready);
    let d = f.data.lock().unwrap();
    let request = d
        .calls
        .iter()
        .find(|r| r.method == "POST" && r.url.ends_with("/session"))
        .unwrap();
    let body: Value = serde_json::from_str(&request.body).unwrap();
    assert_eq!(body["appearance"], "000000000102030405");
    assert_eq!(body["local_players"][0], json!({"screen_name":"Guest"}));
    assert_eq!(
        body["local_players"][1],
        json!({"screen_name":"Signed","identity_token":"private-secondary-grant"})
    );
    assert!(body.get("player_id").is_none());
}

#[test]
fn speaker_binding_uses_the_admitted_row_and_physical_owner() {
    let f = Fixture::new(config());
    f.start_match();
    assert!(f.client.speaker_binding(0, "Chosen Pilot", 0, 0, 0));
    assert!(!f.client.speaker_binding(0, "Peer Pilot", 0, 0, 0));
    assert!(!f.client.speaker_binding(1, "Peer Pilot", 1, 1, 0));
    f.client.command(Command::Cancel);
    assert!(!f.client.speaker_binding(0, "Chosen Pilot", 0, 0, 0));
}

#[test]
fn shutdown_revokes_the_owned_session_without_external_grant_reuse() {
    let f = Fixture::new(config());
    f.ready();
    f.join();
    f.client.shutdown(&f);
    assert_eq!(f.count("DELETE", "/session"), 1);
    assert!(!f.data.lock().unwrap().queued);
}

#[test]
fn invalid_native_input_is_a_rust_decision_and_a_fresh_command_can_recover() {
    let mut c = config();
    c.input_valid = false;
    assert_eq!(Client::new(c).snapshot().status, Status::Failed);
    let f = Fixture::new(config());
    f.ready();
    let before = f.data.lock().unwrap().calls.len();
    f.client.command(Command::Invalid);
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Failed);
    assert_eq!(f.data.lock().unwrap().calls.len(), before);
    f.client.command(Command::Refresh(profile()));
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Ready);
}

#[test]
fn runtime_protocol_comes_from_the_actual_shared_transport_configuration() {
    assert_eq!(runtime_compatibility(false), "halo2-native-queue-v12");
    assert_eq!(runtime_compatibility(true), "halo2-native-queue-v16");
}

#[test]
fn invalid_profile_cannot_reuse_a_previous_name_through_join_or_cancel() {
    let f = Fixture::new(config());
    f.ready();
    let before = f.data.lock().unwrap().calls.len();
    for command in [
        Command::InvalidProfile,
        Command::Join("ranked".into()),
        Command::Cancel,
    ] {
        f.client.command(command);
        f.client.poll(&f);
        assert_eq!(f.client.snapshot().status, Status::Failed);
        assert_eq!(f.data.lock().unwrap().calls.len(), before);
    }
    f.client.command(Command::Refresh(profile()));
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Ready);
    f.client.command(Command::Invalid);
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Failed);
    f.client.command(Command::Cancel);
    f.client.poll(&f);
    assert_eq!(f.client.snapshot().status, Status::Ready);
}

#[test]
fn session_rotation_cannot_enqueue_or_install_again_after_gameplay_takes_the_room() {
    let f = Fixture::new(config());
    f.start_match();
    f.data.lock().unwrap().phase = ConnectionPhase::Loading;
    f.client.poll(&f);
    assert!(f.client.take_launch(launch(), &[]));
    let before = f.count("POST", "/ticket");
    {
        let mut d = f.data.lock().unwrap();
        d.token = "r".repeat(43);
        d.now += 900_000;
    }
    f.client.poll(&f);
    assert_eq!(
        f.count("POST", "/session"),
        2,
        "The lease must actually renew"
    );
    assert_eq!(f.client.snapshot().status, Status::Loading);
    assert_eq!(f.data.lock().unwrap().qos.last(), Some(&"r".repeat(43)));
    assert_eq!(
        f.count("POST", "/ticket"),
        before,
        "Lease rotation re-enqueued a transferred native room"
    );
    assert_eq!(
        f.data.lock().unwrap().installed.len(),
        1,
        "Lease rotation installed a second native connection"
    );
}
