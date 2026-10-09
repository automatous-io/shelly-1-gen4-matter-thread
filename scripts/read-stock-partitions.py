#!/usr/bin/env python3
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""
Print what Shelly's stock data partitions hold: shelly, fs_0, fs_1 and zb_storage.

File names are never used to tell which partition a file is.

  Full-chip backup   Split by the partition table stored in it at 0x10000, or by
                     --table when an 8MB backup's own table is missing.
  One partition      Identified by its contents: a JSON object at offset 0 is shelly,
                     and a LittleFS image is fs_0 or fs_1 (the data can't say which).
                     The size only hints when the contents can't decide, as with a
                     blank file, and only warns when it disagrees with them. Pass
                     --offset, the flash address it was read from, to name it exactly;
                     it is looked up in --table (a copy of the partition table, read
                     from 0x10000) or, without one, in the stock Gen4 table built in.

    python3 scripts/read-stock-partitions.py shelly-1-gen4-stock-<MAC>.bin
    python3 scripts/read-stock-partitions.py partition.bin
    python3 scripts/read-stock-partitions.py partition.bin --offset 0x700000 --table pt.bin
    python3 scripts/read-stock-partitions.py shelly-<MAC>.bin --keys

When the table names the partition, its subtype decides how it is read, and contents
that don't match stop the script with an error:

  subtype 0x88 (shelly)       JSON object at offset 0, ended by a NUL or 0xFF.
  subtype 0x82 (fs_0, fs_1)   LittleFS that fits in the partition. Lists the files
                              and prints the JSON ones. Needs littlefs-python:
                              pip install littlefs-python
  subtype 0x81 (zb_storage)   Not decoded.

Any partition may also be blank (all 0xFF). --extract DIR writes shelly.json to DIR
and each file system's files to DIR/<partition>, readable only by you.

--keys prints the shelly partition's keys with their types and lengths, the file
systems' file names and sizes, and the keys of their JSON files with types only.
It never prints a value. That output is meant to be shared.

WITHOUT --keys THE OUTPUT HOLDS SECRETS. The shelly partition holds the device's
Matter private key and Shelly Cloud key, and fs_1 holds the Wi-Fi password and
location. Do not paste it into an issue. See docs/PARTITIONS.md.
"""
import argparse, hashlib, json, os, struct, sys

FLASH_SIZE = 0x800000
PT_ADDR = 0x10000
PT_MAX = 0xC00
ENTRY_MAGIC = b"\xaa\x50"
MD5_MAGIC = b"\xeb\xeb"
DATA = 0x01
SHELLY, FS, ZIGBEE = 0x88, 0x82, 0x81
STOCK_DATA = (SHELLY, FS, ZIGBEE)

# The stock Gen4 table, the same offsets as this repository's partitions.csv.
STOCK_TABLE = [
    # label,        type, subtype, offset,   size
    ("otadata",     DATA, 0x00,    0x11000,  0x2000),
    ("nvs",         DATA, 0x02,    0x14000,  0xC000),
    ("app_0",       0x00, 0x10,    0x20000,  0x300000),
    ("fs_0",        DATA, FS,      0x320000, 0xE0000),
    ("app_1",       0x00, 0x11,    0x400000, 0x300000),
    ("fs_1",        DATA, FS,      0x700000, 0xE0000),
    ("zb_storage",  DATA, ZIGBEE,  0x7E8000, 0x8000),
    ("shelly",      DATA, SHELLY,  0x7F0000, 0x10000),
]


class Mismatch(Exception):
    """The contents don't match the partition the table says they are."""


def warn(msg):
    print(f"warning: {msg}", file=sys.stderr)


def parse_table(raw):
    """Entries from an ESP-IDF partition table, or None if there is none.

    Stops at the first entry without the entry magic. If an MD5 entry follows the
    entries, it must match them, or the table is rejected as corrupt.
    """
    entries = []
    for off in range(0, min(len(raw), PT_MAX), 32):
        entry = raw[off:off + 32]
        if len(entry) < 32:
            break
        if entry[:2] == MD5_MAGIC:
            if entry[16:32] != hashlib.md5(raw[:off]).digest():
                sys.exit("partition table MD5 does not match its entries: the table is corrupt")
            break
        if entry[:2] != ENTRY_MAGIC:
            break
        ptype, subtype, offset, size, label = struct.unpack_from("<BBII16s", entry, 2)
        entries.append((label.rstrip(b"\x00").decode("ascii", "replace"), ptype, subtype, offset, size))
    return entries or None


def load_table(path):
    with open(path, "rb") as fh:
        table = parse_table(fh.read())
    if table is None:
        sys.exit(f"{path} holds no partition table")
    return table


def is_blank(data):
    return bool(data) and data.count(0xFF) == len(data)


def is_littlefs(data):
    return data[8:16] == b"littlefs"


def is_json_object(data):
    return data.lstrip(b" \t\r\n")[:1] == b"{"


def json_end(data):
    """Length of the JSON text at offset 0: up to the first NUL or 0xFF."""
    ends = [i for i in (data.find(b"\x00"), data.find(b"\xff")) if i >= 0]
    return min(ends) if ends else len(data)


def write_private(root, rel, content):
    """Write content to root/rel, readable only by its owner. Names come from the dump,
    so one that would land outside root is skipped. Returns the path, or None."""
    root = os.path.abspath(root)
    out = os.path.abspath(os.path.join(root, rel))
    if os.path.commonpath([root, out]) != root or out == root:
        warn(f"not extracting {rel!r}: it would land outside {root}")
        return None
    os.makedirs(root, mode=0o700, exist_ok=True)
    os.makedirs(os.path.dirname(out), mode=0o700, exist_ok=True)
    fd = os.open(out, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "wb") as fh:
        fh.write(content)
    return out


def print_keys(obj, path="", indent="  ", lengths=True):
    """Each key with its type, and its length if asked, never its value."""
    items = obj.items() if isinstance(obj, dict) else ((f"[{i}]", v) for i, v in enumerate(obj))
    for key, value in items:
        name = f"{path}{key}" if str(key).startswith("[") or not path else f"{path}.{key}"
        if isinstance(value, (dict, list)) and value:
            print_keys(value, name, indent, lengths)
        elif lengths:
            print(f"{indent}{name}: {type(value).__name__}, {len(str(value))} chars")
        else:
            print(f"{indent}{name}: {type(value).__name__}")


def show_shelly(data, extract, keys):
    end = json_end(data)
    try:
        obj = json.loads(data[:end].decode("utf-8"))
    except ValueError as e:
        raise Mismatch(f"expected a JSON object at offset 0 ({e})")
    if not isinstance(obj, dict):
        raise Mismatch("expected a JSON object at offset 0")
    missing = [k for k in ("matter", "factory") if k not in obj]
    if missing:
        warn(f"JSON has no {', '.join(missing)} section; this unit's layout differs from the documented one")
    print(f"  JSON, {end} bytes:")
    if keys:
        print_keys(obj)
    else:
        print(json.dumps(obj, indent=2))
    if extract:
        out = write_private(extract, "shelly.json", data[:end])
        if out:
            print(f"  extracted to {out}")


def show_littlefs(folder, data, extract, keys):
    if not is_littlefs(data):
        raise Mismatch("expected a LittleFS superblock at offset 0")
    version, block_size, block_count = struct.unpack_from("<III", data, 20)
    used = block_size * block_count
    if not used or used > len(data):
        raise Mismatch(f"LittleFS geometry {block_count} x {block_size} bytes does not fit "
                       f"in the {len(data)}-byte partition")
    if used < len(data):
        warn(f"LittleFS geometry {block_count} x {block_size} bytes uses only part of the "
             f"{len(data)}-byte partition")
    print(f"  LittleFS {version >> 16}.{version & 0xFFFF}, {block_count} blocks of {block_size} bytes")
    try:
        from littlefs import LittleFS, LittleFSError, UserContext
    except ImportError:
        print("  install littlefs-python to list the files: pip install littlefs-python")
        return
    # mount=False: with mount=True, littlefs-python formats an image that fails to
    # mount, and a corrupt file system would be listed as empty.
    fs = LittleFS(context=UserContext(buffer=bytearray(data)),
                  block_size=block_size, block_count=block_count, mount=False)
    try:
        fs.mount()
        files = []
        for root, _, names in fs.walk("/"):
            for name in sorted(names):
                path = root.rstrip("/") + "/" + name
                with fs.open(path, "rb") as fh:
                    files.append((path, fh.read()))
    except (LittleFSError, UnicodeDecodeError) as e:
        raise Mismatch(f"LittleFS superblock found but the file system does not read back ({e})")
    extracted = False
    for path, content in files:
        print(f"  {path}  ({len(content)} bytes)")
        if extract and write_private(extract, os.path.join(folder, path.lstrip("/")), content):
            extracted = True
        if path.endswith(".json"):
            try:
                obj = json.loads(content)
            except ValueError:
                print("    (not valid JSON)")
                continue
            if not keys:
                print(json.dumps(obj, indent=2))
            elif isinstance(obj, (dict, list)):
                # No lengths: in stock's config they would give away the Wi-Fi
                # password's length and narrow down the time zone.
                print_keys(obj, indent="    ", lengths=False)
    if extracted:
        print(f"  extracted to {os.path.join(extract, folder)}")


def show(label, subtype, offset, data, extract, folder, keys):
    """Read one partition the way its subtype says, and check the contents agree."""
    where = f" at 0x{offset:x}" if offset is not None else ""
    kind = f" (data, subtype 0x{subtype:02x})" if offset is not None else ""
    print(f"{label}{kind}{where}: {len(data)} bytes")
    if is_blank(data):
        print("  blank: all 0xFF")
    elif subtype == SHELLY:
        show_shelly(data, extract, keys)
    elif subtype == FS:
        show_littlefs(folder, data, extract, keys)
    else:
        print("  not decoded")
    print()


def by_size(size):
    """Stock data partitions of this size. A hint only, never an identification."""
    return [n for n, t, st, _, sz in STOCK_TABLE if t == DATA and st in STOCK_DATA and sz == size]


def detect(data):
    """A lone partition, identified by its contents. Size only hints when the contents
    can't decide (a blank file), and only warns when it disagrees with them."""
    sized = by_size(len(data))
    hint = " or ".join(sized)
    if is_blank(data):
        if not sized:
            return "unidentified (blank, and no stock partition is this size)", None, "unknown"
        return f"probably {hint} (blank: by size only)", None, "unknown"
    if is_littlefs(data):
        if "fs_0" not in sized:
            warn(f"LittleFS, but {len(data)} bytes is not the size of fs_0 or fs_1")
        return "fs_0 or fs_1 (LittleFS; the contents can't tell which)", FS, "fs"
    if is_json_object(data):
        if "shelly" not in sized:
            warn(f"a JSON object, but {len(data)} bytes is not the size of the shelly partition")
        return "shelly (a JSON object at offset 0)", SHELLY, "shelly"
    extra = f"; its size matches {hint}" if sized else ""
    return f"unidentified (contents not recognized{extra})", None, "unknown"


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file", help="full-chip backup, or one partition")
    parser.add_argument("--offset", type=lambda v: int(v, 0),
                        help="flash address a lone partition was read from, such as 0x7f0000")
    parser.add_argument("--table", help="the partition table, read from 0x10000: to look --offset up in, "
                                        "or to split a full image whose own table is missing")
    parser.add_argument("--extract", metavar="DIR", help="write shelly.json and the LittleFS files here")
    parser.add_argument("--keys", action="store_true",
                        help="print keys, types, lengths and file names only, never a value: safe to share")
    args = parser.parse_args()

    with open(args.file, "rb") as fh:
        data = fh.read()
    own_table = None
    if args.offset is None:
        # A lone shelly or LittleFS partition is not searched for a table: its bytes at
        # 0x10000 are its own data. A full-chip backup starts with the bootloader.
        if not (is_littlefs(data) or is_json_object(data)):
            own_table = parse_table(data[PT_ADDR:PT_ADDR + PT_MAX])
        if args.table:
            if own_table:
                warn(f"{args.file} has its own partition table at 0x10000; --table is ignored")
            elif len(data) == FLASH_SIZE:
                own_table = load_table(args.table)
            else:
                sys.exit(f"{args.file} is not an 8MB full-chip backup, so --table can't split it; "
                         f"for a single partition, also pass --offset")
        elif own_table is None and len(data) == FLASH_SIZE:
            warn("8MB with no partition table at 0x10000: if this is a full-chip backup, "
                 "its table is missing; pass --table to split it")

    jobs = []  # (label, subtype, offset, bytes, extract folder)
    if own_table:
        for label, ptype, subtype, offset, size in own_table:
            if ptype != DATA or subtype not in STOCK_DATA:
                continue
            if offset + size > len(data):
                sys.exit(f"{label} runs past the end of the file: the backup is truncated")
            jobs.append((label, subtype, offset, data[offset:offset + size], label))
        if not jobs:
            sys.exit("the partition table has none of Shelly's data partitions (subtypes 0x81, 0x82, 0x88)")
    elif args.offset is not None:
        if args.table:
            table = load_table(args.table)
        else:
            warn("no --table given; looking the offset up in the built-in stock Gen4 table")
            table = STOCK_TABLE
        entry = next((e for e in table if e[3] == args.offset), None)
        if entry is None:
            sys.exit(f"no partition starts at 0x{args.offset:x}")
        label, ptype, subtype, offset, size = entry
        if ptype != DATA or subtype not in STOCK_DATA:
            sys.exit(f"{label} is not one of Shelly's data partitions")
        if len(data) != size:
            sys.exit(f"{label} at 0x{offset:x} is {size} bytes but the file is {len(data)}: "
                     f"wrong offset or a partial read")
        jobs.append((label, subtype, offset, data, label))
    else:
        label, subtype, folder = detect(data)
        if subtype is None:
            warn("pass --offset with the address it was read from to name this partition")
        jobs.append((label, subtype, None, data, folder))

    if not args.keys or args.extract:
        warn("the output holds the device's private key, cloud key and Wi-Fi password. Keep it private.")
    failed = False
    for label, subtype, offset, part, folder in jobs:
        try:
            show(label, subtype, offset, part, args.extract, folder, args.keys)
        except Mismatch as e:
            print(f"  error: {e}\n")
            failed = True
    if failed:
        if own_table or args.offset is not None:
            sys.exit("contents do not match the partition table: check the offset and the file")
        sys.exit("contents are not the partition they look like: the file may be partial or corrupt")


if __name__ == "__main__":
    main()
