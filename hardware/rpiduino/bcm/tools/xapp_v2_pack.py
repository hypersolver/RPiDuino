#!/usr/bin/env python3
"""Pack relocatable ARM32 objects into an XD OS XAPP V2 module (C3).

    xapp_v2_pack.py --kernel kernel8-32.elf --image kernel.img \
                    --out APP.XAPP obj1.o obj2.o ...
    xapp_v2_pack.py --symbols syms.txt --fingerprint 0 --out APP.XAPP objs...

Objects must define `xapp_entry` (int32_t xapp_entry(const XdosHostApiV1 *))
and should be compiled with -marm -mword-relocations (no MOVW/MOVT for
module-internal addresses). Undefined symbols are bound to the kernel's
absolute addresses, exactly like V1.

Pipeline: link at XDOS_XAPP_V2_LINK_BASE with --emit-relocs, classify every
output relocation by its symbol (SHN_ABS = kernel binding, otherwise module):
  ABS32/TARGET1 -> module: XDOS_RELOC_ABS32_INTERNAL   kernel: nothing
  CALL/JUMP24/PC24 -> module: nothing (PC-relative)    kernel: XDOS_RELOC_BRANCH_EXTERNAL
  REL32/PREL31 -> module: nothing                      kernel: rejected
  MOVW/MOVT_ABS -> module: rejected                    kernel: nothing
  NONE/V4BX: ignored; anything else: rejected.
"""
import argparse
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

LINK_BASE = 0x400000
SLOT_BYTES = 320 * 1024
HEADER_BYTES = 48
VENEER_SLOTS = 32          # ARM->Thumb veneers for B/conditional branches to Thumb kernel code
ABS32_INTERNAL, BRANCH_EXTERNAL = 1, 2

R_ARM_NONE, R_ARM_PC24, R_ARM_ABS32, R_ARM_REL32 = 0, 1, 2, 3
R_ARM_CALL, R_ARM_JUMP24, R_ARM_TARGET1, R_ARM_V4BX = 28, 29, 38, 40
R_ARM_PREL31, R_ARM_MOVW_ABS_NC, R_ARM_MOVT_ABS = 42, 43, 44
NAMES = {0: "NONE", 1: "PC24", 2: "ABS32", 3: "REL32", 28: "CALL", 29: "JUMP24",
         38: "TARGET1", 40: "V4BX", 42: "PREL31", 43: "MOVW_ABS_NC", 44: "MOVT_ABS"}


class PackError(RuntimeError):
    pass


def run(*args, **kw):
    return subprocess.run([str(a) for a in args], check=True, **kw)


def undefined(objects):
    out = run("arm-none-eabi-nm", "-u", *objects, capture_output=True, text=True).stdout
    names = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == "U":
            names.add(parts[1])
    return names


def defined(objects):
    out = run("arm-none-eabi-nm", "-g", "--defined-only", *objects,
              capture_output=True, text=True).stdout
    return {line.split()[-1] for line in out.splitlines() if len(line.split()) == 3}


def kernel_symbols(elf):
    """Global kernel symbols with their real st_value: Thumb functions keep
    bit 0 set (nm would clear it), so calls and function pointers interwork."""
    table = {}
    with open(elf, "rb") as f:
        for sym in ELFFile(f).get_section_by_name(".symtab").iter_symbols():
            if sym["st_info"]["bind"] in ("STB_GLOBAL", "STB_WEAK") and sym["st_shndx"] != "SHN_UNDEF" \
                    and sym.name:
                table[sym.name] = sym["st_value"]
    return table


def kernel_fingerprint(elf_symbols, image):
    raw = Path(image).read_bytes()
    return zlib.crc32(raw[:(elf_symbols["_etext"] & ~1) - (elf_symbols["_start"] & ~1)])


def linker_script(bindings, fingerprint, bound):
    flags = 1 if bound else 0
    checksum = fingerprint if bound else 0
    return f"""OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(xapp_entry)
{bindings}
SECTIONS {{
    . = 0x{LINK_BASE:x};
    .header : {{ LONG(0x50414458) LONG(2) LONG({HEADER_BYTES})
        LONG(0) LONG(xapp_entry - 0x{LINK_BASE:x}) LONG({flags}) LONG(0x{checksum:x})
        LONG(0x{LINK_BASE:x}) LONG(image_end - 0x{LINK_BASE:x}) LONG(bss_end - image_end)
        LONG(0) LONG(0) }}
    .text : {{ *(.text*) *(.rodata*) . = ALIGN(4); xapp_v2_veneers = .; . += {VENEER_SLOTS * 8}; }}
    .data : {{ *(.data*) . = ALIGN(4); }}
    image_end = .;
    .bss (NOLOAD) : {{ *(.bss*) *(COMMON) . = ALIGN(4); }}
    bss_end = .;
    ASSERT(bss_end - 0x{LINK_BASE:x} <= 0x{SLOT_BYTES:x}, "XAPP V2 exceeds one slot")
    /DISCARD/ : {{ *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) *(.comment*) *(.note*)
                  *(.ARM.attributes*) }}
}}
"""


