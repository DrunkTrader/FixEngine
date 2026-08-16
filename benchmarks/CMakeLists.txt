add_executable(floofix_storage_bench
    storage_bench.cpp
)

target_link_libraries(floofix_storage_bench
    PRIVATE
        floofix
)

target_compile_features(floofix_storage_bench
    PRIVATE
        cxx_std_20
)

if(MSVC)

    target_compile_options(
        floofix_storage_bench
        PRIVATE
            /W4
            /permissive-
    )

else()

    target_compile_options(
        floofix_storage_bench
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -O3
    )

endif()