---
name: cloture
description: Clôturer un milestone ou une phase de Levain — vérification de la définition de « terminé », tag, release avec mesures, fermeture du milestone, et pour une phase, étude, ratio passé/estimé, recalibrage et détail de la phase N+2. À utiliser quand Donnovan demande de clôturer.
---

# Clôture d'un milestone ou d'une phase

Lire d'abord [`GOTCHA.md`](GOTCHA.md).

## Milestone

1. Vérifier la définition de « terminé » (SPECS §9) et que toutes les issues du milestone sont fermées ou
   explicitement reportées (avec l'accord de Donnovan).
2. Tag, release, fermeture :

```bash
git tag m1.3 && git push origin m1.3
gh release create m1.3 --title "M1.3 — Premier triangle" --notes-file <notes avec mesures>
gh api repos/PhantomDO/Levain/milestones --jq '.[] | "\(.number) \(.title)"'   # numéro du milestone
gh api -X PATCH repos/PhantomDO/Levain/milestones/<numéro> -f state=closed
```

3. Entrée de clôture dans `docs/JOURNAL.md`, board à jour.

## Phase

En plus du dernier milestone :

1. Écrire l'étude prévue dans `docs/etudes/`.
2. Calculer le ratio passé/estimé de la phase, sur le **temps total** de Donnovan, et le reporter dans le
   tableau « Cumul » du journal.
3. Recalibrer si le ratio sort de la fourchette 0,8–1,25 (ROADMAP, section « Recalibrage »).
4. Détailler en issues la phase N+2 : milestone, labels existants, estimation, champ Phase du board.
