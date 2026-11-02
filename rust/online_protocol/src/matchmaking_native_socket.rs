//! Shared bounded socket writes. An AGAIN observation never acknowledges bytes.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    Invalid,
    Io,
    Cancelled,
    Timeout,
}
pub type Result<T> = std::result::Result<T, Error>;
pub struct Sent {
    pub again: bool,
    pub count: usize,
}

pub fn write_all(
    bytes: &[u8],
    now: impl Fn() -> u64,
    cancelled: impl Fn() -> bool,
    wait: impl Fn(u32),
    send: impl Fn(&[u8]) -> Result<Sent>,
) -> Result<()> {
    let start = now();
    let mut offset = 0;
    loop {
        if cancelled() {
            return Err(Error::Cancelled);
        }
        let sent = send(&bytes[offset..])?;
        if sent.count > bytes.len() - offset || (sent.again && sent.count != 0) {
            return Err(Error::Io);
        }
        if !sent.again {
            offset += sent.count;
            if offset == bytes.len() {
                return Ok(());
            }
        }
        if now().checked_sub(start).ok_or(Error::Io)? >= 500 {
            return Err(Error::Timeout);
        }
        wait(2);
    }
}
