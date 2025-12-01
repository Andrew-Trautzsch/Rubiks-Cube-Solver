#include "cube.hpp"

#include <random>
#include <algorithm> // for std::fill, std::equal

///// RUBIKSCUBE IMPLEMENTATION /////

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

///// FACE ROTATION HELPERS /////
//
// These currently only have the *structure*.
// You will fill in the actual neighbor-strip logic later.
// For now, they just rotate the face itself, so you can test Side behavior.
// Once you’re ready, you’ll extend each to also adjust the adjacent faces.
//

void RubiksCube::rotateTop(Turn t)
{
    // rotate the Up face itself
    Side& U = faces_[Up];

    switch (t)
    {
    case CW:
        U.rotateCW();
        break;
    case CCW:
        U.rotateCCW();
        break;
    case Double:
        U.rotate180();
        break;
    }

    // TODO: update neighboring faces' rows/columns
}

void RubiksCube::rotateBottom(Turn t)
{
    Side& D = faces_[Down];

    switch (t)
    {
    case CW:
        D.rotateCW();
        break;
    case CCW:
        D.rotateCCW();
        break;
    case Double:
        D.rotate180();
        break;
    }

    // TODO: update neighbors
}

void RubiksCube::rotateLeft(Turn t)
{
    Side& L = faces_[Left];

    switch (t)
    {
    case CW:
        L.rotateCW();
        break;
    case CCW:
        L.rotateCCW();
        break;
    case Double:
        L.rotate180();
        break;
    }

    // TODO: update neighbors
}

void RubiksCube::rotateRight(Turn t)
{
    Side& R = faces_[Right];

    switch (t)
    {
    case CW:
        R.rotateCW();
        break;
    case CCW:
        R.rotateCCW();
        break;
    case Double:
        R.rotate180();
        break;
    }

    // TODO: update neighbors
}

void RubiksCube::rotateFront(Turn t)
{
    Side& F = faces_[Front];

    switch (t)
    {
    case CW:
        F.rotateCW();
        break;
    case CCW:
        F.rotateCCW();
        break;
    case Double:
        F.rotate180();
        break;
    }

    // TODO: update neighbors
}

void RubiksCube::rotateBack(Turn t)
{
    Side& B = faces_[Back];

    switch (t)
    {
    case CW:
        B.rotateCW();
        break;
    case CCW:
        B.rotateCCW();
        break;
    case Double:
        B.rotate180();
        break;
    }

    // TODO: update neighbors
}
