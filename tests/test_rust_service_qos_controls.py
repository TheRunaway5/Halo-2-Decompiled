"""Bounded native readiness protocol, including the pinned SDK startup line."""
from contextlib import contextmanager
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('rust_qos_controls',ROOT/'tools/rust_service_qos_controls.py')
CONTROLS=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONTROLS)


@contextmanager
def probe(code):
    process=subprocess.Popen([sys.executable,'-u','-c',code],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    try:yield process
    finally:
        if process.poll() is None:process.terminate()
        try:process.communicate(timeout=2)
        except subprocess.TimeoutExpired:process.kill();process.communicate()


class NativeReadyTests(unittest.TestCase):
    def test_pinned_sdk_prelude_and_ready_in_one_pipe_write(self):
        # This is the observed SDK line. One write also catches the buffered
        # readline/select bug when both lines have already reached the pipe.
        with probe("import os;os.write(1,b'Shutdown handler: initialize.\\nREADY native QoS clients\\nAFTER\\n')") as process:
            startup,pending=CONTROLS._await_native_ready(process)
            self.assertEqual(startup,['Shutdown handler: initialize.'])
            self.assertEqual(pending+process.communicate(timeout=2)[0],b'AFTER\n')

    def test_ready_without_sdk_keeps_following_output(self):
        with probe("import os;os.write(1,b'READY native QoS clients\\nAFTER\\n')") as process:
            startup,pending=CONTROLS._await_native_ready(process)
            self.assertEqual(startup,[])
            self.assertEqual(pending+process.communicate(timeout=2)[0],b'AFTER\n')

    def test_delayed_ready_after_sdk_line(self):
        with probe("import os,time;os.write(1,b'Shutdown handler: initialize.\\n');time.sleep(.02);os.write(1,b'READY native QoS clients\\n')") as process:
            self.assertEqual(CONTROLS._await_native_ready(process)[0],['Shutdown handler: initialize.'])

    def test_unknown_prelude_is_rejected_without_printing_it(self):
        with probe("import os;os.write(1,b'private-unexpected-value\\nREADY native QoS clients\\n')") as process:
            with self.assertRaises((AssertionError,RuntimeError)) as failure:CONTROLS._await_native_ready(process)
            self.assertNotIn('private-unexpected-value',str(failure.exception))

    def test_missing_ready_is_not_a_successful_launch(self):
        with probe("print('Shutdown handler: initialize.')") as process:
            with self.assertRaises((AssertionError,RuntimeError)):CONTROLS._await_native_ready(process)

    def test_oversize_startup_is_rejected(self):
        with probe("import os;os.write(1,b'x'*8192+b'\\nREADY native QoS clients\\n')") as process:
            with self.assertRaises((AssertionError,RuntimeError)):CONTROLS._await_native_ready(process)


if __name__=='__main__':unittest.main()
