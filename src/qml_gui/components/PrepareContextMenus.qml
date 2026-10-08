import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "../controls"

Item {
    id: root

    required property var editorVm
    // U08 (⑬): ConfigViewModel for per-slot filament preset labels in the
    // Change Filament submenu (upstream reads preset_bundle->filament_presets,
    // GUI_Factories.cpp:1073-1086). Optional: without it the submenu falls
    // back to the "Filament %n" naming.
    property var configVm: null

    // G-07: bumped whenever a context menu is about to show. Dynamic item
    // texts below reference this tick so their one-shot Q_INVOKABLE bindings
    // re-evaluate at every popup instead of keeping creation-time values.
    property int menuRefreshTick: 0

    signal requestAddModels()
    signal requestReplacePart()
    signal requestReplaceAll()
    signal requestExport(bool separateFiles, bool drcFormat)
    signal requestConfirmDelete()
    signal requestConfirmClearPlate()
    signal requestConfirmDeletePlate()
    signal requestRenameObject()
    signal requestActivateGizmo(int mode)
    signal requestObjectLayers()
    signal requestRenamePlate()
    signal requestPlateSettings()

    function addHandyModel(modelId) {
        if (root.editorVm)
            root.editorVm.addHandyModelToContextPlate(modelId)
    }

    // P16.4: Change Filament submenu family (upstream MenuFactory::
    // append_menu_item_change_filament, GUI_Factories.cpp:1879-1961).
    // U08 (⑬): items carry the slot's preset label with a "Filament %n"
    // fallback, and the active extruder of a single selection gets a
    // "(current)" suffix and is disabled — both mirror upstream
    // GUI_Factories.cpp:1067-1086 (label lookup + initial_extruder, where an
    // unconfigured extruder key reads as 1).
    component ChangeFilamentSubmenu: CxMenu {
        id: filamentMenu
        // Upstream labels the submenu "Change Filament" for a single
        // selection and "Set Filament for selected items" for multi
        // (GUI_Factories.cpp:1907).
        title: (root.editorVm && root.editorVm.selectedObjectCount > 1)
                   ? qsTr("Set Filament for selected items") : qsTr("Change Filament")
        enabled: root.editorVm && root.editorVm.contextActionAvailable("changeFilament")
        Instantiator {
            model: root.editorVm ? root.editorVm.configFilamentCount() + 1 : 1
            delegate: CxMenuItem {
                id: filItem
                required property int index
                // G-07: re-evaluate per popup (Q_INVOKABLE reads are not
                // tracked by the QML engine).
                readonly property string _presetLabel: {
                    void root.menuRefreshTick
                    if (index === 0)
                        return qsTr("Default")
                    if (root.configVm) {
                        var preset = root.configVm.filamentPresetForSlot(index)
                        if (preset && preset.length > 0)
                            return preset
                    }
                    return qsTr("Filament %1").arg(index)
                }
                readonly property bool _isCurrent: {
                    void root.menuRefreshTick
                    if (index === 0 || !root.editorVm
                            || root.editorVm.selectedObjectCount !== 1)
                        return false
                    var current = root.editorVm.objectExtruderId(root.editorVm.selectedObjectIndex)
                    if (current < 0)
                        current = 1  // upstream: unconfigured key defaults to 1
                    return current === index
                }
                text: _presetLabel + (_isCurrent ? " (" + qsTr("current") + ")" : "")
                // Upstream keeps these as plain items and disables the active
                // extruder (GUI_Factories.cpp:1088-1095 enable updater).
                enabled: !_isCurrent
                onTriggered: if (root.editorVm) root.editorVm.setExtruderForSelectedItems(index)
            }
            onObjectAdded: (index, object) => filamentMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => filamentMenu.removeItem(object)
        }
    }

    // P16.9: Flush Options submenu (upstream append_menu_items_flush_options,
    // GUI_Factories.cpp:937-1028). U08 (③): real check items like upstream
    // append_menu_check_item; the "[x]" text prefix is gone and the check
    // state stays backend-owned (flushOptionValue per popup via G-07 tick).
    component FlushOptionsSubmenu: CxMenu {
        title: qsTr("Flush Options")
        enabled: root.editorVm && root.editorVm.contextActionAvailable("flushOptions")
        onAboutToShow: ++root.menuRefreshTick
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.flushOptionValue(0)
            }
            text: qsTr("Flush into objects' infill")
            onTriggered: if (root.editorVm) root.editorVm.toggleFlushOption(0)
        }
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.flushOptionValue(1)
            }
            text: qsTr("Flush into this object")
            onTriggered: if (root.editorVm) root.editorVm.toggleFlushOption(1)
        }
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.flushOptionValue(2)
            }
            text: qsTr("Flush into objects' support")
            onTriggered: if (root.editorVm) root.editorVm.toggleFlushOption(2)
        }
    }

    // P16.5: per-type add-volume submenus (upstream ADD_VOLUME_MENU_ITEMS +
    // append_submenu_add_generic, GUI_Factories.cpp:285-292, :642-662).
    // Primitives land as the submenu's volume type; text/SVG creation opens
    // the matching gizmo (upstream append_menu_item_add_text/svg) and is only
    // meaningful for the part type in the Qt6 backend.
    component AddVolumeTypeSubmenu: CxMenu {
        id: addVolMenu
        property int volumeType: 0
        title: volumeType === 0 ? qsTr("Add part")
             : volumeType === 1 ? qsTr("Add negative part")
             : volumeType === 2 ? qsTr("Add modifier")
             : volumeType === 3 ? qsTr("Add support blocker")
             : qsTr("Add support enforcer")
        enabled: root.editorVm && root.editorVm.contextActionAvailable("addVolume")
        CxMenuItem {
            text: qsTr("Load...")
            onTriggered: addPartFileDialogComp.createObject(root, {
                volumeType: addVolMenu.volumeType
            }).open()
        }
        MenuSeparator { }
        CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 0, addVolMenu.volumeType) }
        CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 1, addVolMenu.volumeType) }
        CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 2, addVolMenu.volumeType) }
        CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 3, addVolMenu.volumeType) }
        MenuSeparator { }
        CxMenuItem {
            visible: addVolMenu.volumeType === 0
                     && !!root.editorVm
                     && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
            text: qsTr("Add text")
            onTriggered: root.requestActivateGizmo(16)
        }
        CxMenuItem {
            visible: addVolMenu.volumeType === 0
                     && !!root.editorVm
                     && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
            text: qsTr("Add SVG")
            onTriggered: root.requestActivateGizmo(17)
        }
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
            // U08 (⑩): Add text/SVG also live on the default menu's Add
            // Primitive submenu (upstream append_submenu_add_generic with
            // ModelVolumeType::INVALID accepts the gizmo items,
            // GUI_Factories.cpp:651-656); same gizmo-mask gating as the
            // add-volume submenus above.
            MenuSeparator { }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
                text: qsTr("Add text")
                onTriggered: root.requestActivateGizmo(16)
            }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
                text: qsTr("Add SVG")
                onTriggered: root.requestActivateGizmo(17)
            }
        }
        // U08 (③): upstream "Show Labels" is a check item
        // (append_menu_check_item, GUI_Factories.cpp:1415-1417), checked =
        // labels shown.
        CxMenuItem {
            checkable: true
            checked: !!root.editorVm && root.editorVm.showLabels
            text: qsTr("Show Labels")
            onTriggered: root.editorVm.showLabels = !root.editorVm.showLabels
        }
    }

    CxMenu {
        id: objectMenu
        onAboutToShow: ++root.menuRefreshTick
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
        // U08 (⑥): upstream Split is a submenu with To Objects + To Parts
        // (GUI_Factories.cpp:1446-1453).
        CxMenu {
            title: qsTr("Split")
            enabled: root.editorVm && (root.editorVm.contextActionAvailable("splitObjects")
                                       || root.editorVm.contextActionAvailable("splitParts"))
            CxMenuItem {
                text: qsTr("To objects")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
                onTriggered: root.editorVm.splitSelectedToObjects()
            }
            CxMenuItem {
                text: qsTr("To parts")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
                onTriggered: root.editorVm.splitSelectedToParts()
            }
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
        // U08 (⑫): upstream label is "Drop" (append_menu_item_drop,
        // GUI_Factories.cpp:2136).
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        // U08 (⑫): upstream labels are "Along X/Y/Z Axis"
        // (append_menu_items_mirror, GUI_Factories.cpp:1282-1287).
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        // U08 (③): upstream "Printable" is a check item reflecting the
        // printable state (append_menu_item_set_printable,
        // GUI_Factories.cpp:2293-2319).
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
            }
            text: qsTr("Printable")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(
                             !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
        }
        CxMenuItem {
            text: qsTr("Show or hide")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("visibility")
            onTriggered: root.editorVm.toggleSelectedObjectsVisibility()
        }
        // U08 (⑫): upstream label is "Fix Model"
        // (append_menu_item_fix_through_cgal, GUI_Factories.cpp:963).
        CxMenuItem {
            text: qsTr("Fix Model")
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
        // U08 (⑥): Mesh boolean on the object menu (upstream
        // append_menu_item_mesh_boolean, GUI_Factories.cpp:1271; API
        // EditorViewModel.h:657).
        CxMenuItem {
            text: qsTr("Mesh boolean")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("meshBoolean")
            onTriggered: root.editorVm.booleanExecute()
        }
        CxMenu {
            title: qsTr("Convert units")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("convertUnits")
            CxMenuItem { text: qsTr("Convert from inches"); onTriggered: root.editorVm.convertSelectedObjectUnits(1) }
            // U08 (⑫): upstream labels "Restore to Inch"/"Restore to Meter"
            // (SettingsFactory conversion map, GUI_Factories.cpp:1233-1235).
            CxMenuItem { text: qsTr("Restore to Inch"); onTriggered: root.editorVm.convertSelectedObjectUnits(0) }
            CxMenuItem { text: qsTr("Convert from meters"); onTriggered: root.editorVm.convertSelectedObjectUnits(3) }
            CxMenuItem { text: qsTr("Restore to Meter"); onTriggered: root.editorVm.convertSelectedObjectUnits(2) }
        }
        // U08 (⑥): upstream object menu carries Replace 3D file... /
        // Replace all with 3D files... (append_menu_item_replace_with_stl /
        // _replace_all_with_stl, GUI_Factories.cpp:1020-1033). Gated on an
        // object selection; "replaceWithStl" resolves through the generic
        // has-object fallthrough of contextActionAvailable.
        CxMenuItem {
            text: qsTr("Replace 3D file...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplacePart()
        }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplaceAll()
        }
        // P16.5: five add-volume submenus (upstream append_menu_items_add_volume,
        // GUI_Factories.cpp:642-662, sits after Delete / before the process
        // entries in create_extra_object_menu)
        AddVolumeTypeSubmenu { volumeType: 0 }
        AddVolumeTypeSubmenu { volumeType: 1 }
        AddVolumeTypeSubmenu { volumeType: 2 }
        AddVolumeTypeSubmenu { volumeType: 3 }
        AddVolumeTypeSubmenu { volumeType: 4 }
        MenuSeparator { }
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
            // U08 (⑫): upstream label "Edit in Parameter Table"
            // (append_menu_item_per_object_settings, GUI_Factories.cpp:2191).
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        // P16.9: Flush Options + Invalidate cut info (upstream
        // append_menu_items_flush_options / append_menu_item_invalidate_cut_info)
        FlushOptionsSubmenu { }
        CxMenuItem {
            text: qsTr("Invalidate cut info")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("invalidateCutInfo")
            onTriggered: root.editorVm.invalidateSelectedCutInfo()
        }
        // P16.4: Change Filament (upstream append_menu_item_change_filament)
        ChangeFilamentSubmenu { }
        CxMenuItem {
            text: qsTr("Reload from disk")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("reload")
            onTriggered: root.editorVm.reloadSelectedFromDisk()
        }
        // U08 (⑫⑦): upstream labels "Export as one STL"/"Export as one DRC"
        // (append_menu_item_export_stl / _export_drc, GUI_Factories.cpp:972-994).
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        CxMenuItem {
            text: qsTr("Export as one DRC...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, true)
        }
    }

    CxMenu {
        id: partMenu
        // U08 (⑧): upstream part menu Split is a submenu with To Objects +
        // To Parts (create_bbl_part_menu, GUI_Factories.cpp:1626-1635).
        CxMenu {
            title: qsTr("Split")
            enabled: root.editorVm && (root.editorVm.contextActionAvailable("splitObjects")
                                       || root.editorVm.contextActionAvailable("splitParts"))
            CxMenuItem {
                text: qsTr("To objects")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
                onTriggered: root.editorVm.splitSelectedToObjects()
            }
            CxMenuItem {
                text: qsTr("To parts")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
                onTriggered: root.editorVm.splitSelectedToParts()
            }
        }
        CxMenuItem {
            text: qsTr("Replace part...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replacePart")
            onTriggered: root.requestReplacePart()
        }
        // U08 (⑧): upstream create_bbl_part_menu ends with
        // reload_from_disk / replace_with_stl / replace_all_with_stl
        // (GUI_Factories.cpp:1638-1641).
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplaceAll()
        }
        CxMenuItem {
            text: qsTr("Reload from disk")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("reload")
            onTriggered: root.editorVm.reloadSelectedFromDisk()
        }
        // U08 (⑧): per_object_process entry (upstream
        // append_menu_item_per_object_process, GUI_Factories.cpp:2151).
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            // U08 (⑫): "Edit settings" → upstream "Edit in Parameter Table".
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.canOpenSelectionSettings
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        // P16.9/P16.4: Flush Options + Change Filament also live on the part
        // menu upstream (part_menu() appends change_filament,
        // GUI_Factories.cpp:1624).
        FlushOptionsSubmenu { }
        ChangeFilamentSubmenu { }
        CxMenuItem {
            text: qsTr("Repair part")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("simplify")
                     && (root.editorVm.availableGizmoMask & (1 << 9)) !== 0
            onTriggered: root.editorVm.simplifyMeshSelected()
        }
        // P16.7: Center + Mirror on the part menu (upstream create_bbl_part_menu
        // appends center/drop/mirror, GUI_Factories.cpp:1425-1454).
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        // U08 (⑫): upstream label is "Drop" (append_menu_item_drop).
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        // U08 (⑫): upstream labels are "Along X/Y/Z Axis".
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        // P16.2: merge(false) entry (upstream append_menu_item_merge_to_single_object)
        CxMenuItem {
            text: qsTr("Merge parts to a single object")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mergeToSingle")
            onTriggered: root.editorVm.mergeSelectedPartsToSingleObject()
        }
        // P16.3: Mesh boolean entry (upstream
        // append_menu_item_merge_parts_to_single_part, GUI_Factories.cpp:1101-1107)
        CxMenuItem {
            text: qsTr("Mesh boolean")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("meshBoolean")
            onTriggered: root.editorVm.booleanExecute()
        }
        // P16.7: Change type submenu (upstream append_menu_item_change_type)
        CxMenu {
            title: qsTr("Change type")
            enabled: root.editorVm && root.editorVm.hasSelectedVolume
            CxMenuItem { text: qsTr("Part"); onTriggered: root.editorVm.changeVolumeType(0) }
            CxMenuItem { text: qsTr("Negative volume"); onTriggered: root.editorVm.changeVolumeType(1) }
            CxMenuItem { text: qsTr("Modifier"); onTriggered: root.editorVm.changeVolumeType(2) }
            CxMenuItem { text: qsTr("Support blocker"); onTriggered: root.editorVm.changeVolumeType(3) }
            CxMenuItem { text: qsTr("Support enforcer"); onTriggered: root.editorVm.changeVolumeType(4) }
        }
        CxMenuItem {
            text: qsTr("Subdivision mesh")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("subdivide")
            onTriggered: root.editorVm.subdivideSelectedMesh()
        }
        // U08 (⑧): Convert units also lives on the part menu (part_menu(),
        // GUI_Factories.cpp:1874).
        CxMenu {
            title: qsTr("Convert units")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("convertUnits")
            CxMenuItem { text: qsTr("Convert from inches"); onTriggered: root.editorVm.convertSelectedObjectUnits(1) }
            CxMenuItem { text: qsTr("Restore to Inch"); onTriggered: root.editorVm.convertSelectedObjectUnits(0) }
            CxMenuItem { text: qsTr("Convert from meters"); onTriggered: root.editorVm.convertSelectedObjectUnits(3) }
            CxMenuItem { text: qsTr("Restore to Meter"); onTriggered: root.editorVm.convertSelectedObjectUnits(2) }
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
        // U08 (⑫⑦): upstream labels "Export as one STL"/"Export as one DRC".
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        CxMenuItem {
            text: qsTr("Export as one DRC...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, true)
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
            text: qsTr("Delete text")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        // U08 (⑤): the seven upstream text-part entries
        // (create_text_part_menu + text_part_menu(), GUI_Factories.cpp:1578-1591
        // and :1880-1884): Fix Model / Simplify / Center / Mirror /
        // Edit Process Settings / Change Type / Change Filament.
        CxMenuItem {
            text: qsTr("Fix Model")
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
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            // U08 (⑫): upstream label "Edit in Parameter Table".
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenu {
            title: qsTr("Change type")
            enabled: root.editorVm && root.editorVm.hasSelectedVolume
            CxMenuItem { text: qsTr("Part"); onTriggered: root.editorVm.changeVolumeType(0) }
            CxMenuItem { text: qsTr("Negative volume"); onTriggered: root.editorVm.changeVolumeType(1) }
            CxMenuItem { text: qsTr("Modifier"); onTriggered: root.editorVm.changeVolumeType(2) }
            CxMenuItem { text: qsTr("Support blocker"); onTriggered: root.editorVm.changeVolumeType(3) }
            CxMenuItem { text: qsTr("Support enforcer"); onTriggered: root.editorVm.changeVolumeType(4) }
        }
        ChangeFilamentSubmenu { }
        // Export entries kept from the OWzx menu (upstream text-part menu has
        // none; keep-register).
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        CxMenuItem {
            text: qsTr("Export as one DRC...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, true)
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
            text: qsTr("Delete SVG")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        // U08 (⑤): the seven upstream svg-part entries
        // (create_svg_part_menu + svg_part_menu(), GUI_Factories.cpp:1594-1606
        // and :1886-1890).
        CxMenuItem {
            text: qsTr("Fix Model")
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
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            // U08 (⑫): upstream label "Edit in Parameter Table".
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        CxMenu {
            title: qsTr("Change type")
            enabled: root.editorVm && root.editorVm.hasSelectedVolume
            CxMenuItem { text: qsTr("Part"); onTriggered: root.editorVm.changeVolumeType(0) }
            CxMenuItem { text: qsTr("Negative volume"); onTriggered: root.editorVm.changeVolumeType(1) }
            CxMenuItem { text: qsTr("Modifier"); onTriggered: root.editorVm.changeVolumeType(2) }
            CxMenuItem { text: qsTr("Support blocker"); onTriggered: root.editorVm.changeVolumeType(3) }
            CxMenuItem { text: qsTr("Support enforcer"); onTriggered: root.editorVm.changeVolumeType(4) }
        }
        ChangeFilamentSubmenu { }
        // Export entries kept from the OWzx menu (upstream svg-part menu has
        // none; keep-register).
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        CxMenuItem {
            text: qsTr("Export as one DRC...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, true)
        }
    }

    CxMenu {
        id: multiMenu
        onAboutToShow: ++root.menuRefreshTick
        // U08 (⑫): upstream label is "Assemble"
        // (append_menu_item_assemble, GUI_Factories.cpp:1263).
        CxMenuItem {
            text: qsTr("Assemble")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: root.editorVm.assembleSelectedObjects()
        }
        // P16.3: Mesh boolean entry for the two-object backend (upstream
        // append_menu_item_merge_parts_to_single_part, GUI_Factories.cpp:1101)
        CxMenuItem {
            text: qsTr("Mesh boolean")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("meshBoolean")
            onTriggered: root.editorVm.booleanExecute()
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
        // U08 (⑫): upstream label is "Fix Model" (append_menu_item_fix_through_cgal).
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        // U08 (⑫): upstream label is "Drop" (append_menu_item_drop).
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        // P16.6: split entries in the multi-selection menu (upstream
        // multi_selection_menu split submenu, GUI_Factories.cpp:1700-1717)
        CxMenu {
            title: qsTr("Split")
            CxMenuItem {
                text: qsTr("To objects")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
                onTriggered: root.editorVm.splitSelectedToObjects()
            }
            CxMenuItem {
                text: qsTr("To parts")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
                onTriggered: root.editorVm.splitSelectedToParts()
            }
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
        // U08 (③): upstream "Printable" check item in the multi menu
        // (append_menu_item_set_printable, GUI_Factories.cpp:1950).
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
            }
            text: qsTr("Printable")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(
                             !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
        }
        // P16.6: Edit Process Settings (upstream append_menu_item_per_object_process,
        // GUI_Factories.cpp:1846-1860)
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        // U08 (⑨): Change type in the multi-volume branch (upstream
        // multi_selection_menu else-branch, GUI_Factories.cpp:1989).
        CxMenu {
            title: qsTr("Change type")
            enabled: root.editorVm && root.editorVm.hasSelectedVolume
            CxMenuItem { text: qsTr("Part"); onTriggered: root.editorVm.changeVolumeType(0) }
            CxMenuItem { text: qsTr("Negative volume"); onTriggered: root.editorVm.changeVolumeType(1) }
            CxMenuItem { text: qsTr("Modifier"); onTriggered: root.editorVm.changeVolumeType(2) }
            CxMenuItem { text: qsTr("Support blocker"); onTriggered: root.editorVm.changeVolumeType(3) }
            CxMenuItem { text: qsTr("Support enforcer"); onTriggered: root.editorVm.changeVolumeType(4) }
        }
        // P16.6: Convert units submenu (upstream multi menu appends
        // append_menu_items_convert_unit, GUI_Factories.cpp:1688)
        CxMenu {
            title: qsTr("Convert units")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("convertUnits")
            CxMenuItem { text: qsTr("Convert from inches"); onTriggered: root.editorVm.convertSelectedObjectUnits(1) }
            // U08 (⑫): upstream labels "Restore to Inch"/"Restore to Meter".
            CxMenuItem { text: qsTr("Restore to Inch"); onTriggered: root.editorVm.convertSelectedObjectUnits(0) }
            CxMenuItem { text: qsTr("Convert from meters"); onTriggered: root.editorVm.convertSelectedObjectUnits(3) }
            CxMenuItem { text: qsTr("Restore to Meter"); onTriggered: root.editorVm.convertSelectedObjectUnits(2) }
        }
        // U08 (⑨): Replace all with 3D files also lives on the multi menu
        // (upstream append_menu_item_replace_all_with_stl, GUI_Factories.cpp:1961).
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplaceAll()
        }
        // P16.4: Change Filament (upstream multi menu, GUI_Factories.cpp:1692)
        ChangeFilamentSubmenu { }
        // U08 (⑫⑦): upstream multi labels "Export as one STL"/"Export as
        // STLs" + the DRC pair (append_menu_item_export_stl(true) /
        // append_menu_item_export_drc(true), GUI_Factories.cpp:971-1009).
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        // P16.6: separate-files export (upstream "Export as STLs",
        // append_menu_item_export_stl(is_mulity_menu=true),
        // GUI_Factories.cpp:842-848)
        CxMenuItem {
            text: qsTr("Export as STLs...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(true, false)
        }
        // U08 (⑦): DRC pair on the multi menu
        // (append_menu_item_export_drc(true), GUI_Factories.cpp:989-1009).
        CxMenuItem {
            text: qsTr("Export as one DRC...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, true)
        }
        CxMenuItem {
            text: qsTr("Export as DRCs...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(true, true)
        }
    }

    CxMenu {
        id: plateMenu
        onAboutToShow: ++root.menuRefreshTick
        CxMenuItem {
            text: qsTr("Select all objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateSelect")
            onTriggered: root.editorVm.selectAllOnPlate(root.editorVm.contextPlateIndex)
        }
        // U08 (⑪): upstream "Select All Plates" (GUI_Factories.cpp:1721-1727);
        // reuses the existing cross-plate VM entry (EditorViewModel.h:836).
        CxMenuItem {
            text: qsTr("Select All Plates")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateSelect")
            onTriggered: root.editorVm.selectAllVisibleObjects()
        }
        // U08 (⑫): upstream label is "Delete All" (GUI_Factories.cpp:1729).
        CxMenuItem {
            text: qsTr("Delete All")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateClear")
            onTriggered: root.requestConfirmClearPlate()
        }
        CxMenuItem {
            text: qsTr("Arrange objects")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateArrange")
            onTriggered: root.editorVm.arrangePlate(root.editorVm.contextPlateIndex)
        }
        // U08 (⑫): upstream label is "Auto Rotate" (GUI_Factories.cpp:1763).
        CxMenuItem {
            text: qsTr("Auto Rotate")
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
        // U08 (⑫): upstream label is "Reload All" (GUI_Factories.cpp:1753).
        CxMenuItem {
            text: qsTr("Reload All")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateReload")
            onTriggered: root.editorVm.reloadAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateReplaceAll")
            onTriggered: root.requestReplaceAll()
        }
        // U08 (⑫): upstream label is "Edit Plate Name"
        // (append_menu_item_plate_name, GUI_Factories.cpp:2377).
        CxMenuItem {
            text: qsTr("Edit Plate Name")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateRename")
            onTriggered: root.requestRenamePlate()
        }
        CxMenuItem {
            text: qsTr("Plate settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateSettings")
            onTriggered: root.requestPlateSettings()
        }
        // U08 (⑫): upstream labels are "Unlock"/"Lock"
        // (append_menu_item_locked, GUI_Factories.cpp:2346-2352).
        CxMenuItem {
            text: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return (root.editorVm && root.editorVm.isPlateLocked(root.editorVm.contextPlateIndex))
                       ? qsTr("Unlock") : qsTr("Lock")
            }
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateLock")
            onTriggered: root.editorVm.togglePlateLocked(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return (root.editorVm && root.editorVm.isPlatePrintable(root.editorVm.contextPlateIndex))
                       ? qsTr("Set plate unprintable") : qsTr("Set plate printable")
            }
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
            onTriggered: root.requestConfirmDeletePlate()
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
        }
        // G-07: refresh the spin box at open time — the one-shot declarative
        // binding captured objectInstanceCount() when the menu was built, so
        // the dialog kept showing a stale count for a different object and
        // OK silently wrote the wrong number.
        onOpened: {
            const idx = root.editorVm ? root.editorVm.selectedObjectIndex : -1
            instanceCount.value = (root.editorVm && idx >= 0)
                ? root.editorVm.objectInstanceCount(idx) : 1
        }
    }

    // P16.5: "Load..." part-import dialog for the add-volume submenus
    // (upstream ObjectList::load_subobject, GUI_Factories.cpp:680).
    Component {
        id: addPartFileDialogComp
        FileDialog {
            required property int volumeType
            title: qsTr("Select a model file to import as a part")
            nameFilters: ["Model files (*.stl *.obj *.3mf)", "All files (*)"]
            onAccepted: {
                if (root.editorVm)
                    root.editorVm.addVolumeFromFile(root.editorVm.selectedObjectIndex,
                                                    selectedFile.toString().replace("file:///", ""),
                                                    volumeType)
                destroy()
            }
            onRejected: destroy()
        }
    }
}
