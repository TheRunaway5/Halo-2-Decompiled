//! Quantized skeletal poses for presentation replication.
//!
//! The native WORLD pose bank carries a full 52-byte matrix per node. Here a
//! rigid node becomes a smallest-three quaternion (1/4096 per component) and a
//! position relative to the body's sphere center (1/1024 world unit). Nodes
//! that are not rigid rotations keep their raw float words. Poses are visual
//! state only: the replicated view is excluded from snapshot digests.

use super::range::{Bit, Decoder, Encoder, Error, Int};

pub const NODE_BYTES: usize = 52;
const POS_SCALE: f32 = 1024.0;
const ROT_SCALE: f32 = 4096.0;
const ROT_LIMIT: i32 = 2896; // 4096 / sqrt(2)
const COORD_LIMIT: f32 = (1u32 << 30) as f32;
const MAX_NODES: usize = 0x7fff / 0x34;
const MAX_BODIES: usize = 2048;

#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug, PartialOrd, Ord)]
pub struct Key {
    pub object: u32,
    pub nodes: u16,
}

pub struct RawPose {
    pub key: Key,
    pub sphere: [f32; 4],
    pub nodes: Vec<[f32; 13]>,
}

pub struct RawBank {
    pub authoritative: u8,
    pub poses: Vec<RawPose>,
}

fn f32_at(bytes: &[u8], at: usize) -> f32 {
    f32::from_le_bytes(bytes[at..at + 4].try_into().expect("float"))
}

/// Parses the native pose bank wire layout; `None` leaves it generic.
pub fn parse(bytes: &[u8]) -> Option<RawBank> {
    let authoritative = *bytes.first()?;
    let count = usize::from(u16::from_le_bytes(bytes.get(1..3)?.try_into().ok()?));
    if count > MAX_BODIES || authoritative > 1 {
        return None;
    }
    let mut at = 3usize;
    let mut poses = Vec::with_capacity(count);
    for _ in 0..count {
        let object = u32::from_le_bytes(bytes.get(at..at + 4)?.try_into().ok()?);
        let nodes = u16::from_le_bytes(bytes.get(at + 4..at + 6)?.try_into().ok()?);
        at += 6;
        if nodes == 0 || usize::from(nodes) > MAX_NODES {
            return None;
        }
        let end = at.checked_add(16 + usize::from(nodes) * NODE_BYTES)?;
        let body = bytes.get(at..end)?;
        let sphere = std::array::from_fn(|n| f32_at(body, n * 4));
        let nodes_raw = (0..usize::from(nodes))
            .map(|node| std::array::from_fn(|n| f32_at(body, 16 + node * NODE_BYTES + n * 4)))
            .collect();
        if sphere.iter().any(|v| !v.is_finite()) {
            return None;
        }
        poses.push(RawPose { key: Key { object, nodes }, sphere, nodes: nodes_raw });
        at = end;
    }
    (at == bytes.len()).then_some(RawBank { authoritative, poses })
}

/// Float offsets and per-body anchors of a native pose bank starting at
/// `origin`, for lossless generic coding of the whole bank.
pub fn bank_layout(bank: &RawBank, origin: u32) -> (Vec<u32>, Vec<(u32, u64)>) {
    const SECTION: u64 = 0xFE << 40;
    let (mut floats, mut anchors) = (Vec::new(), vec![(origin, SECTION | 0xFFFF_FFFF)]);
    let mut at = origin + 3;
    for pose in &bank.poses {
        anchors.push((at, SECTION | u64::from(pose.key.object)));
        at += 6;
        let words = 4 + u32::from(pose.key.nodes) * (NODE_BYTES as u32 / 4);
        floats.extend((0..words).map(|n| at + n * 4));
        at += words * 4;
    }
    (floats, anchors)
}

pub fn bank_len(keys: &[Key]) -> usize {
    3 + keys.iter().map(|k| 6 + 16 + usize::from(k.nodes) * NODE_BYTES).sum::<usize>()
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Rot {
    Quat { largest: u8, c: [i16; 3] },
    Raw([u32; 9]),
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Node {
    pub scale: u32,
    pub rot: Rot,
    pub pos: [i32; 3],
}

#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Pose {
    pub center: [i32; 3],
    pub radius: u32,
    pub nodes: Vec<Node>,
}

/// Sphere and node 0: enough to carry a held pose along with its body.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Root {
    pub center: [i32; 3],
    pub radius: u32,
    pub rot: Option<(u8, [i16; 3])>,
    pub pos: [i32; 3],
}

impl Pose {
    pub fn root(&self) -> Root {
        let node = &self.nodes[0];
        Root {
            center: self.center,
            radius: self.radius,
            rot: match node.rot {
                Rot::Quat { largest, c } => Some((largest, c)),
                Rot::Raw(_) => None,
            },
            pos: node.pos,
        }
    }

