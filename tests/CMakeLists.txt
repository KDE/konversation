find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)

include(ECMAddTests)

ecm_add_test(
    testcommon.cpp
    ../src/common.cpp
    config/preferences.cpp
    TEST_NAME testcommon
    LINK_LIBRARIES KF5::I18n Qt5::Test
)
target_include_directories(testcommon PRIVATE ${CMAKE_SOURCE_DIR}/src)
