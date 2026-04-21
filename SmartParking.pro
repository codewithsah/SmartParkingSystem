# Qt modules jo hume chahiye (sql database aur widgets UI ke liye)
QT       += core gui sql widgets

# Agar Qt 5 ya usse upar use kar rahe hain toh widgets module add hota hai
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Project ka naam jo .exe file banayega
TARGET = SmartParking
TEMPLATE = app

# Modern C++ standard use karne ke liye
CONFIG += c++17

# Saari C++ Source files jo humne banayi hain
SOURCES += main.cpp \
           mainwindow.cpp

# Header file jahan variables aur functions declare hain
HEADERS += mainwindow.h

# Windows par console window hide karne ke liye
CONFIG += no_lflags_qt_main