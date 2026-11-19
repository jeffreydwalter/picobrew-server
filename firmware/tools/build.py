#!/usr/bin/env python3
"""Compile recovered application modules for Cortex-M4 with the real ARM ABI.

The archive contains only the recovered application. The separately named
validation ELF adds explicit test fixtures and links the compiler's own math
and C libraries. Neither is a complete brewer firmware image.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--arm-prefix', default=os.environ.get('PICOBREW_ARM_PREFIX'))
    parser.add_argument('--sdk-root', type=Path,
                        default=Path(os.environ.get('PICOBREW_SDK', str(ROOT / '.deps/energia'))))
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--standalone', action='store_true',
                        help='Build the local API profile without cloud services')
    parser.add_argument('--archive-only', action='store_true')
    parser.add_argument('--core-only', action='store_true',
                        help='Build portable application core without the Energia adapter')
    options = parser.parse_args()
    prefix = options.arm_prefix
    if not prefix:
        candidate = ROOT / '.deps/toolchain/bin/arm-none-eabi-gcc'
        compiler = str(candidate) if candidate.is_file() else shutil.which('arm-none-eabi-gcc')
        if not compiler:
            parser.error('ARM compiler missing. Run the dependency bootstrap or set PICOBREW_ARM_PREFIX to the arm-none-eabi- tool prefix.')
        prefix = compiler[:-3]
    gcc, ar, nm, readelf = (prefix + suffix for suffix in ['gcc', 'ar', 'nm', 'readelf'])
    if options.standalone:
        ar, nm = prefix + 'gcc-ar', prefix + 'gcc-nm'
    version = subprocess.check_output([gcc, '--version'], text=True).splitlines()[0]
    output = (options.output_dir or (ROOT / 'build/standalone/core' if options.standalone
                                    else ROOT / 'build')).resolve()
    output.mkdir(parents=True, exist_ok=True)
    flags = ['-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft', '-mabi=aapcs',
             '--specs=nano.specs', '-std=gnu11', '-Os', '-g', '-ffunction-sections', '-fdata-sections',
             '-fno-fast-math', '-ffp-contract=off', '-fexcess-precision=standard',
             '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'include')]
    if options.standalone:
        flags += ['-DPICO_STANDALONE=1', '-flto']
    sources = sorted((ROOT / 'src').rglob('*.c'))
    if not options.standalone:
        sources = [p for p in sources if not (p.name.startswith('local_') or p.name == 'http_server.c')]
    objects = []
    source_hashes = {}
    for source in sources:
        object_path = output / 'objects' / source.relative_to(ROOT / 'src').with_suffix('.o')
        object_path.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([gcc, *flags, '-c', str(source), '-o', str(object_path)], check=True)
        objects.append(object_path)
        source_hashes[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    archive = output / 'libpicobrew.a'
    # Recreate it so deleted modules cannot linger from an earlier build.
    archive.unlink(missing_ok=True)
    subprocess.run([ar, 'rcs', str(archive), *(str(p) for p in objects)], check=True)
    report = {'application_archive': archive.name, 'source_files': len(sources),
              'compiler': version,
              'flags': ['-Iinclude' if flag == '-I' + str(ROOT / 'include') else flag for flag in flags],
              'source_sha256': source_hashes,
              'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
              'full_firmware': False, 'validation_elf': None,
              'profile': 'standalone' if options.standalone else 'legacy'}
    report['header_sha256'] = {
        str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted((ROOT / 'include').rglob('*.h'))}
    if not options.core_only:
        sdk = options.sdk_root.resolve()
        if not (sdk / 'cores/cc3200/Energia.h').is_file():
            parser.error('Pinned SDK missing. Run make bootstrap or set PICOBREW_SDK. Use --core-only only for the portable application archive.')
        cxx = prefix + 'g++'
        cpp_flags = ['-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft', '-mabi=aapcs',
                     '--specs=nano.specs', '-std=gnu++11', '-Os', '-g', '-ffunction-sections', '-fdata-sections',
                     '-fno-rtti', '-fno-exceptions', '-fno-threadsafe-statics',
                     '-Wall', '-Wextra', '-Werror', '-DENERGIA', '-D__CC3200R1M1RGC__',
                     '-I' + str(ROOT / 'include')]
        if options.standalone:
            cpp_flags += ['-DPICO_STANDALONE=1', '-flto']
        for directory in ['cores/cc3200', 'variants/CC3200-LAUNCHXL', 'system',
                          'libraries/WiFi', 'libraries/SPI', 'libraries/WiFi/utility',
                          'libraries/WiFi/utility/middleware/driver',
                          'libraries/WiFi/utility/middleware/driver/hal']:
            cpp_flags += ['-isystem', str(sdk / directory)]
        adapter_source = ROOT / 'platform/energia_cc3200.cpp'
        adapter_sources = sorted(p for p in (ROOT / 'platform').glob('*.cpp')
                                 if p.name != 'firmware_main.cpp')
        if not options.standalone:
            adapter_sources = [p for p in adapter_sources if p.name not in ('energia_http_server.cpp', 'energia_history.cpp', 'energia_telemetry.cpp')]
        adapter_objects = []
        for source in adapter_sources:
            adapter_object = output / ('platform_' + source.stem + '.o')
            subprocess.run([cxx, *cpp_flags, '-c', str(source), '-o', str(adapter_object)], check=True)
            adapter_objects.append(adapter_object)
        adapter_archive = output / 'libpicobrew_energia.a'
        adapter_archive.unlink(missing_ok=True)
        subprocess.run([ar, 'rcs', str(adapter_archive), *map(str, adapter_objects)], check=True)
        report.update({'energia_adapter_archive': adapter_archive.name,
                       'energia_adapter_source_sha256': hashlib.sha256(adapter_source.read_bytes()).hexdigest(),
                       'platform_source_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in adapter_sources},
                       'energia_adapter_sha256': hashlib.sha256(adapter_archive.read_bytes()).hexdigest(),
                       'sdk_dependency_lock_sha256': hashlib.sha256((ROOT / 'dependencies/lock.json').read_bytes()).hexdigest(),
                       'sdk_interface_check': 'Real pinned Energia/TI headers; C++11 Cortex-M4 soft-float compilation with warnings as errors'})
    if not options.archive_only:
        fixture_objects = []
        fixtures = [ROOT / 'tests/validation_entry.c', *sorted(
            p for p in (ROOT / 'tests').glob('*_fixture.c')
            if not p.name.startswith(('http_', 'local_', 'energia_http')))]
        for source in fixtures:
            object_path = output / ('test_' + source.stem + '.o')
            subprocess.run([gcc, *flags, '-c', str(source), '-o', str(object_path)], check=True)
            fixture_objects.append(object_path)
        elf = output / 'application-validation.elf'
        subprocess.run([gcc, '-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft',
                        '-mabi=aapcs', '-nostartfiles', '--specs=nano.specs', '--specs=nosys.specs',
                        '-T', str(ROOT / 'tests/validation.ld'),
                        '-Wl,-Map=' + str(output / 'application-validation.map'),
                        *(str(p) for p in fixture_objects),
                        '-Wl,--whole-archive', str(archive), '-Wl,--no-whole-archive',
                        '-Wl,--start-group', '-lm', '-lc', '-lgcc', '-Wl,--end-group',
                        '-o', str(elf)], check=True)
        undefined = subprocess.check_output([nm, '--undefined-only', str(elf)], text=True).strip()
        if undefined:
            raise RuntimeError('Validation ELF has undefined symbols:\n' + undefined)
        attributes = subprocess.check_output([readelf, '-h', '-A', str(elf)], text=True)
        if 'soft-float ABI' not in attributes or 'Machine:                           ARM' not in attributes:
            raise RuntimeError('Unexpected ELF machine/float ABI:\n' + attributes)
        (output / 'elf_attributes.txt').write_text(attributes)
        report.update({'validation_elf': elf.name,
                       'test_fixture_sha256': {
                           str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in fixtures},
                       'validation_elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest(),
                       'undefined_symbols': [],
                       'runtime': 'Compiler-provided libgcc, libc, libm; no decompiled runtime bodies',
                       'validation_scope': 'Function-level image with test I/O fixtures, not an appliance boot image'})
    (output / 'build-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ['source_files', 'compiler', 'application_archive',
                                                  'validation_elf', 'full_firmware']}, indent=2))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
