// Zenith - moteur de traitement master : OTT 3 bandes (LR4) -> EQ -> compresseur -> clipper doux -> limiter (anticipation)
// + compteurs LUFS, mid/side, macros, comparaison a volume egal. Aucune dependance JUCE.
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>
#include "Models.h"

namespace tn
{
//  id        nom affiche        min    max   defaut  type(0 float,1 bool,2 choix)  centre de courbure (0 = lineaire)
#define TN_PARAMS(X) \
    X (in,        "Input",              -12.f,  12.f,   0.f,   0, 0.f)    \
    X (out,       "Output",             -12.f,   6.f,   0.f,   0, 0.f)    \
    X (intensity, "Intensity",            0.f,   1.f,   0.f,   0, 0.f)    \
    X (match,     "Level Match",          0.f,   1.f,   1.f,   1, 0.f)    \
    X (bypass,    "Bypass",               0.f,   1.f,   0.f,   1, 0.f)    \
    X (ottOn,     "OTT On",               0.f,   1.f,   1.f,   1, 0.f)    \
    X (ottDepth,  "OTT Depth",            0.f,   1.f,   0.9f,  0, 0.f)    \
    X (ottAtk,    "OTT Attack",           0.f,   1.f,   0.3f,  0, 0.f)    \
    X (ottRel,    "OTT Release",          0.f,   1.f,   0.45f, 0, 0.f)    \
    X (xo1,       "Crossover Low/Mid",   40.f, 400.f, 120.f,   0, 120.f)  \
    X (xo2,       "Crossover Mid/High", 1000.f, 8000.f, 2500.f, 0, 2500.f)\
    X (ottMS,     "OTT Mid/Side",         0.f,   1.f,   0.f,   1, 0.f)    \
    X (tL,        "Low Threshold",      -50.f,  -6.f, -24.f,   0, 0.f)    \
    X (fL,        "Low Floor",          -70.f, -20.f, -45.f,   0, 0.f)    \
    X (dL,        "Low Down",             0.f,   1.f,   0.6f,  0, 0.f)    \
    X (uL,        "Low Up",               0.f,   1.f,   0.5f,  0, 0.f)    \
    X (gL,        "Low Gain",           -12.f,  12.f,   0.f,   0, 0.f)    \
    X (tM,        "Mid Threshold",      -50.f,  -6.f, -24.f,   0, 0.f)    \
    X (fM,        "Mid Floor",          -70.f, -20.f, -42.f,   0, 0.f)    \
    X (dM,        "Mid Down",             0.f,   1.f,   0.6f,  0, 0.f)    \
    X (uM,        "Mid Up",               0.f,   1.f,   0.55f, 0, 0.f)    \
    X (gM,        "Mid Gain",           -12.f,  12.f,   0.f,   0, 0.f)    \
    X (tH,        "High Threshold",     -50.f,  -6.f, -26.f,   0, 0.f)    \
    X (fH,        "High Floor",         -70.f, -20.f, -42.f,   0, 0.f)    \
    X (dH,        "High Down",            0.f,   1.f,   0.55f, 0, 0.f)    \
    X (uH,        "High Up",              0.f,   1.f,   0.6f,  0, 0.f)    \
    X (gH,        "High Gain",          -12.f,  12.f,   0.f,   0, 0.f)    \
    X (eqOn,      "EQ On",                0.f,   1.f,   1.f,   1, 0.f)    \
    X (eqHpf,     "EQ HPF",              20.f, 250.f,  20.f,   0, 50.f)   \
    X (lsG,       "Low Shelf Gain",     -12.f,  12.f,   0.f,   0, 0.f)    \
    X (lsF,       "Low Shelf Freq",      40.f, 400.f, 100.f,   0, 120.f)  \
    X (mdG,       "Mid Gain",           -12.f,  12.f,   0.f,   0, 0.f)    \
    X (mdF,       "Mid Freq",           200.f, 8000.f, 1500.f, 0, 1500.f) \
    X (mdQ,       "Mid Q",                0.3f,  4.f,   0.9f,  0, 1.f)    \
    X (hsG,       "High Shelf Gain",    -12.f,  12.f,   0.f,   0, 0.f)    \
    X (hsF,       "High Shelf Freq",   2000.f, 14000.f, 8000.f, 0, 7000.f)\
    X (cmpOn,     "Comp On",              0.f,   1.f,   1.f,   1, 0.f)    \
    X (cmpThr,    "Comp Threshold",     -40.f,   0.f, -14.f,   0, 0.f)    \
    X (cmpRat,    "Comp Ratio",           1.5f, 20.f,   3.f,   0, 4.f)    \
    X (cmpAtk,    "Comp Attack",          0.1f, 100.f, 15.f,   0, 12.f)   \
    X (cmpRel,    "Comp Release",        20.f, 1200.f, 180.f,  0, 200.f)  \
    X (cmpKnee,   "Comp Knee",            0.f,  12.f,   4.f,   0, 0.f)    \
    X (cmpSc,     "Comp Sidechain HPF",  20.f, 400.f,  90.f,   0, 90.f)   \
    X (cmpAuto,   "Comp Auto Gain",       0.f,   1.f,   1.f,   1, 0.f)    \
    X (cmpMk,     "Comp Makeup",         -6.f,  12.f,   0.f,   0, 0.f)    \
    X (cmpMS,     "Comp Mid/Side",        0.f,   1.f,   0.f,   1, 0.f)    \
    X (limOn,     "Limiter On",           0.f,   1.f,   1.f,   1, 0.f)    \
    X (limPush,   "Limiter Push",         0.f,  18.f,   0.f,   0, 0.f)    \
    X (limCeil,   "Limiter Ceiling",     -6.f,   0.f,  -1.f,   0, 0.f)    \
    X (limRel,    "Limiter Release",     10.f, 500.f,  80.f,   0, 80.f)   \
    X (limClip,   "Limiter Clip",         0.f,   1.f,   0.f,   0, 0.f)    \
    X (mDyn,      "Macro Dynamics",       0.f,   1.f,   0.5f,  0, 0.f)    \
    X (mBri,      "Macro Brilliance",     0.f,   1.f,   0.5f,  0, 0.f)    \
    X (mSq,       "Macro Snappy/Squashy", 0.f,   1.f,   0.5f,  0, 0.f)    \
    X (cmpModel,  "Comp Model",           0.f,   3.f,   0.f,   2, 0.f)    \
    X (vmIn,      "Vari-Mu Input",      -12.f,  12.f,   0.f,   0, 0.f)    \
    X (vmThr,     "Vari-Mu Threshold",    0.f,  10.f,   5.f,   0, 0.f)    \
    X (vmTc,      "Vari-Mu Time Constant",0.f,   5.f,   1.f,   2, 0.f)    \
    X (vmBal,     "Vari-Mu Balance",     -1.f,   1.f,   0.f,   0, 0.f)    \
    X (fetIn,     "FET Input",          -12.f,  24.f,   0.f,   0, 0.f)    \
    X (fetAtk,    "FET Attack",           1.f,   7.f,   3.f,   0, 0.f)    \
    X (fetRel,    "FET Release",          1.f,   7.f,   7.f,   0, 0.f)    \
    X (fetRat,    "FET Ratio",            0.f,   4.f,   0.f,   2, 0.f)    \
    X (dcThr,     "Diode Comp Threshold",-20.f, 10.f,   0.f,   0, 0.f)    \
    X (dcRat,     "Diode Comp Ratio",     0.f,   4.f,   1.f,   2, 0.f)    \
    X (dcRec,     "Diode Comp Recovery",  0.f,   5.f,   1.f,   2, 0.f)    \
    X (dlOn,      "Diode Limiter On",     0.f,   1.f,   0.f,   1, 0.f)    \
    X (dlThr,     "Diode Limit Level",    4.f,  15.f,   8.f,   0, 0.f)    \
    X (dlRec,     "Diode Limit Recovery", 0.f,   5.f,   1.f,   2, 0.f)    \
    X (dlFast,    "Diode Limit Fast",     0.f,   1.f,   0.f,   1, 0.f)    \
    X (lufsTgt,   "Loudness Target",      0.f,   3.f,   2.f,   2, 0.f)

enum P
{
#define TN_ENUM(id, nm, mn, mx, df, kind, ctr) p_##id,
    TN_PARAMS (TN_ENUM)
#undef TN_ENUM
    kNumP
};

struct ParamDef { const char* id; const char* name; float mn, mx, def; int kind; float centre; };
inline const ParamDef* paramDefs()
{
    static const ParamDef defs[] = {
#define TN_DEF(id, nm, mn, mx, df, kind, ctr) { #id, nm, mn, mx, df, kind, ctr },
        TN_PARAMS (TN_DEF)
#undef TN_DEF
    };
    return defs;
}

// libelles des parametres "choix"
inline std::vector<const char*> choiceNames (int i)
{
    switch (i)
    {
        case p_cmpModel: return { "CLEAN", "VARI-MU", "FET", "DIODE" };
        case p_vmTc:     return { "1", "2", "3", "4", "5", "6" };
        case p_fetRat:   return { "4", "8", "12", "20", "ALL" };
        case p_dcRat:    return { "1.5", "2", "3", "4", "6" };
        case p_dcRec:    return { "100", "400", "800", "1.5K", "A1", "A2" };
        case p_dlRec:    return { "50", "100", "200", "800", "A1", "A2" };
        case p_lufsTgt:  return { "-14 STREAM", "-11 CD", "-9 CLUB", "-6 LOUD" };
        default:         return {};
    }
}

constexpr float kPi = 3.14159265358979f;
inline float dbToLin (float d) { return std::pow (10.f, d * 0.05f); }
inline float linToDb (float l) { return 20.f * std::log10 (std::max (l, 1e-6f)); }

// ---------------------------------------------------------------- filtres
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (float B0, float B1, float B2, float A0, float A1, float A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    float run (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0.f; }
    void lowpass  (float f, float fs, float Q = 0.70710678f) { const float w = 2 * kPi * f / fs, c = std::cos (w), a = std::sin (w) / (2 * Q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a); }
    void highpass (float f, float fs, float Q = 0.70710678f) { const float w = 2 * kPi * f / fs, c = std::cos (w), a = std::sin (w) / (2 * Q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a); }
    void peak (float f, float fs, float gDb, float Q)
    {
        const float A = std::pow (10.f, gDb / 40.f), w = 2 * kPi * f / fs, c = std::cos (w), a = std::sin (w) / (2 * Q);
        set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
    }
    void lowshelf (float f, float fs, float gDb)
    {
        const float A = std::pow (10.f, gDb / 40.f), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w), al = s / 2.f * std::sqrt (2.f), sq = 2.f * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
             (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highshelf (float f, float fs, float gDb)
    {
        const float A = std::pow (10.f, gDb / 40.f), w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w), al = s / 2.f * std::sqrt (2.f), sq = 2.f * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
};

// filtre Linkwitz-Riley 4e ordre : deux Butterworth en cascade ; LP + HP = passe-tout (somme plate)
struct Lr4
{
    Biquad lp[2], hp[2];
    void set (float f, float fs) { for (auto& b : lp) b.lowpass (f, fs); for (auto& b : hp) b.highpass (f, fs); }
    float low  (float x) { return lp[1].run (lp[0].run (x)); }
    float high (float x) { return hp[1].run (hp[0].run (x)); }
};

// ---------------------------------------------------------------- mesure de volume (ITU-R BS.1770 / EBU R128)
struct Lufs
{
    Biquad hs[2], hp[2];
    int hop = 4410, cnt = 0, nBlocks = 0;
    double acc = 0.0, blk[30] {};
    int head = 0;
    std::vector<double> histE;  std::vector<int> histN;
    std::atomic<float> momentary { -70.f }, shortTerm { -70.f }, integrated { -70.f };
    Lufs() : histE (801, 0.0), histN (801, 0) {}
    void prepare (float fs)
    {
        const float K1 = std::tan (kPi * 1681.974450955533f / fs), Vh = std::pow (10.f, 3.999843853973347f / 20.f), Vb = std::pow (Vh, 0.4996667741545416f), Q1 = 0.7071752369554196f;
        const float a0 = 1.f + K1 / Q1 + K1 * K1;
        const float b0 = (Vh + Vb * K1 / Q1 + K1 * K1) / a0, b1 = 2.f * (K1 * K1 - Vh) / a0, b2 = (Vh - Vb * K1 / Q1 + K1 * K1) / a0;
        const float a1 = 2.f * (K1 * K1 - 1.f) / a0, a2 = (1.f - K1 / Q1 + K1 * K1) / a0;
        const float K2 = std::tan (kPi * 38.13547087602444f / fs), Q2 = 0.5003270373238773f, c0 = 1.f + K2 / Q2 + K2 * K2;
        for (int c = 0; c < 2; ++c)
        {
            hs[c].b0 = b0; hs[c].b1 = b1; hs[c].b2 = b2; hs[c].a1 = a1; hs[c].a2 = a2; hs[c].reset();
            hp[c].b0 = 1.f; hp[c].b1 = -2.f; hp[c].b2 = 1.f; hp[c].a1 = 2.f * (K2 * K2 - 1.f) / c0; hp[c].a2 = (1.f - K2 / Q2 + K2 * K2) / c0; hp[c].reset();
        }
        hop = (int) (fs * 0.1f); cnt = 0; acc = 0.0; nBlocks = 0; head = 0;
        for (auto& b : blk) b = 0.0;
        resetIntegrated();
        momentary.store (-70.f); shortTerm.store (-70.f); integrated.store (-70.f);
    }
    void resetIntegrated() { std::fill (histE.begin(), histE.end(), 0.0); std::fill (histN.begin(), histN.end(), 0); }
    static float toLufs (double e) { return e <= 1e-12 ? -70.f : (float) (-0.691 + 10.0 * std::log10 (e)); }
    double meanLast (int n) const { double s = 0; for (int i = 0; i < n; ++i) s += blk[(head - 1 - i + 60) % 30]; return s / n; }
    void push (float l, float r)
    {
        const float kl = hp[0].run (hs[0].run (l)), kr = hp[1].run (hs[1].run (r));
        acc += (double) kl * kl + (double) kr * kr;
        if (++cnt >= hop)
        {
            blk[head] = acc / hop; head = (head + 1) % 30; acc = 0.0; cnt = 0; nBlocks = std::min (nBlocks + 1, 30);
            if (nBlocks >= 4)
            {
                const double e400 = meanLast (4);
                const float lm = toLufs (e400);
                momentary.store (lm);
                if (lm > -70.f) { const int bin = std::min (800, (int) ((lm + 70.f) * 10.f)); histE[(size_t) bin] += e400; histN[(size_t) bin] += 1; }
                if (nBlocks >= 30) shortTerm.store (toLufs (meanLast (30)));
                else shortTerm.store (toLufs (meanLast (nBlocks)));
                // volume integre : double seuil (absolu -70, relatif -10 LU)
                double sum = 0; long n = 0;
                for (size_t i = 0; i < histE.size(); ++i) { sum += histE[i]; n += histN[i]; }
                if (n > 0)
                {
                    const float gate = toLufs (sum / (double) n) - 10.f;
                    double s2 = 0; long n2 = 0;
                    for (size_t i = 0; i < histE.size(); ++i)
                        if ((float) i / 10.f - 70.f >= gate - 0.1f) { s2 += histE[i]; n2 += histN[i]; }
                    integrated.store (n2 > 0 ? toLufs (s2 / (double) n2) : -70.f);
                }
            }
        }
    }
};

// ---------------------------------------------------------------- moteur
class Master
{
public:
    // compteurs lus par l'interface
    std::atomic<float> inPk { 0.f }, outPk { 0.f }, cmpGr { 0.f }, limGr { 0.f }, matchDb { 0.f };
    std::atomic<float> bandDown[3], bandUp[3];
    std::atomic<bool> resetLufsFlag { false };
    Lufs lufsIn, lufsOut;
    std::atomic<float> macroInfo[3];   // reserve

    Master() { for (int i = 0; i < 3; ++i) { bandDown[i] = 0.f; bandUp[i] = 0.f; macroInfo[i] = 0.f; } }

    int latency() const { return N + 1 + mdl::Os2::M; }

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        N = (int) std::lround (0.003f * fs);
        for (int c = 0; c < 2; ++c)
        {
            dly[c].assign ((size_t) N + 1, 0.f); byp[c].assign ((size_t) N + 1, 0.f);
            avgBuf[c].assign ((size_t) N + 1, 1.f);
        }
        dqVal.assign ((size_t) N + 2, 1.f); dqIdx.assign ((size_t) N + 2, 0);
        dqHead = dqTail = 0; sampleIdx = 0; wpos = 0; avgSum = (double) (N + 1); stGain = 1.f; 
        for (int c = 0; c < 2; ++c) { sc[c].reset(); eqHp[c].reset(); eqLs[c].reset(); eqMd[c].reset(); eqHs[c].reset(); sp1[c] = Lr4(); sp2[c] = Lr4(); sp2lo[c] = Lr4(); }
        for (auto& b : band) b = Band();
        grState = 0.f; limEnv = 0.f; xoCache1 = xoCache2 = -1.f; eqKey = 0;
        lufsIn.prepare (fs); lufsOut.prepare (fs);
        os.prepare();
        vm.prepare (2.f * fs); fet.prepare (2.f * fs); dc.prepare (2.f * fs); dl.prepare (2.f * fs);
        for (int c = 0; c < 2; ++c) { osDly[c].assign ((size_t) mdl::Os2::M, 0.f); dryDly[c].assign ((size_t) mdl::Os2::M, 0.f); }
        osPos = 0;
        bypassMix = 0.f; matchGainDb = 0.f; matchSm = 0.f;
        inPkS = outPkS = 0.f;
    }

    // valeurs reellement appliquees apres les macros, dans les unites de chaque reglage (sans bornage).
    // Sert au moteur ET a l'interface (arcs orange sur les potards deplaces par les macros).
    static void applyMacros (const float* v, float* e)
    {
        for (int i = 0; i < kNumP; ++i) e[i] = v[i];
        const float dyn = (v[p_mDyn] - 0.5f) * 2.f, bri = (v[p_mBri] - 0.5f) * 2.f, sq = (v[p_mSq] - 0.5f) * 2.f;
        static const float ln2 = std::log (2.f);
        e[p_ottAtk] = v[p_ottAtk] + dyn * 1.5f * ln2 / std::log (150.f);        // = attaque x 2^(1.5 dyn)
        e[p_ottRel] = v[p_ottRel] + sq * 0.8f * ln2 / std::log (60.f);          // = release x 2^(0.8 sq)
        static const int iT[3] { p_tL, p_tM, p_tH }, iF[3] { p_fL, p_fM, p_fH }, iD[3] { p_dL, p_dM, p_dH }, iU[3] { p_uL, p_uM, p_uH }, iG[3] { p_gL, p_gM, p_gH };
        static const float briGain[3] { -2.5f, 0.f, 2.5f }, briUp[3] { -0.4f, 0.f, 0.4f };
        for (int b = 0; b < 3; ++b)
        {
            e[iT[b]] = v[iT[b]] - 10.f * sq;
            e[iF[b]] = v[iF[b]] - 5.f * sq;
            e[iD[b]] = v[iD[b]] * (1.f + 0.5f * sq) * (1.f - 0.35f * dyn);
            e[iU[b]] = v[iU[b]] * (1.f - 0.4f * dyn) * (1.f + briUp[b] * bri);
            e[iG[b]] = v[iG[b]] + briGain[b] * bri;
        }
        e[p_cmpThr] = v[p_cmpThr] - 6.f * sq;
        e[p_cmpRat] = 1.f + (v[p_cmpRat] - 1.f) * (1.f + 0.5f * sq);
        e[p_cmpAtk] = v[p_cmpAtk] * std::pow (2.f, dyn * 1.2f);
        e[p_cmpRel] = v[p_cmpRel] * std::pow (2.f, sq * 0.8f);
        e[p_lsG] = v[p_lsG] - 2.5f * bri;
        e[p_hsG] = v[p_hsG] + 4.f * bri;
        // modeles de circuits
        e[p_vmThr] = v[p_vmThr] + 2.f * sq;                                  // squashy : seuil plus bas
        e[p_vmIn]  = v[p_vmIn] + 2.f * dyn;                                  // punch : les tubes chauffent plus
        e[p_fetIn]  = v[p_fetIn] + 6.f * sq;                                 // squashy : on pousse l'entree
        e[p_fetAtk] = v[p_fetAtk] - 1.5f * dyn;                              // punch : attaque plus lente (le transitoire passe)
        e[p_fetRel] = v[p_fetRel] - 1.2f * sq;                               // squashy : recuperation plus lente
        e[p_dcThr] = v[p_dcThr] - 6.f * sq;
    }

    void setParams (const float* v)
    {
        float e[kNumP];
        applyMacros (v, e);
        for (int i = 0; i < kNumP; ++i) p[i] = v[i];
        const float I = p[p_intensity];

        // ---- OTT
        ottAtkMs = 0.2f * std::pow (150.f, e[p_ottAtk]);
        ottRelMs = 15.f * std::pow (60.f, e[p_ottRel]);
        const float depth = p[p_ottDepth] * I;
        static const int iT[3] { p_tL, p_tM, p_tH }, iF[3] { p_fL, p_fM, p_fH }, iD[3] { p_dL, p_dM, p_dH }, iU[3] { p_uL, p_uM, p_uH }, iG[3] { p_gL, p_gM, p_gH };
        static const float relScale[3] { 2.5f, 1.f, 0.5f };
        for (int b = 0; b < 3; ++b)
        {
            band[b].thr = e[iT[b]];
            band[b].flo = e[iF[b]];
            band[b].down = std::min (1.f, depth * e[iD[b]]);
            band[b].up = std::min (1.f, depth * e[iU[b]]);
            band[b].gainDb = e[iG[b]];
            const float aMs = std::max (0.1f, ottAtkMs), rMs = ottRelMs * relScale[b];
            band[b].aC = 1.f - std::exp (-1.f / (aMs * 0.001f * fs));
            band[b].rC = 1.f - std::exp (-1.f / (rMs * 0.001f * fs));
        }
        // ---- compresseur
        cThr = e[p_cmpThr];
        cRat = 1.f + (e[p_cmpRat] - 1.f) * I;
        const float cAtk = std::max (0.05f, e[p_cmpAtk]), cRel = e[p_cmpRel];
        cAC = 1.f - std::exp (-1.f / (cAtk * 0.001f * fs));
        cRC = 1.f - std::exp (-1.f / (std::max (5.f, cRel) * 0.001f * fs));
        cMakeDb = (p[p_cmpAuto] > 0.5f ? -cThr * (1.f - 1.f / cRat) * 0.45f : 0.f) + p[p_cmpMk];
        // ---- modeles de circuits (dans le compresseur) et limiteur a diodes
        model = (int) std::lround (p[p_cmpModel]);
        const auto ri = [this] (int id) { return (int) std::lround (p[id]); };
        if (model == 1) vm.set (e[p_vmIn], e[p_vmThr], ri (p_vmTc), p[p_vmBal], p[p_cmpMk], I);
        if (model == 2) fet.set (e[p_fetIn], p[p_cmpMk], e[p_fetAtk], e[p_fetRel], ri (p_fetRat), I);
        if (model == 3) { dc.setComp (true, e[p_dcThr], ri (p_dcRat), ri (p_dcRec), p[p_cmpMk], I); dc.setLimit (false, 15.f, 0, false); }
        dl.setComp (false, 0.f, 0, 0, 0.f, 1.f); dl.limiterOnlyColour();
        dl.setLimit (true, p[p_dlThr], ri (p_dlRec), p[p_dlFast] > 0.5f);
        // ---- limiteur
        pushLin = dbToLin (p[p_limPush]);
        ceilLin = dbToLin (p[p_limCeil]);
        limRC = 1.f - std::exp (-1.f / (p[p_limRel] * 0.001f * fs));
        inLin = dbToLin (p[p_in]); outLin = dbToLin (p[p_out]);
        // ---- filtres dependant des parametres (recalcules seulement s'ils changent)
        if (p[p_xo1] != xoCache1) { xoCache1 = p[p_xo1]; for (int c = 0; c < 2; ++c) sp1[c].set (xoCache1, fs); }
        const float xo2 = std::max (p[p_xo2], p[p_xo1] * 2.f);
        if (xo2 != xoCache2) { xoCache2 = xo2; for (int c = 0; c < 2; ++c) { sp2[c].set (xo2, fs); sp2lo[c].set (xo2, fs); } }
        const float lsG = e[p_lsG], hsG = e[p_hsG];
        const float key = p[p_eqHpf] + 3.1f * p[p_lsF] + 5.7f * lsG + 7.3f * p[p_mdF] + 11.f * p[p_mdG] + 13.f * p[p_mdQ] + 17.f * hsG + 19.f * p[p_hsF] + 23.f * p[p_cmpSc];
        if (key != eqKey)
        {
            eqKey = key;
            for (int c = 0; c < 2; ++c)
            {
                eqHp[c].highpass (p[p_eqHpf], fs); eqLs[c].lowshelf (p[p_lsF], fs, lsG);
                eqMd[c].peak (p[p_mdF], fs, p[p_mdG], p[p_mdQ]); eqHs[c].highshelf (std::min (p[p_hsF], 0.45f * fs), fs, hsG);
                sc[c].highpass (p[p_cmpSc], fs);
            }
        }
    }

    void process (float* L, float* R, int n)
    {
        if (resetLufsFlag.exchange (false)) { lufsOut.resetIntegrated(); lufsIn.resetIntegrated(); }
        const bool byp = p[p_bypass] > 0.5f, match = p[p_match] > 0.5f;
        const bool ottOn = p[p_ottOn] > 0.5f, eqOn = p[p_eqOn] > 0.5f, cmpOn = p[p_cmpOn] > 0.5f, limOn = p[p_limOn] > 0.5f;
        const bool ottMS = p[p_ottMS] > 0.5f, cmpMS = p[p_cmpMS] > 0.5f;
        float peakIn = 0.f, peakOut = 0.f, grMax = 0.f, limMax = 0.f;
        const float xfade = 1.f / (0.01f * fs);

        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            lufsIn.push (dryL, dryR);
            peakIn = std::max (peakIn, std::max (std::fabs (dryL), std::fabs (dryR)));
            // chemin de bypass : entree retardee de la latence du limiteur, avec compensation de volume
            float dL2 = dryDly[0][(size_t) osPos], dR2 = dryDly[1][(size_t) osPos];
            dryDly[0][(size_t) osPos] = dryL; dryDly[1][(size_t) osPos] = dryR;
            const float bl = byp_(0, dL2), br = byp_(1, dR2);

            float l = dryL * inLin, r = dryR * inLin;

            // ---- OTT
            if (ottOn)
            {
                if (ottMS) { const float m = 0.5f * (l + r), s = 0.5f * (l - r); l = m; r = s; }
                ottProcess (l, r, ottMS);
                if (ottMS) { const float m = l, s = r; l = m + s; r = m - s; }
            }
            // ---- EQ
            if (eqOn)
            {
                l = eqHs[0].run (eqMd[0].run (eqLs[0].run (eqHp[0].run (l))));
                r = eqHs[1].run (eqMd[1].run (eqLs[1].run (eqHp[1].run (r))));
            }
            // ---- compresseur
            float grNow = 0.f, dlGr = 0.f;
            if (cmpOn && cmpMS) { const float m = 0.5f * (l + r), s = 0.5f * (l - r); l = m; r = s; }
            if (cmpOn && model == 0) grNow = cmpProcess (l, r, cmpMS);
            // modeles de circuits + limiteur a diodes : suréchantillonnage x2 (sinon simple retard de meme duree)
            const bool modelOn = cmpOn && model > 0 && p[p_intensity] > 0.0005f, dlOnNow = p[p_dlOn] > 0.5f;
            const bool decode = cmpOn && cmpMS;
            if (modelOn || dlOnNow)
            {
                float u[2][2];
                os.upsample (0, l, u[0][0], u[0][1]); os.upsample (1, r, u[1][0], u[1][1]);
                for (int k = 0; k < 2; ++k)
                {
                    float a = u[0][k], b = u[1][k], lg = 0.f;
                    if (modelOn)
                    {
                        const float g = model == 1 ? vm.process (a, b) : model == 2 ? fet.process (a, b) : dc.process (a, b, lg);
                        grNow = std::max (grNow, g);
                    }
                    if (decode) { const float m = a, s = b; a = m + s; b = m - s; }
                    if (dlOnNow) { dl.process (a, b, lg); dlGr = std::max (dlGr, lg); }
                    u[0][k] = a; u[1][k] = b;
                }
                l = os.downsample (0, u[0][0], u[0][1]); r = os.downsample (1, u[1][0], u[1][1]);
            }
            else if (decode) { const float m = l, s = r; l = m + s; r = m - s; }
            {
                // retard compensateur : les sorties des deux chemins restent alignees
                const float dlL = osDly[0][(size_t) osPos], dlR = osDly[1][(size_t) osPos];
                osDly[0][(size_t) osPos] = l; osDly[1][(size_t) osPos] = r;
                if (! (modelOn || dlOnNow)) { l = dlL; r = dlR; }
                osPos = (osPos + 1) % mdl::Os2::M;
            }
            grMax = std::max (grMax, grNow);
            limMax = std::max (limMax, dlGr);

            // ---- clipper doux + limiteur
            l *= pushLin; r *= pushLin;
            if (p[p_limClip] > 0.001f)
            {
                const float c = p[p_limClip];
                l = (1.f - c) * l + c * ceilLin * std::tanh (l / ceilLin);
                r = (1.f - c) * r + c * ceilLin * std::tanh (r / ceilLin);
            }
            float ol = l, orr = r;
            if (limOn) limProcess (ol, orr, limMax);
            else { ol = delay_(0, ol); orr = delay_(1, orr); }                       // meme latence, sans limitation
            ol *= outLin; orr *= outLin;

            lufsOut.push (ol, orr);
            // ---- bypass (fondu) et niveau egal
            const float target = byp ? 1.f : 0.f;
            bypassMix += std::max (-xfade, std::min (xfade, target - bypassMix));
            float fl = ol, fr = orr;
            if (bypassMix > 0.f)
            {
                const float g = match ? dbToLin (matchGainDb) : 1.f;
                fl = ol * (1.f - bypassMix) + bl * g * bypassMix;
                fr = orr * (1.f - bypassMix) + br * g * bypassMix;
            }
            L[i] = fl; R[i] = fr;
            peakOut = std::max (peakOut, std::max (std::fabs (fl), std::fabs (fr)));
        }
        // niveau egal : la version contournee est amenee au volume court terme du signal traite
        {
            const float lo = lufsOut.shortTerm.load(), li = lufsIn.shortTerm.load();
            const float want = (lo > -69.f && li > -69.f) ? std::max (-24.f, std::min (24.f, lo - li)) : 0.f;
            matchSm += (want - matchSm) * std::min (1.f, 0.002f * (float) n / 4.f);
            matchGainDb = matchSm;
            matchDb.store (matchGainDb);
        }
        const float dec = std::pow (0.9f, (float) n / 256.f);
        inPkS = std::max (peakIn, inPkS * dec); outPkS = std::max (peakOut, outPkS * dec);
        inPk.store (inPkS); outPk.store (outPkS);
        cmpGr.store (std::max (grMax, cmpGr.load() * dec));
        limGr.store (std::max (limMax, limGr.load() * dec));
        for (int b = 0; b < 3; ++b)
        {
            bandDown[b].store (std::max (band[b].meterDown, bandDown[b].load() * dec));
            bandUp[b].store (std::max (band[b].meterUp, bandUp[b].load() * dec));
            band[b].meterDown = band[b].meterUp = 0.f;
        }
    }

private:
    struct Det { float env = 0.f; };
    struct Band
    {
        Det det[2]; float g[2] { 1.f, 1.f }, gT[2] { 1.f, 1.f };
        float thr = -24.f, flo = -45.f, down = 0.f, up = 0.f, gainDb = 0.f, aC = 0.01f, rC = 0.001f;
        float meterDown = 0.f, meterUp = 0.f;
    };

