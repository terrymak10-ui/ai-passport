cmake_minimum_required(VERSION 3.16)
project(sax_ui_host C CXX)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
get_filename_component(SAX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_subdirectory(${SAX_ROOT}/managed_components/lvgl__lvgl lvgl)
add_executable(preview ${SAX_ROOT}/tests/test_sax_ui_host.c
 ${SAX_ROOT}/main/sax_ui.c ${SAX_ROOT}/main/sax_diagram.c ${SAX_ROOT}/main/sax_music.c
 ${SAX_ROOT}/assets/fonts/sax_tc_14.c
 ${SAX_ROOT}/components/bsp/src/bsp_display_rounding.c)
target_include_directories(preview PRIVATE ${SAX_ROOT}/main ${SAX_ROOT}/components/bsp/src)
target_link_libraries(preview PRIVATE lvgl m)