    pub fn world(&self) -> [f32; 3] {
        self.center.map(|v| v as f32 / POS_SCALE)
    }
}

fn quantize_coord(value: f32) -> i32 {
    (value * POS_SCALE).round().clamp(-COORD_LIMIT, COORD_LIMIT) as i32
}

fn dequantize_coord(value: i32) -> f32 {
    value as f32 / POS_SCALE
}

type Matrix = [[f32; 3]; 3];

fn quaternion(m: &Matrix) -> [f32; 4] {
    // Shepperd's method; the encoder alone evaluates it.
    let trace = m[0][0] + m[1][1] + m[2][2];
    let (x, y, z, w);
    if trace > 0.0 {
        let s = (trace + 1.0).sqrt() * 2.0;
        w = 0.25 * s;
        x = (m[2][1] - m[1][2]) / s;
        y = (m[0][2] - m[2][0]) / s;
        z = (m[1][0] - m[0][1]) / s;
    } else if m[0][0] > m[1][1] && m[0][0] > m[2][2] {
        let s = (1.0 + m[0][0] - m[1][1] - m[2][2]).sqrt() * 2.0;
        w = (m[2][1] - m[1][2]) / s;
        x = 0.25 * s;
        y = (m[0][1] + m[1][0]) / s;
        z = (m[0][2] + m[2][0]) / s;
    } else if m[1][1] > m[2][2] {
        let s = (1.0 + m[1][1] - m[0][0] - m[2][2]).sqrt() * 2.0;
        w = (m[0][2] - m[2][0]) / s;
        x = (m[0][1] + m[1][0]) / s;
        y = 0.25 * s;
        z = (m[1][2] + m[2][1]) / s;
    } else {
        let s = (1.0 + m[2][2] - m[0][0] - m[1][1]).sqrt() * 2.0;
        w = (m[1][0] - m[0][1]) / s;
        x = (m[0][2] + m[2][0]) / s;
        y = (m[1][2] + m[2][1]) / s;
        z = 0.25 * s;
    }
    let n = (x * x + y * y + z * z + w * w).sqrt();
    [x / n, y / n, z / n, w / n]
}

fn matrix(q: [f32; 4]) -> Matrix {
    let [x, y, z, w] = q;
    let s = 2.0 / (x * x + y * y + z * z + w * w);
    [
        [1.0 - s * (y * y + z * z), s * (x * y - w * z), s * (x * z + w * y)],
        [s * (x * y + w * z), 1.0 - s * (x * x + z * z), s * (y * z - w * x)],
        [s * (x * z - w * y), s * (y * z + w * x), 1.0 - s * (x * x + y * y)],
    ]
}

fn rigid(m: &Matrix) -> bool {
    let column = |c: usize| [m[0][c], m[1][c], m[2][c]];
    let dot = |a: [f32; 3], b: [f32; 3]| a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    for a in 0..3 {
        for b in a..3 {
            let expected = if a == b { 1.0 } else { 0.0 };
            if (dot(column(a), column(b)) - expected).abs() > 2e-3 {
                return false;
            }
        }
    }
    let (a, b, c) = (column(0), column(1), column(2));
    let det = a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) + a[2] * (b[0] * c[1] - b[1] * c[0]);
    det > 0.0
}

fn encode_quat(m: &Matrix) -> (u8, [i16; 3]) {
    let mut q = quaternion(m);
    let largest = (0..4).max_by(|&a, &b| q[a].abs().total_cmp(&q[b].abs())).unwrap_or(3);
    if q[largest] < 0.0 {
        q = q.map(|v| -v);
    }
    let mut c = [0i16; 3];
    let mut k = 0;
    for (n, &value) in q.iter().enumerate() {
        if n != largest {
            c[k] = ((value * ROT_SCALE).round() as i32).clamp(-ROT_LIMIT, ROT_LIMIT) as i16;
            k += 1;
        }
    }
    (largest as u8, c)
}

fn decode_quat(largest: u8, c: [i16; 3]) -> [f32; 4] {
    let parts = c.map(|v| f32::from(v) / ROT_SCALE);
    let rest = (1.0 - (parts[0] * parts[0] + parts[1] * parts[1] + parts[2] * parts[2])).max(0.0).sqrt();
    let mut q = [0.0f32; 4];
    let mut k = 0;
    for (n, value) in q.iter_mut().enumerate() {
        if n == usize::from(largest) {
            *value = rest;
        } else {
            *value = parts[k];
            k += 1;
        }
    }
    q
}