    float p[kNumP] {};
    float fs = 44100.f;
    int N = 132;
    // OTT
    Band band[3];
    Lr4 sp1[2], sp2[2], sp2lo[2];
    float ottAtkMs = 5.f, ottRelMs = 100.f;
    int ctl = 0;
    // EQ
    Biquad eqHp[2], eqLs[2], eqMd[2], eqHs[2], sc[2];
    float xoCache1 = -1.f, xoCache2 = -1.f, eqKey = 0.f;
    // comp
    float cThr = -14.f, cRat = 3.f, cAC = 0.01f, cRC = 0.001f, cMakeDb = 0.f, grState = 0.f, grStateB = 0.f;
    // limiter
    float pushLin = 1.f, ceilLin = 0.89f, limRC = 0.001f, limEnv = 0.f, stGain = 1.f, inLin = 1.f, outLin = 1.f;
    std::vector<float> dly[2], byp[2], avgBuf[2];
    std::vector<float> dqVal; std::vector<int64_t> dqIdx;
    int dqHead = 0, dqTail = 0, wpos = 0; int64_t sampleIdx = 0; double avgSum = 0.0;
    std::vector<float> stRing;
    float bypassMix = 0.f, matchGainDb = 0.f, matchSm = 0.f, inPkS = 0.f, outPkS = 0.f;
    // modeles
    int model = 0, osPos = 0;
    mdl::Os2 os;
    mdl::VariMu vm; mdl::Fet fet; mdl::Diode dc, dl;
    std::vector<float> osDly[2], dryDly[2];

