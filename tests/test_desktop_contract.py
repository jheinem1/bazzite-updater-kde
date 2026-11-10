# SPDX-License-Identifier: GPL-2.0-or-later
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class DesktopContract(unittest.TestCase):
    def test_plain_controller_independent_labels(self):
        for path in (ROOT / "src").rglob("*.qml"):
            if "controllable" in path.parts:
                continue
            with self.subTest(path=path):
                self.assertNotIn("GP.Labels", path.read_text())

    def test_persistent_navigation_is_separate_from_input_focus(self):
        main = (ROOT / "src/pages/Main.qml").read_text()
        for setting in ("modal: false", "drawerOpen: true", "collapsible: false", "handleVisible: false"):
            self.assertIn(setting, main)
        self.assertNotIn("drawerOpen = false", main)
        self.assertNotIn('i18n("Navigation")', main)
        for path in (ROOT / "src/pages").rglob("*.qml"):
            self.assertNotIn("globalDrawer.drawerOpen", path.read_text())

    def test_search_and_menu_use_native_controls(self):
        main = (ROOT / "src/pages/Main.qml").read_text()
        self.assertIn("Kirigami.SearchField", main)
        self.assertIn("autoAccept: false", main)
        self.assertIn("QQC2.MenuItem { action: actionReboot }", main)
        self.assertIn("QQC2.MenuItem { action: actionQuit }", main)
        self.assertIn('focusSequences: [StandardKey.Find]', main)
        self.assertIn('i18n("No matching pages")', main)

    def test_no_unfocused_enter_to_update(self):
        page = (ROOT / "src/pages/system_update/SystemUpdate.qml").read_text()
        self.assertNotIn('shortcut: "Return"', page)
        self.assertIn("action: updateAction", page)

    def test_controller_actions_honor_enabled_guards(self):
        for path in (ROOT / "src/pages").rglob("*.qml"):
            self.assertNotIn(".triggered()", path.read_text())

    def test_context_is_ready_before_qml_load(self):
        source = (ROOT / "src/main.cpp").read_text()
        load = source.index("engine.loadFromModule")
        for name in ("sessionStorage", "UseFullscreen", "ApplicationVersion"):
            self.assertLess(source.index('setContextProperty(u"' + name), load)

    def test_probe_cannot_launch_system_commands(self):
        source = (ROOT / "src/utils.cpp").read_text()
        self.assertIn("UI probe blocked command launch", source)
        self.assertIn('#ifdef BAZZITE_UPDATER_UI_PROBE', source)
        self.assertIn('process->start(u"/usr/bin/false"_s)', source)

    def test_image_identity_does_not_use_inherited_base_metadata(self):
        info = (ROOT / "src/components/customFormcards/FCSystemInfo.qml").read_text()
        self.assertIn("AppData.ostree.bootedDeployment", info)
        self.assertIn("AppData.ostree.pendingDeployment", info)
        self.assertIn('i18n("Base distribution metadata")', info)
        self.assertNotIn("OtherUtilsBackend.currentImage", info)
        self.assertNotIn('i18n("Last Update")', info)

    def test_details_does_not_impersonate_terminal(self):
        for path in (ROOT / "src/pages").rglob("*.qml"):
            self.assertNotIn('icon.name: "utilities-terminal"', path.read_text())

    def test_original_license_and_policy_are_separate(self):
        self.assertTrue((ROOT / "LICENSES/GPL-2.0-or-later.txt").is_file())
        self.assertIn("not an additional license restriction", (ROOT / "CONTRIBUTING.md").read_text())


if __name__ == "__main__":
    unittest.main()
