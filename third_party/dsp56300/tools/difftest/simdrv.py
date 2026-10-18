"""Pty driver for Motorola's sim56300.exe under wine.

The simulator is an interactive console program that cannot be scripted via
stdin piping; it works fine on a pty with adaptive quiescence detection
(wait until output has been quiet briefly before sending the next command).

Findings that shape this driver (verified against sim 6.4.2):
  - `load file.lod` loads Motorola _DATA-format LOD files (relative to cwd)
  - `step N` executes N instructions and prints ONE full register dump
  - `break p:$addr` + `go` stops at the breakpoint and prints one dump
  - `change` takes a single addr/value pair; multi-value forms error
  - `input` is for pin stimuli, NOT command files
  - `log s file` mirrors the session to a file, but the raw pty stream is
    what we parse (the log can truncate after error redraws)
"""

import os
import pty
import re
import select
import signal
import time

SIM = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "..", "dsp56300-ref",
    "official-tools", "DSP56300Tools", "sim", "sim56300.exe")

ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b\][^\x07]*\x07")

# Register dump fields: name=$hex or name={$hex}
FIELD_RE = re.compile(r"([a-z]{1,4}[0-7]?)=\s*\{?\$([0-9a-fA-F]+)\}?")
# Counters: cyc=   {000010}  (decimal, no $)
CTR_RE = re.compile(r"(cyc|ictr)=\s*\{?(\d+)\}?")

DUMP_REGS = [
    "pc", "sr", "omr", "la", "lc", "sp", "ssh", "ssl", "ep", "sz", "sc",
    "vba", "x0", "x1", "y0", "y1", "a0", "a1", "a2", "b0", "b1", "b2",
] + ["r%d" % i for i in range(8)] + ["n%d" % i for i in range(8)] + \
    ["m%d" % i for i in range(8)]


class SimSession:
    def __init__(self, cwd=None, boot_wait=15.0, sim=SIM):
        self.out = b""
        pid, fd = pty.fork()
        if pid == 0:
            if cwd:
                os.chdir(cwd)
            os.execvp("wine", ["wine", sim])
        self.pid, self.fd = pid, fd
        self._drain(boot_wait, quiet=1.5)

    def _drain(self, max_t, quiet=0.25):
        end = time.time() + max_t
        last = time.time()
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    chunk = os.read(self.fd, 65536)
                except OSError:
                    return
                if chunk:
                    self.out += chunk
                    last = time.time()
            elif time.time() - last > quiet:
                return

    def cmd(self, c, max_t=30.0, quiet=0.25):
        """Send one command; return the output it produced (ANSI-stripped)."""
        mark = len(self.out)
        os.write(self.fd, c.encode() + b"\r")
        self._drain(max_t, quiet)
        return ANSI_RE.sub("", self.out[mark:].decode("latin1"))

    def parse_dump(self, text):
        """Extract the LAST register dump from command output."""
        regs = {}
        for name, val in FIELD_RE.findall(text):
            if name in DUMP_REGS:
                regs[name] = int(val, 16)
        for name, val in CTR_RE.findall(text):
            regs[name] = int(val)
        return regs

    def close(self):
        try:
            os.write(self.fd, b"quit\r")
            self._drain(3.0)
        except OSError:
            pass
        try:
            os.kill(self.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
