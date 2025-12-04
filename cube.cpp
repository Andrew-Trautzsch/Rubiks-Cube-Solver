#include "cube.hpp"

#include <random>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <array>
#include <cstdint>
#include <limits>
#include <functional>

///// RUBIKSCUBE IMPLEMENTATION /////

// Shared move list for both A* and IDA*
namespace {
    const Move kAllMoves[] = {
        {Up,    CW}, {Up,    CCW}, {Up,    Double},
        {Down,  CW}, {Down,  CCW}, {Down,  Double},
        {Left,  CW}, {Left,  CCW}, {Left,  Double},
        {Right, CW}, {Right, CCW}, {Right, Double},
        {Front, CW}, {Front, CCW}, {Front, Double},
        {Back,  CW}, {Back,  CCW}, {Back,  Double}
    };

    inline bool isInverseMove(const Move& a, const Move& b)
    {
        if (a.face != b.face) return false;
        if (a.turn == Double && b.turn == Double) return true;
        if ((a.turn == CW && b.turn == CCW) || (a.turn == CCW && b.turn == CW))
            return true;
        return false;
    }

    inline Move inverseOf(const Move& m)
    {
        Move inv = m;
        if (inv.turn == CW)      inv.turn = CCW;
        else if (inv.turn == CCW) inv.turn = CW;
        // Double stays Double
        return inv;
    }
}

// solved cube constructor
RubiksCube::RubiksCube()
{
    // You can choose any consistent color mapping.
    // Example:
    // Up    -> White
    // Down  -> Yellow
    // Left  -> Orange
    // Right -> Red
    // Front -> Green
    // Back  -> Blue

    faces_[Up]    = Side(Color::White);
    faces_[Down]  = Side(Color::Yellow);
    faces_[Left]  = Side(Color::Orange);
    faces_[Right] = Side(Color::Red);
    faces_[Front] = Side(Color::Green);
    faces_[Back]  = Side(Color::Blue);
}

// manual-input constructor
RubiksCube::RubiksCube(const std::array<Side, FACE_COUNT>& faces)
    : faces_(faces)
{
}

// randomizes cube with a series of random moves
void RubiksCube::scramble(int moveCount)
{
    // random engine (you might want to seed from std::random_device)
    static std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> faceDist(0, Count - 1);
    std::uniform_int_distribution<int> turnDist(0, 2); // CW, CCW, Double

    for (int i = 0; i < moveCount; ++i)
    {
        Face f = static_cast<Face>(faceDist(rng));
        Turn t = static_cast<Turn>(turnDist(rng));
        applyMove(f, t);
    }
}

// core move dispatcher
void RubiksCube::applyMove(Face f, Turn t)
{
    switch (f)
    {
    case Up:
        rotateTop(t);
        break;
    case Down:
        rotateBottom(t);
        break;
    case Left:
        rotateLeft(t);
        break;
    case Right:
        rotateRight(t);
        break;
    case Front:
        rotateFront(t);
        break;
    case Back:
        rotateBack(t);
        break;
    default:
        break;
    }
}

// check if each face is a solid color
bool RubiksCube::isSolved() const
{
    for (int fi = 0; fi < FACE_COUNT; ++fi)
    {
        const Side& s = faces_[fi];
        Color center = s.getCenter();

        for (int r = 0; r < Side::SIZE; ++r)
        {
            for (int c = 0; c < Side::SIZE; ++c)
            {
                if (s.squares[r][c] != center)
                    return false;
            }
        }
    }
    return true;
}

// equality: all faces must match
bool RubiksCube::operator==(const RubiksCube& other) const
{
    for (int fi = 0; fi < FACE_COUNT; ++fi)
    {
        const Side& a = faces_[fi];
        const Side& b = other.faces_[fi];

        for (int r = 0; r < Side::SIZE; ++r)
        {
            for (int c = 0; c < Side::SIZE; ++c)
            {
                if (a.squares[r][c] != b.squares[r][c])
                    return false;
            }
        }
    }
    return true;
}

