#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// QrCode.h  —  single-header QR code generator, C++17, zero dependencies
//
// Supports alphanumeric mode (digits, A-Z, space, $%*+-./:)
// Covers all IPv4:Port strings (e.g. "192.168.1.5:5000")
// ECL M is used; version is auto-selected (1-10).
//
// Usage:
//   auto matrix = QrGen::encode("192.168.1.5:5000");
//   int sz = (int)matrix.size();           // matrix is sz × sz
//   bool dark = matrix[row][col];          // true = dark module
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <climits>

namespace QrGen {
namespace detail {

// ── GF(256), primitive polynomial x^8+x^4+x^3+x^2+1 ─────────────────────────
static uint8_t GFE[512], GFL[256];
static bool gfReady = false;
static void gfInit() {
    if (gfReady) return; gfReady = true;
    uint16_t x = 1;
    for (int i = 0; i < 255; i++) {
        GFE[i] = (uint8_t)x;
        GFL[x]  = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11D;
    }
    for (int i = 255; i < 512; i++) GFE[i] = GFE[i - 255];
    GFL[0] = 0;
}
static uint8_t gfMul(uint8_t a, uint8_t b) {
    if (!a || !b) return 0;
    return GFE[(int)GFL[a] + GFL[b]];
}

// ── Reed-Solomon EC codewords ─────────────────────────────────────────────────
// Returns `ecDeg` EC bytes for the given data bytes.
static std::vector<uint8_t> rsEc(const std::vector<uint8_t>& data, int ecDeg) {
    gfInit();
    // Build monic generator polynomial: prod_{i=0}^{ecDeg-1}(x + alpha^i)
    // g[0] = leading coefficient (= 1), g[ecDeg] = constant term
    std::vector<uint8_t> g = {1};
    for (int i = 0; i < ecDeg; i++) {
        std::vector<uint8_t> ng(g.size() + 1, 0);
        for (int j = 0; j < (int)g.size(); j++) {
            ng[j]     ^= g[j];
            ng[j + 1] ^= gfMul(g[j], GFE[i]);
        }
        g = ng;
    }
    // Polynomial long-division: (data * x^ecDeg) mod g
    std::vector<uint8_t> msg(data.begin(), data.end());
    msg.resize(msg.size() + ecDeg, 0);
    for (int i = 0; i < (int)data.size(); i++) {
        if (msg[i]) {
            for (int j = 1; j <= ecDeg; j++)
                msg[i + j] ^= gfMul(g[j], msg[i]);
        }
    }
    return {msg.begin() + (int)data.size(), msg.end()};
}

// ── Alphanumeric character values ─────────────────────────────────────────────
static int charVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a'); // tolerate lowercase
    const char* ex = " $%*+-./:";
    const int   ev[] = {36, 37, 38, 39, 40, 41, 42, 43, 44};
    for (int i = 0; ex[i]; i++) if (c == ex[i]) return ev[i];
    throw std::invalid_argument(std::string("QR: unsupported char '") + c + "'");
}

// ── QR parameters for ECL M (error correction level M = 00 binary) ───────────
// Max alphanumeric chars per version
static const int ALPHA_MAX[11] = {0, 20, 38, 70, 101, 131, 163, 211, 259, 302, 360};

struct EcInfo { int ecPerBlock, blocks1, data1, blocks2, data2; };
static const EcInfo EC_M[11] = {
    {},
    {10, 1, 16, 0,  0},  // V1:  16 data, 10 EC
    {16, 1, 28, 0,  0},  // V2:  28 data, 16 EC
    {26, 1, 44, 0,  0},  // V3:  44 data, 26 EC
    {18, 2, 32, 0,  0},  // V4:  64 data, 36 EC
    {24, 2, 43, 0,  0},  // V5:  86 data, 48 EC
    {16, 4, 27, 0,  0},  // V6: 108 data, 64 EC
    {18, 4, 31, 0,  0},  // V7: 124 data, 72 EC
    {22, 2, 38, 2, 39},  // V8: 154 data, 88 EC
    {22, 3, 36, 2, 37},  // V9: 182 data, 110 EC
    {26, 4, 43, 1, 44},  // V10:216 data, 130 EC
};

// Alignment pattern center-coordinate lists per version (0-terminated)
static const int AP[][8] = {
    {0},{0},{6,18,0},{6,22,0},{6,26,0},{6,30,0},{6,34,0},
    {6,22,38,0},{6,24,42,0},{6,26,46,0},{6,28,50,0}
};

// Format information (15 bits) for ECL M (=00) and mask patterns 0-7.
// = BCH(ecl_bits:mask_bits) XOR 101010000010010
static const uint16_t FMT_M[8] = {
    0x5412, 0x5125, 0x5E7C, 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0
};

// ── Matrix types ──────────────────────────────────────────────────────────────
using Mat  = std::vector<std::vector<int8_t>>; // -1=data, 0=light, 1=dark
using Func = std::vector<std::vector<bool>>;   // true = function (not maskable)

// ── Function pattern drawing ──────────────────────────────────────────────────
static void setMod(Mat& m, Func& f, int r, int c, bool dark) {
    int n = (int)m.size();
    if (r < 0 || r >= n || c < 0 || c >= n) return;
    m[r][c]   = dark ? 1 : 0;
    f[r][c]   = true;
}

// 7×7 finder pattern + 1-module separator at (dr, dc)
static void drawFinder(Mat& m, Func& f, int dr, int dc) {
    int n = (int)m.size();
    for (int r = -1; r <= 7; r++)
    for (int c = -1; c <= 7; c++) {
        int rr = dr + r, cc = dc + c;
        if (rr < 0 || rr >= n || cc < 0 || cc >= n) continue;
        bool dark = (r >= 0 && r <= 6 && c >= 0 && c <= 6) &&
                    (r == 0 || r == 6 || c == 0 || c == 6 ||
                     (r >= 2 && r <= 4 && c >= 2 && c <= 4));
        setMod(m, f, rr, cc, dark);
    }
}

// 5×5 alignment pattern centred at (cr, cc), skip if already function module
static void drawAlign(Mat& m, Func& f, int cr, int cc) {
    for (int r = -2; r <= 2; r++)
    for (int c = -2; c <= 2; c++) {
        if (f[cr + r][cc + c]) continue;
        bool dark = (r == -2 || r == 2 || c == -2 || c == 2 || (r == 0 && c == 0));
        setMod(m, f, cr + r, cc + c, dark);
    }
}

// Place format information bits in both canonical positions (no func update)
static void placeFmt(Mat& m, int n, uint16_t fmt) {
    static const int R1[] = {0,1,2,3,4,5,7,8, 8,8,8,8,8,8,8};
    static const int C1[] = {8,8,8,8,8,8,8,8, 7,5,4,3,2,1,0};
    for (int i = 0; i < 15; i++) m[R1[i]][C1[i]] = (fmt >> (14 - i)) & 1;
    // Copy 2 — top-right area (bits 0-7) and bottom-left area (bits 8-14)
    for (int i = 0; i < 8;  i++) m[8][n - 1 - i]  = (fmt >>  i) & 1;
    for (int i = 8; i < 15; i++) m[n - 15 + i][8]  = (fmt >>  i) & 1;
    m[n - 8][8] = 1; // dark module (always dark)
}

// ── Masking and penalty ───────────────────────────────────────────────────────
static bool maskCondition(int mask, int r, int c) {
    switch (mask) {
        case 0: return (r + c) % 2 == 0;
        case 1: return  r % 2 == 0;
        case 2: return  c % 3 == 0;
        case 3: return (r + c) % 3 == 0;
        case 4: return (r / 2 + c / 3) % 2 == 0;
        case 5: return (r * c) % 2 + (r * c) % 3 == 0;
        case 6: return ((r * c) % 2 + (r * c) % 3) % 2 == 0;
        case 7: return ((r + c) % 2 + (r * c) % 3) % 2 == 0;
    }
    return false;
}

static int penaltyScore(const Mat& m) {
    int n = (int)m.size(), score = 0;

    // Rule 1: 5+ consecutive same-colour in rows/cols
    for (int r = 0; r < n; r++) {
        int run = 1;
        for (int c = 1; c < n; c++) {
            if (m[r][c] == m[r][c - 1]) { if (++run == 5) score += 3; else if (run > 5) score++; }
            else run = 1;
        }
    }
    for (int c = 0; c < n; c++) {
        int run = 1;
        for (int r = 1; r < n; r++) {
            if (m[r][c] == m[r - 1][c]) { if (++run == 5) score += 3; else if (run > 5) score++; }
            else run = 1;
        }
    }

    // Rule 2: 2×2 blocks
    for (int r = 0; r < n - 1; r++)
    for (int c = 0; c < n - 1; c++) {
        int v = m[r][c];
        if (v == m[r][c+1] && v == m[r+1][c] && v == m[r+1][c+1]) score += 3;
    }

    // Rule 3: finder-like patterns (1:1:3:1:1 in row or column)
    static const int PAT1[] = {1,0,1,1,1,0,1,0,0,0,0};
    static const int PAT2[] = {0,0,0,0,1,0,1,1,1,0,1};
    auto checkRow = [&](int r, int c) {
        bool a = true, b = true;
        for (int k = 0; k < 11; k++) {
            a = a && (m[r][c+k] == PAT1[k]);
            b = b && (m[r][c+k] == PAT2[k]);
        }
        return (a ? 40 : 0) + (b ? 40 : 0);
    };
    auto checkCol = [&](int r, int c) {
        bool a = true, b = true;
        for (int k = 0; k < 11; k++) {
            a = a && (m[r+k][c] == PAT1[k]);
            b = b && (m[r+k][c] == PAT2[k]);
        }
        return (a ? 40 : 0) + (b ? 40 : 0);
    };
    for (int r = 0; r < n;     r++) for (int c = 0; c + 11 <= n; c++) score += checkRow(r, c);
    for (int c = 0; c < n;     c++) for (int r = 0; r + 11 <= n; r++) score += checkCol(r, c);

    // Rule 4: proportion of dark modules
    int dark = 0;
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++) if (m[r][c]) dark++;
    int pct = dark * 100 / (n * n);
    score += (std::abs(pct - 50) / 5) * 10;

