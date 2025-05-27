#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <limits>
#include <algorithm>
#include <array>
#include <random>
#include <cstdint>
#include <stack>

using namespace std;

const int BOARD_SIZE = 8;
enum PieceType { EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
enum Color { NONE, WHITE, BLACK };

struct Piece {
    PieceType type;
    Color color;
    Piece(PieceType t = EMPTY, Color c = NONE) : type(t), color(c) {}
    char symbol() const {
        if (color == NONE) return '.';
        char s;
        switch (type) {
        case PAWN: s = 'P'; break;
        case ROOK: s = 'R'; break;
        case KNIGHT: s = 'N'; break;
        case BISHOP: s = 'B'; break;
        case QUEEN: s = 'Q'; break;
        case KING: s = 'K'; break;
        default: s = '.'; break;
        }
        return color == WHITE ? s : tolower(s);
    }
};

struct Move {
    int fromX, fromY, toX, toY;
    bool operator==(const Move& m) const {
        return fromX == m.fromX && fromY == m.fromY && toX == m.toX && toY == m.toY;
    }
};

typedef array<array<Piece, BOARD_SIZE>, BOARD_SIZE> Board;

// --- Новая структура дерева ходов ---
struct TreeNode {
    Board board;
    Move move;
    vector<TreeNode*> children;

