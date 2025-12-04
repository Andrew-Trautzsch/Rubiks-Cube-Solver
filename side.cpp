#include "cube.hpp"

///// FACE ROTATIONS /////

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

// rotate 180°
void Side::rotate180()
{
    rotateCW();
    rotateCW();
}
