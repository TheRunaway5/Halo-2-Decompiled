//! Private, bounded SDK-worker messages. These are never room/network packets.
use serde::{Deserialize, Serialize, de::DeserializeOwned};
use std::io::{self, Read, Write};

pub(crate) const VERSION: u16 = 1;
// A linked executable can emit bytes before main redirects ordinary stdout.
// Only startup permits a bounded preamble; every subsequent frame is strict.
pub(crate) const MAX_STARTUP_PREAMBLE_BYTES: usize = 4096;
const STARTUP_MAGIC: &[u8; 16] = b"\x89H2-EOS-SDK-IPC\0";
const STARTUP_TAIL: &[u8; 4] = b"\r\n\0!";
// JSON byte arrays expand by up to four times. A poll can contain512 data
// packets plus512 callback events; this cap also bounds malformed input.
pub(crate) const MAX_FRAME_BYTES: usize = 8 * 1024 * 1024;
pub(crate) const MAX_HANDLES: usize = 32;
pub(crate) const MAX_EVENTS: usize = 1024;
pub(crate) const MAX_PACKET_BYTES: usize = 1170;

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct Envelope<T> {
    pub version: u16,
    pub sequence: u64,
    pub body: T,
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct Config {
    pub product: String,
    pub sandbox: String,
    pub deployment: String,
    pub client: String,
    pub secret: String,
    pub cache: String,
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct Session {
    pub enabled: bool,
    pub playlist: String,
    pub revision: String,
    pub map: String,
    pub variant: String,
}

#[derive(Serialize, Deserialize)]
#[serde(tag = "kind", content = "value", rename_all = "snake_case")]
#[serde(deny_unknown_fields)]
pub(crate) enum Request {
    Hello,
    Create(Config),
    Call { handle: u64, call: Call },
    Destroy { handle: u64 },
    Shutdown,
}

#[derive(Serialize, Deserialize)]
#[serde(tag = "kind", content = "value", rename_all = "snake_case")]
#[serde(deny_unknown_fields)]
pub(crate) enum Call {
    Device,
    Login,
    ExternalLogin {
        token: String,
    },
    IdToken,
    CreateUser,
    Socket {
        name: String,
        force: bool,
    },
    Poll,
    Accept {
        user: String,
    },
    Send {
        user: String,
        bytes: Vec<u8>,
        delayed: bool,
    },
    QueueInfo,
    SessionCreate {
        session: Session,
        bucket: String,
        capacity: u32,
    },
    SessionInvite {
        user: String,
    },
    SessionQuery,
    SessionRegister {
        user: String,
    },
    SessionInfo {
        invite: String,
    },
    SessionJoin {
        invite: String,
    },
}

#[derive(Serialize, Deserialize)]
#[serde(tag = "kind", content = "value", rename_all = "snake_case")]
#[serde(deny_unknown_fields)]
pub(crate) enum Reply {
    Hello,
    Handle(u64),
    Unit,
    Bool(bool),
    Result(i32),
    Text(String),
    Events(Vec<Event>),
    Queue(QueueInfo),
    Invite(InviteInfo),
    // Never forward SDK exception strings, credentials, or malformed input.
    Failed,
}

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct Event {
    pub kind: u8,
    pub code: i32,
    pub user: String,
    pub socket: String,
    pub data: Vec<u8>,
}

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct QueueInfo {
    pub result: i32,
    pub incoming_bytes: u64,
    pub incoming_packets: u64,
    pub outgoing_bytes: u64,
    pub outgoing_packets: u64,
}

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct InviteInfo {
    pub session: String,
    pub owner: String,
    pub playlist: String,
    pub revision: String,
    pub map: String,
    pub variant: String,
}

fn bounded(value: &str, maximum: usize) -> bool {
    value.len() <= maximum && !value.contains('\0')
}

impl Request {
    pub fn accepts(&self, reply: &Reply) -> bool {
        if matches!(reply, Reply::Failed) {
            return true;
        }
        match self {
            Self::Hello => matches!(reply, Reply::Hello),
            Self::Create(_) => matches!(reply, Reply::Handle(_)),
            Self::Destroy { .. } => matches!(reply, Reply::Unit),
            Self::Shutdown => matches!(reply, Reply::Bool(_)),
            Self::Call { call, .. } => match call {
                Call::IdToken => matches!(reply, Reply::Text(_)),
                Call::Poll => matches!(reply, Reply::Events(_)),
                Call::Accept { .. } => matches!(reply, Reply::Bool(_)),
                Call::Send { .. } => matches!(reply, Reply::Result(_)),
                Call::QueueInfo => matches!(reply, Reply::Queue(_)),
                Call::SessionInfo { .. } => matches!(reply, Reply::Invite(_)),
                _ => matches!(reply, Reply::Unit),
            },
        }
    }

    pub fn valid(&self) -> bool {
        match self {
            Self::Hello | Self::Shutdown => true,
            Self::Create(config) => {
                [&config.product, &config.sandbox, &config.deployment]
                    .iter()
                    .all(|id| {
                        id.len() == 32
                            && id
                                .bytes()
                                .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
                    })
                    && [&config.client, &config.secret].iter().all(|text| {
                        !text.is_empty()
                            && bounded(text, 4096)
                            && !text.bytes().any(|b| b < 32 || b == 127)
                    })
                    && bounded(&config.cache, 16384)
                    && std::path::Path::new(&config.cache).is_absolute()
            }
            Self::Call { handle, call } => *handle != 0 && call.valid(),
            Self::Destroy { handle } => *handle != 0,
        }
    }
}

impl Call {
    fn valid(&self) -> bool {
        match self {
            Self::ExternalLogin { token } => bounded(token, 8192),
            Self::Socket { name, .. } => bounded(name, 32),
            Self::Accept { user }
            | Self::SessionInvite { user }
            | Self::SessionRegister { user } => bounded(user, 64),
            Self::Send { user, bytes, .. } => bounded(user, 64) && bytes.len() <= MAX_PACKET_BYTES,
            Self::SessionCreate {
                session, bucket, ..
            } => {
                bounded(bucket, 64)
                    && [
                        &session.playlist,
                        &session.revision,
                        &session.map,
                        &session.variant,
                    ]
                    .iter()
                    .all(|text| bounded(text, 64))
            }
            Self::SessionInfo { invite } | Self::SessionJoin { invite } => bounded(invite, 256),
            _ => true,
        }
    }
}

impl Reply {
    pub fn valid(&self) -> bool {
        match self {
            Self::Handle(handle) => *handle != 0,
            Self::Text(text) => bounded(text, 8192),
            Self::Events(events) => {
                events.len() <= MAX_EVENTS
                    && events.iter().all(|event| {
                        bounded(&event.user, 64)
                            && bounded(&event.socket, 256)
                            && event.data.len() <= MAX_PACKET_BYTES
                    })
            }
            Self::Invite(info) => {
                bounded(&info.session, 256)
                    && bounded(&info.owner, 64)
                    && [&info.playlist, &info.revision, &info.map, &info.variant]
                        .iter()
                        .all(|text| bounded(text, 4096))
            }
            _ => true,
        }
    }
}

fn invalid() -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, "Invalid EOS worker frame")
}