def classify(elf_path):
    """Return (relocs, report) for the linked ELF, raising PackError on unsupported ones."""
    relocs, rejected, counts = [], [], {}
    with open(elf_path, "rb") as f:
        elf = ELFFile(f)
        load_sections = {}
        for sec in elf.iter_sections():
            if sec["sh_flags"] & 2 and sec["sh_type"] != "SHT_NOBITS":       # SHF_ALLOC
                load_sections[sec.name] = sec
            if sec["sh_flags"] & 2 and sec.name not in (".header", ".text", ".data", ".bss"):
                if sec["sh_size"]:
                    raise PackError(f"unexpected allocatable section {sec.name}")
        for sec in elf.iter_sections():
            if not isinstance(sec, RelocationSection):
                continue
            if sec.is_RELA():
                raise PackError(f"{sec.name}: RELA not expected for ARM32")
            target = elf.get_section(sec["sh_info"])
            if not (target["sh_flags"] & 2):
                continue                               # debug sections etc.
            symtab = elf.get_section(sec["sh_link"])
            for r in sec.iter_relocations():
                rtype = r["r_info_type"]
                sym = symtab.get_symbol(r["r_info_sym"])
                external = sym["st_shndx"] == "SHN_ABS"
                where = r["r_offset"]
                name = sym.name or f"<section {sym['st_shndx']}>"
                key = (NAMES.get(rtype, str(rtype)), "kernel" if external else "module")
                counts[key] = counts.get(key, 0) + 1
                if rtype in (R_ARM_NONE, R_ARM_V4BX):
                    continue
                if where & 3 or where < LINK_BASE + HEADER_BYTES:
                    rejected.append(f"0x{where:x} {NAMES.get(rtype, rtype)} {name}: bad site")
                    continue
                if rtype in (R_ARM_ABS32, R_ARM_TARGET1):
                    if not external:
                        relocs.append((where - LINK_BASE, ABS32_INTERNAL))
                elif rtype in (R_ARM_CALL, R_ARM_JUMP24, R_ARM_PC24):
                    if external:
                        relocs.append((where - LINK_BASE, BRANCH_EXTERNAL))
                elif rtype in (R_ARM_REL32, R_ARM_PREL31):
                    if external:
                        rejected.append(f"0x{where:x} {NAMES[rtype]} -> kernel {name}")
                elif rtype in (R_ARM_MOVW_ABS_NC, R_ARM_MOVT_ABS):
                    if not external:
                        rejected.append(f"0x{where:x} {NAMES[rtype]} -> module {name} "
                                        "(compile with -mword-relocations)")
                else:
                    rejected.append(f"0x{where:x} type {rtype} {name}: unsupported")
    if rejected:
        raise PackError("unsupported relocations:\n  " + "\n  ".join(rejected[:20]))
    relocs.sort()
    if len({off for off, _ in relocs}) != len(relocs):
        raise PackError("two relocations at one site")
    return relocs, counts


def interworking(image, relocs, symbols, imports, elf):
    """ARM code calling Thumb kernel functions (libgcc, libm): BL -> BLX(imm);
    B / conditional BL -> branch to a local `ldr pc,[pc,#-4]; .word target|1`
    veneer (no relocation needed: target is an absolute kernel address)."""
    thumb = {symbols[n] & ~1 for n in imports if symbols[n] & 1}
    if not thumb:
        return image, relocs, 0
    veneer_base = None
    with open(elf, "rb") as f:
        for sym in ELFFile(f).get_section_by_name(".symtab").iter_symbols():
            if sym.name == "xapp_v2_veneers":
                veneer_base = sym["st_value"] - LINK_BASE
    veneers, out, count = {}, [], 0
    for off, kind in relocs:
        if kind != BRANCH_EXTERNAL:
            out.append((off, kind))
            continue
        word = struct.unpack_from("<I", image, off)[0]
        target = (LINK_BASE + off + 8 + ((((word & 0xFFFFFF) ^ 0x800000) - 0x800000) << 2)) & 0xFFFFFFFF
        if word >> 28 == 0xF:                       # already BLX(imm): Thumb-correct
            out.append((off, kind))
            continue
        if target not in thumb:
            out.append((off, kind))
            continue
        count += 1
        if word >> 24 == 0xEB:                      # BL, always: becomes BLX(imm), still external
            disp = target - (LINK_BASE + off + 8)
            struct.pack_into("<I", image, off, 0xFA000000 | ((disp >> 1) & 1) << 24 | ((disp >> 2) & 0xFFFFFF))
            out.append((off, kind))
            continue
        if veneer_base is None:
            raise PackError("Thumb tail call but no veneer area")
        if target not in veneers:
            if len(veneers) >= VENEER_SLOTS:
                raise PackError(f"more than {VENEER_SLOTS} Thumb tail-call targets")
            v = veneer_base + 8 * len(veneers)
            struct.pack_into("<II", image, v, 0xE51FF004, target | 1)   # ldr pc,[pc,#-4]; .word
            veneers[target] = v
        disp = veneers[target] - (off + 8)          # module-internal: no relocation
        struct.pack_into("<I", image, off, (word & 0xFF000000) | ((disp >> 2) & 0xFFFFFF))
    return image, out, count


