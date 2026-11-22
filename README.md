# VocalEnhancePro

Wtyczka VST3 do czyszczenia i poprawiania wokali (FL Studio i inne DAW-y).
Nowoczesny interfejs w kolorach granat / czerń / neonowy fiolet.

## Pokrętła
| Pokrętło | Co robi |
|---|---|
| CLEAN | Redukcja szumu w przerwach (expander/gate) + delikatne wycięcie "mułu" ~280 Hz |
| LOW CUT | Filtr górnoprzepustowy 24 dB/okt (20-300 Hz) - usuwa dudnienie, pop, wibracje |
| DE-ESS | De-esser - łagodzi syczące "s" i "sz" (pasmo od ~6 kHz) |
| COMPRESS | Kompresor wokalu z automatycznym wzmocnieniem |
| WARMTH | Półka basowa 200 Hz (-6..+6 dB) - ciepło |
| PRESENCE | Wzmocnienie obecności ~3.8 kHz (-6..+6 dB) - wokal "z przodu" |
| AIR | Półka wysokich 11 kHz (0..+8 dB) - przestrzeń i połysk |
| MIX | Dry/Wet |
| OUTPUT | Głośność wyjścia (-12..+12 dB) |

## Instalacja (Windows)
1. Wejdź w zakładkę **Actions** -> ostatni zielony build -> pobierz **VocalEnhancePro-Windows-VST3**.
   (albo w **Releases**, jeśli utworzysz tag np. `v1.0.0`)
2. Rozpakuj i skopiuj folder `VocalEnhancePro.vst3` do:
   `C:\Program Files\Common Files\VST3\`
3. FL Studio: *Options -> Manage plugins -> Find installed plugins* (Start scan).
4. Wtyczka pojawi się w Mixerze jako efekt: VocalEnhancePro.

## Ręczne budowanie
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Wynik: `build/VocalEnhancePro_artefacts/Release/VST3/VocalEnhancePro.vst3`

## Wskazówki
Zacznij od ustawień domyślnych. Dla szumiącego nagrania podnieś CLEAN, dla ostrych "s" - DE-ESS.
