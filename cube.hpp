#ifndef CUBE_HPP
#define CUBE_HPP

#include <vector>
#include <array>
#include <string>

using std::vector; using std::array;

///// TYPES /////

enum Color { White = 0, Yellow, Red, Orange, Blue, Green };
enum Face { Up = 0, Down, Left, Right, Front, Back, Count };
enum Turn { CW, CCW, Double };

struct Move {
    Face face;
    Turn turn;
};

///// CUBE STRUCTURES /////

struct Side
{
    static constexpr int SIZE = 3;
    array<array<Color, SIZE>, SIZE> squares{};

    Side() = default;
    explicit Side(Color fillColor) {
        for(int i=0; i<SIZE; i++) for(int j=0; j<SIZE; j++) squares[i][j] = fillColor;
    }

    int colorMismatch() const;
    vector<Color> getRow(int) const;
    vector<Color> getColumn(int) const;
    Color getCenter() const {return squares[1][1];}
    void setRow(int, const array<Color, SIZE>&);
    void setColumn(int, const array<Color, SIZE>&);
    void rotateCW();
    void rotateCCW();
    void rotate180();
};

class RubiksCube
{
public:
    static constexpr int FACE_COUNT = Face::Count;

    RubiksCube(); 
    explicit RubiksCube(const std::array<Side, FACE_COUNT>& faces);

    void scramble(int moveCount);

    Side& face(Face input) { return faces_[input]; }
    const Side& face(Face input) const { return faces_[input]; }

    void applyMove(Face, Turn);
    void applyMove(const Move& m) { applyMove(m.face, m.turn); }

    bool isSolved() const;
    bool operator==(const RubiksCube& other) const;
    bool operator!=(const RubiksCube& other) const { return !(*this == other); }
    
    // Heuristics (implemented in heuristics.cpp)
    int cubieHeuristic() const;
    int heuristic() const { return cubieHeuristic(); }
    std::size_t hash() const;

    // Solvers (implemented in heuristics.cpp)
    vector<Move> solveIDAStar(int maxIterations = -1, int iterationDepth = 24) const;

private:
    array<Side, FACE_COUNT> faces_;

    void rotateTop(Turn);
    void rotateBottom(Turn);
    void rotateLeft(Turn);
    void rotateRight(Turn);
    void rotateFront(Turn);
    void rotateBack(Turn);
};

// ----- Visual Functions -----

void setColor(Color c);
void drawCube3D(const RubiksCube& cube, bool isAnimating, const Move& animMove, float animProgress, float animDuration);
void drawText2D(int x, int y, const std::string &s);
void drawFilledRect2D(int x0,int y0,int x1,int y1);

#endif // CUBE_HPP