    float byp_ (int c, float x) { auto& b = byp[c]; const float y = b[(size_t) wpos]; b[(size_t) wpos] = x; return y; }
    float delay_ (int c, float x) { auto& b = dly[c]; const float y = b[(size_t) wpos]; b[(size_t) wpos] = x; if (c == 1) wpos = (wpos + 1) % (N + 1); return y; }
    // NB : wpos est avance une fois par echantillon (dans limProcess ou delay_ canal 1) ; byp_ lit avant l'avance.

    static float gainCurve (float lvlDb, float thr, float flo, float down, float up, float& gDown, float& gUp)
    {
        gDown = gUp = 0.f;
        const float knee = 6.f;
        if (down > 0.001f)
        {
            const float Rd = 1.f + 19.f * std::pow (down, 1.5f), over = lvlDb - thr, k = 1.f - 1.f / Rd;
            if (over >= knee * 0.5f) gDown = over * k;
            else if (over > -knee * 0.5f) { const float x = over + knee * 0.5f; gDown = k * x * x / (2.f * knee); }
        }
        if (up > 0.001f)
        {
            const float Ru = 1.f + 9.f * up, under = flo - lvlDb;
            if (under > 0.f)
            {
                float g = under * (1.f - 1.f / Ru);
                g = std::min (g, 6.f + 24.f * up);
                g *= std::min (1.f, under / 6.f);
                g *= std::max (0.f, std::min (1.f, (lvlDb + 95.f) / 15.f));           // pas de gain sur le silence
                gUp = g;
            }
        }
        return gUp - gDown;
    }

