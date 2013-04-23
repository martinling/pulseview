find_package(PythonLibs 2.7 REQUIRED)
find_package(Shiboken REQUIRED)
find_package(PySide REQUIRED)
find_package(Qt4 4.6.2 REQUIRED)

find_program(SHIBOKEN shiboken REQUIRED)

add_subdirectory(pulseview)
