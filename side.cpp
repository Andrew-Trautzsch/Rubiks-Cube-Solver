#include "cube.hpp"

///// SIDE IMPLEMENTATION /////

// returns 0-8, number == amount of colors != center
int Side::colorMismatch()
{
    Color center = getCenter();
    int count = 0;

    for (int r = 0; r < SIZE; ++r)
    {
        for (int c = 0; c < SIZE; ++c)
        {
            if (squares[r][c] != center)
                ++count;
        }
    }

    // exclude the center cell itself (always matches)
    return count - 1; // or std::max(0, count - 1) if you want to be extra safe
}

// accessors
vector<Color> Side::getRow(int row) const
{
    vector<Color> result;
    result.reserve(SIZE);

    for (int c = 0; c < SIZE; ++c)
        result.push_back(squares[row][c]);

    return result;
}

vector<Color> Side::getColumn(int col) const
{
    vector<Color> result;
    result.reserve(SIZE);

    for (int r = 0; r < SIZE; ++r)
        result.push_back(squares[r][col]);

    return result;
}

// mutators
void Side::setRow(int row, const array<Color, SIZE>& vals)
{
    for (int c = 0; c < SIZE; ++c)
        squares[row][c] = vals[c];
}

void Side::setColumn(int col, const array<Color, SIZE>& vals)
{
    for (int r = 0; r < SIZE; ++r)
        squares[r][col] = vals[r];
}

// rotate the 3x3 face 90° clockwise
void Side::rotateCW()
{
    array<array<Color, SIZE>, SIZE> tmp = squares;

    for (int r = 0; r < SIZE; ++r)
    {
        for (int c = 0; c < SIZE; ++c)
        {
            squares[r][c] = tmp[SIZE - 1 - c][r];
        }
    }
}

// rotate 90° counter-clockwise
void Side::rotateCCW()
{
    array<array<Color, SIZE>, SIZE> tmp = squares;

    for (int r = 0; r < SIZE; ++r)
    {
        for (int c = 0; c < SIZE; ++c)
        {
            squares[r][c] = tmp[c][SIZE - 1 - r];
        }
    }
}

// rotate 180° (can just do two CW rotations)
void Side::rotate180()
{
    // simplest: two CW turns
    rotateCW();
    rotateCW();
}
