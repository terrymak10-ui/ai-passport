idf_component_register(
    SRCS "sax_app.c" "sax_music.c" "sax_diagram.c" "sax_ui.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer
)

target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/sax_tc_14.c"
)
