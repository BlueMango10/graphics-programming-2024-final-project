
set(libraries glad glfw assimp imgui itugl ${APPLE_LIBRARIES})

file(GLOB_RECURSE target_inc "*.h" )
file(GLOB_RECURSE target_src "*.cpp" )

file(GLOB_RECURSE shaders "*.vert" "*.frag" "*.geom" "*.glsl")
source_group("Shaders" FILES ${shaders})

add_executable(${TARGETNAME} ${target_inc} ${target_src} ${shaders})
target_link_libraries(${TARGETNAME} ${libraries})

# Copy shaders folder to build folder
add_custom_target(${TARGETNAME}-shaders ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/shaders
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)
add_dependencies(${TARGETNAME} ${TARGETNAME}-shaders)

# Copy models folder to build folder
add_custom_target(${TARGETNAME}-models ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/models
	${CMAKE_CURRENT_BINARY_DIR}/models
)
add_dependencies(${TARGETNAME} ${TARGETNAME}-models)
