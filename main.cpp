// main.cpp - Rubik's Cube visualizer with interactive heuristic UI

#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include "cube.hpp"

// ---------- Globals (3D) ----------
RubiksCube g_cube;

float g_camAngleX = 30.0f;
float g_camAngleY = -30.0f;
float g_camDist   = 6.0f;

bool g_isDragging = false;
int  g_lastMouseX = 0;
int  g_lastMouseY = 0;

const float CUBE_HALF     = 1.0f;
const float CUBIE_SPACING = (2.0f * CUBE_HALF) / 3.0f;
const float CUBIE_SIZE    = CUBIE_SPACING * 0.92f;

// ---------- Solver / animation ----------
std::vector<Move> g_solutionMoves;
int   g_solutionIndex       = 0;
bool  g_solutionPlaying     = false;
bool  g_currentMoveActive   = false;
Move  g_currentMove;
float g_moveProgress        = 0.0f;
const float g_moveDuration  = 0.30f;
int   g_lastTimeMs          = 0;

// ---------- UI state ----------
int g_winW = 800, g_winH = 600;
int g_uiHeight = 180; // pixels for bottom UI bar (tall enough to be readable)
int g_scrambleCount = 7;

// Tabs for bottom menu
enum UITab {
    TAB_MANUAL = 0,
    TAB_HEURISTIC = 1
};
int g_activeTab = TAB_MANUAL;

// Scramble / solve histories for display
std::vector<Move> g_scrambleHistory;
std::string       g_scrambleText;
std::string       g_solveText;

struct Button {
    int x0,y0,x1,y1;       // pixel bounds (inclusive)
    std::string label;
    std::function<void()> onClick;
    bool contains(int x,int y) const {
        return x>=x0 && x<=x1 && y>=y0 && y<=y1;
    }
};

std::vector<Button> g_buttons;

// ---------- Forward declarations ----------
void drawCube3D();
void drawUI();
void recomputeButtons();
void startSolveAndPlay();

// ---------- Utilities ----------
void drawText2D(int x, int y, const std::string &s)
{
    glRasterPos2i(x,y);
    for(char c: s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawFilledRect2D(int x0,int y0,int x1,int y1)
{
    glBegin(GL_QUADS);
      glVertex2i(x0,y0);
      glVertex2i(x1,y0);
      glVertex2i(x1,y1);
      glVertex2i(x0,y1);
    glEnd();
}

// ---------- Color helper ----------
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

// ---------- Face rotation helpers for animation ----------

void axisForFace(Face f, float &ax, float &ay, float &az)
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

bool cubieOnFaceLayer(int ix,int iy,int iz, Face f)
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

bool currentAnimatedAngle(float &outAngle)
{
    if (!g_currentMoveActive) return false;
    float t = g_moveProgress / g_moveDuration;
    if (t > 1.0f) t = 1.0f;
    float baseAngle = 90.0f;
    switch(g_currentMove.turn){
        case CW:     outAngle = -baseAngle * t;    break;
        case CCW:    outAngle =  baseAngle * t;    break;
        case Double: outAngle = -2.0f * baseAngle * t; break;
    }
    return true;
}

// ---------- Accessing stickers from cube state ----------

Color stickerColorForCubieFace(int ix,int iy,int iz, Face f)
{
    const Side &side = g_cube.face(f);
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

// ---------- Drawing cubies and cube ----------

void drawCubieWithStickers(int ix,int iy,int iz)
{
    float half = CUBIE_SIZE*0.5f;
    const float stickerInset = 0.01f;

    glColor3f(0,0,0);
    glutSolidCube(CUBIE_SIZE);

    if (iz == 1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Front));
        glBegin(GL_QUADS);
         glVertex3f(-half,-half, half+stickerInset);
         glVertex3f( half,-half, half+stickerInset);
         glVertex3f( half, half, half+stickerInset);
         glVertex3f(-half, half, half+stickerInset);
        glEnd();
    }
    if (iz == -1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Back));
        glBegin(GL_QUADS);
         glVertex3f( half,-half,-half-stickerInset);
         glVertex3f(-half,-half,-half-stickerInset);
         glVertex3f(-half, half,-half-stickerInset);
         glVertex3f( half, half,-half-stickerInset);
        glEnd();
    }
    if (ix == 1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Right));
        glBegin(GL_QUADS);
         glVertex3f( half+stickerInset,-half,-half);
         glVertex3f( half+stickerInset,-half, half);
         glVertex3f( half+stickerInset, half, half);
         glVertex3f( half+stickerInset, half,-half);
        glEnd();
    }
    if (ix == -1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Left));
        glBegin(GL_QUADS);
         glVertex3f(-half-stickerInset,-half, half);
         glVertex3f(-half-stickerInset,-half,-half);
         glVertex3f(-half-stickerInset, half,-half);
         glVertex3f(-half-stickerInset, half, half);
        glEnd();
    }
    if (iy == 1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Up));
        glBegin(GL_QUADS);
         glVertex3f(-half, half+stickerInset, half);
         glVertex3f( half, half+stickerInset, half);
         glVertex3f( half, half+stickerInset,-half);
         glVertex3f(-half, half+stickerInset,-half);
        glEnd();
    }
    if (iy == -1) {
        setColor(stickerColorForCubieFace(ix,iy,iz,Down));
        glBegin(GL_QUADS);
         glVertex3f(-half,-half-stickerInset,-half);
         glVertex3f( half,-half-stickerInset,-half);
         glVertex3f( half,-half-stickerInset, half);
         glVertex3f(-half,-half-stickerInset, half);
        glEnd();
    }
}

