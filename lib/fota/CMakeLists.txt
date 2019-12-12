zephyr_library_named(telenor_fota)
zephyr_include_directories(include)
zephyr_library_sources(
    src/fota.c
    src/settings.c
)
zephyr_link_libraries(telenor_fota)
