//! Modern Steam OpenID/device authorization. Rust owns every pending state,
//! validation and publication; provider/HTTP/SQLite IO cannot invent an owner.
use crate::database::Clock;
use base64::{Engine as _, engine::general_purpose::URL_SAFE_NO_PAD};
use halo2_online_protocol::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet};

#[path = "steam_provider.rs"]
pub mod provider;
pub const OP: &str = "https://steamcommunity.com/openid/login";
pub const NS: &str = "http://specs.openid.net/auth/2.0";
pub const SESSION_LIFETIME: i64 = 28800;
pub const REFRESH_LIFETIME: i64 = 30 * 86400;

pub struct Reply {
    pub status: u16,
    pub body: String,
    pub content_type: &'static str,
    pub headers: Vec<(&'static str, String)>,
}
impl Reply {
    pub fn json(value: Value) -> Self {
        Self {
            status: 200,
            body: value.to_string(),
            content_type: "application/json",
            headers: Vec::new(),
        }
    }
    fn redirect(path: String) -> Self {
        Self {
            status: 303,
            body: String::new(),
            content_type: "text/plain; charset=utf-8",
            headers: vec![
                ("location", path),
                ("referrer-policy", "no-referrer".into()),
            ],
        }
    }
    pub fn complete() -> Self {
        Self {
            status: 200,
            body: "Steam sign-in response received. Return to Halo 2 to check your sign-in status."
                .into(),
            content_type: "text/plain; charset=utf-8",
            headers: vec![
                ("referrer-policy", "no-referrer".into()),
                ("x-content-type-options", "nosniff".into()),
                ("content-security-policy", "default-src 'none'".into()),
            ],
        }
    }
}
#[derive(Clone)]
pub struct Owner {
    subject: String,
    name: String,
}
impl Owner {
    pub(crate) fn stored(subject: String, name: String) -> Result<Self> {
        valid_subject(&subject)?;
        m::text(&json!(name), 128)?;
        Ok(Self { subject, name })
    }
    pub fn subject(&self) -> &str {
        &self.subject
    }
    pub fn name(&self) -> &str {
        &self.name
    }
    pub fn signed_in(&self, token: String, refresh: Option<String>) -> Reply {
        let mut value = json!({"state":"signed_in","token":token,"expires_in":SESSION_LIFETIME,
            "identity":{"id":self.subject,"username":self.name,"display_name":self.name}});
        if let Some(refresh) = refresh {
            value["refresh_token"] = json!(refresh);
        }
        Reply::json(value)
    }
}
#[derive(Clone, Copy, PartialEq, Eq)]
enum Stage {
    Pending,
    Verifying,
    AwaitingConfirmation,
    Complete,
    Declined,
    Failed,
}
impl Stage {
    fn text(self) -> &'static str {
        match self {
            Self::Pending => "pending",
            Self::Verifying => "verifying",
            Self::AwaitingConfirmation => "awaiting_confirmation",
            Self::Complete => "complete",
            Self::Declined => "declined",
            Self::Failed => "failed",
        }
    }
}
#[derive(Clone)]
struct Pending {
    generation: u64,
    challenge: String,
    confirmation: String,
    expires: f64,
    stage: Stage,
    source: String,
    prefix: String,
    owner: Option<Owner>,
    cookie: String,
    attempts: u8,
}
#[derive(Default)]
pub struct Broker {
    issuer: String,
    next: u64,
    pending: BTreeMap<String, Pending>,
}
#[derive(Clone)]
pub struct Plan {
    pub(crate) state: String,
    pub(crate) generation: u64,
    pub(crate) subject: String,
    pub(crate) nonce: String,
    pub(crate) values: Vec<(String, String)>,
    issuer: String,
}
pub enum Callback {
    Redirect(Reply),
    Verify(Plan),
}
#[derive(Clone)]
pub struct Confirmation {
    state: String,
    generation: u64,
    cookie: String,
}
pub enum Poll {
    Waiting(Reply),
    Complete {
        state: String,
        generation: u64,
        owner: Owner,
    },
}

