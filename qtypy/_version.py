from pathlib import Path
import re

_CMAKE_LISTS = Path(__file__).resolve().parents[1] / "CMakeLists.txt"


def _read_version_from_cmake() -> str:
    text = _CMAKE_LISTS.read_text(encoding="utf-8")
    match = re.search(
        r"set\(QUERYOSITY_MAJOR_VERSION\s+(\d+)\)\s*"
        r"set\(QUERYOSITY_MINOR_VERSION\s+(\d+)\)\s*"
        r"set\(QUERYOSITY_PATCH_VERSION\s+(\d+)\)",
        text,
    )
    if not match:
        raise RuntimeError("Could not determine queryosity version from CMakeLists.txt")
    return ".".join(match.groups())


__version__ = _read_version_from_cmake()

__all__ = ["__version__"]