pub fn quantize(raw: &RawPose) -> Pose {
    let center = [raw.sphere[0], raw.sphere[1], raw.sphere[2]].map(quantize_coord);
    let radius = ((raw.sphere[3].max(0.0) * POS_SCALE).ceil().min(COORD_LIMIT) as u32).saturating_add(1);
    let origin = center.map(dequantize_coord);
    let nodes = raw
        .nodes
        .iter()
        .map(|values| {
            let m: Matrix = std::array::from_fn(|r| std::array::from_fn(|c| values[1 + c * 3 + r]));
            let finite = values.iter().all(|v| v.is_finite());
            let rot = if finite && rigid(&m) {
                let (largest, c) = encode_quat(&m);
                Rot::Quat { largest, c }
            } else {
                Rot::Raw(std::array::from_fn(|n| values[1 + n].to_bits()))
            };
            let pos = std::array::from_fn(|axis| {
                let value = values[10 + axis];
                if value.is_finite() { quantize_coord(value - origin[axis]) } else { 0 }
            });
            Node { scale: values[0].to_bits(), rot, pos }
        })
        .collect();
    Pose { center, radius, nodes }
}

/// Native wire bytes for one entry. `root` re-roots a held full pose onto a
/// newer sphere/node-0 update; the result is presentation-only.
pub fn materialize(key: Key, pose: &Pose, root: Option<&Root>, out: &mut Vec<u8>) {
    let held = pose.root();
    let target = root.copied().unwrap_or(held);
    let center = target.center.map(dequantize_coord);
    out.extend_from_slice(&key.object.to_le_bytes());
    out.extend_from_slice(&key.nodes.to_le_bytes());
    for v in center {
        out.extend_from_slice(&v.to_le_bytes());
    }
    out.extend_from_slice(&(target.radius as f32 / POS_SCALE).to_le_bytes());
    let origin = held.center.map(dequantize_coord);
    let world = |pos: [i32; 3], base: [f32; 3]| std::array::from_fn::<f32, 3, _>(|a| base[a] + dequantize_coord(pos[a]));
    // Rigid motion from the held node 0 to the newer node 0.
    let held0 = world(held.pos, origin);
    let target0 = world(target.pos, center);
    let rotation: Option<Matrix> = match (held.rot, target.rot) {
        (Some((la, ca)), Some((lb, cb))) if root.is_some() && (la, ca) != (lb, cb) => {
            let a = matrix(decode_quat(la, ca));
            let b = matrix(decode_quat(lb, cb));
            // b * a^T
            Some(std::array::from_fn(|r| {
                std::array::from_fn(|c| (0..3).map(|k| b[r][k] * a[c][k]).sum())
            }))
        }
        _ => None,
    };
    let apply = |m: &Option<Matrix>, v: [f32; 3]| match m {
        Some(m) => std::array::from_fn::<f32, 3, _>(|r| (0..3).map(|k| m[r][k] * v[k]).sum()),
        None => v,
    };
    for node in &pose.nodes {
        out.extend_from_slice(&node.scale.to_le_bytes());
        let basis: Matrix = match node.rot {
            Rot::Quat { largest, c } => matrix(decode_quat(largest, c)),
            Rot::Raw(words) => std::array::from_fn(|r| std::array::from_fn(|c| f32::from_bits(words[c * 3 + r]))),
        };
        let raw = matches!(node.rot, Rot::Raw(_));
        for column in (0..3).map(|c| basis.map(|row| row[c])) {
            let column = if raw { column } else { apply(&rotation, column) };
            for v in column {
                out.extend_from_slice(&v.to_le_bytes());
            }
        }
        let position = world(node.pos, origin);
        let relative = std::array::from_fn(|a| position[a] - held0[a]);
        let moved = apply(&rotation, relative);
        for a in 0..3 {
            out.extend_from_slice(&(target0[a] + moved[a]).to_le_bytes());
        }
    }
}

/// Largest component difference; zero means presentation-identical.
pub fn distance(a: &Pose, b: &Pose) -> u64 {
    if a.nodes.len() != b.nodes.len() {
        return u64::MAX;
    }
    let mut worst = root_distance(&a.root(), &b.root());
    for (x, y) in a.nodes.iter().zip(&b.nodes) {
        if x.scale != y.scale {
            return u64::MAX;
        }
        for axis in 0..3 {
            worst = worst.max(u64::from(x.pos[axis].abs_diff(y.pos[axis])));
        }
        match (x.rot, y.rot) {
            (Rot::Quat { largest: la, c: ca }, Rot::Quat { largest: lb, c: cb }) if la == lb => {
                for k in 0..3 {
                    worst = worst.max(u64::from(ca[k].abs_diff(cb[k])));
                }
            }
            (Rot::Raw(a), Rot::Raw(b)) if a == b => {}
            _ => return u64::MAX,
        }
    }
    worst
}

