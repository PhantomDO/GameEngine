//! Socle du moteur : types de base, logs, temps, fichiers.
//!
//! État en M0.4 : quasiment vide, comme son prédécesseur C++ l'était en M0.2. Ne contient
//! que la bannière de démarrage, dont le seul rôle est de prouver que la chaîne tient
//! debout de bout en bout. Le vrai contenu arrive en M0.3 : logs, assertions, allocateurs.

/// Version du moteur, prise dans le `Cargo.toml` du crate.
pub fn version() -> &'static str {
    env!("CARGO_PKG_VERSION")
}

/// Système et architecture de la cible, par exemple `linux/x86_64`.
///
/// Remplace la bannière C++ qui affichait le compilateur et `__cplusplus`. Ce diagnostic
/// existait parce que MSVC et libstdc++ ne fournissaient pas le même C++23 (ADR-0001) ;
/// cargo ne laisse pas ce genre d'écart s'installer, donc la cible suffit.
pub fn target() -> String {
    format!("{}/{}", std::env::consts::OS, std::env::consts::ARCH)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_is_a_three_part_semver() {
        let parts: Vec<&str> = version().split('.').collect();

        assert_eq!(parts.len(), 3, "version attendue sous la forme x.y.z");
        assert!(
            parts.iter().all(|part| part.parse::<u32>().is_ok()),
            "chaque composant doit être un entier, obtenu : {}",
            version()
        );
    }

    #[test]
    fn target_names_a_system_and_an_architecture() {
        let target = target();
        let (os, arch) = target.split_once('/').expect("format attendu : os/arch");

        assert!(!os.is_empty());
        assert!(!arch.is_empty());
    }
}
