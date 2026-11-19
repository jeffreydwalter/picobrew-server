#!/usr/bin/env python3
r"""Reproduce the 19 instruction-level cases from the owner's bootloader audit.

Supply your own extracted /sys/mcuimg.bin; private firmware is not bundled.
Only the exact audited 14,956-byte image is accepted. Install the existing
requirements-test.txt (Unicorn 2.1.4), then run, for example:

    python tools/audit_device_bootloader.py --image /path/to/mcuimg.bin \
        --output /path/to/new-bootloader-audit.json

The actual relocated Thumb instructions execute in Unicorn. Hardware setup and
SimpleLink filesystem calls are stubbed. Image1/image2/image3 are synthetic
buffers with the observed lengths/vectors; their contents are never executed.
All boot-record writes affect only an in-memory dictionary. This tool does not
open USB, modify the input image, or prove physical boot, filesystem mirror
selection, interrupted-write recovery, or safe appliance operation.

Without --output, deterministic JSON is written to stdout. An existing output
file is never replaced. Progress and errors go to stderr.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys


IMAGE_SIZE = 14956
IMAGE_SHA256 = "67e22913fd7805a84d965928198f7d8cb795af125b81f1e0c29479a84980b975"
SRAM_BASE = 0x20000000
SRAM_SIZE = 0x40000
RELOCATED_FILE_OFFSET = 0x100
RELOCATED_RESET = 0x20000B21
RELOCATED_STACK = 0x20003BA8
APPLICATION_BASE = 0x20004000
NORMAL = 0xABCDDCBA
READY = 0x56788765
TESTING = 0x12344321
BOOT_INFO = "/sys/mcubootinfo.bin"
INSTRUCTION_LIMIT = 10000
REQUIRED_UNICORN_VERSION = "2.1.4"

# These numbers describe the reviewed files, not additional binary content.
# Slot3 uses the main-image fixture solely to exercise filename dispatch.
SYNTHETIC_IMAGES = {
    1: (50504, 0x20019688, 0x2000AF29),
    2: (162792, 0x200365B8, 0x20018CD1),
    3: (162792, 0x200365B8, 0x20018CD1),
}

ADDRESS_EVIDENCE = {
    "outer_reset": "0x20004068",
    "outer_relocation_loop": "0x20004028",
    "outer_relocation_source": "0x20004100",
    "outer_relocation_destination": "0x20000000",
    "outer_relocation_copy_bytes": 16384,
    "relocated_main": "0x200008b4",
    "normal_selector_dispatch": ["0x200009ae", "0x200009d4"],
    "trial_state_write_and_dispatch": ["0x200009da", "0x200009fa"],
    "unconfirmed_trial_rollback": ["0x20000a00", "0x20000a34"],
    "missing_boot_info_initialization": ["0x20000926", "0x20000996"],
    "boot_info_write_helper": "0x20000800",
    "application_loader": "0x2000085c",
    "application_vector_jump": "0x2000084c",
    "selected_image_failure_loop": "0x20000a38",
    "no_image_initialization_loop": "0x20000988",
}


class AuditError(Exception):
    pass


def validate_image(image):
    digest = hashlib.sha256(image).hexdigest()
    if len(image) != IMAGE_SIZE or digest != IMAGE_SHA256:
        raise AuditError(
            f"This audit is specific to {IMAGE_SIZE} bytes, SHA256 {IMAGE_SHA256}; "
            f"received {len(image)} bytes, SHA256 {digest}"
        )


def load_unicorn():
    try:
        import unicorn
        from unicorn import arm_const
    except ImportError as exc:
        raise AuditError("Install requirements-test.txt to obtain Unicorn 2.1.4") from exc
    if unicorn.__version__ != REQUIRED_UNICORN_VERSION:
        raise AuditError(
            f"This reproducible audit requires Unicorn {REQUIRED_UNICORN_VERSION}; "
            f"found {unicorn.__version__}"
        )
    return unicorn, arm_const


def boot_record(selector, magic):
    # The reserved bytes are deliberately retained across trial transitions.
    return bytes((selector, 0x3D, 0x00, 0x20)) + struct.pack("<I", magic)


def synthetic_image(slot):
    size, stack, reset = SYNTHETIC_IMAGES[slot]
    return struct.pack("<II", stack, reset) + bytes(size - 8)


def execute_case(image, unicorn, regs, name, selector=1, magic=NORMAL,
                 existing=(1, 2, 3), boot_info_present=True,
                 fail_info=False, fail_read=False):
    """Run actual loader instructions, intercepting only services and terminal states."""
    cpu = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_THUMB)
    cpu.mem_map(SRAM_BASE, SRAM_SIZE)
    cpu.mem_write(SRAM_BASE, image[RELOCATED_FILE_OFFSET:])
    cpu.mem_map(0x4402D000, 0x1000)  # Chip-identification register; ordinary variant.
    cpu.reg_write(regs.UC_ARM_REG_SP, RELOCATED_STACK)
    files = {f"/sys/mcuimg{slot}.bin": synthetic_image(slot) for slot in existing}
    if boot_info_present:
        files[BOOT_INFO] = boot_record(selector, magic)
    handles = {}
    output = {
        "name": name,
        "initial_selector": selector if boot_info_present else None,
        "initial_magic": f"{magic:08x}" if boot_info_present else None,
        "image_opens": [],
        "writes": [],
        "reads": [],
    }

    def read_string(address):
        return bytes(cpu.mem_read(address, 100)).split(b"\0", 1)[0].decode("ascii")

    def return_value(value):
        cpu.reg_write(regs.UC_ARM_REG_R0, value & 0xFFFFFFFF)
        cpu.reg_write(regs.UC_ARM_REG_PC, cpu.reg_read(regs.UC_ARM_REG_LR))

    def instruction_hook(_cpu, address, _size, _data):
        r0, r1, r2, r3 = (
            cpu.reg_read(register) for register in (
                regs.UC_ARM_REG_R0, regs.UC_ARM_REG_R1,
                regs.UC_ARM_REG_R2, regs.UC_ARM_REG_R3,
            )
        )
        if address in (0x20002610, 0x20002600, 0x200026E8, 0x20002B20,
                       0x20000AD0, 0x20000B60, 0x20000C9C):
            # Hardware initialization, sl_Start and sl_Stop.
            return_value(0)
        elif address == 0x20001D84:
            # FS create-mode helper; the fake recognizes any non-read/write mode.
            output["create_args"] = [r0, r1]
            return_value(0x2000)
        elif address == 0x20001DDC:  # sl_FsOpen(name, mode, token*, handle*)
            path = read_string(r0)
            if path.startswith("/sys/mcuimg"):
                output["image_opens"].append(path)
            if path not in files and r1 in (0, 0x1000):
                return_value(-11)
                return
            files.setdefault(path, b"")
            handle = len(handles) + 1
            handles[handle] = path
            cpu.mem_write(r3, struct.pack("<I", handle))
            return_value(0)
        elif address == 0x20001E34:  # sl_FsClose
            return_value(0)
        elif address == 0x20001FA0:  # sl_FsGetInfo(name, token, info*)
            path = read_string(r0)
            if path not in files or (fail_info and path != BOOT_INFO):
                return_value(-11)
                return
            size = len(files[path])
            cpu.mem_write(r2, struct.pack("<III", 0, size, size))
            return_value(0)
        elif address == 0x20001E98:  # sl_FsRead(handle, offset, buffer, length)
            path = handles[r0]
            output["reads"].append({
                "path": path, "offset": r1, "destination": f"{r2:08x}", "length": r3,
            })
            if fail_read and path != BOOT_INFO:
                return_value(-11)
                return
            payload = files[path][r1:r1 + r3]
            cpu.mem_write(r2, payload)
            return_value(len(payload))
        elif address == 0x20001F1C:  # sl_FsWrite; emulator dictionary only.
            path = handles[r0]
            payload = bytes(cpu.mem_read(r2, r3))
            files[path] = files[path][:r1] + payload + files[path][r1 + r3:]
            output["writes"].append({"path": path, "offset": r1, "bytes": payload.hex()})
            return_value(len(payload))
        elif address == 0x2000084C:
            # Stop before entering the synthetic application's vectors.
            output["outcome"] = "vector_jump"
            output["jump_base"] = f"{r0:08x}"
            output["vectors"] = [
                f"{value:08x}" for value in struct.unpack("<II", cpu.mem_read(r0, 8))
            ]
            cpu.emu_stop()
        elif address in (0x20000A38, 0x20000988):
            output["outcome"] = "spin"
            output["spin_at"] = f"{address:08x}"
            cpu.emu_stop()

    cpu.hook_add(unicorn.UC_HOOK_CODE, instruction_hook)
    cpu.emu_start(RELOCATED_RESET, 0, count=INSTRUCTION_LIMIT)
    if "outcome" not in output:
        raise AuditError(f"{name}: did not reach an expected terminal state")
    output["final_bootinfo"] = files.get(BOOT_INFO, b"").hex()
    return output


def check(case, condition, explanation):
    if not condition:
        raise AuditError(f"{case['name']}: {explanation}; trace={json.dumps(case)}")


def selected_image(selector):
    return 2 if selector == 1 else (3 if selector == 2 else 1)


def run_audit(image):
    validate_image(image)
    unicorn, regs = load_unicorn()
    cases = []

    def run(name, **options):
        case = execute_case(image, unicorn, regs, name, **options)
        cases.append(case)
        return case

    def expect_image(case, slot):
        check(case, case["outcome"] == "vector_jump", "expected a vector jump")
        check(case, case["image_opens"] == [f"/sys/mcuimg{slot}.bin"],
              "unexpected image selection or fallback")
        check(case, case["jump_base"] == f"{APPLICATION_BASE:08x}",
              "unexpected application load address")

    for selector in (0, 1, 2, 3, 255):
        case = run(f"normal-selector-{selector}", selector=selector)
        expect_image(case, selected_image(selector))
        check(case, not case["writes"], "normal selection wrote boot metadata")

    for selector in (0, 1, 2):
        case = run(f"trial-ready-selector-{selector}", selector=selector, magic=READY)
        expect_image(case, 3 if selector == 1 else 2)
        check(case, case["final_bootinfo"] == boot_record(selector, TESTING).hex(),
              "trial state did not retain selector/reserved bytes and set testing magic")
        case = run(f"unconfirmed-trial-selector-{selector}", selector=selector, magic=TESTING)
        expect_image(case, selected_image(selector))
        check(case, case["final_bootinfo"] == boot_record(selector, NORMAL).hex(),
              "rollback did not retain selector/reserved bytes and restore normal magic")

    case = run("invalid-magic", magic=0)
    check(case, case["outcome"] == "spin" and not case["image_opens"] and not case["writes"],
          "invalid magic should neither load an image nor update the boot record")
    case = run("selected-image-missing", selector=2, existing=(1, 2))
    check(case, case["outcome"] == "spin" and case["image_opens"] == ["/sys/mcuimg3.bin"],
          "missing selected image unexpectedly fell back")
    case = run("selected-image-info-error", fail_info=True)
    check(case, case["outcome"] == "spin" and case["image_opens"] == ["/sys/mcuimg2.bin"],
          "selected-image GetInfo error unexpectedly fell back")
    case = run("selected-image-read-error", fail_read=True)
    check(case, case["outcome"] == "vector_jump" and case["vectors"] == ["00000000", "00000000"],
          "read-error path no longer reaches the unchecked vector jump")

    for existing in ((1, 2, 3), (2, 3), (3,), ()):
        suffix = "-".join(map(str, existing)) or "none"
        case = run(f"bootinfo-missing-images-{suffix}", existing=existing, boot_info_present=False)
        if existing:
            expect_image(case, existing[0])
            record = bytes.fromhex(case["final_bootinfo"])
            check(case, len(record) == 8 and record[0] == existing[0] - 1
                  and struct.unpack("<I", record[4:])[0] == NORMAL,
                  "metadata initialization selected the wrong available image")
        else:
            check(case, case["outcome"] == "spin" and case["spin_at"] == "20000988",
                  "missing metadata with no images reached an unexpected state")

    return {
        "audit_revision": 1,
        "passed": True,
        "case_count": len(cases),
        "hardware_executed": False,
        "input_image": {"size_bytes": IMAGE_SIZE, "sha256": IMAGE_SHA256},
        "emulator": {"name": "Unicorn", "version": unicorn.__version__,
                     "architecture": "ARM Thumb", "instruction_limit_per_case": INSTRUCTION_LIMIT},
        "fixtures": {
            "application_images": "Synthetic zero-filled buffers; only length and initial vectors are populated",
            "slot3": "Uses the same synthetic length/vectors as slot2; no slot3 image was present in the reviewed backup",
            "boot_record_reserved_bytes": "3d0020",
            "filesystem_writes": "In-memory dictionary only",
        },
        "scope": "Actual relocated loader instructions with hardware and SimpleLink filesystem services stubbed",
        "limitations": [
            "Outer relocation is disassembly evidence; emulation starts at the relocated reset vector",
            "Application payloads are not executed and do not contain private firmware",
            "Filesystem mirror selection, electrical behavior, and interrupted-write recovery are not emulated",
            "No flash-space allocation or firmware installation is attempted",
        ],
        "address_evidence": ADDRESS_EVIDENCE,
        "cases": cases,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--image", type=Path, required=True, help="Externally supplied extracted /sys/mcuimg.bin")
    parser.add_argument("--output", type=Path, help="New JSON report path; default: stdout")
    args = parser.parse_args(argv)
    try:
        path = args.image.expanduser()
        if path.stat().st_size != IMAGE_SIZE:
            raise AuditError(f"Expected a {IMAGE_SIZE}-byte bootloader; no emulation was run")
        result = run_audit(path.read_bytes())
        serialized = json.dumps(result, indent=2, sort_keys=True) + "\n"
        if args.output is None:
            sys.stdout.write(serialized)
        else:
            with args.output.expanduser().open("x", encoding="utf-8") as output:
                output.write(serialized)
        print(f"PASS: {result['case_count']} actual-instruction cases; no physical device access", file=sys.stderr)
        return 0
    except (AuditError, OSError) as exc:
        print(f"AUDIT FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
