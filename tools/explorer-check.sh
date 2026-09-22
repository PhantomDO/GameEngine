#!/usr/bin/env bash
# Vérifie l'explorer flecs du sandbox (M3.1, #62), par l'API REST dont se sert l'explorer
# (https://www.flecs.dev/flecs/FlecsRemoteApi.html) :
#   1. en Debug, le serveur n'écoute que sur la boucle locale, jamais sur toutes les interfaces ;
#   2. un cube se lit, avec ses composants ;
#   3. une vitesse posée par l'API le fait bouger : les entités sont modifiables et le rendu suit le monde ;
#   4. en Release, aucun serveur n'écoute.
#
# Usage, depuis la racine du dépôt, après un build linux-debug et linux-release :
#   ./tools/explorer-check.sh
#   DEBUG_SANDBOX=./build/linux-asan/sandbox/levain_sandbox ./tools/explorer-check.sh
set -euo pipefail

debug=${DEBUG_SANDBOX:-./build/linux-debug/sandbox/levain_sandbox}
release=./build/linux-release/sandbox/levain_sandbox
api=http://127.0.0.1:27750
cube=cube_50_50
log=$(mktemp)
pid=
# Un échec en cours de route ne laisse pas de sandbox tourner derrière lui.
trap '[ -n "$pid" ] && kill "$pid" 2>/dev/null; rm -f "$log"' EXIT

fail() { echo "ÉCHEC : $1"; exit 1; }
position_y() { curl -sf "$api/entity/$cube?values=true" | grep -o '"position":{[^}]*}' | grep -o '"y":[-0-9.e]*' | cut -d: -f2; }

echo "--- Debug"
SDL_VIDEO_DRIVER=offscreen "$debug" --seconds 6 >"$log" 2>&1 &
pid=$!
sleep 3

listening=$(ss -ltn | grep ':27750 ' || true)
echo "écoute : $listening"
grep -q '127.0.0.1:27750' <<<"$listening" || fail "le serveur REST n'écoute pas sur 127.0.0.1:27750"
grep -Eq '(0\.0\.0\.0|\*|\[::\]):27750' <<<"$listening" && fail "le serveur REST écoute sur toutes les interfaces"

before=$(position_y) || fail "$cube illisible par l'API REST"
echo "$cube, position.y avant : $before"
velocity=$(python3 -c 'import urllib.parse; print(urllib.parse.quote("{\"linear\":{\"x\":0,\"y\":5,\"z\":0}}"))')
curl -sf -X PUT "$api/component/$cube?component=levain.scene.SceneModule.Velocity&value=$velocity" >/dev/null \
    || fail "vitesse refusée par l'API REST"
sleep 1
after=$(position_y)
echo "$cube, position.y une seconde après une vitesse de 5 : $after"
python3 -c "import sys; sys.exit(0 if float('$after') - float('$before') > 3 else 1)" \
    || fail "le cube n'a pas bougé"

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
