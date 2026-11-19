#!/usr/bin/env python3
"""Create a private per-device bearer key for /local/api.key.

This command only creates a local file. It never connects to or flashes a Pico.
Provision the file through the verified UART connection after backing up the
device. Keep it outside the repository's tracked files and out of shared logs.
"""
import argparse
import os
from pathlib import Path
import secrets


def create_key(path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation protects the only copy of a previously provisioned key.
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        with os.fdopen(descriptor, 'w', encoding='ascii', newline='\n') as handle:
            handle.write(secrets.token_hex(32) + '\n')
            handle.flush()
            os.fsync(handle.fileno())
    except BaseException:
        path.unlink(missing_ok=True)
        raise
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('private/api.key'),
                        help='New private file; existing files are never overwritten')
    args = parser.parse_args()
    try:
        path = create_key(args.output)
    except FileExistsError:
        parser.error('Key file already exists; choose another output path to rotate a key.')
    print('Created private API key: ' + str(path.resolve()))
    print('Device destination: /local/api.key (provisioning is a separate UART operation).')


if __name__ == '__main__':
    main()
