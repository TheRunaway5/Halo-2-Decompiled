//! Modern native account-controller contract. This is not an Xbox Live oracle.
use halo2_online_protocol::{matchmaking_account_client::*, matchmaking_origin::UrlParts};
use serde_json::{Value, json};
use std::{
    cell::{Cell, RefCell},
    sync::Arc,
};

const SECOND: i64 = 1_000_000_000;
const GRANT: &str = "PRIVATE-ACCOUNT-GRANT";
const SUBJECT: &str = "PRIVATE-PROVIDER-SUBJECT";
const HANDLE: &str = "00000000-0000-0000-0000-000000000001";

#[derive(Clone, Debug)]
struct Request {
    method: String,
    path: String,
    body: String,
    bearer: String,
    cleanup: bool,
}
type Hook = Box<dyn Fn(&Env, &Request) -> Option<Result<Reply>>>;
struct Env {
    client: Arc<Client>,
    now: Cell<i64>,
    held: Cell<bool>,
    cancelled: Cell<bool>,
    identity: RefCell<Option<Identity>>,
    requests: RefCell<Vec<Request>>,
    hook: RefCell<Option<Hook>>,
    entropy: Cell<u8>,
    party: RefCell<Value>,
    eos: Cell<bool>,
}
fn party_fixture() -> Value {
    json!({"party":{"version":7,"leader":true,"members":[{"handle":HANDLE,"name":"Pilot","ready_playlist":"","self":true,"leader":true,"online":true}],
        "outgoing":[],"countdown":null,"privacy":{"mode":0,"revision":1,"context":HANDLE}},"invitations":[]})
}
impl Env {
    fn new() -> Self {
        Self::configured(false, false)
    }
    fn configured(steam: bool, profile_only: bool) -> Self {
        let endpoint = "https://account.example".to_string();
        let client = Arc::new(Client::new(Config {
            input_valid: true,
            endpoint: endpoint.clone(),
            url: UrlParts {
                parsed: true,
                normalized: format!("{endpoint}/"),
                scheme: "https".into(),
                host: "account.example".into(),
                path: "/".into(),
                ..Default::default()
            },
            login_present: true,
            poll_ms: 10,
            parties: true,
            steam,
            profile_only,
            ..Default::default()
        }));
        Self {
            client,
            now: Cell::new(100 * SECOND),
            held: Cell::new(false),
            cancelled: Cell::new(false),
            identity: RefCell::new(Some(Identity {
                bearer: "PRIVATE-OAUTH".into(),
                subject: SUBJECT.into(),
            })),
            requests: RefCell::new(vec![]),
            hook: RefCell::new(None),
            entropy: Cell::new(0),
            party: RefCell::new(party_fixture()),
            eos: Cell::new(false),
        }
    }
    // Reentry into the public Rust snapshot must be possible at every IO seam.
    fn reenter(&self) {
        let _ = self.client.snapshot();
    }
    fn public<T>(&self, f: impl FnOnce(&Client, &Self) -> T) -> T {
        self.lock().unwrap();
        let value = f(&self.client, self);
        self.unlock().unwrap();
        value
    }
    fn connect(&self) {
        self.public(|c, io| c.connect("Pilot".into(), io));
        self.client.poll(self).unwrap();
    }
    fn advance(&self, seconds: i64) {
        self.now.set(self.now.get() + seconds * SECOND);
    }
    fn count(&self, method: &str, path: &str) -> usize {
        self.requests
            .borrow()
            .iter()
            .filter(|r| r.method == method && r.path == path)
            .count()
    }
}
impl Io for Env {
    fn lock(&self) -> Result<()> {
        self.reenter();
        assert!(!self.held.replace(true), "recursive native lock");
        Ok(())
    }
    fn unlock(&self) -> Result<()> {
        self.reenter();
        assert!(self.held.replace(false), "native unlock without ownership");
        Ok(())
    }
    fn now(&self) -> i64 {
        self.reenter();
        self.now.get()
    }
    fn identity(&self) -> Result<Option<Identity>> {
        self.reenter();
        Ok(self.identity.borrow().clone())
    }
    fn request(
        &self,
        method: &str,
        url: &str,
        body: &str,
        bearer: &str,
        cleanup: bool,
    ) -> Result<Reply> {
        self.reenter();
        assert!(!self.held.get(), "native mutex held over HTTP");
        let r = Request {
            method: method.into(),
            path: url
                .strip_prefix("https://account.example")
                .expect("pinned origin")
                .into(),
            body: body.into(),
            bearer: bearer.into(),
            cleanup,
        };
        self.requests.borrow_mut().push(r.clone());
        if let Some(reply) = self.hook.borrow().as_ref().and_then(|h| h(self, &r)) {
            return reply;
        }
        let body = match (method, r.path.as_str()) {
            ("POST", "/v1/account/session") => {
                assert_eq!(bearer, "PRIVATE-OAUTH");
                json!({"token":GRANT,"expires_in":900})
            }
            ("DELETE", "/v1/account/session") => {
                assert!(cleanup);
                json!({})
            }
            ("GET", "/v1/clans?presence=1") => json!({"clan":null,"invitations":[]}),
            ("GET", "/v1/friends?party=1") => json!({"friend_tag":"Pilot#12345678","friends":[]}),
            ("GET", "/v1/party?local_players=1&appearance=1&postgame=1&countdown=1&privacy=1")
            | ("POST", "/v1/party/commands?countdown=1&privacy=1") => self.party.borrow().clone(),
            ("GET", "/v1/party/countdown") => {
                json!({"version":self.party.borrow()["party"]["version"],"countdown":self.party.borrow()["party"]["countdown"]})
            }
            ("PUT" | "GET", "/v1/account/profile") => json!({}),
            ("POST", "/v1/account/eos-token") => json!({"token":"PRIVATE-EOS-TOKEN"}),
            ("POST", "/v1/account/eos-proof") => json!({}),
            ("GET", "/v1/account/steam-friends") => json!({"state":"ready","friends":[]}),
            ("POST", "/v1/party/activity") => {
                json!({"party_id":HANDLE,"join_secret":format!("h2p1_{}","s".repeat(43)),
                "version":self.party.borrow()["party"]["version"],"current_size":self.party.borrow()["party"]["members"].as_array().unwrap().len(),"max_size":16,"expires_in":600})
            }
            _ => panic!("unhandled request {method} {}", r.path),
        };
        if !(method == "POST" && r.path == "/v1/account/session") {
            assert_eq!(bearer, GRANT);
        }
        Ok(Reply {
            status: 200,
            body: body.to_string(),
        })
    }
    fn cancelled(&self) -> bool {
        self.reenter();
        self.cancelled.get()
    }
    fn reset_cancellation(&self) -> Result<()> {
        self.reenter();
        assert!(self.held.get());
        self.cancelled.set(false);
        Ok(())
    }
    fn cancel(&self) {
        self.reenter();
        self.cancelled.set(true);
    }
    fn wake(&self) {
        self.reenter();
    }
    fn entropy(&self) -> Result<[u8; 16]> {
        self.reenter();
        let n = self.entropy.get();
        self.entropy.set(n.wrapping_add(1));
        Ok([n; 16])
    }
    fn eos_profiles(&self) -> Result<bool> {
        self.reenter();
        assert!(!self.held.get());
        Ok(self.eos.get())
    }
    fn eos_identity(&self, token: &str) -> Result<(String, String)> {
        self.reenter();
        assert!(!self.held.get());
        assert_eq!(token, "PRIVATE-EOS-TOKEN");
        Ok(("PRIVATE-PUID".into(), "PRIVATE-EOS-PROOF".into()))
    }
}

