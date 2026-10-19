import QtQuick
pragma ComponentBehavior: Bound
import OWzx

// CxDialogHost — deferred construction host for the shell's dialogs.
//
// STARTUP-DEFER stage 2 (2026-10-19): the ~30 eagerly instantiated dialogs
// cost ~490ms of engine->load (measured 2043ms -> 1556ms). The host keeps the
// dialog's id and API surface: `dlg.open()` synchronously forces construction
// (inline sourceComponent Loaders build synchronously) and then opens, so
// every existing call site — including early deep links and the first-run
// ConfigWizard — keeps working before the deferred gate flips. After the
// first paint the shell sets dialogsReady and the rest of the dialogs build
// off the critical path.
Loader {
    id: root

    /// Flips true shortly after the first paint; open()/invoke() do not wait
    /// for it (they force construction on demand).
    required property bool deferredGate

    active: root.deferredGate

    function open()
    {
        root.active = true
        if (root.item)
            root.item.open()
    }

    /// Non-modal dialogs (SettingsDialog) surface via show()/requestActivate.
    function show()
    {
        root.active = true
        if (root.item && typeof root.item.show === "function")
            root.item.show()
        else if (root.item)
            root.item.open()
    }

    function close()
    {
        if (root.item)
            root.item.close()
    }

    /// Property write that forces construction first (dialogs configured
    /// before being shown, e.g. EditGCodeDialog's title/key/body).
    function setProp(name, value)
    {
        root.active = true
        if (root.item)
            root.item[name] = value
    }

    /// Method call that forces construction first (FileArchiveDialog.openFor).
    function invoke(method)
    {
        root.active = true
        if (root.item) {
            var args = Array.prototype.slice.call(arguments, 1)
            root.item[method].apply(root.item, args)
        }
    }
}