    void ottProcess (float& l, float& r, bool ms)
    {
        // separation en 3 bandes, somme a phase coherente (la bande grave traverse un passe-tout a la 2e frequence)
        float in[2] { l, r }, lo[2], mid[2], hi[2];
        for (int c = 0; c < 2; ++c)
        {
            const float x = in[c];
            const float lowA = sp1[c].low (x), rest = sp1[c].high (x);
            lo[c]  = sp2lo[c].low (lowA) + sp2lo[c].high (lowA);
            mid[c] = sp2[c].low (rest);
            hi[c]  = sp2[c].high (rest);
        }
        float* bands[3][2] { { &lo[0], &lo[1] }, { &mid[0], &mid[1] }, { &hi[0], &hi[1] } };
        const float smooth = 0.05f;
        ++ctl;
        const bool update = (ctl & 15) == 0;
        for (int b = 0; b < 3; ++b)
        {
            auto& B = band[b];
            for (int c = 0; c < 2; ++c)
            {
                const float a = std::fabs (*bands[b][c]) + 1e-20f;
                B.det[c].env += (a > B.det[c].env ? B.aC : B.rC) * (a - B.det[c].env);
            }
            if (update)
            {
                float gd[2] { 0, 0 }, gu[2] { 0, 0 };
                if (ms)
                    for (int c = 0; c < 2; ++c)
                    {
                        const float d = gainCurve (linToDb (B.det[c].env), B.thr, B.flo, B.down, B.up, gd[c], gu[c]);
                        B.gT[c] = dbToLin (d + B.gainDb);
                    }
                else
                {
                    const float d = gainCurve (linToDb (std::max (B.det[0].env, B.det[1].env)), B.thr, B.flo, B.down, B.up, gd[0], gu[0]);
                    B.gT[0] = B.gT[1] = dbToLin (d + B.gainDb); gd[1] = gd[0]; gu[1] = gu[0];
                }
                B.meterDown = std::max (B.meterDown, std::max (gd[0], gd[1]));
                B.meterUp = std::max (B.meterUp, std::max (gu[0], gu[1]));
            }
            for (int c = 0; c < 2; ++c) { B.g[c] += (B.gT[c] - B.g[c]) * smooth; *bands[b][c] *= B.g[c]; }
        }
        l = lo[0] + mid[0] + hi[0];
        r = lo[1] + mid[1] + hi[1];
    }

