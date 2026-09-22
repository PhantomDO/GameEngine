add_executable(levain_sandbox src/main.cpp src/shader_reload.cpp)
target_link_libraries(levain_sandbox PRIVATE
    levain::assets levain::core levain::platform levain::gpu levain::render)

# Les textures sont lues dans le dépôt, comme les shaders compilés dans le build : pas de copie à
# tenir à jour.
target_compile_definitions(levain_sandbox PRIVATE LEVAIN_DATA_DIR="${PROJECT_SOURCE_DIR}/data")

# Le hot-reload des shaders relance le build qui a produit ce binaire (ADR-0014) : le CMake qui l'a
# configuré, son dossier de build, et les sources à surveiller.
target_compile_definitions(levain_sandbox PRIVATE
    LEVAIN_CMAKE_COMMAND="${CMAKE_COMMAND}"
    LEVAIN_BUILD_DIR="${CMAKE_BINARY_DIR}"
    LEVAIN_SHADER_SOURCE_DIR="${PROJECT_SOURCE_DIR}/shaders")