#[test]
fn accepted_state_excludes_credentials_and_uses_native_mutex_only_for_capture_and_publish() {
    let e = Env::new();
    e.connect();
    let s = e.client.snapshot();
    assert_eq!(s["authenticated"], true);
    assert!(!e.held.get());
    for secret in [GRANT, SUBJECT, "PRIVATE-OAUTH"] {
        assert!(!s.to_string().contains(secret));
    }
    let v = e.public(|c, io| c.voice(true, io).unwrap()).unwrap();
    assert_eq!(v.bearer, GRANT);
    assert_eq!(v.player, HANDLE);
    assert!(e.public(|c, io| c.voice(false, io).unwrap()).is_none());
}

#[test]
fn late_complete_grant_after_signout_is_retired_without_publishing() {
    let e = Env::new();
    e.hook.replace(Some(Box::new(|e, r| {
        if r.method == "POST" && r.path == "/v1/account/session" {
            e.public(|c, io| c.sign_out(io));
            Some(Ok(Reply {
                status: 201,
                body: json!({"token":GRANT,"expires_in":900}).to_string(),
            }))
        } else {
            None
        }
    })));
    e.connect();
    assert_eq!(e.client.snapshot()["message"], "Signed out.");
    e.client.poll(&e).unwrap();
    assert_eq!(
        e.count("DELETE", "/v1/account/session"),
        1,
        "complete late grant was orphaned"
    );
    assert_eq!(e.client.snapshot()["authenticated"], false);
}