void drawCube3D()
{
    glEnable(GL_DEPTH_TEST);
    float angle=0.0f; 
    bool hasAngle = currentAnimatedAngle(angle);
    float ax=0,ay=0,az=0;
    if (hasAngle) axisForFace(g_currentMove.face,ax,ay,az);

    for(int ix=-1; ix<=1; ++ix)
    for(int iy=-1; iy<=1; ++iy)
    for(int iz=-1; iz<=1; ++iz)
    {
        glPushMatrix();
        if(hasAngle && cubieOnFaceLayer(ix,iy,iz,g_currentMove.face))
            glRotatef(angle,ax,ay,az);
        glTranslatef(ix * CUBIE_SPACING, iy * CUBIE_SPACING, iz * CUBIE_SPACING);
        drawCubieWithStickers(ix,iy,iz);
        glPopMatrix();
    }
}

// ---------- UI code ----------

void addButton(int x0,int y0,int w,int h, const std::string &label, std::function<void()> cb)
{
    Button b; b.x0=x0; b.y0=y0; b.x1=x0+w; b.y1=y0+h; b.label=label; b.onClick=cb;
    g_buttons.push_back(std::move(b));
}

// --- Move → text helpers ---

std::string moveToString(const Move& m)
{
    char faceChar = '?';
    switch (m.face) {
    case Front: faceChar = 'F'; break;
    case Back:  faceChar = 'B'; break;
    case Up:    faceChar = 'U'; break;
    case Down:  faceChar = 'D'; break;
    case Left:  faceChar = 'L'; break;
    case Right: faceChar = 'R'; break;
    default: break;
    }

    std::string s;
    s += faceChar;
    if (m.turn == CCW)          s += "'";
    else if (m.turn == Double)  s += "2";
    return s;
}

std::string movesToString(const std::vector<Move>& seq)
{
    std::string out;
    bool first = true;
    for (const Move& m : seq) {
        if (!first) out += ' ';
        out += moveToString(m);
        first = false;
    }
    return out;
}

// --- Scramble helper that logs moves and clears old solve text ---

void doScrambleWithLog(int moveCount)
{
    if (moveCount <= 0) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> faceDist(0, Face::Count - 1);
    std::uniform_int_distribution<int> turnDist(0, 2); // 0:CW, 1:CCW, 2:Double

    std::vector<Move> thisScramble;
    thisScramble.reserve(moveCount);

    for (int i = 0; i < moveCount; ++i)
    {
        Face f  = static_cast<Face>(faceDist(rng));
        Turn t  = static_cast<Turn>(turnDist(rng));
        g_cube.applyMove(f, t);

        Move m{f,t};
        g_scrambleHistory.push_back(m);
        thisScramble.push_back(m);
    }

    // Clear any previous solution text since cube state changed
    g_solveText.clear();
    g_solutionMoves.clear();
    g_solutionPlaying   = false;
    g_currentMoveActive = false;

    std::string seqText = movesToString(thisScramble);
    if (!seqText.empty()) {
        if (!g_scrambleText.empty())
            g_scrambleText += "  |  ";
        g_scrambleText += seqText;
    }
}

