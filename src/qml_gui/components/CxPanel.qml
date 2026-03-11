import QtQuick
import ".."

Rectangle {
    id: root

    enum Surface { Panel, Floating, Card, Elevated, Inset, Transparent }

    property int cxSurface: 0
    readonly property color resolvedColor: {
        if (root.cxSurface === 1)
            return Theme.bgFloating
        if (root.cxSurface === 2)
            return Theme.bgSurface
        if (root.cxSurface === 3)
            return Theme.bgElevated
        if (root.cxSurface === 4)
            return Theme.bgInset
        if (root.cxSurface === 5)
            return "transparent"
        return Theme.bgPanel
    }
    readonly property int resolvedBorderWidth: root.cxSurface === 5 ? 0 : 1
    readonly property color resolvedBorderColor: root.cxSurface === 2 ? Theme.borderDefault : Theme.borderSubtle

    radius: Theme.radiusXL
    color: resolvedColor
    border.width: resolvedBorderWidth
    border.color: resolvedBorderColor
}