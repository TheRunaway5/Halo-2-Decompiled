//! Native room socket scheduling, framing and reconnection. No world is created here.
use crate::{
    matchmaking_native_qos::{Frame, Open, Receive},
    matchmaking_native_socket::{self, Error, Result, Sent},
};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SendKind {
    Text,
    Binary,
    Pong,
    Close,
}
pub struct Config {
    pub url: String,
    pub bearer: String,
    pub direct: bool,
    pub continuity: bool,
}
#[derive(Default)]
pub struct Flush {
    pub loaded: Option<String>,
    pub completion: Option<String>,
    pub confirmation: Option<String>,
    pub commands: Vec<String>,
    pub packets: Vec<crate::matchmaking_native_session::Packet>,
}
pub trait Io {
    fn now_ms(&self) -> u64;
    fn cancelled(&self) -> bool;
    fn wait(&self, milliseconds: u32);
    fn open(&self, options: Open<'_>) -> Result<()>;
    fn receive(&self) -> Result<Receive>;
    fn send(&self, kind: SendKind, bytes: &[u8]) -> Result<Sent>;
    fn close(&self);
    /// These callbacks acquire the actual native generation lock before the
    /// Rust state is observed or mutated. Native effects run under that lock.
    fn text(&self, bytes: &[u8]) -> Result<Option<String>>;
    fn datagram(&self, peer: u8, epoch: u32, bytes: &[u8]) -> Result<()>;
    fn pump(&self);
    fn flush(&self) -> Result<Flush>;
    fn packet_current(&self, packet: &crate::matchmaking_native_session::Packet) -> bool;
    fn failure(&self, terminal: bool, message: &str) -> bool;
}
struct Failure {
    terminal: bool,
    message: String,
}
impl From<Error> for Failure {
    fn from(_: Error) -> Self {
        Self {
            terminal: false,
            message: "ONLINE match connection failed or ended. Search again.".into(),
        }
    }
}
pub struct Worker(Config);
impl Worker {
    /// Configuration is admitted only by the authoritative session constructor.
    pub fn new(config: Config) -> Self {
        Self(config)
    }
    fn send(io: &impl Io, kind: SendKind, bytes: &[u8], shutdown: bool) -> Result<()> {
        matchmaking_native_socket::write_all(
            bytes,
            || io.now_ms(),
            || !shutdown && io.cancelled(),
            |ms| io.wait(ms),
            |bytes| io.send(kind, bytes),
        )
    }
    pub fn run(&self, io: &impl Io) {
        while !io.cancelled() {
            let result = self.connection(io);
            io.close();
            if io.cancelled() {
                break;
            }
            if let Err(failure) = result {
                if !io.failure(failure.terminal, &failure.message) {
                    break;
                }
            }
            let start = io.now_ms();
            loop {
                if io.cancelled() {
                    return;
                }
                let Some(elapsed) = io.now_ms().checked_sub(start) else {
                    io.failure(false, "ONLINE socket clock moved backwards.");
                    return;
                };
                if elapsed >= 1000 {
                    break;
                }
                io.wait(10);
            }
        }
    }
    fn connection(&self, io: &impl Io) -> std::result::Result<(), Failure> {
        io.open(Open {
            url: &self.0.url,
            bearer: &self.0.bearer,
            connect_ms: 2000,
            timeout_ms: 3000,
        })?;
        let mut last = io.now_ms();
        let mut assembled = Vec::new();
        let mut message_text = None;
        let (mut loaded, mut completed, mut confirmed) = (false, false, false);
        while !io.cancelled() {
            for _ in 0..256 {
                if io.cancelled() {
                    break;
                }
                let frame = match io.receive()? {
                    Receive::Again => break,
                    Receive::Frame(frame) => frame,
                };
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
                if kinds != 1 || frame.bytes_left < 0 {
                    return Err(Error::Invalid.into());
                }
                last = io.now_ms();
                if frame.ping || frame.pong || frame.close {
                    Self::control_frame(&frame)?;
                    if frame.close {
                        if frame.data.len() == 1 {
                            return Err(Error::Invalid.into());
                        }
                        let code = if frame.data.len() >= 2 {
                            u16::from_be_bytes([frame.data[0], frame.data[1]])
                        } else {
                            1000
                        };
                        // Preserve the service's bounded reason; never decode unchecked UTF-8.
                        let reason = if frame.data.len() > 2 && frame.data.len() <= 122 {
                            std::str::from_utf8(&frame.data[2..])
                                .map_err(|_| Error::Invalid)?
                                .to_owned()
                        } else {
                            "ONLINE match ended.".into()
                        };
                        return Err(Failure {
                            terminal: self.0.direct
                                && code != 1001
                                && reason != "Match service is restarting.",
                            message: reason,
                        });
                    }
                    if frame.ping {
                        Self::send(io, SendKind::Pong, &frame.data, false)?;
                    }
                    continue;
                }
                if assembled
                    .len()
                    .saturating_add(frame.data.len())
                    .saturating_add(frame.bytes_left as usize)
                    > crate::matchmaking_native_session::MESSAGE_BYTES
                    || message_text.is_some_and(|text| text != frame.text)
                {
                    return Err(Error::Invalid.into());
                }
                message_text = Some(frame.text);
                assembled.extend_from_slice(&frame.data);
                if frame.bytes_left != 0 || frame.continued {
                    continue;
                }
                if frame.text {
                    if let Some(reply) = io.text(&assembled)? {
                        Self::send(io, SendKind::Text, reply.as_bytes(), false)?;
                    }
                } else {
                    let offset = if self.0.continuity { 5 } else { 1 };
                    if self.0.direct || !(offset + 7..=offset + 1400).contains(&assembled.len()) {
                        return Err(Error::Invalid.into());
                    }
                    let epoch = if self.0.continuity {
                        u32::from_le_bytes(assembled[1..5].try_into().map_err(|_| Error::Invalid)?)
                    } else {
                        0
                    };
                    io.datagram(assembled[0], epoch, &assembled[offset..])?;
                }
                assembled.clear();
                message_text = None;
            }
            if io.now_ms().checked_sub(last).ok_or(Error::Io)? >= 20_000 {
                return Err(Error::Timeout.into());
            }
            io.pump();
            let flush = io.flush()?;
            if !loaded {
                if let Some(value) = flush.loaded {
                    let text = serde_json::json!({"loaded":value}).to_string();
                    Self::send(io, SendKind::Text, text.as_bytes(), false)?;
                    loaded = true;
                }
            }
            for command in flush.commands {
                Self::send(io, SendKind::Text, command.as_bytes(), false)?;
            }
            for packet in flush.packets {
                if self.0.direct {
                    return Err(Error::Invalid.into());
                }
                if !io.packet_current(&packet) {
                    continue;
                }
                let mut bytes = vec![packet.peer];
                if self.0.continuity {
                    bytes.extend_from_slice(&packet.epoch.to_le_bytes());
                }
                bytes.extend_from_slice(&packet.bytes);
                Self::send(io, SendKind::Binary, &bytes, false)?;
            }
            if !completed {
                if let Some(value) = flush.completion {
                    Self::send(io, SendKind::Text, value.as_bytes(), false)?;
                    completed = true;
                }
            }
            if !confirmed {
                if let Some(value) = flush.confirmation {
                    Self::send(io, SendKind::Text, value.as_bytes(), false)?;
                    confirmed = true;
                }
            }
            io.wait(2);
        }
        if self.0.direct {
            let _ = Self::send(io, SendKind::Text, b"{\"leave\":true}", true);
        }
        let _ = Self::send(io, SendKind::Close, &[], true);
        Ok(())
    }
    fn control_frame(frame: &Frame) -> Result<()> {
        if frame.data.len() > 125 || frame.bytes_left != 0 || frame.continued {
            Err(Error::Invalid)
        } else {
            Ok(())
        }
    }
}
