// main.cpp - Rubik's Cube visualizer with interactive heuristic UI

#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include "cube.hpp"

// ---------- Globals ----------
RubiksCube g_cube;

float g_camAngleX = 30.0f;
float g_camAngleY = -30.0f;
float g_camDist   = 6.0f;

bool g_isDragging = false;
int  g_lastMouseX = 0;
int  g_lastMouseY = 0;

// Solver / animation
std::vector<Move> g_solutionMoves;
int   g_solutionIndex       = 0;
bool  g_solutionPlaying     = false;
bool  g_currentMoveActive   = false;
Move  g_currentMove;
float g_moveProgress        = 0.0f;
const float g_moveDuration  = 0.30f;
int   g_lastTimeMs          = 0;

// UI state
int g_winW = 800, g_winH = 600;
int g_uiHeight = 180;
int g_scrambleCount = 7;
enum UITab { TAB_MANUAL = 0, TAB_HEURISTIC = 1 };
int g_activeTab = TAB_MANUAL;

std::vector<Move> g_scrambleHistory;
std::string       g_scrambleText;
std::string       g_solveText;

struct Button {
    int x0,y0,x1,y1;
    std::string label;
    std::function<void()> onClick;
    bool contains(int x,int y) const { return x>=x0 && x<=x1 && y>=y0 && y<=y1; }
};

std::vector<Button> g_buttons;

void recomputeButtons();
void startSolveAndPlay();

void enqueueAnimatedMove(Face f, Turn t)
{
    if (g_solutionPlaying || g_currentMoveActive) return;

    g_solutionMoves.clear();
    g_solutionMoves.push_back(Move{f, t});
    g_solutionIndex      = 0;
    g_solutionPlaying    = true;
    g_currentMoveActive  = false;
    g_moveProgress       = 0.0f;
    g_lastTimeMs         = glutGet(GLUT_ELAPSED_TIME);
}

