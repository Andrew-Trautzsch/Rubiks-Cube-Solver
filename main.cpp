#include <iostream>
#include <string>
#include "cube.hpp"

// Map Color enum to a single character
char colorToChar(Color c)
{
    switch (c)
    {
    case White:  return 'W';
    case Yellow: return 'Y';
    case Red:    return 'R';
    case Orange: return 'O';
    case Blue:   return 'B';
    case Green:  return 'G';
    default:     return '?';
    }
}

// Print a single 3x3 side
void printSide(const Side& s)
{
    for (int r = 0; r < Side::SIZE; ++r)
    {
        for (int c = 0; c < Side::SIZE; ++c)
        {
            std::cout << colorToChar(s.squares[r][c]) << ' ';
        }
        std::cout << '\n';
    }
}

// Print the entire cube, face by face
void printCube(const RubiksCube& cube)
{
    std::cout << "Up:\n";
    printSide(cube.face(Up));
    std::cout << '\n';

    std::cout << "Down:\n";
    printSide(cube.face(Down));
    std::cout << '\n';

    std::cout << "Left:\n";
    printSide(cube.face(Left));
    std::cout << '\n';

    std::cout << "Right:\n";
    printSide(cube.face(Right));
    std::cout << '\n';

    std::cout << "Front:\n";
    printSide(cube.face(Front));
    std::cout << '\n';

    std::cout << "Back:\n";
    printSide(cube.face(Back));
    std::cout << '\n';
}

// Helper to get a string for a move (for printing)
std::string moveName(Face f, Turn t)
{
    std::string name;
    switch (f)
    {
    case Up:    name = "U"; break;
    case Down:  name = "D"; break;
    case Left:  name = "L"; break;
    case Right: name = "R"; break;
    case Front: name = "F"; break;
    case Back:  name = "B"; break;
    default:    name = "?"; break;
    }

    switch (t)
    {
    case CW:     /* nothing */    break;      // e.g. F
    case CCW:    name += "'";     break;      // e.g. F'
    case Double: name += "2";     break;      // e.g. F2
    }
    return name;
}

// Inverse of a Turn
Turn inverseTurn(Turn t)
{
    switch (t)
    {
    case CW:     return CCW;
    case CCW:    return CW;
    case Double: return Double;
    }
    return CW; // fallback
}

int main()
{
    RubiksCube cube;

    std::cout << "Solved cube:\n";
    printCube(cube);
    std::cout << "Heuristic: " << cube.heuristic() << "\n\n";

    // Scramble a bit
    cube.scramble(5);
    std::cout << "After scramble:\n";
    printCube(cube);
    std::cout << "Heuristic: " << cube.heuristic() << "\n\n";

    // Solve with A*
    std::vector<Move> solution = cube.solveAStar(12, 1000000);

    if (solution.empty() && !cube.isSolved())
    {
        std::cout << "A* failed to find a solution within the limits.\n";
        return 0;
    }

    std::cout << "Solution has " << solution.size() << " moves:\n";
    for (const Move& m : solution)
    {
        char fChar = '?';
        switch (m.face)
        {
        case Up:    fChar = 'U'; break;
        case Down:  fChar = 'D'; break;
        case Left:  fChar = 'L'; break;
        case Right: fChar = 'R'; break;
        case Front: fChar = 'F'; break;
        case Back:  fChar = 'B'; break;
        default: break;
        }

        std::string tStr;
        switch (m.turn)
        {
        case CW:     tStr = "";  break;
        case CCW:    tStr = "'"; break;
        case Double: tStr = "2"; break;
        }

        std::cout << fChar << tStr << ' ';
    }
    std::cout << "\n\n";

    // Apply the solution and verify
    for (const Move& m : solution)
        cube.applyMove(m);

    std::cout << "After applying solution:\n";
    printCube(cube);
    std::cout << "isSolved(): " << std::boolalpha << cube.isSolved() << "\n";
    std::cout << "Heuristic: " << cube.heuristic() << "\n";

    return 0;
}