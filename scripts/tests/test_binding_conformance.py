"""Tests for the binding acceptance checker, not a substitute binding adapter."""
import importlib.util
import json
from pathlib import Path
import unittest
# CLI tests run trusted test code without a shell.
import subprocess  # nosec B404
import sys
import tempfile

PATH = Path(__file__).resolve().parents[1] / "check-binding-conformance.py"
SPEC = importlib.util.spec_from_file_location("conformance", PATH)
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class CheckerTests(unittest.TestCase):
    def test_requests_do_not_disclose_expectations(self):
        cases = CHECK.load_cases(CHECK.DEFAULT_CORPUS)
        self.assertGreater(len(cases), 200)
        for request, _ in cases:
            self.assertNotIn('"expected":', json.dumps(request))
            if request["operation"] == "build":
                self.assertIn("recipes", request)
                self.assertIn("config", request)

    def test_partial_run_is_explicit(self):
        cases = CHECK.load_cases(CHECK.DEFAULT_CORPUS, ["builders"])
        self.assertTrue(all(request["operation"] == "build" for request, _ in cases))
        with self.assertRaises(ValueError):
            CHECK.load_cases(CHECK.DEFAULT_CORPUS, ["typo"])

    def test_correct_results_accept_object_key_reordering(self):
        cases = [({"id": "one"}, {"a": 1, "b": ["2"]})]
        self.assertEqual(CHECK.compare(cases, '{"id":"one","result":{"b":["2"],"a":1}}'), [])

    def test_wrong_hash_and_precision_loss_fail(self):
        self.assertIsNotNone(CHECK.difference({"hash": "00"}, {"hash": "01"}))
        self.assertIsNotNone(CHECK.difference(18446744073709551616, 18446744073709551615))
        self.assertIsNotNone(CHECK.difference(1, "1"))
        self.assertIsNotNone(CHECK.difference(True, 1))

    def test_order_missing_and_extra_fields_fail(self):
        self.assertIsNotNone(CHECK.difference([2, 1], [1, 2]))
        self.assertIsNotNone(CHECK.difference({}, {"guards": []}))
        self.assertIsNotNone(CHECK.difference({"guards": [], "required_signers": []}, {"guards": []}))

    def test_missing_duplicate_unknown_and_malformed_responses_fail(self):
        cases = [({"id": "one"}, {"ok": True})]
        good = '{"id":"one","result":{"ok":true}}'
        for output in ["", good + "\n" + good, '{"id":"other","result":{}}', "not json", '{"id":"one"}', '[]']:
            with self.subTest(output=output):
                self.assertTrue(CHECK.compare(cases, output))

    def test_cli_rejects_nonzero_adapter_even_with_correct_output(self):
        # This miniature corpus tests the process contract, not real binding behavior.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "manifest.json").write_text(json.dumps({"schema_version": 1, "files": [
                {"file": "case.json", "suite": "test", "format": "cases", "case_count": 1}
            ]}))
            (root / "case.json").write_text(json.dumps({"schema_version": 1, "cases": [
                {"id": "one", "operation": "test", "input": {}, "expected": {"ok": True}}
            ]}))
            script = 'import sys; sys.stdin.read(); print(\'{"id":"one","result":{"ok":true}}\'); sys.exit(3)'
            # The interpreter, checker path, and adapter source are controlled by this test.
            result = subprocess.run(  # nosec B603
                [sys.executable, str(PATH), "--corpus", str(root), "--", sys.executable, "-c", script],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 1)
            self.assertFalse(json.loads(result.stdout)["passed"])

    def test_cli_timeout_fails(self):
        # Exercise a real adapter timeout using a fixed, local Python command.
        result = subprocess.run(  # nosec B603
            [
                sys.executable, str(PATH), "--timeout", "0.01", "--suite", "primitives", "--",
                sys.executable, "-c", "import time; time.sleep(1)",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 1)
        self.assertIn("timed out", result.stderr)

    def test_unsupported_cannot_pass_as_success(self):
        self.assertTrue(CHECK.compare([({"id": "one"}, {"passed": True})], '{"id":"one","result":{"unsupported":true}}'))


if __name__ == "__main__":
    unittest.main()