// handles individual moves
std::string moveToString(const Move& m)
{
    char faceChar = ' ';
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

// handles a sequence of moves
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

void scramble(int moveCount)
{
    if (moveCount <= 0) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> faceDist(0, Face::Count - 1);
    std::uniform_int_distribution<int> turnDist(0, 2);

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

    g_solveText.clear();
    g_solutionMoves.clear();
    g_solutionPlaying   = false;
    g_currentMoveActive = false;

    std::string seqText = movesToString(thisScramble);
    if (!seqText.empty()) {
        if (!g_scrambleText.empty()) g_scrambleText += "  |  ";
        g_scrambleText += seqText;
    }
}

void recomputeButtons()
{
    g_buttons.clear();
    const int marginTop = 8, marginBottom = 8, marginSide = 8, padX = 8, padY = 6;
    int tabHeight = 30, tabWidth = 120;
    int tabY0 = g_uiHeight - tabHeight - marginTop;
    int x = marginSide;

    // creates tabs and manages switching
    auto addTab = [&](const std::string& label, UITab tabId) {
        Button b; b.x0=x; b.y0=tabY0; b.x1=x+tabWidth; b.y1=tabY0+tabHeight; b.label=label;
        b.onClick = [tabId](){ g_activeTab = tabId; recomputeButtons(); glutPostRedisplay(); };
        g_buttons.push_back(b);
        x += tabWidth + padX;
    };

    addTab("Manual",     TAB_MANUAL);
    addTab("Heuristics", TAB_HEURISTIC);

    int contentTop = tabY0 - padY;
    int contentBottom = marginBottom;
    int contentHeight = contentTop - contentBottom;

    if (g_activeTab == TAB_MANUAL)
    {
        const int rows = 3, cols = 6;
        int rowH = (contentHeight - (rows-1)*padY) / rows;
        if (rowH < 30) rowH = 30;
        int colW = 60;

        struct FaceInfo { Face f; const char* base; };
        FaceInfo faces[cols] = {{Front,"F"},{Back,"B"},{Up,"U"},{Down,"D"},{Left,"L"},{Right,"R"}};

        for (int r = 0; r < rows; ++r) {
            int y0 = contentBottom + r * (rowH + padY);
            int cx = marginSide;
            for (int c = 0; c < cols; ++c) {
                Face f = faces[c].f;
                std::string lab(faces[c].base);
                Turn t = (r==0) ? CW : ((r==1) ? CCW : Double);
                if (r==1) lab+="'"; else if (r==2) lab+="2";

                Button b; b.x0=cx; b.y0=y0; b.x1=cx+colW; b.y1=y0+rowH; b.label=lab;
                b.onClick = [f,t](){ enqueueAnimatedMove(f, t); };
                g_buttons.push_back(b);
                cx += colW + padX;
            }
            if (r == 1) {
                Button b; b.x0=cx; b.y0=y0; b.x1=cx+80; b.y1=y0+rowH; b.label="Reset";
                b.onClick = [](){
                    g_cube = RubiksCube();
                    g_scrambleHistory.clear(); g_scrambleText.clear(); g_solveText.clear();
                    g_solutionMoves.clear(); g_solutionPlaying = false; g_currentMoveActive = false;
                    glutPostRedisplay();
                };
                g_buttons.push_back(b);
            }
        }
    }
    else // TAB_HEURISTIC
    {
        int rowH = contentHeight - padY;
        if (rowH < 36) rowH = 36;
        int y0 = contentBottom + (contentHeight - rowH)/2;
        x = marginSide;

        auto addBtn = [&](int w, const std::string &lbl, std::function<void()> cb) {
            Button b; b.x0=x; b.y0=y0; b.x1=x+w; b.y1=y0+rowH; b.label=lbl; b.onClick=cb;
            g_buttons.push_back(b);
            x += w + padX;
        };

        addBtn(90, "Scramble", [](){ scramble(g_scrambleCount); glutPostRedisplay(); });
        addBtn(70, "Reset", [](){
             g_cube = RubiksCube();
             g_scrambleHistory.clear(); g_scrambleText.clear(); g_solveText.clear();
             g_solutionMoves.clear(); g_solutionPlaying = false; g_currentMoveActive = false;
             glutPostRedisplay();
        });
        addBtn(120, "Solve IDA*", [](){ startSolveAndPlay(); glutPostRedisplay(); });

        addBtn(32, "-", [](){ if (g_scrambleCount>0) g_scrambleCount--; recomputeButtons(); glutPostRedisplay(); });
        x-=padX; x+=4;
        addBtn(80, "N:"+std::to_string(g_scrambleCount), [](){});
        x-=padX; x+=4;
        addBtn(32, "+", [](){ g_scrambleCount++; recomputeButtons(); glutPostRedisplay(); });
    }
}

void drawUI()
{
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, g_winW, 0, g_uiHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.08f, 0.08f, 0.08f, 0.9f);
    drawFilledRect2D(0,0,g_winW,g_uiHeight);
    glDisable(GL_BLEND);

    for(const auto &b: g_buttons){
        glColor3f(0.22f, 0.22f, 0.22f);
        drawFilledRect2D(b.x0,b.y0,b.x1,b.y1);
        glColor3f(0.05f,0.05f,0.05f);
        glBegin(GL_LINE_LOOP);
          glVertex2i(b.x0,b.y0); glVertex2i(b.x1,b.y0);
          glVertex2i(b.x1,b.y1); glVertex2i(b.x0,b.y1);
        glEnd();
        int tx = b.x0 + 8;
        int ty = b.y0 + (b.y1 - b.y0)/2 - 9;
        glColor3f(1,1,1);
        drawText2D(tx, ty, b.label);
    }

    int textX = 8 + 2*(120 + 8) + 16;
    int textY = g_uiHeight - 22;
    glColor3f(0.8f, 0.8f, 0.8f);
    if (!g_scrambleText.empty()) {
        drawText2D(textX, textY, "Scramble: " + g_scrambleText);
        textY -= 20;
    }
    if (!g_solveText.empty()) {
        drawText2D(textX, textY, "Solve:    " + g_solveText);
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y)
{
    int uiY = g_winH - y;
    if (uiY <= g_uiHeight) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            for (auto &b : g_buttons) {
                if (b.contains(x, uiY)) { if (b.onClick) b.onClick(); return; }
            }
        }
        return;
    }
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { g_isDragging = true; g_lastMouseX = x; g_lastMouseY = y; }
        else { g_isDragging = false; }
    }
}

