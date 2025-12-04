#include "cube.hpp"
#include <random>

///// RUBIKSCUBE IMPLEMENTATION /////

// solved cube constructor
RubiksCube::RubiksCube()
{
    faces_[Up]    = Side(Color::White);
    faces_[Down]  = Side(Color::Yellow);
    faces_[Left]  = Side(Color::Orange);
    faces_[Right] = Side(Color::Red);
    faces_[Front] = Side(Color::Green);
    faces_[Back]  = Side(Color::Blue);
}

// randomizes cube with a series of random moves
// void RubiksCube::scramble(int moveCount)
// {
//     static std::mt19937 rng(std::random_device{}());

//     std::uniform_int_distribution<int> faceDist(0, Count - 1);
//     std::uniform_int_distribution<int> turnDist(0, 2); // CW, CCW, Double

//     for (int i = 0; i < moveCount; ++i)
//     {
//         Face f = static_cast<Face>(faceDist(rng));
//         Turn t = static_cast<Turn>(turnDist(rng));
//         applyMove(f, t);
//     }
// }

// core move dispatcher
void RubiksCube::applyMove(Face f, Turn t)
{
    switch (f)
    {
    case Up:    rotateTop(t);    break;
    case Down:  rotateBottom(t); break;
    case Left:  rotateLeft(t);   break;
    case Right: rotateRight(t);  break;
    case Front: rotateFront(t);  break;
    case Back:  rotateBack(t);   break;
    default:    break;
    }
}

///// FACE ROTATION HELPERS /////

void RubiksCube::rotateTop(Turn t)
{
    Side &U = faces_[Up];
    auto cwNeighbors = [this]() {
        Side &F = faces_[Front], &R = faces_[Right], &B = faces_[Back], &L = faces_[Left];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = F.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) F.squares[0][i] = R.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) R.squares[0][i] = B.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) B.squares[0][i] = L.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) L.squares[0][i] = tmp[i];
    };
    switch (t) {
    case CW: U.rotateCW(); cwNeighbors(); break;
    case CCW: U.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: U.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}

void RubiksCube::rotateBottom(Turn t)
{
    Side &D = faces_[Down];
    auto cwNeighbors = [this]() {
        Side &F = faces_[Front], &R = faces_[Right], &B = faces_[Back], &L = faces_[Left];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = F.squares[Side::SIZE - 1][i];
        for (int i = 0; i < Side::SIZE; ++i) F.squares[Side::SIZE - 1][i] = L.squares[Side::SIZE - 1][i];
        for (int i = 0; i < Side::SIZE; ++i) L.squares[Side::SIZE - 1][i] = B.squares[Side::SIZE - 1][i];
        for (int i = 0; i < Side::SIZE; ++i) B.squares[Side::SIZE - 1][i] = R.squares[Side::SIZE - 1][i];
        for (int i = 0; i < Side::SIZE; ++i) R.squares[Side::SIZE - 1][i] = tmp[i];
    };
    switch (t) {
    case CW: D.rotateCW(); cwNeighbors(); break;
    case CCW: D.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: D.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}

void RubiksCube::rotateLeft(Turn t)
{
    Side &L = faces_[Left];
    auto cwNeighbors = [this]() {
        Side &U = faces_[Up], &F = faces_[Front], &D = faces_[Down], &B = faces_[Back];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = U.squares[i][0];
        for (int i = 0; i < Side::SIZE; ++i) U.squares[i][0] = B.squares[Side::SIZE - 1 - i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) B.squares[i][Side::SIZE - 1] = D.squares[Side::SIZE - 1 - i][0];
        for (int i = 0; i < Side::SIZE; ++i) D.squares[i][0] = F.squares[i][0];
        for (int i = 0; i < Side::SIZE; ++i) F.squares[i][0] = tmp[i];
    };
    switch (t) {
    case CW: L.rotateCW(); cwNeighbors(); break;
    case CCW: L.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: L.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}

void RubiksCube::rotateRight(Turn t)
{
    Side &R = faces_[Right];
    auto cwNeighbors = [this]() {
        Side &U = faces_[Up], &F = faces_[Front], &D = faces_[Down], &B = faces_[Back];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = U.squares[i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) U.squares[i][Side::SIZE - 1] = F.squares[i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) F.squares[i][Side::SIZE - 1] = D.squares[i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) D.squares[i][Side::SIZE - 1] = B.squares[Side::SIZE - 1 - i][0];
        for (int i = 0; i < Side::SIZE; ++i) B.squares[i][0] = tmp[Side::SIZE - 1 - i];
    };
    switch (t) {
    case CW: R.rotateCW(); cwNeighbors(); break;
    case CCW: R.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: R.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}

void RubiksCube::rotateFront(Turn t)
{
    Side &F = faces_[Front];
    auto cwNeighbors = [this]() {
        Side &U = faces_[Up], &D = faces_[Down], &L = faces_[Left], &R = faces_[Right];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = U.squares[Side::SIZE - 1][i];
        for (int i = 0; i < Side::SIZE; ++i) U.squares[Side::SIZE - 1][i] = L.squares[Side::SIZE - 1 - i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) L.squares[i][Side::SIZE - 1] = D.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) D.squares[0][i] = R.squares[Side::SIZE - 1 - i][0];
        for (int i = 0; i < Side::SIZE; ++i) R.squares[i][0] = tmp[i];
    };
    switch (t) {
    case CW: F.rotateCW(); cwNeighbors(); break;
    case CCW: F.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: F.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}

void RubiksCube::rotateBack(Turn t)
{
    Side &B = faces_[Back];
    auto cwNeighbors = [this]() {
        Side &U = faces_[Up], &D = faces_[Down], &L = faces_[Left], &R = faces_[Right];
        Color tmp[Side::SIZE];
        for (int i = 0; i < Side::SIZE; ++i) tmp[i] = U.squares[0][i];
        for (int i = 0; i < Side::SIZE; ++i) U.squares[0][i] = R.squares[i][Side::SIZE - 1];
        for (int i = 0; i < Side::SIZE; ++i) R.squares[i][Side::SIZE - 1] = D.squares[Side::SIZE - 1][Side::SIZE - 1 - i];
        for (int i = 0; i < Side::SIZE; ++i) D.squares[Side::SIZE - 1][i] = L.squares[i][0];
        for (int i = 0; i < Side::SIZE; ++i) L.squares[i][0] = tmp[Side::SIZE - 1 - i];
    };
    switch (t) {
    case CW: B.rotateCW(); cwNeighbors(); break;
    case CCW: B.rotateCCW(); cwNeighbors(); cwNeighbors(); cwNeighbors(); break;
    case Double: B.rotate180(); cwNeighbors(); cwNeighbors(); break;
    }
}