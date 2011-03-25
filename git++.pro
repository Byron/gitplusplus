INCLUDEPATH = /usr/local/include/ryppl-1.45.0 \
/usr/include \
test \
src

DEFINES += DEBUG

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
    src/gtl/db/odb_mem.hpp \
    src/gtl/db/odb_iter.hpp \
    src/gtl/db/odb_alloc.hpp \
    src/gtl/db/odb.hpp \
    src/git/db/traits.hpp \
    src/git/obj/object.hpp \
    src/git/obj/tree.h \
    src/git/obj/multiobj.h \
    src/git/obj/commit.h \
    src/git/obj/blob.h \
    src/git/obj/tag.h \
    src/gtl/db/generator_filter.hpp \
    src/gtl/db/generator_stream.hpp \
    src/gtl/db/odb_object.hpp \
    src/gtl/db/generator.hpp \
    src/gtl/db/hash.hpp \
    src/gtl/util.hpp \
    src/git/db/policy.hpp \
    src/git/obj/stream.h \
    src/gtl/db/hash_generator_stream.hpp \
    src/gtl/db/hash_generator_filter.hpp \
    src/gtl/db/hash_generator.hpp \
    src/git/db/util.hpp \
    src/gtl/db/odb_loose.hpp \
    src/git/db/odb_mem.h \
    src/git/db/odb_loose.h \
    test/git/fixture.hpp \
    src/gtl/db/odb_pack.hpp \
    src/git/db/odb_pack.h \
    src/git/db/pack_file.h \
    src/git/db/pack_stream.h \
    src/gtl/db/sliding_mmap_device.hpp \
    src/gtl/db/mapped_memory_manager.hpp \
    test/gtl/fixture.hpp \
    src/gtl/db/zlib_mmap_device.hpp

SOURCES += \
    test/git/model/db/odb_test.cpp \
    src/git/lib/db/odb.cpp \
    test/git/model/db/model_odb_test.cpp \
    test/git/db/lib_odb_test.cpp \
    test/gitmodel/db/model_odb_test.cpp \
    src/git/db/sha1.cpp \
    src/git/db/sha1_gen.cpp \
    test/git/db/sha1_performance_test.cpp \
    test/gtl/db/model_odb_test.cpp \
    src/git/obj/Tree.cpp \
    src/git/obj/tree.cpp \
    src/git/obj/commit.cpp \
    src/git/obj/blob.cpp \
    src/git/obj/tag.cpp \
    src/git/obj/object.cpp \
    src/git/db/policy.cpp \
    src/git/obj/stream.cpp \
    src/git/db/util.cpp \
    src/git/db/odb_mem.cpp \
    src/git/db/odb_loose.cpp \
    src/git/obj/multiobj.cpp \
    test/git/db/looseodb_performance_test.cpp \
    src/git/db/odb_pack.cpp \
    src/git/db/pack_file.cpp \
    src/git/db/pack_stream.cpp \
    test/git/db/packodb_performance_test.cpp
