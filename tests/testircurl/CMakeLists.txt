
ecm_add_test(
    testircurl.cpp
    preferences.cpp
    irccharsets.cpp
    ../../src/connectionsettings.cpp
    ../../src/identity.cpp
    ../../src/irc/servergroupsettings.cpp
    ../../src/irc/serversettings.cpp
    TEST_NAME testIrcUrl
    LINK_LIBRARIES Qt::Test Qt6::Core5Compat
)

target_include_directories(testIrcUrl PRIVATE ${CMAKE_SOURCE_DIR}/src/irc ${CMAKE_SOURCE_DIR}/src/)