def pack(objects, symbols, fingerprint, out, bound=True, keep=None):
    objects = [Path(o) for o in objects]
    if "xapp_entry" not in defined(objects):
        raise PackError("objects do not define xapp_entry")
    # Unwind personality routines are referenced only from .ARM.exidx, which the
    # linker script discards (same as V1, which strips them after linking).
    imports = {n for n in undefined(objects) - defined(objects)
               if not n.startswith("__aeabi_unwind_cpp_pr")}
    missing = sorted(imports - symbols.keys())
    if missing:
        raise PackError(f"kernel is missing imports {missing}")
    bindings = "\n".join(f"{n} = 0x{symbols[n]:x};" for n in sorted(imports))
    work = Path(keep) if keep else Path(tempfile.mkdtemp(prefix="xappv2-"))
    work.mkdir(parents=True, exist_ok=True)
    script = work / "xapp_v2.ld"
    script.write_text(linker_script(bindings, fingerprint, bound), encoding="ascii")
    elf = work / (Path(out).stem + ".elf")
    run("arm-none-eabi-ld", "--gc-sections", "--emit-relocs", "--no-warn-rwx-segments",
        "-T", script, *objects, "-o", elf)
    left = [l.split()[-1] for l in run("arm-none-eabi-nm", "-u", elf, capture_output=True,
            text=True).stdout.splitlines() if l.strip()]
    left = [n for n in left if not n.startswith("__aeabi_unwind_cpp_pr")]
    if left:
        raise PackError(f"unresolved symbols after link: {left[:10]}")
    relocs, counts = classify(elf)
    raw = work / (Path(out).stem + ".bin")
    run("arm-none-eabi-objcopy", "-O", "binary", "--only-section=.header",
        "--only-section=.text", "--only-section=.data", elf, raw)
    image = bytearray(raw.read_bytes())
    fields = list(struct.unpack_from("<4s11I", image, 0))
    magic, abi, head, _, entry, flags, csum, base, load, bss, _, _ = fields
    if len(image) < load:
        image += b"\0" * (load - len(image))       # objcopy drops trailing zero fill
    image, relocs, interwork = interworking(image, relocs, symbols, imports, elf)
    if (magic, abi, head, base) != (b"XDAP", 2, HEADER_BYTES, LINK_BASE) or load != len(image):
        raise PackError(f"header mismatch after link (load={load}, image={len(image)})")
    reloc_offset = (len(image) + 3) & ~3
    relocs = sorted(relocs)
    image += b"\0" * (reloc_offset - len(image))
    for off, kind in relocs:
        image += struct.pack("<II", off, kind)
    struct.pack_into("<I", image, 12, len(image))          # image_bytes
    struct.pack_into("<II", image, 40, reloc_offset, len(relocs))
    Path(out).write_bytes(bytes(image))
    return {"load": load, "bss": bss, "entry": entry, "relocs": len(relocs),
            "abs32": sum(1 for _, k in relocs if k == ABS32_INTERNAL),
            "branch": sum(1 for _, k in relocs if k == BRANCH_EXTERNAL),
            "counts": counts, "file": len(image), "elf": str(elf), "interwork": interwork}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--kernel", help="kernel8-32.elf (symbols)")
    ap.add_argument("--image", help="kernel image for the fingerprint (with --kernel)")
    ap.add_argument("--symbols", help="text file 'name 0xaddress' instead of --kernel")
    ap.add_argument("--fingerprint", type=lambda s: int(s, 0), default=None)
    ap.add_argument("--unbound", action="store_true", help="do not bind to a kernel fingerprint")
    ap.add_argument("--keep", help="keep intermediate files here")
    ap.add_argument("--out", required=True)
    ap.add_argument("objects", nargs="+")
    a = ap.parse_args()
    if a.kernel:
        symbols = kernel_symbols(a.kernel)
        fp = kernel_fingerprint(symbols, a.image) if a.image else (a.fingerprint or 0)
    elif a.symbols:
        symbols = {n: int(v, 0) for n, v in (l.split() for l in Path(a.symbols).read_text().splitlines() if l.strip())}
        fp = a.fingerprint or 0
    else:
        ap.error("--kernel or --symbols required")
    try:
        info = pack(a.objects, symbols, fp, a.out, bound=not a.unbound, keep=a.keep)
    except PackError as e:
        print(f"xapp_v2_pack: {e}", file=sys.stderr)
        return 1
    print(f"{Path(a.out).name}: {info['file']} bytes, load {info['load']}, bss {info['bss']}, "
          f"relocs {info['relocs']} (abs32 {info['abs32']}, branch {info['branch']}), "
          f"fingerprint {fp:08x}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
