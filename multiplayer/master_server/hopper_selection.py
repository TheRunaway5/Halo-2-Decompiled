"""Owned catalog-row adapter to the original ordered Rust hopper draw.

No Python random distribution or selector fallback. Authentication supplies
installed maps; Python only maps Rust's bounded row index to the immutable row.
"""
import ctypes
from functools import lru_cache
import threading

from .protocol import integer, require
from .security_policy import library


@lru_cache(maxsize=1)
def native():
    lib = library()
    require(hasattr(lib, "h2_hopper_draw"), "Hopper selection library requires an update", 503)
    draw = lib.h2_hopper_draw
    draw.argtypes = [ctypes.c_uint32, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int32),
                    ctypes.POINTER(ctypes.c_uint8)] + [ctypes.POINTER(ctypes.c_uint32)] * 4
    draw.restype = ctypes.c_uint32
    return draw


def available_games(games, players):
    from .native_matchmaking import value
    games, players = tuple(games), tuple(players)
    indexes = value('available', game_maps=tuple(game.map for game in games), maps=tuple(p.maps for p in players))
    return tuple(games[index] for index in indexes)


class HopperSelector:
    def __init__(self, seed):
        self.seed = integer(seed, 0, 0xffffffff)
        self.lock = threading.Lock()

    def choose(self, games, players):
        require(1 <= len(games) <= 100, "Invalid hopper row count")
        available = {id(game) for game in available_games(games, players)}
        weights = (ctypes.c_int32 * len(games))(*(game.weight for game in games))
        admitted = (ctypes.c_uint8 * len(games))(*(int(id(game) in available) for game in games))
        next_seed, index, total, choice = (ctypes.c_uint32() for _ in range(4))
        with self.lock:
            status = native()(self.seed, len(games), weights, admitted,
                              ctypes.byref(next_seed), ctypes.byref(index), ctypes.byref(total), ctypes.byref(choice))
            require(status in (0, 1), "Invalid hopper weight domain")
            if status == 1:
                return None
            require(index.value < len(games) and admitted[index.value], "Invalid selected hopper row")
            self.seed = next_seed.value
            return games[index.value]
