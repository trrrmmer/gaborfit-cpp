"""Verify the frozen numerical and interface sources on every CI platform."""
from pathlib import Path
import hashlib
root=Path(__file__).resolve().parents[1]
for line in (root/"SOURCE_HASHES.sha256").read_text().splitlines():
    expected,name=line.split(None,1);path=root/name.strip()
    actual=hashlib.sha256(path.read_bytes()).hexdigest()
    if actual!=expected:raise SystemExit(f"hash mismatch: {name}: {actual} != {expected}")
print("frozen source hashes verified")
