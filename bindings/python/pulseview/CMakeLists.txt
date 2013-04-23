project(pulseview)
 
set(pulseview_SRC
    ${CMAKE_CURRENT_BINARY_DIR}/pulseview/pulseview_module_wrapper.cpp
)
 
set(pulseview_INCLUDE_DIRECTORIES
    ${SHIBOKEN_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${PYSIDE_INCLUDE_DIR}
    ${PYSIDE_INCLUDE_DIR}/QtCore
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${PULSEVIEW_DIR}/pv
)
 
set(pulseview_LINK_LIBRARIES
    ${QT_QTCORE_LIBRARY}
    ${SHIBOKEN_PYTHON_LIBRARIES}
    ${SHIBOKEN_LIBRARY}
    ${PYSIDE_LIBRARY}
    ${PULSEVIEW_DIR}/libpv.so
)
 
include_directories(pulseview ${pulseview_INCLUDE_DIRECTORIES})
add_library(pulseview MODULE ${pulseview_SRC})
set_property(TARGET pulseview PROPERTY PREFIX "")
target_link_libraries(pulseview ${pulseview_LINK_LIBRARIES})
 
add_custom_command(OUTPUT ${pulseview_SRC}
                   COMMAND ${GENERATOR}
                   --generator-set=shiboken --enable-parent-ctor-heuristic --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
                   ${CMAKE_SOURCE_DIR}/pulseview/global.h
                   --include-paths=${QT_INCLUDE_DIR}:${PULSEVIEW_DIR}
                   --typesystem-paths=${typesystem_path}:${PYSIDE_TYPESYSTEMS}
                   --output-directory=${CMAKE_CURRENT_BINARY_DIR}
                   ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_pulseview.xml
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   COMMENT "Running generator for pulseview..."
                  )

