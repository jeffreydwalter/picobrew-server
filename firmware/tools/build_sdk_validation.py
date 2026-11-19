#!/usr/bin/env python3
"""Compile pinned Energia/CC3200 dependencies and link a target validation entry.

The default entry verifies compiler, core, WiFi, SPI, startup and runtime
integration only. It is not a PicoBrew appliance firmware implementation.
"""
from pathlib import Path
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
import shutil
import subprocess


# The pinned newlib-nano allocator dereferences its empty free list after an
# sbrk failure. Select a coherent, compiler-supplied full-newlib allocation
# family while keeping nano stdio. Unused entry sections are still collected.
ALLOCATION_MEMBERS = (
    'malloc', 'calloc', 'realloc', 'mallocr', 'callocr', 'freer', 'reallocr',
    'msizer', 'aligned_alloc', 'malign', 'malignr', 'valloc', 'vallocr',
    'pvallocr', 'cfreer', 'mallinfor', 'malloptr', 'mallstatsr',
    'reallocarray', 'reallocf')


def prepare_allocator(prefix, cc, target_flags, destination):
    """Extract native allocator objects from the same compiler's full libc."""
    library = Path(subprocess.check_output(
        [cc, *target_flags, '-print-file-name=libc.a'], text=True).strip()).resolve()
    if not library.is_file():
        raise ValueError('Selected compiler must provide full libc.a allocation objects')
    destination.mkdir(parents=True, exist_ok=True)
    objects = []
    for name in ALLOCATION_MEMBERS:
        member = 'lib_a-' + name + '.o'
        data = subprocess.check_output([prefix + 'ar', 'p', str(library), member])
        if not data.startswith(b'\x7fELF'):
            raise ValueError('Missing native allocation object: ' + member)
        path = destination / member
        path.write_bytes(data)
        objects.append({'member': member, 'path': str(path),
                        'sha256': hashlib.sha256(data).hexdigest()})
    return {'library': str(library),
            'library_sha256': hashlib.sha256(library.read_bytes()).hexdigest(),
            'objects': objects,
            'reason': 'Use one coherent full-newlib allocation family; pinned nano malloc has an empty-free-list exhaustion defect. Nano stdio remains selected.'}


def prepare_sdk(pristine, destination, patches):
    """Apply four declaration updates needed by newlib in the selected compiler.

    unsigned int and unsigned long are both 32 bits for this ARM ABI. The
    algorithm bodies are unchanged. The original dependency tree stays intact.
    """
    if destination == pristine or pristine in destination.parents:
        raise ValueError('SDK output must not be inside the source SDK')
    shutil.copytree(pristine, destination, dirs_exist_ok=True)
    records = []
    for edit in patches:
        relative, before, after = (edit[key] for key in ('file', 'before', 'after'))
        path = destination / relative
        original = path.read_bytes()
        original_digest = hashlib.sha256(original).hexdigest()
        old, new = before.encode(), after.encode()
        if original_digest == edit['patched_sha256']:
            records.append(dict(edit, input_already_patched=True))
            continue
        if original_digest != edit['original_sha256'] or original.count(old) != 1:
            raise ValueError('Unexpected pinned SDK content: ' + relative)
        patched = original.replace(old, new)
        if hashlib.sha256(patched).hexdigest() != edit['patched_sha256']:
            raise ValueError('Unexpected compatibility patch output: ' + relative)
        path.write_bytes(patched)
        records.append(dict(edit, input_already_patched=False))
    (destination.parent / 'sdk_compatibility_patches.json').write_text(json.dumps(records, indent=2) + '\n')
    return destination


