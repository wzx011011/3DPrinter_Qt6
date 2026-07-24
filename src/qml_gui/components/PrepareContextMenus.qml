import QtQuick
import QtQuick.Controls
import "../controls"

Item {
    id: root

    required property var editorVm
    signal requestAddModels()
    signal requestReplacePart()
    signal requestReplaceAll()
    signal requestExport(bool separateFiles, bool drcFormat)
    signal requestConfirmDelete()
    signal requestRenameObject()
    signal requestActivateGizmo(int mode)
    signal requestObjectLayers()
    signal requestRenamePlate()
    signal requestPlateSettings()

    function addHandyModel(modelId) {
        if (root.editorVm)
            root.editorVm.addHandyModelToContextPlate(modelId)
    }

    function openResolved(family, x, y) {
        if (family === 0)
            defaultMenu.popup(x, y)
        else if (family === 1)
            objectMenu.popup(x, y)
        else if (family === 2)
            partMenu.popup(x, y)
        else if (family === 3)
            multiMenu.popup(x, y)
        else if (family === 4)
            plateMenu.popup(x, y)
        else if (family === 5)
            textMenu.popup(x, y)
        else if (family === 6)
            svgMenu.popup(x, y)
    }

    CxMenu {
        id: defaultMenu
        CxMenuItem { text: qsTr("Add model..."); onTriggered: root.requestAddModels() }
        CxMenu {
            title: qsTr("Add handy model")
            CxMenuItem { text: qsTr("Orca Cube"); onTriggered: root.addHandyModel("orca-cube") }
            CxMenuItem { text: qsTr("OrcaSliced Combo"); onTriggered: root.addHandyModel("orca-sliced-combo") }
            CxMenuItem { text: qsTr("Orca Badge"); onTriggered: root.addHandyModel("orca-badge") }
            CxMenuItem { text: qsTr("Orca Tolerance Test"); onTriggered: root.addHandyModel("orca-tolerance-test") }
            CxMenuItem { text: qsTr("3DBenchy"); onTriggered: root.addHandyModel("3dbenchy") }
            CxMenuItem { text: qsTr("Cali Cat"); onTriggered: root.addHandyModel("cali-cat") }
            CxMenuItem { text: qsTr("Autodesk FDM Test"); onTriggered: root.addHandyModel("autodesk-fdm-test") }
            CxMenuItem { text: qsTr("Voron Cube"); onTriggered: root.addHandyModel("voron-cube") }
            CxMenuItem { text: qsTr("Stanford Bunny"); onTriggered: root.addHandyModel("stanford-bunny") }
            CxMenuItem { text: qsTr("Orca String Hell"); onTriggered: root.addHandyModel("orca-string-hell") }
        }
        CxMenu {
            title: qsTr("Add primitive")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("addPrimitive")
            CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitiveToContextPlate(0) }
            CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitiveToContextPlate(1) }
            CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitiveToContextPlate(2) }
            CxMenuItem { text: qsTr("Cone"); onTriggered: root.editorVm.addPrimitiveToContextPlate(3) }
            CxMenuItem { text: qsTr("Prism"); onTriggered: root.editorVm.addPrimitiveToContextPlate(4) }
            CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitiveToContextPlate(5) }
            CxMenuItem { text: qsTr("Disk"); onTriggered: root.editorVm.addPrimitiveToContextPlate(6) }
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.showLabels ? qsTr("Hide labels") : qsTr("Show labels")
            onTriggered: root.editorVm.showLabels = !root.editorVm.showLabels
        }
    }

    CxMenu {
        id: objectMenu
        CxMenuItem {
            text: qsTr("Add instance")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("addInstance")
            onTriggered: root.editorVm.addSelectedInstance()
        }
        CxMenuItem {
            text: qsTr("Remove instance")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("removeInstance")
            onTriggered: root.editorVm.removeSelectedInstance()
        }
        CxMenuItem {
            text: qsTr("Set number of instances")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("setInstances")
            onTriggered: instanceCountDialog.open()
        }
        CxMenuItem {
            text: qsTr("Fill bed with instances")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("fillBedInstances")
            onTriggered: root.editorVm.fillBedWithInstances()
        }
        CxMenuItem {
            text: qsTr("Instance to object")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("instanceToObject")
            onTriggered: root.editorVm.instanceToObject(-1)
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Clone")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("duplicate")
            onTriggered: root.editorVm.duplicateSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Delete")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Rename")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("rename")
            onTriggered: root.requestRenameObject()
        }
        CxMenuItem {
            text: qsTr("Copy")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copy")
            onTriggered: root.editorVm.copySelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Paste")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("paste")
            onTriggered: root.editorVm.pasteObjects()
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Split to objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
            onTriggered: root.editorVm.splitSelectedToObjects()
        }
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Auto orient")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("orient")
            onTriggered: root.editorVm.autoOrientSelected()
        }
        CxMenuItem {
            text: qsTr("Layer height range...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.requestObjectLayers()
        }
        CxMenuItem {
            text: qsTr("Drop to build plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.selectedObjectsAutoDrop()
                  ? qsTr("Disable auto drop") : qsTr("Enable auto drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("autoDrop")
            onTriggered: root.editorVm.toggleSelectedObjectsAutoDrop()
        }
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Mirror X"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Mirror Y"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Mirror Z"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
                  ? qsTr("Set unprintable") : qsTr("Set printable")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(
                             !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
        }
        CxMenuItem {
            text: qsTr("Show or hide")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("visibility")
            onTriggered: root.editorVm.toggleSelectedObjectsVisibility()
        }
        CxMenuItem {
            text: qsTr("Repair model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("simplify")
                     && (root.editorVm.availableGizmoMask & (1 << 9)) !== 0
            onTriggered: root.editorVm.simplifyMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Subdivision mesh")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("subdivide")
            onTriggered: root.editorVm.subdivideSelectedMesh()
        }
        CxMenu {
            title: qsTr("Convert units")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("convertUnits")
            CxMenuItem { text: qsTr("Convert from inches"); onTriggered: root.editorVm.convertSelectedObjectUnits(1) }
            CxMenuItem { text: qsTr("Revert inches conversion"); onTriggered: root.editorVm.convertSelectedObjectUnits(0) }
            CxMenuItem { text: qsTr("Convert from meters"); onTriggered: root.editorVm.convertSelectedObjectUnits(3) }
            CxMenuItem { text: qsTr("Revert meters conversion"); onTriggered: root.editorVm.convertSelectedObjectUnits(2) }
        }
        CxMenuItem {
            text: qsTr("Copy process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copyProcessSettings")
            onTriggered: root.editorVm.copyContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Paste process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("pasteProcessSettings")
            onTriggered: root.editorVm.pasteContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Edit settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            text: qsTr("Reload from disk")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("reload")
            onTriggered: root.editorVm.reloadSelectedFromDisk()
        }
        CxMenuItem {
            text: qsTr("Export STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
    }

    CxMenu {
        id: partMenu
        CxMenuItem {
            text: qsTr("Split to parts")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
            onTriggered: root.editorVm.splitSelectedToParts()
        }
        CxMenuItem {
            text: qsTr("Replace part...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replacePart")
            onTriggered: root.requestReplacePart()
        }
        CxMenuItem {
            text: qsTr("Edit settings")
            enabled: root.editorVm && root.editorVm.canOpenSelectionSettings
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            text: qsTr("Repair part")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Drop to build plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        CxMenuItem {
            text: qsTr("Subdivision mesh")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("subdivide")
            onTriggered: root.editorVm.subdivideSelectedMesh()
        }
        CxMenuItem {
            text: qsTr("Copy process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copyProcessSettings")
            onTriggered: root.editorVm.copyContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Paste process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("pasteProcessSettings")
            onTriggered: root.editorVm.pasteContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Delete part")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Export STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
    }

    CxMenu {
        id: textMenu
        CxMenuItem {
            text: qsTr("Edit text")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("textEdit")
                     && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
            onTriggered: root.requestActivateGizmo(16)
        }
        CxMenuItem {
            text: qsTr("Edit settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            text: qsTr("Delete text")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Export STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
    }

    CxMenu {
        id: svgMenu
        CxMenuItem {
            text: qsTr("Edit SVG")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("svgEdit")
                     && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
            onTriggered: root.requestActivateGizmo(17)
        }
        CxMenuItem {
            text: qsTr("Edit settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            text: qsTr("Delete SVG")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Export STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
    }

    CxMenu {
        id: multiMenu
        CxMenuItem {
            text: qsTr("Merge")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: root.editorVm.assembleSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Clone")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: root.editorVm.duplicateSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Repair models")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Drop to build plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.selectedObjectsAutoDrop()
                  ? qsTr("Disable auto drop") : qsTr("Enable auto drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("autoDrop")
            onTriggered: root.editorVm.toggleSelectedObjectsAutoDrop()
        }
        CxMenuItem {
            text: qsTr("Delete")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Copy")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copy")
            onTriggered: root.editorVm.copySelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Paste")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("paste")
            onTriggered: root.editorVm.pasteObjects()
        }
        CxMenuItem {
            text: qsTr("Set printable")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(true)
        }
        CxMenuItem {
            text: qsTr("Export selected STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
    }

    CxMenu {
        id: plateMenu
        CxMenuItem {
            text: qsTr("Select all objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateSelect")
            onTriggered: root.editorVm.selectAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Clear plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateClear")
            onTriggered: root.editorVm.removeAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Arrange objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateArrange")
            onTriggered: root.editorVm.arrangePlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Auto orient objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateOrient")
            onTriggered: root.editorVm.autoOrientContextPlate()
        }
        CxMenuItem {
            text: qsTr("Add models...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateAddModels")
            onTriggered: root.requestAddModels()
        }
        CxMenu {
            title: qsTr("Add handy model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateHandyModels")
            CxMenuItem { text: qsTr("Orca Cube"); onTriggered: root.addHandyModel("orca-cube") }
            CxMenuItem { text: qsTr("OrcaSliced Combo"); onTriggered: root.addHandyModel("orca-sliced-combo") }
            CxMenuItem { text: qsTr("Orca Badge"); onTriggered: root.addHandyModel("orca-badge") }
            CxMenuItem { text: qsTr("Orca Tolerance Test"); onTriggered: root.addHandyModel("orca-tolerance-test") }
            CxMenuItem { text: qsTr("3DBenchy"); onTriggered: root.addHandyModel("3dbenchy") }
            CxMenuItem { text: qsTr("Cali Cat"); onTriggered: root.addHandyModel("cali-cat") }
            CxMenuItem { text: qsTr("Autodesk FDM Test"); onTriggered: root.addHandyModel("autodesk-fdm-test") }
            CxMenuItem { text: qsTr("Voron Cube"); onTriggered: root.addHandyModel("voron-cube") }
            CxMenuItem { text: qsTr("Stanford Bunny"); onTriggered: root.addHandyModel("stanford-bunny") }
            CxMenuItem { text: qsTr("Orca String Hell"); onTriggered: root.addHandyModel("orca-string-hell") }
        }
        CxMenu {
            title: qsTr("Add primitive")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateAddPrimitive")
            CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitiveToContextPlate(0) }
            CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitiveToContextPlate(1) }
            CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitiveToContextPlate(2) }
            CxMenuItem { text: qsTr("Cone"); onTriggered: root.editorVm.addPrimitiveToContextPlate(3) }
            CxMenuItem { text: qsTr("Prism"); onTriggered: root.editorVm.addPrimitiveToContextPlate(4) }
            CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitiveToContextPlate(5) }
            CxMenuItem { text: qsTr("Disk"); onTriggered: root.editorVm.addPrimitiveToContextPlate(6) }
        }
        CxMenuItem {
            text: qsTr("Paste")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("paste")
            onTriggered: root.editorVm.pasteToContextPlate()
        }
        CxMenuItem {
            text: qsTr("Reload objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateReload")
            onTriggered: root.editorVm.reloadAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateReplaceAll")
            onTriggered: root.requestReplaceAll()
        }
        CxMenuItem {
            text: qsTr("Rename plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateRename")
            onTriggered: root.requestRenamePlate()
        }
        CxMenuItem {
            text: qsTr("Plate settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateSettings")
            onTriggered: root.requestPlateSettings()
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.isPlateLocked(root.editorVm.contextPlateIndex)
                  ? qsTr("Unlock plate") : qsTr("Lock plate")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateLock")
            onTriggered: root.editorVm.togglePlateLocked(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: root.editorVm && root.editorVm.isPlatePrintable(root.editorVm.contextPlateIndex)
                  ? qsTr("Set plate unprintable") : qsTr("Set plate printable")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("platePrintable")
            onTriggered: root.editorVm.setPlatePrintable(root.editorVm.contextPlateIndex,
                !root.editorVm.isPlatePrintable(root.editorVm.contextPlateIndex))
        }
        CxMenuItem {
            text: qsTr("Clone plate")
            enabled: root.editorVm && root.editorVm.canAddPlate
            onTriggered: root.editorVm.clonePlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Move plate left")
            enabled: root.editorVm && root.editorVm.contextPlateIndex > 0
            onTriggered: root.editorVm.movePlate(root.editorVm.contextPlateIndex,
                                                  root.editorVm.contextPlateIndex - 1)
        }
        CxMenuItem {
            text: qsTr("Move plate right")
            enabled: root.editorVm && root.editorVm.contextPlateIndex < root.editorVm.plateCount - 1
            onTriggered: root.editorVm.movePlate(root.editorVm.contextPlateIndex,
                                                  root.editorVm.contextPlateIndex + 1)
        }
        CxMenuItem {
            text: qsTr("Delete plate")
            enabled: root.editorVm && root.editorVm.canDeletePlate(root.editorVm.contextPlateIndex)
            onTriggered: root.editorVm.deletePlate(root.editorVm.contextPlateIndex)
        }
    }

    Dialog {
        id: instanceCountDialog
        modal: true
        title: qsTr("Set number of instances")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (root.editorVm)
                root.editorVm.setSelectedInstanceCount(instanceCount.value)
        }
        CxSpinBox {
            id: instanceCount
            anchors.fill: parent
            from: 1
            to: 1000
            value: root.editorVm && root.editorVm.contextSourceObjectIndex >= 0
                   ? root.editorVm.objectInstanceCount(root.editorVm.selectedObjectIndex) : 1
        }
    }
}
