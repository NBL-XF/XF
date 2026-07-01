# File: comments.py
#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import argparse
import re
import subprocess


ROOT = Path(".").resolve()
COMMENTS_ROOT = ROOT / "comments"

SKIP_DIRS = {
    ".git",
    "build",
    "bin",
    "obj",
    "dist",
    "comments",
    ".DS_Store",
}

EXTS = {
    ".c",
    ".h",
    ".lua",
    ".sh",
    ".mk",
    ".md",
    ".txt",
}

NAMED_FILES = {
    "Makefile",
}

BLOCK_COMMENT_RE = re.compile(
    r"/\*(.*?)\*/|--\[\[(.*?)\]\]",
    re.DOTALL,
)

LINE_COMMENT_RE = re.compile(
    r"^\s*(//|#|--)\s?(.*)$"
)


def run_git(*args: str) -> str | None:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None

    value = result.stdout.strip()
    return value or None


def detect_version() -> str:
    exact_tag = run_git("describe", "--tags", "--exact-match")
    if exact_tag:
        return exact_tag

    branch = run_git("rev-parse", "--abbrev-ref", "HEAD")
    if branch and branch != "HEAD":
        return branch

    return "current"


def sanitize_version(value: str) -> str:
    cleaned = value.strip().replace("\\", "/").strip("/")
    cleaned = re.sub(r"[^A-Za-z0-9._/-]+", "-", cleaned)
    cleaned = re.sub(r"/+", "/", cleaned)
    return cleaned or "current"


def should_scan(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return False

    if path.name in NAMED_FILES:
        return True

    return path.suffix in EXTS


def clean_block(text: str) -> str:
    lines = text.splitlines()
    cleaned: list[str] = []

    for line in lines:
        stripped = re.sub(r"^\* ?", "", line.strip())
        cleaned.append(stripped.rstrip())

    return "\n".join(cleaned).strip()


def extract_comments(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    comments: list[str] = []

    for match in BLOCK_COMMENT_RE.finditer(text):
        block = match.group(1) or match.group(2) or ""
        block = clean_block(block)
        if block:
            comments.append(block)

    line_group: list[str] = []

    for line in text.splitlines():
        match = LINE_COMMENT_RE.match(line)

        if match:
            body = match.group(2).rstrip()

            if body.startswith("!/"):
                continue

            line_group.append(body)
            continue

        if line_group:
            comments.append("\n".join(line_group).strip())
            line_group = []

    if line_group:
        comments.append("\n".join(line_group).strip())

    return [comment for comment in comments if comment]


def out_path_for(src: Path, out_dir: Path) -> Path:
    rel = src.relative_to(ROOT)
    safe = "__".join(rel.parts)
    return out_dir / f"{safe}.md"


def write_comment_file(src: Path, comments: list[str], out_dir: Path, version: str) -> None:
    out = out_path_for(src, out_dir)
    out.parent.mkdir(parents=True, exist_ok=True)

    rel = src.relative_to(ROOT)

    content = [
        f"# Comments extracted from `{rel}`",
        "",
        f"Version: `{version}`",
        "",
        f"Source: `{rel}`",
        "",
    ]

    if not comments:
        content.append("_No comments found._")
    else:
        for index, comment in enumerate(comments, start=1):
            content.append(f"## Comment {index}")
            content.append("")
            content.append(comment)
            content.append("")

    out.write_text("\n".join(content).rstrip() + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract comments from source files into comments/<version>/",
    )
    parser.add_argument(
        "version",
        nargs="?",
        help="Version folder name, e.g. v1.0.3. Defaults to current git tag/branch.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    version = sanitize_version(args.version or detect_version())
    out_dir = COMMENTS_ROOT / version
    out_dir.mkdir(parents=True, exist_ok=True)

    files = sorted(
        path
        for path in ROOT.rglob("*")
        if path.is_file() and should_scan(path)
    )

    updated = 0

    for path in files:
        comments = extract_comments(path)
        write_comment_file(path, comments, out_dir, version)
        updated += 1

    print(f"Updated {updated} comment files in {out_dir.relative_to(ROOT)}/")


if __name__ == "__main__":
    main()