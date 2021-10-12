project(system)

target_include_directories(system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(system PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/./time.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./file.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./socket.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./thread.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./systeminfo.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./mutex.cc
		${CMAKE_CURRENT_SOURCE_DIR}/src/./system.cc)
