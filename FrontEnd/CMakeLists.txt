cmake_minimum_required(VERSION 3.15)
project(ReceiptAnalyzerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include src)
file(GLOB SOURCES "src/*.cpp")

find_package(wxWidgets 3.3.3 REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

find_package(nlohmann_json REQUIRED)

add_executable(ReceiptAnalyzer ${SOURCES})

target_link_libraries(ReceiptAnalyzer PRIVATE 
	${wxWidgets_LIBRARIES}
	nlohmann_json::nlohmann_json
)

add_custom_command(TARGET ReceiptAnalyzer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_SOURCE_DIR}/assets"
	"${CMAKE_CURRENT_BINARY_DIR}/assets"
	COMMENT "Syncing Asset Files to Build Directory.."
)
