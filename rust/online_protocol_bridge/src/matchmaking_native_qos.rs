//! Typed native socket observations into the complete Rust search worker.
use crate::ffi;
use halo2_online_protocol::{matchmaking_native_qos as core, matchmaking_origin::UrlParts};

pub struct MatchmakingQos(core::Worker);

pub fn new_matchmaking_qos(
    c: &ffi::MatchmakingQosConfig,
) -> Result<Box<MatchmakingQos>, &'static str> {
    let parts = UrlParts {
        parsed: c.url_parsed,
        normalized: c.url_normalized.clone(),
        scheme: c.url_scheme.clone(),
        host: c.url_host.clone(),
        user: c.url_user.clone(),
        password: c.url_password.clone(),
        options: c.url_options.clone(),
        path: c.url_path.clone(),
    };
    core::Worker::new(c.input_valid, &c.origin, &parts, &c.bearer, c.loopback)
        .map(|v| Box::new(MatchmakingQos(v)))
        .map_err(|_| "Search connection configuration is invalid")
}

struct Io<'a>(&'a ffi::NativeMatchmakingQosIo);
impl core::Io for Io<'_> {
    fn now_ms(&self) -> u64 {
        self.0.qos_now_ms()
    }
    fn cancelled(&self) -> bool {
        self.0.qos_cancelled()
    }
    fn wait(&self, milliseconds: u32) {
        self.0.qos_wait(milliseconds);
    }
    fn open(&self, o: core::Open<'_>) -> core::Result<()> {
        self.0
            .qos_open(&ffi::MatchmakingQosOpen {
                url: o.url.into(),
                bearer: o.bearer.into(),
                connect_ms: o.connect_ms,
                timeout_ms: o.timeout_ms,
            })
            .map_err(|_| core::Error::Io)
    }
    fn receive(&self) -> core::Result<core::Receive> {
        let f = self.0.qos_receive().map_err(|_| core::Error::Io)?;
        if f.again {
            if !f.data.is_empty()
                || f.text
                || f.binary
                || f.ping
                || f.pong
                || f.close
                || f.continued
                || f.bytes_left != 0
            {
                return Err(core::Error::Io);
            }
            return Ok(core::Receive::Again);
        }
        Ok(core::Receive::Frame(core::Frame {
            text: f.text,
            binary: f.binary,
            ping: f.ping,
            pong: f.pong,
            close: f.close,
            continued: f.continued,
            bytes_left: f.bytes_left,
            data: f.data,
        }))
    }
    fn send(&self, kind: core::SendKind, data: &[u8]) -> core::Result<core::Sent> {
        let kind = match kind {
            core::SendKind::Text => ffi::MatchmakingQosSendKind::Text,
            core::SendKind::Pong => ffi::MatchmakingQosSendKind::Pong,
        };
        let s = self.0.qos_send(kind, data).map_err(|_| core::Error::Io)?;
        Ok(core::Sent {
            again: s.again,
            count: s.count,
        })
    }
    fn close(&self) {
        self.0.qos_close();
    }
}

pub fn matchmaking_qos_run(qos: &MatchmakingQos, io: &ffi::NativeMatchmakingQosIo) {
    qos.0.run(&Io(io));
}
