"""Retail gathering-session admission, scoring, ordering and selection.

Authority: unchanged retail 0x90D90/0x91000/0x90F10/0x91400, sort
0x13DA70/0x13DCD0 and threshold prefix 0x90B30. See the candidate evidence
note. The 50 x 0x204 bank is an INTERNAL value representation used to preserve
the original byte-cache and comparator behavior. It is not a network protocol,
a public input parser, a central party assembler or a QoS measurement adapter.
In particular, relay percentages and ticket ages are not accepted as inputs.
"""
from dataclasses import dataclass
import math
import struct


COUNT, STRIDE = 50, 0x204
BANK_SIZE = COUNT * STRIDE
EMPTY_BANK = bytes(BANK_SIZE)
I32_MIN, I32_MAX = -(1 << 31), (1 << 31) - 1


def _integer(value, low=I32_MIN, high=I32_MAX):
    if type(value) is not int or not low <= value <= high:
        raise ValueError("Invalid session-search integer")
    return value


def _f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def _signed(value):
    return (value + (1 << 31)) % (1 << 32) - (1 << 31)


def _divide(a, b):
    # IDIV truncates toward zero, including negative configured coefficients.
    return (abs(a) // abs(b)) * (-1 if (a < 0) != (b < 0) else 1)


def _convert(value):
    # Retail's truncating float conversion returns the integer indefinite value
    # on overflow. Normal authored coefficients stay far inside this range.
    return math.trunc(value) if math.isfinite(value) and I32_MIN <= value < I32_MAX + 1 else I32_MIN


def _read(row, offset):
    return struct.unpack_from("<i", row, offset)[0]


def _write(row, offset, value):
    struct.pack_into("<i", row, offset, _signed(value))


def _language_table():
    special = {1, 6, 7}
    return tuple(tuple(100 if a == b and a in special else
                       0 if a == b else -100 if a in special or b in special else -50
                       for b in range(9)) for a in range(9))


@dataclass(frozen=True)
class Configuration:
    """0x66330 retail defaults; later downloaded configurations remain unknown."""
    rank_radius: int = 4
    rank_high: int = 100
    rank_low: int = 25
    exact_space_bonus: int = 50
    maximum_age_seconds: int = 180
    age_coefficient: float = 2.0
    first_quality_threshold: int = 262144
    second_quality_threshold: int = 262144
    quality_adjustment: int = 100
    latency_curve: tuple[int, ...] = (100, 80, 50, 0, -100, -200, -200, -200, -200, -200)
    language: tuple[tuple[int, ...], ...] = _language_table()
    partial_qos_delay_ms: int = 5000
    threshold_initial: int = 200
    threshold_final: int = 100

    def __post_init__(self):
        _integer(self.rank_radius, 0, I32_MAX - 1)
        _integer(self.maximum_age_seconds, 0)
        _integer(self.partial_qos_delay_ms, 0)
        for value in (self.rank_high, self.rank_low, self.exact_space_bonus,
                      self.first_quality_threshold, self.second_quality_threshold,
                      self.quality_adjustment, self.threshold_initial, self.threshold_final):
            _integer(value)
        if type(self.age_coefficient) is not float or not math.isfinite(self.age_coefficient):
            raise ValueError("Invalid session-age coefficient")
        if type(self.latency_curve) is not tuple or len(self.latency_curve) != 10:
            raise ValueError("Invalid latency curve")
        if type(self.language) is not tuple or len(self.language) != 9:
            raise ValueError("Invalid language matrix")
        for value in self.latency_curve:
            _integer(value)
        for row in self.language:
            if type(row) is not tuple or len(row) != 9:
                raise ValueError("Invalid language matrix")
            for value in row:
                _integer(value)


@dataclass(frozen=True)
class Search:
    player_count: int
    average_rank: int
    language: int
    quality_first: int
    quality_second: int
    minimum_rank: int | None = None
    maximum_rank: int | None = None
    own_address: bytes | None = None
    clan: bytes | None = None

    def __post_init__(self):
        _integer(self.player_count, 1, 16)
        _integer(self.average_rank, 0, 127)
        _integer(self.language, 0, 8)
        for value in (self.quality_first, self.quality_second):
            _integer(value)
        for value in (self.minimum_rank, self.maximum_rank):
            if value is not None:
                _integer(value, 0, 127)
        for value, size in ((self.own_address, 36), (self.clan, 12)):
            if value is not None and (type(value) is not bytes or len(value) != size):
                raise ValueError("Invalid private session identity")


def _allowed(search, row):
    detailed = bool(row[0xA4])
    if detailed:
        # The original lower/upper comparisons look unusual. Preserve their
        # executed direction rather than silently correcting the decompile.
        if _read(row, 0xAC) != 4 or _read(row, 0xB0) < 0x2651 or _read(row, 0xB4) > 0x2651:
            return False
        mask, minimum, maximum = (_read(row, at) for at in (0x110, 0x12C, 0x124))
    else:
        mask, minimum, maximum = (_read(row, at) for at in (0x70, 0x64, 0x68))
    if search.own_address is not None and row[0x24:0x48] == search.own_address:
        return False
    if detailed and search.clan is not None:
        count = _read(row, 0x130)
        if not 0 <= count <= 16:
            raise ValueError("Invalid detailed clan count")
        if any(row[0x134 + i*12:0x140 + i*12] == search.clan for i in range(count)):
            return False
    return (bool(mask & (1 << (search.player_count - 1))) and
            (search.minimum_rank is None or minimum >= search.minimum_rank) and
            (search.maximum_rank is None or maximum <= search.maximum_rank))


def _latency_score(ms, config):
    t = min(9.0, max(0.0, _f32(_f32(ms) * _f32(0.01))))
    index = math.trunc(t)
    if index == 9:
        return config.latency_curve[9]
    lower, upper = (_f32(config.latency_curve[i]) for i in (index, index + 1))
    return _convert(_f32(lower + _f32(_f32(t - _f32(index)) * _f32(upper - lower))))


def _score(search, row, config):
    _write(row, 0x1F8, 0)
    if row[0] and row[1] and not _allowed(search, row):
        row[1] = 0
    if not row[0] or not row[1]:
        return
    detailed = bool(row[0xA4])
    space, rank, age = (_read(row, at) for at in
                        ((0x10C, 0x120, 0x11C) if detailed else (0x60, 0x5C, 0x6C)))
    _integer(rank, 0, 127)
    difference = abs(rank - search.average_rank)
    _write(row, 0x1FC, difference)
    score = 0
    if difference <= config.rank_radius:
        step = _divide(_signed(config.rank_high - config.rank_low), config.rank_radius + 1)
        score = _signed(config.rank_high - _signed(step * difference))
    if space == search.player_count:
        score = _signed(score + config.exact_space_bonus)
    bounded_age = min(config.maximum_age_seconds, max(0, age))
    _write(row, 0x200, bounded_age)
    score = _convert(score + bounded_age * config.age_coefficient)
    if detailed:
        if ((_read(row, 0xF8) < config.first_quality_threshold or
             _read(row, 0xFC) < config.second_quality_threshold) and
                search.quality_first >= config.first_quality_threshold and
                search.quality_second >= config.second_quality_threshold):
            score = _signed(score + config.quality_adjustment)
        language = _integer(_read(row, 0xB8), 0, 8)
        score = _signed(score + config.language[search.language][language])
    if row[0x74]:
        score = _signed(score + _latency_score(_read(row, 0x90), config))
    _write(row, 0x1F8, score)


def _precedes(first, second):
    # This means "worse than". Readiness is deliberately tested only on the
    # FIRST record. A tuple key or Python sort would alter mixed-ready ties.
    a, b = _read(first, 0x1F8), _read(second, 0x1F8)
    if a != b:
        return a < b
    if first[0x74]:
        a, b = _read(first, 0x90), _read(second, 0x90)
        if a != b:
            return a > b
    a, b = _read(first, 0x1FC), _read(second, 0x1FC)
    if a != b:
        return a > b
    return _read(first, 0x10C) > _read(second, 0x10C)


def _sort(rows):
    """The fixed-middle-pivot retail sort, including its eight-row short sort."""
    stack = [(0, COUNT - 1)]
    while stack:
        lo, hi = stack.pop()
        while True:
            size = hi - lo + 1
            if size <= 8:
                while hi > lo:
                    worst = lo
                    for p in range(lo + 1, hi + 1):
                        if _precedes(rows[p], rows[worst]):
                            worst = p
                    rows[worst], rows[hi] = rows[hi], rows[worst]
                    hi -= 1
                break
            middle = lo + size // 2
            rows[middle], rows[lo] = rows[lo], rows[middle]
            low, high = lo, hi + 1
            while True:
                low += 1
                while low <= hi and not _precedes(rows[low], rows[lo]):
                    low += 1
                high -= 1
                while high > lo and not _precedes(rows[lo], rows[high]):
                    high -= 1
                if high < low:
                    break
                rows[low], rows[high] = rows[high], rows[low]
            rows[lo], rows[high] = rows[high], rows[lo]
            # Original compares byte pointers and subtracts ONE BYTE here.
            if (high - lo)*STRIDE - 1 >= (hi - low)*STRIDE:
                if lo + 1 < high:
                    stack.append((lo, high - 1))
                if low < hi:
                    lo = low
                    continue
            else:
                if low < hi:
                    stack.append((low, hi))
                if lo + 1 < high:
                    hi = high - 1
                    continue
            break


def threshold(radius, maximum_radius, config=Configuration()):
    """0x90B30 radius interpolation, not a guessed elapsed-time schedule."""
    _integer(radius, -1, 127)
    _integer(maximum_radius, 0, 127)
    if radius == -1 or maximum_radius <= 0:
        return config.threshold_final
    ratio = _f32(_f32(radius) / _f32(maximum_radius))
    difference = _f32(_f32(config.threshold_final) - _f32(config.threshold_initial))
    return _convert(_f32(_f32(config.threshold_initial) + _f32(ratio * difference)))


class Candidates:
    """One searching party's bank; recreate it for each genuine new query.

    update() takes all 50 records, retains opaque bytes, and returns the sorted
    bank. Feeding its unchanged output back in deliberately bypasses rescoring,
    even after a configuration change, just as the original memcmp cache does.
    select() returns a complete private selected record or None. The caller
    owns advertisements, query start time, QoS and actual all-party joining.
    """
    def __init__(self, search, config=Configuration()):
        if type(search) is not Search or type(config) is not Configuration:
            raise ValueError("Trusted session-search values required")
        self.search, self.config = search, config
        self.bank = EMPTY_BANK

    def update(self, bank):
        if type(bank) is not bytes or len(bank) != BANK_SIZE:
            raise ValueError("A complete session-candidate bank is required")
        if bank == self.bank:
            return self.bank
        rows = [bytearray(bank[i*STRIDE:(i+1)*STRIDE]) for i in range(COUNT)]
        for row in rows:
            _score(self.search, row, self.config)
        _sort(rows)
        self.bank = b"".join(rows)
        return self.bank

    def select(self, minimum_score, elapsed_query_ms, *, active=True):
        _integer(minimum_score)
        _integer(elapsed_query_ms)
        if type(active) is not bool:
            raise ValueError("Invalid active-query flag")
        if not active:
            return None
        eligible, ready, selected = 0, 0, None
        for i in range(COUNT):
            row = self.bank[i*STRIDE:(i+1)*STRIDE]
            if row[0] and row[1]:
                eligible += 1
                if row[0x74]:
                    ready += 1
                    if selected is None and _read(row, 0x1F8) >= minimum_score:
                        selected = row
        if selected is not None and (ready == eligible or
                (ready >= eligible // 4 and elapsed_query_ms > self.config.partial_qos_delay_ms)):
            return selected
        return None
