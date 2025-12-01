#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include "cube.hpp"

// ---------- Global cube & camera ----------

RubiksCube g_cube;

// Camera / arcball
float g_camAngleX = 30.0f;   // up-down rotation
float g_camAngleY = -30.0f;  // left-right rotation
float g_camDist   = 6.0f;    // distance from origin

bool g_isDragging = false;
int  g_lastMouseX = 0;
int  g_lastMouseY = 0;

// Overall cube half-size
const float CUBE_HALF = 1.0f;

// For the 3x3x3 cubies
const float CUBIE_SPACING = (2.0f * CUBE_HALF) / 3.0f;   // distance between cubie centers
const float CUBIE_SIZE    = CUBIE_SPACING * 0.92f;       // cubie edge length

// ---------- A* solution playback state ----------

std::vector<Move> g_solutionMoves;
int   g_solutionIndex       = 0;
bool  g_solutionPlaying     = false;

bool  g_currentMoveActive   = false;
Move  g_currentMove;
float g_moveProgress        = 0.0f;   // 0..1
const float g_moveDuration  = 0.3f;   // seconds per 90° turn
int   g_lastTimeMs          = 0;

// ---------- Color helper ----------

void setColor(Color c)
{
    switch (c)
    {
    case White:  glColor3f(1.0f, 1.0f, 1.0f); break;
    case Yellow: glColor3f(1.0f, 1.0f, 0.0f); break;
    case Red:    glColor3f(1.0f, 0.0f, 0.0f); break;
    case Orange: glColor3f(1.0f, 0.5f, 0.0f); break;
    case Blue:   glColor3f(0.0f, 0.0f, 1.0f); break;
    case Green:  glColor3f(0.0f, 1.0f, 0.0f); break;
    default:     glColor3f(0.0f, 0.0f, 0.0f); break;
    }
}

// ---------- Helpers for animation ----------

bool cubieOnFaceLayer(int ix, int iy, int iz, Face f)
{
    switch (f)
    {
    case Front: return iz ==  1;
    case Back:  return iz == -1;
    case Right: return ix ==  1;
    case Left:  return ix == -1;
    case Up:    return iy ==  1;
    case Down:  return iy == -1;
    default:    return false;
    }
}

void axisForFace(Face f, float &ax, float &ay, float &az)
{
    ax = ay = az = 0.0f;
    switch (f)
    {
    case Front:
    case Back:
        az = 1.0f; break;
    case Right:
    case Left:
        ax = 1.0f; break;
    case Up:
    case Down:
        ay = 1.0f; break;
    default: break;
    }
}

// sign convention to make CW look reasonable on screen
float baseSignForFace(Face f)
{
    switch (f)
    {
    case Front: return -1.0f;
    case Right: return -1.0f;
    case Up:    return -1.0f;

    case Back:  return  1.0f;
    case Left:  return  1.0f;
    case Down:  return  1.0f;

    default:    return -1.0f;
    }
}

float fullAngleForMove(const Move& m)
{
    float base = baseSignForFace(m.face);
    switch (m.turn)
    {
    case CW:     return base * 90.0f;
    case CCW:    return -base * 90.0f;
    case Double: return base * 180.0f;
    default:     return base * 90.0f;
    }
}

bool currentAnimatedAngle(float &angleOut)
{
    if (!g_currentMoveActive)
        return false;
    float full = fullAngleForMove(g_currentMove);
    angleOut = full * g_moveProgress;
    return true;
}

// ---------- Mapping from cubie position to sticker indices ----------
// ix, iy, iz are in {-1,0,1}
//
// These mappings keep things consistent enough that the visual state
// matches g_cube's sticker layout.

Color stickerColorForCubieFace(int ix, int iy, int iz, Face f)
{
    const Side &side = g_cube.face(f);
    int row = 0, col = 0;

    switch (f)
    {
    case Front: // iz == +1
        // y: +1 -> row 0 (top), 0 -> 1, -1 -> 2
        row = 1 - iy;
        // x: -1 -> col 0 (left), 0 -> 1, +1 -> 2
        col = ix + 1;
        break;

    case Back: // iz == -1
        row = 1 - iy;
        // mirrored horizontally compared to front
        col = 1 - ix; // +1 -> 0, 0 -> 1, -1 -> 2
        break;

    case Right: // ix == +1
        row = 1 - iy;
        // from viewer on +X looking toward origin, left = +Z
        // z: +1 -> col 0, 0 -> 1, -1 -> 2
        col = 1 - iz; // +1->0, 0->1, -1->2
        break;

    case Left: // ix == -1
        row = 1 - iy;
        // viewer on -X, left = -Z
        // z: -1 -> 0, 0 ->1, +1->2
        col = iz + 1;
        break;

    case Up: // iy == +1
        // Arrange so row 2 of Up touches top row of Front
        // z: +1 (toward Front) -> row 2, 0 -> 1, -1 -> 0
        row = iz + 1;   // -1->0,0->1,+1->2
        // x: -1 -> col 0, 0 -> 1, +1 -> 2
        col = ix + 1;
        break;

    case Down: // iy == -1
        // Row 0 of Down touches bottom row of Front
        // z: +1 (toward Front) -> row 0, 0->1, -1->2
        row = 1 - iz;   // +1->0,0->1,-1->2
        col = ix + 1;
        break;

    default:
        row = col = 0;
        break;
    }

    // Clamp just in case (should always be 0..2)
    if (row < 0) row = 0; if (row > 2) row = 2;
    if (col < 0) col = 0; if (col > 2) col = 2;
    return side.squares[row][col];
}

