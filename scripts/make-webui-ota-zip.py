#!/usr/bin/env python3
#
# Copyright 2026 AUTOMATOUS.IO
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
Build a Shelly Web UI OTA zip from a Gen4 variant's build output.

Point it at a variant directory (defaults to the current one):

    python3 scripts/make-webui-ota-zip.py source/shelly-1-gen4/light
    python3 scripts/make-webui-ota-zip.py source/shelly-1pm-gen4/outlet
    python3 scripts/make-webui-ota-zip.py source/shelly-1-mini-gen4/outlet


It reads the variant and version from build/project_description.json, pulls the
bootloader, partition table, otadata and app from build/, adds an empty filesystem
image, and writes automatous-io-<hardware>-<variant>-v<version>-ota.zip, where
<hardware> is the source/<hardware> directory the variant lives in.

The Web UI OTA refuses a package whose name/compatible don't match the unit's Shelly
hardware code. That code is looked up from the variant's hardware directory (see the
HARDWARE table below) and can be overridden with --app-code/--compatible for testing 
purposes.

Every part in the zip comes from the build; no Shelly binaries are included. The zip
replaces the stock bootloader with the build's own.
"""
import argparse, datetime, hashlib, json, os, sys, tempfile, zipfile

# Shelly's internal hardware codes, keyed by the source/<hardware> directory. Only
# codes confirmed against real hardware belong here; read an unknown unit's code from
# its stock Web UI device info before adding it.
HARDWARE = {
    "shelly-1-gen4": {"app_code": "S1G4", "compatible": "S1G4*"},
    "shelly-1pm-gen4" : {"app_code": "S1PMG4", "compatible": "S1PMG4*"},
    "shelly-1-mini-gen4" : {"app_code": "Mini1G4", "compatible": "Mini1G4*"},
}
PLATFORM     = "esp32c6"
# Kept above any stock version; the device never refuses it as a downgrade. The
# real firmware version is in the app and the zip filename and not here.
MANIFEST_VER = "99.0.0"
# The stock updater compares the boot part's min_version against its running
# bootloader (logged as "Boot: cur ..., min ..., update? N") and flashes the
# bundled bootloader only when min_version is higher. Set above any stock
# version.
BOOT_MIN     = "99.0.0"
# Must match the variant's partitions.csv.
PT_ADDR  = 0x10000
NVS_SIZE = 0xC000
FS_SIZE  = 0xE0000
# No flash encryption on these units. Left true because that
# is the correct value if a unit ever ships with encryption enabled.
ENCRYPT  = True


def digest(path):
    data = open(path, "rb").read()
    return len(data), hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("variant_dir", nargs="?", default=".",
                        help="variant directory (default: current)")
    parser.add_argument("--app-code", help="Shelly code for the manifest 'name' (overrides the HARDWARE table)")
    parser.add_argument("--compatible", help="Shelly string for the manifest 'compatible' (overrides the HARDWARE table)")
    args = parser.parse_args()

    variant_dir = os.path.abspath(args.variant_dir)
    hardware = os.path.basename(os.path.dirname(variant_dir))
    codes = HARDWARE.get(hardware, {})
    app_code = args.app_code or codes.get("app_code")
    compatible = args.compatible or codes.get("compatible")
    if not app_code or not compatible:
        sys.exit(f"unknown hardware '{hardware}': add it to HARDWARE or pass "
                 f"--app-code/--compatible (read them from the unit's stock Web UI)")

    build = os.path.join(variant_dir, "build")
    desc_path = os.path.join(build, "project_description.json")
    if not os.path.isfile(desc_path):
        sys.exit(f"no build found at {build} — run `idf.py build` in {variant_dir} first")
    desc = json.load(open(desc_path))
    variant, version = desc["project_name"], desc["project_version"]

    src = {
        "bootloader.bin":      os.path.join(build, "bootloader", "bootloader.bin"),
        "partition-table.bin": os.path.join(build, "partition_table", "partition-table.bin"),
        "otadata.bin":         os.path.join(build, "ota_data_initial.bin"),
        "app.bin":             os.path.join(build, f"{variant}.bin"),
    }
    missing = [p for p in src.values() if not os.path.isfile(p)]
    if missing:
        sys.exit("missing build outputs:\n  " + "\n  ".join(missing))

    stem = f"automatous-io-{hardware}-{variant}-v{version}-ota"
    zip_out = os.path.join(variant_dir, f"{stem}.zip")

    with tempfile.TemporaryDirectory() as tmp:
        fs_img = os.path.join(tmp, "fs.img")
        with open(fs_img, "wb") as f:
            f.write(b"\xff" * FS_SIZE)
        paths = {**src, "fs.img": fs_img}

        def part(member, **extra):
            size, sha = digest(paths[member])
            return {"src": member, "size": size, "cs_sha256": sha, **extra}

        now = datetime.datetime.now(datetime.timezone.utc)
        manifest = {
            "name": app_code,
            "platform": PLATFORM,
            "version": MANIFEST_VER,
            "build_id": now.strftime("%Y%m%d-%H%M%S") + f"/{stem}",
            "build_timestamp": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "parts": {
                "boot":    part("bootloader.bin", type="boot", addr=0x0, min_version=BOOT_MIN, encrypt=ENCRYPT),
                "pt":      part("partition-table.bin", type="pt", addr=PT_ADDR, encrypt=ENCRYPT),
                "otadata": part("otadata.bin", type="otadata", ptn="otadata", encrypt=ENCRYPT),
                "nvs":     {"type": "nvs", "size": NVS_SIZE, "fill": 255, "ptn": "nvs"},
                "app":     part("app.bin", type="app", ptn="app_0", encrypt=ENCRYPT),
                "fs":      part("fs.img", type="fs", ptn="fs_0", fs_size=FS_SIZE, encrypt=ENCRYPT),
            },
            "compatible": compatible,
        }
        manifest_path = os.path.join(tmp, "manifest.json")
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)

        order = ["manifest.json", "bootloader.bin", "partition-table.bin",
                 "otadata.bin", "app.bin", "fs.img"]
        src_for = {**paths, "manifest.json": manifest_path}
        with zipfile.ZipFile(zip_out, "w", zipfile.ZIP_STORED) as z:
            for member in order:
                z.write(src_for[member], arcname=member)

    print(f"Created {os.path.basename(zip_out)} ({os.path.getsize(zip_out) / 1024 / 1024:.1f} MB)")
    print(f"  {variant} v{version}, manifest name={app_code}, version={MANIFEST_VER}")
    print(f"  boot min_version={BOOT_MIN}: this zip replaces the stock bootloader")


if __name__ == "__main__":
    main()