void recomputeButtons()
{
    g_buttons.clear();

    const int marginTop    = 8;
    const int marginBottom = 8;
    const int marginSide   = 8;
    const int padX         = 8;
    const int padY         = 6;

    // ---- Tabs at the TOP of the UI bar ----
    int tabHeight = 30;
    int tabWidth  = 120;

    int tabY0 = g_uiHeight - tabHeight - marginTop;
    int x     = marginSide;

    auto addTab = [&](const std::string& label, UITab tabId) {
        addButton(x, tabY0, tabWidth, tabHeight, label, [tabId]() {
            g_activeTab = tabId;
            recomputeButtons();
            glutPostRedisplay();
        });
        x += tabWidth + padX;
    };

    addTab("Manual",     TAB_MANUAL);
    addTab("Heuristics", TAB_HEURISTIC);

    // ---- Content area just *below* the tabs ----
    int contentTop    = tabY0 - padY;               // just below tabs
    int contentBottom = marginBottom;
    int contentHeight = contentTop - contentBottom;

    if (g_activeTab == TAB_MANUAL)
    {
        // 3×6 grid:
        //   Row 0: CW   (F, B, U, D, L, R)
        //   Row 1: CCW  (F',B',U',D',L',R')
        //   Row 2: 2    (F2,B2,U2,D2,L2,R2)

        const int rows = 3;
        const int cols = 6;

        int rowH = (contentHeight - (rows-1)*padY) / rows;
        if (rowH < 30) rowH = 30;

        int colW = 60;

        struct FaceInfo { Face f; const char* base; };
        FaceInfo faces[cols] = {
            {Front, "F"},
            {Back,  "B"},
            {Up,    "U"},
            {Down,  "D"},
            {Left,  "L"},
            {Right, "R"}
        };

        auto labelFor = [](const char* base, int row)->std::string {
            std::string s(base);
            if      (row == 1) s += "'";
            else if (row == 2) s += "2";
            return s;
        };

        auto turnForRow = [](int row)->Turn {
            if      (row == 0) return CW;
            else if (row == 1) return CCW;
            else               return Double;
        };

        for (int r = 0; r < rows; ++r) {
            int y0 = contentBottom + r * (rowH + padY);
            int cx = marginSide;

            for (int c = 0; c < cols; ++c) {
                Face f          = faces[c].f;
                std::string lab = labelFor(faces[c].base, r);
                Turn t          = turnForRow(r);

                addButton(cx, y0, colW, rowH, lab, [f,t]() {
                    g_cube.applyMove(f,t);
                    glutPostRedisplay();
                });
                cx += colW + padX;
            }

            // Put a Reset button to the right of the middle row
            if (r == 1) {
                addButton(cx, y0, 80, rowH, "Reset", [](){
                    g_cube = RubiksCube();
                    g_scrambleHistory.clear();
                    g_scrambleText.clear();
                    g_solveText.clear();
                    g_solutionMoves.clear();
                    g_solutionPlaying   = false;
                    g_currentMoveActive = false;
                    glutPostRedisplay();
                });
            }
        }
    }
    else // TAB_HEURISTIC
    {
        int rowH = contentHeight - padY;
        if (rowH < 36) rowH = 36;

        int y0 = contentBottom + (contentHeight - rowH)/2;
        x      = marginSide;

        addButton(x, y0, 90, rowH, "Scramble", [](){
            doScrambleWithLog(g_scrambleCount);
            glutPostRedisplay();
        });
        x += 90 + padX;

        addButton(x, y0, 70, rowH, "Reset", [](){
            g_cube = RubiksCube();
            g_scrambleHistory.clear();
            g_scrambleText.clear();
            g_solveText.clear();
            g_solutionMoves.clear();
            g_solutionPlaying   = false;
            g_currentMoveActive = false;
            glutPostRedisplay();
        });
        x += 70 + padX;

        addButton(x, y0, 120, rowH, "Solve IDA* (p)", [](){
            startSolveAndPlay();
            glutPostRedisplay();
        });
        x += 120 + padX;

        // Scramble / depth controls
        addButton(x, y0, 32, rowH, "-", [](){
            if (g_scrambleCount > 0) g_scrambleCount--;
            recomputeButtons();
            glutPostRedisplay();
        });
        x += 32 + 4;

        addButton(x, y0, 80, rowH,
                  "N:" + std::to_string(g_scrambleCount),
                  [](){ /* label only */ });
        x += 80 + 4;

        addButton(x, y0, 32, rowH, "+", [](){
            g_scrambleCount++;
            recomputeButtons();
            glutPostRedisplay();
        });
        x += 32 + padX;

        // Small help text (non-interactive)
        addButton(x, y0, 220, rowH, "Drag: rotate view   |   p: Solve", [](){});
    }
}

