// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FC

import io.github.rfrench3.bazzite_updater

FC.FormCard {
    id: root
    Layout.fillWidth: true
    readonly property ostreeDeployment booted: AppData.ostree.bootedDeployment
    readonly property ostreeDeployment pending: AppData.ostree.pendingDeployment
    readonly property string unavailable: AppData.ostreeLoading ? i18n("Detecting image…") : i18n("Unavailable")

    FC.FormTextDelegate {
        id: stagedImageInfo
        visible: root.pending.present
        text: i18n("A system deployment is pending. Reboot to use it.")
        font.bold: true
    }
    FormDelegateSeparatorFixed { visible: stagedImageInfo.visible }

    FC.FormTextDelegate {
        objectName: "bootedImageReference"
        textItem.wrapMode: Text.WrapAnywhere
        textItem.textFormat: Text.PlainText
        text: root.booted.reference || root.unavailable
        description: i18n("Current Image (booted)")
    }
    FormDelegateSeparatorFixed { visible: root.pending.present }
    FC.FormTextDelegate {
        objectName: "pendingImageReference"
        visible: root.pending.present
        textItem.wrapMode: Text.WrapAnywhere
        textItem.textFormat: Text.PlainText
        text: root.pending.reference || i18n("Unavailable")
        description: i18n("Pending Image (after reboot)")
    }
    FormDelegateSeparatorFixed { visible: root.booted.buildDate.length > 0 }
    FC.FormTextDelegate {
        visible: root.booted.buildDate.length > 0
        text: root.booted.buildDate
        description: i18n("Booted Deployment Build Date")
    }

    FormDelegateSeparatorFixed { visible: driverInfo.visible }
    FC.FormTextDelegate {
        id: driverInfo
        visible: AppData.gpu.best !== Gpu.Drivers.UNKNOWN
        text: {
            switch (AppData.gpu.best) {
            case Gpu.Drivers.BASE:
                return i18n("The recommended drivers for your GPU are provided by the Linux kernel.");
            case Gpu.Drivers.NVIDIA:
                return i18n("The recommended drivers for your GPU are nvidia.");
            case Gpu.Drivers.NVIDIA_OPEN:
                return i18n("The recommended drivers for your GPU are nvidia-open.");
            case Gpu.Drivers.UNSUPPORTED:
                return i18n("Your GPU is unsupported.");
            default:
                return "";
            }
        }
        description: i18n("Hardware recommendation; custom image names do not establish which drivers are installed.")
    }

    FormDelegateSeparatorFixed {
        visible: layeredPackagesInfo.visible
        below: layeredPackagesInfo.itemBelowPrev
    }
    FormDelegateCollapsible {
        id: layeredPackagesInfo
        readonly property int count: root.booted.layeredPackages.length
        visible: count !== 0
        text: expanded ? i18n("Close layered packages view") : i18n("View layered packages (%1)", count)
        FC.FormTextAreaDelegate {
            label: i18n("Layered Packages (%1):", layeredPackagesInfo.count)
            readOnly: true
            text: root.booted.layeredPackages.join(", ")
            description: i18n("Layering packages is a last-resort for when there is no other way to install a package. Avoid layering packages whenever possible, as it can cause system updates to fail.")
        }
    }
    FormDelegateSeparatorFixed {
        above: layeredPackagesInfo.visible ? layeredPackagesInfo.itemAboveNext : null
        below: detailedInfo.itemBelowPrev
    }
    FormDelegateCollapsible {
        id: detailedInfo
        text: expanded ? i18n("Hide deployment details") : i18n("Show deployment details")
        FC.FormTextDelegate {
            textItem.wrapMode: Text.WrapAnywhere
        textItem.textFormat: Text.PlainText
            text: root.booted.repository || i18n("Unavailable")
            description: i18n("Booted Image Repository")
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            text: root.booted.tag || i18n("Not specified")
            description: i18n("Booted Image Tag")
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            objectName: "bootedImageDigest"
            textItem.wrapMode: Text.WrapAnywhere
        textItem.textFormat: Text.PlainText
            text: root.booted.digest || i18n("Unavailable")
            description: i18n("Booted Image Digest")
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            text: root.booted.version || i18n("Unavailable")
            description: i18n("Booted Deployment Version")
        }
        FormDelegateSeparatorFixed { visible: root.pending.present }
        FC.FormTextDelegate {
            objectName: "pendingImageDigest"
            visible: root.pending.present
            textItem.wrapMode: Text.WrapAnywhere
        textItem.textFormat: Text.PlainText
            text: root.pending.digest || i18n("Unavailable")
            description: i18n("Pending Image Digest")
        }
        FormDelegateSeparatorFixed { visible: root.pending.present }
        FC.FormTextDelegate {
            visible: root.pending.present
            text: root.pending.version || i18n("Unavailable")
            description: i18n("Pending Deployment Version")
        }
    }

    FormDelegateSeparatorFixed { visible: baseMetadata.visible }
    FormDelegateCollapsible {
        id: baseMetadata
        visible: OtherUtilsBackend.baseImage.load_successful
        text: i18n("Base distribution metadata")
        FC.FormTextDelegate {
            text: i18n("This metadata comes from the base distribution and may differ from the custom image currently running.")
            textItem.wrapMode: Text.WordWrap
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            text: OtherUtilsBackend.baseImage.name || i18n("Unavailable")
            description: i18n("Base Distribution Image")
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            text: OtherUtilsBackend.baseImage.branch || i18n("Unavailable")
            description: i18n("Base Distribution Branch")
        }
        FormDelegateSeparatorFixed {}
        FC.FormTextDelegate {
            text: OtherUtilsBackend.baseImage.versionPretty || OtherUtilsBackend.baseImage.version || i18n("Unavailable")
            description: i18n("Base Distribution Version")
        }
    }
}
