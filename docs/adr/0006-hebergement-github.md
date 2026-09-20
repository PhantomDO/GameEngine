# ADR-0006 — Hébergement et suivi : GitHub, dépôt public

- **Statut** : proposé
- **Date** : 2026-09-20
- **Milestone** : M0.1

## Contexte

Il faut héberger le code, faire tourner une CI Windows + Linux, et suivre l'avancement (étapes, temps estimé et
passé, notifications). Budget : 0 €. Candidats : GitHub, GitLab, Azure DevOps.

## Options envisagées

| Critère | **GitHub** | GitLab.com | Azure DevOps |
|---|---|---|---|
| CI gratuite | Runners standard **gratuits et illimités pour les dépôts publics** | 400 minutes de calcul par mois en Free (la moitié du coût pour les projets admis au programme open-source) | Jobs parallèles gratuits sur demande par formulaire, avec des délais de réponse signalés |
| Suivi | Issues, milestones, Projects (tableaux, roadmap, champs personnalisés) | Issues, milestones, boards, **suivi du temps natif** (`/estimate`, `/spend`) | Boards très complets, champs d'heures natifs |
| Pilotage par Claude Code | CLI `gh` très complète | CLI `glab` | CLI `az` plus lourde |
| Visibilité | Portfolio déjà sur GitHub Pages | Compte existant (projet souls-like) | Faible dans l'open-source |

Un build C++ avec dépendances sous Windows et Linux peut prendre 10 à 20 minutes à froid : 400 minutes par mois
ne représentent que quelques dizaines de pipelines.

## Décision

GitHub, dépôt public. Le suivi du temps, absent nativement, est compensé par deux champs numériques du board
GitHub Projects (« Estimé (h) » et « Passé (h) ») et par `docs/JOURNAL.md`.

## Conséquences

- Le code est public dès le premier jour : c'est aussi une vitrine pour le profil tools/engine de Donnovan.
- Les GitHub Releases servent de jalons visibles : une par milestone, avec binaires et mesures.
- Notifications : suivre le dépôt (« Watch » → « All activity ») ou l'application mobile GitHub.

## Sources

- GitHub, *Pricing changes for GitHub Actions* (2026) : l'usage des runners standard sur les dépôts publics
  reste gratuit.
- Documentation GitLab, *Compute minutes* : 400 minutes par mois en Free.
- Microsoft Learn, *Configure and pay for parallel jobs* (Azure Pipelines).