// ---------- Draw a single cubie (with stickers) ----------

void drawCubieWithStickers(int ix, int iy, int iz)
{
    float half = CUBIE_SIZE * 0.5f;
    const float stickerInset = 0.01f; // distance above cubie surface

    // Base black cube
    glColor3f(0.0f, 0.0f, 0.0f);
    glutSolidCube(CUBIE_SIZE);

    // FRONT face (z = +half)
    if (iz == 1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Front);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f(-half, -half,  half + stickerInset);
            glVertex3f( half, -half,  half + stickerInset);
            glVertex3f( half,  half,  half + stickerInset);
            glVertex3f(-half,  half,  half + stickerInset);
        glEnd();
    }

    // BACK face (z = -half)
    if (iz == -1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Back);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f( half, -half, -half - stickerInset);
            glVertex3f(-half, -half, -half - stickerInset);
            glVertex3f(-half,  half, -half - stickerInset);
            glVertex3f( half,  half, -half - stickerInset);
        glEnd();
    }

    // RIGHT face (x = +half)
    if (ix == 1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Right);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f( half + stickerInset, -half, -half);
            glVertex3f( half + stickerInset, -half,  half);
            glVertex3f( half + stickerInset,  half,  half);
            glVertex3f( half + stickerInset,  half, -half);
        glEnd();
    }

    // LEFT face (x = -half)
    if (ix == -1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Left);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f(-half - stickerInset, -half,  half);
            glVertex3f(-half - stickerInset, -half, -half);
            glVertex3f(-half - stickerInset,  half, -half);
            glVertex3f(-half - stickerInset,  half,  half);
        glEnd();
    }

    // UP face (y = +half)
    if (iy == 1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Up);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f(-half,  half + stickerInset,  half);
            glVertex3f( half,  half + stickerInset,  half);
            glVertex3f( half,  half + stickerInset, -half);
            glVertex3f(-half,  half + stickerInset, -half);
        glEnd();
    }

    // DOWN face (y = -half)
    if (iy == -1)
    {
        Color col = stickerColorForCubieFace(ix, iy, iz, Down);
        setColor(col);

        glBegin(GL_QUADS);
            glVertex3f(-half, -half - stickerInset, -half);
            glVertex3f( half, -half - stickerInset, -half);
            glVertex3f( half, -half - stickerInset,  half);
            glVertex3f(-half, -half - stickerInset,  half);
        glEnd();
    }
}

// ---------- Draw full cube (27 cubies, with animation) ----------

void drawCube()
{
    glEnable(GL_DEPTH_TEST);

    float angle = 0.0f;
    bool hasAngle = currentAnimatedAngle(angle);
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (hasAngle)
        axisForFace(g_currentMove.face, ax, ay, az);

    for (int ix = -1; ix <= 1; ++ix)
    {
        for (int iy = -1; iy <= 1; ++iy)
        {
            for (int iz = -1; iz <= 1; ++iz)
            {
                glPushMatrix();

                // Rotate this cubie if it's on the active face layer
                if (hasAngle && cubieOnFaceLayer(ix, iy, iz, g_currentMove.face))
                {
                    glRotatef(angle, ax, ay, az);
                }

                glTranslatef(ix * CUBIE_SPACING,
                             iy * CUBIE_SPACING,
                             iz * CUBIE_SPACING);

                drawCubieWithStickers(ix, iy, iz);

                glPopMatrix();
            }
        }
    }
}

// ---------- GLUT callbacks ----------

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera transform: move back, then rotate by arcball angles
    glTranslatef(0.0f, 0.0f, -g_camDist);
    glRotatef(g_camAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_camAngleY, 0.0f, 1.0f, 0.0f);

    drawCube();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    float aspect = static_cast<float>(w) / static_cast<float>(h);

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, aspect, 0.1, 100.0);
}

