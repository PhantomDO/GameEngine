#pragma once

#include "levain/scene/components.hpp"

namespace levain::scene
{

/// La matrice d'un `Transform` dans le repère de son parent : l'échelle d'abord, puis la rotation,
/// puis la translation. Écrite à la main plutôt qu'en trois produits de glm, parce que le système
/// la calcule pour chaque entité à chaque tour.
inline glm::mat4 localMatrix(const Transform& transform)
{
    glm::mat4 matrix = glm::mat4_cast(transform.rotation);
    matrix[0] *= transform.scale.x; // les colonnes 0 à 2 sont les axes tournés
    matrix[1] *= transform.scale.y;
    matrix[2] *= transform.scale.z;
    matrix[3] = glm::vec4(transform.position, 1.0f);
    return matrix;
}

/// La matrice monde d'une entité : celle de son parent, **puis** la sienne. L'ordre du produit est
/// le piège : `local * parent` ferait tourner le parent autour de l'enfant. Pour une racine,
/// `parentWorld` est l'identité.
inline glm::mat4 worldMatrix(const glm::mat4& parentWorld, const Transform& local)
{
    return parentWorld * localMatrix(local);
}

/// Où l'entité se trouve dans le monde : la dernière colonne de sa matrice, celle de la
/// translation. Ce que le rendu envoie au GPU.
inline glm::vec3 worldPosition(const WorldTransform& transform)
{
    return glm::vec3(transform.matrix[3]);
}

} // namespace levain::scene
