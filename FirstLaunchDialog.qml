import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: firstLaunchDialog
    modal: true
    anchors.centerIn: parent
    width: 440
    padding: 28

    signal shortcutsRequested()
    signal dismissed()

    title: "First Launch"

    background: Rectangle {
        color: "#090b12"
        border.color: "#26324b"
        border.width: 1
        radius: 14
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56
            Layout.preferredHeight: 56
            radius: 12
            color: "#22d3ee"
            Text {
                anchors.centerIn: parent
                text: "PX"
                color: "#07101c"
                font.bold: true
                font.pixelSize: 18
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "Welcome to Pixelator"
                color: "#e5edff"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Text {
                text: "Would you like to add Pixelator to your system shortcuts for quick access?"
                color: "#a1aec5"
                font.pixelSize: 13
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 8

            Button {
                text: "Not Now"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                palette.button: "#18233a"
                palette.buttonText: "#b9c8e5"

                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#263858" : "#18233a"
                    border.color: "#26324b"
                    border.width: 1
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.palette.buttonText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }

                onClicked: {
                    firstLaunchDialog.dismissed()
                    firstLaunchDialog.close()
                }
            }

            Button {
                text: "Create Shortcuts"
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                background: Rectangle {
                    radius: 8
                    color: parent.hovered ? "#2dd4bf" : "#22d3ee"
                }

                contentItem: Text {
                    text: parent.text
                    color: "#07101c"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                    font.pixelSize: 13
                }

                onClicked: {
                    firstLaunchDialog.shortcutsRequested()
                    firstLaunchDialog.close()
                }
            }
        }
    }
}
