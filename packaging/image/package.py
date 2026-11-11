#!/usr/bin/env python3
"""Package a clean production staging tree and its exact corresponding source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile

PREFIX = Path('usr/lib/bazzite-updater-kde')


def output(*args):
    return subprocess.check_output(args, text=True).strip()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_tree(stage):
    root = stage / PREFIX
    if not (root / 'bin/bazzite-updater').is_file():
        raise ValueError('Missing production executable')
    if (root / 'runtime-verified').exists():
        raise ValueError('Runtime verification must be performed in the consuming image')
    for path in stage.rglob('*'):
        if path.is_dir() and path in (stage / 'usr', stage / 'usr/lib'):
            continue
        if root not in (path, *path.parents):
            raise ValueError(f'Unexpected staging path: {path}')
        if path.is_symlink():
            target = os.readlink(path)
            if os.path.isabs(target) or not path.resolve().is_relative_to(root.resolve()):
                raise ValueError(f'Unsafe link: {path}')
        elif not (path.is_file() or path.is_dir()):
            raise ValueError(f'Unsupported staging entry: {path}')
        if path.lstat().st_mode & 0o6000:
            raise ValueError(f'Privileged staging entry: {path}')


def archive(source, destination, epoch, members):
    def normalize(info):
        info.uid = info.gid = 0
        info.uname = info.gname = 'root'
        info.mtime = epoch
        return info
    with tarfile.open(destination, 'x:xz', format=tarfile.PAX_FORMAT) as tar:
        for member in members:
            tar.add(source / member, arcname=str(member), recursive=True, filter=normalize)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--builder', required=True)
    parser.add_argument('--revision', type=int, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'ghcr.io/ublue-os/bazzite@sha256:[0-9a-f]{64}', args.builder):
        raise ValueError('Builder must be an immutable Bazzite digest')
    if args.revision < 1:
        raise ValueError('Package revision must be positive')
    source = Path(os.environ.get('SOURCE_DIR', Path(__file__).resolve().parents[2])).resolve()
    packaging_commit = os.environ['PACKAGING_COMMIT']
    if not re.fullmatch(r'[0-9a-f]{40}', packaging_commit):
        raise ValueError('Packaging commit must be a full SHA')
    os.chdir(source)
    if output('git', 'status', '--porcelain', '--untracked-files=no'):
        raise ValueError('Tracked source tree is dirty')
    commit = output('git', 'rev-parse', 'HEAD')
    submodule = source / 'src/components/controllable'
    controllable = output('git', '-C', str(submodule), 'rev-parse', 'HEAD')
    expected = output('git', 'ls-tree', 'HEAD', 'src/components/controllable').split()[2]
    if controllable != expected or output('git', '-C', str(submodule), 'status', '--porcelain', '--untracked-files=no'):
        raise ValueError('Controllable does not match the clean pinned source')
    version = (source / 'fork-version.txt').read_text().strip()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+-kde\.[0-9]+', version):
        raise ValueError('Unexpected fork version')
    architecture = output('uname', '-m')
    if architecture != 'x86_64':
        raise ValueError('Only x86_64 is currently supported')
    root = args.stage / PREFIX
    validate_tree(args.stage)
    (root / 'source-commit').write_text(commit + '\n')
    (root / 'fork-version').write_text(version + '\n')
    shutil.copytree(source / 'LICENSES', root / 'share/licenses/bazzite-updater-kde', dirs_exist_ok=True)
    shutil.copy2(source / 'THIRD_PARTY_NOTICES.md', root / 'share/licenses/bazzite-updater-kde/THIRD_PARTY_NOTICES.md')
    shutil.copy2(submodule / 'LICENSE.txt', root / 'share/licenses/bazzite-updater-kde/Controllable-LICENSE.txt')
    packages = output('rpm', '-qa', '--qf', '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}\n').splitlines()
    metadata = dict(schema_version=1, fork_version=version, source_commit=commit,
                    controllable_commit=controllable, packaging_commit=packaging_commit, package_revision=args.revision,
                    architecture=architecture, os_id='fedora', os_version='44',
                    builder_image=args.builder, ui_probe=False,
                    binary_sha256=digest(root / 'bin/bazzite-updater'),
                    build_packages=sorted(packages),
                    build_flags=['Release', '_INCLUDE_SUBMODULES=ON', '_USE_XDG_CONFIG=ON',
                                 'BAZZITE_UPDATER_UI_PROBE=OFF'],
                    verification=['source contracts', 'isolated native UI regression',
                                  'clean production build', 'dynamic linking', 'offscreen startup'],
                    unperformed_by_verification=['system update', 'rollback', 'rebase', 'reboot'])
    (root / 'BUILD-INFO.json').write_text(json.dumps(metadata, indent=2) + '\n')
    validate_tree(args.stage)
    args.destination.mkdir(parents=True, exist_ok=True)
    name = f'bazzite-updater-kde-{version}-image-{args.revision}'
    binary = args.destination / f'{name}-fedora44-{architecture}.tar.xz'
    sources = args.destination / f'{name}-source-with-submodules.tar.xz'
    epoch = int(output('git', 'show', '-s', '--format=%ct', 'HEAD'))
    archive(args.stage, binary, epoch, [PREFIX])
    with tempfile.TemporaryDirectory() as tmp:
        base = Path(tmp)
        tree = base / name
        tree.mkdir()
        subprocess.run(['bash', '-o', 'pipefail', '-c', 'git archive HEAD | tar -x -C "$1"', '_', str(tree)], check=True)
        subdest = tree / 'src/components/controllable'
        subdest.mkdir(parents=True, exist_ok=True)
        subprocess.run(['bash', '-o', 'pipefail', '-c', 'git -C "$1" archive HEAD | tar -x -C "$2"', '_', str(submodule), str(subdest)], check=True)
        archive(base, sources, epoch, [Path(name)])
    build_info = args.destination / 'BUILD-INFO.json'
    if build_info.exists() or (args.destination / 'SHA256SUMS').exists():
        raise ValueError('Refusing to overwrite release metadata')
    shutil.copy2(root / 'BUILD-INFO.json', build_info)
    (args.destination / 'SHA256SUMS').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in (binary, sources, build_info)))
    print(binary)


if __name__ == '__main__':
    main()
