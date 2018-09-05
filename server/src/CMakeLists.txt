add_executable(server main.cpp observers.h precmp.h precmp.cpp)
enable_precompiled_header(server precmp.h precmp.cpp)
target_link_libraries(server ${Boost_LIBRARIES} ${WEBRTC_LIBRARIES} Secur32 Msdmo Winmm Dmoguids wmcodecdspuuid)