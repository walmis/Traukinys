# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'traukinys.ui'
#
# Created: Mon May 20 16:49:34 2013
#      by: pyside-uic 0.2.14 running on PySide 1.1.2
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_Traukinys(object):
    def setupUi(self, Traukinys):
        Traukinys.setObjectName("Traukinys")
        Traukinys.setEnabled(False)
        Traukinys.resize(350, 450)
        Traukinys.setMinimumSize(QtCore.QSize(350, 450))
        Traukinys.setAutoFillBackground(False)
        self.verticalLayout = QtGui.QVBoxLayout(Traukinys)
        self.verticalLayout.setObjectName("verticalLayout")
        self.frame = QtGui.QFrame(Traukinys)
        self.frame.setMinimumSize(QtCore.QSize(350, 450))
        self.frame.setFrameShape(QtGui.QFrame.StyledPanel)
        self.frame.setFrameShadow(QtGui.QFrame.Raised)
        self.frame.setObjectName("frame")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.frame)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.frame_2 = QtGui.QFrame(self.frame)
        self.frame_2.setMinimumSize(QtCore.QSize(320, 300))
        self.frame_2.setMaximumSize(QtCore.QSize(320, 300))
        self.frame_2.setBaseSize(QtCore.QSize(320, 300))
        self.frame_2.setFrameShape(QtGui.QFrame.StyledPanel)
        self.frame_2.setFrameShadow(QtGui.QFrame.Plain)
        self.frame_2.setObjectName("frame_2")
        self.label_6 = QtGui.QLabel(self.frame_2)
        self.label_6.setGeometry(QtCore.QRect(20, 130, 51, 17))
        font = QtGui.QFont()
        font.setWeight(75)
        font.setItalic(True)
        font.setBold(True)
        self.label_6.setFont(font)
        self.label_6.setObjectName("label_6")
        self.bStabis = QtGui.QPushButton(self.frame_2)
        self.bStabis.setGeometry(QtCore.QRect(220, 220, 81, 71))
        self.bStabis.setStyleSheet("color: rgb(214, 0, 0);\n"
"background-color: rgb(255, 170, 73)")
        self.bStabis.setObjectName("bStabis")
        self.pSignalas = QtGui.QProgressBar(self.frame_2)
        self.pSignalas.setGeometry(QtCore.QRect(110, 60, 191, 23))
        self.pSignalas.setMinimum(-91)
        self.pSignalas.setMaximum(0)
        self.pSignalas.setProperty("value", -90)
        self.pSignalas.setObjectName("pSignalas")
        self.label_3 = QtGui.QLabel(self.frame_2)
        self.label_3.setGeometry(QtCore.QRect(20, 60, 66, 21))
        self.label_3.setObjectName("label_3")
        self.label_5 = QtGui.QLabel(self.frame_2)
        self.label_5.setGeometry(QtCore.QRect(140, 130, 66, 17))
        font = QtGui.QFont()
        font.setWeight(75)
        font.setItalic(True)
        font.setBold(True)
        self.label_5.setFont(font)
        self.label_5.setObjectName("label_5")
        self.label_4 = QtGui.QLabel(self.frame_2)
        self.label_4.setGeometry(QtCore.QRect(10, 10, 141, 31))
        self.label_4.setAutoFillBackground(False)
        self.label_4.setTextFormat(QtCore.Qt.RichText)
        self.label_4.setObjectName("label_4")
        self.lSpeed = QtGui.QLabel(self.frame_2)
        self.lSpeed.setGeometry(QtCore.QRect(210, 140, 111, 20))
        self.lSpeed.setStyleSheet("font: 75 italic 12pt sans;")
        self.lSpeed.setObjectName("lSpeed")
        self.label = QtGui.QLabel(self.frame_2)
        self.label.setGeometry(QtCore.QRect(210, 100, 111, 41))
        self.label.setStyleSheet("font: 75 italic 20pt sans;")
        self.label.setTextFormat(QtCore.Qt.RichText)
        self.label.setObjectName("label")
        self.dGreitis = QtGui.QDial(self.frame_2)
        self.dGreitis.setGeometry(QtCore.QRect(20, 140, 161, 151))
        self.dGreitis.setStyleSheet("background: rgba(0, 25, 255, 128)")
        self.dGreitis.setMinimum(-100)
        self.dGreitis.setMaximum(100)
        self.dGreitis.setSingleStep(1)
        self.dGreitis.setInvertedAppearance(False)
        self.dGreitis.setWrapping(False)
        self.dGreitis.setNotchesVisible(True)
        self.dGreitis.setObjectName("dGreitis")
        self.stopButton = QtGui.QPushButton(self.frame_2)
        self.stopButton.setGeometry(QtCore.QRect(80, 100, 41, 31))
        self.stopButton.setStyleSheet("font: 1000 10pt \"Ubuntu\";")
        self.stopButton.setFlat(False)
        self.stopButton.setObjectName("stopButton")
        self.buttonRemote = QtGui.QToolButton(self.frame_2)
        self.buttonRemote.setGeometry(QtCore.QRect(280, 10, 27, 26))
        self.buttonRemote.setText("")
        self.buttonRemote.setCheckable(True)
        self.buttonRemote.setChecked(True)
        self.buttonRemote.setObjectName("buttonRemote")
        self.verticalLayout_2.addWidget(self.frame_2)
        spacerItem = QtGui.QSpacerItem(20, 40, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.verticalLayout_2.addItem(spacerItem)
        self.dockWidget = QtGui.QDockWidget(self.frame)
        self.dockWidget.setObjectName("dockWidget")
        self.dockWidgetContents_2 = QtGui.QWidget()
        self.dockWidgetContents_2.setObjectName("dockWidgetContents_2")
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.dockWidgetContents_2)
        self.verticalLayout_3.setObjectName("verticalLayout_3")
        self.infoText = QtGui.QPlainTextEdit(self.dockWidgetContents_2)
        self.infoText.setStyleSheet("font: 8pt \"monospace\";")
        self.infoText.setUndoRedoEnabled(False)
        self.infoText.setPlainText("")
        self.infoText.setTextInteractionFlags(QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.infoText.setObjectName("infoText")
        self.verticalLayout_3.addWidget(self.infoText)
        self.dockWidget.setWidget(self.dockWidgetContents_2)
        self.verticalLayout_2.addWidget(self.dockWidget)
        self.verticalLayout.addWidget(self.frame)

        self.retranslateUi(Traukinys)
        QtCore.QMetaObject.connectSlotsByName(Traukinys)

    def retranslateUi(self, Traukinys):
        Traukinys.setWindowTitle(QtGui.QApplication.translate("Traukinys", "Form", None, QtGui.QApplication.UnicodeUTF8))
        self.label_6.setText(QtGui.QApplication.translate("Traukinys", "Atgal", None, QtGui.QApplication.UnicodeUTF8))
        self.bStabis.setText(QtGui.QApplication.translate("Traukinys", "STABDIS", None, QtGui.QApplication.UnicodeUTF8))
        self.pSignalas.setFormat(QtGui.QApplication.translate("Traukinys", "%v dBm", None, QtGui.QApplication.UnicodeUTF8))
        self.label_3.setText(QtGui.QApplication.translate("Traukinys", "Signalas:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_5.setText(QtGui.QApplication.translate("Traukinys", "Pirmyn", None, QtGui.QApplication.UnicodeUTF8))
        self.label_4.setText(QtGui.QApplication.translate("Traukinys", "<html><head/><body><p><span style=\" font-size:14pt;\">Traukinys:</span></p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.lSpeed.setText(QtGui.QApplication.translate("Traukinys", "0", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("Traukinys", "Greitis:", None, QtGui.QApplication.UnicodeUTF8))
        self.stopButton.setText(QtGui.QApplication.translate("Traukinys", "0", None, QtGui.QApplication.UnicodeUTF8))
        self.buttonRemote.setToolTip(QtGui.QApplication.translate("Traukinys", "Nuotilinis valdymas", None, QtGui.QApplication.UnicodeUTF8))
        self.dockWidget.setWindowTitle(QtGui.QApplication.translate("Traukinys", "Info", None, QtGui.QApplication.UnicodeUTF8))
