set(pv_SRC
    ${CMAKE_BINARY_DIR}/pv/pv_module_wrapper.cpp
)
 
set(pv_INCLUDE_DIRECTORIES
    ${SHIBOKEN_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${PYSIDE_INCLUDE_DIR}
    ${PYSIDE_INCLUDE_DIR}/QtCore
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
)
 
set(pv_LINK_LIBRARIES
    ${QT_QTCORE_LIBRARY}
    ${SHIBOKEN_PYTHON_LIBRARIES}
    ${SHIBOKEN_LIBRARY}
    ${PYSIDE_LIBRARY}
    ${CMAKE_BINARY_DIR}/libpulseview.so
)
 
include_directories(pv ${pv_INCLUDE_DIRECTORIES})
add_library(pv MODULE ${pv_SRC})
set_property(TARGET pv PROPERTY PREFIX "")
target_link_libraries(pv ${pv_LINK_LIBRARIES})
 
add_custom_command(OUTPUT ${pv_SRC}
                   COMMAND ${SHIBOKEN}
                   --generator-set=shiboken --enable-parent-ctor-heuristic --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
                   ${CMAKE_CURRENT_SOURCE_DIR}/global.h
                   --include-paths=${QT_INCLUDE_DIR}:${CMAKE_SOURCE_DIR}
                   --typesystem-paths=${typesystem_path}:${PYSIDE_TYPESYSTEMS}
                   --output-directory=${CMAKE_BINARY_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_pv.xml
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMENT "Running generator for pv..."
                  )

