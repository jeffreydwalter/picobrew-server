#!/usr/bin/env python3
"""Fetch pinned CC3200 source and an ARM compiler into a project-local directory.

No system packages are installed. Archives and guarded compatibility patches are
specified by dependencies/lock.json. The bundled compiler targets Linux x86-64.
"""
import argparse
import hashlib
import json
import platform
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request


def digest(path):
    checksum = hashlib.sha256()
    with path.open('rb') as handle:
        while block := handle.read(1024 * 1024):
            checksum.update(block)
    return checksum.hexdigest()


def apply_patches(destination, patches):
    for patch in patches:
        path = destination / patch['file']
        actual = digest(path)
        if actual == patch['patched_sha256']:
            continue
        if actual != patch['original_sha256']:
            raise RuntimeError('Refusing to replace unexpected dependency content: ' + str(path))
        contents = path.read_bytes()
        before, after = patch['before'].encode(), patch['after'].encode()
        if contents.count(before) != 1:
            raise RuntimeError('Patch context is not unique: ' + str(path))
        patched = contents.replace(before, after)
        if hashlib.sha256(patched).hexdigest() != patch['patched_sha256']:
            raise RuntimeError('Patch output failed verification: ' + str(path))
        path.write_bytes(patched)


def install(entry, prefix):
    destination = prefix / entry['directory']
    marker = destination / '.picobrew-dependency.json'
    if destination.exists():
        if not marker.is_file():
            raise RuntimeError('Refusing to overwrite an existing untracked directory: ' + str(destination))
        previous = json.loads(marker.read_text())
        if previous.get('archive_sha256') != entry['sha256']:
            raise RuntimeError('Existing dependency has a different identity: ' + str(destination))
        apply_patches(destination, entry.get('patches', []))
        print('Using verified dependency identity: ' + str(destination), flush=True)
        return destination
    archives = prefix / 'archives'
    archives.mkdir(parents=True, exist_ok=True)
    archive = archives / entry['archive_filename']
    if archive.exists() and digest(archive) != entry['sha256']:
        raise RuntimeError('Cached archive hash differs from the lock: ' + str(archive))
    if not archive.exists():
        partial = archive.with_name(archive.name + '.part')
        print('Downloading ' + entry['name'], flush=True)
        try:
            with urllib.request.urlopen(entry['url'], timeout=60) as response, partial.open('wb') as output:
                shutil.copyfileobj(response, output, length=1024 * 1024)
            if partial.stat().st_size != entry['bytes'] or digest(partial) != entry['sha256']:
                raise RuntimeError('Downloaded archive does not match its pinned size and SHA-256')
            partial.replace(archive)
        finally:
            partial.unlink(missing_ok=True)
    if not hasattr(tarfile, 'data_filter'):
        raise RuntimeError('Use Python 3.12+ or a Python version with tarfile.data_filter backported')
    with tempfile.TemporaryDirectory(prefix='extract-', dir=prefix) as temp:
        temporary = Path(temp)
        with tarfile.open(archive) as tar:
            tar.extractall(temporary, filter='data')
        extracted = temporary / entry['archive_root']
        if not extracted.is_dir():
            raise RuntimeError('Expected archive root is absent: ' + entry['archive_root'])
        apply_patches(extracted, entry.get('patches', []))
        (extracted / '.picobrew-dependency.json').write_text(json.dumps({
            'name': entry['name'], 'archive_sha256': entry['sha256'],
            'url': entry['url'], 'patches': entry.get('patches', []),
        }, indent=2) + '\n')
        extracted.rename(destination)
    print('Installed ' + str(destination), flush=True)
    return destination


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=project / '.deps')
    parser.add_argument('--lock', type=Path, default=project / 'dependencies/lock.json')
    parser.add_argument('--sdk-only', action='store_true', help='Use an independently installed ARM compiler')
    args = parser.parse_args()
    lock = json.loads(args.lock.read_text())
    prefix = args.prefix.resolve()
    prefix.mkdir(parents=True, exist_ok=True)
    if not args.sdk_only and (platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'amd64')):
        raise RuntimeError('The pinned compiler host is Linux x86-64; use --sdk-only with another installed compiler')
    sdk = install(lock['energia'], prefix)
    output = {'sdk': str(sdk)}
    if not args.sdk_only:
        compiler = install(lock['toolchain'], prefix)
        cc = compiler / 'bin/arm-none-eabi-gcc'
        output['arm_prefix'] = str(compiler / 'bin/arm-none-eabi-')
        output['compiler_version'] = subprocess.check_output([str(cc), '--version'], text=True).splitlines()[0]
    print(json.dumps(output, indent=2))


if __name__ == '__main__':
    raise SystemExit(main())