void drawUI()
{
    // Switch to orthographic pixel coords for UI bar
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, g_winW, 0, g_uiHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // UI should ignore depth from 3D pass
    glDisable(GL_DEPTH_TEST);

    // Background bar
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.08f, 0.08f, 0.08f, 0.9f);
    drawFilledRect2D(0,0,g_winW,g_uiHeight);
    glDisable(GL_BLEND);

    // Buttons
    for(const auto &b: g_buttons){
        glColor3f(0.22f, 0.22f, 0.22f);
        drawFilledRect2D(b.x0,b.y0,b.x1,b.y1);
        glColor3f(0.05f,0.05f,0.05f);
        glBegin(GL_LINE_LOOP);
          glVertex2i(b.x0,b.y0);
          glVertex2i(b.x1,b.y0);
          glVertex2i(b.x1,b.y1);
          glVertex2i(b.x0,b.y1);
        glEnd();
        int tx = b.x0 + 8;
        int ty = b.y0 + (b.y1 - b.y0)/2 - 9;
        glColor3f(1,1,1);
        drawText2D(tx, ty, b.label);
    }

    // Scramble / solve sequences to the right of the tabs
    {
        // Tabs start at x = 8 with width 120 and padX = 8
        int textX  = 8 + 2*(120 + 8) + 16;
        int textY  = g_uiHeight - 22;

        glColor3f(0.8f, 0.8f, 0.8f);

        if (!g_scrambleText.empty()) {
            drawText2D(textX, textY, "Scramble: " + g_scrambleText);
            textY -= 20;
        }
        if (!g_solveText.empty()) {
            drawText2D(textX, textY, "Solve:    " + g_solveText);
        }
    }

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ---------- Input handling ----------

void mouse(int button, int state, int x, int y)
{
    int uiY = y;              // our UI coordinate origin is at bottom in orthographic, but
    uiY = g_winH - uiY;       // GLUT gives y from top; convert

    // Click inside UI bar?
    if (uiY <= g_uiHeight) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            for (auto &b : g_buttons) {
                if (b.contains(x, uiY)) {
                    if (b.onClick) b.onClick();
                    return;
                }
            }
        }
        return;
    }

    // Otherwise: control camera
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            g_isDragging = true;
            g_lastMouseX = x;
            g_lastMouseY = y;
        } else {
            g_isDragging = false;
        }
    }
}

void motion(int x,int y)
{
    if (!g_isDragging) return;
    int dx = x - g_lastMouseX;
    int dy = y - g_lastMouseY;
    g_lastMouseX = x;
    g_lastMouseY = y;

    g_camAngleY += dx * 0.5f;
    g_camAngleX += dy * 0.5f;
    glutPostRedisplay();
}

void keyboard(unsigned char key,int x,int y)
{
    (void)x;(void)y;
    if ((g_solutionPlaying || g_currentMoveActive) && key != 27) return;

    switch(key){
    case 'f': g_cube.applyMove(Front, CW); break;
    case 'F': g_cube.applyMove(Front, CCW); break;
    case 'b': g_cube.applyMove(Back, CW); break;
    case 'B': g_cube.applyMove(Back, CCW); break;
    case 'u': g_cube.applyMove(Up, CW); break;
    case 'U': g_cube.applyMove(Up, CCW); break;
    case 'd': g_cube.applyMove(Down, CW); break;
    case 'D': g_cube.applyMove(Down, CCW); break;
    case 'l': g_cube.applyMove(Left, CW); break;
    case 'L': g_cube.applyMove(Left, CCW); break;
    case 'r': g_cube.applyMove(Right, CW); break;
    case 'R': g_cube.applyMove(Right, CCW); break;

    case '1': g_cube.applyMove(Front, Double); break;
    case '2': g_cube.applyMove(Back, Double); break;
    case '3': g_cube.applyMove(Up, Double); break;
    case '4': g_cube.applyMove(Down, Double); break;
    case '5': g_cube.applyMove(Left, Double); break;
    case '6': g_cube.applyMove(Right, Double); break;

    case 's': doScrambleWithLog(g_scrambleCount); break;

    case '0':
        g_cube = RubiksCube();
        g_scrambleHistory.clear();
        g_scrambleText.clear();
        g_solveText.clear();
        g_solutionMoves.clear();
        g_solutionPlaying   = false;
        g_currentMoveActive = false;
        break;

    case 'p': startSolveAndPlay(); break;

    case '+': case '=': g_camDist -= 0.3f; if(g_camDist<3.f) g_camDist=3.f; break;
    case '-': case '_': g_camDist += 0.3f; break;
    case 27: exit(0); break;
    default: break;
    }
    recomputeButtons();
    glutPostRedisplay();
}

