// ============================================================================
// M5Unified compatibility shim for CoreInk
// ============================================================================
// Provides M5CoreInk API compatibility on top of M5Unified.
// This allows main.cpp to compile unchanged with either library.
// Used when USE_M5UNIFIED=1 is defined in build_flags.
// ============================================================================
#pragma once

#include <M5Unified.h>

// --- M5.M5Ink.clear() → M5.Display.clear() ---
// The preprocessor sees M5.M5Ink as tokens: M5 . M5Ink
// Defining M5Ink as Display makes M5.M5Ink.clear() → M5.Display.clear()
#define M5Ink Display

// --- Ink_Sprite compatibility ---
// M5CoreInk's Ink_Sprite: white background, black text (e-ink paper style)
// M5GFX M5Canvas: default black background, white text
// We override clear() and creatSprite() to match e-ink behavior.

class Ink_Sprite : public m5gfx::M5Canvas {
public:
    Ink_Sprite(lgfx::LovyanGFX* parent = nullptr)
        : m5gfx::M5Canvas(parent ? parent : &M5.Display) {}

    // M5CoreInk "creatSprite" (typo in original API) — also configure e-ink colors
    void creatSprite(int32_t x, int32_t y, int32_t w, int32_t h) {
        (void)x; (void)y;
        createSprite(w, h);
        setColorDepth(1);          // 1-bit per e-ink
        setTextColor(TFT_BLACK, TFT_WHITE);  // Testo nero su sfondo bianco
        fillSprite(TFT_WHITE);     // Sfondo bianco (come carta e-ink)
    }

    // clear() = riempi di bianco (come M5CoreInk originale)
    void clear() {
        fillSprite(TFT_WHITE);
    }

    // M5CoreInk: drawString(x, y, str, font)
    // M5GFX:     drawString(str, x, y) with setFont()
    using m5gfx::M5Canvas::drawString;
    void drawString(int32_t x, int32_t y, const char* str, const lgfx::IFont* font) {
        setFont(font);
        setTextColor(TFT_BLACK, TFT_WHITE);  // Garantisci nero su bianco
        m5gfx::M5Canvas::drawString(str, x, y);
    }

    // M5CoreInk: pushSprite() → push to e-ink at (0,0)
    void pushSprite() {
        m5gfx::M5Canvas::pushSprite(&M5.Display, 0, 0);
    }
};

// --- E-ink refresh modes ---
// epd_quality: full refresh con inversione completa (~0.8s)
// Usato come modo permanente per evitare ghosting.

inline void m5unified_setup_eink() {
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
}

// Pulizia profonda display: cicli nero(1s) → bianco(1s) ripetuti N volte.
// Forza la scarica completa della carica residua accumulata nei pixel.
// Da usare on-demand (menu emergenza) quando il display ha ghosting persistente.
inline void eink_deep_clean(int cycles = 10) {
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    for (int i = 0; i < cycles; i++) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.display();
        M5.Display.waitDisplay();
        delay(1000);
        M5.Display.fillScreen(TFT_WHITE);
        M5.Display.display();
        M5.Display.waitDisplay();
        delay(1000);
    }
}

inline void eink_clear_ghosting() {
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
    M5.Display.clearDisplay();
    M5.Display.waitDisplay();
}

// --- Button name mapping ---
// M5CoreInk: BtnUP(GPIO37), BtnDOWN(GPIO39), BtnMID(GPIO38), BtnEXT(GPIO5), BtnPWR(GPIO27)
// M5Unified CoreInk (verificato sul campo v1.3.0):
//   BtnA = GPIO37 (top/up)    → was BtnUP   ✅ confermato
//   BtnB = GPIO38 (center)    → was BtnMID  (NON down!)
//   BtnC = GPIO39 (bottom/dn) → was BtnDOWN (NON mid!)
//   BtnPWR = GPIO27           → same
//   BtnEXT = GPIO5 (hat)      → same
#ifndef BtnUP
#define BtnUP BtnA
#endif
#ifndef BtnDOWN
#define BtnDOWN BtnC
#endif
#ifndef BtnMID
#define BtnMID BtnB
#endif
// BtnEXT and BtnPWR — M5Unified should provide these natively for CoreInk
// If compilation fails on BtnEXT, we'll need to map it or create a dummy button
