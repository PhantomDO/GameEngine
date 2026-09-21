# Fonctions communes aux scripts RenderDoc de tools/ : lancer le sandbox sous RenderDoc, capturer une frame,
# rejouer la capture.
#
# Ces scripts tournent dans le Python de qrenderdoc, le seul qui fournisse le module `renderdoc` (le paquet
# Arch ne l'installe pas pour le Python du système), depuis la racine du dépôt :
#
#   QT_QPA_PLATFORM=offscreen qrenderdoc --python tools/<script>.py
#
# QT_QPA_PLATFORM=offscreen : aucune fenêtre de RenderDoc ne s'ouvre. Pièges : skill `build`, GOTCHA.md.

import os
import sys
import time
import traceback

import renderdoc as rd

ROOT = os.getcwd()
SANDBOX = os.path.join(ROOT, "build", "linux-debug", "sandbox", "levain_sandbox")
OUTPUT = os.path.join(ROOT, "captures")  # ignoré par git


def redirect_output(log_name):
    """qrenderdoc garde les print dans sa propre console Python : la sortie va dans captures/<log_name>."""
    os.makedirs(OUTPUT, exist_ok=True)
    sys.stdout = sys.stderr = open(os.path.join(OUTPUT, log_name), "w", buffering=1)


def fail(message):
    print(f"ÉCHEC : {message}", flush=True)
    os._exit(1)  # sys.exit rendrait la main à qrenderdoc, qui ouvrirait son interface


def run(main):
    """Lance main, puis quitte qrenderdoc avant son interface : code 0 si main a réussi, 1 sinon."""
    try:
        main()
    except Exception:
        # Sans ce filet, qrenderdoc attraperait l'exception et ouvrirait son interface, invisible ici.
        fail(traceback.format_exc())
    os._exit(0)


def capture_frame(arguments, capture_name):
    """Lance le sandbox sous RenderDoc avec `arguments` et rend le chemin d'une capture d'une frame."""
    if not os.path.exists(SANDBOX):
        fail(f"{SANDBOX} absent : compiler d'abord avec cmake --build --preset linux-debug")

    # X11 (XWayland) : RenderDoc 1.45 masque VK_KHR_wayland_surface, et SDL ne pourrait pas créer la
    # fenêtre sous Wayland.
    x11 = rd.EnvironmentModification(rd.EnvMod.Set, rd.EnvSep.NoSep, "SDL_VIDEO_DRIVER", "x11")
    launched = rd.ExecuteAndInject(SANDBOX, ROOT, arguments, [x11], os.path.join(OUTPUT, capture_name),
                                   rd.GetDefaultCaptureOptions(), False)
    if launched.result.code != rd.ResultCode.Succeeded:
        fail(f"lancement sous RenderDoc : {launched.result.Message()}")

    target = rd.CreateTargetControl("", launched.ident, "levain-tools", True)
    if target is None:
        fail("connexion au sandbox impossible")

    # La capture se déclenche une fois la boucle lancée : textures et meshes sont envoyés avant.
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


def open_replay(capture_path):
    """Ouvre une capture pour la rejouer. Rend (capture, controller), à fermer par close_replay."""
    capture = rd.OpenCaptureFile()
    if capture.OpenFile(capture_path, "", None).code != rd.ResultCode.Succeeded:
        fail(f"{capture_path} illisible")
    result, controller = capture.OpenCapture(rd.ReplayOptions(), None)
    if result.code != rd.ResultCode.Succeeded:
        fail(f"rejeu impossible : {result.Message()}")
    return capture, controller


def close_replay(capture, controller):
    controller.Shutdown()
    capture.Shutdown()


def save_png(controller, resource_id, path, mip=0):
    save = rd.TextureSave()
    save.resourceId = resource_id
    save.destType = rd.FileType.PNG
    save.mip = mip
    if not controller.SaveTexture(save, path):
        fail(f"{path} non enregistré")
