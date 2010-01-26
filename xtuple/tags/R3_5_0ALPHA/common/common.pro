include( ../global.pri )

TARGET      = xtuplecommon
TEMPLATE    = lib
CONFIG      += qt warn_on staticlib
DEFINES     += MAKELIB

DESTDIR = ../lib
OBJECTS_DIR = tmp
MOC_DIR     = tmp
UI_DIR      = tmp

SOURCES = calendarcontrol.cpp \
          calendargraphicsitem.cpp \
          format.cpp \
          graphicstextbuttonitem.cpp \
          login2.cpp \
          login2Options.cpp \
          metrics.cpp \
          metricsenc.cpp \
          qbase64encode.cpp \
          qmd5.cpp \
          storedProcErrorLookup.cpp \
          xbase32.cpp \
          xtsettings.cpp
HEADERS = calendarcontrol.h \
          calendargraphicsitem.h \
          format.h \
          graphicstextbuttonitem.h \
          login2.h \
          login2Options.h \
          metrics.h \
          metricsenc.h \
          qbase64encode.h \
          qmd5.h \
          storedProcErrorLookup.h \
          xbase32.h \
          xtsettings.h
FORMS = login2.ui login2Options.ui


#The following line was inserted by qt3to4
QT +=  sql

RESOURCES += xTupleCommon.qrc