void specialKeys(int key,int x,int y)
{
    (void)x;(void)y;
    const float step=5.0f;
    switch(key){
    case GLUT_KEY_UP:    g_camAngleX -= step; break;
    case GLUT_KEY_DOWN:  g_camAngleX += step; break;
    case GLUT_KEY_LEFT:  g_camAngleY -= step; break;
    case GLUT_KEY_RIGHT: g_camAngleY += step; break;
    default: break;
    }
    glutPostRedisplay();
}

// ---------- Animation / display ----------

void updateCurrentMove(float dt)
{
    if (!g_solutionPlaying) return;

    if (!g_currentMoveActive) {
        if (g_solutionIndex >= (int)g_solutionMoves.size()) {
            g_solutionPlaying = false;
            return;
        }
        g_currentMove = g_solutionMoves[g_solutionIndex];
        g_currentMoveActive = true;
        g_moveProgress = 0.0f;
    }

    g_moveProgress += dt;
    if (g_moveProgress >= g_moveDuration) {
        g_cube.applyMove(g_currentMove.face, g_currentMove.turn);
        g_currentMoveActive = false;
        g_solutionIndex++;
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3D viewport
    int viewH = (g_winH > g_uiHeight) ? (g_winH - g_uiHeight) : 1;
    glViewport(0, g_uiHeight, g_winW, viewH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)g_winW / (double)viewH, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0,0,-g_camDist);
    glRotatef(g_camAngleX, 1,0,0);
    glRotatef(g_camAngleY, 0,1,0);

    drawCube3D();

    // Bottom UI bar
    glViewport(0,0,g_winW,g_uiHeight);
    drawUI();

    glutSwapBuffers();
}

void reshape(int w,int h)
{
    g_winW = w;
    g_winH = h;
    recomputeButtons();
    glutPostRedisplay();
}

void idle()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - g_lastTimeMs) * 0.001f;
    g_lastTimeMs = now;
    updateCurrentMove(dt);
    glutPostRedisplay();
}

// ---------- Solver trigger (IDA*) ----------

void startSolveAndPlay()
{
    int maxIterations  = -1;              // endless IDA* iterations
    int iterationDepth = g_scrambleCount; // depth limit matches scramble length

    std::vector<Move> sol = g_cube.solveIDAStar(maxIterations, iterationDepth);

    if (sol.empty()) {
        std::cout << "IDA* found no solution within limits "
                  << "(iterations=" << maxIterations
                  << ", depth=" << iterationDepth << ").\n";
        return;
    }

    std::cout << "IDA* solution length: " << sol.size()
              << " (depth limit " << iterationDepth << ")\n";

    g_solutionMoves      = sol;
    g_solutionIndex      = 0;
    g_solutionPlaying    = true;
    g_currentMoveActive  = false;
    g_moveProgress       = 0.0f;
    g_lastTimeMs         = glutGet(GLUT_ELAPSED_TIME);

    // Log the solution moves for display
    g_solveText = movesToString(sol);
}

// ---------- Main ----------
int main(int argc,char** argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g_winW,g_winH);
    glutCreateWindow("Rubik's Cube - Interactive Visualizer");

    glClearColor(0.15f,0.15f,0.18f,1.0f);
    glEnable(GL_DEPTH_TEST);

    g_lastTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idle);

    recomputeButtons();
    glutMainLoop();
    return 0;
}
