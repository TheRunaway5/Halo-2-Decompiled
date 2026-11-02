//! Complete modern Steam friend HTTP request and invitation-proof coordinator.
//! The host executes bounded SQL/network/presence IO; it never selects policy.
use crate::matchmaking_model::{self as m, Error, Result};
use crate::matchmaking_steam_cache::{clean, credential_context, steam_id};
use crate::matchmaking_unicode::casefold;
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet};

const MAX_HTTP: usize = 1024 * 1024;

/// Native account IO must admit the complete, escaped thousand-friend reply.
/// Other account operations retain their existing response bound.
pub fn account_http_response_limit(method: &str, url: &str) -> usize {
    if method == "GET" && url.ends_with("/v1/account/steam-friends") {
        MAX_HTTP
    } else {
        256 * 1024
    }
}

/// The native caller supplies a URL built from its admitted account origin.
/// Match exact routes so long Steam IO never widens ordinary account calls.
/// The service owns a75s Steam operation inside a95s absolute HTTP bound.
pub fn account_http_timeout_ms(method: &str, url: &str, body: &str) -> u32 {
    const ORDINARY: u32 = 3_000;
    const STEAM: u32 = 95_000;
    if url.contains('#') {
        return ORDINARY;
    }
    let Some((scheme, authority_path)) = url.split_once("://") else {
        return ORDINARY;
    };
    if !matches!(scheme, "https" | "http") {
        return ORDINARY;
    }
    let Some(slash) = authority_path.find('/') else {
        return ORDINARY;
    };
    if slash == 0 {
        return ORDINARY;
    }
    let path = &authority_path[slash..];
    if method == "GET" && path == "/v1/account/steam-friends" {
        return STEAM;
    }
    let path = path.split_once('?').map_or(path, |(path, _)| path);
    if method == "POST" && path == "/v1/party/commands" {
        if let Ok(value) = crate::matchmaking_json::parse(body.as_bytes(), 16_384) {
            if value.get("action").and_then(Value::as_str) == Some("invite_steam") {
                return STEAM;
            }
        }
    }
    ORDINARY
}
#[derive(Clone, Copy)]
enum Phase {
    Start,
    IdentityStart,
    Acquire,
    Wait,
    Friends,
    Summary,
    IdentityHttp,
    Presence,
    Projection,
    IdentityFinish,
    Publish,
    Done,
}
pub struct Request {
    cache: u64,
    accounts: u64,
    key: String,
    player: String,
    mode: String,
    body: Value,
    context: String,
    now: f64,
    deadline: f64,
    started: bool,
    frame: u64,
    pending: Option<Value>,
    phase: Phase,
    subject: String,
    epoch: u64,
    expires: f64,
    selected: Vec<String>,
    offset: usize,
    seen: BTreeSet<String>,
    rows: Vec<Value>,
    truncated: bool,
    result: Value,
    output: Value,
    present: BTreeSet<String>,
    http: Option<Value>,
    http_summary: bool,
}
pub fn payload(body: &Value) -> String {
    format!("{:x}", Sha256::digest(body.to_string().as_bytes()))
}
fn uuid(v: &Value) -> Result<()> {
    let s = m::text(v, 36)?;
    m::check(
        s.len() == 36
            && s.bytes().enumerate().all(|(i, c)| {
                if [8, 13, 18, 23].contains(&i) {
                    c == b'-'
                } else {
                    c.is_ascii_digit() || (b'a'..=b'f').contains(&c)
                }
            }),
        "Invalid Steam command UUID",
    )
}
fn statement(sql: String, params: Value) -> Value {
    json!({"sql":sql,"params":params})
}
fn upstream_rows<'a>(body: &'a Value, section: &str, key: &str, max: usize) -> Result<&'a [Value]> {
    if !body.is_object() {
        return Err(Error(502, "Invalid Steam response"));
    }
    let Some(section) = body.get(section) else {
        return Ok(&[]);
    };
    if !section.is_object() {
        return Err(Error(502, "Invalid Steam response"));
    }
    let Some(rows) = section.get(key) else {
        return Ok(&[]);
    };
    let rows = rows
        .as_array()
        .ok_or(Error(502, "Invalid Steam response rows"))?;
    if rows.len() > max {
        return Err(Error(502, "Steam response rows exceed bounds"));
    }
    Ok(rows)
}
impl Request {
    pub fn new(r: &Value) -> Result<Self> {
        m::fields(
            r,
            &[
                "cache_source",
                "account_source",
                "account_now",
                "key",
                "player",
                "mode",
                "body",
                "now",
                "credential",
                "transaction_owned",
            ],
        )?;
        m::real(&r["account_now"])?;
        if r["transaction_owned"] != true {
            return Err(Error(
                409,
                "Steam lookup requires no pending SQL transaction",
            ));
        }
        m::hex(&r["key"], 32)?;
        m::text(&r["player"], 64)?;
        m::check(
            r["mode"] == "view" || r["mode"] == "invite",
            "Unknown Steam request mode",
        )?;
        if r["mode"] == "invite" {
            m::fields(&r["body"], &["action", "request_id", "version", "steam_id"])?;
            m::check(
                r["body"]["action"] == "invite_steam",
                "Steam invitation command required",
            )?;
            uuid(&r["body"]["request_id"])?;
            m::integer(&r["body"]["version"], 1, i64::MAX)?;
            steam_id(&r["body"]["steam_id"])?;
        } else {
            m::check(r["body"].is_null(), "Unexpected Steam request body")?;
        }
        let now = m::real(&r["now"])?;
        m::check(
            now >= 0. && (now + 300.).is_finite(),
            "Invalid Steam request clock",
        )?;
        Ok(Self {
            cache: r["cache_source"]
                .as_u64()
                .ok_or(Error(400, "Steam cache source required"))?,
            accounts: r["account_source"]
                .as_u64()
                .ok_or(Error(400, "Steam account source required"))?,
            key: r["key"].as_str().unwrap().to_owned(),
            player: r["player"].as_str().unwrap().to_owned(),
            mode: r["mode"].as_str().unwrap().to_owned(),
            body: r["body"].clone(),
            context: credential_context(&r["credential"])?,
            now,
            deadline: now + 75.,
            started: false,
            frame: 0,
            pending: None,
            phase: Phase::Start,
            subject: String::new(),
            epoch: 0,
            expires: 0.,
            selected: Vec::new(),
            offset: 0,
            seen: BTreeSet::new(),
            rows: Vec::new(),
            truncated: false,
            result: Value::Null,
            output: Value::Null,
            present: BTreeSet::new(),
            http: None,
            http_summary: false,
        })
    }
    pub fn binding(&self) -> (u64, u64, String, String) {
        (
            self.cache,
            self.accounts,
            self.key.clone(),
            self.player.clone(),
        )
    }
    pub fn now(&self) -> f64 {
        self.now
    }
    pub fn observe(&mut self, now: f64, current_credential: &Value) -> Result<()> {
        m::check(
            now >= 0. && (now + 300.).is_finite(),
            "Invalid Steam request clock",
        )?;
        self.now = self.now.max(now);
        if self.now >= self.deadline {
            return Err(Error(503, "Steam friends lookup timed out"));
        }
        if credential_context(current_credential)? != self.context {
            return Err(Error(409, "Steam lookup configuration changed"));
        }
        Ok(())
    }
    fn emit(&mut self, phase: Phase, mut event: Value) -> Result<Value> {
        self.frame = self
            .frame
            .checked_add(1)
            .ok_or(Error(503, "Steam continuation generation exhausted"))?;
        event["frame"] = json!(self.frame);
        self.phase = phase;
        self.pending = Some(event.clone());
        Ok(event)
    }
    fn identity(&mut self, phase: Phase) -> Result<Value> {
        self.emit(phase,json!({"kind":"sql","statements":[statement("SELECT subject FROM player_identities WHERE player=? AND provider='steam'".to_owned(),json!([self.player]))]}))
    }
    fn identity_row(&self, reply: &Value, initial: bool) -> Result<String> {
        let banks = reply["rows"]
            .as_array()
            .ok_or(Error(503, "Steam SQL row banks required"))?;
        let rows = banks
            .first()
            .and_then(Value::as_array)
            .ok_or(Error(503, "Steam identity rows required"))?;
        if rows.is_empty() {
            return Err(Error(403, "Sign in with Steam to view Steam friends"));
        }
        m::check(
            rows.len() == 1 && rows[0].as_array().is_some_and(|r| r.len() == 1),
            "Steam identity uniqueness differs",
        )?;
        let subject = steam_id(&rows[0][0])?.to_owned();
        if !initial && subject != self.subject {
            return Err(Error(403, "Steam identity changed; refresh friends"));
        }
        Ok(subject)
    }
    fn acquire(&mut self) -> Result<Value> {
        self.emit(Phase::Acquire,json!({"kind":"steam_cache","request":{"op":"acquire","subject":self.subject,"context":self.context}}))
    }
    fn http(&mut self, summary: bool) -> Result<Value> {
        self.http_summary = summary;
        let (endpoint, params) = if summary {
            let ids =
                self.selected[self.offset..(self.offset + 100).min(self.selected.len())].join(",");
            ("GetPlayerSummaries/v2/", json!({"steamids":ids}))
        } else {
            (
                "GetFriendList/v1/",
                json!({"steamid":self.subject,"relationship":"friend"}),
            )
        };
        self.emit(if summary{Phase::Summary}else{Phase::Friends},json!({"kind":"http","method":"GET","url":format!("https://api.steampowered.com/ISteamUser/{endpoint}"),"params":params,"credential_context":self.context,"read_limit":MAX_HTTP,"allow_redirects":false}))
    }
    fn ready(&mut self) -> Result<Value> {
        clean(&self.result)?;
        if self.mode == "view" {
            self.emit(Phase::Presence, json!({"kind":"presence"}))
        } else {
            self.identity(Phase::IdentityFinish)
        }
    }
    fn received_http(&mut self) -> Result<Value> {
        let reply = self
            .http
            .take()
            .ok_or(Error(503, "Steam IO reply unavailable"))?;
        let status = m::integer(&reply["status"], 0, 599)?;
        if !self.http_summary && [401, 403].contains(&status) {
            self.result = json!({"state":"private","friends":[]});
            return self.ready();
        }
        if status != 200 {
            return Err(Error(502, "Steam friends are temporarily unavailable"));
        }
        if reply["encoding"] != ""
            || !reply["length"].is_null()
                && !reply["length"]
                    .as_u64()
                    .is_some_and(|n| n <= MAX_HTTP as u64)
        {
            return Err(Error(502, "Invalid Steam response size or encoding"));
        }
        let hex = reply["body"]
            .as_str()
            .ok_or(Error(502, "Steam body carrier required"))?;
        if hex.len() > MAX_HTTP * 2 || hex.len() % 2 != 0 {
            return Err(Error(502, "Steam response too large"));
        }
        let bytes = m::hex(&reply["body"], hex.len() / 2)
            .map_err(|_| Error(502, "Invalid Steam response bytes"))?;
        let body = crate::matchmaking_json::parse(&bytes, MAX_HTTP)
            .map_err(|_| Error(502, "Invalid or duplicate Steam JSON"))?;
        if !self.http_summary {
            let mut subjects = BTreeSet::new();
            for row in upstream_rows(&body, "friendslist", "friends", 10000)? {
                if row.is_object() && row["relationship"] == "friend" {
                    subjects.insert(
                        steam_id(&row["steamid"])
                            .map_err(|_| Error(502, "Invalid Steam friends response"))?
                            .to_owned(),
                    );
                }
            }
            self.selected = subjects.iter().take(1000).cloned().collect();
            self.truncated = subjects.len() > self.selected.len();
            if !self.selected.is_empty() {
                return self.http(true);
            }
        } else {
            let batch = &self.selected[self.offset..(self.offset + 100).min(self.selected.len())];
            for row in upstream_rows(&body, "response", "players", 100)? {
                let Some(id) = row["steamid"].as_str() else {
                    continue;
                };
                if !batch.iter().any(|s| s == id) || !self.seen.insert(id.to_owned()) {
                    continue;
                }
                let Some(name) = row["personaname"].as_str() else {
                    continue;
                };
                if name.is_empty()
                    || name.len() > 128
                    || name.chars().any(|c| c < ' ' || c == '\x7f')
                    || !row["personastate"]
                        .as_i64()
                        .is_some_and(|n| (1..=6).contains(&n))
                {
                    continue;
                }
                self.rows.push(json!({"id":id,"name":name}));
            }
            self.offset += batch.len();
            if self.offset < self.selected.len() {
                return self.http(true);
            }
        }
        self.result = json!({"state":"ready","friends":self.rows,"truncated":self.truncated});
        self.ready()
    }
    fn projection(&mut self) -> Result<Value> {
        let mut statements = vec![statement(
            "SELECT subject FROM player_identities WHERE player=? AND provider='steam'".to_owned(),
            json!([self.player]),
        )];
        for chunk in self.result["friends"].as_array().unwrap().chunks(128) {
            let placeholders = (2..chunk.len() + 2)
                .map(|n| format!("?{n}"))
                .collect::<Vec<_>>()
                .join(",");
            let mut params = vec![json!(self.player)];
            params.extend(chunk.iter().map(|r| r["id"].clone()));
            statements.push(statement(format!("SELECT i.subject,i.player,EXISTS(SELECT 1 FROM player_friend_blocks WHERE (player=?1 AND target=i.player) OR (player=i.player AND target=?1)) OR EXISTS(SELECT 1 FROM party_blocks WHERE (player=?1 AND inviter=i.player) OR (player=i.player AND inviter=?1)) FROM player_identities i WHERE i.provider='steam' AND i.subject IN ({placeholders}) ORDER BY i.subject LIMIT 129"),json!(params)));
        }
        self.emit(
            Phase::Projection,
            json!({"kind":"sql","statements":statements}),
        )
    }
    fn projected(&mut self, r: &Value) -> Result<()> {
        self.identity_row(r, false)?;
        let banks = r["rows"].as_array().unwrap();
        let upstream = self.result["friends"].as_array().unwrap();
        m::check(
            banks.len() == 1 + upstream.len().div_ceil(128),
            "Steam projection row banks differ",
        )?;
        let mut registered = BTreeMap::new();
        for (index, chunk) in upstream.chunks(128).enumerate() {
            for row in banks[index + 1]
                .as_array()
                .ok_or(Error(503, "Steam projection rows required"))?
            {
                m::check(
                    row.as_array().is_some_and(|r| r.len() == 3),
                    "Steam projection columns differ",
                )?;
                let id = steam_id(&row[0])?;
                let target = m::text(&row[1], 64)?;
                let blocked = m::integer(&row[2], 0, 1)? == 1;
                m::check(
                    chunk.iter().any(|r| r["id"] == id)
                        && registered
                            .insert(id.to_owned(), (target.to_owned(), blocked))
                            .is_none(),
                    "Steam projection identity differs",
                )?;
            }
        }
        let mut rows = Vec::new();
        for source in upstream {
            let found = registered.get(source["id"].as_str().unwrap());
            if found.is_some_and(|(_, blocked)| *blocked) {
                continue;
            }
            rows.push(json!({"id":source["id"],"name":source["name"],"registered":found.is_some(),"halo_online":found.is_some_and(|(target,_)|self.present.contains(target))}));
        }
        rows.sort_by_cached_key(|r| {
            (
                !r["halo_online"].as_bool().unwrap(),
                casefold(r["name"].as_str().unwrap()),
                r["id"].as_str().unwrap().to_owned(),
            )
        });
        self.output = self.result.clone();
        self.output["friends"] = json!(rows);
        Ok(())
    }
    fn publish(&mut self) -> Result<Value> {
        let invitation = if self.mode == "invite" {
            json!({"player":self.player,"key":self.key,"target":self.body["steam_id"],"payload":payload(&self.body)})
        } else {
            Value::Null
        };
        self.emit(Phase::Publish,json!({"kind":"steam_cache","request":{"op":"publish","subject":self.subject,"context":self.context,"epoch":self.epoch,"expires":self.expires,"value":self.result,"invitation":invitation}}))
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        if r["op"] == "begin" {
            m::fields(r, &["op"])?;
            m::check(!self.started, "Steam request already started")?;
            self.started = true;
            return self.identity(Phase::IdentityStart);
        }
        let pending = self
            .pending
            .as_ref()
            .ok_or(Error(409, "Steam continuation retired"))?;
        if r["op"] != "reply" || r["frame"] != pending["frame"] {
            return Err(Error(409, "Stale Steam continuation"));
        }
        let names: &[&str] = match pending["kind"].as_str().unwrap() {
            "sql" => &["op", "frame", "rows", "committed"],
            "http" => &["op", "frame", "status", "encoding", "length", "body"],
            "wait" => &["op", "frame"],
            _ => &["op", "frame", "value"],
        };
        m::fields(r, names)?;
        if pending["kind"] == "sql" {
            if r["committed"] != true {
                return Err(Error(409, "Steam SQL observation is uncommitted"));
            }
            let banks = r["rows"]
                .as_array()
                .ok_or(Error(503, "Steam SQL banks required"))?;
            m::check(
                banks.len() == pending["statements"].as_array().unwrap().len() && banks.len() <= 9,
                "Steam SQL bank count differs",
            )?;
            for bank in banks {
                m::check(
                    bank.as_array().is_some_and(|r| {
                        r.len() <= 128
                            && r.iter().all(|v| v.as_array().is_some_and(|v| v.len() <= 3))
                    }),
                    "Steam SQL result exceeds bounds",
                )?;
            }
        }
        self.pending = None;
        match self.phase {
            Phase::IdentityStart => {
                self.subject = self.identity_row(r, true)?;
                self.acquire()
            }
            Phase::Acquire => match r["value"]["state"].as_str() {
                Some("wait") => self.emit(Phase::Wait, json!({"kind":"wait","delay_ms":50})),
                Some("hit" | "active") => {
                    self.epoch = r["value"]["epoch"]
                        .as_u64()
                        .ok_or(Error(503, "Steam cache epoch required"))?;
                    self.expires = m::real(&r["value"]["expires"])?;
                    if r["value"]["state"] == "hit" {
                        self.result = r["value"]["value"].clone();
                        self.ready()
                    } else {
                        self.http(false)
                    }
                }
                _ => Err(Error(503, "Unknown native Steam cache state")),
            },
            Phase::Wait => self.acquire(),
            Phase::Friends | Phase::Summary => {
                self.http = Some(r.clone());
                self.identity(Phase::IdentityHttp)
            }
            Phase::IdentityHttp => {
                self.identity_row(r, false)?;
                self.received_http()
            }
            Phase::Presence => {
                let present = r["value"]
                    .as_object()
                    .ok_or(Error(503, "Native Steam presence required"))?;
                m::check(present.len() <= 8192, "Steam presence exceeds bounds")?;
                for (who, state) in present {
                    m::text(&json!(who), 64)?;
                    m::check(
                        state == "online" || state == "playing",
                        "Invalid Steam presence state",
                    )?;
                    self.present.insert(who.clone());
                }
                self.projection()
            }
            Phase::Projection => {
                self.projected(r)?;
                self.publish()
            }
            Phase::IdentityFinish => {
                self.identity_row(r, false)?;
                self.publish()
            }
            Phase::Publish => {
                let result = if self.mode == "invite" {
                    if r["value"]["claim"].is_null() {
                        return Err(Error(
                            403,
                            "Refresh Steam friends before inviting this player",
                        ));
                    }
                    r["value"]["claim"].clone()
                } else {
                    self.output.clone()
                };
                self.phase = Phase::Done;
                Ok(
                    json!({"kind":"done","value":result,"status":200,"headers":{"Cache-Control":"no-store"}}),
                )
            }
            _ => Err(Error(409, "Steam request retired")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn native_timeout_admits_only_exact_steam_operations() {
        let friends = "https://account.example/v1/account/steam-friends";
        let invite = r#"{"action":"invite_steam","value":"76561197960287931"}"#;
        assert_eq!(account_http_timeout_ms("GET", friends, ""), 95_000);
        assert_eq!(account_http_timeout_ms("GET", "http://127.0.0.1:1234/v1/account/steam-friends", ""), 95_000);
        assert_eq!(account_http_timeout_ms("POST", "https://account.example/v1/party/commands?countdown=1&privacy=1", invite), 95_000);
        assert_eq!(account_http_timeout_ms("POST", "https://account.example/v1/party/commands", invite), 95_000);
        for url in [format!("{friends}?other=1"), format!("{friends}#other"), format!("{friends}/"),
                    "https://account.example/other/v1/account/steam-friends".into(),
                    "https://account.example/v1/clans?presence=1".into(),
                    "/v1/account/steam-friends".into()] {
            assert_eq!(account_http_timeout_ms("GET", &url, ""), 3_000, "{url}");
        }
        assert_eq!(account_http_timeout_ms("POST", friends, invite), 3_000);
        assert_eq!(account_http_timeout_ms("GET", "https://account.example/v1/party/commands", invite), 3_000);
        assert_eq!(account_http_timeout_ms("POST", "https://account.example/v1/friends/commands", invite), 3_000);
        assert_eq!(account_http_timeout_ms("POST", "https://account.example/v1/party/commands#other", invite), 3_000);
    }
    #[test]
    fn native_timeout_does_not_admit_ambiguous_or_other_commands() {
        let url = "https://account.example/v1/party/commands?countdown=1&privacy=1";
        for body in [r#"{"action":"create"}"#, r#"{"action":"invite_friend"}"#,
                     r#"{"action":"invite_steam","action":"create"}"#,
                     r#"{"action":"create","a\u0063tion":"invite_steam"}"#,
                     r#"[{"action":"invite_steam"}]"#, r#"{"action":null}"#,
                     r#"{"action":"invite_steam"} {}"#, "not json"] {
            assert_eq!(account_http_timeout_ms("POST", url, body), 3_000, "{body}");
        }
        let oversized = json!({"action":"invite_steam","padding":"x".repeat(16_384)}).to_string();
        assert_eq!(account_http_timeout_ms("POST", url, &oversized), 3_000);
    }
    #[test]
    fn native_friend_reply_budget_does_not_widen_other_account_operations() {
        let endpoint = "https://account.example/v1/account/steam-friends";
        assert_eq!(account_http_response_limit("GET", endpoint), 1024 * 1024);
        assert_eq!(account_http_response_limit("POST", endpoint), 256 * 1024);
        assert_eq!(
            account_http_response_limit("GET", &format!("{endpoint}?other=1")),
            256 * 1024
        );
        assert_eq!(
            account_http_response_limit("GET", "https://account.example/v1/clans?presence=1"),
            256 * 1024
        );
    }
    fn config() -> Value {
        json!({"cache_source":1,"account_source":2,"account_now":0.,"key":"11".repeat(32),"player":"actor","mode":"view","body":null,"now":0.,"credential":"fixture-key","transaction_owned":true})
    }
    #[test]
    fn stale_and_uncommitted_sql_cannot_consume_a_live_continuation() {
        let mut request = Request::new(&config()).unwrap();
        let first = request.execute(&json!({"op":"begin"})).unwrap();
        let mut reply = json!({"op":"reply","frame":first["frame"],"rows":[[["76561197960287930"]]],"committed":true});
        reply["frame"] = json!(2);
        assert_eq!(request.execute(&reply).unwrap_err().0, 409);
        reply["frame"] = first["frame"].clone();
        reply["committed"] = json!(false);
        assert_eq!(request.execute(&reply).unwrap_err().0, 409);
        reply["committed"] = json!(true);
        assert_eq!(request.execute(&reply).unwrap()["kind"], "steam_cache");
        assert_eq!(request.execute(&reply).unwrap_err().0, 409);
    }
    #[test]
    fn credential_changes_deadlines_and_outer_transactions_cannot_extend_work() {
        let mut request = Request::new(&config()).unwrap();
        assert_eq!(
            request.observe(1., &json!("changed-key")).unwrap_err().0,
            409
        );
        request.observe(74., &json!("fixture-key")).unwrap();
        request.observe(1., &json!("fixture-key")).unwrap();
        assert_eq!(request.now(), 74.);
        assert_eq!(
            request.observe(75., &json!("fixture-key")).unwrap_err().0,
            503
        );
        let mut outer = config();
        outer["transaction_owned"] = json!(false);
        assert_eq!(Request::new(&outer).err().unwrap().0, 409);
        let mut empty = config();
        empty["credential"] = json!("");
        assert_eq!(Request::new(&empty).err().unwrap().0, 503);
    }
}