pub fn root_distance(a: &Root, b: &Root) -> u64 {
    let mut worst = u64::from(a.radius.abs_diff(b.radius));
    for axis in 0..3 {
        worst = worst.max(u64::from(a.center[axis].abs_diff(b.center[axis])));
        worst = worst.max(u64::from(a.pos[axis].abs_diff(b.pos[axis])));
    }
    match (a.rot, b.rot) {
        (Some((la, ca)), Some((lb, cb))) if la == lb => {
            for k in 0..3 {
                worst = worst.max(u64::from(ca[k].abs_diff(cb[k])));
            }
            worst
        }
        (None, None) => worst,
        _ => u64::MAX,
    }
}

/// Per-component residuals against a same-object reference, for velocity
/// prediction. Layout: center xyz, radius, then per node rotation c0..c2 and
/// position xyz. A root update uses the first ten components.
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Velocity {
    pub residuals: Vec<i32>,
    pub span: u32,
}

const ROOT_COMPONENTS: usize = 10;

fn components(root: &Root) -> [i64; ROOT_COMPONENTS] {
    let rot = root.rot.map_or([0; 3], |(_, c)| c.map(i64::from));
    [
        i64::from(root.center[0]),
        i64::from(root.center[1]),
        i64::from(root.center[2]),
        i64::from(root.radius),
        rot[0],
        rot[1],
        rot[2],
        i64::from(root.pos[0]),
        i64::from(root.pos[1]),
        i64::from(root.pos[2]),
    ]
}

/// Integer constant-velocity extrapolation shared by encoder and decoder.
fn predict(velocity: Option<&Velocity>, index: usize, elapsed: u32) -> i64 {
    let Some(velocity) = velocity else { return 0 };
    if velocity.span == 0 || elapsed == 0 || elapsed > 1 << 16 {
        return 0;
    }
    let Some(&residual) = velocity.residuals.get(index) else { return 0 };
    let elapsed = elapsed.min(velocity.span.saturating_mul(4));
    (i64::from(residual) * i64::from(elapsed) / i64::from(velocity.span)).clamp(-(1 << 30), 1 << 30)
}

fn narrow(value: i64) -> Result<i32, Error> {
    i32::try_from(value).map_err(|_| Error)
}

#[derive(Default, Clone)]
pub struct Models {
    center: [Int; 2],
    radius: [Int; 2],
    scale_same: Bit,
    scale_one: Bit,
    quat: [Bit; 2],
    largest_same: Bit,
    rot_delta: [Int; 2],
    rot_abs: Int,
    raw_delta: Int,
    pos_delta: [Int; 2],
    pos_abs: Int,
    root_rot: Bit,
}

/// How a pose was referenced, with the extrapolation inputs if same-object.
pub struct Reference<'a> {
    pub pose: &'a Pose,
    pub velocity: Option<&'a Velocity>,
    pub elapsed: u32,
}

fn ordered(bits: u32) -> i64 {
    if bits & 0x8000_0000 != 0 { -i64::from(bits & 0x7FFF_FFFF) - 1 } else { i64::from(bits) }
}

fn unordered(value: i64) -> Result<u32, Error> {
    if value >= 0 {
        u32::try_from(value).ok().filter(|v| *v <= 0x7FFF_FFFF).ok_or(Error)
    } else {
        let magnitude = u32::try_from(-(value + 1)).map_err(|_| Error)?;
        (magnitude <= 0x7FFF_FFFF).then_some(magnitude | 0x8000_0000).ok_or(Error)
    }
}