#[test]
fn rejected_session_lease_is_never_used_and_owned_grant_is_retired() {
    let e = Env::new();
    e.connect();
    e.requests.borrow_mut().clear();
    e.hook.replace(Some(Box::new(|_, r| {
        if r.method == "POST" && r.path == "/v1/account/session" {
            Some(Ok(Reply {
                status: 201,
                body: json!({"token":GRANT,"expires_in":0}).to_string(),
            }))
        } else {
            None
        }
    })));
    e.public(|c, io| c.connect("Renamed".into(), io));
    e.client.poll(&e).unwrap();
    e.client.poll(&e).unwrap();
    assert_eq!(
        e.count("POST", "/v1/account/session"),
        2,
        "rejected grant was cached for reuse"
    );
    assert_eq!(
        e.count("DELETE", "/v1/account/session"),
        3,
        "rejected owned grants were not retired"
    );
    assert_eq!(
        e.count("GET", "/v1/clans?presence=1"),
        0,
        "malformed grant authorized social reads"
    );
}

#[test]
fn countdown_fast_lane_uses_accepted_version_and_keeps_full_poll_cadence() {
    let e = Env::new();
    e.connect();
    assert_eq!(e.client.cadence_ms(), 500);
    let cd = json!({"serial":1,"seconds":10,"mode":0,"complete":false,"requester":null});
    e.party.borrow_mut()["party"]["countdown"] = cd;
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.cadence_ms(), 100);
    assert_eq!(e.count("GET", "/v1/clans?presence=1"), 1);
    assert_eq!(e.client.snapshot()["party"]["countdown"]["seconds"], 10);
    e.party.borrow_mut()["party"]["version"] = json!(8);
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.snapshot()["party"]["version"], 7);
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.snapshot()["party"]["version"], 8);
    assert_eq!(e.count("GET", "/v1/clans?presence=1"), 2);
    e.advance(2);
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("GET", "/v1/clans?presence=1"), 3);
}

#[test]
fn uncertain_party_reply_replays_same_request_once_but_http_refusal_does_not() {
    for status in [0, 400] {
        let e = Env::new();
        e.connect();
        let attempts = Cell::new(0);
        e.hook.replace(Some(Box::new(move |_, r| {
            if r.method != "POST" || !r.path.starts_with("/v1/party/commands") {
                return None;
            }
            let n = attempts.get();
            attempts.set(n + 1);
            if n == 0 {
                Some(if status == 0 {
                    Err(Error::Transport("lost reply".into()))
                } else {
                    Ok(Reply {
                        status,
                        body: json!({"error":"version rejected"}).to_string(),
                    })
                })
            } else {
                None
            }
        })));
        assert_eq!(
            e.public(|c, io| c.party_command(
                PartyCommand {
                    action: "countdown_start".into(),
                    playlist: "team-slayer".into(),
                    revision: "catalog-pin".into(),
                    ..Default::default()
                },
                0,
                io
            ))
            .unwrap(),
            1
        );
        e.client.poll(&e).unwrap();
        let requests = e.requests.borrow();
        let rows: Vec<_> = requests
            .iter()
            .filter(|r| r.method == "POST" && r.path.starts_with("/v1/party/commands"))
            .collect();
        assert_eq!(rows.len(), if status == 0 { 2 } else { 1 });
        if rows.len() == 2 {
            assert_eq!(rows[0].body, rows[1].body);
        }
        let body: Value = serde_json::from_str(&rows[0].body).unwrap();
        assert_eq!(body["version"], 7);
        assert_eq!(body["request_id"], "00000000-0000-4000-8000-000000000000");
        assert_eq!(e.client.snapshot()["party_command_successful"], status == 0);
    }
}

