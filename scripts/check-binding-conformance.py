#!/usr/bin/env python3
"""Run a language binding's JSON-lines adapter against the shared corpus."""

# Copyright 2026 Biglup Labs. Licensed under the Apache License, Version 2.0.

import argparse
import json
from pathlib import Path
import subprocess
import sys


DEFAULT_CORPUS = Path(__file__).resolve().parents[1] / "lib/tests/vectors/bindings/v1"


def read_json(path):
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def load_cases(root, suites=None):
    manifest = read_json(root / "manifest.json")
    if manifest["schema_version"] != 1:
        raise ValueError("Unsupported manifest schema")
    known = {entry["suite"] for entry in manifest["files"]}
    if suites and not set(suites) <= known:
        raise ValueError("Unknown suite: " + ", ".join(sorted(set(suites) - known)))
    cases = []
    ids = set()
    for entry in manifest["files"]:
        if suites and entry["suite"] not in suites:
            continue
        path = (root / entry["file"]).resolve()
        if not path.is_relative_to(root.resolve()):
            raise ValueError("Fixture path escapes corpus")
        document = read_json(path)
        if document["schema_version"] != 1:
            raise ValueError("Unsupported fixture schema")
        if entry["format"] == "transaction":
            items = [{
                "id": document["id"], "operation": "cbor", "type": "transaction",
                "input": {"cbor": document["transaction_cbor"], "signing_seed": document["signing_seed"]},
                "expected": {
                    "cbor": document["transaction_cbor"], "body_cbor": document["body_cbor"],
                    "hash": document["transaction_id"], "verification_key": document["verification_key"],
                    "signature": document["signature"], "inspection": document["inspection"],
                },
            }]
        elif entry["format"] == "cases":
            items = document["cases"]
        else:
            raise ValueError("Unknown fixture format")
        if len(items) != entry["case_count"]:
            raise ValueError("Manifest count mismatch: " + entry["file"])
        for item in items:
            if item["id"] in ids:
                raise ValueError("Duplicate fixture ID: " + item["id"])
            ids.add(item["id"])
            request = {key: item[key] for key in ("id", "operation", "type", "input") if key in item}
            request.update(schema_version=1, suite=entry["suite"])
            if "config" in document:
                request["config"] = document["config"]
            if item["operation"] == "build":
                # Child recipes are inputs. Never send expected CBOR or other oracle values.
                request["recipes"] = [{key: recipe[key] for key in ("id", "type", "input")} for recipe in items]
            cases.append((request, {key: value for key, value in item["expected"].items() if key != "native_error"}))
    if not cases:
        raise ValueError("No cases selected")
    return cases


def difference(actual, expected, path="result"):
    if type(actual) is not type(expected):
        return path + ": value type differs"
    if isinstance(expected, dict):
        if actual.keys() != expected.keys():
            return path + ": missing or unexpected fields"
        for key in expected:
            result = difference(actual[key], expected[key], path + "." + key)
            if result:
                return result
    elif isinstance(expected, list):
        if len(actual) != len(expected):
            return path + ": array length differs"
        for index, value in enumerate(expected):
            result = difference(actual[index], value, f"{path}[{index}]")
            if result:
                return result
    elif actual != expected:
        return path + ": value differs"
    return None


def compare(cases, output):
    wanted = {request["id"]: expected for request, expected in cases}
    received = set()
    errors = []
    for line_number, line in enumerate(output.splitlines(), 1):
        if not line.strip():
            continue
        try:
            response = json.loads(line)
            identifier = response["id"]
            if not isinstance(identifier, str):
                raise ValueError("ID must be a string")
            if identifier in received:
                errors.append(identifier + ": duplicate response")
                continue
            received.add(identifier)
            if identifier not in wanted:
                errors.append(identifier + ": unexpected response")
                continue
            error = difference(response["result"], wanted[identifier])
            if error:
                errors.append(identifier + ": " + error)
        except (ValueError, KeyError, TypeError) as error:
            errors.append(f"line {line_number}: malformed adapter response ({error})")
    errors.extend(identifier + ": missing response" for identifier in wanted.keys() - received)
    return sorted(errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, default=DEFAULT_CORPUS)
    parser.add_argument("--suite", action="append", help="Select a suite for an explicitly partial run")
    parser.add_argument("--timeout", type=float, default=600, help="Adapter timeout in seconds")
    parser.add_argument("--report", type=Path, help="Write machine-readable results")
    parser.add_argument("adapter", nargs=argparse.REMAINDER, help="Adapter executable and arguments, after --")
    args = parser.parse_args()
    command = args.adapter[1:] if args.adapter[:1] == ["--"] else args.adapter
    if not command:
        parser.error("Supply an adapter command after --")
    try:
        cases = load_cases(args.corpus, args.suite)
        payload = "".join(json.dumps(request, separators=(",", ":")) + "\n" for request, _ in cases)
        result = subprocess.run(command, input=payload, text=True, capture_output=True, timeout=args.timeout, check=False)
        errors = compare(cases, result.stdout)
        if result.returncode:
            errors.append(f"Adapter exited with status {result.returncode}")
        if result.stderr:
            print(result.stderr[-4000:], file=sys.stderr)
        report = {"schema_version": 1, "full_corpus": not bool(args.suite), "cases": len(cases), "passed": not errors, "errors": errors}
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(report, indent=2))
        return 1 if errors else 0
    except (OSError, ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
        print(f"Conformance check failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
