# Registers a QtTest executable named after its source file.
function(add_drawer_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE drawer_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_drawer_test(tst_smoke)
