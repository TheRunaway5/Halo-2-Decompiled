//! One bounded owned lookup per HTTP slot. Provider IO never holds the runtime
//! mutex or SQL transaction. Cancellation retains that slot until actual IO ends.
use super::*;
use crate::runtime::SteamFriendsBegin;
use std::sync::atomic::{AtomicBool, Ordering};
use tokio::sync::{OwnedSemaphorePermit, oneshot};
struct Alive(Arc<AtomicBool>);
impl Drop for Alive {
    fn drop(&mut self) {
        self.0.store(false, Ordering::Release);
    }
}
struct Lookup {
    app: Arc<App>,
    id: Option<u64>,
    _request: OwnedSemaphorePermit,
}
impl Drop for Lookup {
    fn drop(&mut self) {
        if let Some(id) = self.id {
            if let Ok(mut runtime) = self.app.lock() {
                runtime.steam_friends_retire(id);
            }
        }
    }
}
fn live(alive: &AtomicBool) -> Result<()> {
    if alive.load(Ordering::Acquire) {
        Ok(())
    } else {
        Err(Error(503, "Steam request cancelled"))
    }
}
fn run(
    lookup: &mut Lookup,
    alive: &AtomicBool,
    header: String,
    source: String,
    body: Option<Value>,
    features: Value,
) -> Result<Value> {
    live(alive)?;
    let app = lookup.app.clone();
    let begin = app.lock()?.steam_friends_begin(
        &header,
        &source,
        body.as_ref(),
        &features,
        &app.steam_key,
    )?;
    let (id, mut event, features) = match begin {
        SteamFriendsBegin::Done(value) => return Ok(value),
        SteamFriendsBegin::Work {
            id,
            event,
            features,
        } => {
            lookup.id = Some(id);
            (id, event, features)
        }
    };
    for _ in 0..4096 {
        live(alive)?;
        if event["kind"] == "done" {
            return if let Some(body) = body {
                app.lock()?.steam_friends_invite(
                    &header,
                    body,
                    features,
                    event["value"].clone(),
                    &app.steam_key,
                    alive,
                )
            } else {
                Ok(event["value"].clone())
            };
        }
        let mut reply = match event["kind"].as_str() {
            Some("sql") => app.lock()?.steam_friends_sql(&event)?,
            Some("http") => crate::steam_friends_provider::get(&event, &app.steam_key)?,
            Some("presence") => json!({"value":app.lock()?.steam_friends_presence()?}),
            Some("wait") => {
                let millis =
                    halo2_online_protocol::matchmaking_model::integer(&event["delay_ms"], 1, 1000)?;
                std::thread::sleep(Duration::from_millis(millis as u64));
                json!({})
            }
            _ => return Err(Error(503, "Unknown native Steam IO event")),
        };
        live(alive)?;
        reply["op"] = json!("reply");
        reply["frame"] = event["frame"].clone();
        event = app.lock()?.steam_friends_step(id, reply, &app.steam_key)?;
    }
    Err(Error(503, "Native Steam work exceeds bounds"))
}
pub(super) async fn handle(
    app: Arc<App>,
    header: String,
    source: IpAddr,
    body: Option<Value>,
    features: Value,
    request: OwnedSemaphorePermit,
    budget: Option<Arc<super::connection_budget::Budget>>,
) -> Result<Response> {
    if let Some(budget) = budget {
        budget.steam();
    }
    let alive = Alive(Arc::new(AtomicBool::new(true)));
    let current = alive.0.clone();
    let (answer, receive) = oneshot::channel();
    let start = std::thread::Builder::new()
        .name("steam-friends".into())
        .spawn(move || {
            let mut lookup = Lookup {
                app,
                id: None,
                _request: request,
            };
            let result = run(
                &mut lookup,
                &current,
                header,
                crate::admission::prefix(source, 128),
                body,
                features,
            );
            drop(lookup);
            let _ = answer.send(result);
        });
    if start.is_err() {
        return Err(Error(503, "Steam lookup worker unavailable"));
    }
    let value = receive
        .await
        .map_err(|_| Error(503, "Steam lookup worker unavailable"))??;
    Ok(response(200, value))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn dropping_owned_lookup_retires_its_fifo_and_http_slot_without_releasing_another_owner() {
        let mut runtime = Runtime::open(std::path::Path::new(":memory:"), true, None).unwrap();
        let account = runtime
            .login(
                authentication::Identity {
                    provider: "steam".into(),
                    subject: "76561197960287930".into(),
                    username: "Private unit receipt".into(),
                    steam_digest: None,
                },
                json!({"screen_name":"Unit"}),
            )
            .unwrap();
        let header = format!("Bearer {}", account["token"].as_str().unwrap());
        let key = "1".repeat(32);
        let mut prepare = |expected: &str| {
            let SteamFriendsBegin::Work { id, mut event, .. } = runtime
                .steam_friends_begin(&header, "private-unit", None, &Value::Null, &key)
                .unwrap()
            else {
                panic!("Private request unexpectedly complete")
            };
            for _ in 0..4 {
                if event["kind"] == expected {
                    return (id, event);
                }
                assert_eq!(event["kind"], "sql");
                let reply = runtime.steam_friends_sql(&event).unwrap();
                event = runtime.steam_friends_step(id, reply, &key).unwrap();
            }
            panic!("Native lookup did not reach expected IO")
        };
        let (first, _) = prepare("http");
        let (second, wait) = prepare("wait");
        let app = Arc::new(App::new(
            runtime,
            "1558522073272426519".into(),
            false,
            None,
            key.clone(),
        ));
        let owned = Lookup {
            app: app.clone(),
            id: Some(first),
            _request: app.requests.clone().try_acquire_owned().unwrap(),
        };
        let following = Lookup {
            app: app.clone(),
            id: Some(second),
            _request: app.requests.clone().try_acquire_owned().unwrap(),
        };
        assert_eq!(app.requests.available_permits(), 62);
        drop(owned);
        assert_eq!(app.requests.available_permits(), 63);
        assert_eq!(
            app.lock()
                .unwrap()
                .steam_friends_step(first, json!({"op":"begin"}), &key)
                .err()
                .unwrap()
                .0,
            503
        );
        let next = app
            .lock()
            .unwrap()
            .steam_friends_step(second, json!({"op":"reply","frame":wait["frame"]}), &key)
            .unwrap();
        assert_eq!(next["kind"], "http");
        drop(following);
        assert_eq!(app.requests.available_permits(), 64);
    }
}
