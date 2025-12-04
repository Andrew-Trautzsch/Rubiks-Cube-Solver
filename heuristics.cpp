#include "cube.hpp"
#include <algorithm>
#include <vector>
#include <limits>
#include <functional>
#include <cstdint>

// ----- Heuristic Data Structures -----

struct StickerPos 
{
    Face f;
    int r;
    int c;
};

struct CornerSlot 
{
    StickerPos s[3];
};

struct EdgeSlot 
{
    StickerPos s[2];
};

// Corner slots: each is a fixed *position* in the cube
// 0: UFR, 1: UFL, 2: UBL, 3: UBR,
// 4: DFR, 5: DFL, 6: DBL, 7: DBR
static const CornerSlot kCornerSlots[8] =
{
    { { {Up,   2, 2}, {Front, 0, 2}, {Right, 0, 0} } }, // 0: UFR
    { { {Up,   2, 0}, {Front, 0, 0}, {Left,  0, 2} } }, // 1: UFL
    { { {Up,   0, 0}, {Back,  0, 2}, {Left,  0, 0} } }, // 2: UBL
    { { {Up,   0, 2}, {Back,  0, 0}, {Right, 0, 2} } }, // 3: UBR
    { { {Down, 0, 2}, {Front, 2, 2}, {Right, 2, 0} } }, // 4: DFR
    { { {Down, 0, 0}, {Front, 2, 0}, {Left,  2, 2} } }, // 5: DFL
    { { {Down, 2, 0}, {Back,  2, 2}, {Left,  2, 0} } }, // 6: DBL
    { { {Down, 2, 2}, {Back,  2, 0}, {Right, 2, 2} } }  // 7: DBR
};

// Edge slots: 12 fixed positions
// 0: UF, 1: UR, 2: UB, 3: UL, 4: DF, 5: DR, 6: DB, 7: DL,
// 8: FR, 9: FL, 10: BR, 11: BL
static const EdgeSlot kEdgeSlots[12] =
{
    { { {Up,   2, 1}, {Front, 0, 1} } }, // 0: UF
    { { {Up,   1, 2}, {Right, 0, 1} } }, // 1: UR
    { { {Up,   0, 1}, {Back,  0, 1} } }, // 2: UB
    { { {Up,   1, 0}, {Left,  0, 1} } }, // 3: UL
    { { {Down, 0, 1}, {Front, 2, 1} } }, // 4: DF
    { { {Down, 1, 2}, {Right, 2, 1} } }, // 5: DR
    { { {Down, 2, 1}, {Back,  2, 1} } }, // 6: DB
    { { {Down, 1, 0}, {Left,  2, 1} } }, // 7: DL
    { { {Front, 1, 2}, {Right, 1, 0} } }, // 8: FR
    { { {Front, 1, 0}, {Left,  1, 2} } }, // 9: FL
    { { {Back,  1, 0}, {Right, 1, 2} } }, // 10: BR
    { { {Back,  1, 2}, {Left,  1, 0} } }  // 11: BL
};

// ----- RubiksCube Heuristic Implementation -----

