#!/usr/bin/env python3
"""Compile and exercise standalone application workflows at SDK boundaries."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--no-sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='pico-control-test-') as temporary:
        sources = sorted((root / 'src').rglob('*.c'))
        environment = dict(os.environ)
        environment.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        cases = [('control-tests', 'standalone_control_host.c', 1, []),
                 ('factory-reset-standalone', 'factory_reset_host.c', 1,
                  ['-Wl,--wrap=pico_encoder_take_click']),
                 ('factory-reset-legacy', 'factory_reset_host.c', 0,
                  ['-Wl,--wrap=pico_encoder_take_click'])]
        for name, harness, standalone, link_flags in cases:
            executable = Path(temporary) / name
            command = [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
                       f'-DPICO_STANDALONE={standalone}', '-ffunction-sections', '-fdata-sections',
                       '-I' + str(root / 'include')]
            if not args.no_sanitize:
                command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            command += [*map(str, sources), str(root / 'tests' / harness),
                        '-Wl,--gc-sections', *link_flags, '-lm', '-o', str(executable)]
            subprocess.run(command, check=True)
            subprocess.run([str(executable)], check=True, env=environment, timeout=30)


if __name__ == '__main__':
    main()
