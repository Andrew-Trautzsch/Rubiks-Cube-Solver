#ifndef CUBE_HPP
#define CUBE_HPP

#include <vector>
#include <array>

using std::vector; using std::array;

///// TYPES /////

// cube colors
enum Color
{
    White = 0,
    Yellow,
    Red,
    Orange,
    Blue,
    Green
};

// cube faces
enum Face
{
    Up = 0,
    Down,
    Left,
    Right,
    Front,
    Back,
    Count // not a face, just tells you how many
};

// types of rotations
enum Turn
{
    CW,
    CCW,
    Double
};

struct Move
{
    Face face;
    Turn turn;
};

///// CUBE STRUCTURES /////

/*
  0  1  2
0 _  _  _
1 _  C  _
2 _  _  _
*/
struct Side
{
    static constexpr int SIZE = 3;
    array<array<Color, SIZE>, SIZE> squares{};

    Side() = default;
    explicit Side(Color fillColor)
    {
        for(int i=0; i<SIZE; i++)
            for(int j=0; j<SIZE; j++)
                squares[i][j] = fillColor;
    }

    // returns 0-8, number == amount of colors =/ center
    int colorMismatch() const;

    // accessors
    vector<Color> getRow(int) const;
    vector<Color> getColumn(int) const;
    Color getCenter() const {return squares[1][1];}

    // mutators: index, input
    void setRow(int, const array<Color, SIZE>&);
    void setColumn(int, const array<Color, SIZE>&);

    // rotations
    void rotateCW();
    void rotateCCW();
    void rotate180();
};

class RubiksCube
{
public:
    static constexpr int FACE_COUNT = Face::Count;

    RubiksCube(); // creates solved cube
    explicit RubiksCube(const std::array<Side, FACE_COUNT>& faces); // used for manual input

    void scramble(int moveCount); // randomizes cube

    Side& face(Face input) { return faces_[input]; }
    const Side& face(Face input) const { return faces_[input]; }

    // baseline for moves, will call rotations
    void applyMove(Face, Turn);

    bool isSolved() const;

    // Heurstic helpers
    bool operator==(const RubiksCube& other) const;
    bool operator!=(const RubiksCube& other) const { return !(*this == other); }

    int misplacedFacelets() const;
    // Convenience: apply a Move struct
    void applyMove(const Move& m) { applyMove(m.face, m.turn); }

    // For A*: simple heuristic wrapper you already added earlier
    int heuristic() const { return misplacedFacelets(); }

    // Hash of the cube state (for unordered_map)
    std::size_t hash() const;

    // A* solver: returns sequence of moves from *this to solved
    // maxDepth: depth cutoff; maxNodes: safety cap to avoid explosion
    vector<Move> solveAStar(int maxDepth = 12, int maxNodes = 100000) const;

private:
    array<Side, FACE_COUNT> faces_;

    void rotateTop(Turn);
    void rotateBottom(Turn);
    void rotateLeft(Turn);
    void rotateRight(Turn);
    void rotateFront(Turn);
    void rotateBack(Turn);
};



#endif // CUBE_HPP