#[test]
fn malformed_command_response_retries_without_partial_party_state() {
    let e = Env::new();
    e.connect();
    e.hook.replace(Some(Box::new(|_, r| {
        if r.method == "POST" && r.path.starts_with("/v1/party/commands") {
            Some(Ok(Reply {
                status: 200,
                body: json!({"party":null,"invitations":[{"id":"partial","name":"Partial"}]})
                    .to_string(),
            }))
        } else {
            None
        }
    })));
    e.public(|c, io| {
        c.party_command(
            PartyCommand {
                action: "leave".into(),
                ..Default::default()
            },
            0,
            io,
        )
    })
    .unwrap();
    e.client.poll(&e).unwrap();
    assert_eq!(
        e.count("POST", "/v1/party/commands?countdown=1&privacy=1"),
        2
    );
    let s = e.client.snapshot();
    assert_eq!(s["party"]["version"], 7);
    assert_eq!(s["party_invitations"], json!([]));
    assert_eq!(s["party_command_successful"], false);
}

#[test]
fn accepted_party_ack_survives_later_social_failure() {
    let e = Env::new();
    e.connect();
    e.hook.replace(Some(Box::new(|_, r| {
        if r.path == "/v1/clans?presence=1" {
            Some(Err(Error::Transport("social temporarily failed".into())))
        } else {
            None
        }
    })));
    e.public(|c, io| {
        c.party_command(
            PartyCommand {
                action: "countdown_cancel".into(),
                ..Default::default()
            },
            0,
            io,
        )
    })
    .unwrap();
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.snapshot()["party_command_successful"], true);
    assert_eq!(
        e.count("POST", "/v1/party/commands?countdown=1&privacy=1"),
        1
    );
}

#[test]
fn changed_identity_cannot_issue_queued_mutation_or_obtain_old_voice() {
    let e = Env::new();
    e.connect();
    e.public(|c, io| {
        c.party_command(
            PartyCommand {
                action: "leave".into(),
                ..Default::default()
            },
            0,
            io,
        )
    })
    .unwrap();
    e.identity.borrow_mut().as_mut().unwrap().subject = "OTHER-SUBJECT".into();
    assert!(e.public(|c, io| c.voice(true, io)).unwrap().is_none());
    e.client.poll(&e).unwrap();
    assert_eq!(
        e.count("POST", "/v1/party/commands?countdown=1&privacy=1"),
        0
    );
    assert_eq!(e.count("DELETE", "/v1/account/session"), 1);
    assert_eq!(e.client.snapshot()["authenticated"], false);
}

#[test]
fn identity_changed_during_http_cannot_publish_authenticated_state() {
    let e = Env::new();
    e.hook.replace(Some(Box::new(|e, r| {
        if r.path.starts_with("/v1/party?local_players") {
            e.identity.borrow_mut().as_mut().unwrap().subject = "OTHER-SUBJECT".into();
        }
        None
    })));
    e.connect();
    assert_eq!(e.client.snapshot()["authenticated"], false);
    assert_eq!(
        e.client.snapshot()["message"],
        "Account identity changed. Reconnecting ONLINE..."
    );
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("DELETE", "/v1/account/session"), 1);
    assert_eq!(e.client.snapshot()["authenticated"], true);
}

#[test]
fn invalid_party_metadata_keeps_original_entropy_call_order() {
    let e = Env::new();
    e.connect();
    let bad = PartyCommand {
        action: "leave".into(),
        privacy: Some(0),
        ..Default::default()
    };
    assert_eq!(e.public(|c, io| c.party_command(bad, 0, io)).unwrap(), 0);
    assert_eq!(e.entropy.get(), 1);
    e.public(|c, io| {
        c.party_command(
            PartyCommand {
                action: "leave".into(),
                ..Default::default()
            },
            0,
            io,
        )
    })
    .unwrap();
    e.client.poll(&e).unwrap();
    let requests = e.requests.borrow();
    let request = requests
        .iter()
        .find(|r| r.path.starts_with("/v1/party/commands"))
        .unwrap();
    assert_eq!(
        serde_json::from_str::<Value>(&request.body).unwrap()["request_id"],
        "01010101-0101-4101-8101-010101010101"
    );
}