    float cmpProcess (float& l, float& r, bool ms)
    {
        const float sl = sc[0].run (l), sr = sc[1].run (r);
        float a;
        if (ms) a = std::max (std::fabs (sl), std::fabs (sr)); else a = std::max (std::fabs (sl), std::fabs (sr));
        // detecteur lie (meme reduction pour les deux voies, en L/R comme en M/S : la rotation M/S est deja appliquee en amont)
        const float lvl = linToDb (a + 1e-12f);
        float gr = 0.f;
        const float knee = std::max (0.01f, p[p_cmpKnee]), over = lvl - cThr, k = 1.f - 1.f / std::max (1.f, cRat);
        if (over >= knee * 0.5f) gr = over * k;
        else if (over > -knee * 0.5f) { const float x = over + knee * 0.5f; gr = k * x * x / (2.f * knee); }
        grState += (gr > grState ? cAC : cRC) * (gr - grState);
        const float g = dbToLin (-grState + cMakeDb);
        l *= g; r *= g;
        return grState;
    }

    void limProcess (float& l, float& r, float& limMax)
    {
        // 1. gain requis pour cet echantillon (futur par rapport a la sortie retardee)
        const float pk = std::max (std::fabs (l), std::fabs (r));
        const float need = pk > ceilLin ? ceilLin / pk : 1.f;
        // 2. minimum glissant sur N+1 echantillons (file monotone)
        const int cap = N + 2;
        while (dqTail != dqHead && dqVal[(size_t) ((dqTail - 1 + cap) % cap)] >= need) dqTail = (dqTail - 1 + cap) % cap;
        dqVal[(size_t) dqTail] = need; dqIdx[(size_t) dqTail] = sampleIdx; dqTail = (dqTail + 1) % cap;
        while (dqIdx[(size_t) dqHead] <= sampleIdx - (N + 1)) dqHead = (dqHead + 1) % cap;
        const float mn = dqVal[(size_t) dqHead];
        ++sampleIdx;
        // 3. relachement (la reduction ne remonte que lentement)
        if (mn < stGain) stGain = mn; else stGain += (mn - stGain) * limRC;
        // 4. attaque : moyenne glissante sur N+1 echantillons (rampe douce, jamais au-dessus du gain requis)
        auto& ring = avgBuf[0];
        const float old = ring[(size_t) wpos];
        ring[(size_t) wpos] = stGain;
        avgSum += (double) stGain - (double) old;
        const float g = (float) (avgSum / (double) (N + 1));
        // 5. audio retarde de N echantillons
        const float dl = dly[0][(size_t) wpos], dr = dly[1][(size_t) wpos];
        dly[0][(size_t) wpos] = l; dly[1][(size_t) wpos] = r;
        wpos = (wpos + 1) % (N + 1);
        l = std::max (-ceilLin, std::min (ceilLin, dl * g));
        r = std::max (-ceilLin, std::min (ceilLin, dr * g));
        limMax = std::max (limMax, -linToDb (std::min (1.f, g)));
    }
};
} // namespace tn
