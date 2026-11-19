#!/usr/bin/env python3
"""Host-test the actual bounded HTTP state machine, including malformed input.

Uses address/undefined-behavior sanitizers unless --no-sanitize is supplied.
No device, network service, or credentials are contacted.
"""
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
    with tempfile.TemporaryDirectory(prefix='pico-http-test-') as directory:
        command = [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
                   '-I' + str(root / 'include')]
        if not args.no_sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        command += [str(root / 'src/http_server.c')]
        # The firmware transport performs no heap allocation. LeakSanitizer is
        # unnecessary here and cannot run in ptrace-based build sandboxes;
        # address and undefined-behavior instrumentation remain enabled.
        environment = dict(os.environ)
        environment.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        for fixture in ['http_server_fixture', 'http_asset_fixture']:
            executable = Path(directory) / fixture
            subprocess.run([*command, str(root / 'tests' / (fixture + '.c')),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True, env=environment)


if __name__ == '__main__':
    main()
