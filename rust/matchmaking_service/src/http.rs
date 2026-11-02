//! Real HTTP interface; endpoint coordination and storage stay inside Rust.
use crate::{admission::Admission, authentication, runtime::Runtime};
use axum::{
    Router,
    body::to_bytes,
    extract::{Request, State},
    response::{IntoResponse, Response},
};
use halo2_online_protocol::{
    matchmaking_json,
    matchmaking_model::{Error, Result},
};
use hyper_util::{
    rt::{TokioIo, TokioTimer},
    service::TowerToHyperService,
};
use serde_json::{Value, json};
use std::{
    net::{IpAddr, SocketAddr},
    sync::{Arc, Mutex},
    time::{Duration, Instant},
};
use tokio::{net::TcpListener, sync::Semaphore};
#[path = "connection_budget.rs"]
mod connection_budget;
#[path = "eos_http.rs"]
mod eos_http;
#[path = "room_websocket.rs"]
mod room_websocket;
#[path = "social_voice_http.rs"]
mod social_voice_http;
#[path = "steam_friends_http.rs"]
mod steam_friends_http;
#[path = "steam_http.rs"]
mod steam_http;
#[path = "websocket.rs"]
mod websocket;
const VOICE_CONNECTIONS: usize = 512;

pub struct App {
    pub runtime: Mutex<Runtime>,
    pub discord_client_id: String,
    pub behind_proxy: bool,
    pub(crate) eos: Option<Arc<crate::eos_identity::Identity>>,
    steam_key: String,
    steam_providers: Arc<Semaphore>,
    budgets: Mutex<Admission>,
    started: Instant,
    requests: Arc<Semaphore>,
    search_sockets: Arc<Semaphore>,
    room_sockets: Arc<Semaphore>,
    voice_sockets: Arc<Semaphore>,
    socket_work: Arc<Semaphore>,
}
impl App {
    pub fn new(
        runtime: Runtime,
        discord_client_id: String,
        behind_proxy: bool,
        eos: Option<Arc<crate::eos_identity::Identity>>,
        steam_key: String,
    ) -> Self {
        Self {
            runtime: Mutex::new(runtime),
            discord_client_id,
            behind_proxy,
            eos,
            steam_key,
            steam_providers: Arc::new(Semaphore::new(2)),
            budgets: Mutex::new(Admission::default()),
            started: Instant::now(),
            requests: Arc::new(Semaphore::new(64)),
            search_sockets: Arc::new(Semaphore::new(1000)),
            room_sockets: Arc::new(Semaphore::new(1024)),
            voice_sockets: Arc::new(Semaphore::new(VOICE_CONNECTIONS)),
            socket_work: Arc::new(Semaphore::new(32)),
        }
    }
    fn clock(&self) -> f64 {
        self.started.elapsed().as_secs_f64()
    }
    fn lock(&self) -> Result<std::sync::MutexGuard<'_, Runtime>> {
        self.runtime
            .lock()
            .map_err(|_| Error(503, "Rust service state unavailable"))
    }
    fn identity(&self, token: &str, deadline: Instant) -> Result<authentication::Identity> {
        let identity = if token.starts_with("h2s_") {
            self.lock()?.steam(token)?
        } else {
            authentication::discord(token, &self.discord_client_id)?
        };
        if Instant::now() >= deadline {
            return Err(Error(503, "Service request expired"));
        }
        self.budgets
            .lock()
            .map_err(|_| Error(503, "Request budgets unavailable"))?
            .owner(&identity.provider, &identity.subject, self.clock())?;
        Ok(identity)
    }
}
fn response(status: u16, value: Value) -> Response {
    let text = value.to_string();
    let (status, text) = if text.len() > 2 * 1024 * 1024 {
        (
            500,
            "{\"error\":\"Response exceeds service bound\"}".to_owned(),
        )
    } else {
        (status, text)
    };
    (
        axum::http::StatusCode::from_u16(status)
            .unwrap_or(axum::http::StatusCode::INTERNAL_SERVER_ERROR),
        [
            ("content-type", "application/json"),
            ("cache-control", "no-store"),
            ("connection", "close"),
        ],
        text,
    )
        .into_response()
}
fn error(e: Error) -> Response {
    response(e.0, json!({"error":e.1}))
}
fn header(headers: &axum::http::HeaderMap, name: &str) -> Result<String> {
    if headers.get_all(name).iter().count() > 1 {
        return Err(Error(400, "Duplicate request header"));
    }
    headers
        .get(name)
        .map(|v| {
            v.to_str()
                .map(str::to_owned)
                .map_err(|_| Error(400, "Invalid request header"))
        })
        .unwrap_or(Ok(String::new()))
}
async fn handle(State(app): State<Arc<App>>, request: Request) -> Response {
    match dispatch(app, request).await {
        Ok(value) => value,
        Err(e) => error(e),
    }
}
async fn read_body(request: Request) -> Result<Value> {
    read_body_limit(request, 16384).await
}
async fn read_body_limit(request: Request, limit: usize) -> Result<Value> {
    if header(request.headers(), "content-type")?
        .split(';')
        .next()
        .unwrap_or("")
        .trim()
        != "application/json"
    {
        return Err(Error(415, "Use application/json"));
    }
    let data = tokio::time::timeout(Duration::from_secs(5), to_bytes(request.into_body(), limit))
        .await
        .map_err(|_| Error(408, "Request body timed out"))?
        .map_err(|_| Error(413, "Request too large"))?;
    matchmaking_json::parse(&data, limit)
}
async fn dispatch(app: Arc<App>, request: Request) -> Result<Response> {
    let _lease = app
        .requests
        .clone()
        .try_acquire_owned()
        .map_err(|_| Error(503, "Service request capacity reached"))?;
    let method = request.method().as_str().to_owned();
    let path = request.uri().path().to_owned();
    let uri_limit = if path.starts_with("/v1/auth/steam/callback/") {
        8192
    } else {
        1024
    };
    if request.uri().to_string().len() > uri_limit {
        return Err(Error(414, "Request path too long"));
    }
    if !header(request.headers(), "content-encoding")?.is_empty() {
        return Err(Error(415, "Compressed bodies are not accepted"));
    }
    let peer = request
        .extensions()
        .get::<SocketAddr>()
        .ok_or(Error(503, "Request source unavailable"))?
        .ip();
    let source = if app.behind_proxy {
        let forwarded = header(request.headers(), "x-forwarded-for")?;
        if forwarded.is_empty() {
            peer
        } else {
            forwarded
                .rsplit(',')
                .next()
                .unwrap()
                .trim()
                .parse::<IpAddr>()
                .map_err(|_| Error(400, "Invalid forwarded source"))?
        }
    } else {
        peer
    };
    app.budgets
        .lock()
        .map_err(|_| Error(503, "Request budgets unavailable"))?
        .request(source, app.clock())?;
    let authorization = header(request.headers(), "authorization")?;
    if let Some(kind) = social_voice_http::route(&path) {
        if method != "GET" {
            return Err(Error(405, "Use GET for voice connections"));
        }
        return social_voice_http::upgrade(app, request, authorization, kind, _lease).await;
    }
    if path == "/v1/account/voice-blocks" {
        if method != "PUT" {
            return Err(Error(405, "Use PUT for voice preferences"));
        }
        let body = read_body_limit(request, 48 * 1024).await?;
        let deadline = Instant::now() + Duration::from_secs(25);
        let public = app.clone();
        return tokio::task::spawn_blocking(move || {
            let _lease = _lease;
            public
                .budgets
                .lock()
                .map_err(|_| Error(503, "Request budgets unavailable"))?
                .owner_source(source, public.clock())?;
            let token = authentication::bearer(&authorization)?;
            let identity = public.identity(token, deadline)?;
            public.lock()?.voice_preferences(identity, body, deadline)
        })
        .await
        .map_err(|_| Error(503, "Voice preferences worker unavailable"))?
        .map(|value| response(200, value));
    }
    let connection_budget = request
        .extensions()
        .get::<Arc<connection_budget::Budget>>()
        .cloned();
    if method == "GET" && path == "/v1/account/steam-friends" {
        return steam_friends_http::handle(
            app,
            authorization,
            source,
            None,
            Value::Null,
            _lease,
            connection_budget,
        )
        .await;
    }
    if let Some(route) = steam_http::route(&method, &path) {
        return steam_http::handle(app, request, authorization, route, source, _lease).await;
    }
    if let Some(route) = eos_http::route(&method, &path) {
        return eos_http::handle(app, request, authorization, route).await;
    }
    if method == "GET" && path == "/v1/matchmaking/qos" {
        return websocket::upgrade(app, request, authorization, _lease).await;
    }
    if method == "GET" {
        if let Some((id, route)) = room_websocket::route(&path) {
            return room_websocket::upgrade(app, request, id, route, authorization, _lease).await;
        }
    }
    let mut pairs = Vec::new();
    let mut query = serde_json::Map::new();
    for (key, value) in form_urlencoded::parse(request.uri().query().unwrap_or("").as_bytes()) {
        pairs.push(json!([key, value]));
        query.entry(key.into_owned()).or_insert(json!(value));
    }
    let body = if method == "POST" || method == "PUT" {
        read_body(request).await?
    } else {
        Value::Null
    };
    let public = app.clone();
    if method == "POST" && path == "/v1/party/commands" && body["action"] == "invite_steam" {
        return steam_friends_http::handle(
            app,
            authorization,
            source,
            Some(body),
            Value::Object(query),
            _lease,
            connection_budget,
        )
        .await;
    }
    let deadline = Instant::now() + Duration::from_secs(25);
    tokio::task::spawn_blocking(move || {
        let _lease = _lease;
        if Instant::now() >= deadline {
            return Err(Error(503, "Service request expired"));
        }
        match (method.as_str(), path.as_str()) {
            ("GET", "/health" | "/v1/health") => {
                let mut value = public.lock()?.health()?;
                value["eos_identity"] = public.eos.as_ref().map_or(Value::Null, |eos| eos.health());
                if !public.steam_key.is_empty() {
                    value["capabilities"]
                        .as_array_mut()
                        .unwrap()
                        .push(json!("steam_relationships"));
                }
                Ok((200, value))
            }
            ("POST", "/v1/account/session") => {
                public.lock()?.validate_login(&body)?;
                public
                    .budgets
                    .lock()
                    .map_err(|_| Error(503, "Request budgets unavailable"))?
                    .owner_source(source, public.clock())?;
                let token = authentication::bearer(&authorization)?;
                let identity = public.identity(token, deadline)?;
                Ok((201, public.lock()?.login(identity, body)?))
            }
            ("DELETE", "/v1/account/session") => Ok((200, public.lock()?.logout(&authorization)?)),
            ("GET", "/v1/online/status") => Ok((200, public.lock()?.population(json!(pairs))?)),
            ("GET", "/v1/friends") => Ok((
                200,
                public.lock()?.friends(&authorization, None, json!(pairs))?,
            )),
            ("POST", "/v1/friends/commands") => Ok((
                200,
                public
                    .lock()?
                    .friends(&authorization, Some(body), json!(pairs))?,
            )),
            ("POST", "/v1/account/link-discord") => {
                public
                    .lock()?
                    .link_discord(&authorization, body.clone(), None)?;
                let _primary =
                    public.identity(authentication::bearer(&authorization)?, deadline)?;
                let token =
                    halo2_online_protocol::matchmaking_model::text(&body["discord_token"], 2048)?;
                let social = authentication::discord(token, &public.discord_client_id)?;
                if Instant::now() >= deadline {
                    return Err(Error(503, "Service request expired"));
                }
                Ok((
                    200,
                    public
                        .lock()?
                        .link_discord(&authorization, body, Some(social))?,
                ))
            }
            ("POST", "/v1/matchmaking/session") => {
                let plan = public.lock()?.matchmaking_body(&body)?;
                public
                    .budgets
                    .lock()
                    .map_err(|_| Error(503, "Request budgets unavailable"))?
                    .owner_source(source, public.clock())?;
                let primary = public.identity(authentication::bearer(&authorization)?, deadline)?;
                let mut identities = vec![Some(primary)];
                for local in halo2_online_protocol::matchmaking_model::array(&plan, "locals") {
                    identities.push(if let Some(proof) = local["proof"].as_str() {
                        let header = format!("Bearer {proof}");
                        Some(public.identity(authentication::bearer(&header)?, deadline)?)
                    } else {
                        None
                    });
                }
                Ok((201, public.lock()?.matchmaking_login(body, identities)?))
            }
            ("DELETE", "/v1/matchmaking/session") => {
                Ok((200, public.lock()?.matchmaking_logout(&authorization)?))
            }
            ("GET", "/v1/matchmaking/playlists") => {
                Ok((200, public.lock()?.playlists(&authorization)?))
            }
            ("GET", "/v1/matchmaking/transport") => {
                Ok((200, public.lock()?.room_descriptor(&authorization)?))
            }
            ("POST", "/v1/matchmaking/ticket") => Ok((
                201,
                public
                    .lock()?
                    .ticket(&authorization, "POST", body, json!({}))?,
            )),
            ("POST", "/v1/matchmaking/failure") => Ok((
                200,
                public.lock()?.room_content_failure(&authorization, body)?,
            )),
            ("GET", "/v1/matchmaking/ticket") => Ok((
                200,
                public
                    .lock()?
                    .ticket(&authorization, "GET", Value::Null, Value::Object(query))?,
            )),
            ("DELETE", "/v1/matchmaking/ticket") => Ok((
                200,
                public.lock()?.ticket(
                    &authorization,
                    "DELETE",
                    Value::Null,
                    Value::Object(query),
                )?,
            )),
            ("GET", "/v1/party") => Ok((
                200,
                public
                    .lock()?
                    .party(&authorization, "get", Value::Null, Value::Object(query))?,
            )),
            ("POST", "/v1/party/commands") => Ok((
                200,
                public
                    .lock()?
                    .party(&authorization, "command", body, Value::Object(query))?,
            )),
            ("POST", "/v1/party/activity") => Ok((
                200,
                public
                    .lock()?
                    .party(&authorization, "activity", body, Value::Null)?,
            )),
            ("GET", "/v1/party/countdown") => {
                Ok((200, public.lock()?.party_countdown(&authorization)?))
            }
            ("GET", "/v1/clans") => Ok((
                200,
                public
                    .lock()?
                    .clan(&authorization, None, Value::Object(query))?,
            )),
            ("POST", "/v1/clans/commands") => Ok((
                200,
                public
                    .lock()?
                    .clan(&authorization, Some(body), Value::Object(query))?,
            )),
            ("GET", "/v1/account/profile") => {
                Ok((200, public.lock()?.profile(&authorization, None)?))
            }
            ("PUT", "/v1/account/profile") => {
                Ok((200, public.lock()?.profile(&authorization, Some(body))?))
            }
            ("GET", "/v1/history/players") => {
                Ok((200, public.lock()?.history("search", None, json!(pairs))?))
            }
            ("GET", _) => {
                let segments = path.split('/').collect::<Vec<_>>();
                let route = match segments.as_slice() {
                    ["", "v1", "history", "players", target, "matches"] => {
                        Some(("matches", *target))
                    }
                    ["", "v1", "history", "players", target, "profile"] => {
                        Some(("profile", *target))
                    }
                    ["", "v1", "history", "matches", target] => Some(("report", *target)),
                    _ => None,
                };
                if let Some((mode, target)) = route {
                    Ok((
                        200,
                        public.lock()?.history(mode, Some(target), json!(pairs))?,
                    ))
                } else {
                    Err(Error(503, "Required service capability unavailable"))
                }
            }
            _ => Err(Error(503, "Required service capability unavailable")),
        }
    })
    .await
    .map_err(|_| Error(503, "Service request worker unavailable"))?
    .map(|(status, value)| response(status, value))
}
pub async fn maintain(app: Arc<App>) {
    let mut interval = tokio::time::interval(Duration::from_millis(50));
    interval.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
    let mut failed = false;
    loop {
        interval.tick().await;
        let app = app.clone();
        let result = tokio::task::spawn_blocking(move || {
            if let Ok(mut runtime) = app.runtime.try_lock() {
                runtime.maintenance()
            } else {
                Ok(())
            }
        })
        .await;
        let error = match result {
            Ok(Err(e)) => Some(e),
            Err(_) => Some(Error(503, "Service maintenance unavailable")),
            _ => None,
        };
        if let Some(e) = error {
            if !failed {
                eprintln!("Rust service maintenance: {} ({})", e.1, e.0);
            }
            failed = true;
        } else {
            failed = false;
        }
    }
}
pub async fn close_voice(app: Arc<App>) {
    // A101 can already be on the wire before the post-upgrade owner attaches.
    // Include those transport owners in the shutdown fence as well.
    app.voice_sockets.close();
    if let Ok(mut runtime) = app.lock() {
        runtime.voice_shutdown();
    }
    let deadline = Instant::now() + Duration::from_secs(2);
    while Instant::now() < deadline {
        if app.voice_sockets.available_permits() == VOICE_CONNECTIONS
            && app
                .lock()
                .is_ok_and(|runtime| runtime.voice_connections() == 0)
        {
            break;
        }
        tokio::time::sleep(Duration::from_millis(10)).await;
    }
}
pub async fn serve(listener: TcpListener, app: Arc<App>) -> std::io::Result<()> {
    let connections = Arc::new(Semaphore::new(256));
    let router = Router::new().fallback(handle).with_state(app);
    loop {
        let (stream, peer) = listener.accept().await?;
        let Ok(lease) = connections.clone().try_acquire_owned() else {
            drop(stream);
            continue;
        };
        let budget = Arc::new(connection_budget::Budget::new());
        let router = router
            .clone()
            .layer(axum::Extension(peer))
            .layer(axum::Extension(budget.clone()));
        tokio::spawn(async move {
            let _lease = lease;
            let mut builder = hyper::server::conn::http1::Builder::new();
            builder
                .timer(TokioTimer::new())
                .header_read_timeout(Duration::from_secs(5))
                .max_headers(64)
                .max_buf_size(16384)
                // Ordinary replies explicitly close above. Disabling this
                // globally also adds `close` to a101 upgrade response.
                .keep_alive(true);
            tokio::select! {
                _=budget.expired()=>{},
                _=builder
                    .serve_connection(TokioIo::new(stream), TowerToHyperService::new(router))
                    .with_upgrades()=>{},
            }
        });
    }
}
