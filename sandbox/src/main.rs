fn main() {
    // Bannière de démarrage : la preuve minimale que le crate moteur est lié et appelable.
    // Le sandbox reste lancé en CI, sur les deux systèmes, pour la même raison qu'en M0.2.
    println!(
        "Levain {} — {} — rust edition 2024",
        levain_core::version(),
        levain_core::target()
    );
}
