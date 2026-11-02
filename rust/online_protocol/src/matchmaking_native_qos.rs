//! Complete native search challenge worker; C++ supplies socket IO only.
//! Echoes authenticated challenges. It never asserts ranks, RTT or bandwidth.
//! This replacement-service wire protocol is not recovered original XNet/Live.
use crate::{
    matchmaking_json,
    matchmaking_origin::{UrlParts, pinned_origin},
};

const TEXT_LIMIT: usize = 256;
const IDLE_MS: u64 = 20_000;
const RECONNECT_MS: u64 = 1000;

pub use crate::matchmaking_native_socket::{Error, Result, Sent};

#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub text: bool,
    pub binary: bool,
    pub ping: bool,
    pub pong: bool,
    pub close: bool,
    pub continued: bool,
    pub bytes_left: i64,
    pub data: Vec<u8>,
}

pub enum Receive {
    Again,
    Frame(Frame),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SendKind {
    Text,
    Pong,
}

pub struct Open<'a> {
    pub url: &'a str,
    pub bearer: &'a str,
    pub connect_ms: u32,
    pub timeout_ms: u32,
}

pub trait Io {
    fn now_ms(&self) -> u64;
    fn cancelled(&self) -> bool;
    /// Actual cancellable scheduler wait, never a supplied search clock.
    fn wait(&self, milliseconds: u32);
    fn open(&self, options: Open<'_>) -> Result<()>;
    fn receive(&self) -> Result<Receive>;
    fn send(&self, kind: SendKind, data: &[u8]) -> Result<Sent>;
    fn close(&self);
}

pub struct Worker {
    url: String,
    bearer: String,
}
impl Worker {
    pub fn new(
        input_valid: bool,
        origin: &str,
        parts: &UrlParts,
        bearer: &str,
        loopback: bool,
    ) -> Result<Self> {
        if !input_valid
            || !(32..=128).contains(&bearer.len())
            || !bearer
                .bytes()
                .all(|v| v.is_ascii_alphanumeric() || v == b'-' || v == b'_')
        {
            return Err(Error::Invalid);
        }
        let origin = pinned_origin(origin, parts, loopback).map_err(|_| Error::Invalid)?;
        let (_, rest) = origin.split_once(':').ok_or(Error::Invalid)?;
        let scheme = if parts.scheme == "https" { "wss" } else { "ws" };
        Ok(Self {
            url: format!("{scheme}:{rest}/v1/matchmaking/qos"),
            bearer: bearer.into(),
        })
    }

    pub fn run(&self, io: &impl Io) {
        while !io.cancelled() {
            let _ = self.connection(io);
            // Closing the actual socket invalidates the service's old samples.
            io.close();
            if Self::delay(io, RECONNECT_MS, 10).is_err() {
                break;
            }
        }
    }

    fn elapsed(io: &impl Io, start: u64) -> Result<u64> {
        io.now_ms().checked_sub(start).ok_or(Error::Io)
    }

    fn delay(io: &impl Io, milliseconds: u64, step: u32) -> Result<()> {
        let start = io.now_ms();
        while Self::elapsed(io, start)? < milliseconds {
            if io.cancelled() {
                return Err(Error::Cancelled);
            }
            io.wait(step);
        }
        if io.cancelled() {
            Err(Error::Cancelled)
        } else {
            Ok(())
        }
    }

    fn send(io: &impl Io, kind: SendKind, data: &[u8]) -> Result<()> {
        crate::matchmaking_native_socket::write_all(
            data,
            || io.now_ms(),
            || io.cancelled(),
            |ms| io.wait(ms),
            |bytes| io.send(kind, bytes),
        )
    }

    fn connection(&self, io: &impl Io) -> Result<()> {
        io.open(Open {
            url: &self.url,
            bearer: &self.bearer,
            connect_ms: 2000,
            timeout_ms: 3000,
        })?;
        let mut last = io.now_ms();
        let mut text = Vec::new();
        loop {
            if io.cancelled() {
                return Err(Error::Cancelled);
            }
            let frame = match io.receive()? {
                Receive::Again => {
                    if Self::elapsed(io, last)? >= IDLE_MS {
                        return Err(Error::Timeout);
                    }
                    io.wait(2);
                    continue;
                }
                Receive::Frame(frame) => frame,
            };
            if frame.close || frame.bytes_left < 0 {
                return Err(Error::Invalid);
            }
            last = io.now_ms();
            let kinds = [
                frame.text,
                frame.binary,
                frame.ping,
                frame.pong,
                frame.close,
            ]
            .into_iter()
            .filter(|v| *v)
            .count();
            if kinds != 1 {
                return Err(Error::Invalid);
            }
            if frame.ping {
                if frame.data.len() > 125 || frame.bytes_left != 0 || frame.continued {
                    return Err(Error::Invalid);
                }
                Self::send(io, SendKind::Pong, &frame.data)?;
                continue;
            }
            if frame.pong {
                if frame.data.len() > 125 || frame.bytes_left != 0 || frame.continued {
                    return Err(Error::Invalid);
                }
                continue;
            }
            if !frame.text || text.len().saturating_add(frame.data.len()) > TEXT_LIMIT {
                return Err(Error::Invalid);
            }
            text.extend_from_slice(&frame.data);
            if frame.bytes_left != 0 || frame.continued {
                continue;
            }
            let value = matchmaking_json::parse(&text, TEXT_LIMIT).map_err(|_| Error::Invalid)?;
            text.clear();
            let object = value.as_object().ok_or(Error::Invalid)?;
            if object.len() != 1 {
                return Err(Error::Invalid);
            }
            let (kind, value) = object.iter().next().ok_or(Error::Invalid)?;
            let reply = match kind.as_str() {
                "ping" => "pong",
                "probe" => "echo",
                _ => return Err(Error::Invalid),
            };
            let nonce = value.as_str().ok_or(Error::Invalid)?;
            if nonce.len() != 32
                || !nonce
                    .bytes()
                    .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v))
            {
                return Err(Error::Invalid);
            }
            let reply = format!("{{\"{reply}\":\"{nonce}\"}}");
            Self::send(io, SendKind::Text, reply.as_bytes())?;
        }
    }
}