int RubiksCube::cubieHeuristic() const 
{
    using std::array;
    using std::sort;

    static bool initialized = false;
    static Color solvedCornerColors[8][3];
    static Color solvedEdgeColors[12][2];

    if (!initialized) {
        RubiksCube solved; // default ctor = solved cube

        // Record colors in solved state
        for (int i = 0; i < 8; ++i) {
            for (int k = 0; k < 3; ++k) {
                const StickerPos &sp = kCornerSlots[i].s[k];
                solvedCornerColors[i][k] = solved.faces_[sp.f].squares[sp.r][sp.c];
            }
        }
        for (int i = 0; i < 12; ++i) {
            for (int k = 0; k < 2; ++k) {
                const StickerPos &sp = kEdgeSlots[i].s[k];
                solvedEdgeColors[i][k] = solved.faces_[sp.f].squares[sp.r][sp.c];
            }
        }
        initialized = true;
    }

    int misplacedCorners     = 0;
    int misorientedCorners   = 0;
    int misplacedEdges       = 0;
    int misorientedEdges     = 0;

    // Corners
    for (int i = 0; i < 8; ++i) {
        Color cur[3];
        for (int k = 0; k < 3; ++k) {
            const StickerPos &sp = kCornerSlots[i].s[k];
            cur[k] = faces_[sp.f].squares[sp.r][sp.c];
        }

        array<int,3> setSolved{}, setCur{};
        for (int k = 0; k < 3; ++k) {
            setSolved[k] = static_cast<int>(solvedCornerColors[i][k]);
            setCur[k]    = static_cast<int>(cur[k]);
        }
        sort(setSolved.begin(), setSolved.end());
        sort(setCur.begin(), setCur.end());

        if (setSolved != setCur) {
            ++misplacedCorners;
        } else {
            bool sameOrder = true;
            for (int k = 0; k < 3; ++k) {
                if (cur[k] != solvedCornerColors[i][k]) {
                    sameOrder = false;
                    break;
                }
            }
            if (!sameOrder) ++misorientedCorners;
        }
    }

    // Edges
    for (int i = 0; i < 12; ++i) {
        Color cur[2];
        for (int k = 0; k < 2; ++k) {
            const StickerPos &sp = kEdgeSlots[i].s[k];
            cur[k] = faces_[sp.f].squares[sp.r][sp.c];
        }

        int s0 = static_cast<int>(solvedEdgeColors[i][0]);
        int s1 = static_cast<int>(solvedEdgeColors[i][1]);
        int c0 = static_cast<int>(cur[0]);
        int c1 = static_cast<int>(cur[1]);

        bool setMatch = ((s0 == c0 && s1 == c1) || (s0 == c1 && s1 == c0));

        if (!setMatch) {
            ++misplacedEdges;
        } else {
            bool sameOrder = (s0 == c0 && s1 == c1);
            if (!sameOrder) ++misorientedEdges;
        }
    }

    auto ceil_div = [](int x, int d) {
        return (x + d - 1) / d;
    };

    int h_pos_corners = ceil_div(misplacedCorners,     4);
    int h_pos_edges   = ceil_div(misplacedEdges,       4);
    int h_ori_corners = ceil_div(misorientedCorners,   4);
    int h_ori_edges   = ceil_div(misorientedEdges,     4);

    int h = std::max(
        std::max(h_pos_corners, h_pos_edges),
        std::max(h_ori_corners, h_ori_edges)
    );

    if (h < 0) h = 0;
    return h;
}

// ----- IDA* Solver -----

vector<Move> RubiksCube::solveIDAStar(int maxIterations, int iterationDepth) const
{
    if (isSolved()) return {};

    const int INF = std::numeric_limits<int>::max();
    const int maxDepth = iterationDepth;

    RubiksCube start = *this;

    static const Move allMoves[] = {
        {Up,    CW}, {Up,    CCW}, {Up,    Double},
        {Down,  CW}, {Down,  CCW}, {Down,  Double},
        {Left,  CW}, {Left,  CCW}, {Left,  Double},
        {Right, CW}, {Right, CCW}, {Right, Double},
        {Front, CW}, {Front, CCW}, {Front, Double},
        {Back,  CW}, {Back,  CCW}, {Back,  Double}
    };

    auto isInverse = [](const Move& a, const Move& b) -> bool
    {
        if (a.face != b.face) return false;
        if (a.turn == Double && b.turn == Double) return true;
        if ((a.turn == CW && b.turn == CCW) || (a.turn == CCW && b.turn == CW))
            return true;
        return false;
    };

    auto inverseOf = [](const Move& m) -> Move
    {
        Move inv = m;
        if (inv.turn == CW)       inv.turn = CCW;
        else if (inv.turn == CCW) inv.turn = CW;
        return inv;
    };

    std::vector<Move> path;
    int threshold = start.heuristic();

    // Recursive DFS
    std::function<int(RubiksCube&, int, int, const Move&)> dfs;
    dfs = [&](RubiksCube& cube, int g, int curThreshold, const Move& prevMove) -> int
    {
        int h = cube.heuristic();
        int f = g + h;

        if (f > curThreshold) return f;
        if (cube.isSolved()) return -1;
        if (g >= maxDepth) return INF;

        int minNext = INF;

        for (const Move& m : allMoves)
        {
            if (prevMove.face != Face::Count && m.face == prevMove.face) continue;
            if (prevMove.face != Face::Count && isInverse(prevMove, m)) continue;

            cube.applyMove(m.face, m.turn);
            path.push_back(m);

            int t = dfs(cube, g + 1, curThreshold, m);

            if (t == -1) return -1;
            if (t < minNext) minNext = t;

            path.pop_back();
            Move inv = inverseOf(m);
            cube.applyMove(inv.face, inv.turn);
        }
        return minNext;
    };

    int iteration = 0;
    while (true)
    {
        path.clear();
        RubiksCube work = start;
        Move dummyPrev{ Face::Count, CW };

        int t = dfs(work, 0, threshold, dummyPrev);

        if (t == -1) return path;
        if (t == INF) return {};

        threshold = t;
        ++iteration;

        if (maxIterations > 0 && iteration >= maxIterations) return {};
    }
}