def main():
    here = Path(__file__).resolve().parent
    project = here.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', '--sdk', dest='sdk_root', type=Path,
                        default=Path(os.environ.get('PICOBREW_SDK', str(project / '.deps/energia'))))
    parser.add_argument('--arm-prefix', default=os.environ.get('PICOBREW_ARM_PREFIX'),
                        help='Compiler tool prefix including final arm-none-eabi-')
    parser.add_argument('--toolchain', type=Path, help='Legacy alias for a compiler installation root')
    parser.add_argument('--output-dir', '--output', dest='output_dir', type=Path,
                        default=project / 'build/sdk-validation')
    parser.add_argument('--entry', type=Path, default=project / 'tests/sdk_validation.cpp')
    parser.add_argument('--runtime-compat', type=Path, default=project / 'platform/energia_runtime_compat.c')
    parser.add_argument('--lock', type=Path, default=project / 'dependencies/lock.json')
    parser.add_argument('--application-lib', type=Path,
                        help='Recovered application archive; all exported definitions are retained')
    parser.add_argument('--adapter-lib', type=Path,
                        help='Typed Energia adapter archive; all exported definitions are retained')
    parser.add_argument('--source', type=Path, action='append', default=[])
    parser.add_argument('--include', type=Path, action='append', default=[])
    parser.add_argument('--linker', type=Path)
    parser.add_argument('--appliance-image', action='store_true',
                        help='Use the complete recovered entry and Pico linker (called by build_firmware.py)')
    parser.add_argument('--standalone', action='store_true',
                        help='Local API profile; retain reachable production code, not every legacy export')
    parser.add_argument('--jobs', type=int, default=min(8, os.cpu_count() or 1))
    args = parser.parse_args()
    if args.appliance_image and (args.entry.resolve() != project / 'platform/firmware_main.cpp' or
                                not args.linker or args.linker.resolve() != project / 'platform/pico_cc3200.ld'):
        parser.error('--appliance-image requires the recovered appliance entry and Pico linker')
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if args.arm_prefix and args.toolchain:
        parser.error('Use either --arm-prefix or --toolchain')
    prefix = args.arm_prefix
    if args.toolchain:
        prefix = str(args.toolchain.resolve() / 'bin/arm-none-eabi-')
    if not prefix:
        candidate = project / '.deps/toolchain/bin/arm-none-eabi-gcc'
        compiler = str(candidate) if candidate.is_file() else shutil.which('arm-none-eabi-gcc')
        if not compiler:
            parser.error('ARM compiler missing; bootstrap dependencies or set --arm-prefix')
        prefix = compiler[:-3]
    sdk, out = args.sdk_root.resolve(), args.output_dir.resolve()
    if not (sdk / 'cores/cc3200/Energia.h').is_file():
        parser.error('SDK missing; bootstrap dependencies or set --sdk-root')
    if not args.lock.is_file():
        parser.error('Dependency lock missing: ' + str(args.lock))
    lock = json.loads(args.lock.read_text())
    selected_archives = [p.resolve() for p in (args.application_lib, args.adapter_lib) if p]
    for path in [args.entry, args.runtime_compat, *args.source, *selected_archives]:
        if not path.is_file():
            parser.error('Build input missing: ' + str(path))
    out.mkdir(parents=True, exist_ok=True)
    sdk_input = sdk
    patches = list(lock['energia']['patches'])
    if args.standalone:
        patches += lock['energia']['standalone_patches']
    sdk = prepare_sdk(sdk, out / 'sdk-compatible', patches)
    core = sdk / 'cores/cc3200'
    variant = sdk / 'variants/CC3200-LAUNCHXL'
    libraries = [sdk / 'libraries/WiFi', sdk / 'libraries/SPI']
    cc, cxx = prefix + 'gcc', prefix + 'g++'
    target_flags = ['-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft', '-mabi=aapcs']
    nosys_library = Path(subprocess.check_output(
        [cc, *target_flags, '-print-file-name=libnosys.a'], text=True).strip()).resolve()
    if not nosys_library.is_file():
        parser.error('The selected compiler must provide libnosys.a for unsupported process/file-descriptor operations')
    allocator = prepare_allocator(prefix, cc, target_flags, out / 'runtime-allocator')
    (out / 'allocator_providers.json').write_text(json.dumps(allocator, indent=2) + '\n')
    common = ['-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft', '-mabi=aapcs',
              '--specs=nano.specs', '-Os', '-g', '-ffunction-sections', '-fdata-sections', '-fcommon',
              '-fno-fast-math', '-ffp-contract=off', '-Wall', '-DF_CPU=80000000L',
              '-DARDUINO=10810', '-DENERGIA=10810', '-DENERGIA_CC3200_LAUNCHXL',
              '-DENERGIA_ARCH_CC3200', '-Dprintf=iprintf']
    if args.standalone:
        common.append('-DPICO_STANDALONE=1')
    include_dirs = [core, sdk / 'system', sdk / 'system/inc', sdk / 'system/driverlib', variant,
                    *libraries, sdk / 'libraries/WiFi/utility',
                    sdk / 'libraries/WiFi/utility/middleware/driver',
                    sdk / 'libraries/WiFi/utility/middleware/driver/hal',
                    project / 'include', *[p.resolve() for p in args.include]]
    includes = ['-I' + str(p) for p in include_dirs]
    upstream = sorted([p for directory in [core, *libraries] for p in directory.rglob('*')
                       if p.suffix in ('.c', '.cpp', '.S') and 'examples' not in p.parts])
    application = [args.entry.resolve(), args.runtime_compat.resolve(),
                   *[p.resolve() for p in args.source]]
    def compile_one(source):
        try:
            relative = source.relative_to(sdk)
            obj = out / 'objects/upstream' / relative.with_suffix(source.suffix + '.o')
            label = str(relative)
        except ValueError:
            unique = hashlib.sha256(str(source).encode()).hexdigest()[:10]
            obj = out / 'objects/application' / (source.stem + '_' + unique + '.o')
            label = source.name
        obj.parent.mkdir(parents=True, exist_ok=True)
        cpp = source.suffix == '.cpp'
        command = [str(cxx if cpp else cc), *common, *includes,
                   *(['-flto'] if args.standalone and source != args.runtime_compat.resolve()
                     and source.name != 'startup_gcc.c' else []),
                   *(['-std=gnu++11', '-fno-rtti', '-fno-exceptions', '-fno-threadsafe-statics']
                     if cpp else ['-std=gnu11']), '-c', str(source), '-o', str(obj)]
        result = subprocess.run(command, capture_output=True, text=True)
        return {'source': label, 'source_path': str(source), 'object': str(obj),
                'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                'exit_code': result.returncode, 'diagnostics': result.stdout + result.stderr,
                'command': command}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        compiled = list(pool.map(compile_one, upstream + application))
    (out / 'compile_report.json').write_text(json.dumps(compiled, indent=2) + '\n')
    failures = [x for x in compiled if x['exit_code']]
    if failures:
        for item in failures:
            print(item['source'] + '\n' + item['diagnostics'])
        print(str(len(failures)) + ' compile failures')
        return 1
    archive = out / 'energia_cc3200.a'
    archive.unlink(missing_ok=True)
    subprocess.run([prefix + ('gcc-ar' if args.standalone else 'ar'), 'rcs', str(archive),
                    *[x['object'] for x in compiled[:len(upstream)]]], check=True)
    # --whole-archive alone still allows --gc-sections to discard unreferenced
    # functions. Root every public definition from the supplied application
    # archives, so linking really checks every exposed module's dependencies.
    exported_symbols = set()
    for selected in selected_archives:
        symbols = subprocess.check_output([prefix + ('gcc-nm' if args.standalone else 'nm'), '--extern-only',
                    '--defined-only', '--format=posix', str(selected)], text=True)
        for line in symbols.splitlines():
            fields = line.split()
            if len(fields) >= 2 and fields[1] in 'TDBRSCVW':
                exported_symbols.add(fields[0])
    # Application/adapter LTO removes unused callback bodies and data without
    # reducing the native stack or heap. Preserve the inspected public appliance
    # entry boundaries. SDK startup and the bounded allocator adapter remain
    # non-LTO so their linker-provider checks retain the explicit contract.
    appliance_roots = ('setup', 'loop', 'pico_application_setup', 'pico_application_loop',
                       'pico_application_initialize', 'pico_application_bind_services',
                       'pico_recipe_run_step', 'pico_temperature_control_timer_isr',
                       'pico_heater_pump_timer_isr')
    retained = (['-Wl,--undefined=' + name for name in appliance_roots] if args.standalone else
                ['-Wl,--undefined=' + name for name in sorted(exported_symbols)])
    archive_args = (['-Wl,--whole-archive', *map(str, selected_archives),
                     '-Wl,--no-whole-archive'] if selected_archives else [])
    elf = out / 'sdk_validation.elf'
    raw = out / 'sdk_validation.bin'
    map_path = out / 'sdk_validation.map'
    linker = (args.linker or core / 'cc3200.ld').resolve()
    link = [str(cxx), '-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=soft', '-mabi=aapcs',
            *(['-flto', '-ffunction-sections', '-fdata-sections'] if args.standalone else []),
            '-nostartfiles', '-nostdlib', '--specs=nano.specs', '-Os', '-Wl,--gc-sections', '-T', str(linker),
            '-Wl,-Map,' + str(map_path), '-Wl,--check-sections', '-Wl,--cref',
            '-Wl,--entry=ResetISR', '-Wl,--unresolved-symbols=report-all', '-Wl,--warn-common',
            '-Wl,--wrap=_sbrk',
            *['-Wl,--undefined=' + name for name in ('malloc', 'calloc', 'realloc', 'free')],
            *retained, '-o', str(elf), '-Wl,--start-group',
            *[x['object'] for x in compiled[len(upstream):]], *archive_args,
            *[item['path'] for item in allocator['objects']],
            # -nostdlib disables the compiler's default library sequence,
            # including nosys.specs. Keep its real bare-metal failure handlers
            # after the SDK. The real SDK _sbrk retains heap ownership behind
            # the bounded failure-normalizing wrapper.
            str(archive), str(sdk / 'system/driverlib/libdriverlib.a'),
            '-lm', '-lc', '-lgcc', '-lnosys',
            '-Wl,--end-group']
    result = subprocess.run(link, capture_output=True, text=True)
    (out / 'link_report.json').write_text(json.dumps({'command': link,
             'exit_code': result.returncode, 'diagnostics': result.stdout + result.stderr}, indent=2) + '\n')
    if result.returncode:
        print(result.stdout + result.stderr)
        return result.returncode
    subprocess.run([prefix + 'objcopy', '-O', 'binary', str(elf), str(raw)], check=True)
    def capture(*command):
        return subprocess.check_output([prefix + command[0], *command[1:]], text=True)
    attrs = capture('readelf', '-h', '-A', str(elf))
    undefined = capture('nm', '-u', str(elf)).strip()
    linked_definitions = {line.split()[0] for line in
                          capture('nm', '--defined-only', '--format=posix', str(elf)).splitlines()
                          if line.split()}
    discarded_application_symbols = sorted(exported_symbols - linked_definitions)
    missing_application_symbols = [] if args.standalone else discarded_application_symbols
    # In GNU ld's cross-reference table the definition is listed first for a
    # resolved symbol. Check the selected definition, not merely its presence.
    cross_references = map_path.read_text().partition('Cross Reference Table')[2]
    providers = {}
    for line in cross_references.splitlines():
        if line and not line[0].isspace():
            fields = line.split(None, 1)
            if len(fields) == 2:
                providers[fields[0]] = fields[1]
    process_fd_hooks = ('_exit', '_kill', '_getpid', '_write', '_close',
                        '_fstat', '_isatty', '_lseek', '_read')
    sbrk_linked = '_sbrk' in linked_definitions
    expected_sbrk_provider = str(archive) + '(startup_gcc.c.o)'
    sdk_heap_provider_verified = (not sbrk_linked or
                                  providers.get('_sbrk') == expected_sbrk_provider)
    expected_allocator = {
        'malloc': 'malloc', 'free': 'malloc', 'calloc': 'calloc', 'realloc': 'realloc',
        '_malloc_r': 'mallocr', '_free_r': 'freer', '_calloc_r': 'callocr',
        '_realloc_r': 'reallocr'}
    allocation_providers = {name: providers.get(name) for name in expected_allocator}
    allocator_provider_verified = all(
        allocation_providers[name] == str(out / 'runtime-allocator' / ('lib_a-' + member + '.o'))
        for name, member in expected_allocator.items())
    runtime_object = compiled[len(upstream) + 1]['object']
    bounded_heap_adapter_verified = providers.get('__wrap__sbrk') == runtime_object
    runtime_boundary = {
        'support_library': str(nosys_library),
        'support_library_sha256': hashlib.sha256(nosys_library.read_bytes()).hexdigest(),
        'process_file_descriptor_providers': {
            name: providers.get(name) for name in process_fd_hooks
            if name in linked_definitions},
        'contract_for_pinned_toolchain': {
            'process_and_file_descriptors': 'Unsupported; libnosys reports ENOSYS rather than successful I/O/process services',
            'read_write_close_fstat_lseek_kill_getpid': 'Return -1 and set errno to ENOSYS (88)',
            'isatty': 'Return 0 and set errno to ENOSYS (88)',
            'exit': 'Does not return; enters an infinite loop',
            'device_io': 'No POSIX descriptor retargeting to Serial, WiFi, or SLFS is supplied'},
        'sbrk_linked': sbrk_linked,
        'sbrk_provider': providers.get('_sbrk') if sbrk_linked else None,
        'sdk_heap_provider_verified': sdk_heap_provider_verified,
        'bounded_heap_adapter_provider': providers.get('__wrap__sbrk'),
        'bounded_heap_adapter_verified': bounded_heap_adapter_verified,
        'allocation_providers': allocation_providers,
        'allocator_provider_verified': allocator_provider_verified,
        'allocator': allocator,
        'sdk_heap_contract': 'Unmodified Energia startup_gcc.c owns heap_end. __wrap__sbrk checks signed growth/shrink against linker _heap/_eheap before invoking it and reports (void*)-1 with syscall errno=ENOMEM on failure. Full-newlib allocation objects from the selected compiler replace the unsafe nano allocation family; nano stdio remains in use.',
        'scope': 'Link integration and selected provider verification; no OS services or hardware-execution claim'}
    (out / 'runtime_boundary.json').write_text(json.dumps(runtime_boundary, indent=2) + '\n')
    sizes = capture('size', '-A', str(elf))
    (out / 'elf_attributes.txt').write_text(attrs)
    (out / 'size.txt').write_text(sizes)
    report = {'scope': ('Complete recovered Pico Pro application and pinned native SDK compile/link; hardware not executed'
                        if args.appliance_image else 'Pinned native SDK compile/link smoke entry; not full PicoBrew firmware'),
              'compiler': subprocess.check_output([str(cc), '--version'], text=True).splitlines()[0],
              'multilib': subprocess.check_output([str(cc), '-mcpu=cortex-m4', '-mthumb',
                                                   '-mfloat-abi=soft', '-print-multi-directory'], text=True).strip(),
              'upstream_sources_compiled': len(upstream), 'application_sources_compiled': len(application),
              'warnings': sum(x['diagnostics'].count('warning:') for x in compiled),
              'undefined_symbols': undefined, 'elf_size': elf.stat().st_size,
              'binary_size': raw.stat().st_size, 'binary_sha256': hashlib.sha256(raw.read_bytes()).hexdigest(),
              'sdk_input': str(sdk_input),
              'dependency_lock_sha256': hashlib.sha256(args.lock.read_bytes()).hexdigest(),
              'selected_archives': [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                                    for p in selected_archives],
              'profile': 'standalone' if args.standalone else 'legacy',
              'retained_application_symbols': sorted(exported_symbols & linked_definitions),
              'discarded_application_symbols': discarded_application_symbols,
              'missing_application_symbols': missing_application_symbols,
              'runtime_boundary': runtime_boundary,
              'linker': str(linker), 'board_variant': 'CC3200-LAUNCHXL',
              'hardware_executed': False, 'full_firmware': args.appliance_image,
              'pass': not bool(undefined) and not missing_application_symbols and sdk_heap_provider_verified
                      and bounded_heap_adapter_verified and allocator_provider_verified
                      and 'soft-float ABI' in attrs and 'ARM' in attrs}
    (out / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