void motion(int x,int y)
{
    if (!g_isDragging) return;
    int dx = x - g_lastMouseX;
    int dy = y - g_lastMouseY;
    g_lastMouseX = x; g_lastMouseY = y;
    g_camAngleY += dx * 0.5f;
    g_camAngleX += dy * 0.5f;
    glutPostRedisplay();
}

void keyboard(unsigned char key,int x,int y)
{
    if ((g_solutionPlaying || g_currentMoveActive) && key != 27) return;

    switch(key){
    case 'f': enqueueAnimatedMove(Front, CW); break; case 'F': enqueueAnimatedMove(Front, CCW); break;
    case 'b': enqueueAnimatedMove(Back,  CW); break; case 'B': enqueueAnimatedMove(Back,  CCW); break;
    case 'u': enqueueAnimatedMove(Up,    CW); break; case 'U': enqueueAnimatedMove(Up,    CCW); break;
    case 'd': enqueueAnimatedMove(Down,  CW); break; case 'D': enqueueAnimatedMove(Down,  CCW); break;
    case 'l': enqueueAnimatedMove(Left,  CW); break; case 'L': enqueueAnimatedMove(Left,  CCW); break;
    case 'r': enqueueAnimatedMove(Right, CW); break; case 'R': enqueueAnimatedMove(Right, CCW); break;
    case '1': enqueueAnimatedMove(Front, Double); break; case '2': enqueueAnimatedMove(Back,  Double); break;
    case '3': enqueueAnimatedMove(Up,    Double); break; case '4': enqueueAnimatedMove(Down,  Double); break;
    case '5': enqueueAnimatedMove(Left,  Double); break; case '6': enqueueAnimatedMove(Right, Double); break;
    case 's': scramble(g_scrambleCount); break;
    case '0':
        g_cube = RubiksCube();
        g_scrambleHistory.clear(); g_scrambleText.clear(); g_solveText.clear();
        g_solutionMoves.clear(); g_solutionPlaying = false; g_currentMoveActive = false;
        break;
    case 'p': startSolveAndPlay(); break;
    case '+': case '=': g_camDist -= 0.3f; if(g_camDist<3.f) g_camDist=3.f; break;
    case '-': case '_': g_camDist += 0.3f; break;
    case 27: exit(0); break;
    }
    recomputeButtons(); glutPostRedisplay();
}

void specialKeys(int key,int x,int y)
{
    float step=5.0f;
    switch(key){
    case GLUT_KEY_UP: g_camAngleX -= step; break;
    case GLUT_KEY_DOWN: g_camAngleX += step; break;
    case GLUT_KEY_LEFT: g_camAngleY -= step; break;
    case GLUT_KEY_RIGHT: g_camAngleY += step; break;
    }
    glutPostRedisplay();
}

void updateCurrentMove(float dt)
{
    if (!g_solutionPlaying) return;
    if (!g_currentMoveActive) {
        if (g_solutionIndex >= (int)g_solutionMoves.size()) { g_solutionPlaying = false; return; }
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
    int viewH = (g_winH > g_uiHeight) ? (g_winH - g_uiHeight) : 1;
    glViewport(0, g_uiHeight, g_winW, viewH);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)g_winW / (double)viewH, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glTranslatef(0,0,-g_camDist);
    glRotatef(g_camAngleX, 1,0,0);
    glRotatef(g_camAngleY, 0,1,0);

    drawCube3D(g_cube, g_currentMoveActive, g_currentMove, g_moveProgress, g_moveDuration);

    glViewport(0,0,g_winW,g_uiHeight);
    drawUI();

    glutSwapBuffers();
}

void reshape(int w,int h)
{
    g_winW = w; g_winH = h;
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

void startSolveAndPlay()
{
    int maxIterations  = g_scrambleCount;
    int iterationDepth = g_scrambleCount;
    std::vector<Move> sol = g_cube.solveIDAStar(maxIterations, iterationDepth);

    if (sol.empty()) {
        std::cout << "IDA* found no solution within limits.\n";
        return;
    }
    std::cout << "IDA* solution length: " << sol.size() << "\n";
    g_solutionMoves = sol;
    g_solutionIndex = 0;
    g_solutionPlaying = true;
    g_currentMoveActive = false;
    g_moveProgress = 0.0f;
    g_lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
    g_solveText = movesToString(sol);
}

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