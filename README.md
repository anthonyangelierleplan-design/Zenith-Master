# Zenith - Foudre Audio

Traitement master : OTT multibande (3 bandes, compression vers le bas et vers le haut) -> EQ -> compresseur (4 modeles) -> limiteur a diodes -> clipper doux -> limiteur a anticipation.
Pas de dry/wet : le bouton INTENSITY regle la force du traitement (pas de probleme de phase).

- Demarrage neutre (Intensity = 0, rien n'est ajoute au son).
- Macros : DYNAMICS (squeeze/punch), BRILLIANCE (dark/bright), SNAPPY/SQUASHY. Les points orange indiquent les reglages qu'elles deplacent.
- Compresseur a 4 modeles (emulations comportementales tirees des schemas d'origine, suréchantillonnage x2) :
  - CLEAN : compresseur numerique transparent.
  - VARI-MU (type Fairchild 660) : 4 tubes 6386 en push-pull, detection sur la sortie, TIME CONSTANT 1-6 (5 et 6 dependent du programme), BALANCE (desequilibre = harmoniques paires + thump).
  - FET (type UREI 1176LN) : attenuateur FET, preampli et ampli de ligne classe A, transfo ; ratios 4/8/12/20 et ALL (all buttons in).
  - DIODE (type Neve 33609) : pont de diodes, ampli classe A, transfos ; ratios 1.5 a 6, recuperation 100 ms a 1.5 s et AUTO 1/2.
- DIODE LIMITER (section limiteur type 33609, +4 a +15 dBu, recuperation 50 ms a AUTO 2) avant le limiteur de securite. Reference : 0 dBFS = +18 dBu.
- Les macros montrent sur chaque potard un arc orange jusqu'a la valeur reellement appliquee.
- Modes Mid/Side pour l'OTT et le compresseur.
- Compteurs LUFS (momentary, short-term, integrated) et comparaison BYPASS a volume egal (LEVEL MATCH).
- Fenetre redimensionnable. Presets utilisateur : `%APPDATA%\Foudre Audio\Zenith\Presets`.

Compilation : pousser sur GitHub, onglet Actions -> artefacts `Zenith-VST3-Windows` et `Zenith-Standalone-Windows`.
