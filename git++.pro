INCLUDEPATH = /usr/include \
include

HEADERS += \
    include/git/model/db/odb.hpp \
    include/git/global.h

SOURCES += \
    test/git/model/db/odb_test.cpp \
    src/git/lib/db/odb.cpp \
    test/git/lib/db/lib_odb_test.cpp \
    test/git/model/db/model_odb_test.cpp