pub(crate) fn write_startup(writer: &mut impl Write) -> io::Result<()> {
    writer.write_all(STARTUP_MAGIC)?;
    writer.write_all(&VERSION.to_be_bytes())?;
    writer.write_all(STARTUP_TAIL)?;
    writer.flush()
}

pub(crate) fn read_startup(reader: &mut impl Read) -> io::Result<usize> {
    let mut window = [0; STARTUP_MAGIC.len()];
    for consumed in 1..=MAX_STARTUP_PREAMBLE_BYTES + STARTUP_MAGIC.len() {
        let mut byte = [0];
        reader.read_exact(&mut byte).map_err(|_| invalid())?;
        window.rotate_left(1);
        window[STARTUP_MAGIC.len() - 1] = byte[0];
        if consumed >= STARTUP_MAGIC.len() && &window == STARTUP_MAGIC {
            let mut suffix = [0; 2 + STARTUP_TAIL.len()];
            reader.read_exact(&mut suffix).map_err(|_| invalid())?;
            if suffix[..2] != VERSION.to_be_bytes() || &suffix[2..] != STARTUP_TAIL {
                // A recognized marker with an invalid suffix never resyncs.
                return Err(invalid());
            }
            return Ok(consumed - STARTUP_MAGIC.len());
        }
    }
    Err(invalid())
}

