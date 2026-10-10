cmake_minimum_required(VERSION 3.16)
project(qthunt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_standard_project_setup()

qt_add_executable(qthunt
    src/main.cpp
    src/Types.h
    src/Types.cpp
    src/UhuntClient.h
    src/UhuntClient.cpp
    src/ProblemTableModel.h
    src/ProblemTableModel.cpp
    src/ProblemFilterProxyModel.h
    src/ProblemFilterProxyModel.cpp
    src/SubmissionTableModel.h
    src/SubmissionTableModel.cpp
    src/RanklistTableModel.h
    src/RanklistTableModel.cpp
    src/LiveEventTableModel.h
    src/LiveEventTableModel.cpp
    src/ProblemDetailsDialog.h
    src/ProblemDetailsDialog.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(qthunt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS qthunt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
