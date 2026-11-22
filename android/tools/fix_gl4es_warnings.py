#!/usr/bin/env python3
"""Apply narrowly scoped fixes to the pinned, locally cloned GL4ES source.

Only the known upstream revision is accepted. Reads pristine files from Git,
checks every original line, and refuses unrelated changes. Safe to rerun.
"""
import json
import os
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
GL4ES = ROOT / "gl4es"
SPEC = pathlib.Path(__file__).with_name("gl4es_warning_fixes.json")


def git(*args):
    return subprocess.check_output(
        ["git", "-C", str(GL4ES), *args], stderr=subprocess.STDOUT
    ).decode("utf-8")


def main():
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    if not (GL4ES / ".git").exists():
        raise RuntimeError("GL4ES checkout absent; run android/setup-gl4es.ps1 first")
    revision = git("rev-parse", "HEAD").strip()
    if revision != spec["revision"]:
        raise RuntimeError("Refusing unknown GL4ES revision: " + revision)

    pending = []
    for filename, operations in spec["files"].items():
        path = GL4ES / "src" / "gl" / filename
        original = git("show", "HEAD:src/gl/" + filename)
        lines = original.splitlines(keepends=True)
        changed = set()
        for op in operations:
            i = op["line"] - 1
            if i in changed or lines[i].rstrip("\r\n") != op["before"]:
                raise RuntimeError("Pinned upstream source mismatch: " + filename + ":" + str(i + 1))
            changed.add(i)
        for op in sorted(operations, key=lambda x: x["line"], reverse=True):
            i = op["line"] - 1
            lines[i:i+1] = [x + "\n" for x in op["after"].split("\n")]
        expected = "".join(lines)
        current = path.read_text(encoding="utf-8")
        if current == expected:
            print("Already fixed:", filename)
        elif current == original:
            pending.append((path, expected))
        else:
            raise RuntimeError("Refusing to overwrite other local edits to " + str(path))

    for path, content in pending:
        fd, temp = tempfile.mkstemp(prefix=".gl4es-fix-", dir=str(path.parent))
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as out:
                out.write(content)
            os.replace(temp, path)
            print("Fixed:", path.name)
        finally:
            if os.path.exists(temp):
                os.unlink(temp)
    print("GL4ES warning fixes verified for pinned revision.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print("GL4ES warning fix failed:", exc, file=sys.stderr)
        sys.exit(1)
