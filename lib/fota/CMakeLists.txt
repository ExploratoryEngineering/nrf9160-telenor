if(CONFIG_TELENOR_FOTA)

add_library(telenor_fota)
target_link_libraries(telenor_fota PUBLIC zephyr_interface)
target_include_directories(telenor_fota PUBLIC include)
target_sources(telenor_fota PRIVATE
    src/fota.c
    src/settings.c
)
zephyr_link_libraries(telenor_fota)

endif()
