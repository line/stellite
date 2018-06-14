################################################################################
# env setting
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


################################################################################
# googletest
INCLUDE_DIRECTORIES( googletest/include )
ADD_LIBRARY( googletest STATIC IMPORTED )
SET_TARGET_PROPERTIES( googletest PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/googletest/bin/libgtest.a )
MESSAGE("GOOGLETEST LIB PATH : ${CMAKE_SOURCE_DIR}/googletest/bin/libgtest.a")



################################################################################
# stellite
INCLUDE_DIRECTORIES( stellite/include )
ADD_LIBRARY( stellite SHARED IMPORTED )
SET_TARGET_PROPERTIES( stellite PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/stellite/bin/libstellite_http_client.so )
LINK_DIRECTORIES( ${CMAKE_SOURCE_DIR}/stellite/bin/ )
MESSAGE("STELLITE LIB PATH : ${CMAKE_SOURCE_DIR}/stellite/bin/libstellite_http_client.so")



################################################################################
# main build
ADD_EXECUTABLE( MyStelliteLinuxTest 
                stellite_get_quic_unittest.cpp
                MyStelliteHttpCallback.cpp)

TARGET_LINK_LIBRARIES(  MyStelliteLinuxTest 
                        googletest
                        stellite
                        pthread)



################################################################################
# install dependencies
ADD_CUSTOM_COMMAND (
    TARGET MyStelliteLinuxTest
    COMMENT "copy so files ..."
    COMMAND sudo cp ${CMAKE_SOURCE_DIR}/stellite/bin/libstellite_http_client.so /usr/local/lib
    COMMENT "ldconfig ..."
    COMMAND sudo ldconfig
)