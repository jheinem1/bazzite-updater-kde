"""Staging/archive contract tests independent of the desktop runtime."""
import importlib.util
from pathlib import Path
import tarfile
import shutil
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[1] / 'packaging/image/package.py'
spec = importlib.util.spec_from_file_location('image_package', MODULE)
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


probe_spec = importlib.util.spec_from_file_location('image_probe', MODULE.with_name('run_probe.py'))
probe = importlib.util.module_from_spec(probe_spec)
probe_spec.loader.exec_module(probe)


class ProbeIsolationTests(unittest.TestCase):
    def test_restores_trusted_configs_after_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            original = root / 'etc/bazzite-updater'
            original.mkdir(parents=True)
            (original / 'config.ini').write_text('trusted-original')
            with probe.isolated_configs(root):
                self.assertFalse(original.exists())
            self.assertEqual((original / 'config.ini').read_text(), 'trusted-original')

    def test_restores_trusted_configs_after_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            original = root / 'usr/etc/bazzite-updater'
            original.mkdir(parents=True)
            (original / 'config.ini').write_text('trusted-original')
            with self.assertRaisesRegex(RuntimeError, 'probe failed'):
                with probe.isolated_configs(root):
                    raise RuntimeError('probe failed')
            self.assertEqual((original / 'config.ini').read_text(), 'trusted-original')

    def test_unexpected_write_restores_all_originals_and_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = [root / path for path in probe.TRUSTED_CONFIGS]
            for path in paths:
                path.mkdir(parents=True)
                (path / 'config.ini').write_text('trusted-original')
            with self.assertRaisesRegex(RuntimeError, 'unexpectedly wrote'):
                with probe.isolated_configs(root) as backup:
                    self.addCleanup(shutil.rmtree, backup, True)
                    paths[-1].mkdir()
                    (paths[-1] / 'config.ini').write_text('unexpected')
            for path in paths:
                self.assertEqual((path / 'config.ini').read_text(), 'trusted-original')
            self.assertEqual((backup / 'unexpected-2/config.ini').read_text(), 'unexpected')

    def test_restores_symlink(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            original = root / 'etc/bazzite-updater'
            original.parent.mkdir(parents=True)
            original.symlink_to('../usr/etc/bazzite-updater')
            with probe.isolated_configs(root):
                self.assertFalse(original.is_symlink())
            self.assertEqual(str(original.readlink()), '../usr/etc/bazzite-updater')


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.stage = Path(self.tmp.name) / 'stage'
        self.root = self.stage / package.PREFIX
        (self.root / 'bin').mkdir(parents=True)
        self.binary = self.root / 'bin/bazzite-updater'
        self.binary.write_bytes(b'production fixture')
        self.binary.chmod(0o755)

    def test_valid_private_tree(self):
        package.validate_tree(self.stage)

    def test_missing_binary(self):
        self.binary.unlink()
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_no_runtime_verified(self):
        (self.root / 'runtime-verified').write_text('bad')
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_no_external_paths(self):
        (self.stage / 'etc').mkdir()
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_no_absolute_symlinks(self):
        (self.root / 'link').symlink_to('/etc/passwd')
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_no_escaping_symlinks(self):
        (self.root / 'link').symlink_to('../../../../etc/passwd')
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_relative_internal_symlink(self):
        (self.root / 'link').symlink_to('bin/bazzite-updater')
        package.validate_tree(self.stage)

    def test_no_privileged_modes(self):
        self.binary.chmod(0o4755)
        with self.assertRaises(ValueError):
            package.validate_tree(self.stage)

    def test_normalized_archive(self):
        archive = Path(self.tmp.name) / 'image.tar.xz'
        package.archive(self.stage, archive, 1234567890, [package.PREFIX])
        with tarfile.open(archive) as tar:
            names = tar.getnames()
            self.assertIn(str(package.PREFIX / 'bin/bazzite-updater'), names)
            for member in tar.getmembers():
                self.assertTrue(member.name.startswith(str(package.PREFIX)))
                self.assertEqual(member.uid, 0)
                self.assertEqual(member.gid, 0)
                self.assertEqual(member.mtime, 1234567890)
        with self.assertRaises(FileExistsError):
            package.archive(self.stage, archive, 1234567890, [package.PREFIX])

    def test_repeated_archive_bytes(self):
        first = Path(self.tmp.name) / 'first.tar.xz'
        second = Path(self.tmp.name) / 'second.tar.xz'
        package.archive(self.stage, first, 1234567890, [package.PREFIX])
        package.archive(self.stage, second, 1234567890, [package.PREFIX])
        self.assertEqual(package.digest(first), package.digest(second))


if __name__ == '__main__':
    unittest.main()