    TreeNode(const Board& b, const Move& m) : board(b), move(m) {}
    ~TreeNode() {
        for (auto child : children)
            delete child;
    }
};

bool isInside(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

void makeMove(Board& board, const Move& m) {
    Piece moving = board[m.fromY][m.fromX];
    if (moving.type == PAWN && (m.toY == 0 || m.toY == 7))
        board[m.toY][m.toX] = Piece(QUEEN, moving.color);
    else
        board[m.toY][m.toX] = moving;
    board[m.fromY][m.fromX] = Piece();
}

// --- Простой стек истории ходов ---
stack<Move> moveHistory;

void undoMove(Board& board, const Move& m, const Piece& captured) {
    board[m.fromY][m.fromX] = board[m.toY][m.toX];
    board[m.toY][m.toX] = captured;
    moveHistory.pop();
}

Board simulateMove(Board board, const Move& m) {
    makeMove(board, m);
    return board;
}

bool isInCheck(const Board& board, Color color);

vector<Move> generateLegalMoves(const Board& b, Color turn, bool filterCheck) {
    vector<Move> moves;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            const Piece& p = b[y][x];
            if (p.color != turn) continue;

            if (p.type == PAWN) {
                int dir = (turn == WHITE ? -1 : 1);
                int ny = y + dir;
                if (isInside(x, ny) && b[ny][x].type == EMPTY)
                    moves.push_back({ x, y, x, ny });
                if ((turn == WHITE && y == 6) || (turn == BLACK && y == 1)) {
                    int jumpY = y + 2 * dir;
                    if (isInside(x, jumpY) && b[ny][x].type == EMPTY && b[jumpY][x].type == EMPTY)
                        moves.push_back({ x, y, x, jumpY });
                }
                int dxs[] = { -1, 1 };
                for (int dx : dxs) {
                    int nx = x + dx;
                    if (isInside(nx, ny) && b[ny][nx].color != NONE && b[ny][nx].color != turn)
                        moves.push_back({ x, y, nx, ny });
                }
            }
            else if (p.type == KNIGHT) {
                pair<int, int> dirs[] = { {1,2},{2,1},{-1,2},{-2,1},{1,-2},{2,-1},{-1,-2},{-2,-1} };
                for (auto& d : dirs) {
                    int nx = x + d.first, ny = y + d.second;
                    if (isInside(nx, ny) && b[ny][nx].color != turn)
                        moves.push_back({ x, y, nx, ny });
                }
            }
            else if (p.type == KING) {
                for (int dx = -1; dx <= 1; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                        if (dx || dy) {
                            int nx = x + dx, ny = y + dy;
                            if (isInside(nx, ny) && b[ny][nx].color != turn)
                                moves.push_back({ x, y, nx, ny });
                        }
            }
            else {
                vector<pair<int, int>> dirs;
                if (p.type == ROOK || p.type == QUEEN)
                    dirs.insert(dirs.end(), { {0,1}, {1,0}, {0,-1}, {-1,0} });
                if (p.type == BISHOP || p.type == QUEEN)
                    dirs.insert(dirs.end(), { {1,1}, {-1,1}, {1,-1}, {-1,-1} });

                for (auto& d : dirs) {
                    int dx = d.first, dy = d.second;
                    int nx = x + dx, ny = y + dy;
                    while (isInside(nx, ny)) {
                        if (b[ny][nx].color == turn) break;
                        moves.push_back({ x, y, nx, ny });
                        if (b[ny][nx].color != NONE) break;
                        nx += dx;
                        ny += dy;
                    }
                }
            }
        }
    }

    if (!filterCheck) return moves;

    vector<Move> filtered;
    for (const auto& move : moves)
        if (!isInCheck(simulateMove(b, move), turn))
            filtered.push_back(move);

    return filtered;
}

bool isInCheck(const Board& board, Color color) {
    int kingX = -1, kingY = -1;
    for (int y = 0; y < BOARD_SIZE; ++y)
        for (int x = 0; x < BOARD_SIZE; ++x)
            if (board[y][x].type == KING && board[y][x].color == color) {
                kingX = x;
                kingY = y;
            }
    if (kingX == -1) return true;
    Color opponent = (color == WHITE ? BLACK : WHITE);
    auto opponentMoves = generateLegalMoves(board, opponent, false);
    for (const auto& move : opponentMoves)
        if (move.toX == kingX && move.toY == kingY)
            return true;
    return false;
}

void initBoard(Board& board) {
    PieceType order[] = { ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };
    for (int y = 0; y < BOARD_SIZE; ++y)
        for (int x = 0; x < BOARD_SIZE; ++x)
            board[y][x] = Piece();
    for (int x = 0; x < BOARD_SIZE; ++x) {
        board[0][x] = Piece(order[x], BLACK);
        board[1][x] = Piece(PAWN, BLACK);
        board[6][x] = Piece(PAWN, WHITE);
        board[7][x] = Piece(order[x], WHITE);
    }
}

void printBoard(const Board& board) {
    cout << "  a b c d e f g h\n";
    for (int y = 0; y < BOARD_SIZE; ++y) {
        cout << 8 - y << ' ';
        for (int x = 0; x < BOARD_SIZE; ++x)
            cout << board[y][x].symbol() << ' ';
        cout << 8 - y << '\n';
    }
    cout << "  a b c d e f g h\n";
}

int evaluateBoard(const Board& board) {
    int score = 0;
    for (int y = 0; y < BOARD_SIZE; ++y)
        for (int x = 0; x < BOARD_SIZE; ++x) {
            const Piece& p = board[y][x];
            if (p.color == NONE) continue;
            int val = 0;
            switch (p.type) {
            case PAWN: val = 10; break;
            case KNIGHT:
            case BISHOP: val = 30; break;
            case ROOK: val = 50; break;
            case QUEEN: val = 90; break;
            case KING: val = 900; break;
            default: break;
            }
            score += (p.color == WHITE ? val : -val);
        }
    return score;
}

// Minimax hat im Worst Case O(b^d), mit b = durchschnittliche Züge (~30), d = Suchtiefe (z. B. 3)
int minimax(Board board, int depth, bool maximizing, int alpha, int beta) {
    Color turn = maximizing ? WHITE : BLACK;
    auto moves = generateLegalMoves(board, turn, true);
    if (depth == 0 || moves.empty())
        return evaluateBoard(board);

    int best = maximizing ? INT_MIN : INT_MAX;
    for (const auto& move : moves) {
        Board copy = simulateMove(board, move);
        int score = minimax(copy, depth - 1, !maximizing, alpha, beta);
        if (maximizing) {
            best = max(best, score);
            alpha = max(alpha, best);
        }
        else {
            best = min(best, score);
            beta = min(beta, best);
        }
        if (beta <= alpha) break;
    }
    return best;
}

Move findBestMove(Board& board, Color aiColor) {
    auto moves = generateLegalMoves(board, aiColor, true);
    if (moves.empty()) return { 0,0,0,0 };
    int bestScore = aiColor == WHITE ? INT_MIN : INT_MAX;
    Move bestMove = moves[0];
    for (const auto& move : moves) {
        Board copy = simulateMove(board, move);
        int score = minimax(copy, 3, aiColor == WHITE, -10000, 10000);
        if ((aiColor == WHITE && score > bestScore) || (aiColor == BLACK && score < bestScore)) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}

void playGame() {
    Board board;
    initBoard(board);
    Color turn = WHITE;
    while (true) {
        printBoard(board);
        auto legalMoves = generateLegalMoves(board, turn, true);

        if (legalMoves.empty()) {
            if (isInCheck(board, turn)) {
                cout << (turn == WHITE ? "Schwarz" : "Weiß") << " gewinnt durch Matt!\n";
            }
            else {
                cout << "Patt! Unentschieden.\n";
            }
            break;
        }

        if (turn == WHITE) {
            cout << "Weißer Zug (z.B. e2e4): ";
            string input;
            cin >> input;
            if (input.length() != 4) {
                cout << "Ungültiges Format.\n";
                continue;
            }
            Move m = { input[0] - 'a', 8 - (input[1] - '0'), input[2] - 'a', 8 - (input[3] - '0') };
            bool valid = false;
            for (const auto& move : legalMoves) {
                if (move == m) {
                    moveHistory.push(m);
                    makeMove(board, m);
                    valid = true;
                    break;
                }
            }
            if (!valid) {
                cout << "Ungültiger Zug.\n";
                continue;
            }
            if (isInCheck(board, BLACK)) {
                cout << "Schwarz steht im Schach!\n";
            }
            turn = BLACK;
        }
        else {
            cout << "Schwarzer Zug ...\n";
            Move m = findBestMove(board, BLACK);
            if (!(m.fromX == 0 && m.fromY == 0 && m.toX == 0 && m.toY == 0)) {
                moveHistory.push(m);
                makeMove(board, m);
                cout << "Schwarz spielt: " << (char)('a' + m.fromX) << (8 - m.fromY)
                    << (char)('a' + m.toX) << (8 - m.toY) << endl;
            }
            if (isInCheck(board, WHITE)) {
                cout << "Weiß steht im Schach!\n";
            }
            turn = WHITE;
        }
    }
}

int main() {
    playGame();
    return 0;
}
