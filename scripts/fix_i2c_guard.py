"""
fix_i2c_guard.py — PlatformIO extra script (pre:build)

Corregge il conflitto di include guard tra M5Core-Ink e M5Unit-ENV.
Entrambe le librerie usano il guard _I2C_DEVICE_BUS_ in header diversi,
causando la mancata dichiarazione di I2C_Class.

Vedi docs/build_notes_v1.md §4 per i dettagli.
"""
Import("env")
import os

def fix_guard(source=None, target=None, env=env):
    libdeps = os.path.join(env["PROJECT_LIBDEPS_DIR"], env["PIOENV"])
    header = os.path.join(libdeps, "M5Unit-ENV", "src", "I2C_Class.h")

    if not os.path.isfile(header):
        return

    with open(header, "r", encoding="utf-8") as f:
        content = f.read()

    old_guard = "_I2C_DEVICE_BUS_"
    new_guard = "_M5UNITENV_I2C_CLASS_H_"

    if old_guard in content and new_guard not in content:
        content = content.replace(old_guard, new_guard)
        with open(header, "w", encoding="utf-8") as f:
            f.write(content)
        print("[fix_i2c_guard] Patched I2C_Class.h include guard")

# Eseguire immediatamente alla valutazione dello script
fix_guard()