pub fn need(ok: bool, status: u16, message: &'static str) -> Result<()> {
    if ok {
        Ok(())
    } else {
        Err(Error(status, message))
    }
}
pub fn proof(value: &str, low: usize, high: usize) -> Result<()> {
    need(
        (low..=high).contains(&value.len())
            && value
                .bytes()
                .all(|c| c.is_ascii_alphanumeric() || b"_-".contains(&c)),
        400,
        "Invalid Steam login proof",
    )
}
pub fn random() -> Result<String> {
    let mut bytes = [0u8; 32];
    getrandom::getrandom(&mut bytes)
        .map_err(|_| Error(503, "Steam sign-in entropy unavailable"))?;
    Ok(URL_SAFE_NO_PAD.encode(bytes))
}
pub fn digest(value: &str) -> String {
    format!("{:x}", Sha256::digest(value.as_bytes()))
}
fn challenge(value: &str) -> String {
    URL_SAFE_NO_PAD.encode(Sha256::digest(value.as_bytes()))
}
fn same(a: &str, b: &str) -> bool {
    a.len() == b.len() && openssl::memcmp::eq(a.as_bytes(), b.as_bytes())
}
fn valid_subject(subject: &str) -> Result<()> {
    need(
        subject.len() == 17 && subject.bytes().all(|c| c.is_ascii_digit()),
        400,
        "Invalid Steam identity",
    )
}
pub fn refresh(header: &str) -> Result<String> {
    let token = header
        .strip_prefix("Bearer h2r_")
        .ok_or(Error(401, "Saved Steam sign-in rejected"))?;
    proof(token, 43, 43).map_err(|_| Error(401, "Saved Steam sign-in rejected"))?;
    Ok(digest(&header[7..]))
}
pub fn query(raw: &str) -> Result<Vec<(String, String)>> {
    let pairs = form_urlencoded::parse(raw.as_bytes())
        .map(|(k, v)| (k.into_owned(), v.into_owned()))
        .collect::<Vec<_>>();
    let unique = pairs.iter().map(|(k, _)| k).collect::<BTreeSet<_>>();
    need(
        pairs.len() <= 16
            && unique.len() == pairs.len()
            && pairs.iter().all(|(k, v)| k.len() <= 64 && v.len() <= 1024),
        400,
        "Invalid Steam response",
    )?;
    Ok(pairs)
}
fn nonce(value: &str, wall: i64) -> Result<()> {
    let bytes = value.as_bytes();
    need(
        (21..=256).contains(&bytes.len()) && bytes.iter().all(|c| (33..127).contains(c)),
        400,
        "Invalid Steam response nonce",
    )?;
    let text = std::str::from_utf8(&bytes[..20])
        .map_err(|_| Error(400, "Invalid Steam response nonce"))?;
    need(
        bytes[..20]
            .iter()
            .enumerate()
            .all(|(i, c)| [4, 7, 10, 13, 16, 19].contains(&i) || c.is_ascii_digit()),
        400,
        "Invalid Steam response nonce",
    )?;
    need(
        [4, 7, 10, 13, 16, 19]
            .into_iter()
            .zip(b"--T::Z")
            .all(|(i, c)| bytes[i] == *c),
        400,
        "Invalid Steam response nonce",
    )?;
    let number = |a: usize, b: usize| {
        text[a..b]
            .parse::<u32>()
            .map_err(|_| Error(400, "Invalid Steam response nonce"))
    };
    let time = chrono::NaiveDate::from_ymd_opt(number(0, 4)? as i32, number(5, 7)?, number(8, 10)?)
        .and_then(|d| {
            d.and_hms_opt(
                number(11, 13).ok()?,
                number(14, 16).ok()?,
                number(17, 19).ok()?,
            )
        })
        .ok_or(Error(400, "Invalid Steam response nonce"))?;
    need(
        wall.abs_diff(time.and_utc().timestamp()) <= 300,
        400,
        "Steam response expired or replayed",
    )
}
fn escaped(text: &str) -> String {
    let mut out = String::new();
    for c in text.chars() {
        match c {
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '"' => out.push_str("&quot;"),
            '\'' => out.push_str("&#x27;"),
            _ => out.push(c),
        }
    }
    out
}
impl Broker {
    pub fn configure(&mut self, issuer: &str, allow_http: bool) -> Result<()> {
        if issuer.is_empty() {
            return Ok(());
        }
        let uri: axum::http::Uri = issuer
            .parse()
            .map_err(|_| Error(400, "Invalid Steam issuer"))?;
        let local = uri
            .host()
            .and_then(|h| h.trim_matches(['[', ']']).parse::<std::net::IpAddr>().ok())
            .is_some_and(|a| a.is_loopback());
        need(
            issuer.len() <= 2048
                && uri.host().is_some_and(|h| !h.is_empty())
                && (uri.scheme_str() == Some("https")
                    || (allow_http && local && uri.scheme_str() == Some("http")))
                && uri.authority().is_none_or(|a| !a.as_str().contains('@'))
                && uri.query().is_none()
                && !issuer.contains('#')
                && matches!(uri.path(), "" | "/"),
            400,
            "Steam issuer requires HTTPS or explicit loopback development HTTP",
        )?;
        self.issuer = issuer.trim_end_matches('/').into();
        Ok(())
    }
    pub fn configured(&self) -> bool {
        !self.issuer.is_empty()
    }
    pub fn start(
        &mut self,
        body: &Value,
        source: String,
        prefix: String,
        now: &Clock,
        admit: impl FnOnce() -> Result<()>,
    ) -> Result<Reply> {
        need(
            self.configured(),
            503,
            "Steam sign-in issuer is not configured",
        )?;
        m::fields(body, &["challenge", "confirmation"])?;
        let wanted = m::text(&body["challenge"], 43)?;
        proof(wanted, 43, 43)?;
        let confirmation = m::text(&body["confirmation"], 43)?;
        proof(confirmation, 43, 43)?;
        self.pending.retain(|_, p| p.expires > now.monotonic);
        let anonymous = self
            .pending
            .values()
            .filter(|p| p.owner.is_none())
            .collect::<Vec<_>>();
        need(
            anonymous.iter().filter(|p| p.source == source).count() < 4
                && anonymous.iter().filter(|p| p.prefix == prefix).count() < 8,
            429,
            "Too many pending Steam sign-ins from this source",
        )?;
        need(
            anonymous.len() < 128 && self.pending.len() < 1000,
            503,
            "Steam sign-in capacity reached",
        )?;
        let state = random()?;
        need(
            !self.pending.contains_key(&state),
            503,
            "Steam sign-in identity collision",
        )?;
        let generation = self
            .next
            .checked_add(1)
            .ok_or(Error(503, "Steam sign-in generation exhausted"))?;
        admit()?; // Rejected occupancy never consumes the shared start budget.
        let url = format!(
            "{OP}?{}",
            form_urlencoded::Serializer::new(String::new())
                .extend_pairs([
                    ("openid.ns", NS),
                    ("openid.mode", "checkid_setup"),
                    ("openid.realm", &format!("{}/", self.issuer)),
                    (
                        "openid.return_to",
                        &format!("{}/v1/auth/steam/callback/{state}", self.issuer)
                    ),
                    ("openid.identity", &format!("{NS}/identifier_select")),
                    ("openid.claimed_id", &format!("{NS}/identifier_select"))
                ])
                .finish()
        );
        self.next = generation;
        self.pending.insert(
            state.clone(),
            Pending {
                generation,
                challenge: wanted.into(),
                confirmation: confirmation.into(),
                expires: now.monotonic + 300.,
                stage: Stage::Pending,
                source,
                prefix,
                owner: None,
                cookie: String::new(),
                attempts: 0,
            },
        );
        let mut reply =
            Reply::json(json!({"device":state,"url":url,"expires_in":300,"interval":1}));
        reply.status = 201;
        Ok(reply)
    }
    pub fn callback(
        &mut self,
        state: &str,
        pairs: Vec<(String, String)>,
        now: &Clock,
        seen: impl FnOnce(&str) -> Result<bool>,
    ) -> Result<Callback> {
        proof(state, 43, 43)?;
        let p = self
            .pending
            .get(state)
            .ok_or(Error(400, "Steam sign-in expired or already completed"))?;
        need(
            p.expires > now.monotonic && p.stage == Stage::Pending,
            400,
            "Steam sign-in expired or already completed",
        )?;
        let values = pairs
            .iter()
            .map(|(k, v)| (k.as_str(), v.as_str()))
            .collect::<BTreeMap<_, _>>();
        if values.get("openid.mode") == Some(&"cancel") {
            self.pending.get_mut(state).unwrap().stage = Stage::Declined;
            return Ok(Callback::Redirect(Reply::redirect(
                "/v1/auth/steam/complete".into(),
            )));
        }
        let required = [
            "openid.ns",
            "openid.mode",
            "openid.op_endpoint",
            "openid.claimed_id",
            "openid.identity",
            "openid.return_to",
            "openid.response_nonce",
            "openid.assoc_handle",
            "openid.signed",
            "openid.sig",
        ];
        need(
            required.iter().all(|k| values.contains_key(k))
                && values.keys().all(|k| k.starts_with("openid.")),
            400,
            "Invalid Steam response",
        )?;
        need(
            values["openid.ns"] == NS
                && values["openid.mode"] == "id_res"
                && values["openid.op_endpoint"] == OP
                && values["openid.return_to"]
                    == format!("{}/v1/auth/steam/callback/{state}", self.issuer)
                && values["openid.identity"] == values["openid.claimed_id"],
            400,
            "Steam response has a different destination",
        )?;
        let signed = values["openid.signed"].split(',').collect::<Vec<_>>();
        let unique = signed.iter().copied().collect::<BTreeSet<_>>();
        need(
            signed.len() == unique.len()
                && [
                    "op_endpoint",
                    "claimed_id",
                    "identity",
                    "return_to",
                    "response_nonce",
                    "assoc_handle",
                ]
                .iter()
                .all(|k| unique.contains(k)),
            400,
            "Steam response omitted a signed identity field",
        )?;
        let subject = values["openid.claimed_id"]
            .strip_prefix("https://steamcommunity.com/openid/id/")
            .or_else(|| {
                values["openid.claimed_id"].strip_prefix("http://steamcommunity.com/openid/id/")
            })
            .ok_or(Error(400, "Invalid Steam identity"))?;
        valid_subject(subject)?;
        nonce(values["openid.response_nonce"], now.wall)?;
        need(
            !seen(values["openid.response_nonce"])?,
            400,
            "Steam response expired or replayed",
        )?;
        let plan = Plan {
            state: state.into(),
            generation: p.generation,
            subject: subject.into(),
            nonce: values["openid.response_nonce"].into(),
            values: pairs,
            issuer: self.issuer.clone(),
        };
        self.pending.get_mut(state).unwrap().stage = Stage::Verifying;
        Ok(Callback::Verify(plan))
    }
    pub fn verify_live(&self, plan: &Plan, owner: &Owner, now: &Clock) -> Result<()> {
        let p = self
            .pending
            .get(&plan.state)
            .ok_or(Error(400, "Steam sign-in expired"))?;
        need(
            p.generation == plan.generation
                && p.stage == Stage::Verifying
                && p.expires > now.monotonic
                && self.issuer == plan.issuer,
            400,
            "Steam sign-in expired",
        )?;
        need(
            owner.subject() == plan.subject,
            403,
            "Steam returned a different profile",
        )?;
        nonce(&plan.nonce, now.wall)
    }
    pub fn publish_verified(&mut self, plan: &Plan, owner: Owner, cookie: String) -> Reply {
        let p = self
            .pending
            .get_mut(&plan.state)
            .expect("Verified pending publication remains under the runtime lock");
        p.owner = Some(owner);
        p.stage = Stage::AwaitingConfirmation;
        p.cookie = cookie.clone();
        let path = format!("/v1/auth/steam/confirm/{}", plan.state);
        let mut reply = Reply::redirect(path.clone());
        reply.headers.push((
            "set-cookie",
            format!(
                "steam-confirm={cookie}; Path={path}; Max-Age=300; HttpOnly; SameSite=Strict{}",
                if self.issuer.starts_with("https://") {
                    "; Secure"
                } else {
                    ""
                }
            ),
        ));
        reply
    }
    pub fn failed(&mut self, plan: &Plan) {
        if let Some(p) = self.pending.get_mut(&plan.state) {
            if p.generation == plan.generation && p.stage == Stage::Verifying {
                p.stage = Stage::Failed;
                p.cookie.clear();
            }
        }
    }
    pub fn confirm(&self, state: &str, cookie: &str, now: &Clock) -> Result<Confirmation> {
        let p = self
            .pending
            .get(state)
            .ok_or(Error(401, "Steam confirmation expired"))?;
        need(
            p.expires > now.monotonic
                && p.stage == Stage::AwaitingConfirmation
                && !p.cookie.is_empty()
                && same(cookie, &p.cookie),
            401,
            "Steam confirmation expired",
        )?;
        Ok(Confirmation {
            state: state.into(),
            generation: p.generation,
            cookie: cookie.into(),
        })
    }
    pub fn origin(&self, origin: &str) -> Result<()> {
        need(origin == self.issuer, 403, "Different confirmation origin")
    }
    pub fn page(&self, confirmation: &Confirmation) -> Reply {
        let p = &self.pending[&confirmation.state];
        let name = escaped(p.owner.as_ref().unwrap().name());
        Reply {
            status: 200,
            body: format!(
                "<!doctype html><meta charset=\"utf-8\"><title>Confirm Halo sign-in</title><h1>Sign in as {name}</h1><p>Enter the confirmation code shown in your Halo 2 game. Only continue if you started sign-in in that game. Never use a code sent by someone else.</p><form method=\"post\"><label>Game confirmation code <input name=\"code\" maxlength=\"12\" required autocomplete=\"off\"></label><button>Authorize this game</button></form>"
            ),
            content_type: "text/html; charset=utf-8",
            headers: vec![
                ("referrer-policy", "no-referrer".into()),
                ("x-content-type-options", "nosniff".into()),
                (
                    "content-security-policy",
                    "default-src 'none'; form-action 'self'; frame-ancestors 'none'".into(),
                ),
            ],
        }
    }
    pub fn confirmed(&mut self, prior: &Confirmation, body: &[u8], now: &Clock) -> Result<Reply> {
        let current = self.confirm(&prior.state, &prior.cookie, now)?;
        need(
            current.generation == prior.generation,
            401,
            "Steam confirmation expired",
        )?;
        need(
            body.len() <= 1024 && body.is_ascii(),
            400,
            "Invalid confirmation",
        )?;
        let pairs = form_urlencoded::parse(body).collect::<Vec<_>>();
        need(
            pairs.len() == 1 && pairs[0].0 == "code",
            400,
            "Invalid confirmation",
        )?;
        let code = pairs[0].1.trim().to_ascii_uppercase();
        let p = self.pending.get_mut(&prior.state).unwrap();
        p.attempts += 1;
        if code.len() != 12
            || !code
                .bytes()
                .all(|c| c.is_ascii_digit() || (b'A'..=b'F').contains(&c))
            || !same(&challenge(&code), &p.confirmation)
        {
            if p.attempts >= 5 {
                p.stage = Stage::Failed;
                p.cookie.clear();
            }
            return Err(Error(401, "Game confirmation code rejected"));
        }
        p.stage = Stage::Complete;
        p.cookie.clear();
        Ok(Reply::redirect("/v1/auth/steam/complete".into()))
    }
    pub fn poll(&self, body: &Value, now: &Clock) -> Result<Poll> {
        m::fields(body, &["device", "verifier"])?;
        let state = m::text(&body["device"], 43)?;
        proof(state, 43, 43)?;
        let verifier = m::text(&body["verifier"], 128)?;
        proof(verifier, 43, 128)?;
        let p = self
            .pending
            .get(state)
            .ok_or(Error(401, "Steam sign-in expired or proof rejected"))?;
        need(
            p.expires > now.monotonic && same(&p.challenge, &challenge(verifier)),
            401,
            "Steam sign-in expired or proof rejected",
        )?;
        if p.stage != Stage::Complete {
            Ok(Poll::Waiting(Reply::json(json!({"state":p.stage.text()}))))
        } else {
            Ok(Poll::Complete {
                state: state.into(),
                generation: p.generation,
                owner: p.owner.clone().unwrap(),
            })
        }
    }
    pub fn poll_live(&self, state: &str, generation: u64, now: &Clock) -> Result<()> {
        let p = self
            .pending
            .get(state)
            .ok_or(Error(401, "Steam sign-in expired or proof rejected"))?;
        need(
            p.generation == generation && p.stage == Stage::Complete && p.expires > now.monotonic,
            401,
            "Steam sign-in expired or proof rejected",
        )
    }
    pub fn publish_poll(&mut self, state: &str) {
        self.pending.remove(state);
    }
}
