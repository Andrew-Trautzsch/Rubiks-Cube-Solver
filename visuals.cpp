#include "cube.hpp"
#include <GL/freeglut.h>
#include <string>

// ----- Constants -----
const float CUBE_HALF     = 1.0f;
const float CUBIE_SPACING = (2.0f * CUBE_HALF) / 3.0f;
const float CUBIE_SIZE    = CUBIE_SPACING * 0.92f;

// ----- Helpers -----

void setColor(Color c)
{
    switch (c) {
    case White:  glColor3f(1,1,1); break;
    case Yellow: glColor3f(1,1,0); break;
    case Red:    glColor3f(1,0,0); break;
    case Orange: glColor3f(1,0.5f,0); break;
    case Blue:   glColor3f(0,0,1); break;
    case Green:  glColor3f(0,1,0); break;
    default:     glColor3f(0.5f,0.5f,0.5f); break;
    }
}

// used for UI
void drawText2D(int x, int y, const std::string &s)
{
    glRasterPos2i(x,y);
    for(char c: s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

// used for UI
void drawFilledRect2D(int x0,int y0,int x1,int y1)
{
    glBegin(GL_QUADS);
      glVertex2i(x0,y0);
      glVertex2i(x1,y0);
      glVertex2i(x1,y1);
      glVertex2i(x0,y1);
    glEnd();
}

// used for animation
static void axisForFace(Face f, float &ax, float &ay, float &az)
{
    ax = ay = az = 0.0f;
    switch (f)
    {
    case Up:    ay = 1.0f; break;
    case Down:  ay = -1.0f; break;
    case Left:  ax = -1.0f; break;
    case Right: ax = 1.0f;  break;
    case Front: az = 1.0f;  break;
    case Back:  az = -1.0f; break;
    default: break;
    }
}

// used for animation
static bool cubieOnFaceLayer(int ix,int iy,int iz, Face f)
{
    switch(f){
    case Front: return (iz ==  1);
    case Back:  return (iz == -1);
    case Right: return (ix ==  1);
    case Left:  return (ix == -1);
    case Up:    return (iy ==  1);
    case Down:  return (iy == -1);
    default:    return false;
    }
}

static bool calculateAnimAngle(bool active, const Move& move, float progress, float duration, float &outAngle)
{
    if (!active) return false;

    float tmp = progress / duration;
    if (tmp > 1.0f) tmp = 1.0f;
    const float baseAngle = 90.0f;

    // Standard CW logic
    float faceSign = -1.0f; 

    switch (move.turn)
    {
    case CW: outAngle = faceSign * baseAngle * tmp; break;
    case CCW: outAngle = -faceSign * baseAngle * tmp; break;
    case Double: outAngle = 2.0f * faceSign * baseAngle * tmp; break;
    }
    return true;
}

static Color getStickerColor(const RubiksCube& cube, int ix, int iy, int iz, Face f)
{
    const Side &side = cube.face(f);
    int row=0,col=0;
    switch(f){
    case Front: row = 1-iy; col = ix+1; break;
    case Back:  row = 1-iy; col = 1-ix; break;
    case Right: row = 1-iy; col = 1-iz; break;
    case Left:  row = 1-iy; col = iz+1; break;
    case Up:    row = iz+1; col = ix+1; break;
    case Down:  row = 1-iz; col = ix+1; break;
    default:    break;
    }
    if (row < 0) row = 0; else if (row > 2) row = 2;
    if (col < 0) col = 0; else if (col > 2) col = 2;
    return side.squares[row][col];
}

// helper for each cubie, draws stickers on each aswell
static void drawCubie(const RubiksCube& cube, int ix, int iy, int iz)
{
    float half = CUBIE_SIZE*0.5f;
    const float stickerInset = 0.01f;

    glColor3f(0,0,0);
    glutSolidCube(CUBIE_SIZE);

    if (iz == 1) {
        setColor(getStickerColor(cube, ix,iy,iz,Front));
        glBegin(GL_QUADS);
         glVertex3f(-half,-half, half+stickerInset);
         glVertex3f( half,-half, half+stickerInset);
         glVertex3f( half, half, half+stickerInset);
         glVertex3f(-half, half, half+stickerInset);
        glEnd();
    }
    if (iz == -1) {
        setColor(getStickerColor(cube, ix,iy,iz,Back));
        glBegin(GL_QUADS);
         glVertex3f( half,-half,-half-stickerInset);
         glVertex3f(-half,-half,-half-stickerInset);
         glVertex3f(-half, half,-half-stickerInset);
         glVertex3f( half, half,-half-stickerInset);
        glEnd();
    }
    if (ix == 1) {
        setColor(getStickerColor(cube, ix,iy,iz,Right));
        glBegin(GL_QUADS);
         glVertex3f( half+stickerInset,-half,-half);
         glVertex3f( half+stickerInset,-half, half);
         glVertex3f( half+stickerInset, half, half);
         glVertex3f( half+stickerInset, half,-half);
        glEnd();
    }
    if (ix == -1) {
        setColor(getStickerColor(cube, ix,iy,iz,Left));
        glBegin(GL_QUADS);
         glVertex3f(-half-stickerInset,-half, half);
         glVertex3f(-half-stickerInset,-half,-half);
         glVertex3f(-half-stickerInset, half,-half);
         glVertex3f(-half-stickerInset, half, half);
        glEnd();
    }
    if (iy == 1) {
        setColor(getStickerColor(cube, ix,iy,iz,Up));
        glBegin(GL_QUADS);
         glVertex3f(-half, half+stickerInset, half);
         glVertex3f( half, half+stickerInset, half);
         glVertex3f( half, half+stickerInset,-half);
         glVertex3f(-half, half+stickerInset,-half);
        glEnd();
    }
    if (iy == -1) {
        setColor(getStickerColor(cube, ix,iy,iz,Down));
        glBegin(GL_QUADS);
         glVertex3f(-half,-half-stickerInset,-half);
         glVertex3f( half,-half-stickerInset,-half);
         glVertex3f( half,-half-stickerInset, half);
         glVertex3f(-half,-half-stickerInset, half);
        glEnd();
    }
}

// Main 3D Draw Function
void drawCube3D(const RubiksCube& cube, bool isAnimating, const Move& animMove, float animProgress, float animDuration)
{
    glEnable(GL_DEPTH_TEST);
    float angle = 0.0f;
    bool hasAngle = calculateAnimAngle(isAnimating, animMove, animProgress, animDuration, angle);
    
    float ax=0, ay=0, az=0;
    if (hasAngle) axisForFace(animMove.face, ax, ay, az);

    for(int ix=-1; ix<=1; ++ix)
        for(int iy=-1; iy<=1; ++iy)
            for(int iz=-1; iz<=1; ++iz)
            {
                glPushMatrix();
                // If this cubie is on the face currently rotating, apply rotation
                if(hasAngle && cubieOnFaceLayer(ix, iy, iz, animMove.face))
                    glRotatef(angle, ax, ay, az);
                    
                glTranslatef(ix * CUBIE_SPACING, iy * CUBIE_SPACING, iz * CUBIE_SPACING);
                drawCubie(cube, ix, iy, iz);
                glPopMatrix();
            }
}