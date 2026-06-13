#!/usr/bin/env python3

from pathlib import Path
import re

ROOT = Path(".")
OUT_DIR = Path("comments")

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

def should_scan(path: Path) -> bool:
    if any(part in SKIP_DIRS for part in path.parts):
        return False

    if path.name in NAMED_FILES:
        return True

    return path.suffix in EXTS

def clean_block(text: str) -> str:
    lines = text.splitlines()
    cleaned = []

    for line in lines:
        line = line.strip()
        line = re.sub(r"^\* ?", "", line)
        cleaned.append(line.rstrip())

    return "\n".join(cleaned).strip()

def extract_comments(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    comments = []

    for match in BLOCK_COMMENT_RE.finditer(text):
        block = match.group(1) or match.group(2) or ""
        block = clean_block(block)
        if block:
            comments.append(block)

    line_group = []

    for line in text.splitlines():
        m = LINE_COMMENT_RE.match(line)

        if m:
            body = m.group(2).rstrip()

            # Ignore shebangs.
            if body.startswith("!/"):
                continue

            line_group.append(body)
        else:
            if line_group:
                comments.append("\n".join(line_group).strip())
                line_group = []

    if line_group:
        comments.append("\n".join(line_group).strip())

    return [c for c in comments if c]

def out_path_for(src: Path) -> Path:
    rel = src.relative_to(ROOT)
    safe = "__".join(rel.parts)
    return OUT_DIR / f"{safe}.md"

def write_comment_file(src: Path, comments: list[str]) -> None:
    out = out_path_for(src)
    out.parent.mkdir(parents=True, exist_ok=True)

    rel = src.relative_to(ROOT)

    content = [
        f"# Comments extracted from `{rel}`",
        "",
        f"Source: `{rel}`",
        "",
    ]

    if not comments:
        content.append("_No comments found._")
    else:
        for i, comment in enumerate(comments, start=1):
            content.append(f"## Comment {i}")
            content.append("")
            content.append(comment)
            content.append("")

    out.write_text("\n".join(content).rstrip() + "\n", encoding="utf-8")

def main() -> None:
    OUT_DIR.mkdir(exist_ok=True)

    files = sorted(
        path for path in ROOT.rglob("*")
        if path.is_file() and should_scan(path)
    )

    updated = 0

    for path in files:
        comments = extract_comments(path)
        write_comment_file(path, comments)
        updated += 1

    print(f"Updated {updated} comment files in {OUT_DIR}/")

if __name__ == "__main__":
    main()