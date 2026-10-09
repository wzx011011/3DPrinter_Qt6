import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "../controls"

Item {
    id: root

    required property var editorVm
    // U08 (⑬): ConfigViewModel for per-slot filament preset labels in the
    // Change Filament submenu (upstream reads preset_bundle->filament_presets,
    // GUI_Factories.cpp:2282-2287). Optional: without it the submenu falls
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
    signal requestActivateGizmo(int mode)
    signal requestObjectLayers()
    signal requestRenamePlate()

    function addHandyModel(modelId) {
        if (root.editorVm)
            root.editorVm.addHandyModelToContextPlate(modelId)
    }

    // P16.4: Change Filament submenu family (upstream MenuFactory::
    // append_menu_item_change_filament, GUI_Factories.cpp:2209-2295).
    // U08 (⑬): items carry the slot's preset label with a "Filament %n"
    // fallback (:2282-2287). The "Default" row is only appended when the
    // selection contains a modifier volume (:2271-2278, loop start :2280).
    // Upstream keeps is_active_extruder constantly false (:2268-2269), so no
    // "(current)" suffix and no disabled row.
    component ChangeFilamentSubmenu: CxMenu {
        id: filamentMenu
        // Upstream labels the submenu "Change Filament" for a single
        // selection and "Set Filament for selected items" for multi
        // (:2266-2267, :2289).
        title: (root.editorVm && root.editorVm.selectedObjectCount > 1)
                   ? qsTr("Set Filament for selected items") : qsTr("Change Filament")
        enabled: root.editorVm && root.editorVm.contextActionAvailable("changeFilament")
        onAboutToShow: ++root.menuRefreshTick
        Instantiator {
            model: root.editorVm ? root.editorVm.configFilamentCount() + 1 : 1
            delegate: CxMenuItem {
                id: filItem
                required property int index
                // G-07: re-evaluate per popup (Q_INVOKABLE reads are not
                // tracked by the QML engine).
                readonly property bool _isDefaultRow: index === 0
                readonly property bool _defaultRowAllowed: {
                    void root.menuRefreshTick
                    return !!root.editorVm && root.editorVm.selectionHasModifierVolume()
                }
                // Upstream row loop starts at i=0 ("Default") only when the
                // selection carries a modifier volume (GUI_Factories.cpp:2280).
                visible: !_isDefaultRow || _defaultRowAllowed
                readonly property string _presetLabel: {
                    void root.menuRefreshTick
                    if (_isDefaultRow)
                        return qsTr("Default")
                    if (root.configVm) {
                        var preset = root.configVm.filamentPresetForSlot(index)
                        if (preset && preset.length > 0)
                            return preset
                    }
                    return qsTr("Filament %1").arg(index)
                }
                // No "(current)" suffix and no per-row disable: upstream
                // is_active_extruder is constantly false (:2268-2269).
                text: _presetLabel
                onTriggered: if (root.editorVm) root.editorVm.setExtruderForSelectedItems(index)
            }
            onObjectAdded: (index, object) => filamentMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => filamentMenu.removeItem(object)
        }
    }

    // P16.9: Flush Options submenu (upstream append_menu_items_flush_options,
    // GUI_Factories.cpp:1104-1196). U08 (③): real check items like upstream
    // append_menu_check_item; the check state stays backend-owned
    // (flushOptionValue per popup via G-07 tick).
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

    // U08 (⑨): Change Type submenu shared by the part/text/svg menus
    // (upstream append_menu_item_change_type, GUI_Factories.cpp:809-876):
    // title "Change Type" (:811), five check items reflecting the selected
    // volume type (:838-852), with Support Blocker/Enforcer disabled while
    // the selection holds a text or SVG volume (:854-871).
    component ChangeTypeSubmenu: CxMenu {
        id: typeMenu
        title: qsTr("Change Type")
        enabled: root.editorVm && root.editorVm.hasSelectedVolume
        onAboutToShow: ++root.menuRefreshTick
        CxMenuItem {
            checkable: true
            checked: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.getSelectedVolumeType() === 0
            }
            text: qsTr("Part")
            onTriggered: root.editorVm.changeVolumeType(0)
        }
        CxMenuItem {
            checkable: true
            checked: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.getSelectedVolumeType() === 1
            }
            text: qsTr("Negative Part")
            onTriggered: root.editorVm.changeVolumeType(1)
        }
        CxMenuItem {
            checkable: true
            checked: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.getSelectedVolumeType() === 2
            }
            text: qsTr("Modifier")
            onTriggered: root.editorVm.changeVolumeType(2)
        }
        CxMenuItem {
            checkable: true
            checked: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.getSelectedVolumeType() === 3
            }
            // Upstream disables Blocker/Enforcer while the selection carries
            // a text or SVG volume (GUI_Factories.cpp:854-871); volume types
            // 5/6 are TextEmboss/SvgEmboss on the Qt6 baseline.
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                var t = root.editorVm ? root.editorVm.getSelectedVolumeType() : -1
                return t !== 5 && t !== 6
            }
            text: qsTr("Support Blocker")
            onTriggered: root.editorVm.changeVolumeType(3)
        }
        CxMenuItem {
            checkable: true
            checked: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.getSelectedVolumeType() === 4
            }
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                var t = root.editorVm ? root.editorVm.getSelectedVolumeType() : -1
                return t !== 5 && t !== 6
            }
            text: qsTr("Support Enforcer")
            onTriggered: root.editorVm.changeVolumeType(4)
        }
    }

    // U08 (⑥): the four convert entries ride as FLAT items at the tail of
    // the object/part/multi menus (append_menu_items_convert_unit,
    // GUI_Factories.cpp:1197-1251; call sites :1853/:1875/:1959) instead of a
    // submenu. Upstream appends each entry only while the selection's volume
    // conversion state allows it; the Qt6 service exposes no per-volume
    // conversion state (ProjectServiceMock keeps none outside
    // convertObjectUnits), so all four stay listed.
    component ConvertUnitsItems: Repeater {
        model: [
            { name: qsTr("Convert from Inches"), conv: 1 },
            { name: qsTr("Restore to Inch"), conv: 0 },
            { name: qsTr("Convert from Meters"), conv: 3 },
            { name: qsTr("Restore to Meter"), conv: 2 }
        ]
        delegate: CxMenuItem {
            required property var modelData
            text: modelData.name
            enabled: root.editorVm && root.editorVm.contextActionAvailable("convertUnits")
            onTriggered: root.editorVm.convertSelectedObjectUnits(modelData.conv)
        }
    }

    // P16.5: per-type add-volume submenus (upstream ADD_VOLUME_MENU_ITEMS +
    // append_submenu_add_generic, GUI_Factories.cpp:338-345, :556-572).
    // Primitives land as the submenu's volume type. Text/SVG creation opens
    // the matching gizmo and is only whitelisted for the part/negative/
    // modifier types (append_menu_itemm_add_, :689-691).
    component AddVolumeTypeSubmenu: CxMenu {
        id: addVolMenu
        property int volumeType: 0
        title: volumeType === 0 ? qsTr("Add Part")
             : volumeType === 1 ? qsTr("Add Negative Part")
             : volumeType === 2 ? qsTr("Add Modifier")
             : volumeType === 3 ? qsTr("Add Support Blocker")
             : qsTr("Add Support Enforcer")
        enabled: root.editorVm && root.editorVm.contextActionAvailable("addVolume")
        onAboutToShow: ++root.menuRefreshTick
        CxMenuItem {
            text: qsTr("Load...")
            onTriggered: addPartFileDialogComp.createObject(root, {
                volumeType: addVolMenu.volumeType
            }).open()
        }
        MenuSeparator { }
        // Upstream primitive order Cube/Cylinder/Sphere/Cone/Disc/Torus
        // (:556-572). The Qt6 in-object primitive backend only covers
        // cube/sphere/cylinder/torus (ProjectServiceMock::addPrimitive), so
        // Cone and Disc stay honestly disabled with a tooltip.
        CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 0, addVolMenu.volumeType) }
        CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 2, addVolMenu.volumeType) }
        CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 1, addVolMenu.volumeType) }
        CxMenuItem {
            text: qsTr("Cone")
            enabled: false
            ToolTip.visible: coneHover.hovered
            ToolTip.text: qsTr("Cone is not available as an in-object primitive in this build")
            HoverHandler { id: coneHover }
        }
        CxMenuItem {
            text: qsTr("Disc")
            enabled: false
            ToolTip.visible: discHover.hovered
            ToolTip.text: qsTr("Disc is not available as an in-object primitive in this build")
            HoverHandler { id: discHover }
        }
        CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitive(root.editorVm.selectedObjectIndex, 3, addVolMenu.volumeType) }
        MenuSeparator { visible: addVolMenu.volumeType <= 2 }
        CxMenuItem {
            visible: addVolMenu.volumeType <= 2
                     && !!root.editorVm
                     && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
            text: qsTr("Text")
            onTriggered: root.requestActivateGizmo(16)
        }
        CxMenuItem {
            visible: addVolMenu.volumeType <= 2
                     && !!root.editorVm
                     && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
            text: qsTr("SVG")
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
        onAboutToShow: ++root.menuRefreshTick
        // Upstream create_default_menu Windows branch (GUI_Factories.cpp:
        // 1391-1417): Add Primitive ▸, Add Handy models ▸, Add Models, one
        // separator, then the Show Labels check item (:1413-1417).
        CxMenu {
            title: qsTr("Add Primitive")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("addPrimitive")
            // INVALID-typed append_submenu_add_generic (:556-572): no
            // "Load..." row, six primitives in upstream order, then the
            // Text/SVG gizmo rows (:575-576 + :689-697 whitelist INVALID).
            CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitiveToContextPlate(0) }
            CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitiveToContextPlate(2) }
            CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitiveToContextPlate(1) }
            CxMenuItem { text: qsTr("Cone"); onTriggered: root.editorVm.addPrimitiveToContextPlate(3) }
            CxMenuItem { text: qsTr("Disc"); onTriggered: root.editorVm.addPrimitiveToContextPlate(6) }
            CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitiveToContextPlate(5) }
            MenuSeparator { }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
                text: qsTr("Text")
                onTriggered: root.requestActivateGizmo(16)
            }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
                text: qsTr("SVG")
                onTriggered: root.requestActivateGizmo(17)
            }
        }
        CxMenu {
            title: qsTr("Add Handy models")
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
        // Upstream label "Add Models" without dots (:1400).
        CxMenuItem { text: qsTr("Add Models"); onTriggered: root.requestAddModels() }
        MenuSeparator { }
        // Upstream "Show Labels" is a check item
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
        // Upstream create_extra_object_menu (GUI_Factories.cpp:1463-1524)
        // plus the object_menu() dynamic tail (:1851-1860).
        // Instance manipulation group (append_menu_items_instance_manipulation,
        // :2018-2035); upstream labels carry the +/- shortcut annotations.
        CxMenuItem {
            text: qsTr("Add instance") + "\t+"
            enabled: root.editorVm && root.editorVm.contextActionAvailable("addInstance")
            onTriggered: root.editorVm.addSelectedInstance()
        }
        CxMenuItem {
            text: qsTr("Remove instance") + "\t-"
            enabled: root.editorVm && root.editorVm.contextActionAvailable("removeInstance")
            onTriggered: root.editorVm.removeSelectedInstance()
        }
        CxMenuItem {
            text: qsTr("Set number of instances...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("setInstances")
            onTriggered: instanceCountDialog.open()
        }
        CxMenuItem {
            text: qsTr("Fill bed with instances...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("fillBedInstances")
            onTriggered: root.editorVm.fillBedWithInstances()
        }
        MenuSeparator { }
        // Dynamic label: "Set as Individual Objects" for a single full-object
        // selection (append_menu_item_instance_to_object :878-898).
        CxMenuItem {
            text: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return (root.editorVm && root.editorVm.selectionIsSingleFullObject())
                           ? qsTr("Set as Individual Objects")
                           : qsTr("Set as An Individual Object")
            }
            enabled: root.editorVm && root.editorVm.contextActionAvailable("instanceToObject")
            onTriggered: root.editorVm.instanceToObject(-1)
        }
        MenuSeparator { }
        // Clone with the upstream Ctrl+K annotation (append_menu_item_clone,
        // :2084-2099).
        CxMenuItem {
            text: qsTr("Clone") + "\t" + qsTr("Ctrl+") + "K"
            enabled: root.editorVm && root.editorVm.contextActionAvailable("duplicate")
            onTriggered: root.editorVm.duplicateSelectedObjects()
        }
        // Fix Model / Simplify Model / Subdivision mesh (Lost color)
        // (:1473-1475; labels :963/:2103/:2111).
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("simplify")
                     && (root.editorVm.availableGizmoMask & (1 << 9)) !== 0
            onTriggered: root.editorVm.simplifyMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Subdivision mesh") + qsTr("(Lost color)")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("subdivide")
            onTriggered: root.editorVm.subdivideSelectedMesh()
        }
        // append_menu_item_merge_parts_to_single_part opens with its own
        // separator (:1268-1274).
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Mesh boolean")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("meshBoolean")
            onTriggered: root.editorVm.booleanExecute()
        }
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        // Split submenu with the upstream "To Objects"/"To Parts" labels
        // (:1484-1503).
        CxMenu {
            title: qsTr("Split")
            enabled: root.editorVm && (root.editorVm.contextActionAvailable("splitObjects")
                                       || root.editorVm.contextActionAvailable("splitParts"))
            CxMenuItem {
                text: qsTr("To Objects")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
                onTriggered: root.editorVm.splitSelectedToObjects()
            }
            CxMenuItem {
                text: qsTr("To Parts")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
                onTriggered: root.editorVm.splitSelectedToParts()
            }
        }
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        // Upstream Delete carries the Del annotation on Windows
        // (append_menu_item_delete, :534-545).
        CxMenuItem {
            text: qsTr("Delete") + "\t" + qsTr("Del")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        MenuSeparator { }
        // Five add-volume submenus plus the Height Range Modifier entry
        // (append_menu_items_add_volume :708-729 + layers_editing :731-739).
        AddVolumeTypeSubmenu { volumeType: 0 }
        AddVolumeTypeSubmenu { volumeType: 1 }
        AddVolumeTypeSubmenu { volumeType: 2 }
        AddVolumeTypeSubmenu { volumeType: 3 }
        AddVolumeTypeSubmenu { volumeType: 4 }
        CxMenuItem {
            text: qsTr("Height Range Modifier")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.requestObjectLayers()
        }
        MenuSeparator { }
        // Printable check item with the V annotation
        // (append_menu_item_printable :914-934, called :1514).
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
            }
            text: qsTr("Printable") + "\tV"
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(
                             !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
        }
        MenuSeparator { }
        // Upstream Auto Drop check item (append_menu_item_auto_drop :936-951,
        // called :1518). The Qt6 baseline has no per-selection auto_drop
        // state to read or toggle, so the entry stays honestly disabled with
        // a tooltip instead of a dead toggle.
        CxMenuItem {
            checkable: true
            checked: false
            enabled: false
            text: qsTr("Auto Drop")
            ToolTip.visible: autoDropHover.hovered
            ToolTip.text: qsTr("Per-selection auto drop is not available in this build")
            HoverHandler { id: autoDropHover }
        }
        MenuSeparator { }
        // Edit Process Settings + Copy/Paste Process Settings
        // (append_menu_item_per_object_process :2149-2187, called :1521).
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
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
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Reload from disk")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("reload")
            onTriggered: root.editorVm.reloadSelectedFromDisk()
        }
        // Replace 3D file... / Replace all with 3D files... (:1527-1528).
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
        // object_menu() dynamic tail (:1851-1860): convert entries, Flush
        // Options, Invalidate cut info, then Change Filament. The Edit
        // text/Edit SVG entries (:1856-1857) stay conditional on a text/SVG
        // volume selection upstream; the Qt6 menu family routes those
        // selections to textMenu/svgMenu, which carry them.
        ConvertUnitsItems { }
        FlushOptionsSubmenu { }
        CxMenuItem {
            text: qsTr("Invalidate cut info")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("invalidateCutInfo")
            onTriggered: root.editorVm.invalidateSelectedCutInfo()
        }
        ChangeFilamentSubmenu { }
    }

    CxMenu {
        id: partMenu
        onAboutToShow: ++root.menuRefreshTick
        // Upstream create_bbl_part_menu (GUI_Factories.cpp:1611-1643) plus
        // the part_menu() tail (:1873-1879): convert entries, Change Filament,
        // and "Edit in Parameter Table" re-mounted last. The conditional
        // "Edit text" entry (:1614) is skipped: text volumes reach textMenu
        // in the Qt6 menu family.
        CxMenuItem {
            text: qsTr("Delete") + "\t" + qsTr("Del")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("simplify")
                     && (root.editorVm.availableGizmoMask & (1 << 9)) !== 0
            onTriggered: root.editorVm.simplifyMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Subdivision mesh") + qsTr("(Lost color)")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("subdivide")
            onTriggered: root.editorVm.subdivideSelectedMesh()
        }
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        CxMenu {
            title: qsTr("Mirror")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("mirror")
            CxMenuItem { text: qsTr("Along X Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(0) }
            CxMenuItem { text: qsTr("Along Y Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(1) }
            CxMenuItem { text: qsTr("Along Z Axis"); onTriggered: root.editorVm.mirrorSelectedObjects(2) }
        }
        CxMenu {
            title: qsTr("Split")
            enabled: root.editorVm && (root.editorVm.contextActionAvailable("splitObjects")
                                       || root.editorVm.contextActionAvailable("splitParts"))
            CxMenuItem {
                text: qsTr("To Objects")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitObjects")
                onTriggered: root.editorVm.splitSelectedToObjects()
            }
            CxMenuItem {
                text: qsTr("To Parts")
                enabled: root.editorVm && root.editorVm.contextActionAvailable("splitParts")
                onTriggered: root.editorVm.splitSelectedToParts()
            }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
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
        ChangeTypeSubmenu { }
        CxMenuItem {
            text: qsTr("Reload from disk")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("reload")
            onTriggered: root.editorVm.reloadSelectedFromDisk()
        }
        // Upstream label "Replace 3D file..." (append_menu_item_replace_with_stl,
        // :1019-1024, called :1641), same as the object menu.
        CxMenuItem {
            text: qsTr("Replace 3D file...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replacePart")
            onTriggered: root.requestReplacePart()
        }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplaceAll()
        }
        // part_menu() tail (:1873-1879).
        ConvertUnitsItems { }
        ChangeFilamentSubmenu { }
        CxMenuItem {
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.canOpenSelectionSettings
            onTriggered: root.editorVm.requestSelectionSettings()
        }
    }

    CxMenu {
        id: textMenu
        onAboutToShow: ++root.menuRefreshTick
        // Upstream create_text_part_menu (GUI_Factories.cpp:1580-1594) plus
        // the text_part_menu() tail (:1880-1885): Change Filament then
        // "Edit in Parameter Table" re-mounted last.
        CxMenuItem {
            text: qsTr("Edit text")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("textEdit")
                     && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
            onTriggered: root.requestActivateGizmo(16)
        }
        CxMenuItem {
            text: qsTr("Delete") + "\t" + qsTr("Del")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify Model")
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
            text: qsTr("Copy process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copyProcessSettings")
            onTriggered: root.editorVm.copyContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Paste process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("pasteProcessSettings")
            onTriggered: root.editorVm.pasteContextProcessSettings()
        }
        ChangeTypeSubmenu { }
        ChangeFilamentSubmenu { }
        CxMenuItem {
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
    }

    CxMenu {
        id: svgMenu
        onAboutToShow: ++root.menuRefreshTick
        // Upstream create_svg_part_menu (GUI_Factories.cpp:1596-1609) plus
        // the svg_part_menu() tail (:1887-1893). No Center entry (unlike the
        // text menu, :1588 has it but the SVG menu does not).
        CxMenuItem {
            text: qsTr("Edit SVG")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("svgEdit")
                     && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
            onTriggered: root.requestActivateGizmo(17)
        }
        CxMenuItem {
            text: qsTr("Delete") + "\t" + qsTr("Del")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Simplify Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("simplify")
                     && (root.editorVm.availableGizmoMask & (1 << 9)) !== 0
            onTriggered: root.editorVm.simplifyMeshSelected()
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
            text: qsTr("Copy process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("copyProcessSettings")
            onTriggered: root.editorVm.copyContextProcessSettings()
        }
        CxMenuItem {
            text: qsTr("Paste process settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("pasteProcessSettings")
            onTriggered: root.editorVm.pasteContextProcessSettings()
        }
        ChangeTypeSubmenu { }
        ChangeFilamentSubmenu { }
        CxMenuItem {
            text: qsTr("Edit in Parameter Table")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
        }
    }

    CxMenu {
        id: multiMenu
        onAboutToShow: ++root.menuRefreshTick
        // Upstream multi_selection_menu normal multi-object branch
        // (GUI_Factories.cpp:1937-1966). The Qt6 family dispatch makes this
        // the only reachable multi branch: single volume selections resolve
        // to part/text/svg families and the all-plates branch needs a
        // multi-plate context the Qt6 viewport does not produce.
        CxMenuItem {
            text: qsTr("Assemble")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: root.editorVm.assembleSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Center")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("Drop")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("drop")
            onTriggered: root.editorVm.dropSelectedObjectsToBed()
        }
        CxMenuItem {
            text: qsTr("Fix Model")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("repair")
            onTriggered: root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("Delete") + "\t" + qsTr("Del")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("delete")
            onTriggered: root.requestConfirmDelete()
        }
        MenuSeparator { }
        CxMenuItem {
            checkable: true
            checked: {
                const tick = root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
            }
            text: qsTr("Printable") + "\tV"
            enabled: root.editorVm && root.editorVm.contextActionAvailable("printable")
            onTriggered: root.editorVm.setSelectedObjectsPrintable(
                             !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
        }
        MenuSeparator { }
        // Upstream carries an Auto Drop check item here too
        // (append_menu_item_set_auto_drop :2336-2354, called :1953); disabled
        // for the same no-backend reason as on the object menu.
        CxMenuItem {
            checkable: true
            checked: false
            enabled: false
            text: qsTr("Auto Drop")
            ToolTip.visible: multiAutoDropHover.hovered
            ToolTip.text: qsTr("Per-selection auto drop is not available in this build")
            HoverHandler { id: multiAutoDropHover }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Edit Process Settings")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("settings")
            onTriggered: root.editorVm.requestSelectionSettings()
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
        MenuSeparator { }
        ConvertUnitsItems { }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("replaceWithStl")
            onTriggered: root.requestReplaceAll()
        }
        ChangeFilamentSubmenu { }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("Export as one STL...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(false, false)
        }
        CxMenuItem {
            text: qsTr("Export as STLs...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("export")
            onTriggered: root.requestExport(true, false)
        }
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
        // Upstream create_plate_menu (GUI_Factories.cpp:1707-1813) plus the
        // plate_menu() tail (:2043-2048): Unlock/Lock and Edit Plate Name.
        CxMenuItem {
            text: qsTr("Select All")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateSelect")
            }
            onTriggered: root.editorVm.selectAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Select All Plates")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateSelectAllPlates")
            }
            onTriggered: root.editorVm.selectAllVisibleObjects()
        }
        CxMenuItem {
            text: qsTr("Delete All")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateClear")
            }
            onTriggered: root.requestConfirmClearPlate()
        }
        CxMenuItem {
            text: qsTr("Arrange")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateArrange")
            }
            onTriggered: root.editorVm.arrangePlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Reload All")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateReload")
            }
            onTriggered: root.editorVm.reloadAllOnPlate(root.editorVm.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("Auto Rotate")
            enabled: {
                void root.menuRefreshTick  // G-07: re-evaluate per popup
                return !!root.editorVm && root.editorVm.contextActionAvailable("plateOrient")
            }
            onTriggered: root.editorVm.autoOrientContextPlate()
        }
        CxMenuItem {
            text: qsTr("Delete Plate")
            enabled: root.editorVm && root.editorVm.canDeletePlate(root.editorVm.contextPlateIndex)
            onTriggered: root.requestConfirmDeletePlate()
        }
        MenuSeparator { }
        CxMenu {
            title: qsTr("Add Primitive")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateAddPrimitive")
            // Same INVALID-typed submenu as the default menu (upstream
            // create_plate_menu reuses append_submenu_add_generic with
            // ModelVolumeType::INVALID, :1789): six primitives plus Text/SVG.
            CxMenuItem { text: qsTr("Cube"); onTriggered: root.editorVm.addPrimitiveToContextPlate(0) }
            CxMenuItem { text: qsTr("Cylinder"); onTriggered: root.editorVm.addPrimitiveToContextPlate(2) }
            CxMenuItem { text: qsTr("Sphere"); onTriggered: root.editorVm.addPrimitiveToContextPlate(1) }
            CxMenuItem { text: qsTr("Cone"); onTriggered: root.editorVm.addPrimitiveToContextPlate(3) }
            CxMenuItem { text: qsTr("Disc"); onTriggered: root.editorVm.addPrimitiveToContextPlate(6) }
            CxMenuItem { text: qsTr("Torus"); onTriggered: root.editorVm.addPrimitiveToContextPlate(5) }
            MenuSeparator { }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 16)) !== 0
                text: qsTr("Text")
                onTriggered: root.requestActivateGizmo(16)
            }
            CxMenuItem {
                visible: !!root.editorVm
                         && (root.editorVm.availableGizmoMask & (1 << 17)) !== 0
                text: qsTr("SVG")
                onTriggered: root.requestActivateGizmo(17)
            }
        }
        CxMenu {
            title: qsTr("Add Handy models")
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
        CxMenuItem {
            text: qsTr("Add Models")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateAddModels")
            onTriggered: root.requestAddModels()
        }
        CxMenuItem {
            text: qsTr("Replace all with 3D files...")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateReplaceAll")
            onTriggered: root.requestReplaceAll()
        }
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
            text: qsTr("Edit Plate Name")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("plateRename")
            onTriggered: root.requestRenamePlate()
        }
        // Qt6-only tail retained under test locks (upstream has none of
        // these): Paste stays because QmlUiAuditTests.cpp:2104-2106 requires
        // the pasteToContextPlate( route; the plate lifecycle trio stays
        // because :3025-3029 requires clonePlate(/movePlate(/setPlatePrintable(.
        CxMenuItem {
            text: qsTr("Paste")
            enabled: root.editorVm && root.editorVm.contextActionAvailable("paste")
            onTriggered: root.editorVm.pasteToContextPlate()
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
