// Zenith - emulations de circuits reels (modeles comportementaux bases sur les schemas)
//   VARI-MU : limiteur a tubes 6386 en push-pull, detection sur la sortie, 6 constantes de temps (type 660)
//   FET     : attenuateur FET + preampli/ampli ligne classe A + transfo de sortie, boutons de ratio dont ALL (type 1176LN)
//   DIODE   : pont de diodes + ampli classe A + transfos, compresseur et limiteur a recuperation AUTO (type 33609)
// Tout tourne en suréchantillonnage x2 (filtre demi-bande a phase lineaire, latence fixe). Aucune dependance JUCE.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace tn
{
namespace mdl
{
inline float coefMs (float ms, float fs) { return 1.f - std::exp (-1.f / (std::max (0.001f, ms) * 0.001f * fs)); }
inline float softplus (float x) { return x > 18.f ? x : (x < -18.f ? std::exp (x) : std::log1p (std::exp (x))); }
inline float sigm (float x) { return 1.f / (1.f + std::exp (-x)); }
inline float todB (float l) { return 20.f * std::log10 (std::max (l, 1e-6f)); }
inline float fromdB (float d) { return std::pow (10.f, d * 0.05f); }

// courbe statique d'un compresseur (genou doux de largeur w), renvoie la reduction en dB
inline float grCurve (float over, float ratio, float w)
{
    const float k = 1.f - 1.f / std::max (1.f, ratio);
    if (over >= w * 0.5f) return over * k;
    if (over > -w * 0.5f) { const float x = over + w * 0.5f; return k * x * x / (2.f * std::max (0.01f, w)); }
    return 0.f;
}

// ---------------------------------------------------------------- suréchantillonnage x2 (FIR demi-bande, Kaiser)
struct Os2
{
    static constexpr int M = 47, T = 2 * M + 1;   // latence totale (montee + descente) = M echantillons
    float h[T] {};
    float ub[2][2 * T] {}, db[2][2 * T] {};
    int up[2] { 0, 0 }, dp[2] { 0, 0 };

    static double i0 (double x) { double s = 1, t = 1; for (int k = 1; k < 40; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; }
    void prepare()
    {
        const double beta = 8.0;
        double sum = 0;
        for (int n = 0; n < T; ++n)
        {
            const double m = n - M, x = 0.5 * m;
            const double sinc = m == 0 ? 1.0 : std::sin (3.14159265358979 * x) / (3.14159265358979 * x);
            const double r = m / (double) M, w = i0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0 (beta);
            h[n] = (float) (0.5 * sinc * w); sum += h[n];
        }
        for (auto& v : h) v = (float) (v / sum);            // gain DC = 1
        reset();
    }
    void reset() { for (int c = 0; c < 2; ++c) { std::fill (ub[c], ub[c] + 2 * T, 0.f); std::fill (db[c], db[c] + 2 * T, 0.f); up[c] = dp[c] = 0; } }
    static void push (float* b, int& p, float x) { p = (p == 0 ? T - 1 : p - 1); b[p] = b[p + T] = x; }
    float fir (const float* b, int p) const { float s = 0; const float* x = b + p; for (int k = 0; k < T; ++k) s += h[k] * x[k]; return s; }
    void upsample (int c, float x, float& y0, float& y1)
    {
        push (ub[c], up[c], 2.f * x); y0 = fir (ub[c], up[c]);
        push (ub[c], up[c], 0.f);     y1 = fir (ub[c], up[c]);
    }
    float downsample (int c, float a, float b)
    {
        push (db[c], dp[c], a); const float y = fir (db[c], dp[c]);
        push (db[c], dp[c], b);
        return y;
    }
};

// ---------------------------------------------------------------- transformateur : blocage du continu + saturation du grave
struct Iron
{
    float drive = 0.f, lp = 0.f, hx = 0.f, hy = 0.f, cLp = 0.01f, rHp = 0.999f;
    void prepare (float fs, float fLow = 140.f, float fHp = 5.f)
    {
        cLp = 1.f - std::exp (-2.f * 3.14159265f * fLow / fs);
        rHp = std::exp (-2.f * 3.14159265f * fHp / fs);
        lp = hx = hy = 0.f;
    }
    float run (float x)
    {
        hy = x - hx + rHp * hy; hx = x;                                     // le transfo ne passe pas le continu
        lp += cLp * (hy - lp);
        if (drive < 1e-4f) return hy;
        const float lf = std::tanh (drive * lp) / drive;                     // le fer sature d'abord dans le grave
        return hy - lp + lf;
    }
};

// etage classe A : saturation douce asymetrique (harmoniques paires), pente 1 au repos
inline float classA (float u, float k, float bias)
{
    if (k < 1e-4f) return u;
    const float t0 = std::tanh (k * bias), slope = k * (1.f - t0 * t0);
    return (std::tanh (k * (u + bias)) - t0) / slope;
}

// ================================================================ VARI-MU (tubes 6386 push-pull, type 660)
struct VariMu
{
    Iron tin[2], tout[2];
    float fs = 88200.f;
    float inG = 1.f, outG = 1.f, scG = 1.f, bal = 0.f, balS = 0.f, I = 0.f;
    float cv = 0.f, cs1 = 0.f, cs2 = 0.f;
    float aC = 0.1f, rF = 0.001f, chg1 = 0.f, dec1 = 0.f, chg2 = 0.f, dec2 = 0.f;
    bool slow1 = false, slow2 = false;
    static constexpr float grid = 1.1f, A = 2.f;

    void prepare (float sr) { fs = sr; for (int c = 0; c < 2; ++c) { tin[c].prepare (fs, 120.f); tout[c].prepare (fs, 160.f); } cv = cs1 = cs2 = 0.f; balS = 0.f; }

    // thrKnob 0..10 (10 = seuil le plus bas), tc 0..5
    void set (float inDb, float thrKnob, int tc, float balance, float outDb, float intensity)
    {
        I = intensity;
        inG = fromdB (inDb * I); outG = fromdB (outDb * I);
        scG = fromdB (4.f + 3.f * thrKnob);                                 // seuil : -4 dBFS (0) a -34 dBFS (10)
        bal = 0.12f * balance * I;
        for (int c = 0; c < 2; ++c) { tin[c].drive = 0.35f * I; tout[c].drive = 0.45f * I; }
        // constantes de temps d'origine : attaque / recuperation (5 et 6 dependent du programme)
        static const float atk[6] { 0.2f, 0.2f, 0.4f, 0.4f, 0.4f, 0.2f };
        static const float rel[6] { 300.f, 800.f, 2000.f, 5000.f, 2000.f, 300.f };
        tc = std::max (0, std::min (5, tc));
        aC = coefMs (atk[tc], fs);
        rF = coefMs (rel[tc] * 0.45f, fs);
        slow1 = tc >= 4; slow2 = tc == 5;
        chg1 = coefMs (tc == 4 ? 1500.f : 900.f, fs); dec1 = coefMs (10000.f * 0.45f, fs);
        chg2 = coefMs (4000.f, fs);                    dec2 = coefMs (25000.f * 0.45f, fs);
    }

    float process (float& l, float& r)
    {
        balS += 0.0005f * (bal - balS);
        const float cc = cv * I;
        const float spIdle = 2.f * std::log (2.f);
        float y[2], x[2] { l * inG, r * inG };
        for (int c = 0; c < 2; ++c)
        {
            const float u = grid * tin[c].run (x[c]);
            // deux triodes a pente variable en opposition : la polarisation cc baisse le gain et courbe la caracteristique
            const float v = ((1.f + balS) * softplus (u - cc) - (1.f - balS) * softplus (-u - cc) - balS * spIdle) / grid;
            y[c] = tout[c].run (v);
        }
        // detection sur la sortie (contre-reaction), redressement double alternance
        const float rr = std::max (std::fabs (y[0]), std::fabs (y[1])) * scG;
        const float target = A * std::max (0.f, rr - 1.f);
        if (target > cv) cv += aC * (target - cv);
        else
        {
            const float floor = std::max (slow1 ? cs1 : 0.f, slow2 ? cs2 : 0.f);
            if (cv > floor) cv += rF * (floor - cv);
        }
        if (slow1) cs1 += (cv > cs1 ? chg1 : dec1) * ((cv > cs1 ? cv : 0.f) - cs1);
        if (slow2) cs2 += (cv > cs2 ? chg2 : dec2) * ((cv > cs2 ? cv : 0.f) - cs2);
        l = y[0] * outG; r = y[1] * outG;
        return -todB (2.f * sigm (-cv * I));
    }
};

// ================================================================ FET (type 1176LN)
struct Fet
{
    Iron tout[2];
    float fs = 88200.f, inG = 1.f, outG = 1.f, I = 0.f, ratio = 4.f, thr = -22.f, aC = 0.1f, rC = 0.001f, relMs = 500.f;
    float d2 = 0.f, kPre = 0.f, kLine = 0.f, gr = 0.f;
    bool all = false;

    void prepare (float sr) { fs = sr; for (auto& t : tout) t.prepare (fs, 110.f); gr = 0.f; }

    // atk / rel : echelle 1..7 du panneau (7 = le plus rapide), ratioIdx : 4, 8, 12, 20, ALL
    void set (float inDb, float outDb, float atk, float rel, int ratioIdx, float intensity)
    {
        I = intensity;
        static const float ratios[5] { 4.f, 8.f, 12.f, 20.f, 20.f }, thrs[5] { -22.f, -19.f, -17.f, -15.f, -20.f };
        ratioIdx = std::max (0, std::min (4, ratioIdx));
        all = ratioIdx == 4;
        ratio = ratios[ratioIdx]; thr = thrs[ratioIdx];
        inG = fromdB (inDb * I); outG = fromdB (outDb * I);
        const float a = std::max (0.f, std::min (1.f, (atk - 1.f) / 6.f)), r = std::max (0.f, std::min (1.f, (rel - 1.f) / 6.f));
        const float atkMs = 0.8f * std::pow (0.02f / 0.8f, a);              // 800 us -> 20 us
        relMs = 1100.f * std::pow (50.f / 1100.f, r);                       // 1100 ms -> 50 ms
        aC = coefMs (atkMs, fs);
        d2 = 0.12f * I * (all ? 3.f : 1.f);
        kPre = 0.55f * I * (all ? 1.4f : 1.f);
        kLine = 0.45f * I;
        for (auto& t : tout) t.drive = 0.3f * I;
    }

    float process (float& l, float& r)
    {
        float x[2] { l * inG, r * inG };
        // detection (courbe equivalente a la boucle de contre-reaction d'origine)
        const float lvl = todB (std::max (std::fabs (x[0]), std::fabs (x[1])));
        const float over = lvl - thr;
        float target = grCurve (over, ratio, 4.f);
        if (all && over > 0.f) target = std::min (over * 0.95f + 0.045f * over * over, over * 1.35f);   // ALL : la courbe s'inverse
        const float rel = relMs * (1.f + gr / 15.f) * (all ? 1.3f : 1.f);  // recuperation qui ralentit quand ca comprime fort
        if (target > gr) gr += aC * (target - gr);
        else gr += coefMs (rel * 0.35f, fs) * (target - gr);
        const float g = fromdB (-gr * I);
        for (int c = 0; c < 2; ++c)
        {
            float v = x[c] * g;
            v += d2 * (1.f - g) * v * v;                                     // le FET ajoute de la 2e harmonique quand il travaille
            v = classA (v, kPre, 0.18f);                                     // preampli
            v *= outG;                                                       // potard OUTPUT (avant l'ampli de ligne)
            v = classA (v, kLine, 0.12f);                                    // ampli de ligne classe A
            x[c] = tout[c].run (v);
        }
        l = x[0]; r = x[1];
        return gr * I;
    }
};

// ================================================================ DIODE (type 33609 : pont de diodes)
struct Diode
{
    Iron tin[2], tout[2];
    float fs = 88200.f, color = 0.f, grScale = 1.f, makeG = 1.f;
    bool comp = false, lim = false;
    float cThr = -18.f, cRatio = 2.f, cA = 0.01f, cRelMs = 400.f, cAutoFast = 0.f;
    float lThr = -10.f, lA = 0.1f, lRelMs = 100.f, lAutoFast = 0.f;
    float grC = 0.f, flC = 0.f, grL = 0.f, flL = 0.f;

    void prepare (float sr)
    {
        fs = sr;
        for (int c = 0; c < 2; ++c) { tin[c].prepare (fs, 130.f); tout[c].prepare (fs, 150.f); }
        grC = flC = grL = flL = 0.f;
    }

    // niveaux en dBu (0 dBFS = +18 dBu) ; recIdx : 4 et 5 = AUTO 1 / AUTO 2
    void setComp (bool on, float thrDbu, int ratioIdx, int recIdx, float makeupDb, float intensity)
    {
        comp = on && intensity > 0.f;
        static const float ratios[5] { 1.5f, 2.f, 3.f, 4.f, 6.f }, rec[6] { 100.f, 400.f, 800.f, 1500.f, 800.f, 1500.f };
        cRatio = ratios[std::max (0, std::min (4, ratioIdx))];
        recIdx = std::max (0, std::min (5, recIdx));
        cThr = thrDbu - 18.f;
        cA = coefMs (5.6f, fs);                                             // R31 x C13
        cRelMs = rec[recIdx]; cAutoFast = recIdx >= 4 ? 1.f : 0.f;
        grScale = intensity;
        color = intensity;
        makeG = fromdB (makeupDb * intensity);
        for (int c = 0; c < 2; ++c) { tin[c].drive = 0.3f * color; tout[c].drive = 0.4f * color; }
    }
    void setLimit (bool on, float thrDbu, int recIdx, bool fast)
    {
        lim = on;
        static const float rec[6] { 50.f, 100.f, 200.f, 800.f, 1500.f, 3000.f };
        recIdx = std::max (0, std::min (5, recIdx));
        lThr = thrDbu - 18.f;
        lA = coefMs (fast ? 0.9f : 1.7f, fs);                              // 2 ms / 4 ms (a 1 dB pres)
        lRelMs = rec[recIdx]; lAutoFast = recIdx >= 4 ? 1.f : 0.f;
    }
    void limiterOnlyColour() { color = 1.f; grScale = 1.f; makeG = 1.f; for (int c = 0; c < 2; ++c) { tin[c].drive = 0.3f; tout[c].drive = 0.4f; } }

    // recuperation : simple, ou AUTO (rapide sur les crêtes isolees, lente quand la compression dure)
    void release (float& gr, float& fl, float target, float relMs, bool autoRel)
    {
        if (autoRel)
        {
            if (gr > fl) gr += coefMs (relMs / 7.f, fs) * (std::max (target, fl) - gr);
            fl += (gr > fl ? coefMs (700.f, fs) * (gr - fl) : coefMs (relMs * 0.45f, fs) * (0.f - fl));
            if (gr < fl) gr = fl;
        }
        else gr += coefMs (relMs * 0.33f, fs) * (target - gr);
    }

    float process (float& l, float& r, float& limGrOut)
    {
        float x[2] { tin[0].run (l), tin[1].run (r) };
        const float lvl = todB (std::max (std::fabs (x[0]), std::fabs (x[1])));
        // chaine de diodes du compresseur : genou large ; limiteur ~100:1
        if (comp)
        {
            const float t = grCurve (lvl - cThr, cRatio, 8.f);
            if (t > grC) grC += cA * (t - grC); else release (grC, flC, t, cRelMs, cAutoFast > 0.5f);
        }
        else grC = flC = 0.f;
        if (lim)
        {
            const float lvlOut = lvl - grC * grScale + todB (makeG);       // le limiteur ecoute la sortie de l'ampli
            const float t = grCurve (lvlOut - lThr, 100.f, 1.f);
            if (t > grL) grL += lA * (t - grL); else release (grL, flL, t, lRelMs, lAutoFast > 0.5f);
        }
        else grL = flL = 0.f;
        const float gr = std::max (grC * grScale, grL);  // le plus grand des deux pilote le pont
        const float g = fromdB (-gr);
        const float a = 0.25f * (1.f - g) * std::max (color, lim ? 1.f : 0.f);
        for (int c = 0; c < 2; ++c)
        {
            float v = g * ((1.f - a) * x[c] + a * std::tanh (1.5f * x[c]) / 1.5f);   // les diodes polarisees courbent le signal
            v *= makeG;
            v = classA (v, 0.35f * color, 0.1f);                             // ampli classe A BA640
            x[c] = tout[c].run (v);
        }
        l = x[0]; r = x[1];
        limGrOut = grL;
        return gr;
    }
};
} // namespace mdl
} // namespace tn
