INCLUDEPATH = /usr/include \
test \
src

HEADERS += \
    include/git/model/db/odb.hpp \
    include/git/global.h.in \
    include/git/model/db/odb_alloc.hpp \
    include/git/model/db/odb_mem.hpp \
    include/git/model/db/odb_iter.hpp \
    include/git/model/db/odb_obj.hpp \
    include/git/model/db/odb_stream.hpp \
    include/git/lib/db/odb.h \
    include/git/lib/libconfig.h \
    model/git/gitmodelconfig.h \
    model/git/db/odb_stream.hpp \
    model/git/db/odb_obj.hpp \
    model/git/db/odb_mem.hpp \
    model/git/db/odb_iter.hpp \
    model/git/db/odb_alloc.hpp \
    model/git/db/odb.hpp \
    src/git/db/odb.h \
    src/gitmodel/db/odb_stream.hpp \
    src/gitmodel/db/odb_obj.hpp \
    src/gitmodel/db/odb_mem.hpp \
    src/gitmodel/db/odb_alloc.hpp \
    src/gitmodel/db/odb.hpp \
    src/gitmodel/config.h \
    src/git/config.h.in \
    test/git/util.hpp \
    test/git/testutil.hpp \
    src/git/db/sha1.h \
    src/git/db/sha1_gen.h \
    test/gtl/testutil.hpp \
    src/gtl/config.h \
    src/gtl/db/odb_stream.hpp \
    src/gtl/db/odb_mem.hpp \
    src/gtl/db/odb_iter.hpp \
    src/gtl/db/odb_alloc.hpp \
    src/gtl/db/odb.hpp \
    src/git/db/traits.hpp \
    src/git/obj/object.hpp \
    src/git/obj/tree.h

SOURCES += \
    test/git/model/db/odb_test.cpp \
    src/git/lib/db/odb.cpp \
    test/git/model/db/model_odb_test.cpp \
    src/git/db/odb.cpp \
    test/git/db/lib_odb_test.cpp \
    test/gitmodel/db/model_odb_test.cpp \
    src/git/db/sha1.cpp \
    src/git/db/sha1_gen.cpp \
    test/git/db/sha1_performance_test.cpp \
    test/gtl/db/model_odb_test.cpp \
    src/git/obj/Tree.cpp \
    src/git/obj/tree.cpp
