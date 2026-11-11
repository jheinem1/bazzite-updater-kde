"""Staging/archive contract tests independent of the desktop runtime."""
import importlib.util
from pathlib import Path
import tarfile
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[1] / 'packaging/image/package.py'
spec = importlib.util.spec_from_file_location('image_package', MODULE)
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


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
