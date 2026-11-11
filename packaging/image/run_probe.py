#!/usr/bin/env python3
"""Isolate the synthetic probe from distribution configuration in a build container."""
import argparse
from contextlib import contextmanager
from pathlib import Path
import shutil
import subprocess
import tempfile

TRUSTED_CONFIGS = ('etc/bazzite-updater', 'usr/local/etc/bazzite-updater', 'usr/etc/bazzite-updater')


@contextmanager
def isolated_configs(root):
    """Restore every original directory/symlink, including after a failed probe."""
    backup = Path(tempfile.mkdtemp(prefix='updater-probe-config-'))
    moved = []
    try:
        for index, relative in enumerate(TRUSTED_CONFIGS):
            path = root / relative
            if path.exists() or path.is_symlink():
                saved = backup / str(index)
                shutil.move(str(path), str(saved))
                moved.append((path, saved))
        yield backup
    finally:
        errors = []
        for path, saved in reversed(moved):
            try:
                if path.exists() or path.is_symlink():
                    quarantine = backup / f'unexpected-{saved.name}'
                    shutil.move(str(path), str(quarantine))
                    errors.append(f'Probe unexpectedly wrote trusted configuration; retained at {quarantine}')
                shutil.move(str(saved), str(path))
            except Exception as exc:
                errors.append(str(exc))
        if errors:
            # Never overwrite an unexpected file or discard an unrestored original.
            raise RuntimeError(f'Probe configuration isolation failed; retained backup evidence at {backup}: {errors}')
        backup.rmdir()



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('build', type=Path)
    parser.add_argument('evidence', type=Path)
    args = parser.parse_args()
    if not (Path('/run/.containerenv').is_file() or Path('/.dockerenv').is_file()):
        raise RuntimeError('Probe configuration isolation is allowed only in a disposable container')
    with isolated_configs(Path('/')):
        result = subprocess.run(['bash', str(args.source / 'tests/run-ui-probe.sh'),
                                 str(args.build), str(args.evidence)])
    if result.returncode:
        for log in sorted(args.evidence.glob('*.log')):
            print(f'--- {log.name} ---\n{log.read_text(errors="replace")}', flush=True)
        for report in sorted(args.evidence.glob('*/ui-report.json')):
            print(report.read_text(), flush=True)
    raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
