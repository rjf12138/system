project(system)

target_include_directories(system PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

target_sources(system PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui_tables.cc
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui.cc
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui_impl_sdl2.cc
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui_impl_opengl3.cc
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui_draw.cc
		${CMAKE_CURRENT_SOURCE_DIR}/imgui/./imgui_widgets.cc)