/// Codes a complete pose. Returns same-object residuals for later prediction.
pub fn encode_full(e: &mut Encoder, m: &mut Models, pose: &Pose, reference: Option<&Reference<'_>>) -> Vec<i32> {
    let mut residuals = vec![0i32; ROOT_COMPONENTS - 6 + pose.nodes.len() * 6];
    let Some(r) = reference else {
        for axis in 0..3 {
            m.center[0].encode(e, i64::from(pose.center[axis]));
        }
        m.radius[0].encode(e, i64::from(pose.radius));
        for node in &pose.nodes {
            let one = node.scale == 1.0f32.to_bits();
            e.bit(&mut m.scale_one, one);
            if !one {
                e.direct(u64::from(node.scale), 32);
            }
            encode_rot_absolute(e, m, &node.rot);
            for axis in 0..3 {
                m.pos_abs.encode(e, i64::from(node.pos[axis]));
            }
        }
        return residuals;
    };
    let code = |e: &mut Encoder, model: &mut [Int; 2], index: usize, value: i64, base: i64, residuals: &mut Vec<i32>| {
        let residual = value - base;
        let prediction = predict(r.velocity, index, r.elapsed);
        model[usize::from(prediction != 0)].encode(e, residual - prediction);
        residuals[index] = residual.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
    };
    for axis in 0..3 {
        code(e, &mut m.center, axis, i64::from(pose.center[axis]), i64::from(r.pose.center[axis]), &mut residuals);
    }
    code(e, &mut m.radius, 3, i64::from(pose.radius), i64::from(r.pose.radius), &mut residuals);
    for (n, (node, base)) in pose.nodes.iter().zip(&r.pose.nodes).enumerate() {
        let index = 4 + n * 6;
        let same = node.scale == base.scale;
        e.bit(&mut m.scale_same, same);
        if !same {
            let one = node.scale == 1.0f32.to_bits();
            e.bit(&mut m.scale_one, one);
            if !one {
                e.direct(u64::from(node.scale), 32);
            }
        }
        let base_quat = matches!(base.rot, Rot::Quat { .. });
        e.bit(&mut m.quat[usize::from(base_quat)], matches!(node.rot, Rot::Quat { .. }));
        match (node.rot, base.rot) {
            (Rot::Quat { largest, c }, Rot::Quat { largest: base_largest, c: base_c }) => {
                e.bit(&mut m.largest_same, largest == base_largest);
                if largest == base_largest {
                    for k in 0..3 {
                        code(e, &mut m.rot_delta, index + k, i64::from(c[k]), i64::from(base_c[k]), &mut residuals);
                    }
                } else {
                    e.direct(u64::from(largest), 2);
                    for value in c {
                        m.rot_abs.encode(e, i64::from(value));
                    }
                }
            }
            (Rot::Quat { largest, c }, Rot::Raw(_)) => {
                e.direct(u64::from(largest), 2);
                for value in c {
                    m.rot_abs.encode(e, i64::from(value));
                }
            }
            (Rot::Raw(words), Rot::Raw(base_words)) => {
                for k in 0..9 {
                    m.raw_delta.encode(e, ordered(words[k]) - ordered(base_words[k]));
                }
            }
            (Rot::Raw(words), Rot::Quat { .. }) => {
                for word in words {
                    e.direct(u64::from(word), 32);
                }
            }
        }
        for axis in 0..3 {
            code(e, &mut m.pos_delta, index + 3 + axis, i64::from(node.pos[axis]), i64::from(base.pos[axis]), &mut residuals);
        }
    }
    residuals
}

fn encode_rot_absolute(e: &mut Encoder, m: &mut Models, rot: &Rot) {
    e.bit(&mut m.quat[0], matches!(rot, Rot::Quat { .. }));
    match *rot {
        Rot::Quat { largest, c } => {
            e.direct(u64::from(largest), 2);
            for value in c {
                m.rot_abs.encode(e, i64::from(value));
            }
        }
        Rot::Raw(words) => {
            for word in words {
                e.direct(u64::from(word), 32);
            }
        }
    }
}

fn decode_rot_absolute(d: &mut Decoder<'_>, m: &mut Models, quat: bool) -> Result<Rot, Error> {
    if quat {
        let largest = d.direct(2) as u8;
        let mut c = [0i16; 3];
        for value in &mut c {
            *value = rot_component(m.rot_abs.decode(d)?)?;
        }
        Ok(Rot::Quat { largest, c })
    } else {
        Ok(Rot::Raw(std::array::from_fn(|_| d.direct(32) as u32)))
    }
}

fn rot_component(value: i64) -> Result<i16, Error> {
    if value.unsigned_abs() > ROT_LIMIT as u64 {
        return Err(Error);
    }
    Ok(value as i16)
}

