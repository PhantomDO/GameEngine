# Vérifie dans une capture RenderDoc les niveaux de mip d'une texture : le damier « checker » par défaut
# (critère de M2.2, #43), ou une texture cuite en BC7 (critère de M4.3, #92).
#
# Capture une frame du sandbox, puis en extrait chaque niveau de mip de la texture, tel que le GPU l'a reçu.
# Depuis la racine du dépôt, après un build linux-debug :
#
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/renderdoc-mips.py
#
# Une autre texture se choisit par l'environnement, par exemple une texture de Sponza cuite :
#
#   LEVAIN_MIPS_TEXTURE=14118779221266351425.jpg LEVAIN_MIPS_PREFIX=m4.3 \
#   LEVAIN_MIPS_ARGS="--seconds 6 --model assets-cache/Models/Sponza/glTF/Sponza.gltf" \
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/renderdoc-mips.py
#
# Sorties dans captures/ : <préfixe>_frame<N>.rdc, <préfixe>-mip<N>.png pour chaque niveau, et
# renderdoc-mips.log. Le code de sortie dit si la vérification a réussi.

import os
import sys

sys.path.insert(0, os.path.join(os.getcwd(), "tools"))  # qrenderdoc ne l'ajoute pas lui-même

import renderdoc_capture as capture  # noqa: E402

TEXTURE_NAME = os.environ.get("LEVAIN_MIPS_TEXTURE", "checker")
SANDBOX_ARGUMENTS = os.environ.get("LEVAIN_MIPS_ARGS", "--seconds 6")
PREFIX = os.environ.get("LEVAIN_MIPS_PREFIX", "m2.2")


def main():
    path = capture.capture_frame(SANDBOX_ARGUMENTS, PREFIX)
    print(f"capture : {path}", flush=True)
    replay, controller = capture.open_replay(path)

    # Le nom vient du debugName de NVRHI, transmis au pilote par VK_EXT_debug_utils.
    names = {resource.resourceId: resource.name for resource in controller.GetResources()}
    textures = [texture for texture in controller.GetTextures()
                if names.get(texture.resourceId) == TEXTURE_NAME]
    if len(textures) != 1:
        capture.fail(f"{len(textures)} texture(s) nommée(s) « {TEXTURE_NAME} » dans la capture, 1 attendue")
    texture = textures[0]
    print(f"{TEXTURE_NAME} : {texture.width} × {texture.height}, {texture.mips} niveaux, format "
          f"{texture.format.Name()}", flush=True)

    # Le contenu tel qu'il est à la fin de la frame capturée.
    controller.SetFrameEvent(controller.GetRootActions()[-1].eventId, True)
    for mip in range(texture.mips):
        output = os.path.join(capture.OUTPUT, f"{PREFIX}-mip{mip}.png")
        capture.save_png(controller, texture.resourceId, output, mip)
        width, height = max(texture.width >> mip, 1), max(texture.height >> mip, 1)
        print(f"niveau {mip} : {width} × {height} → captures/{PREFIX}-mip{mip}.png", flush=True)

    capture.close_replay(replay, controller)


capture.redirect_output("renderdoc-mips.log")
capture.run(main)