#[test]
fn failed_profile_retries_after_sixty_seconds_or_new_serial() {
    let e = Env::new();
    e.hook.replace(Some(Box::new(|_, r| {
        if r.path == "/v1/account/profile" {
            Some(Err(Error::Transport("profile refused".into())))
        } else {
            None
        }
    })));
    e.public(|c, io| c.profile([0; 9], "A117", io));
    e.connect();
    assert_eq!(e.count("PUT", "/v1/account/profile"), 1);
    e.advance(59);
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("PUT", "/v1/account/profile"), 1);
    e.advance(1);
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("PUT", "/v1/account/profile"), 2);
    e.public(|c, io| c.profile([1; 9], "B117", io));
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("PUT", "/v1/account/profile"), 3);
    e.public(|c, io| c.profile([2; 9], "bad!", io));
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("PUT", "/v1/account/profile"), 3);
    assert_eq!(e.client.snapshot()["authenticated"], true);
}

#[test]
fn eos_proof_and_profile_only_worker_do_not_admit_social_commands() {
    let e = Env::configured(false, true);
    e.eos.set(true);
    e.connect();
    assert_eq!(e.count("POST", "/v1/account/eos-token"), 1);
    assert_eq!(e.count("POST", "/v1/account/eos-proof"), 1);
    assert_eq!(e.count("GET", "/v1/account/profile"), 1);
    assert_eq!(e.count("GET", "/v1/clans?presence=1"), 0);
    assert_eq!(
        e.public(|c, io| c.clan_command("create", "Clan", 0, false, io))
            .unwrap(),
        false
    );
    assert_eq!(
        e.public(|c, io| c.party_command(
            PartyCommand {
                action: "create".into(),
                ..Default::default()
            },
            0,
            io
        ))
        .unwrap(),
        0
    );
    assert_eq!(
        e.public(|c, io| c.friend_command("search", "Pilot#12345678", io))
            .unwrap(),
        false
    );
    assert!(e.public(|c, io| c.voice(true, io)).unwrap().is_none());
    let s = e.client.snapshot().to_string();
    for secret in ["PRIVATE-PUID", "PRIVATE-EOS-PROOF", "PRIVATE-EOS-TOKEN"] {
        assert!(!s.contains(secret));
    }
}

#[test]
fn strict_network_json_rejects_duplicates_depth_and_response_budget() {
    for malformed in [
        "{\"clan\":null,\"clan\":null,\"invitations\":[]}".to_string(),
        format!("{}0{}", "[".repeat(18), "]".repeat(18)),
        " ".repeat(16385),
    ] {
        let e = Env::new();
        e.hook.replace(Some(Box::new(move |_, r| {
            if r.path == "/v1/clans?presence=1" {
                Some(Ok(Reply {
                    status: 200,
                    body: malformed.clone(),
                }))
            } else {
                None
            }
        })));
        e.connect();
        let s = e.client.snapshot();
        assert_eq!(s["authenticated"], false);
        assert_eq!(s["clan"], Value::Null);
        assert_eq!(s["message"], "Invalid ONLINE response");
    }
}

#[test]
fn signout_during_social_response_fences_publication_and_retires_exact_grant() {
    let e = Env::new();
    e.hook.replace(Some(Box::new(|e, r| {
        if r.path == "/v1/clans?presence=1" {
            e.public(|c, io| c.sign_out(io));
        }
        None
    })));
    e.connect();
    e.client.poll(&e).unwrap();
    let s = e.client.snapshot();
    assert_eq!(s["message"], "Signed out.");
    assert_eq!(s["authenticated"], false);
    let requests = e.requests.borrow();
    let deletes: Vec<_> = requests.iter().filter(|r| r.method == "DELETE").collect();
    assert_eq!(deletes.len(), 1);
    assert_eq!(deletes[0].bearer, GRANT);
    assert!(deletes[0].cleanup);
}

#[test]
fn voice_expiry_and_steam_fetch_are_bound_to_the_account() {
    let e = Env::configured(true, false);
    e.connect();
    e.public(|c, io| c.refresh_steam(io));
    e.client.poll(&e).unwrap();
    assert_eq!(e.count("GET", "/v1/account/steam-friends"), 1);
    assert_eq!(e.client.snapshot()["steam_friends_state"], "ready");
    e.advance(899);
    assert!(e.public(|c, io| c.voice(true, io)).unwrap().is_none());
    e.client.poll(&e).unwrap();
    assert!(e.public(|c, io| c.voice(true, io)).unwrap().is_some());
}