pub fn decode_full(
    d: &mut Decoder<'_>,
    m: &mut Models,
    nodes: u16,
    reference: Option<&Reference<'_>>,
) -> Result<(Pose, Vec<i32>), Error> {
    let count = usize::from(nodes);
    let mut residuals = vec![0i32; 4 + count * 6];
    let Some(r) = reference else {
        let mut center = [0i32; 3];
        for value in &mut center {
            *value = narrow(m.center[0].decode(d)?)?;
        }
        let radius = u32::try_from(m.radius[0].decode(d)?).map_err(|_| Error)?;
        let mut out = Vec::with_capacity(count);
        for _ in 0..count {
            let scale = if d.bit(&mut m.scale_one) { 1.0f32.to_bits() } else { d.direct(32) as u32 };
            let quat = d.bit(&mut m.quat[0]);
            let rot = decode_rot_absolute(d, m, quat)?;
            let mut pos = [0i32; 3];
            for value in &mut pos {
                *value = narrow(m.pos_abs.decode(d)?)?;
            }
            out.push(Node { scale, rot, pos });
        }
        return Ok((Pose { center, radius, nodes: out }, residuals));
    };
    if r.pose.nodes.len() != count {
        return Err(Error);
    }
    let code = |d: &mut Decoder<'_>, model: &mut [Int; 2], index: usize, base: i64, residuals: &mut Vec<i32>| -> Result<i64, Error> {
        let prediction = predict(r.velocity, index, r.elapsed);
        let residual = model[usize::from(prediction != 0)].decode(d)?.checked_add(prediction).ok_or(Error)?;
        residuals[index] = residual.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
        base.checked_add(residual).ok_or(Error)
    };
    let mut center = [0i32; 3];
    for (axis, value) in center.iter_mut().enumerate() {
        *value = narrow(code(d, &mut m.center, axis, i64::from(r.pose.center[axis]), &mut residuals)?)?;
    }
    let radius = u32::try_from(code(d, &mut m.radius, 3, i64::from(r.pose.radius), &mut residuals)?).map_err(|_| Error)?;
    let mut out = Vec::with_capacity(count);
    for (n, base) in r.pose.nodes.iter().enumerate() {
        let index = 4 + n * 6;
        let scale = if d.bit(&mut m.scale_same) {
            base.scale
        } else if d.bit(&mut m.scale_one) {
            1.0f32.to_bits()
        } else {
            d.direct(32) as u32
        };
        let base_quat = matches!(base.rot, Rot::Quat { .. });
        let quat = d.bit(&mut m.quat[usize::from(base_quat)]);
        let rot = match (quat, base.rot) {
            (true, Rot::Quat { largest: base_largest, c: base_c }) => {
                if d.bit(&mut m.largest_same) {
                    let mut c = [0i16; 3];
                    for k in 0..3 {
                        c[k] = rot_component(code(d, &mut m.rot_delta, index + k, i64::from(base_c[k]), &mut residuals)?)?;
                    }
                    Rot::Quat { largest: base_largest, c }
                } else {
                    decode_rot_absolute(d, m, true)?
                }
            }
            (true, Rot::Raw(_)) => decode_rot_absolute(d, m, true)?,
            (false, Rot::Raw(base_words)) => {
                let mut words = [0u32; 9];
                for k in 0..9 {
                    words[k] = unordered(ordered(base_words[k]).checked_add(m.raw_delta.decode(d)?).ok_or(Error)?)?;
                }
                Rot::Raw(words)
            }
            (false, Rot::Quat { .. }) => decode_rot_absolute(d, m, false)?,
        };
        let mut pos = [0i32; 3];
        for (axis, value) in pos.iter_mut().enumerate() {
            *value = narrow(code(d, &mut m.pos_delta, index + 3 + axis, i64::from(base.pos[axis]), &mut residuals)?)?;
        }
        out.push(Node { scale, rot, pos });
    }
    Ok((Pose { center, radius, nodes: out }, residuals))
}

/// Codes a root update against the same object's acknowledged root.
pub fn encode_root(
    e: &mut Encoder,
    m: &mut Models,
    root: &Root,
    base: &Root,
    velocity: Option<&Velocity>,
    elapsed: u32,
) -> Vec<i32> {
    let mut residuals = vec![0i32; ROOT_COMPONENTS];
    let (now, then) = (components(root), components(base));
    let same_rot = matches!((root.rot, base.rot), (Some((a, _)), Some((b, _))) if a == b);
    e.bit(&mut m.root_rot, root.rot.is_some());
    if let Some((largest, _)) = root.rot {
        if base.rot.is_some() {
            e.bit(&mut m.largest_same, same_rot);
        }
        if !same_rot {
            e.direct(u64::from(largest), 2);
        }
    }
    for index in 0..ROOT_COMPONENTS {
        let model = match index {
            0..=2 => &mut m.center,
            3 => &mut m.radius,
            4..=6 => &mut m.rot_delta,
            _ => &mut m.pos_delta,
        };
        if (4..=6).contains(&index) {
            if root.rot.is_none() {
                continue;
            }
            if !same_rot {
                m.rot_abs.encode(e, now[index]);
                continue;
            }
        }
        let residual = now[index] - then[index];
        let prediction = predict(velocity, index, elapsed);
        model[usize::from(prediction != 0)].encode(e, residual - prediction);
        residuals[index] = residual.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
    }
    residuals
}

