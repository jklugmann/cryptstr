# conf
CONFIG -= qt
CONFIG += c++17

# inputs
HEADERS += \
    src/cryptstr.hpp
SOURCES += \
        main.cpp

INCLUDEPATH += src/

# outputs
DESTDIR = .
OBJECTS_DIR = obj/
TARGET = a.out
