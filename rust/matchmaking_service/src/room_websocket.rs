//! Actual bounded room WebSockets; every protocol decision stays in Rust owners.
use super::websocket::{call, checked_upgrade};
use super::*;
use crate::runtime::{RoomBinding, RoomOutbound, RoomQueued};
use axum::extract::ws::{CloseFrame, Message, WebSocket};
use tokio::sync::OwnedSemaphorePermit;

pub(super) fn route(path: &str) -> Option<(String, String)> {
    let (id, route) = path.strip_prefix("/v1/matches/")?.split_once('/')?;
    if id.is_empty() || id.len() > 64 || !["signal", "relay"].contains(&route) {
        return None;
    }
    Some((id.to_owned(), route.to_owned()))
}
pub(super) async fn upgrade(
    app: Arc<App>,
    request: Request,
    id: String,
    route: String,
    authorization: String,
    request_lease: OwnedSemaphorePermit,
) -> Result<Response> {
    let socket_lease = app
        .room_sockets
        .clone()
        .try_acquire_owned()
        .map_err(|_| Error(503, "Match connection capacity reached"))?;
    let (room, kind, header) = (id.clone(), route.clone(), authorization.clone());
    let prior = call(app.clone(), move |runtime| {
        let _lease = request_lease;
        runtime.room_authorize(&room, &kind, &header)
    })
    .await?;
    let mut features = json!({});
    for (key, value) in form_urlencoded::parse(request.uri().query().unwrap_or("").as_bytes()) {
        features
            .as_object_mut()
            .unwrap()
            .entry(key.into_owned())
            .or_insert(json!(value));
    }
    let socket = checked_upgrade(request).await?;
    let mut response = socket
        .read_buffer_size(4096)
        .write_buffer_size(0)
        .max_write_buffer_size(131072)
        .max_frame_size(120000)
        .max_message_size(120000)
        .accept_unmasked_frames(false)
        .on_upgrade(move |socket| {
            run(
                app,
                socket,
                id,
                route,
                authorization,
                prior,
                features,
                socket_lease,
            )
        });
    response.headers_mut().insert(
        "cache-control",
        axum::http::HeaderValue::from_static("no-store"),
    );
    Ok(response)
}
async fn send(socket: &mut WebSocket, message: Message) -> bool {
    matches!(
        tokio::time::timeout(Duration::from_millis(500), socket.send(message)).await,
        Ok(Ok(()))
    )
}
async fn run(
    app: Arc<App>,
    mut socket: WebSocket,
    id: String,
    route: String,
    authorization: String,
    prior: Value,
    features: Value,
    _lease: OwnedSemaphorePermit,
) {
    let mut binding: RoomBinding = match call(app.clone(), move |runtime| {
        runtime.room_attach(&id, &route, &authorization, prior, features)
    })
    .await
    {
        Ok(binding) => binding,
        Err(error) => {
            let _ = send(
                &mut socket,
                Message::Close(Some(CloseFrame {
                    code: 1008,
                    reason: error.1.into(),
                })),
            )
            .await;
            return;
        }
    };
    let wake = binding.wake.clone();
    let mut timer = tokio::time::interval(Duration::from_millis(50));
    timer.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
    let mut io_failure = false;
    let mut rejected = false;
    enum Event {
        Incoming(Option<std::result::Result<Message, axum::Error>>),
        Queued(Option<RoomQueued>),
        Tick,
    }
    loop {
        if binding.closed() {
            break;
        }
        let event = tokio::select! {
            incoming=socket.recv()=>Event::Incoming(incoming),
            queued=binding.incoming.recv()=>Event::Queued(queued),
            _=timer.tick()=>Event::Tick,
            _=wake.notified()=>Event::Tick,
        };
        let reply = match event {
            Event::Incoming(Some(Ok(Message::Text(text)))) => {
                let actor = binding.actor();
                call(app.clone(), move |runtime| {
                    runtime.room_receive(actor, text.to_string())
                })
                .await
            }
            Event::Incoming(Some(Ok(Message::Binary(bytes)))) => {
                let actor = binding.actor();
                if call(app.clone(), move |runtime| {
                    runtime.room_packet(actor, bytes.to_vec())
                })
                .await
                .is_err()
                {
                    rejected = true;
                    break;
                }
                continue;
            }
            Event::Incoming(Some(Ok(Message::Ping(_) | Message::Pong(_)))) => continue,
            Event::Incoming(Some(Err(_))) => {
                rejected = true;
                break;
            }
            Event::Incoming(_) | Event::Queued(None) => break,
            Event::Queued(Some(queued)) => {
                let outgoing = match &queued.message {
                    RoomOutbound::Text(text) => Message::Text(text.clone().into()),
                    RoomOutbound::Packet { bytes, .. } => {
                        let actor = binding.actor();
                        let packet = queued.message.clone();
                        match call(app.clone(), move |runtime| {
                            runtime.room_delivery(actor, &packet)
                        })
                        .await
                        {
                            Ok(true) => Message::Binary(bytes.clone().into()),
                            Ok(false) => continue,
                            Err(_) => {
                                rejected = true;
                                break;
                            }
                        }
                    }
                };
                if !send(&mut socket, outgoing).await {
                    io_failure = true;
                    break;
                }
                continue;
            }
            Event::Tick => {
                let actor = binding.actor();
                call(app.clone(), move |runtime| runtime.room_tick(actor)).await
            }
        };
        let reply = match reply {
            Ok(reply) => reply,
            Err(_) => {
                rejected = true;
                break;
            }
        };
        for text in reply.text {
            if !send(&mut socket, Message::Text(text.into())).await {
                io_failure = true;
                break;
            }
        }
        if io_failure || reply.stop {
            break;
        }
    }
    if io_failure {
        let actor = binding.actor();
        let _ = call(app.clone(), move |runtime| runtime.room_io_failure(actor)).await;
    } else if rejected {
        let actor = binding.actor();
        let _ = call(app.clone(), move |runtime| runtime.room_rejected(actor)).await;
    }
    binding.close();
    let actor = binding.actor();
    let _ = call(app, move |runtime| runtime.room_detach(actor)).await;
    let (code, reason) = binding.reason();
    let _ = send(
        &mut socket,
        Message::Close(Some(CloseFrame {
            code,
            reason: reason.into(),
        })),
    )
    .await;
}
