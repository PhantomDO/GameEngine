add_executable(levain_sandbox src/main.cpp)
target_link_libraries(levain_sandbox PRIVATE
    levain::assets levain::core levain::platform levain::gpu levain::render)

# Les textures sont lues dans le dépôt, comme les shaders compilés dans le build : pas de copie à
# tenir à jour.
target_compile_definitions(levain_sandbox PRIVATE LEVAIN_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
