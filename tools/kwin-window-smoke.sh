#!/usr/bin/env bash
# Redimensionne, minimise et restaure la fenêtre du sandbox à travers KWin, comme le ferait un
# utilisateur : c'est le compositeur qui agit, pas le programme. Plasma uniquement (KWin 6).
#
# Usage, sandbox déjà lancé :
#   ./build/linux-debug/sandbox/levain_sandbox &
#   ./tools/kwin-window-smoke.sh [secondes de redimensionnements en boucle, 0 par défaut]
#
# Avec une durée (300 pour les 5 minutes de M1.2), la fenêtre change de taille sans arrêt avant la
# séquence habituelle. En Debug, une erreur de validation arrête le sandbox sur une assertion : le
# script vérifie à la fin qu'il tourne encore.
#
# Le résultat se lit dans les logs du sandbox (« redimensionnée », « masquée », « visible »).
# Le script mesure aussi le temps CPU du sandbox, visible puis minimisé : une boucle qui ne
# s'endort pas quand la fenêtre est masquée consommerait un cœur entier pour rien.
#
# Sous Wayland, la fenêtre n'existe pour KWin qu'après sa première image présentée (M1.2).
set -euo pipefail

readonly stress_seconds="${1:-0}"
readonly caption="Levain"
script=$(mktemp --suffix=.js)
trap 'rm -f "$script"' EXIT

# Applique un bout de JavaScript KWin à chaque fenêtre dont le titre commence par $caption.
on_window() {
    cat > "$script" <<EOF
for (const w of workspace.windowList()) {
    if (w.caption.startsWith("$caption")) { $1 }
}
EOF
    qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript "$script" levain-smoke >/dev/null
    qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.start >/dev/null
    # start() exécute le script plus tard, dans la boucle de KWin : le décharger tout de suite
    # l'annulerait sans rien dire.
    sleep 0.5
    qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript levain-smoke >/dev/null
}

# Temps CPU consommé par le sandbox depuis son lancement, en millisecondes (champs utime et
# stime de /proc/<pid>/stat, comptés en tops d'horloge).
cpu_ms() {
    awk -v tick="$(getconf CLK_TCK)" '{ print int(($14 + $15) * 1000 / tick) }' \
        "/proc/$(pgrep -x levain_sandbox)/stat"
}

# Temps CPU pendant 2 s d'attente.
measure_cpu() {
    local before
    before=$(cpu_ms)
    sleep 2
    echo "CPU pendant 2 s $1 : $(($(cpu_ms) - before)) ms"
}

# Redimensionnements en boucle : chaque changement de taille reconstruit la swapchain.
readonly sizes=("640 360" "1600 900" "1024 768" "800 600" "1920 1080" "1280 720")
readonly stress_end=$((SECONDS + stress_seconds))
resizes=0
while ((SECONDS < stress_end)); do
    read -r width height <<< "${sizes[resizes % ${#sizes[@]}]}"
    on_window "w.frameGeometry = {x: w.x, y: w.y, width: $width, height: $height};"
    resizes=$((resizes + 1))
done
((stress_seconds > 0)) && echo "Redimensionnements : $resizes en ${stress_seconds} s"

on_window 'w.frameGeometry = {x: w.x, y: w.y, width: 640, height: 360};'
sleep 1
on_window 'w.frameGeometry = {x: w.x, y: w.y, width: 1600, height: 900};'
measure_cpu "visible"
on_window 'w.minimized = true;'
measure_cpu "minimisée"
on_window 'w.minimized = false;'
sleep 1

# Une erreur de validation aurait arrêté le sandbox sur une assertion.
pgrep -x levain_sandbox > /dev/null || { echo "ÉCHEC : le sandbox s'est arrêté pendant le test"; exit 1; }
echo "Sandbox toujours en vie"