#[test]
fn follower_and_postgame_return_cover_all_optional_flags() {
    for leader in [false, true] {
        for own in [None, Some(false), Some(true)] {
            for host in [None, Some(false), Some(true)] {
                let p = json!({"leader":leader,"members":[{"self":true,"leader":false,"postgame":own},{"self":false,"leader":true,"postgame":host}]});
                assert_eq!(party_follower(&p), !leader);
                assert_eq!(
                    leader_left_postgame(&p),
                    !leader && own == Some(true) && host == Some(false)
                );
            }
        }
    }
    assert!(!party_follower(&json!({"leader":false,"members":[]})));
}

#[test]
fn activity_links_keep_original_expiry_and_revoke_on_structural_commands() {
    let e = Env::new();
    e.connect();
    let serial = e
        .public(|c, io| {
            c.party_command(
                PartyCommand {
                    action: "invite_discord".into(),
                    value: "1234567890".into(),
                    ..Default::default()
                },
                1,
                io,
            )
        })
        .unwrap();
    assert_eq!(serial, 1);
    assert!(
        e.public(|c, io| c.discord_operation(serial, io))
            .unwrap()
            .pending
    );
    e.client.poll(&e).unwrap();
    let issued = e.public(|c, io| c.discord_operation(serial, io)).unwrap();
    assert!(issued.successful);
    let link = issued.link.unwrap();
    assert_eq!(link.expires, 700 * SECOND);
    for secret in [&link.join_secret, GRANT, SUBJECT] {
        assert!(!e.client.snapshot().to_string().contains(secret));
    }
    e.party.borrow_mut()["party"]["version"] = json!(8);
    e.party.borrow_mut()["party"]["members"].as_array_mut().unwrap().push(json!({"handle":"peer","name":"Peer","ready_playlist":"","self":false,"leader":false,"online":true}));
    e.advance(2);
    e.client.poll(&e).unwrap();
    let refreshed = e.public(|c, io| c.discord_link(io)).unwrap().unwrap();
    assert_eq!(refreshed.version, 8);
    assert_eq!(refreshed.current_size, 2);
    assert_eq!(refreshed.expires, link.expires);
    assert_eq!(
        e.public(|c, io| c.discord_operation(serial, io))
            .unwrap()
            .link
            .unwrap()
            .expires,
        link.expires
    );
    e.public(|c, io| {
        c.party_command(
            PartyCommand {
                action: "ready".into(),
                playlist: "team-slayer".into(),
                revision: "catalog-pin".into(),
                ..Default::default()
            },
            0,
            io,
        )
    })
    .unwrap();
    assert!(e.public(|c, io| c.discord_link(io)).unwrap().is_none());
}

#[test]
fn activity_queries_reject_changed_subject_and_expired_grant() {
    for changed in [false, true] {
        let e = Env::new();
        e.connect();
        let serial = e
            .public(|c, io| {
                c.party_command(
                    PartyCommand {
                        action: "invite_discord".into(),
                        value: "1234567890".into(),
                        ..Default::default()
                    },
                    1,
                    io,
                )
            })
            .unwrap();
        e.client.poll(&e).unwrap();
        if changed {
            e.identity.borrow_mut().as_mut().unwrap().subject = "OTHER-SUBJECT".into();
        } else {
            e.advance(600);
        }
        assert!(e.public(|c, io| c.discord_link(io)).unwrap().is_none());
        let op = e.public(|c, io| c.discord_operation(serial, io)).unwrap();
        assert!(!op.successful);
        assert!(op.link.is_none());
    }
}

#[test]
fn unauthorized_session_reply_removes_private_authority() {
    let e = Env::new();
    e.connect();
    e.hook.replace(Some(Box::new(|_, r| {
        if r.path == "/v1/party/countdown" {
            Some(Ok(Reply {
                status: 401,
                body: json!({"error":"expired"}).to_string(),
            }))
        } else {
            None
        }
    })));
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.snapshot()["authenticated"], false);
    assert!(e.public(|c, io| c.voice(true, io)).unwrap().is_none());
    e.hook.replace(None);
    e.client.poll(&e).unwrap();
    assert_eq!(e.client.snapshot()["authenticated"], true);
    assert_eq!(e.count("POST", "/v1/account/session"), 2);
}