pub fn decode_root(
    d: &mut Decoder<'_>,
    m: &mut Models,
    base: &Root,
    velocity: Option<&Velocity>,
    elapsed: u32,
) -> Result<(Root, Vec<i32>), Error> {
    let mut residuals = vec![0i32; ROOT_COMPONENTS];
    let then = components(base);
    let has_rot = d.bit(&mut m.root_rot);
    let mut largest = None;
    let mut same_rot = false;
    if has_rot {
        if let Some((base_largest, _)) = base.rot {
            same_rot = d.bit(&mut m.largest_same);
            if same_rot {
                largest = Some(base_largest);
            }
        }
        if !same_rot {
            largest = Some(d.direct(2) as u8);
        }
    }
    let mut now = [0i64; ROOT_COMPONENTS];
    for index in 0..ROOT_COMPONENTS {
        let model = match index {
            0..=2 => &mut m.center,
            3 => &mut m.radius,
            4..=6 => &mut m.rot_delta,
            _ => &mut m.pos_delta,
        };
        if (4..=6).contains(&index) {
            if !has_rot {
                continue;
            }
            if !same_rot {
                now[index] = m.rot_abs.decode(d)?;
                continue;
            }
        }
        let prediction = predict(velocity, index, elapsed);
        let residual = model[usize::from(prediction != 0)].decode(d)?.checked_add(prediction).ok_or(Error)?;
        residuals[index] = residual.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32;
        now[index] = then[index].checked_add(residual).ok_or(Error)?;
    }
    let root = Root {
        center: [narrow(now[0])?, narrow(now[1])?, narrow(now[2])?],
        radius: u32::try_from(now[3]).map_err(|_| Error)?,
        rot: match largest {
            Some(largest) => Some((largest, [rot_component(now[4])?, rot_component(now[5])?, rot_component(now[6])?])),
            None => None,
        },
        pos: [narrow(now[7])?, narrow(now[8])?, narrow(now[9])?],
    };
    Ok((root, residuals))
}

