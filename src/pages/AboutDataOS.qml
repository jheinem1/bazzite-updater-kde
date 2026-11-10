import QtQuick
import QtQuick.Controls // Required for grabScrollbar to find a scrollbar
import org.kde.kirigamiaddons.formcard as FormCard
import io.github.rfrench3.controllable as GP
import io.github.rfrench3.bazzite_updater

FormCard.AboutPage {
    id: page
    title: i18nc("About (user's OS)", "About %1", AppData.aboutDataOs.displayName)

    GP.PageNavigation {
        targetScrollbar: page.grabScrollbar(page)
        active: !globalDrawer.controllerNavigationActive
    }

    function grabScrollbar(item) {
        if (item.contentItem?.ScrollBar?.vertical)
            return item.contentItem.ScrollBar.vertical;

        if (item.parent)
            return grabScrollbar(item.parent);

        console.warn("Parent scrollbar not found, controller scrolling will not function!");
    }

    showLibraries: false

    aboutData: AppData.aboutDataOs
}
