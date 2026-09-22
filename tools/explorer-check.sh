#!/usr/bin/env bash
# Vérifie l'explorer flecs du sandbox (M3.1, #62), par l'API REST dont se sert l'explorer
# (https://www.flecs.dev/flecs/FlecsRemoteApi.html) :
#   1. en Debug, le serveur n'écoute que sur la boucle locale, jamais sur toutes les interfaces ;
#   2. un cube se lit, avec ses composants ;
#   3. une vitesse posée par l'API le fait bouger : les entités sont modifiables et le rendu suit le monde ;
#   4. déplacer leur parent « grid » déplace leur matrice monde (M3.2, #63) ;
#   5. en Release, aucun serveur n'écoute.
#
# Usage, depuis la racine du dépôt, après un build linux-debug et linux-release :
#   ./tools/explorer-check.sh
#   DEBUG_SANDBOX=./build/linux-asan/sandbox/levain_sandbox ./tools/explorer-check.sh
set -euo pipefail

debug=${DEBUG_SANDBOX:-./build/linux-debug/sandbox/levain_sandbox}
release=./build/linux-release/sandbox/levain_sandbox
api=http://127.0.0.1:27750
cube=grid/cube_50_50 # un enfant de « grid » : l'API le nomme par son chemin
log=$(mktemp)
pid=
# Un échec en cours de route ne laisse pas de sandbox tourner derrière lui.
trap '[ -n "$pid" ] && kill "$pid" 2>/dev/null; rm -f "$log"' EXIT

fail() { echo "ÉCHEC : $1"; exit 1; }
position_y() { curl -sf "$api/entity/$cube?values=true" | grep -o '"position":{[^}]*}' | grep -o '"y":[-0-9.e]*' | cut -d: -f2; }
# La 14e valeur de la matrice monde (colonnes) : la translation en y.
world_y() { curl -sf "$api/entity/$cube?values=true" | grep -o '"matrix":\[[^]]*\]' | cut -d'[' -f2 | cut -d, -f14; }
put_component() { curl -sf -X PUT "$api/component/$1?component=$2&value=$(quote "$3")" >/dev/null; }
quote() { python3 -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1]))' "$1"; }

echo "--- Debug"
SDL_VIDEO_DRIVER=offscreen "$debug" --seconds 10 >"$log" 2>&1 &
pid=$!
sleep 3

listening=$(ss -ltn | grep ':27750 ' || true)
echo "écoute : $listening"
grep -q '127.0.0.1:27750' <<<"$listening" || fail "le serveur REST n'écoute pas sur 127.0.0.1:27750"
grep -Eq '(0\.0\.0\.0|\*|\[::\]):27750' <<<"$listening" && fail "le serveur REST écoute sur toutes les interfaces"

before=$(position_y) || fail "$cube illisible par l'API REST"
echo "$cube, position.y avant : $before"
put_component "$cube" levain.scene.SceneModule.Velocity '{"linear":{"x":0,"y":5,"z":0}}' \
    || fail "vitesse refusée par l'API REST"
sleep 1
after=$(position_y)
echo "$cube, position.y une seconde après une vitesse de 5 : $after"
python3 -c "import sys; sys.exit(0 if float('$after') - float('$before') > 3 else 1)" \
    || fail "le cube n'a pas bougé"

# M3.2 : les cubes sont enfants de « grid ». Déplacer le parent déplace leur matrice monde, sans
# toucher à leur Transform, qui est dans le repère de la grille. Vitesse remise à zéro d'abord,
# pour que le seul mouvement mesuré soit celui du parent.
put_component "$cube" levain.scene.SceneModule.Velocity '{"linear":{"x":0,"y":0,"z":0}}' \
    || fail "vitesse refusée par l'API REST"
sleep 1
world_before=$(world_y)
local_before=$(position_y)
put_component grid levain.scene.SceneModule.Transform '{"position":{"x":0,"y":10,"z":0}}' \
    || fail "position de la grille refusée par l'API REST"
sleep 1
echo "$cube, matrice monde y : $world_before avant, $(world_y) après avoir levé la grille de 10"
python3 -c "import sys; sys.exit(0 if float('$(world_y)') - float('$world_before') > 9 else 1)" \
    || fail "le cube n'a pas suivi son parent"
python3 -c "import sys; sys.exit(0 if abs(float('$(position_y)') - float('$local_before')) < 0.5 else 1)" \
    || fail "le Transform du cube a bougé : il n'est pas dans le repère de son parent"

status=0
wait "$pid" || status=$?
pid=
[ "$status" -eq 0 ] || { cat "$log"; fail "le sandbox Debug s'est terminé avec le code $status"; }

echo "--- Release"
SDL_VIDEO_DRIVER=offscreen "$release" --seconds 3 >"$log" 2>&1 &
pid=$!
sleep 2
ss -ltn | grep -q ':27750 ' && fail "un serveur REST écoute en Release"
echo "aucun serveur sur le port 27750"
wait "$pid" || fail "le sandbox Release s'est terminé en erreur"
pid=
echo "OK"
