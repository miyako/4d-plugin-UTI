"""Compile actual first-party sources and the full unchanged SDK on macOS."""

import os
from pathlib import Path
import platform
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(os.environ.get("UTI_OWNERSHIP_SOURCE", ROOT))


@unittest.skipUnless(platform.system() == "Darwin", "macOS native engine shim")
class OwnershipTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="uti-ownership-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = Path(cls.temp.name) / "ownership"
        sdk = SOURCE / "4D Plugin API"
        sources = [
            sdk / "4DPluginAPI.c", SOURCE / "support/4DPlugin-JSON.cpp",
            SOURCE / "4DPlugin-UTI.cpp",
            *(sdk / "Classes" / name for name in (
                "ARRAY_TEXT.cpp", "C_TEXT.cpp", "C_LONGINT.cpp", "C_REAL.cpp",
                "C_BLOB.cpp")),
            ROOT / "tests/ownership_host.cpp",
        ]
        result = subprocess.run([
            "xcrun", "clang++", "-x", "objective-c++", "-std=c++17",
            "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-Wno-deprecated-declarations", "-Wno-multichar",
            "-I", str(SOURCE), "-I", str(sdk), "-I", str(sdk / "Classes"),
            "-I", str(SOURCE / "support"), *map(str, sources),
            "-framework", "Cocoa", "-framework", "Quartz",
            "-o", str(cls.binary),
        ], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def invoke(self, name):
        for empty in ([], ["null-empty"]):
            with self.subTest(empty="null" if empty else "allocated"):
                result = subprocess.run(
                    [str(self.binary), name, *empty], capture_output=True, text=True,
                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                         "UBSAN_OPTIONS": "halt_on_error=1"})
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_setter_empty_overwrite(self):
        self.invoke("setters")

    def test_picture_empty_overwrite(self):
        self.invoke("picture")

    def test_getter_string_owner(self):
        self.invoke("getter-a")

    def test_getter_boolean_mismatch_owner(self):
        self.invoke("getter-b")

    def test_getter_number_mismatch_owner(self):
        self.invoke("getter-n")

    def test_getter_collection_mismatch_owner(self):
        self.invoke("getter-c")

    def test_owning_handoff_and_failed_outputs(self):
        self.invoke("references-errors")

    def test_live_arrays_and_dispatcher(self):
        self.invoke("arrays")