pub fn root_velocity(full: &Velocity) -> Velocity {
    Velocity { residuals: full.residuals.iter().take(ROOT_COMPONENTS).copied().collect(), span: full.span }
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;
    use crate::netcode::generic::tests::Lcg;

    pub fn rotation(yaw: f32, pitch: f32) -> Matrix {
        let (sy, cy) = yaw.sin_cos();
        let (sp, cp) = pitch.sin_cos();
        // Rz(yaw) * Ry(pitch)
        [[cy * cp, -sy, cy * sp], [sy * cp, cy, sy * sp], [-sp, 0.0, cp]]
    }

    /// A biped-like body: nodes hang from a center and swing over time.
    pub fn body(object: u32, nodes: usize, center: [f32; 3], phase: f32) -> RawPose {
        let nodes = (0..nodes)
            .map(|n| {
                let m = rotation(phase * 0.7 + n as f32 * 0.3, (phase + n as f32).sin() * 0.4);
                let mut values = [0.0f32; 13];
                values[0] = 1.0;
                for c in 0..3 {
                    for r in 0..3 {
                        values[1 + c * 3 + r] = m[r][c];
                    }
                }
                values[10] = center[0] + (n as f32 * 0.37 + phase).sin() * 0.3;
                values[11] = center[1] + (n as f32 * 0.21).cos() * 0.3;
                values[12] = center[2] + n as f32 * 0.02 - 0.3;
                values
            })
            .collect();
        RawPose { key: Key { object, nodes: 0 }, sphere: [center[0], center[1], center[2], 0.8], nodes }
    }

    fn bytes_of(poses: &[RawPose]) -> Vec<u8> {
        let mut out = vec![1u8];
        out.extend_from_slice(&(poses.len() as u16).to_le_bytes());
        for pose in poses {
            out.extend_from_slice(&pose.key.object.to_le_bytes());
            out.extend_from_slice(&(pose.nodes.len() as u16).to_le_bytes());
            for v in pose.sphere {
                out.extend_from_slice(&v.to_le_bytes());
            }
            for node in &pose.nodes {
                for v in node {
                    out.extend_from_slice(&v.to_le_bytes());
                }
            }
        }
        out
    }

    #[test]
    fn parse_matches_the_native_layout_and_rejects_trailing_bytes() {
        let poses = vec![body(0x1234_0001, 20, [1.0, 2.0, 3.0], 0.0), body(0x1234_0002, 3, [5.0, 5.0, 0.5], 1.0)];
        let mut bytes = bytes_of(&poses);
        let bank = parse(&bytes).unwrap();
        assert_eq!(bank.poses.len(), 2);
        assert_eq!(bank.poses[0].key, Key { object: 0x1234_0001, nodes: 20 });
        let keys: Vec<Key> = bank.poses.iter().map(|p| p.key).collect();
        assert_eq!(bank_len(&keys), bytes.len());
        bytes.push(0);
        assert!(parse(&bytes).is_none());
    }

    #[test]
    fn quantized_rigid_nodes_stay_within_visual_tolerance() {
        let raw = body(0x2000_0001, 50, [120.5, -33.25, 8.0], 0.4);
        let pose = quantize(&raw);
        let mut out = Vec::new();
        materialize(Key { object: 0x2000_0001, nodes: 50 }, &pose, None, &mut out);
        assert_eq!(out.len(), 6 + 16 + 50 * NODE_BYTES);
        for (n, values) in raw.nodes.iter().enumerate() {
            for k in 0..13 {
                let decoded = f32_at(&out, 22 + n * NODE_BYTES + k * 4);
                let tolerance = if k >= 10 { 1.5e-3 } else { 2e-3 };
                assert!((decoded - values[k]).abs() <= tolerance, "node {n} word {k}: {decoded} vs {}", values[k]);
            }
        }
        let radius = f32_at(&out, 18);
        assert!(radius >= 0.8);
    }

    #[test]
    fn non_rigid_nodes_are_carried_losslessly() {
        let mut raw = body(0x2000_0002, 4, [0.0, 0.0, 0.0], 0.0);
        raw.nodes[2][1] = 3.0; // stretched basis
        let pose = quantize(&raw);
        assert!(matches!(pose.nodes[2].rot, Rot::Raw(_)));
        let mut out = Vec::new();
        materialize(Key { object: 2, nodes: 4 }, &pose, None, &mut out);
        for k in 1..10 {
            assert_eq!(f32_at(&out, 22 + 2 * NODE_BYTES + k * 4).to_bits(), raw.nodes[2][k].to_bits());
        }
    }

    fn round_trip(pose: &Pose, reference: Option<&Reference<'_>>) -> usize {
        let mut e = Encoder::default();
        let sent = encode_full(&mut e, &mut Models::default(), pose, reference);
        let bytes = e.finish();
        let mut d = Decoder::new(&bytes);
        let (decoded, residuals) = decode_full(&mut d, &mut Models::default(), pose.nodes.len() as u16, reference).unwrap();
        assert_eq!(&decoded, pose);
        assert_eq!(residuals, sent);
        bytes.len()
    }

    #[test]
    fn animated_deltas_with_velocity_are_much_smaller_than_absolute() {
        let mut previous = quantize(&body(7, 50, [10.0, 10.0, 1.0], 0.0));
        let mut velocity: Option<Velocity> = None;
        let absolute = round_trip(&previous, None);
        let mut delta_total = 0;
        for tick in 1..=30 {
            let phase = tick as f32 * 0.05;
            let current = quantize(&body(7, 50, [10.0 + phase, 10.0, 1.0], phase));
            let reference = Reference { pose: &previous, velocity: velocity.as_ref(), elapsed: 1 };
            delta_total += round_trip(&current, Some(&reference));
            let mut e = Encoder::default();
            let residuals = encode_full(&mut e, &mut Models::default(), &current, Some(&reference));
            velocity = Some(Velocity { residuals, span: 1 });
            previous = current;
        }
        let average = delta_total / 30;
        assert!(average * 2 < absolute, "delta {average} absolute {absolute}");
    }

    #[test]
    fn root_updates_round_trip_and_re_root_held_poses() {
        let held = quantize(&body(9, 12, [0.0, 0.0, 0.0], 0.0));
        let moved = quantize(&body(9, 12, [2.0, 1.0, 0.0], 0.3));
        let (base, root) = (held.root(), moved.root());
        let mut e = Encoder::default();
        let sent = encode_root(&mut e, &mut Models::default(), &root, &base, None, 1);
        let bytes = e.finish();
        let (decoded, residuals) = decode_root(&mut Decoder::new(&bytes), &mut Models::default(), &base, None, 1).unwrap();
        assert_eq!(decoded, root);
        assert_eq!(residuals, sent);
        assert!(bytes.len() < 16);
        let mut out = Vec::new();
        materialize(Key { object: 9, nodes: 12 }, &held, Some(&root), &mut out);
        // Node 0 lands exactly where the newer root put it.
        let expected = moved.root();
        let center = expected.center.map(dequantize_coord);
        for axis in 0..3 {
            let got = f32_at(&out, 22 + 40 + axis * 4);
            let want = center[axis] + dequantize_coord(expected.pos[axis]);
            assert!((got - want).abs() < 2e-3, "axis {axis} {got} {want}");
        }
    }

    #[test]
    fn corrupt_pose_streams_fail_without_panicking() {
        let mut rng = Lcg(21);
        let base = quantize(&body(1, 8, [0.0; 3], 0.0));
        for _ in 0..300 {
            let garbage: Vec<u8> = (0..(rng.next() % 48)).map(|_| rng.next() as u8).collect();
            let reference = Reference { pose: &base, velocity: None, elapsed: 1 };
            let _ = decode_full(&mut Decoder::new(&garbage), &mut Models::default(), 8, Some(&reference));
            let _ = decode_full(&mut Decoder::new(&garbage), &mut Models::default(), 8, None);
            let _ = decode_root(&mut Decoder::new(&garbage), &mut Models::default(), &base.root(), None, 1);
        }
    }
}
