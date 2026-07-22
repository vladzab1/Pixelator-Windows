import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1040
    height: 720
    minimumWidth: 720
    minimumHeight: 540
    visible: true
    title: "PIXELATOR // sprite cleaner"
    color: "#090b12"

    FirstLaunchDialog {
        id: firstLaunchDialog
        onShortcutsRequested: {
            if (shortcutManager.createShortcuts()) {
                shortcutManager.markFirstLaunchDone()
                showNotification("Shortcuts created successfully!")
            } else {
                showNotification("Failed to create shortcuts.")
            }
        }
        onDismissed: {
            shortcutManager.markFirstLaunchDone()
        }
    }

    function showNotification(message) {
        statusNotification.text = message
        statusNotificationTimer.start()
    }

    Timer {
        id: statusNotificationTimer
        interval: 3000
        onTriggered: statusNotification.text = ""
    }

    property int completed: 0
    property int total: 0
    property string currentFile: "Drop images to begin"
    property var modes: ["Smart auto", "16 Micro", "32 Retro", "48 Dendy", "64 Classic", "96 Medium", "128 Crisp", "192 Detailed", "256 Clean", "320 Large", "400 Max"]
    property var palettes: ["True Color", "GameBoy Original", "NES / Famicom", "SEGA Mega Drive / Genesis", "PICO-8"]

    Connections {
        target: pixelWorker
        function onProgressChanged(done, count, fileName) { root.completed = done; root.total = count; root.currentFile = fileName }
        function onStatusChanged(message) { root.currentFile = message }
        function onProcessingFinished(saved, failed) { root.currentFile = "Finished — " + saved + " saved, " + failed + " failed" }
    }

    component DarkCombo: ComboBox {
        id: control
        implicitHeight: 42
        font.pixelSize: 13
        font.weight: Font.DemiBold
        delegate: ItemDelegate {
            width: control.width
            contentItem: Text { text: modelData; color: highlighted ? "#0b1020" : "#dce5ff"; font: control.font; verticalAlignment: Text.AlignVCenter }
            highlighted: control.highlightedIndex === index
            background: Rectangle { color: parent.highlighted ? "#67e8f9" : "#121827" }
        }
        contentItem: Text { leftPadding: 14; rightPadding: 34; text: control.displayText; color: "#dce5ff"; font: control.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        background: Rectangle { radius: 8; color: "#111827"; border.color: control.hovered ? "#67e8f9" : "#26324b"; border.width: 1 }
        indicator: Text { x: control.width - width - 13; y: (control.height - height) / 2; text: "⌄"; color: "#67e8f9"; font.pixelSize: 19 }
        popup: Popup { y: control.height + 5; width: control.width; implicitHeight: contentItem.implicitHeight; padding: 4
            contentItem: ListView { clip: true; implicitHeight: Math.min(contentHeight, 250); model: control.popup.visible ? control.delegateModel : null; currentIndex: control.highlightedIndex }
            background: Rectangle { color: "#111827"; radius: 8; border.color: "#26324b" }
        }
    }

    header: Rectangle {
        height: 76; color: "#0c101b"
        RowLayout { anchors.fill: parent; anchors.leftMargin: 30; anchors.rightMargin: 30
            Rectangle { width: 38; height: 38; radius: 9; color: "#22d3ee"; Text { anchors.centerIn: parent; text: "PX"; color: "#07101c"; font.bold: true; font.pixelSize: 13 } }
            ColumnLayout { spacing: 1
                Text { text: "PIXELATOR"; color: "#e5edff"; font.pixelSize: 20; font.bold: true; font.letterSpacing: 2 }
                Text { text: "SPRITE CLEANER / BATCH EDITION"; color: "#6f82a8"; font.pixelSize: 10; font.letterSpacing: 1.3 }
            }
            Item { Layout.fillWidth: true }
            Text { id: statusNotification; text: ""; color: "#5eead4"; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1; Layout.alignment: Qt.AlignRight }
            Item { Layout.preferredWidth: statusNotification.visible ? 10 : 0 }
            Text { text: pixelWorker.processing ? "● PROCESSING" : "● READY"; color: pixelWorker.processing ? "#fbbf24" : "#5eead4"; font.bold: true; font.pixelSize: 11; font.letterSpacing: 1 }
        }
    }

    Component.onCompleted: {
        if (shortcutManager.isFirstLaunch()) {
            firstLaunchDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 18
        Rectangle {
            id: dropZone; Layout.fillWidth: true; Layout.preferredHeight: 142; radius: 14
            color: dropArea.containsDrag ? "#10253a" : "#0e1422"
            border.width: 2; border.color: dropArea.containsDrag ? "#67e8f9" : "#26324b"
            Behavior on color { ColorAnimation { duration: 120 } }
            Column { anchors.centerIn: parent; spacing: 8
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "＋"; color: "#67e8f9"; font.pixelSize: 34; font.weight: Font.Light }
                Text { text: "DROP YOUR ASSETS HERE"; color: "#dce5ff"; font.bold: true; font.pixelSize: 14; font.letterSpacing: 1.5 }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "PNG · JPG · JPEG · WEBP  —  outputs saved next to originals"; color: "#7182a5"; font.pixelSize: 11 }
            }
            DropArea { id: dropArea; anchors.fill: parent; keys: ["text/uri-list"]; onDropped: function(drop) { pixelWorker.addFiles(drop.urls); drop.acceptProposedAction() } }
        }
        GridLayout { Layout.fillWidth: true; columns: width > 740 ? 4 : 2; columnSpacing: 12; rowSpacing: 9
            Text { text: "PIXEL SCALE"; color: "#7182a5"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.columnSpan: width > 740 ? 1 : 1 }
            Text { text: "COLOR PALETTE"; color: "#7182a5"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            Item { visible: width > 740 }
            Item { visible: width > 740 }
            DarkCombo { id: modeCombo; model: root.modes; Layout.fillWidth: true; Layout.columnSpan: width > 740 ? 2 : 1 }
            DarkCombo { id: paletteCombo; model: root.palettes; Layout.fillWidth: true; Layout.columnSpan: width > 740 ? 2 : 1 }
        }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 160; radius: 14; color: "#0e1422"; border.color: "#1c2940"
            ListView { id: queueView; anchors.fill: parent; anchors.margins: 10; clip: true; spacing: 7; model: pixelWorker.queue
                delegate: Rectangle { required property string modelData; required property int index; width: queueView.width; height: 64; radius: 9; color: "#121b2c"; border.color: "#22314a"
                    RowLayout { anchors.fill: parent; anchors.margins: 10; spacing: 12
                        Rectangle { Layout.preferredWidth: 38; Layout.preferredHeight: 38; radius: 6; color: "#172b42"; Text { anchors.centerIn: parent; text: "IMG"; color: "#67e8f9"; font.bold: true; font.pixelSize: 9 } }
                        ColumnLayout { Layout.fillWidth: true; spacing: 3
                            Text { text: modelData.split("/").pop(); color: "#dce5ff"; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: modelData; color: "#65789b"; font.pixelSize: 10; elide: Text.ElideMiddle; Layout.fillWidth: true }
                        }
                        Button { enabled: !pixelWorker.processing; text: "×"; onClicked: pixelWorker.removeAt(index)
                            background: Rectangle { implicitWidth: 30; implicitHeight: 30; radius: 6; color: parent.hovered ? "#fb7185" : "#26324b" }
                            contentItem: Text { text: parent.text; color: "#e5edff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 18 }
                        }
                    }
                }
                Text { anchors.centerIn: parent; visible: queueView.count === 0; text: "YOUR PROCESSING QUEUE IS EMPTY"; color: "#526684"; font.pixelSize: 12; font.letterSpacing: 1.2 }
                ScrollBar.vertical: ScrollBar { }
            }
        }
        ColumnLayout { Layout.fillWidth: true; spacing: 8
            ProgressBar { Layout.fillWidth: true; visible: pixelWorker.processing; from: 0; to: Math.max(1, root.total); value: root.completed
                background: Rectangle { implicitHeight: 7; radius: 4; color: "#172238" }
                contentItem: Item { implicitHeight: 7; Rectangle { width: parent.width * (parent.parent.value / parent.parent.to); height: parent.height; radius: 4; color: "#22d3ee" } }
            }
            Text { Layout.alignment: Qt.AlignHCenter; text: root.currentFile; color: "#7182a5"; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.maximumWidth: parent.width }
            RowLayout { Layout.fillWidth: true; spacing: 12
                Button { text: "CLEAR QUEUE"; enabled: !pixelWorker.processing && pixelWorker.queue.length > 0; onClicked: pixelWorker.clearQueue(); Layout.preferredHeight: 44
                    background: Rectangle { radius: 9; color: parent.enabled ? (parent.hovered ? "#263858" : "#18233a") : "#121827" }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "#b9c8e5" : "#4c5c79"; font.bold: true; font.letterSpacing: 1; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Button { text: pixelWorker.processing ? "PROCESSING…" : "▶  PROCESS ALL"; enabled: !pixelWorker.processing && pixelWorker.queue.length > 0; onClicked: pixelWorker.processAll(modeCombo.currentText, paletteCombo.currentText); Layout.fillWidth: true; Layout.preferredHeight: 44
                    background: Rectangle { radius: 9; color: parent.enabled ? (parent.hovered ? "#67e8f9" : "#22d3ee") : "#172238" }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "#07101c" : "#50627e"; font.bold: true; font.letterSpacing: 1.2; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }
}