    return score;
}

} // namespace detail

// ── Public API ────────────────────────────────────────────────────────────────
// Encodes `text` as a QR code (ECL M) and returns the dark-module bit-matrix.
// Characters must be in the QR alphanumeric set: 0-9 A-Z (or a-z) space $%*+-./:
inline std::vector<std::vector<bool>> encode(const std::string& text) {
    using namespace detail;

    // 1. Select minimum version (ECL M)
    int ver = 0;
    for (int v = 1; v <= 10; v++) {
        if ((int)text.size() <= ALPHA_MAX[v]) { ver = v; break; }
    }
    if (!ver) throw std::invalid_argument("QR: text too long (max 360 chars at V10-M)");

    const auto& ec = EC_M[ver];
    int n = 4 * ver + 17; // matrix size

    // 2. Build alphanumeric bit stream
    std::vector<int> vals;
    vals.reserve(text.size());
    for (char c : text) vals.push_back(charVal(c));
    int len = (int)vals.size();

    // Total data bytes
    int totalData = ec.blocks1 * ec.data1 + ec.blocks2 * ec.data2;

    std::vector<uint8_t> rawBits(totalData, 0);
    int bitPos = 0;
    auto push = [&](uint32_t v, int nb) {
        for (int i = nb - 1; i >= 0; i--) {
            if ((v >> i) & 1) rawBits[bitPos / 8] |= (uint8_t)(1 << (7 - bitPos % 8));
            bitPos++;
        }
    };

    push(0b0010, 4);           // mode: alphanumeric
    push((uint32_t)len, 9);    // character count (9 bits for V1-V9)
    for (int i = 0; i + 1 < len; i += 2) push((uint32_t)(vals[i] * 45 + vals[i+1]), 11);
    if (len % 2) push((uint32_t)vals[len - 1], 6);
    push(0, std::min(4, totalData * 8 - bitPos)); // terminator
    while (bitPos % 8) push(0, 1);               // byte-align
    for (int i = bitPos / 8; i < totalData; i++) push(i % 2 == 0 ? 0xEC : 0x11, 8); // pad

    // 3. Reed-Solomon error correction + interleaving
    std::vector<std::vector<uint8_t>> dBlocks, ecBlocks;
    int offset = 0;
    for (int b = 0; b < ec.blocks1; b++, offset += ec.data1) {
        std::vector<uint8_t> blk(rawBits.begin() + offset, rawBits.begin() + offset + ec.data1);
        dBlocks.push_back(blk);
        ecBlocks.push_back(rsEc(blk, ec.ecPerBlock));
    }
    for (int b = 0; b < ec.blocks2; b++, offset += ec.data2) {
        std::vector<uint8_t> blk(rawBits.begin() + offset, rawBits.begin() + offset + ec.data2);
        dBlocks.push_back(blk);
        ecBlocks.push_back(rsEc(blk, ec.ecPerBlock));
    }
    int maxData = (ec.blocks2 > 0) ? ec.data2 : ec.data1;
    std::vector<uint8_t> interleaved;
    for (int i = 0; i < maxData; i++)
        for (auto& blk : dBlocks) if (i < (int)blk.size()) interleaved.push_back(blk[i]);
    for (int i = 0; i < ec.ecPerBlock; i++)
        for (auto& blk : ecBlocks) interleaved.push_back(blk[i]);

    // 4. Initialise matrix and function-pattern mask
    Mat  mat(n, std::vector<int8_t>(n, -1));
    Func func(n, std::vector<bool>(n, false));

    // Finder patterns
    drawFinder(mat, func, 0,     0);
    drawFinder(mat, func, 0,     n - 7);
    drawFinder(mat, func, n - 7, 0);

    // Timing patterns
    for (int i = 8; i <= n - 9; i++) {
        if (!func[6][i]) { mat[6][i] = (i % 2 == 0) ? 1 : 0; func[6][i] = true; }
        if (!func[i][6]) { mat[i][6] = (i % 2 == 0) ? 1 : 0; func[i][6] = true; }
    }

    // Alignment patterns
    const int* apc = AP[ver];
    int apLen = 0; while (apc[apLen]) apLen++;
    for (int ai = 0; ai < apLen; ai++)
    for (int aj = 0; aj < apLen; aj++) {
        int r = apc[ai], c = apc[aj];
        if (!func[r][c]) drawAlign(mat, func, r, c);
    }

    // Reserve format info positions (values set later per mask)
    static const int FR1[] = {0,1,2,3,4,5,7,8, 8,8,8,8,8,8,8};
    static const int FC1[] = {8,8,8,8,8,8,8,8, 7,5,4,3,2,1,0};
    for (int i = 0; i < 15; i++) { func[FR1[i]][FC1[i]] = true; mat[FR1[i]][FC1[i]] = 0; }
    for (int i = 0; i < 8;  i++) { func[8][n-1-i] = true; mat[8][n-1-i] = 0; }
    for (int i = 8; i < 15; i++) { func[n-15+i][8] = true; mat[n-15+i][8] = 0; }
    func[n-8][8] = true; mat[n-8][8] = 1; // dark module

    // 5. Place data bits (zigzag scan)
    {
        int bit = 0, total = (int)interleaved.size() * 8;
        bool goUp = true;
        for (int right = n - 1; right >= 1; right -= 2) {
            if (right == 6) right--; // skip timing column
            for (int v = 0; v < n; v++) {
                int row = goUp ? (n - 1 - v) : v;
                for (int j = 0; j < 2; j++) {
                    int col = right - j;
                    if (!func[row][col]) {
                        bool b = (bit < total) && ((interleaved[bit / 8] >> (7 - bit % 8)) & 1);
                        mat[row][col] = b ? 1 : 0;
                        bit++;
                    }
                }
            }
            goUp = !goUp;
        }
    }

    // 6. Try all 8 masks; pick lowest-penalty
    Mat  bestMat  = mat;
    int  bestPen  = INT_MAX;
    int  bestMask = 0;

    for (int mask = 0; mask < 8; mask++) {
        Mat candidate = mat;
        for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++) {
            if (!func[r][c] && maskCondition(mask, r, c))
                candidate[r][c] ^= 1;
        }
        placeFmt(candidate, n, FMT_M[mask]);

        int pen = penaltyScore(candidate);
        if (pen < bestPen) { bestPen = pen; bestMask = mask; bestMat = candidate; }
    }
    (void)bestMask;

    // 7. Convert to bool matrix
    std::vector<std::vector<bool>> result(n, std::vector<bool>(n, false));
    for (int r = 0; r < n; r++)
    for (int c = 0; c < n; c++)
        result[r][c] = (bestMat[r][c] == 1);

    return result;
}

} // namespace QrGen
