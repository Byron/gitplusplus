INCLUDEPATH = /usr/include \
include

HEADERS += \
    include/git/model/db/odb.hpp \
    include/git/global.h.in \
    include/git/model/modelconfig.h \
    include/git/model/db/odb_alloc.hpp \
    include/git/model/db/odb_mem.hpp \
    include/git/model/db/odb_iter.hpp \
    include/git/model/db/odb_obj.hpp \
    include/git/model/db/odb_stream.hpp \
    include/git/lib/db/odb.h \
    include/git/lib/libconfig.h

SOURCES += \
    test/git/model/db/odb_test.cpp \
    src/git/lib/db/odb.cpp \
    test/git/lib/db/lib_odb_test.cpp \
    test/git/model/db/model_odb_test.cpp
