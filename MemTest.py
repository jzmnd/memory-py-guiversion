#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""
MemTest.py
UI file for MemTest with threading

Created by Jeremy Smith
University of California, Berkeley
j-smith@eecs.berkeley.edu
"""

import os
import sys
import time
import serial
from PyQt4 import QtGui
from PyQt4.QtCore import QThread, QObject, pyqtSignal, pyqtSlot
import numpy as np
import mainwindow

__author__ = "Jeremy Smith"
__version__ = "1.0"

# Define constants
# Serial port address
serialport = '/dev/cu.usbmodem1421'
# Maximum allowable pulse width [ms]
maxpulsewidth = 250
# True if paused to change voltage
paused = False
# Save path
save_path = os.path.dirname(__file__)


class MemTest(QObject):
    """Class for running memory test sequence on Arduino"""
    # Signals for output messages to command window
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)
    # Constants for Arduino ADC
    v_ratio = 5.0/1023
    time_step = 0.5

    def __init__(self, serialport, program, wordline=0, bitline=0, pattern=0, rtime=100, ftime=200, loop=1, gtime=100, baud=115200):
        QObject.__init__(self)
        _progdict = {'camread': 1, 'form': 2, 'writezero': 3, 'writeone': 4, 'stdread': 5}
        try:
            self._prognum = _progdict[program]    # Program number
        except KeyError:
            self.errormesg.emit("Program not specified (camread, form, writezero, writeone, stdread) for MemTest\n")
            return

        self._serialport = serialport             # Serial port
        self._program = program                   # Program name
        self._wordline = wordline                 # Word line number
        self._bitline = bitline                   # Bit line number
        self._pattern = pattern                   # Data pattern to match
        self._rtime = rtime                       # Read/write pulse time
        self._ftime = ftime                       # Forming/precharge pulse time
        self._loop = loop                         # Number of loops
        self._gtime = gtime                       # Ground time
        self._baud = baud                         # Arduino serial port bit rate
        self._connected = False                   # True when Arduino is connected
        self._datastring = ""                     # String for storing Arduino output
        # Header list
        self._headlist = []
        self._headlist.append("Program: {:d} {:s}".format(self._prognum, program))
        self._headlist.append("Address: WL {:d}   BL {:d}".format(wordline, bitline))
        self._headlist.append("Data Pattern: {:03b}".format(pattern))
        self._headlist.append("Read/write time: {:d} ms".format(rtime))
        self._headlist.append("Form/precharge time: {:d} ms".format(ftime))
        self._headlist.append("Number of read/write pulses: {:d}".format(loop))
        self._headlist.append("Ground time: {:d} ms".format(gtime))

    def display(self):
        """Displays settings for MemTest object"""
        self.message.emit('\n'.join(self._headlist))
        self.message.emit('\n')
        return

    def runprogram(self):
        """Connects to Arduino and runs program"""
        self.display()
        try:
            ser = serial.Serial(self._serialport, self._baud)   # Open the serial port that your Ardiono is connected to
        except OSError:
            self.errormesg.emit("\nPlease Connect Arduino via USB\n")
            time.sleep(1.0)
            return

        self.message.emit("Waiting to Connect...")
        while not self._connected:             # Loop until the Arduino tells us it is ready
            serin = ser.read()
            if serin == 'A':
                self.message.emit("Connected to Arduino\n\n")
                self._connected = True

        time.sleep(0.5)
        ser.write(str(self._prognum))          # Write the program command to the Arduino
        ser.write(chr(self._wordline))         # buffer 0
        ser.write(chr(self._bitline))          # buffer 1
        ser.write(chr(self._pattern))          # buffer 2
        ser.write(chr(self._rtime))            # buffer 3
        ser.write(chr(self._ftime))            # buffer 4
        ser.write(chr(self._loop))             # buffer 5
        ser.write(chr(self._gtime))            # buffer 6

        while self._connected:                 # Wait until the Arduino tells us it is finished
            serin = ser.read()
            if serin == 'A':
                continue
            if serin == 'Z':
                self._connected = False
                continue
            self.message.emit(serin)           # Sends tring to message window
            self._datastring += serin          # Stores the Arduino output as a string

        ser.close()                            # Close the port
        self.message.emit("Disconnected successfully\n")
        return

    def output(self):
        """Converts string from serial bus to a list and returns it along with header"""
        voltage_data = []
        if len(self._datastring) is not 0:
            for i in [x.strip().split(',') for x in self._datastring.strip().split('\n')[3:]]:
                if len(i) is not 2:
                    continue
                voltage_data.append([float(i[0])*self.time_step, float(i[1])*self.v_ratio])
            return voltage_data, self._headlist
        else:
            return

    def reset(self):
        """Resets status and empties stored data"""
        self._connected = False
        self._datastring = ""
        return


class RunWriteRead(QThread):
    """Thread class for running Write CAM Read functionality"""
    # Signals for output messages to command window
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)
    # Signal to activate next button when changing voltages manually
    changevoltage = pyqtSignal()
    # Signal to return data and data header
    result = pyqtSignal(list, list)

    def __init__(self, wline, arraysize, pattern, writePW, prePW, gndPW, loop):
        QThread.__init__(self)
        self.wline = wline                      # Word line
        self.arraysize = arraysize              # Memory array size (1,2,3)
        self.pattern = pattern                  # Pattern written into array
        self.writePW = int(writePW)             # Write pulse width
        self.prePW = int(prePW)                 # Precharge pulse width
        self.gndPW = int(gndPW)                 # Ground pulse width
        self.loop = int(loop)                   # Loop number

    def __del__(self):
        self.wait()

    def run(self):
        self.message.emit("Running...")
        self.message.emit("\n================================")
        self.message.emit("Memory Test Program")
        self.message.emit(__author__)
        self.message.emit("Version {:s}".format(__version__))
        self.message.emit("Email: j-smith@eecs.berkeley.edu")
        self.message.emit("================================\n")

        # List of write objects
        writelist = []
        # Use global paused variable to wait for user input
        global paused

        # Creates list of write objects for each CRS device
        for i, c in enumerate(self.pattern):
            if c == '0':
                write = MemTest(serialport, 'writezero', wordline=i//self.arraysize, bitline=i%self.arraysize, rtime=self.writePW, loop=self.loop, gtime=self.gndPW)
                write.message.connect(self.message.emit)
                write.errormesg.connect(self.errormesg.emit)
                writelist.append(write)
            elif c == '1':
                write = MemTest(serialport, 'writeone', wordline=i//self.arraysize, bitline=i%self.arraysize, rtime=self.writePW, loop=self.loop, gtime=self.gndPW)
                write.message.connect(self.message.emit)
                write.errormesg.connect(self.errormesg.emit)
                writelist.append(write)
            else:
                self.errormesg.emit("Write pattern error - use 0 or 1")

        # Runs writes and then does CAM read
        for a in range(2**self.arraysize):
            applypattern = MemTest(serialport, 'camread', wordline=self.wline, pattern=a, ftime=self.prePW)
            applypattern.message.connect(self.message.emit)
            applypattern.errormesg.connect(self.errormesg.emit)

            self.message.emit("\nSet WRITE voltage. Press Continue...\n")
            self.changevoltage.emit()
            paused = True
            while paused:
                self.sleep(1)

            for write in writelist:
                write.runprogram()

            self.message.emit("\nSet READ voltage. Press Continue...\n")
            self.changevoltage.emit()
            paused = True
            while paused:
                self.sleep(1)

            applypattern.runprogram()
            # Attempts to output data if it exists
            try:
                data, header = applypattern.output()
                self.result.emit(data, header)
            except TypeError:
                self.errormesg.emit("No data to output")

        self.message.emit("\n========================")
        self.message.emit("MEMORY TEST COMPLETE")
        self.message.emit("========================\n")
        return


class RunWriteOnly(QThread):
    """Thread class for running Write Only functionality"""
    # Signals for output messages to command window
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)
    # Signal to activate next button when changing voltages manually
    changevoltage = pyqtSignal()

    def __init__(self, arraysize, pattern, writePW, gndPW, loop):
        QThread.__init__(self)
        self.arraysize = arraysize              # Memory array size (1,2,3)
        self.pattern = pattern                  # Pattern written into array
        self.writePW = int(writePW)             # Write pulse width
        self.gndPW = int(gndPW)                 # Ground pulse width
        self.loop = int(loop)                   # Loop number

    def __del__(self):
        self.wait()

    def run(self):
        self.message.emit("Running...")
        self.message.emit("\n================================")
        self.message.emit("Memory Test Program (Write Only)")
        self.message.emit(__author__)
        self.message.emit("Version {:s}".format(__version__))
        self.message.emit("Email: j-smith@eecs.berkeley.edu")
        self.message.emit("================================\n")

        # List of write objects
        writelist = []
        global paused

        # Creates list of write objects for each CRS device
        for i, c in enumerate(self.pattern):
            if c == '0':
                write = MemTest(serialport, 'writezero', wordline=i//self.arraysize, bitline=i%self.arraysize, rtime=self.writePW, loop=self.loop, gtime=self.gndPW)
                write.message.connect(self.message.emit)
                write.errormesg.connect(self.errormesg.emit)
                writelist.append(write)
            elif c == '1':
                write = MemTest(serialport, 'writeone', wordline=i//self.arraysize, bitline=i%self.arraysize, rtime=self.writePW, loop=self.loop, gtime=self.gndPW)
                write.message.connect(self.message.emit)
                write.errormesg.connect(self.errormesg.emit)
                writelist.append(write)
            else:
                self.errormesg.emit("Write pattern error - use 0 or 1")

            self.message.emit("\nSet WRITE voltage. Press Continue...\n")
            self.changevoltage.emit()
            paused = True
            while paused:
                self.sleep(1)

            for write in writelist:
                write.runprogram()

        self.message.emit("\n========================")
        self.message.emit("MEMORY TEST COMPLETE")
        self.message.emit("========================\n")
        return


class SaveFile(QThread):
    """Thread class for saving output to file"""
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)

    def __init__(self, runresult, filename, pathname):
        QThread.__init__(self)
        self.runresult = runresult
        self.filename = filename
        self.pathname = pathname

    def __del__(self):
        self.wait()

    def run(self):
        self.message.emit("Saving...")
        # Create a results folder if it does not exist
        if "results" not in os.listdir(self.pathname):
            os.mkdir(os.path.join(self.pathname, "results"))
        # Opens out file and writes header followed by data
        with open(os.path.join(self.pathname, "results", self.filename), 'w') as outfile:
            for block in self.runresult:
                for line in block:
                    if type(line).__module__ == np.__name__:
                        outfile.write("{:.1f}\t{:.5f}\n".format(line[0], line[1]))
                    else:
                        outfile.write("{:s}\n".format(line))
                outfile.write('\n')
        self.message.emit("Saved as: {:s}".format(self.filename))
        return


class InitWriteRead(QThread):
    """Thread class for initializing variables for Write CAM read"""
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)

    def __init__(self, wline, arraysize, pattern, writePW, prePW, gndPW, loop):
        QThread.__init__(self)
        self.wline = wline
        self.arraysize = arraysize
        self.pattern = pattern
        self.writePW = int(writePW)
        self.prePW = int(prePW)
        self.gndPW = int(gndPW)
        self.loop = int(loop)

    def __del__(self):
        self.wait()

    def run(self):
        if len(self.pattern) != self.arraysize**2:
            self.errormesg.emit("Array size and write pattern do not match")
            return
        try:
            checkbin = int(self.pattern, 2)
        except ValueError:
            self.errormesg.emit("Pattern must be a binary number")
            return
        if self.writePW > maxpulsewidth:
            self.errormesg.emit("Write pulse must be less than {:d} ms".format(maxpulsewidth))
            return
        if self.prePW > maxpulsewidth:
            self.errormesg.emit("Precharge pulse must be less than {:d} ms".format(maxpulsewidth))
            return
        if self.gndPW > maxpulsewidth:
            self.errormesg.emit("Ground pulse must be less than {:d} ms".format(maxpulsewidth))
            return
        self.message.emit("Variables set.")
        return


class InitWriteOnly(QThread):
    """Thread class for initializing variables for Write Only"""
    message = pyqtSignal(str)
    errormesg = pyqtSignal(str)

    def __init__(self, arraysize, pattern, writePW, gndPW, loop):
        QThread.__init__(self)
        self.arraysize = arraysize
        self.pattern = pattern
        self.writePW = int(writePW)
        self.gndPW = int(gndPW)
        self.loop = int(loop)

    def __del__(self):
        self.wait()

    def run(self):
        if len(self.pattern) != self.arraysize**2:
            self.errormesg.emit("Array size and write pattern do not match")
            return
        try:
            checkbin = int(self.pattern, 2)
        except ValueError:
            self.errormesg.emit("Pattern must be a binary number")
            return
        if self.writePW > maxpulsewidth:
            self.errormesg.emit("Write pulse must be less than {:d} ms".format(maxpulsewidth))
            return
        if self.gndPW > maxpulsewidth:
            self.errormesg.emit("Ground pulse must be less than {:d} ms".format(maxpulsewidth))
            return
        self.message.emit("Variables set.")
        return


class MainApp(QtGui.QMainWindow, mainwindow.Ui_MainWindow):
    def __init__(self, parent=None):
        super(MainApp, self).__init__(parent)
        self.setupUi(self)

        self.pushButton_1.clicked.connect(self.init_WR)
        self.pushButton_2.clicked.connect(self.run_WR)
        self.pushButton_3.clicked.connect(self.init_WO)
        self.pushButton_4.clicked.connect(self.run_WO)
        self.pushButton_5.clicked.connect(self.savedata)
        self.pushButton_5.setEnabled(False)
        self.pushButton_6.clicked.connect(self.resetcnt)
        self.pushButton_7.setEnabled(False)
        self.pushButton_8.setEnabled(False)
        self.pushButton_9.setEnabled(False)
        self.pushButton_10.setEnabled(False)
        self.pushButton_9.clicked.connect(self.continue_run)
        self.pushButton_10.clicked.connect(self.continue_run)

        self.wline = 0
        self.arraysize = 1
        self.pattern = '0'
        self.writePW = 100
        self.prePW = 200
        self.gndPW = 100
        self.loop = 1

        self._count = 1
        self._fulldatabuffer = []

    @pyqtSlot(str)
    def writestr(self, text):
        if len(text) == 1:
            self.textBrowser_1.insertPlainText(text)
            self.textBrowser_2.insertPlainText(text)
        else:
            self.textBrowser_1.append(text)
            self.textBrowser_2.append(text)
        self.textBrowser_1.verticalScrollBar().setValue(self.textBrowser_1.verticalScrollBar().maximum())
        self.textBrowser_2.verticalScrollBar().setValue(self.textBrowser_2.verticalScrollBar().maximum())
        return

    @pyqtSlot(str)
    def writestrRED(self, text):
        self.textBrowser_1.setTextColor(QtGui.QColor('red'))
        self.textBrowser_2.setTextColor(QtGui.QColor('red'))
        self.textBrowser_1.append(text)
        self.textBrowser_2.append(text)
        self.textBrowser_1.setTextColor(QtGui.QColor('black'))
        self.textBrowser_2.setTextColor(QtGui.QColor('black'))
        self.textBrowser_1.verticalScrollBar().setValue(self.textBrowser_1.verticalScrollBar().maximum())
        self.textBrowser_2.verticalScrollBar().setValue(self.textBrowser_2.verticalScrollBar().maximum())
        return

    @pyqtSlot()
    def changevoltagewait(self):
        self.pushButton_9.setEnabled(True)
        self.pushButton_10.setEnabled(True)
        return

    @pyqtSlot(list, list)
    def storeresult(self, data, header):
        self._fulldatabuffer.append(header)
        self._fulldatabuffer.append(np.array(data))
        return

    def init_WR(self):
        self.pushButton_1.setEnabled(False)
        self.pushButton_3.setEnabled(False)

        self.wline = self.comboBox_1.currentIndex()
        self.arraysize = self.comboBox_2.currentIndex() + 1
        self.pattern = str(self.lineEdit_1.text())
        self.writePW = self.lineEdit_2.text()
        self.prePW = self.lineEdit_3.text()
        self.gndPW = self.lineEdit_4.text()
        self.loop = self.lineEdit_5.text()

        self.init_check = InitWriteRead(self.wline, self.arraysize, self.pattern, self.writePW, self.prePW, self.gndPW, self.loop)
        self.init_check.message.connect(self.writestr)
        self.init_check.errormesg.connect(self.writestrRED)
        self.init_check.finished.connect(self.done)

        self.init_check.start()

        self.pushButton_7.clicked.connect(self.init_check.terminate)
        self.pushButton_8.clicked.connect(self.init_check.terminate)
        self.pushButton_7.setEnabled(True)
        self.pushButton_8.setEnabled(True)
        return

    def init_WO(self):
        self.pushButton_1.setEnabled(False)
        self.pushButton_3.setEnabled(False)

        self.arraysize = self.comboBox_3.currentIndex() + 1
        self.pattern = str(self.lineEdit_11.text())
        self.writePW = self.lineEdit_6.text()
        self.gndPW = self.lineEdit_7.text()
        self.loop = self.lineEdit_8.text()

        self.init_check = InitWriteOnly(self.arraysize, self.pattern, self.writePW, self.gndPW, self.loop)
        self.init_check.message.connect(self.writestr)
        self.init_check.errormesg.connect(self.writestrRED)
        self.init_check.finished.connect(self.done)

        self.init_check.start()

        self.pushButton_7.clicked.connect(self.init_check.terminate)
        self.pushButton_8.clicked.connect(self.init_check.terminate)
        self.pushButton_7.setEnabled(True)
        self.pushButton_8.setEnabled(True)
        return

    def run_WR(self):
        self.pushButton_2.setEnabled(False)
        self.pushButton_4.setEnabled(False)

        self.runresult = RunWriteRead(self.wline, self.arraysize, self.pattern, self.writePW, self.prePW, self.gndPW, self.loop)
        self.runresult.message.connect(self.writestr)
        self.runresult.errormesg.connect(self.writestrRED)
        self.runresult.changevoltage.connect(self.changevoltagewait)
        self.runresult.finished.connect(self.done)
        self.runresult.result.connect(self.storeresult)

        self.runresult.start()

        self.pushButton_7.clicked.connect(self.runresult.terminate)
        self.pushButton_8.clicked.connect(self.runresult.terminate)
        self.pushButton_7.setEnabled(True)
        self.pushButton_8.setEnabled(True)
        return

    def run_WO(self):
        self.pushButton_2.setEnabled(False)
        self.pushButton_4.setEnabled(False)

        self.runresult = RunWriteOnly(self.arraysize, self.pattern, self.writePW, self.gndPW, self.loop)
        self.runresult.message.connect(self.writestr)
        self.runresult.errormesg.connect(self.writestrRED)
        self.runresult.changevoltage.connect(self.changevoltagewait)
        self.runresult.finished.connect(self.done)

        self.runresult.start()

        self.pushButton_7.clicked.connect(self.runresult.terminate)
        self.pushButton_8.clicked.connect(self.runresult.terminate)
        self.pushButton_7.setEnabled(True)
        self.pushButton_8.setEnabled(True)
        return

    def savedata(self):
        self.pushButton_5.setEnabled(False)
        datafilename = "{:s}_{:s}.txt".format(self.lineEdit_9.text(), self.lineEdit_10.text())

        self.save = SaveFile(self._fulldatabuffer, datafilename, save_path)
        self.save.message.connect(self.writestr)
        self.save.errormesg.connect(self.writestrRED)
        self.save.finished.connect(self.done)

        self.save.start()

        self._count += 1
        self.lineEdit_10.setText(str(self._count))
        return

    def resetcnt(self):
        self._count = 1
        self.lineEdit_10.setText("1")
        return

    def continue_run(self):
        global paused
        paused = False
        self.pushButton_9.setEnabled(False)
        self.pushButton_10.setEnabled(False)
        return

    def done(self):
        self.writestr("Done.")
        self.pushButton_1.setEnabled(True)
        self.pushButton_3.setEnabled(True)
        self.pushButton_2.setEnabled(True)
        self.pushButton_4.setEnabled(True)
        self.pushButton_5.setEnabled(True)
        self.pushButton_7.setEnabled(False)
        self.pushButton_8.setEnabled(False)
        self.pushButton_9.setEnabled(False)
        self.pushButton_10.setEnabled(False)
        return


def main():
    app = QtGui.QApplication(sys.argv)
    form = MainApp()
    form.show()
    app.exec_()
    return


if __name__ == "__main__":
    sys.exit(main())