pub(crate) fn encode<T: Serialize>(value: &T) -> io::Result<Vec<u8>> {
    let bytes = serde_json::to_vec(value).map_err(|_| invalid())?;
    if bytes.is_empty() || bytes.len() > MAX_FRAME_BYTES {
        return Err(invalid());
    }
    Ok(bytes)
}

pub(crate) fn write_frame(writer: &mut impl Write, bytes: &[u8]) -> io::Result<()> {
    if bytes.is_empty() || bytes.len() > MAX_FRAME_BYTES {
        return Err(invalid());
    }
    writer.write_all(&(bytes.len() as u32).to_be_bytes())?;
    writer.write_all(bytes)?;
    writer.flush()
}

pub(crate) fn read_frame<T: DeserializeOwned>(reader: &mut impl Read) -> io::Result<Option<T>> {
    let mut header = [0; 4];
    let mut filled = 0;
    while filled < header.len() {
        match reader.read(&mut header[filled..]) {
            Ok(0) if filled == 0 => return Ok(None),
            Ok(0) => return Err(invalid()),
            Ok(count) => filled += count,
            Err(error) if error.kind() == io::ErrorKind::Interrupted => {}
            Err(_) => return Err(invalid()),
        }
    }
    let length = u32::from_be_bytes(header) as usize;
    if length == 0 || length > MAX_FRAME_BYTES {
        return Err(invalid());
    }
    let mut bytes = vec![0; length];
    reader.read_exact(&mut bytes).map_err(|_| invalid())?;
    // serde structs also accept positional arrays; this protocol requires objects.
    if bytes.iter().find(|byte| !byte.is_ascii_whitespace()) != Some(&b'{') {
        return Err(invalid());
    }
    serde_json::from_slice(&bytes)
        .map(Some)
        .map_err(|_| invalid())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    fn framed(bytes: &[u8]) -> Vec<u8> {
        let mut result = Vec::new();
        write_frame(&mut result, bytes).unwrap();
        result
    }

    #[test]
    fn startup_accepts_only_bounded_preamble_without_consuming_the_first_frame() {
        for prefix in [0, 30, MAX_STARTUP_PREAMBLE_BYTES] {
            let mut bytes = vec![b'x'; prefix];
            write_startup(&mut bytes).unwrap();
            write_frame(
                &mut bytes,
                &encode(&Envelope {
                    version: VERSION,
                    sequence: 1,
                    body: Reply::Hello,
                })
                .unwrap(),
            )
            .unwrap();
            let mut reader = Cursor::new(bytes);
            assert_eq!(read_startup(&mut reader).unwrap(), prefix);
            let reply: Envelope<Reply> = read_frame(&mut reader).unwrap().unwrap();
            assert_eq!(reply.sequence, 1);
            assert!(matches!(reply.body, Reply::Hello));
        }
    }

    #[test]
    fn startup_rejects_missing_overlimit_truncated_or_malformed_markers() {
        let mut marker = vec![];
        write_startup(&mut marker).unwrap();
        let mut oversized = vec![b'x'; MAX_STARTUP_PREAMBLE_BYTES + 1];
        oversized.extend_from_slice(&marker);
        for bytes in [
            vec![],
            vec![b'x'; MAX_STARTUP_PREAMBLE_BYTES + marker.len()],
            oversized,
            marker[..STARTUP_MAGIC.len() - 1].to_vec(),
            marker[..marker.len() - 1].to_vec(),
        ] {
            assert!(read_startup(&mut Cursor::new(bytes)).is_err());
        }
        for index in [STARTUP_MAGIC.len(), marker.len() - 1] {
            let mut malformed = marker.clone();
            malformed[index] ^= 1;
            // A later valid marker cannot hide a recognized invalid version/tail.
            malformed.extend_from_slice(&marker);
            assert!(read_startup(&mut Cursor::new(malformed)).is_err());
        }
    }

    #[test]
    fn startup_does_not_relax_subsequent_frame_parsing() {
        let mut bytes = vec![];
        write_startup(&mut bytes).unwrap();
        write_startup(&mut bytes).unwrap();
        let mut reader = Cursor::new(bytes);
        read_startup(&mut reader).unwrap();
        assert!(read_frame::<Envelope<Reply>>(&mut reader).is_err());
    }

    #[test]
    fn framing_rejects_partial_zero_oversized_and_non_object_messages() {
        for bytes in [
            vec![0],
            vec![0, 0, 0],
            vec![0; 4],
            ((MAX_FRAME_BYTES + 1) as u32).to_be_bytes().to_vec(),
            framed(b"[1,1,\"hello\"]"),
            framed(b"{\"version\":1"),
        ] {
            assert!(read_frame::<Envelope<Request>>(&mut Cursor::new(bytes)).is_err());
        }
        assert!(
            read_frame::<Envelope<Request>>(&mut Cursor::new(Vec::<u8>::new()))
                .unwrap()
                .is_none()
        );
        let mut bytes = framed(br#"{"version":1,"sequence":1,"body":{"kind":"hello"}}"#);
        bytes.pop();
        assert!(read_frame::<Envelope<Request>>(&mut Cursor::new(bytes)).is_err());
    }

    #[test]
    fn framing_rejects_unknown_and_duplicate_fields_and_trailing_values() {
        for bytes in [
            br#"{"version":1,"version":1,"sequence":1,"body":{"kind":"hello"}}"#.as_slice(),
            br#"{"version":1,"sequence":1,"body":{"kind":"hello"},"extra":0}"#.as_slice(),
            br#"{"version":1,"sequence":1,"body":{"kind":"hello"}} {}"#.as_slice(),
        ] {
            assert!(read_frame::<Envelope<Request>>(&mut Cursor::new(framed(bytes))).is_err());
        }
    }

    #[test]
    fn raw_send_result_and_large_poll_round_trip_without_text_errors() {
        let value = Envelope {
            version: VERSION,
            sequence: 27,
            body: Reply::Result(22),
        };
        let bytes = framed(&encode(&value).unwrap());
        let decoded: Envelope<Reply> = read_frame(&mut Cursor::new(bytes)).unwrap().unwrap();
        assert!(matches!(decoded.body, Reply::Result(22)));
        assert_eq!(decoded.sequence, 27);
        let events = (0..512)
            .map(|_| Event {
                kind: 8,
                code: 0,
                user: "a".repeat(32),
                socket: "s".repeat(32),
                data: vec![255; MAX_PACKET_BYTES],
            })
            .collect();
        let events = Reply::Events(events);
        assert!(events.valid());
        assert!(
            encode(&Envelope {
                version: VERSION,
                sequence: 28,
                body: events
            })
            .is_ok()
        );
        assert_eq!(encode(&Reply::Failed).unwrap(), br#"{"kind":"failed"}"#);
    }

    #[test]
    fn semantic_bounds_reject_oversized_fields_and_zero_handles() {
        assert!(!Request::Destroy { handle: 0 }.valid());
        assert!(
            !Request::Call {
                handle: 1,
                call: Call::ExternalLogin {
                    token: "x".repeat(8193)
                }
            }
            .valid()
        );
        assert!(
            !Request::Call {
                handle: 1,
                call: Call::Send {
                    user: String::new(),
                    bytes: vec![0; MAX_PACKET_BYTES + 1],
                    delayed: false
                }
            }
            .valid()
        );
        assert!(
            !Reply::Events(
                (0..MAX_EVENTS + 1)
                    .map(|_| Event {
                        kind: 8,
                        code: 0,
                        user: String::new(),
                        socket: String::new(),
                        data: vec![]
                    })
                    .collect()
            )
            .valid()
        );
    }
}
