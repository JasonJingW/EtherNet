#!/usr/bin/env python3
"""GCC backend implementation. Build policy and tool discovery live here.

Requires Python >= 3.9 and CMake >= 3.20 with Ninja and ARM bare-metal GCC.
Validated: Python 3.14.6, CMake 4.4.5, Ninja 1.13.2, GCC 15.2.1.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

BACKEND = Path(__file__).resolve().parent
ROOT = BACKEND.parents[1]


def find_tool(name, configured, environment):
    candidate = os.environ.get(environment) or configured
    if candidate:
        local = Path(candidate)
        if not local.is_absolute():
            local = BACKEND / local
        if local.is_file():
            return str(local.resolve())
        found = shutil.which(candidate)
        if found:
            return found
    raise ValueError(f"{name} not found. Set {environment} or edit the tools section in {BACKEND / 'build.json'}")


def run(command):
    print("> " + subprocess.list2cmdline([str(item) for item in command]), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def validate_output(output):
    cache = output / "CMakeCache.txt"
    if not cache.is_file():
        return
    for line in cache.read_text(encoding="utf-8").splitlines():
        if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
            home = Path(line.split("=", 1)[1]).resolve()
            if os.path.normcase(str(home)) == os.path.normcase(str(BACKEND)):
                return
            raise ValueError(f"Output directory belongs to a different CMake project: {home}")
    raise ValueError(f"Invalid CMake cache in {output}; use a different output directory")


def main():
    # Use consistent encoding in terminals and redirected logs.
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("configure", "build", "clean", "rebuild", "size"))
    parser.add_argument("--build-type", choices=("Debug", "Release", "RelWithDebInfo", "MinSizeRel"), default="Debug")
    parser.add_argument("--output", type=Path, help="Output directory, relative to build/gcc unless absolute")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("-D", dest="definitions", action="append", default=[], metavar="KEY=VALUE")
    args = parser.parse_args()
    try:
        if args.jobs < 1:
            raise ValueError("--jobs must be positive")
        for definition in args.definitions:
            if "=" not in definition:
                raise ValueError(f"Expected -DKEY=VALUE, got {definition}")
        output = args.output or Path("out") / args.build_type.lower()
        if not output.is_absolute():
            output = BACKEND / output
        output = output.resolve()
        validate_output(output)
        tools = json.loads((BACKEND / "build.json").read_text(encoding="utf-8"))["tools"]
        cmake = find_tool("CMake", tools["cmake"], "CMAKE")
        if args.command == "clean":
            if (output / "CMakeCache.txt").is_file():
                run([cmake, "--build", str(output), "--target", "clean"])
            else:
                print(f"Nothing to clean: {output}")
            return 0
        gcc = find_tool("ARM GCC", tools["arm_gcc"], "ARM_GCC")
        if args.command == "size":
            firmware_path = output / "firmware_path.txt"
            if not firmware_path.is_file():
                raise ValueError(f"Build the firmware first: {output}")
            firmware = Path(firmware_path.read_text(encoding="utf-8").strip())
            if not firmware.is_file():
                raise ValueError(f"Build the firmware first: {firmware}")
            size = Path(gcc).with_name("arm-none-eabi-size" + Path(gcc).suffix)
            run([str(size), str(firmware)])
            return 0
        ninja = find_tool("Ninja", tools["ninja"], "NINJA")
        configure = [cmake, "-S", str(BACKEND), "-B", str(output), "-G", "Ninja",
                     "-DCMAKE_BUILD_TYPE=" + args.build_type,
                     "-DARM_GCC_BIN=" + str(Path(gcc).parent),
                     "-DCMAKE_MAKE_PROGRAM=" + ninja]
        run(configure + ["-D" + item for item in args.definitions])
        if args.command == "configure":
            return 0
        if args.command == "rebuild":
            run([cmake, "--build", str(output), "--target", "clean"])
        run([cmake, "--build", str(output), "--target", "firmware_files", "--parallel", str(args.jobs)])
        firmware = Path((output / "firmware_path.txt").read_text(encoding="utf-8").strip())
        binary = ROOT / "build" / "build_file" / firmware.with_suffix(".bin").name
        print(f">>> Firmware output directory: {binary.parent}", flush=True)
        print(f">>> BIN file: {binary.name}", flush=True)
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"GCC build error: {error}", file=sys.stderr)
        return getattr(error, "returncode", 1)


if __name__ == "__main__":
    raise SystemExit(main())
