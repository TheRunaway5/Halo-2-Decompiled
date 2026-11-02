//! Bounded search WebSocket IO; the shared native owner authorizes every step.
use super::*;
use axum::extract::{
    FromRequestParts,
    ws::{Message, WebSocket, WebSocketUpgrade},
};
use base64::{Engine as _, engine::general_purpose::STANDARD};
use tokio::sync::OwnedSemaphorePermit;

pub(super) async fn call<T: Send + 'static>(
    app: Arc<App>,
    work: impl FnOnce(&mut Runtime) -> Result<T> + Send + 'static,
) -> Result<T> {
    let lease = app
        .socket_work
        .clone()
        .acquire_owned()
        .await
        .map_err(|_| Error(503, "Search work unavailable"))?;
    tokio::task::spawn_blocking(move || {
        let _lease = lease;
        let mut runtime = app.lock()?;
        work(&mut runtime)
    })
    .await
    .map_err(|_| Error(503, "Search worker unavailable"))?
}
pub(super) async fn upgrade(
    app: Arc<App>,
    request: Request,
    authorization: String,
    request_lease: OwnedSemaphorePermit,
) -> Result<Response> {
    let socket_lease = app
        .search_sockets
        .clone()
        .try_acquire_owned()
        .map_err(|_| Error(503, "Search connection capacity reached"))?;
    let public = app.clone();
    let prior = tokio::task::spawn_blocking(move || {
        let _lease = request_lease;
        public.lock()?.qos_authorize(&authorization)
    })
    .await
    .map_err(|_| Error(503, "Search authorization unavailable"))??;
    let socket = checked_upgrade(request).await?;
    let mut response = socket
        .read_buffer_size(1024)
        .write_buffer_size(0)
        .max_write_buffer_size(512)
        .max_frame_size(256)
        .max_message_size(256)
        .accept_unmasked_frames(false)
        .on_upgrade(move |socket| run(app, socket, prior, socket_lease));
    response.headers_mut().insert(
        "cache-control",
        axum::http::HeaderValue::from_static("no-store"),
    );
    Ok(response)
}
pub(super) async fn checked_upgrade(request: Request) -> Result<WebSocketUpgrade> {
    let version = header(request.headers(), "sec-websocket-version")?;
    let key = header(request.headers(), "sec-websocket-key")?;
    if version != "13"
        || key.len() != 24
        || STANDARD.decode(&key).map_or(true, |raw| raw.len() != 16)
    {
        return Err(Error(400, "Invalid WebSocket upgrade"));
    }
    let (mut parts, _) = request.into_parts();
    WebSocketUpgrade::from_request_parts(&mut parts, &())
        .await
        .map_err(|_| Error(400, "Invalid WebSocket upgrade"))
}
async fn run(app: Arc<App>, mut socket: WebSocket, prior: Value, _lease: OwnedSemaphorePermit) {
    // The actual upgraded stream exists now. The owner rechecks its private
    // session/generation proof after the await before any peer is published.
    let binding: crate::runtime::SocketBinding =
        match call(app.clone(), move |runtime| runtime.qos_attach(prior)).await {
            Ok(binding) => binding,
            Err(_) => {
                let _ = tokio::time::timeout(
                    Duration::from_millis(500),
                    socket.send(Message::Close(None)),
                )
                .await;
                return;
            }
        };
    let mut timer = tokio::time::interval(Duration::from_millis(50));
    timer.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
    loop {
        if binding.closed() {
            break;
        }
        let event = tokio::select! {
            incoming=socket.recv()=>Some(incoming),
            _=timer.tick()=>None,
            _=binding.wake.notified()=>None,
        };
        if let Some(incoming) = event {
            let (kind, text) = match incoming {
                Some(Ok(Message::Text(text))) => ("text", Some(text.to_string())),
                Some(Ok(Message::Binary(_))) => ("binary", None),
                Some(Ok(Message::Ping(_) | Message::Pong(_))) => continue,
                _ => break,
            };
            let actor = binding.actor();
            if call(app.clone(), move |runtime| {
                runtime.qos_receive(actor, kind, text)
            })
            .await
            .is_err()
            {
                break;
            }
        } else {
            let actor = binding.actor();
            let effect = match call(app.clone(), move |runtime| runtime.qos_tick(actor)).await {
                Ok(effect) => effect,
                Err(_) => break,
            };
            if effect["close"] == true {
                break;
            }
            let Some(messages) = effect["messages"].as_array() else {
                break;
            };
            let mut failed = false;
            for message in messages {
                let sent = tokio::time::timeout(
                    Duration::from_millis(500),
                    socket.send(Message::Text(message.to_string().into())),
                )
                .await;
                if !matches!(sent, Ok(Ok(()))) {
                    failed = true;
                    break;
                }
            }
            if failed {
                break;
            }
        }
    }
    binding.close();
    let actor = binding.actor();
    let _ = call(app, move |runtime| runtime.qos_detach(actor)).await;
    let _ = tokio::time::timeout(
        Duration::from_millis(500),
        socket.send(Message::Close(None)),
    )
    .await;
}
