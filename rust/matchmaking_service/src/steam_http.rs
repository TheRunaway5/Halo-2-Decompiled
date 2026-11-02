//! Browser/device HTTP and bounded provider IO. No verifier holds SQL or the
//! matchmaking mutex while talking to Valve. Queued work retains its permit.
use super::*;
use crate::steam_login::{self as steam, Callback, Reply};
use std::sync::atomic::{AtomicBool, Ordering};
use tokio::sync::{OwnedSemaphorePermit, oneshot};

pub(super) enum Route {
    Start,
    Poll,
    Resume,
    Forget,
    Callback(String),
    Confirm(String, bool),
    Complete,
    WrongMethod,
}
pub(super) fn route(method: &str, path: &str) -> Option<Route> {
    let suffix = path.strip_prefix("/v1/auth/steam/")?;
    match suffix {
        "start" => Some(if method == "POST" {
            Route::Start
        } else {
            Route::WrongMethod
        }),
        "poll" => Some(if method == "POST" {
            Route::Poll
        } else {
            Route::WrongMethod
        }),
        "resume" => Some(if method == "POST" {
            Route::Resume
        } else {
            Route::WrongMethod
        }),
        "forget" => Some(if method == "POST" {
            Route::Forget
        } else {
            Route::WrongMethod
        }),
        "complete" => Some(if method == "GET" {
            Route::Complete
        } else {
            Route::WrongMethod
        }),
        _ => {
            let (kind, state) = suffix.split_once('/')?;
            if state.contains('/') || state.is_empty() {
                return None;
            }
            match kind {
                "callback" => Some(if method == "GET" {
                    Route::Callback(state.into())
                } else {
                    Route::WrongMethod
                }),
                "confirm" => Some(match method {
                    "GET" => Route::Confirm(state.into(), false),
                    "POST" => Route::Confirm(state.into(), true),
                    _ => Route::WrongMethod,
                }),
                _ => None,
            }
        }
    }
}
fn render(reply: Reply) -> Result<Response> {
    let mut response = Response::builder()
        .status(reply.status)
        .header("content-type", reply.content_type)
        .header("cache-control", "no-store")
        .header("connection", "close");
    for (name, value) in reply.headers {
        response = response.header(name, value);
    }
    response
        .body(axum::body::Body::from(reply.body))
        .map_err(|_| Error(500, "Steam response unavailable"))
}
fn cookie(headers: &axum::http::HeaderMap) -> Result<String> {
    let raw = header(headers, "cookie")?;
    let mut found = None;
    for item in raw.split(';') {
        if let Some(("steam-confirm", value)) = item.trim().split_once('=') {
            if found.replace(value.to_owned()).is_some() {
                return Err(Error(401, "Steam confirmation expired"));
            }
        }
    }
    Ok(found.unwrap_or_default())
}
struct Queued {
    plan: Option<steam::Plan>,
    request: Option<OwnedSemaphorePermit>,
}
struct CallbackWork {
    current: AtomicBool,
    queued: std::sync::Mutex<Queued>,
}
impl CallbackWork {
    fn keep_queued(&self, runtime: &mut Runtime, plan: &steam::Plan) -> Result<()> {
        let Ok(mut queued) = self.queued.lock() else {
            runtime.steam_failed(plan);
            return Err(Error(503, "Steam callback unavailable"));
        };
        if !self.current.load(Ordering::Acquire) {
            runtime.steam_failed(plan);
            return Err(Error(503, "Steam verification request expired"));
        }
        queued.plan = Some(plan.clone());
        Ok(())
    }
    fn start(&self) -> Result<OwnedSemaphorePermit> {
        let mut queued = self
            .queued
            .lock()
            .map_err(|_| Error(503, "Steam callback unavailable"))?;
        let request = queued
            .request
            .take()
            .ok_or(Error(503, "Steam callback unavailable"))?;
        queued.plan.take();
        Ok(request)
    }
}
struct Alive {
    app: Arc<App>,
    work: Arc<CallbackWork>,
}
impl Alive {
    fn new(app: Arc<App>, request: OwnedSemaphorePermit) -> Self {
        Self {
            app,
            work: Arc::new(CallbackWork {
                current: AtomicBool::new(true),
                queued: std::sync::Mutex::new(Queued {
                    plan: None,
                    request: Some(request),
                }),
            }),
        }
    }
}
impl Drop for Alive {
    fn drop(&mut self) {
        self.work.current.store(false, Ordering::Release);
        let plan = self.work.queued.lock().ok().and_then(|mut q| q.plan.take());
        if let Some(plan) = plan {
            let app = self.app.clone();
            let work = self.work.clone();
            if let Ok(handle) = tokio::runtime::Handle::try_current() {
                handle.spawn(async move {
                    let _work = work;
                    let _ = websocket::call(app, move |r| {
                        r.steam_failed(&plan);
                        Ok(())
                    })
                    .await;
                });
            } else if let Ok(mut runtime) = app.runtime.try_lock() {
                runtime.steam_failed(&plan);
            }
        }
    }
}
pub(super) async fn handle(
    app: Arc<App>,
    request: Request,
    authorization: String,
    route: Route,
    source: IpAddr,
    request_lease: OwnedSemaphorePermit,
) -> Result<Response> {
    let source_key = crate::admission::prefix(source, 128);
    match route {
        Route::Complete => render(Reply::complete()),
        Route::WrongMethod => Err(Error(405, "Steam route method not allowed")),
        Route::Start => {
            app.budgets
                .lock()
                .map_err(|_| Error(503, "Request budgets unavailable"))?
                .steam("start", &source_key, app.clock())?;
            let body = read_body(request).await?;
            let public = app.clone();
            let prefix = crate::admission::prefix(source, 64);
            render(
                websocket::call(app, move |runtime| {
                    runtime.steam_start(body, source_key, prefix, || {
                        public
                            .budgets
                            .lock()
                            .map_err(|_| Error(503, "Request budgets unavailable"))?
                            .steam("global", "starts", public.clock())
                    })
                })
                .await?,
            )
        }
        Route::Poll | Route::Resume | Route::Forget => {
            let name = match route {
                Route::Poll => "poll",
                Route::Resume => "resume",
                _ => "forget",
            };
            app.budgets
                .lock()
                .map_err(|_| Error(503, "Request budgets unavailable"))?
                .steam(name, &source_key, app.clock())?;
            let body = read_body(request).await?;
            if matches!(route, Route::Poll) {
                if let Some(device) = body["device"].as_str() {
                    steam::proof(device, 43, 43)?;
                    app.budgets
                        .lock()
                        .map_err(|_| Error(503, "Request budgets unavailable"))?
                        .steam("device", device, app.clock())?;
                }
            }
            render(
                websocket::call(app, move |runtime| match route {
                    Route::Poll => runtime.steam_poll(body),
                    Route::Resume => runtime.steam_resume(&authorization, body),
                    _ => runtime.steam_forget(&authorization, body),
                })
                .await?,
            )
        }
        Route::Confirm(state, post) => {
            let cookie = cookie(request.headers())?;
            if !post {
                return render(
                    websocket::call(app, move |runtime| {
                        runtime.steam_confirmation_page(&state, &cookie)
                    })
                    .await?,
                );
            }
            let origin = header(request.headers(), "origin")?;
            let prior = websocket::call(app.clone(), move |runtime| {
                runtime.steam_confirmation(&state, &cookie, Some(&origin))
            })
            .await?;
            if header(request.headers(), "content-type")?
                .split(';')
                .next()
                .unwrap_or("")
                .trim()
                != "application/x-www-form-urlencoded"
            {
                return Err(Error(415, "Use form encoding for confirmation"));
            }
            let data =
                tokio::time::timeout(Duration::from_secs(5), to_bytes(request.into_body(), 1024))
                    .await
                    .map_err(|_| Error(408, "Confirmation body timed out"))?
                    .map_err(|_| Error(413, "Confirmation too large"))?;
            render(
                websocket::call(app, move |runtime| {
                    runtime.steam_confirmed(prior, data.to_vec())
                })
                .await?,
            )
        }
        Route::Callback(state) => {
            app.budgets
                .lock()
                .map_err(|_| Error(503, "Request budgets unavailable"))?
                .steam("callback", &source_key, app.clock())?;
            let query = steam::query(request.uri().query().unwrap_or(""))?;
            // Own cancellation and the HTTP slot before the blocking preflight
            // can change pending state, including cancellation during its await.
            let alive = Alive::new(app.clone(), request_lease);
            let current = alive.work.clone();
            let preflight = current.clone();
            let prior = websocket::call(app.clone(), move |runtime| {
                steam::need(
                    preflight.current.load(Ordering::Acquire),
                    503,
                    "Steam verification request expired",
                )?;
                let prior = runtime.steam_callback(&state, query)?;
                if let Callback::Verify(plan) = &prior {
                    preflight.keep_queued(runtime, plan)?;
                }
                Ok(prior)
            })
            .await?;
            let Callback::Verify(plan) = prior else {
                let Callback::Redirect(reply) = prior else {
                    unreachable!()
                };
                return render(reply);
            };
            let deadline = Instant::now() + Duration::from_secs(20);
            let permit = match tokio::time::timeout(
                Duration::from_secs(2),
                app.steam_providers.clone().acquire_owned(),
            )
            .await
            {
                Ok(Ok(permit)) => permit,
                _ => {
                    let p = plan.clone();
                    let _ = websocket::call(app, move |r| {
                        r.steam_failed(&p);
                        Ok(())
                    })
                    .await;
                    return Err(Error(503, "Steam verification busy; retry"));
                }
            };
            let (answer, receive) = oneshot::channel();
            let public = app.clone();
            let failed = plan.clone();
            let request_lease = current.start()?;
            let start = std::thread::Builder::new()
                .name("steam-verification".into())
                .spawn(move || {
                    let (_permit, _request) = (permit, request_lease);
                    let result = (|| {
                        if !current.current.load(Ordering::Acquire) || Instant::now() >= deadline {
                            return Err(Error(503, "Steam verification request expired"));
                        }
                        let owner = steam::provider::verify(&plan, &public.steam_key)?;
                        public
                            .lock()?
                            .steam_verified(&plan, owner, &current.current, deadline)
                    })();
                    if result.is_err() {
                        if let Ok(mut runtime) = public.lock() {
                            runtime.steam_failed(&plan);
                        }
                    }
                    let _ = answer.send(result);
                });
            if start.is_err() {
                let _ = websocket::call(app, move |r| {
                    r.steam_failed(&failed);
                    Ok(())
                })
                .await;
                return Err(Error(503, "Steam verifier worker unavailable"));
            }
            let reply = receive
                .await
                .map_err(|_| Error(503, "Steam verifier worker unavailable"))??;
            render(reply)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (Arc<App>, String, String, String) {
        use base64::Engine as _;
        use sha2::{Digest, Sha256};
        let mut runtime = Runtime::open(std::path::Path::new(":memory:"), true, None).unwrap();
        runtime
            .configure_steam("https://steam-fixture.invalid", false)
            .unwrap();
        let verifier = "v".repeat(43);
        let hash = base64::engine::general_purpose::URL_SAFE_NO_PAD
            .encode(Sha256::digest(verifier.as_bytes()));
        let reply = runtime
            .steam_start(
                json!({"challenge":hash,"confirmation":hash}),
                "source".into(),
                "prefix".into(),
                || Ok(()),
            )
            .unwrap();
        let value: Value = serde_json::from_str(&reply.body).unwrap();
        let device = value["device"].as_str().unwrap().to_owned();
        let nonce = format!(
            "{}{}",
            chrono::DateTime::from_timestamp(crate::database::wall_now().unwrap(), 0)
                .unwrap()
                .format("%Y-%m-%dT%H:%M:%SZ"),
            device
        );
        let raw = form_urlencoded::Serializer::new(String::new())
            .extend_pairs([
                ("openid.ns", steam::NS),
                ("openid.mode", "id_res"),
                ("openid.op_endpoint", steam::OP),
                (
                    "openid.claimed_id",
                    "https://steamcommunity.com/openid/id/76561197960287930",
                ),
                (
                    "openid.identity",
                    "https://steamcommunity.com/openid/id/76561197960287930",
                ),
                (
                    "openid.return_to",
                    &format!("https://steam-fixture.invalid/v1/auth/steam/callback/{device}"),
                ),
                ("openid.response_nonce", &nonce),
                ("openid.assoc_handle", "fixture"),
                (
                    "openid.signed",
                    "op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle",
                ),
                ("openid.sig", "private-unit-provider-receipt"),
            ])
            .finish();
        let app = Arc::new(App::new(
            runtime,
            "1558522073272426519".into(),
            false,
            None,
            String::new(),
        ));
        (app, device, verifier, raw)
    }
    #[test]
    fn cancellation_during_preflight_cannot_strand_verification_or_release_its_request_early() {
        let (app, device, verifier, raw) = fixture();
        let alive = Alive::new(
            app.clone(),
            app.requests.clone().try_acquire_owned().unwrap(),
        );
        let work = alive.work.clone();
        let mut runtime = app.lock().unwrap();
        let Callback::Verify(plan) = runtime
            .steam_callback(&device, steam::query(&raw).unwrap())
            .unwrap()
        else {
            panic!("Private callback fixture did not request verification")
        };
        // Cancel after the blocking owner changes state but before its handoff.
        drop(alive);
        assert_eq!(work.keep_queued(&mut runtime, &plan).err().unwrap().0, 503);
        let reply = runtime
            .steam_poll(json!({"device":device,"verifier":verifier}))
            .unwrap();
        assert_eq!(
            serde_json::from_str::<Value>(&reply.body).unwrap()["state"],
            "failed"
        );
        assert_eq!(app.requests.available_permits(), 63);
        drop(runtime);
        drop(work);
        assert_eq!(app.requests.available_permits(), 64);
    }
    #[tokio::test]
    async fn cancelled_provider_wait_retires_pending_and_releases_the_owned_request_slot() {
        let (app, device, verifier, raw) = fixture();
        let held = app
            .steam_providers
            .clone()
            .acquire_many_owned(2)
            .await
            .unwrap();
        let request = Request::builder()
            .uri(format!("/v1/auth/steam/callback/{device}?{raw}"))
            .extension("127.0.0.1:12345".parse::<SocketAddr>().unwrap())
            .body(axum::body::Body::empty())
            .unwrap();
        let public = app.clone();
        let task = tokio::spawn(async move { dispatch(public, request).await });
        let poll = json!({"device":device,"verifier":verifier});
        let phase = |app: &App| {
            let r = app.lock().unwrap().steam_poll(poll.clone()).unwrap();
            serde_json::from_str::<Value>(&r.body).unwrap()["state"]
                .as_str()
                .unwrap()
                .to_owned()
        };
        let deadline = Instant::now() + Duration::from_secs(3);
        while phase(&app) != "verifying" {
            assert!(Instant::now() < deadline);
            tokio::time::sleep(Duration::from_millis(5)).await;
        }
        task.abort();
        assert!(matches!(task.await,Err(e) if e.is_cancelled()));
        while phase(&app) != "failed" || app.requests.available_permits() != 64 {
            assert!(Instant::now() < deadline);
            tokio::time::sleep(Duration::from_millis(5)).await;
        }
        assert_eq!(app.steam_providers.available_permits(), 0);
        drop(held);
        assert_eq!(app.steam_providers.available_permits(), 2);
    }
}
