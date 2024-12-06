idf_component_register(
        SRCS "ic_74hc595_driver.c"
        INCLUDE_DIRS "include"
        REQUIRES driver
)

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})