// Keyboard for moves / zoom / scramble / plan-solve
void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;

    // While animating auto-solve, ignore manual moves (except ESC)
    if ((g_solutionPlaying || g_currentMoveActive) && key != 27)
        return;

    switch (key)
    {
    // Face turns: lowercase = CW, uppercase = CCW
    case 'f': g_cube.applyMove(Front, CW);  break;
    case 'F': g_cube.applyMove(Front, CCW); break;

    case 'b': g_cube.applyMove(Back,  CW);  break;
    case 'B': g_cube.applyMove(Back,  CCW); break;

    case 'u': g_cube.applyMove(Up,    CW);  break;
    case 'U': g_cube.applyMove(Up,    CCW); break;

    case 'd': g_cube.applyMove(Down,  CW);  break;
    case 'D': g_cube.applyMove(Down,  CCW); break;

    case 'l': g_cube.applyMove(Left,  CW);  break;
    case 'L': g_cube.applyMove(Left,  CCW); break;

    case 'r': g_cube.applyMove(Right, CW);  break;
    case 'R': g_cube.applyMove(Right, CCW); break;

    // Double turns
    case '1': g_cube.applyMove(Front, Double); break;
    case '2': g_cube.applyMove(Back,  Double); break;
    case '3': g_cube.applyMove(Up,    Double); break;
    case '4': g_cube.applyMove(Down,  Double); break;
    case '5': g_cube.applyMove(Left,  Double); break;
    case '6': g_cube.applyMove(Right, Double); break;

    // Scramble & reset
    case 's':
        g_cube.scramble(7);
        std::cout << "Scrambled cube.\n";
        break;
    case '0':
        g_cube = RubiksCube();
        std::cout << "Reset cube.\n";
        break;

    // Plan and play A* solution from current state
    case 'p':
    {
        std::vector<Move> sol = g_cube.solveAStar(24, 500000);
        if (sol.empty())
        {
            std::cout << "No solution found within limits.\n";
        }
        else
        {
            std::cout << "Solution length: " << sol.size() << "\n";
            g_solutionMoves     = sol;
            g_solutionIndex     = 0;
            g_solutionPlaying   = true;
            g_currentMoveActive = false;
            g_moveProgress      = 0.0f;
            g_lastTimeMs        = glutGet(GLUT_ELAPSED_TIME);
        }
        break;
    }

    // Zoom
    case '+':
    case '=':
        g_camDist -= 0.3f;
        if (g_camDist < 3.0f) g_camDist = 3.0f;
        break;
    case '-':
    case '_':
        g_camDist += 0.3f;
        break;

    // ESC to quit
    case 27:
        std::exit(0);
        break;

    default:
        break;
    }

    glutPostRedisplay();
}

// Arrow keys still adjust camera if you want
void specialKeys(int key, int x, int y)
{
    (void)x; (void)y;

    const float step = 5.0f;
    switch (key)
    {
    case GLUT_KEY_UP:    g_camAngleX -= step; break;
    case GLUT_KEY_DOWN:  g_camAngleX += step; break;
    case GLUT_KEY_LEFT:  g_camAngleY -= step; break;
    case GLUT_KEY_RIGHT: g_camAngleY += step; break;
    default: break;
    }

    glutPostRedisplay();
}

// ---------- Arcball-style mouse controls ----------

void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            g_isDragging = true;
            g_lastMouseX = x;
            g_lastMouseY = y;
        }
        else if (state == GLUT_UP)
        {
            g_isDragging = false;
        }
    }

    glutPostRedisplay();
}

void motion(int x, int y)
{
    if (!g_isDragging)
        return;

    int dx = x - g_lastMouseX;
    int dy = y - g_lastMouseY;

    const float sensitivity = 0.4f;
    g_camAngleY += dx * sensitivity;
    g_camAngleX += dy * sensitivity;

    g_lastMouseX = x;
    g_lastMouseY = y;

    glutPostRedisplay();
}

// ---------- Idle: advance animation ----------

void idle()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - g_lastTimeMs) / 5000.0f;
    g_lastTimeMs = now;

    if (!g_solutionPlaying)
        return;

    if (!g_currentMoveActive)
    {
        if (g_solutionIndex >= (int)g_solutionMoves.size())
        {
            g_solutionPlaying = false;
            return;
        }

        g_currentMove       = g_solutionMoves[g_solutionIndex++];
        g_moveProgress      = 0.0f;
        g_currentMoveActive = true;
        return;
    }

    g_moveProgress += dt / g_moveDuration;

    if (g_moveProgress >= 1.0f)
    {
        g_moveProgress = 1.0f;
        // Apply logical move to underlying cube
        g_cube.applyMove(g_currentMove.face, g_currentMove.turn);
        g_currentMoveActive = false;
    }

    glutPostRedisplay();
}

// ---------- main ----------

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Rubik's Cube Visualizer");

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    g_lastTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
