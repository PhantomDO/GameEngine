# Le cuiseur (ADR-0020) : sans GPU, il ne lie que assets et core. Construit aussi quand un jeu inclut
# le moteur : le jeu a des assets à cuire.
add_executable(levain_cook main.cpp)
target_link_libraries(levain_cook PRIVATE levain::assets levain::core)
