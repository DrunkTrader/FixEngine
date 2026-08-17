add_executable(floofix_tokenizer_tests
    tokenizer_test.cpp
)

target_link_libraries(floofix_tokenizer_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_tokenizer_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_parser_tests
    parser_test.cpp
)

target_link_libraries(floofix_parser_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_parser_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_validator_tests
    validator_test.cpp
)

target_link_libraries(floofix_validator_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_validator_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_storage_tests
    storage_test.cpp
)

target_link_libraries(floofix_storage_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_storage_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_storage_layout_tests
    storage_layout_test.cpp
)

target_link_libraries(floofix_storage_layout_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_storage_layout_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_ingestion_tests
    ingestion_test.cpp
)

target_link_libraries(floofix_ingestion_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_ingestion_tests
    PRIVATE
        cxx_std_20
)

add_executable(floofix_engine_tests
    fix_engine_test.cpp
)

target_link_libraries(floofix_engine_tests
    PRIVATE
        floofix
)

target_compile_features(floofix_engine_tests
    PRIVATE
        cxx_std_20
)

if(MSVC)

    target_compile_options(
        floofix_tokenizer_tests
        PRIVATE
            /W4
            /permissive-
    )

    target_compile_options(
        floofix_parser_tests
        PRIVATE
            /W4
            /permissive-
    )

    target_compile_options(
        floofix_validator_tests
        PRIVATE
            /W4
            /permissive-
    )

    target_compile_options(
        floofix_ingestion_tests
        PRIVATE
            /W4
            /permissive-
    )
    
    target_compile_options(
        floofix_engine_tests
        PRIVATE
            /W4
            /permissive-
    )

else()

    target_compile_options(
        floofix_tokenizer_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )

    target_compile_options(
        floofix_parser_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )

    target_compile_options(
        floofix_validator_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )

    target_compile_options(
        floofix_storage_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )

    target_compile_options(
        floofix_engine_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
    )   
endif()


enable_testing()

add_test(
    NAME floofix_tokenizer_tests
    COMMAND floofix_tokenizer_tests
)

add_test(
    NAME floofix_parser_tests
    COMMAND floofix_parser_tests
)

add_test(
    NAME floofix_validator_tests
    COMMAND floofix_validator_tests
)

add_test(
    NAME floofix_storage_tests
    COMMAND floofix_storage_tests
)

add_test(
    NAME floofix_storage_layout_tests
    COMMAND floofix_storage_layout_tests
)

add_test(
    NAME floofix_ingestion_tests
    COMMAND floofix_ingestion_tests
)

add_test(
    NAME floofix_engine_tests
    COMMAND floofix_engine_tests
)