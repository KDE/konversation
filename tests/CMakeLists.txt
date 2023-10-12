find_package(Qt6Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_test(
    testcommon.cpp
    ../src/common.cpp
    config/preferences.cpp
    TEST_NAME testcommon
    LINK_LIBRARIES KF6::I18n Qt::Test
)
target_include_directories(testcommon PRIVATE ${CMAKE_SOURCE_DIR}/src)
