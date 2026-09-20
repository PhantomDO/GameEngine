# Levain

Moteur de jeu 3D en **Rust** pour Windows et Linux, sur **wgpu** (Vulkan, Direct3D 12, Metal) et **bevy_ecs**,
construit étape par étape pour faire un jeu et comprendre comment fonctionnent les moteurs du marché
(Unreal, Unity, Godot…).

Un levain, c'est ce qu'on nourrit un peu chaque semaine, qui reste vivant entre deux fournées, et à partir de
quoi on cuit autre chose. C'est le rythme et le rôle de ce moteur.

**Statut** : phase 0. Le moteur a démarré en C++23 (tag `m0.2`) et est passé à Rust à la clôture de M0.2,
pour les raisons mesurées dans l'[ADR-0010](docs/adr/0010-passage-a-rust.md).

- [Spécifications](docs/SPECS.md)
- [Roadmap chiffrée](docs/ROADMAP.md)
- [Journal de bord](docs/JOURNAL.md)
- [Décisions d'architecture (ADR)](docs/adr/)
- [Études comparatives](docs/etudes/)
- [Lectures commentées](docs/LECTURES.md)
- [Questions et réponses](docs/QA.md)
- [Mise en route](docs/SETUP.md)

## Compiler

Un seul prérequis : [rustup](https://rustup.rs).

```bash
cargo run -p levain-sandbox
```

## Licence

[MIT](LICENSE).
