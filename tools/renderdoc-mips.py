# Vérifie dans une capture RenderDoc les niveaux de mip de la texture « checker » (critère de M2.2, #43).
#
# Lance le sandbox sous RenderDoc, capture une frame, puis extrait de la capture chaque niveau de mip de la
# texture, tel que le GPU l'a reçu. À lancer depuis la racine du dépôt, après un build linux-debug :
#
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/renderdoc-mips.py
#
# qrenderdoc fournit le module Python `renderdoc`, que le paquet Arch n'installe pas pour le Python du système.
# QT_QPA_PLATFORM=offscreen : aucune fenêtre de RenderDoc ne s'ouvre. Le script quitte avant l'interface.
#
# Sorties dans captures/ (ignoré par git) : m2.2_frame<N>.rdc, m2.2-mip<N>.png pour chaque niveau, et
# renderdoc-mips.log. Le code de sortie dit si la vérification a réussi.

import os
import sys
import time
import traceback

import renderdoc as rd

ROOT = os.getcwd()
SANDBOX = os.path.join(ROOT, "build", "linux-debug", "sandbox", "levain_sandbox")
OUTPUT = os.path.join(ROOT, "captures")
TEXTURE_NAME = "checker"

# qrenderdoc garde les print dans sa propre console Python : la sortie va dans un fichier.
os.makedirs(OUTPUT, exist_ok=True)
sys.stdout = sys.stderr = open(os.path.join(OUTPUT, "renderdoc-mips.log"), "w", buffering=1)


def fail(message):
    print(f"ÉCHEC : {message}", flush=True)
    os._exit(1)  # sys.exit rendrait la main à qrenderdoc, qui ouvrirait son interface


def capture_frame():
    """Lance le sandbox sous RenderDoc et rend le chemin d'une capture d'une frame."""
    if not os.path.exists(SANDBOX):
        fail(f"{SANDBOX} absent : compiler d'abord avec cmake --build --preset linux-debug")

    # X11 (XWayland) : RenderDoc 1.45 masque VK_KHR_wayland_surface, et SDL ne pourrait pas créer la
    # fenêtre sous Wayland.
    x11 = rd.EnvironmentModification(rd.EnvMod.Set, rd.EnvSep.NoSep, "SDL_VIDEO_DRIVER", "x11")
    launched = rd.ExecuteAndInject(SANDBOX, ROOT, "--seconds 6", [x11], os.path.join(OUTPUT, "m2.2"),
                                   rd.GetDefaultCaptureOptions(), False)
    if launched.result.code != rd.ResultCode.Succeeded:
        fail(f"lancement sous RenderDoc : {launched.result.Message()}")

    target = rd.CreateTargetControl("", launched.ident, "levain-mips", True)
    if target is None:
        fail("connexion au sandbox impossible")

    # La capture se déclenche une fois la boucle lancée : la texture est envoyée avant la première frame.
    time.sleep(2)
    target.TriggerCapture(1)
    deadline = time.time() + 30
    while time.time() < deadline:
        message = target.ReceiveMessage(None)
        if message.type == rd.TargetControlMessageType.NewCapture:
            target.Shutdown()
            return message.newCapture.path
        if message.type == rd.TargetControlMessageType.Disconnected:
            fail("le sandbox s'est arrêté avant la capture")
    fail("aucune capture reçue en 30 s")


def save_mips(capture_path):
    """Ouvre la capture et enregistre chaque niveau de mip de la texture en PNG. Rend leurs tailles."""
    capture = rd.OpenCaptureFile()
    if capture.OpenFile(capture_path, "", None).code != rd.ResultCode.Succeeded:
        fail(f"{capture_path} illisible")
    result, controller = capture.OpenCapture(rd.ReplayOptions(), None)
    if result.code != rd.ResultCode.Succeeded:
        fail(f"rejeu impossible : {result.Message()}")

    # Le nom vient du debugName de NVRHI, transmis au pilote par VK_EXT_debug_utils.
    names = {resource.resourceId: resource.name for resource in controller.GetResources()}
    textures = [texture for texture in controller.GetTextures()
                if names.get(texture.resourceId) == TEXTURE_NAME]
    if len(textures) != 1:
        fail(f"{len(textures)} texture(s) nommée(s) « {TEXTURE_NAME} » dans la capture, 1 attendue")
    texture = textures[0]

    # Le contenu tel qu'il est à la fin de la frame capturée.
    controller.SetFrameEvent(controller.GetRootActions()[-1].eventId, True)

    sizes = []
    for mip in range(texture.mips):
        save = rd.TextureSave()
        save.resourceId = texture.resourceId
        save.destType = rd.FileType.PNG
        save.mip = mip
        path = os.path.join(OUTPUT, f"m2.2-mip{mip}.png")
        if not controller.SaveTexture(save, path):
            fail(f"niveau {mip} non enregistré")
        sizes.append((max(texture.width >> mip, 1), max(texture.height >> mip, 1)))

    controller.Shutdown()
    capture.Shutdown()
    return sizes


try:
    capture_path = capture_frame()
    print(f"capture : {capture_path}", flush=True)
    sizes = save_mips(capture_path)
    for mip, (width, height) in enumerate(sizes):
        print(f"niveau {mip} : {width} × {height} → captures/m2.2-mip{mip}.png", flush=True)
except Exception:
    # Sans ce filet, qrenderdoc attraperait l'exception et ouvrirait son interface, invisible ici.
    fail(traceback.format_exc())
os._exit(0)
