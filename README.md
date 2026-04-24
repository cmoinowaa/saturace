# SATURACE by Nowaa
## Plugin de saturation VST3

### Comment compiler sur Mac

**1. Installe Xcode** (App Store) et CMake :
```bash
brew install cmake
```

**2. Clone JUCE :**
```bash
git clone https://github.com/juce-framework/JUCE.git ~/JUCE
```

**3. Compile :**
```bash
cd ~/saturace
mkdir build && cd build
cmake .. -DJUCE_DIR=$HOME/JUCE -G Xcode
open SATURACE.xcodeproj
```
→ Dans Xcode : sélectionne SATURACE_VST3 → Cmd+B

**4. Installe le .vst3 :**
```bash
cp -r ~/saturace/build/SATURACE_artefacts/VST3/SATURACE.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

**5. Scanner dans Ableton :**
Préférences → Plug-ins → Scanner les VST3

---

## Paramètres
- **DRIVE** (0–100%) : quantité de saturation
- **MODE** : Keep High / Neutral / Keep Low
- **OUTPUT** (-24 à +6 dB) : gain de sortie

## Algorithme DSP
- Saturation tanh (Neutral)
- Saturation asymétrique (Keep High / Keep Low)
- DC Blocker intégré (~5 Hz high-pass)
- Compensation de gain automatique
