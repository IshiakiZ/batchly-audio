"""Package the Windows preview and its complete corresponding source.

Run after building, testing and committing. Uses only Python's standard library.
"""
import hashlib
import argparse
import io
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
JUCE_COMMIT = "72782788ce18c2d4d760b28e0921d6ffc6431102"
OUTPUT = ROOT / "dist"


def git(directory, *arguments):
    return subprocess.check_output(["git", "-C", str(directory), *arguments])


def add_folder(archive, folder, destination):
    for path in sorted(folder.rglob("*")):
        if path.is_file():
            archive.write(path, (Path(destination) / path.relative_to(folder)).as_posix())


def add_source(archive, directory, prefix):
    # git archive exports the committed files without local settings or Git credentials.
    data = git(directory, "archive", "--format=zip", "HEAD")
    with zipfile.ZipFile(io.BytesIO(data)) as source:
        for entry in source.infolist():
            if not entry.is_dir():
                archive.writestr(prefix + entry.filename, source.read(entry.filename))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    artifacts = parser.parse_args().build_dir / "BatchlyAudio_artefacts/Release"
    if git(ROOT, "status", "--porcelain").strip():
        raise RuntimeError("Commit source changes before packaging so the source matches this preview.")
    juce = ROOT / "vendor/JUCE"
    if git(juce, "rev-parse", "HEAD").decode().strip() != JUCE_COMMIT or git(juce, "status", "--porcelain").strip():
        raise RuntimeError("The JUCE source must be clean and match the pinned framework commit.")
    OUTPUT.mkdir(exist_ok=True)
    binary_zip = OUTPUT / f"Batchly-Audio-{VERSION}-Windows-x64.zip"
    source_zip = OUTPUT / f"Batchly-Audio-{VERSION}-Source.zip"
    commit = git(ROOT, "rev-parse", "HEAD").decode().strip()
    with zipfile.ZipFile(binary_zip, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(artifacts / "Standalone/Batchly Audio.exe", "Batchly Audio.exe")
        add_folder(archive, artifacts / "VST3/Batchly Audio.vst3", "Batchly Audio.vst3")
        for name in ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"):
            archive.write(ROOT / name, name)
        archive.write(ROOT / "tools/install-vst3.ps1", "install-vst3.ps1")
        add_folder(archive, ROOT / "third-party", "third-party")
        add_folder(archive, ROOT / "docs", "docs")
        archive.writestr("SOURCE.txt", f"Batchly Audio {VERSION}\nSource commit: {commit}\n"
                         f"Complete source, including JUCE: {source_zip.name}\n"
                         "Available beside this package at https://github.com/IshiakiZ/batchly-audio/releases\n")
    with zipfile.ZipFile(source_zip, "w", zipfile.ZIP_DEFLATED) as archive:
        add_source(archive, ROOT, "BatchlyAudio/")
        add_source(archive, juce, "BatchlyAudio/vendor/JUCE/")
    checksum_lines = []
    for path in (binary_zip, source_zip):
        with zipfile.ZipFile(path) as archive:
            if archive.testzip() is not None:
                raise RuntimeError(f"Archive validation failed: {path.name}")
        checksum_lines.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}")
        print(f"Created {path.name} ({path.stat().st_size:,} bytes)")
    (OUTPUT / "SHA256SUMS.txt").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
