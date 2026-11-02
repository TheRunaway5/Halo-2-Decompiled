//! Private social voice WebSockets; the shared Rust relay owns all decisions.
use super::{
    websocket::{call, checked_upgrade},
    *,
};
use crate::runtime::{VoiceBinding, VoiceGrant, VoiceKind};
use axum::extract::ws::{CloseFrame, Message, WebSocket};
use tokio::sync::OwnedSemaphorePermit;

pub(super) fn route(path: &str) -> Option<VoiceKind> {
    match path {
        "/v1/clans/voice" => Some(VoiceKind::Clan),
        "/v1/party/voice" => Some(VoiceKind::Party),
        _ => None,
    }
}
pub(super) async fn upgrade(
    app: Arc<App>,
    request: Request,
    authorization: String,
    kind: VoiceKind,
    request_lease: OwnedSemaphorePermit,
) -> Result<Response> {
    let lease = app
        .voice_sockets
        .clone()
        .try_acquire_owned()
        .map_err(|_| Error(503, "Voice connection capacity reached"))?;
    let prior = call(app.clone(), move |runtime| {
        let _lease = request_lease;
        runtime.voice_authorize(kind, authorization)
    })
    .await?;
    let socket = checked_upgrade(request).await?;
    let mut response = socket
        .read_buffer_size(512)
        .write_buffer_size(0)
        .max_write_buffer_size(1024)
        .max_message_size(262)
        .max_frame_size(262)
        .accept_unmasked_frames(false)
        .on_upgrade(move |socket| run(app, socket, prior, lease));
    response.headers_mut().insert(
        "cache-control",
        axum::http::HeaderValue::from_static("no-store"),
    );
    Ok(response)
}
struct Retire(VoiceBinding);
impl Drop for Retire {
    fn drop(&mut self) {
        self.0.retire();
    }
}
async fn send(socket: &mut WebSocket, message: Message) -> bool {
    matches!(
        tokio::time::timeout(Duration::from_millis(250), socket.send(message)).await,
        Ok(Ok(()))
    )
}
async fn close(socket: &mut WebSocket, code: u16) {
    if !send(
        socket,
        Message::Close(Some(CloseFrame {
            code,
            reason: "Private voice session ended".into(),
        })),
    )
    .await
    {
        return;
    }
    // Finish the close handshake before dropping a stream with queued input.
    // Otherwise TCP can reset it before the peer observes the close frame.
    let _ = tokio::time::timeout(Duration::from_millis(250), async {
        for _ in 0..64 {
            match socket.recv().await {
                Some(Ok(Message::Close(_))) | Some(Err(_)) | None => break,
                _ => {}
            }
        }
    })
    .await;
}
async fn run(
    app: Arc<App>,
    mut socket: WebSocket,
    prior: VoiceGrant,
    _lease: OwnedSemaphorePermit,
) {
    if app.voice_sockets.is_closed() {
        close(&mut socket, 1001).await;
        return;
    }
    let binding = match call(app.clone(), move |runtime| runtime.voice_attach(prior)).await {
        Ok(binding) => binding,
        Err(_) => {
            close(
                &mut socket,
                if app.voice_sockets.is_closed() {
                    1001
                } else {
                    1008
                },
            )
            .await;
            return;
        }
    };
    let _retire = Retire(binding.clone());
    let wake = binding.wake.clone();
    let mut code = 1000;
    let mut last_input = Instant::now();
    let mut pong_deadline = None;
    enum Event {
        Incoming(Option<std::result::Result<Message, axum::Error>>),
        Poll,
    }
    let mut poll = true;
    loop {
        if app.voice_sockets.is_closed() {
            code = 1001;
            break;
        }
        if pong_deadline.is_some_and(|deadline| Instant::now() >= deadline) {
            code = 1011;
            break;
        }
        if pong_deadline.is_none() && last_input.elapsed() >= Duration::from_secs(10) {
            if !send(&mut socket, Message::Ping(Vec::new().into())).await {
                code = 1011;
                break;
            }
            pong_deadline = Some(Instant::now() + Duration::from_secs(5));
        }
        if binding.closed() {
            code = binding.close_code();
            break;
        }
        let reply = if poll {
            let actor = binding.clone();
            call(app.clone(), move |runtime| runtime.voice_poll(actor)).await
        } else {
            let event = tokio::select! {
                message=socket.recv()=>Event::Incoming(message),
                _=wake.notified()=>Event::Poll,
                _=tokio::time::sleep(Duration::from_millis(500))=>Event::Poll,
            };
            if matches!(&event, Event::Incoming(Some(Ok(_)))) {
                last_input = Instant::now();
                pong_deadline = None;
            }
            match event {
                Event::Poll => {
                    poll = true;
                    continue;
                }
                Event::Incoming(Some(Ok(Message::Ping(_) | Message::Pong(_)))) => continue,
                Event::Incoming(Some(Ok(Message::Binary(bytes)))) => {
                    let actor = binding.clone();
                    call(app.clone(), move |runtime| {
                        runtime.voice_frame(actor, true, bytes.to_vec())
                    })
                    .await
                }
                Event::Incoming(Some(Ok(Message::Text(_)))) => {
                    let actor = binding.clone();
                    call(app.clone(), move |runtime| {
                        runtime.voice_frame(actor, false, Vec::new())
                    })
                    .await
                }
                Event::Incoming(Some(Err(_))) => {
                    code = 1008;
                    break;
                }
                Event::Incoming(_) => break,
            }
        };
        let reply = match reply {
            Ok(reply) => reply,
            Err(_) => {
                code = 1008;
                break;
            }
        };
        if reply["close"].is_string() {
            code = reply["code"]
                .as_u64()
                .and_then(|c| u16::try_from(c).ok())
                .unwrap_or(1008);
            break;
        }
        if let Some(packet) = reply["packet"].as_str() {
            let bytes = match halo2_online_protocol::matchmaking_model::hex(
                &json!(packet),
                packet.len() / 2,
            ) {
                Ok(bytes) => bytes,
                Err(_) => {
                    code = 1011;
                    break;
                }
            };
            if !send(&mut socket, Message::Binary(bytes.into())).await {
                code = 1011;
                break;
            }
        }
        poll = reply["again"] == true;
    }
    close(&mut socket, code).await;
    let actor = binding.clone();
    let _ = call(app, move |runtime| runtime.voice_detach(actor)).await;
}