///// HEURISTICS /////

std::size_t RubiksCube::hash() const
{
    // 64-bit FNV-1a over all 54 stickers
    std::size_t h = 1469598103934665603ull;

    for (int f = 0; f < FACE_COUNT; ++f)
    {
        for (int r = 0; r < Side::SIZE; ++r)
        {
            for (int c = 0; c < Side::SIZE; ++c)
            {
                std::uint8_t v = static_cast<std::uint8_t>(faces_[f].squares[r][c]);
                h ^= v;
                h *= 1099511628211ull;
            }
        }
    }

    return h;
}

struct RubiksCubeHasher
{
    std::size_t operator()(const RubiksCube& cube) const noexcept
    {
        return cube.hash();
    }
};

vector<Move> RubiksCube::solveAStar(int maxDepth, int maxNodes) const
{
    struct Node
    {
        RubiksCube state;
        int g;                 // cost so far (depth)
        int h;                 // heuristic
        int parent;            // index in nodes vector, -1 for root
        Move moveFromParent;   // move used to reach this node
    };

    struct QueueItem
    {
        int index;
        int f; // g + h
    };

    struct QueueCompare
    {
        bool operator()(const QueueItem& a, const QueueItem& b) const
        {
            return a.f > b.f;  // min-heap behavior for priority_queue
        }
    };

    // If already solved, nothing to do
    if (isSolved())
        return {};

    

    std::vector<Node> nodes;
    nodes.reserve(maxNodes);

    std::priority_queue<QueueItem, std::vector<QueueItem>, QueueCompare> open;
    std::unordered_map<RubiksCube, int, RubiksCubeHasher> bestG;

    // Root node
    Node root;
    root.state = *this;
    root.g = 0;
    root.h = root.state.heuristic();
    root.parent = -1;
    root.moveFromParent = Move{Front, CW}; // dummy, never used for root

    nodes.push_back(root);
    open.push(QueueItem{0, root.g + root.h});
    bestG[root.state] = 0;

    int nodesExpanded = 0;

    while (!open.empty())
    {
        QueueItem qi = open.top();
        open.pop();

        Node& current = nodes[qi.index];

        // If we have found a better path to this state, skip this outdated node
        auto itBest = bestG.find(current.state);
        if (itBest != bestG.end() && itBest->second < current.g)
            continue;

        if (current.state.isSolved())
        {
            // Reconstruct path by climbing parent pointers
            vector<Move> path;
            int idx = qi.index;

            while (nodes[idx].parent != -1)
            {
                path.push_back(nodes[idx].moveFromParent);
                idx = nodes[idx].parent;
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        if (current.g >= maxDepth)
            continue;

        if (nodesExpanded++ >= maxNodes)
            break; // Safety: stop if we expand too many nodes

        // Expand neighbors
        for (const Move& m : kAllMoves)
        {
            // Small pruning: don't immediately apply the inverse of the parent move
            if (current.parent != -1)
            {
                const Move& prev = nodes[current.parent].moveFromParent;
                if (isInverseMove(prev, m))
                    continue;
            }

            RubiksCube next = current.state;
            next.applyMove(m.face, m.turn);

            int g2 = current.g + 1;
            if (g2 > maxDepth)
                continue;

            auto it = bestG.find(next);
            if (it != bestG.end() && it->second <= g2)
                continue;

            if ((int)nodes.size() >= maxNodes)
                continue;

            Node child;
            child.state = next;
            child.g = g2;
            child.h = next.heuristic();
            child.parent = qi.index;
            child.moveFromParent = m;

            int childIndex = (int)nodes.size();
            nodes.push_back(child);
            bestG[next] = g2;

            open.push(QueueItem{childIndex, child.g + child.h});
        }
    }

    // Failed to find within the given limits
    return {};
}

vector<Move> RubiksCube::solveIDAStar(int maxIterations, int iterationDepth) const
{
    // If already solved, nothing to do
    if (isSolved())
        return {};

    const int INF = std::numeric_limits<int>::max();
    const int maxDepth = iterationDepth;

    // Start cube (copy of current state)
    RubiksCube start = *this;

    // We’ll reuse the same move list as in A*
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
        // Double stays Double
        return inv;
    };

    // Path we are building
    std::vector<Move> path;

    // Initial f-threshold = h(start)
    int threshold = start.heuristic();

    // Recursive DFS with f-bound and depth bound
    std::function<int(RubiksCube&, int, int, const Move&)> dfs;
    dfs = [&](RubiksCube& cube, int g, int curThreshold, const Move& prevMove) -> int
    {
        int h = cube.heuristic();
        int f = g + h;

        if (f > curThreshold)
            return f;   // candidate for next threshold

        if (cube.isSolved())
            return -1;  // signal success

        if (g >= maxDepth)
            return INF;

        int minNext = INF;

        for (const Move& m : allMoves)
        {
            // Pruning: no same face twice in a row
            if (prevMove.face != Face::Count && m.face == prevMove.face)
                continue;

            // Optional: avoid immediate inverse
            if (prevMove.face != Face::Count && isInverse(prevMove, m))
                continue;

            // Apply move
            cube.applyMove(m.face, m.turn);
            path.push_back(m);

            int t = dfs(cube, g + 1, curThreshold, m);

            if (t == -1)
                return -1;  // solution found, bubble up

            if (t < minNext)
                minNext = t;

            // Undo move
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
        Move dummyPrev{ Face::Count, CW };  // “no previous move”

        int t = dfs(work, 0, threshold, dummyPrev);

        if (t == -1)
        {
            // Solution in 'path'
            return path;
        }

        if (t == INF)
        {
            // No solution within depth bound
            return {};
        }

        threshold = t;
        ++iteration;

        // maxIterations <= 0  → endless
        if (maxIterations > 0 && iteration >= maxIterations)
        {
            // Gave up due to iteration limit
            return {};
        }
    }
}




///// FACE ROTATION HELPERS /////
//
// These currently only have the *structure*.
// You will fill in the actual neighbor-strip logic later.
// For now, they just rotate the face itself, so you can test Side behavior.
// Once you’re ready, you’ll extend each to also adjust the adjacent faces.
//

void RubiksCube::rotateTop(Turn t)
{
    Side &U = faces_[Up];

    auto cwNeighbors = [this]()
    {
        Side &F = faces_[Front];
        Side &R = faces_[Right];
        Side &B = faces_[Back];
        Side &L = faces_[Left];

        // Cycle the TOP rows: F -> R -> B -> L -> F
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = F.squares[0][i];              // save Front top row

        for (int i = 0; i < Side::SIZE; ++i)
            F.squares[0][i] = L.squares[0][i];     // F <- L

        for (int i = 0; i < Side::SIZE; ++i)
            L.squares[0][i] = B.squares[0][i];     // L <- B

        for (int i = 0; i < Side::SIZE; ++i)
            B.squares[0][i] = R.squares[0][i];     // B <- R

        for (int i = 0; i < Side::SIZE; ++i)
            R.squares[0][i] = tmp[i];              // R <- old F
    };

    switch (t)
    {
    case CW:
        U.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        U.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        U.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}


void RubiksCube::rotateBottom(Turn t)
{
    Side &D = faces_[Down];

    auto cwNeighbors = [this]()
    {
        Side &F = faces_[Front];
        Side &R = faces_[Right];
        Side &B = faces_[Back];
        Side &L = faces_[Left];

        // Cycle the BOTTOM rows: F -> R -> B -> L -> F
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = F.squares[Side::SIZE - 1][i];          // save Front bottom row

        for (int i = 0; i < Side::SIZE; ++i)
            F.squares[Side::SIZE - 1][i] = R.squares[Side::SIZE - 1][i]; // F <- R

        for (int i = 0; i < Side::SIZE; ++i)
            R.squares[Side::SIZE - 1][i] = B.squares[Side::SIZE - 1][i]; // R <- B

        for (int i = 0; i < Side::SIZE; ++i)
            B.squares[Side::SIZE - 1][i] = L.squares[Side::SIZE - 1][i]; // B <- L

        for (int i = 0; i < Side::SIZE; ++i)
            L.squares[Side::SIZE - 1][i] = tmp[i];                        // L <- old F
    };

    switch (t)
    {
    case CW:
        D.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        D.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        D.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}


void RubiksCube::rotateLeft(Turn t)
{
    Side &L = faces_[Left];

    auto cwNeighbors = [this]()
    {
        Side &U = faces_[Up];
        Side &F = faces_[Front];
        Side &D = faces_[Down];
        Side &B = faces_[Back];

        // Cycle the LEFT columns: U -> F -> D -> B -> U
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = U.squares[i][0];                 // save Up left column

        for (int i = 0; i < Side::SIZE; ++i)
            U.squares[i][0] = B.squares[i][Side::SIZE - 1]; // U <- B right column

        for (int i = 0; i < Side::SIZE; ++i)
            B.squares[i][Side::SIZE - 1] = D.squares[i][0]; // B <- D left column

        for (int i = 0; i < Side::SIZE; ++i)
            D.squares[i][0] = F.squares[i][0];        // D <- F left column

        for (int i = 0; i < Side::SIZE; ++i)
            F.squares[i][0] = tmp[i];                 // F <- old U
    };

    switch (t)
    {
    case CW:
        L.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        L.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        L.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}


void RubiksCube::rotateRight(Turn t)
{
    Side &R = faces_[Right];

    auto cwNeighbors = [this]()
    {
        Side &U = faces_[Up];
        Side &F = faces_[Front];
        Side &D = faces_[Down];
        Side &B = faces_[Back];

        // Cycle the RIGHT columns: U -> F -> D -> B -> U
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = U.squares[i][Side::SIZE - 1];            // save Up right column

        for (int i = 0; i < Side::SIZE; ++i)
            U.squares[i][Side::SIZE - 1] = F.squares[i][Side::SIZE - 1];  // U <- F right

        for (int i = 0; i < Side::SIZE; ++i)
            F.squares[i][Side::SIZE - 1] = D.squares[i][Side::SIZE - 1];  // F <- D right

        for (int i = 0; i < Side::SIZE; ++i)
            D.squares[i][Side::SIZE - 1] = B.squares[i][0];               // D <- B left

        for (int i = 0; i < Side::SIZE; ++i)
            B.squares[i][0] = tmp[i];                                     // B <- old U
    };

    switch (t)
    {
    case CW:
        R.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        R.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        R.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}


void RubiksCube::rotateFront(Turn t)
{
    Side &F = faces_[Front];

    auto cwNeighbors = [this]()
    {
        Side &U = faces_[Up];
        Side &D = faces_[Down];
        Side &L = faces_[Left];
        Side &R = faces_[Right];

        // Cycle edges around FRONT:
        // U bottom row, L right column, D top row, R left column
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = U.squares[Side::SIZE - 1][i];         // save Up bottom row

        // U bottom <- L right column
        for (int i = 0; i < Side::SIZE; ++i)
            U.squares[Side::SIZE - 1][i] = L.squares[i][Side::SIZE - 1];

        // L right column <- D top row
        for (int i = 0; i < Side::SIZE; ++i)
            L.squares[i][Side::SIZE - 1] = D.squares[0][i];

        // D top row <- R left column
        for (int i = 0; i < Side::SIZE; ++i)
            D.squares[0][i] = R.squares[i][0];

        // R left column <- old U bottom
        for (int i = 0; i < Side::SIZE; ++i)
            R.squares[i][0] = tmp[i];
    };

    switch (t)
    {
    case CW:
        F.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        F.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        F.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}


void RubiksCube::rotateBack(Turn t)
{
    Side &B = faces_[Back];

    auto cwNeighbors = [this]()
    {
        Side &U = faces_[Up];
        Side &D = faces_[Down];
        Side &L = faces_[Left];
        Side &R = faces_[Right];

        // Cycle edges around BACK:
        // U top row, R right column, D bottom row, L left column
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i)
            tmp[i] = U.squares[0][i];                 // save Up top row

        // U top <- R right column
        for (int i = 0; i < Side::SIZE; ++i)
            U.squares[0][i] = R.squares[i][Side::SIZE - 1];

        // R right column <- D bottom row
        for (int i = 0; i < Side::SIZE; ++i)
            R.squares[i][Side::SIZE - 1] = D.squares[Side::SIZE - 1][i];

        // D bottom row <- L left column
        for (int i = 0; i < Side::SIZE; ++i)
            D.squares[Side::SIZE - 1][i] = L.squares[i][0];

        // L left column <- old U top
        for (int i = 0; i < Side::SIZE; ++i)
            L.squares[i][0] = tmp[i];
    };

    switch (t)
    {
    case CW:
        B.rotateCW();
        cwNeighbors();
        break;
    case CCW:
        B.rotateCCW();
        cwNeighbors();
        cwNeighbors();
        cwNeighbors();
        break;
    case Double:
        B.rotate180();
        cwNeighbors();
        cwNeighbors();
        break;
    }
}

///// CUBIE /////

struct StickerPos {
    Face f;
    int r;
    int c;
};

struct CornerSlot {
    StickerPos s[3];
};

struct EdgeSlot {
    StickerPos s[2];
};

// Corner slots: each is a fixed *position* in the cube,
// defined by 3 stickers (face, row, col).
// We choose a consistent mapping in the solved state:
//
// 0: UFR, 1: UFL, 2: UBL, 3: UBR,
// 4: DFR, 5: DFL, 6: DBL, 7: DBR
static const CornerSlot kCornerSlots[8] = {
    // 0: UFR
    { { {Up,   2, 2}, {Front, 0, 2}, {Right, 0, 0} } },
    // 1: UFL
    { { {Up,   2, 0}, {Front, 0, 0}, {Left,  0, 2} } },
    // 2: UBL
    { { {Up,   0, 0}, {Back,  0, 2}, {Left,  0, 0} } },
    // 3: UBR
    { { {Up,   0, 2}, {Back,  0, 0}, {Right, 0, 2} } },
    // 4: DFR
    { { {Down, 0, 2}, {Front, 2, 2}, {Right, 2, 0} } },
    // 5: DFL
    { { {Down, 0, 0}, {Front, 2, 0}, {Left,  2, 2} } },
    // 6: DBL
    { { {Down, 2, 0}, {Back,  2, 2}, {Left,  2, 0} } },
    // 7: DBR
    { { {Down, 2, 2}, {Back,  2, 0}, {Right, 2, 2} } }
};

// Edge slots: 12 fixed positions, each with 2 stickers.
// 0: UF, 1: UR, 2: UB, 3: UL,
// 4: DF, 5: DR, 6: DB, 7: DL,
// 8: FR, 9: FL, 10: BR, 11: BL
static const EdgeSlot kEdgeSlots[12] = {
    // 0: UF
    { { {Up,   2, 1}, {Front, 0, 1} } },
    // 1: UR
    { { {Up,   1, 2}, {Right, 0, 1} } },
    // 2: UB
    { { {Up,   0, 1}, {Back,  0, 1} } },
    // 3: UL
    { { {Up,   1, 0}, {Left,  0, 1} } },
    // 4: DF
    { { {Down, 0, 1}, {Front, 2, 1} } },
    // 5: DR
    { { {Down, 1, 2}, {Right, 2, 1} } },
    // 6: DB
    { { {Down, 2, 1}, {Back,  2, 1} } },
    // 7: DL
    { { {Down, 1, 0}, {Left,  2, 1} } },
    // 8: FR
    { { {Front, 1, 2}, {Right, 1, 0} } },
    // 9: FL
    { { {Front, 1, 0}, {Left,  1, 2} } },
    // 10: BR
    { { {Back,  1, 0}, {Right, 1, 2} } },
    // 11: BL
    { { {Back,  1, 2}, {Left,  1, 0} } }
};

// Cubie-style heuristic implementation
int RubiksCube::cubieHeuristic() const {
    using std::array;
    using std::sort;

    // We'll cache what the corner/edge colors look like
    // in the solved cube, so we can compare against it.
    static bool initialized = false;
    static Color solvedCornerColors[8][3];
    static Color solvedEdgeColors[12][2];

    if (!initialized) {
        RubiksCube solved; // default ctor = solved cube

        // For each corner slot, record its 3 colors in solved state
        for (int i = 0; i < 8; ++i) {
            for (int k = 0; k < 3; ++k) {
                const StickerPos &sp = kCornerSlots[i].s[k];
                solvedCornerColors[i][k] =
                    solved.faces_[sp.f].squares[sp.r][sp.c];
            }
        }

        // For each edge slot, record its 2 colors in solved state
        for (int i = 0; i < 12; ++i) {
            for (int k = 0; k < 2; ++k) {
                const StickerPos &sp = kEdgeSlots[i].s[k];
                solvedEdgeColors[i][k] =
                    solved.faces_[sp.f].squares[sp.r][sp.c];
            }
        }

        initialized = true;
    }

    int misplacedCorners     = 0;
    int misorientedCorners   = 0;
    int misplacedEdges       = 0;
    int misorientedEdges     = 0;

    // ----- Corners -----
    for (int i = 0; i < 8; ++i) {
        Color cur[3];
        for (int k = 0; k < 3; ++k) {
            const StickerPos &sp = kCornerSlots[i].s[k];
            cur[k] = faces_[sp.f].squares[sp.r][sp.c];
        }

        // Compare as sets (ignore order)
        array<int,3> setSolved{};
        array<int,3> setCur{};
        for (int k = 0; k < 3; ++k) {
            setSolved[k] = static_cast<int>(solvedCornerColors[i][k]);
            setCur[k]    = static_cast<int>(cur[k]);
        }
        sort(setSolved.begin(), setSolved.end());
        sort(setCur.begin(), setCur.end());

        if (setSolved != setCur) {
            // This slot doesn't even have the right triple of colors:
            // "wrong cubie" in this corner slot.
            ++misplacedCorners;
        } else {
            // Right colors, check orientation (exact order)
            bool sameOrder = true;
            for (int k = 0; k < 3; ++k) {
                if (cur[k] != solvedCornerColors[i][k]) {
                    sameOrder = false;
                    break;
                }
            }
            if (!sameOrder)
                ++misorientedCorners;
        }
    }

    // ----- Edges -----
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

        // Compare as sets (order-insensitive)
        bool setMatch = ((s0 == c0 && s1 == c1) ||
                         (s0 == c1 && s1 == c0));

        if (!setMatch) {
            ++misplacedEdges;
        } else {
            // Right edge colors, check if flipped
            bool sameOrder = (s0 == c0 && s1 == c1);
            if (!sameOrder)
                ++misorientedEdges;
        }
    }

    auto ceil_div = [](int x, int d) {
        return (x + d - 1) / d;  // integer ceil(x / d)
    };

    // Lower bounds based on how many pieces can be fixed per move.
    int h_pos_corners = ceil_div(misplacedCorners,     4); // ≤ 4 corners moved per turn
    int h_pos_edges   = ceil_div(misplacedEdges,       4); // ≤ 4 edges  moved per turn
    int h_ori_corners = ceil_div(misorientedCorners,   3); // corner twist bound
    int h_ori_edges   = ceil_div(misorientedEdges,     4); // edge flip bound

    int h = std::max(
        std::max(h_pos_corners, h_pos_edges),
        std::max(h_ori_corners, h_ori_edges)
    );

    if (h < 0) h = 0;
    return h;
}
