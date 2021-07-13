#include <cstring>
#include <climits>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

using std::endl;
using std::getline;
using std::string;
using std::stringstream;
using std::to_wstring;
using std::vector;
using std::unordered_map;
using std::wcout;
using std::wcin;
using std::wstring;

#define MAX_EVAL_CACHE_ENTRIES 1000000
#define USE_EVAL_CACHE 1

struct EvalCacheEntry {
    int score;
    int depth;
    string sfen; // check for conflicts
};

struct Board;

unordered_map<string,unsigned int> evalCacheHash;
vector<EvalCacheEntry> evalCache;
unsigned int maxEvalCacheEntries;
unsigned int evalCachePointer;
unsigned long evalCacheHit;
unsigned long evalCacheMiss;
bool evalCacheIsInit=false;

enum PieceType {
    Empty = 0,
    Pawn = 0b00000100,
    Knight = 0b00001000,
    Bishop = 0b00001100,
    Rook = 0b00010000,
    Queen = 0b00010100,
    King = 0b00011000
};
enum Color { None = 0, White = 0b00000001, Black = 0b00000010 };
enum Piece {
    bp = (PieceType::Pawn | Color::Black),
    bn = (PieceType::Knight | Color::Black),
    bb = (PieceType::Bishop | Color::Black),
    br = (PieceType::Rook | Color::Black),
    bq = (PieceType::Queen | Color::Black),
    bk = (PieceType::King | Color::Black),
    wp = (PieceType::Pawn | Color::White),
    wn = (PieceType::Knight | Color::White),
    wb = (PieceType::Bishop | Color::White),
    wr = (PieceType::Rook | Color::White),
    wq = (PieceType::Queen | Color::White),
    wk = (PieceType::King | Color::White),
};
enum CastleRights { NoCastling = 0, CWK = 1, CWQ = 2, CBK = 4, CBQ = 8 };

wstring stringenc(string s) {
#ifdef USE_UTF8
    return s
#else
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(s);
#endif
}

class Term {
    Term() {
    }
    ~Term() {
    }

  public:
    static void cc(int col, int bk) {
        unsigned char esc = 27;
        int c = col + 30;
        int b = bk + 40;
        char s[256];
        // sprintf(s, "%c[%d;%dm", esc, c, b);
        sprintf(s, "\033[%d;%dm", c, b);
        wcout << stringenc(s);
    }

    // Clear screen
    static void clr() {
        unsigned char esc = 27;
        char s[256];
        // sprintf(s, "%c[%dJ", esc, 2);
        sprintf(s, "\033[%dJ", 2);
        wcout << stringenc(s);
    }

    // Cursor goto(line,column)
    static void got(int cy, int cx) {
        unsigned char esc = 27;
        char s[256];
        // sprintf(s, "%c[%d;%dH", esc, cy + 1, cx + 1);
        sprintf(s, "\033[%d;%dH", cy + 1, cx + 1);
        wcout << stringenc(s);
    }
};

#define MAX_EVAL (INT_MAX-3)
#define MIN_EVAL (- MAX_EVAL)
#define INVALID_EVAL INT_MIN


struct Board {
    unsigned char field[120];
    Color activeColor;
    unsigned char hasCastled;
    unsigned char castleRights;
    unsigned char fiftyMoves;
    unsigned char epPos;
    unsigned int moveNumber;

    Board() {
        startPosition();
    }

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    Board(string fen, bool verbose = false) {
        emptyBoard();
        activeColor = Color::White;
        epPos = 0;
        fiftyMoves = 0;
        moveNumber = 0;
        vector<string> parts = split(fen, ' ');
        if (parts.size() < 3) {
            if (verbose)
                wcout << L"Missing parts of fen, invalid!" << endl;
            return;
        }
        vector<string> rows = split(parts[0], '/');
        if (rows.size() != 8) {
            if (verbose)
                wcout << L"Invalid number of rows in fen, invalid!" << endl;
            return;
        }
        for (int y = 7; y >= 0; y--) {
            int x = 0;

            for (int i = 0; i < rows[7 - y].length(); i++) {
                if (x > 7) {
                    if (verbose)
                        wcout << L"Invalid position encoding! Invalid!" << endl;
                    return;
                }
                char c = rows[7 - y][i];
                if (c >= '1' && c <= '8') {
                    for (int j = 0; j < c - '0'; j++)
                        ++x;
                } else {
                    field[toPos(y, x)] = asc2piece(c);
                    x += 1;
                }
            }
        }
        if (parts[1] == "w") {
            activeColor = Color::White;
        } else {
            if (parts[1] == "b") {
                activeColor = Color::Black;
            } else {
                if (verbose)
                    wcout << L"Illegal color, invalid" << endl;
                activeColor = Color::White;
            }
        }
        if (parts[2].find('K') != std::string::npos)
            castleRights |= CastleRights::CWK;
        if (parts[2].find('Q') != std::string::npos)
            castleRights |= CastleRights::CWQ;
        if (parts[2].find('k') != std::string::npos)
            castleRights |= CastleRights::CBK;
        if (parts[2].find('q') != std::string::npos)
            castleRights |= CastleRights::CBQ;
        if (parts.size() > 3) {
            if (parts[3] != "-") {
                epPos = toPos(parts[3]);
            }
        }
        if (parts.size() > 4) {
            if (parts[4] != "-") {
                fiftyMoves = stoi(parts[4]);
            } else
                fiftyMoves = 0;
        }
        if (parts.size() > 5) {
            if (parts[5] != "-") {
                moveNumber = stoi(parts[5]);
            } else
                moveNumber = 1;
        }
    }

    string fen(bool shortFen=false) {
        string f = "";
        for (int y = 7; y >= 0; y--) {
            int bl = 0;
            for (int x = 0; x < 8; x++) {
                unsigned char pc = field[toPos(y, x)];
                if (pc == 0)
                    ++bl;
                else {
                    if (bl > 0) {
                        f += std::to_string(bl);
                        bl = 0;
                    }
                    f += string(1, piece2asc(pc));
                }
            }
            if (bl > 0) {
                f += std::to_string(bl);
                bl = 0;
            }
            if (y > 0)
                f += "/";
        }
        f += " ";
        if (activeColor == Color::Black)
            f += "b ";
        else
            f += "w ";
        string cr = "";
        if (castleRights & CastleRights::CWK)
            cr += "K";
        if (castleRights & CastleRights::CWQ)
            cr += "Q";
        if (castleRights & CastleRights::CBK)
            cr += "k";
        if (castleRights & CastleRights::CBQ)
            cr += "q";
        if (cr == "")
            cr = "-";
        f += cr;
        if (shortFen) return f;
        f += " ";
        string ep = "";
        if (epPos != 0) {
            ep = pos2string(epPos);
        } else
            ep = "-";
        f += ep + " ";
        f += std::to_string((int)fiftyMoves) + " ";
        f += std::to_string((int)moveNumber);
        return f;
    }

    static unsigned char asc2piece(char c) {
        const string wp = "PNBRQK";
        const string bp = "pnbrqk";
        unsigned char pc = 0;
        int ind = wp.find(c);
        if (ind != std::string::npos) {
            pc = ((ind + 1) << 2) | Color::White;
        } else {
            ind = bp.find(c);
            if (ind != std::string::npos) {
                pc = ((ind + 1) << 2) | Color::Black;
            }
        }
        return pc;
    }

    static PieceType asc2piecetype(char c) {
        const string bp = "pnbrqk";
        PieceType pt = PieceType::Empty;
        int ind = bp.find(std::tolower(c));
        if (ind != std::string::npos) {
            pt = (PieceType)(ind + 1);
        }
        return pt;
    }

    static char piece2asc(unsigned char pc) {
        const char *wp = "PNBRQK";
        const char *bp = "pnbrqk";
        if (pc & Color::Black) {
            return bp[(pc >> 2) - 1];
        } else {
            return wp[(pc >> 2) - 1];
        }
    }

    vector<string> split(const string &s, char delim) {
        vector<string> result;
        stringstream ss(s);
        string item;
        while (getline(ss, item, delim))
            result.push_back(item);
        return result;
    }

    void startPosition() {
        unsigned char startFigs[] = {PieceType::Rook,   PieceType::Knight, PieceType::Bishop,
                                     PieceType::Queen,  PieceType::King,   PieceType::Bishop,
                                     PieceType::Knight, PieceType::Rook};
        emptyBoard();
        castleRights =
            CastleRights::CWK | CastleRights::CWQ | CastleRights::CBK | CastleRights::CBQ;
        activeColor = Color::White;
        epPos = 0;
        fiftyMoves = 0;
        moveNumber = 1;
        for (int i = 0; i < 8; i++) {
            field[toPos(0, i)] = startFigs[i] + Color::White;
            field[toPos(7, i)] = startFigs[i] + Color::Black;
            field[toPos(1, i)] = Piece::wp;
            field[toPos(6, i)] = Piece::bp;
        }
    }

    void emptyBoard() {
        memset(field, 0xff, 120);
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                field[toPos(y, x)] = 0;
            }
        }
        castleRights = 0;
        activeColor = None;
        epPos = 0;
        fiftyMoves = 0;
        moveNumber = 0;
        hasCastled = 0;
    }

    static inline unsigned char toPos(unsigned char y, unsigned char x) {
        return ((y + 2) * 10 + x + 1);
    }
    static inline unsigned char toPos(string posStr) {
        return ((posStr[1] - '1' + 2) * 10 + std::tolower(posStr[0]) - 'a' + 1);
    }

    static inline Color invertColor(unsigned char c) {
        if (c & White)
            return Black;
        if (c & Black)
            return White;
        return None;
    }

    static void pos2coord(unsigned char pos, int *y, int *x) {
        if (y)
            *y = pos / 10 - 2;
        if (x)
            *x = (pos % 10) - 1;
    }

    static string pos2string(unsigned char pos) {
        char c1 = pos / 10 - 2 + '1';
        char c2 = (pos % 10) - 1 + 'a';
        string coord = string(1, c2) + string(1, c1);
        return coord;
    }

    void printPos(Board *brd, int py = 0, int px = 0) {
        wchar_t *cPw = (wchar_t *)L"♟♞♝♜♛♚";
        wchar_t *cPb = (wchar_t *)L"♙♘♗♖♕♔";
        int cy, cx, fo, cl, cf;
        char f;
        wchar_t c;
        if (py != -1)
            Term::got(py, px);
        for (cy = 7; cy >= 0; cy--) {
            if (py != -1)
                Term::got(py + 7 - cy, px);
            for (cx = 0; cx < 8; cx++) {
                f = brd->field[toPos(cy, cx)];
                cl = (cx + cy) % 2;
                if (cl)
                    Term::cc(7, 0);
                else
                    Term::cc(0, 7);
                if (cl == 0)
                    cl = -1;
                if (f == 0) {
                    wcout << "   ";
                } else {
                    fo = f >> 2;
                    if (f & Color::Black)
                        cf = -1;
                    else
                        cf = 1;
                    if (cl * cf > 0)
                        c = (wchar_t)cPw[fo - 1];
                    else
                        c = (wchar_t)cPb[fo - 1];
                    wcout << L" " << c << L" ";
                }
            }
            Term::cc(7, 0);
            wcout << endl;
        }
    }

    void printInfo() {
        if (activeColor == Color::White)
            wcout << L"Active: White" << endl;
        else if (activeColor == Color::Black)
            wcout << L"Active: Black" << endl;
        else
            wcout << L"Active color is undefined!" << endl;

        wcout << L"Castle-rights: ";
        if (castleRights & CastleRights::CWK)
            wcout << L"White-King ";
        if (castleRights & CastleRights::CWQ)
            wcout << L"White-Queen ";
        if (castleRights & CastleRights::CBK)
            wcout << L"Black-King ";
        if (castleRights & CastleRights::CBQ)
            wcout << L"Black-Queen ";
        wcout << endl;

        if (epPos == 0)
            wcout << L"No enpassant" << endl;
        else {
            string ep = pos2string(epPos);
            wcout << L"Enpassant: [" << to_wstring((int)epPos) << L"] " << stringenc(ep) << endl;
        }
        wcout << L"Fifty-move-state: " << to_wstring((int)fiftyMoves) << endl;
        wcout << L"Move-number: " << to_wstring((int)moveNumber) << endl;
    }

    struct Move {
        unsigned char from;
        unsigned char to;
        PieceType promote;
        int eval;
        bool isCheck;
        Move() {
            from = 0;
            to = 0;
            promote = Empty;
            eval = INVALID_EVAL;
            isCheck = false;
        }
        Move(unsigned char from, unsigned char to, PieceType promote = Empty)
            : from(from), to(to), promote(promote) {
            eval=INVALID_EVAL;
            isCheck=false;
        }
        // Copy constructor
        Move(const Move &mv) {
            from = mv.from;
            to = mv.to;
            promote = mv.promote;
            eval = mv.eval;
            isCheck = mv.isCheck;
        }

        // Decode uci and algebraic-notation strings to move [TODO: incomplete]
        Move(string alg, Board *brd = nullptr) {
            promote = Empty;
            isCheck = false;
            from = 0;
            to = 0;
            eval = INVALID_EVAL;
            if (alg.length() >= 5 && alg[2] == '-') {  // UCI Format
                from = Board::toPos(alg.substr(0, 2));
                to = Board::toPos(alg.substr(3, 2));
                if (alg.length() >= 6 && alg[5] != '+' && alg[5] != '#') {
                    promote = Board::asc2piecetype(alg[5]);
                }
            } else {  // Algebraic notation, needs board
                if (brd == nullptr)
                    return;
                string al = alg;
                if (alg == "O-O") {
                    if (brd->activeColor == Color::White) {
                        from = Board::toPos("e1");
                        to = Board::toPos("g1");
                    } else if (brd->activeColor == Color::Black) {
                        from = Board::toPos("e8");
                        to = Board::toPos("g8");
                    }
                } else if (alg == "O-O-O") {
                    if (brd->activeColor == Color::White) {
                        from = Board::toPos("e1");
                        to = Board::toPos("c1");
                    } else if (brd->activeColor == Color::Black) {
                        from = Board::toPos("e8");
                        to = Board::toPos("c8");
                    }
                } else {
                    if (al.length() < 2)
                        return;
                    const string pt = "NBRQK";
                    PieceType p;
                    if (pt.find(al[0]) != std::string::npos) {
                        p = Board::asc2piecetype(al[0]);
                        al = al.substr(1);
                    } else {
                        p = PieceType::Pawn;
                    }
                    if (al.length() < 2)
                        return;
                    if (al[0] == 'x') {
                        al = al.substr(1);
                    }
                    if (al.length() < 2)
                        return;
                    int fx = -1, fy = -1, tx = -1, ty = -1;
                    if (al[1] >= 'a' && al[1] <= 'h') {
                        tx = al[1] - 'a';
                        fx = al[0] - 'a';
                        al = al.substr(2);
                    } else {
                        tx = al[0] - 'a';
                        al = al.substr(1);
                    }
                    if (al.length() < 1)
                        return;
                    if (al.length() >= 2) {
                        if (al[1] >= '1' && al[1] <= '8') {
                            ty = al[1] - '1';
                            fy = al[1] - '1';
                            al = al.substr(2);
                        } else {
                            ty = al[0] - '1';
                            al = al.substr(1);
                        }
                    }
                    if (al.length() >= 2) {
                        if (al[0] == '=') {
                            promote = Board::asc2piecetype(al[1]);
                        }
                    }
                    // TODO: algebraic decoding missing.
                }
            }
        }

        string toUci() {
            string uci = Board::pos2string(from) + "-" + pos2string(to);
            if (promote != Empty) {
                uci += string(1, piece2asc(promote));
            }
            return uci;
        }

        string toUciWithEval() {
            string uci = Board::pos2string(from) + "-" + pos2string(to);
            if (promote != Empty) {
                uci += string(1, piece2asc(promote));
            }
            if (eval != INVALID_EVAL) {
                uci += " (" + std::to_string(eval) + ")";
            }
            return uci;
        }
    };

    bool attacked(unsigned char pos, Color col) {
        // TODO: optimize by copying code and directly returning bool
        vector<unsigned char> pl = attackers(pos, col);
        if (pl.size() > 0)
            return true;
        else
            return false;
    }

    vector<unsigned char> attackers(unsigned char pos, Color col) {
        vector<unsigned char> pl;
        Color attColor;
        if (col == Color::White)
            attColor = Black;
        else
            attColor = White;

        if (col == White) {
            if (field[pos + 11] == (Pawn | Black)) {
                pl.push_back(pos + 11);
            }
            if (field[pos + 9] == (Pawn | Black)) {
                pl.push_back(pos + 9);
            }
        } else {
            if (field[pos - 11] == (Pawn | White)) {
                pl.push_back(pos - 11);
            }
            if (field[pos - 9] == (Pawn | White)) {
                pl.push_back(pos - 9);
            }
        }
        signed char knightMoves[]{21, -21, 19, -19, 8, 12, -8, -12};
        signed char bishopMoves[]{11, -11, 9, -9};
        signed char rookMoves[]{10, -10, 1, -1};
        signed char kingQueenMoves[]{11, -11, 9, -9, 10, -10, 1, -1};
        for (signed char kn : knightMoves) {
            if (field[pos + kn] == (PieceType::Knight | attColor))
                pl.push_back(pos + kn);
        }
        for (signed char km : kingQueenMoves) {
            if (field[pos + km] == (PieceType::King | attColor))
                pl.push_back(pos + km);
        }
        for (signed char bm : bishopMoves) {
            unsigned char pn = pos + bm;
            if (field[pn] == 0xff)
                continue;
            while (true) {
                if ((field[pn] == (PieceType::Bishop | attColor)) ||
                    (field[pn] == (PieceType::Queen | attColor))) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn] != 0)
                    break;
                pn = pn + bm;
                if (field[pn] == 0xff)
                    break;
            }
        }
        for (signed char bm : rookMoves) {
            unsigned char pn = pos + bm;
            if (field[pn] == 0xff)
                continue;
            while (true) {
                if ((field[pn] == (PieceType::Rook | attColor)) ||
                    (field[pn] == (PieceType::Queen | attColor))) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn] != 0)
                    break;
                pn = pn + bm;
                if (field[pn] == 0xff)
                    break;
            }
        }
        return pl;
    }

    vector<unsigned char> attackedBy(unsigned char pos, Color col) {
        vector<unsigned char> apl;
        vector<unsigned char> posl = attackers(pos, col);
        for (unsigned char pi : posl)
            apl.push_back(field[pi] >> 2);
        std::sort(apl.begin(), apl.end());
        return apl;
    }

    unsigned char kingsPos(Color col) {
        for (unsigned char c = 21; c < 99; c++) {
            if (field[c] == (PieceType::King | col))
                return c;
        }
        return 0;
    }

    bool inCheck(Color col) {
        unsigned char kpos = kingsPos(col);
        vector<unsigned char> pl = attackers(kpos, col);
        if (pl.size() == 0)
            return false;
        else
            return true;
    }

    static void resetEvalCache() {
        evalCache.clear();
        evalCacheHash.clear();
        maxEvalCacheEntries=MAX_EVAL_CACHE_ENTRIES;
        evalCachePointer=0;
        evalCacheHit=0;
        evalCacheMiss=0;
        evalCacheIsInit=true;
    }

    static void pushEvalCache(Board brd, int depth, int score) {
        unsigned int ind;
        EvalCacheEntry ece;
        std::unordered_map<string,unsigned int>::iterator it;
        string sfen=brd.fen(true);
        #ifndef USE_EVAL_CACHE
        return;
        #endif
        if (!evalCacheIsInit) resetEvalCache();
        it=evalCacheHash.find(sfen);
        if (it!=evalCacheHash.end()) {
            ind=evalCacheHash[sfen];
            if (ind >= evalCache.size()) {
                wcout << L"bad evalCache index out of range!, fix!" << endl;
                exit(-1);
            }
            ece=evalCache[ind];
            if (sfen!=ece.sfen) {
                wcout << L"bad evalCache update for wrong sfen, fix!" << endl;
                exit(-1);
            }
            if (depth<ece.depth) {
                //wcout << L"bad cache write: better entry already available, fix!" << endl;
                //exit(-1);
                return;
            } else {
                if (ece.sfen != sfen) {
                    wcout << L"bad cache update: linked entry with wrong sfen, fix!" << endl;
                    exit(-1);                    
                }
                ece.depth=depth;
                ece.score=score;
                evalCache[ind]=ece;
            }
        } else { 
            if (evalCache.size()==maxEvalCacheEntries) {
                ece=evalCache[evalCachePointer];
                it=evalCacheHash.find(ece.sfen);
                if (it!=evalCacheHash.end()) { 
                    evalCacheHash.erase(it);
                    evalCacheHash[sfen]=evalCachePointer;
                    ece.depth=depth;
                    ece.score=score;
                    ece.sfen=sfen;
                    evalCache[evalCachePointer]=ece;
                    evalCachePointer=(evalCachePointer+1)%maxEvalCacheEntries;
                } else {
                    wcout << L"hash table inconsistent, please fix first!" << endl;
                    exit(-1);
                }
            } else {
                ece.depth=depth;
                ece.score=score;
                ece.sfen=sfen;
                evalCache.push_back(ece);
                ind=evalCache.size()-1;
                evalCacheHash[sfen]=ind;
            }
        }
    }

    static bool readEvalCache(Board brd, int depth, int *pScore, int *pCacheDepth=nullptr) {
        unsigned int ind;
        EvalCacheEntry ece;
        std::unordered_map<string,unsigned int>::iterator it;
        string sfen=brd.fen(true);
        #ifndef USE_EVAL_CACHE
        return false;
        #endif
        if (!evalCacheIsInit) resetEvalCache();
        it=evalCacheHash.find(sfen);
        if (it!=evalCacheHash.end()) {
            ind=evalCacheHash[sfen];
            if (ind >= evalCache.size()) {
                wcout << L"bad evalCache index out of range!, fix!" << endl;
                exit(-1);
            }
            ece=evalCache[ind];
            if (ece.sfen != sfen) {
                wcout << L"bad cache read: linked entry with wrong sfen, fix!" << endl;
                exit(-1);                    
            }
            if (ece.depth==depth /*|| (ece.depth > depth && !((ece.depth-depth)%2))*/ ) {
                *pScore=ece.score;
                if (pCacheDepth!=nullptr) *pCacheDepth=ece.depth;
                ++evalCacheHit;
                return true;
            }
        }
        ++evalCacheMiss;
        return false;
    }

    vector<Move> rawMoveList() {
        vector<Move> ml;
        int x, y, xt, yt;
        int dy, promoteRank, startPawn;
        static const PieceType pp[] = {Knight, Bishop, Rook, Queen};
        static const signed char pawnCapW[] = {9, 11};
        static const signed char pawnCapB[] = {-9, -11};
        static const unsigned char c_a1 = toPos("a1");
        static const unsigned char c_b1 = toPos("b1");
        static const unsigned char c_c1 = toPos("c1");
        static const unsigned char c_d1 = toPos("d1");
        static const unsigned char c_e1 = toPos("e1");
        static const unsigned char c_f1 = toPos("f1");
        static const unsigned char c_g1 = toPos("g1");
        static const unsigned char c_h1 = toPos("h1");
        static const unsigned char c_a8 = toPos("a8");
        static const unsigned char c_b8 = toPos("b8");
        static const unsigned char c_c8 = toPos("c8");
        static const unsigned char c_d8 = toPos("d8");
        static const unsigned char c_e8 = toPos("e8");
        static const unsigned char c_f8 = toPos("f8");
        static const unsigned char c_g8 = toPos("g8");
        static const unsigned char c_h8 = toPos("h8");
        static const signed char kingQueenMoves[] = {-1, -11, -10, -9, 1, 11, 10, 9};
        static const signed char knightMoves[] = {8, 12, 21, 19, -8, -12, -21, -19};
        static const signed char bishopMoves[] = {-9, -11, 9, 11};
        static const signed char rookMoves[] = {-1, -10, 1, 10};
        const signed char *pawnCap;
        unsigned char f, to;
        Color attColor;
        if (activeColor == Color::White)
            attColor = Black;
        else
            attColor = White;
        for (unsigned char c = 21; c < 99; c++) {
            f = field[c];
            if (!f)
                continue;
            if (f == 0xff)
                continue;
            if (f & attColor)
                continue;
            switch (field[c] & 0b00011100) {
            case PieceType::Pawn:
                pos2coord(c, &y, &x);
                if (f & Color::Black) {
                    dy = -10;
                    pawnCap = pawnCapB;
                    promoteRank = 0;
                    startPawn = 6;
                } else {
                    dy = 10;
                    pawnCap = pawnCapW;
                    promoteRank = 7;
                    startPawn = 1;
                }
                to = c + dy;
                if (field[to] == Empty) {
                    pos2coord(to, &yt, &xt);
                    if (yt == promoteRank) {
                        for (PieceType p : pp) {
                            ml.push_back(Move(c, to, p));
                        }
                    } else {
                        ml.push_back(Move(c, to, Empty));
                        if (y == startPawn) {
                            to += dy;
                            if (field[to] == Empty) {
                                ml.push_back(Move(c, to, Empty));
                            }
                        }
                    }
                }
                for (int pci = 0; pci < 2; pci++) {
                    to = c + pawnCap[pci];
                    if (field[to] == 0xff)
                        continue;
                    pos2coord(to, &yt, &xt);
                    if ((field[to] & attColor) || to == epPos) {
                        if (yt == promoteRank) {
                            for (PieceType p : pp) {
                                ml.push_back(Move(c, to, p));
                            }
                        } else {
                            ml.push_back(Move(c, to, Empty));
                        }
                    }
                }
                break;
            case PieceType::King:
                if (f & Color::Black) {
                    if (castleRights & CastleRights::CBK) {
                        if ((field[c_e8] == (King | Black)) && (field[c_f8] == 0) &&
                            (field[c_g8] == 0) && (field[c_h8] == (Rook | Black))) {
                            if (!attacked(c_e8, activeColor) && !attacked(c_f8, activeColor) &&
                                !attacked(c_g8, activeColor)) {
                                ml.push_back(Move(c_e8, c_g8, Empty));
                            }
                        }
                    }
                    if (castleRights & CastleRights::CBQ) {
                        if ((field[c_e8] == (King | Black)) && (field[c_d8] == 0) &&
                            (field[c_c8] == 0) && (field[c_b8] == 0) &&
                            (field[c_a8] == (Rook | Black))) {
                            if (!attacked(c_c8, activeColor) && !attacked(c_d8, activeColor) &&
                                !attacked(c_e8, activeColor)) {
                                ml.push_back(Move(c_e8, c_c8, Empty));
                            }
                        }
                    }
                } else {
                    if (castleRights & CastleRights::CWK) {
                        if ((field[c_e1] == (King | White)) && (field[c_f1] == 0) &&
                            (field[c_g1] == 0) && (field[c_h1] == (Rook | White))) {
                            if (!attacked(c_e1, activeColor) && !attacked(c_f1, activeColor) &&
                                !attacked(c_g1, activeColor)) {
                                ml.push_back(Move(c_e1, c_g1, Empty));
                            }
                        }
                    }
                    if (castleRights & CastleRights::CWQ) {
                        if ((field[c_e1] == (King | White)) && (field[c_d1] == 0) &&
                            (field[c_c1] == 0) && (field[c_b1] == 0) &&
                            (field[c_a1] == (Rook | White))) {
                            if (!attacked(c_c1, activeColor) && !attacked(c_d1, activeColor) &&
                                !attacked(c_e1, activeColor)) {
                                ml.push_back(Move(c_e1, c_c1, Empty));
                            }
                        }
                    }
                }
                for (int di = 0; di < 8; ++di) {
                    to = c + kingQueenMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    if ((field[to] == 0) || (field[to] & attColor)) {
                        ml.push_back(Move(c, to, Empty));
                    }
                }
                break;
            case PieceType::Knight:
                for (int di = 0; di < 8; ++di) {
                    to = c + knightMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    if ((field[to] == 0) || (field[to] & attColor)) {
                        ml.push_back(Move(c, to, Empty));
                    }
                }
                break;
            case PieceType::Bishop:
                for (int di = 0; di < 4; ++di) {
                    to = c + bishopMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += bishopMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            case PieceType::Rook:
                for (int di = 0; di < 4; ++di) {
                    to = c + rookMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += rookMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            case PieceType::Queen:
                for (int di = 0; di < 8; ++di) {
                    to = c + kingQueenMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += kingQueenMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            default:
                wcout << L"Unidentified flying object in rawML at: " << to_wstring(c) << L"->"
                      << to_wstring(field[c]) << endl;
                break;
            }
        }
        return ml;
    }

    vector<Move> rawCaptureList() {
        vector<Move> ml;
        int x, y, xt, yt;
        int dy, promoteRank, startPawn;
        const PieceType pp[] = {Knight, Bishop, Rook, Queen};
        const signed char pawnCapW[] = {9, 11};
        const signed char pawnCapB[] = {-9, -11};
        const signed char *pawnCap;
        const unsigned char c_a1 = toPos("a1");
        const unsigned char c_b1 = toPos("b1");
        const unsigned char c_c1 = toPos("c1");
        const unsigned char c_d1 = toPos("d1");
        const unsigned char c_e1 = toPos("e1");
        const unsigned char c_f1 = toPos("f1");
        const unsigned char c_g1 = toPos("g1");
        const unsigned char c_h1 = toPos("h1");
        const unsigned char c_a8 = toPos("a8");
        const unsigned char c_b8 = toPos("b8");
        const unsigned char c_c8 = toPos("c8");
        const unsigned char c_d8 = toPos("d8");
        const unsigned char c_e8 = toPos("e8");
        const unsigned char c_f8 = toPos("f8");
        const unsigned char c_g8 = toPos("g8");
        const unsigned char c_h8 = toPos("h8");
        const signed char kingQueenMoves[] = {-1, -11, -10, -9, 1, 11, 10, 9};
        const signed char knightMoves[] = {8, 12, 21, 19, -8, -12, -21, -19};
        const signed char bishopMoves[] = {-9, -11, 9, 11};
        const signed char rookMoves[] = {-1, -10, 1, 10};
        unsigned char f, to;
        Color attColor;
        if (activeColor == Color::White)
            attColor = Black;
        else
            attColor = White;
        for (unsigned char c = 21; c < 99; c++) {
            f = field[c];
            if (!f)
                continue;
            if (f == 0xff)
                continue;
            if (f & attColor)
                continue;
            switch (field[c] & 0b00011100) {
            case PieceType::Pawn:
                pos2coord(c, &y, &x);
                if (f & Color::Black) {
                    dy = -10;
                    pawnCap = pawnCapB;
                    promoteRank = 0;
                    startPawn = 6;
                } else {
                    dy = 10;
                    pawnCap = pawnCapW;
                    promoteRank = 7;
                    startPawn = 1;
                }
                for (int pci = 0; pci < 2; pci++) {
                    to = c + pawnCap[pci];
                    if (field[to] == 0xff)
                        continue;
                    pos2coord(to, &yt, &xt);
                    if ((field[to] & attColor) || to == epPos) {
                        if (yt == promoteRank) {
                            for (PieceType p : pp) {
                                ml.push_back(Move(c, to, p));
                            }
                        } else {
                            ml.push_back(Move(c, to, Empty));
                        }
                    }
                }
                break;
            case PieceType::King:
                for (int di = 0; di < 8; ++di) {
                    to = c + kingQueenMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    if (field[to] & attColor) {
                        ml.push_back(Move(c, to, Empty));
                    }
                }
                break;
            case PieceType::Knight:
                for (int di = 0; di < 8; ++di) {
                    to = c + knightMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    if (field[to] & attColor) {
                        ml.push_back(Move(c, to, Empty));
                    }
                }
                break;
            case PieceType::Bishop:
                for (int di = 0; di < 4; ++di) {
                    to = c + bishopMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            if ((field[to] & attColor))
                                ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += bishopMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            case PieceType::Rook:
                for (int di = 0; di < 4; ++di) {
                    to = c + rookMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            if ((field[to] & attColor))
                                ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += rookMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            case PieceType::Queen:
                for (int di = 0; di < 8; ++di) {
                    to = c + kingQueenMoves[di];
                    if (field[to] == 0xff)
                        continue;
                    while (true) {
                        if ((field[to] == 0) || (field[to] & attColor)) {
                            if ((field[to] & attColor))
                                ml.push_back(Move(c, to, Empty));
                        } else
                            break;
                        if (field[to] & attColor)
                            break;
                        to += kingQueenMoves[di];
                        if (field[to] == 0xff)
                            break;
                    }
                }
                break;
            default:
                wcout << L"Unidentified flying object in rawML at: " << to_wstring(c) << L"->"
                      << to_wstring(field[c]) << endl;
                break;
            }
        }
        return ml;
    }

    Board rawApply(Move mv, bool sanityChecks = true) {
        Board brd = *this;
        const unsigned char c_a1 = toPos("a1");
        const unsigned char c_b1 = toPos("b1");
        const unsigned char c_c1 = toPos("c1");
        const unsigned char c_d1 = toPos("d1");
        const unsigned char c_e1 = toPos("e1");
        const unsigned char c_f1 = toPos("f1");
        const unsigned char c_g1 = toPos("g1");
        const unsigned char c_h1 = toPos("h1");
        const unsigned char c_a8 = toPos("a8");
        const unsigned char c_b8 = toPos("b8");
        const unsigned char c_c8 = toPos("c8");
        const unsigned char c_d8 = toPos("d8");
        const unsigned char c_e8 = toPos("e8");
        const unsigned char c_f8 = toPos("f8");
        const unsigned char c_g8 = toPos("g8");
        const unsigned char c_h8 = toPos("h8");
        if (sanityChecks) {
            if ((field[mv.from] & activeColor) != activeColor) {
                wcout << L"can't apply move: from-field is not occupied by piece of active color"
                      << endl;
                string uci = mv.toUci();
                wcout << L"move: " << stringenc(uci) << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.from] == Empty) {
                wcout << L"can't apply move: from-field is empty" << endl;
                string uci = mv.toUci();
                wcout << L"move: " << stringenc(uci) << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.to] & activeColor) {
                wcout << L"can't apply move: to-field is occupied by piece of active color" << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.from] == 0xff) {
                wcout << L"can't apply move: from-field is off-board" << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.to] == 0xff) {
                wcout << L"can't apply move: to-field is off-board" << endl;
                emptyBoard();
                return *this;
            }
            unsigned char pf = brd.field[mv.from];
            unsigned char pt = brd.field[mv.to];
            if (pt != Empty)
                brd.fiftyMoves = 0;
            else
                brd.fiftyMoves += 1;
            if (brd.activeColor == Black)
                brd.moveNumber += 1;
            switch (pf & 0b00011100) {
            case King:
                brd.epPos = 0;
                brd.field[mv.from] = 0;
                brd.field[mv.to] = pf;
                if (pf & White) {
                    brd.castleRights &= (CastleRights::CBK | CastleRights::CBQ);
                    if (mv.from == c_e1 && mv.to == c_g1) {
                        brd.field[c_f1] = brd.field[c_h1];
                        brd.field[c_h1] = 0;
                        brd.hasCastled |= White;
                    }
                    if (mv.from == c_e1 && mv.to == c_c1) {
                        brd.field[c_d1] = brd.field[c_a1];
                        brd.field[c_a1] = 0;
                        brd.hasCastled |= White;
                    }
                } else {
                    brd.castleRights &= (CastleRights::CWK | CastleRights::CWQ);
                    if (mv.from == c_e8 && mv.to == c_g8) {
                        brd.field[c_f8] = brd.field[c_h8];
                        brd.field[c_h8] = 0;
                        brd.hasCastled |= Black;
                    }
                    if (mv.from == c_e8 && mv.to == c_c8) {
                        brd.field[c_d8] = brd.field[c_a8];
                        brd.field[c_a8] = 0;
                        brd.hasCastled |= White;
                    }
                }
                break;
            case Pawn:
                int x, y, xt, yt;
                brd.field[mv.from] = 0;
                brd.field[mv.to] = pf;
                pos2coord(mv.from, &y, &x);
                pos2coord(mv.to, &yt, &xt);
                if (x != xt) {
                    if (mv.to == brd.epPos) {
                        brd.field[toPos(y, xt)] = 0;
                    }
                    brd.epPos = 0;
                } else {
                    brd.epPos = 0;
                    if (y == 1 && yt == 3) {
                        brd.epPos = toPos(2, x);
                    }
                    if (y == 6 && yt == 4) {
                        brd.epPos = toPos(5, x);
                    }
                }
                if (yt == 7 || yt == 0) {
                    brd.field[mv.to] = (mv.promote | activeColor);
                }
                brd.fiftyMoves = 0;
                break;
            case Rook:
                brd.epPos = 0;
                brd.field[mv.from] = 0;
                brd.field[mv.to] = pf;
                if (pf & White) {
                    if (mv.from == c_h1)
                        brd.castleRights &=
                            (CastleRights::CWQ | CastleRights::CBK | CastleRights::CBQ);
                    if (mv.from == c_a1)
                        brd.castleRights &=
                            (CastleRights::CWK | CastleRights::CBK | CastleRights::CBQ);
                } else {
                    if (mv.from == c_h8)
                        brd.castleRights &=
                            (CastleRights::CBQ | CastleRights::CWK | CastleRights::CWQ);
                    if (mv.from == c_a8)
                        brd.castleRights &=
                            (CastleRights::CBK | CastleRights::CWK | CastleRights::CWQ);
                }
                break;
            case Knight:
            case Bishop:
            case Queen:
                brd.epPos = 0;
                brd.field[mv.from] = 0;
                brd.field[mv.to] = pf;
                break;
            default:
                string uci = mv.toUci();
                wcout << L"Unidentified flying object in rawApply at: " << to_wstring(pf) << L"->"
                      << stringenc(uci) << endl;
                emptyBoard();
                return *this;
                break;
            }
            if (brd.activeColor == White)
                brd.activeColor = Black;
            else
                brd.activeColor = White;
        }
        return brd;
    }

    unsigned long int calcPerft(Board brd, int depth, int curDepth = 0,
                                vector<Move> moveHistory = {}) {
        vector<Move> ml = brd.moveList();
        unsigned long int cnt = 0;
        if (depth == curDepth + 1) {
            return ml.size();
        } else {
            for (Move mv : ml) {
                Board new_brd = brd.rawApply(mv, true);  // sanity checks on
                cnt += calcPerft(new_brd, depth, curDepth + 1, moveHistory);
            }
            return cnt;
        }
    }

    static bool move_max_sorter(Move const &m1, Move const &m2) {
        return m1.eval > m2.eval;
    }
    static bool move_min_sorter(Move const &m1, Move const &m2) {
        return m1.eval < m2.eval;
    }

    static int eval(Board brd, Color pov, bool fast=false) {
        // Board brd=*this;
        int evl = 0;
        int pieceEvl = 0;
        int attEvl = 0;
        int defEvl = 0;
        int kingEvl = 0;
        int moveEvl = 0;
        unsigned char fig, figTyp, pos;
        Color figCol;
        int pieceVals[] = {0, 100, 290, 300, 500, 900, 100000};
        int moveVals[] = {0, 4, 12, 12, 5, 1, 0};
        int space[8][8] = {
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 2, 2, 1, 1, 1},
            {1, 1, 3, 4, 4, 3, 1, 1},
            {1, 2, 4, 5, 5, 4, 2, 1},
            {1, 2, 4, 5, 5, 4, 2, 1},
            {1, 1, 3, 4, 4, 3, 1, 1},
            {1, 1, 1, 2, 2, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},            
        };
        int x,y;
        /*
        if (fast) {
            Board brd2(brd);
            //brd2.activeColor=pov;
            if (readEvalCache(brd2,2,&evl)) {
                return evl;
            }
        }
        */
        for (int fInd = 21; fInd < 99; fInd++) {
            fig = brd.field[fInd];
            if (fig == 0xff)
                continue;
            figTyp = (brd.field[fInd] & 0b00011100) >> 2;
            figCol = (Color)(brd.field[fInd] & 3);

            if (figCol == pov) {
                pieceEvl += pieceVals[figTyp];
            } else {
                if (figCol == invertColor(pov)) {
                    pieceEvl -= pieceVals[figTyp];
                }
            }
        }
        if (!fast) {
            Board brda(brd), brdn(brd);
            brda.activeColor = pov;
            brdn.activeColor = invertColor(pov);
            moveEvl = 0;
            for (Move mv : brda.moveList()) {
                fig = (brda.field[mv.from] & 0b00011100) >> 2;
                pos2coord(mv.to,&x,&y);
                moveEvl += moveVals[fig]*space[y][x];
            }
            for (Move mv : brdn.moveList()) {
                fig = (brda.field[mv.from] & 0b00011100) >> 2;
                pos2coord(mv.to,&x,&y);
                moveEvl -= moveVals[fig]*space[y][x];
            }
        }

        int sg;
        if (pov == White)
            sg = 1;
        else
            sg = -1;

        if (brd.castleRights & CastleRights::CWK)
            kingEvl += 30 * sg;
        if (brd.castleRights & CastleRights::CWQ)
            kingEvl += 30 * sg;
        if (brd.castleRights & CastleRights::CBK)
            kingEvl += -30 * sg;
        if (brd.castleRights & CastleRights::CBQ)
            kingEvl += -30 * sg;
        if (brd.hasCastled & White)
            kingEvl += 80 * sg;
        if (brd.hasCastled & Black)
            kingEvl -= 80 * sg;
        evl = pieceEvl * 4 + kingEvl * 2 + moveEvl/2;  // *8+attEvl/2+defEvl;
        // evl=moveEvl*4+pieceEvl*3; // XXX!
        return evl;
    }

    vector<Move> moveList(bool eval = false, bool fast=true) {
        vector<Move> ml = rawMoveList();
        vector<Move> vml;
        for (auto m : ml) {
            Board nbrd = rawApply(m);
            if (!nbrd.inCheck(activeColor)) {
                if (eval) {
                    m.eval = Board::eval(nbrd, activeColor, fast);
                }
                vml.push_back(m);
            }
        }
        if (eval) {
            std::sort(vml.begin(), vml.end(), &Board::move_max_sorter);
        }
        return vml;
    }

    static void printMoveList(vector<Move> &mv, const wchar_t *title) {
        wcout << title << ": ";
        for (Move m : mv) {
            wcout << stringenc(m.toUciWithEval()) << " ";
        }
        wcout << endl;
    }

    // https://de.wikipedia.org/wiki/Alpha-Beta-Suche
    // https://en.wikipedia.org/wiki/Negamax
    // negamax / alpha-beta
    static int negamax(Board brd, int depth, vector<Move> &history, vector<Move> &principal,
                       int col, int *pNodes, int *pCurDynDepth,
                       int alpha = MIN_EVAL, int beta = MAX_EVAL, int curDepth = 1,
                       int maxD = 0, bool siled=false) {
        bool gameOver = false;
        int endEval, eval, nextCol;
        int maxCaptures = -48;
        int origAlpha, origBeta;
        Board new_brd;
        vector<Move> ml(brd.moveList(true, true));  // eval, speed
        vector<Move> history_state,hist_scr;
        int sil=0;
        bool isCheck=false;
        int newDepth;

        //wcout << L"D=" << curDepth << L": ";
        //printMoveList(ml, L"ML");
        ++(*pNodes);

        if (readEvalCache(brd,curDepth,&eval)) {
            return eval;
        }

        if (depth > maxD)
            maxD = depth;
        if (curDepth > *pCurDynDepth)
            *pCurDynDepth=curDepth;
        if (ml.size() == 0) {
            gameOver = true;
        }
        isCheck=brd.inCheck(brd.activeColor);
        if (gameOver) {
            if (!isCheck) {
                eval=0;
            } else {
                eval=MIN_EVAL;
            }
            pushEvalCache(brd,curDepth,eval);            
            return eval;
        }
        if (!isCheck && depth <= 0 && ((brd.field[ml[0].to]==0 || siled) || depth <= maxCaptures)) {
            eval=Board::eval(brd,brd.activeColor,false);
            if (depth>=0) {
                pushEvalCache(brd,curDepth,eval);
            }
            return eval; 
        }

        origAlpha=alpha;
        origBeta=beta;
        endEval = MIN_EVAL;  // alpha;
        nextCol = invertColor(brd.activeColor);
        if (!siled) sil = 1;
        for (Move mv : ml) {
            if (depth <= 0 && brd.field[mv.to] == 0) {
                if (sil > 0) {
                    --sil;
                    siled=true;
                } else {
                    if (!isCheck)
                        break;
                }
            }
            new_brd = brd.rawApply(mv);
            history_state = history;
            history.push_back(mv);
            if (isCheck) {
                newDepth=depth;
            } else {
                newDepth=depth-1;
            }
            eval = -negamax(new_brd, newDepth, history, principal, nextCol, pNodes, pCurDynDepth, -beta, -alpha,
                            curDepth + 1, maxD, siled);
            if (eval > endEval) {
                endEval = eval;
                hist_scr=history;
            }
            history = history_state;
            if (endEval > alpha) 
                alpha = endEval;
            if (alpha >= beta)
                break;
        }
        if (endEval > origAlpha &&  endEval < origBeta) {
            principal = hist_scr;
        }
        //pushEvalCache(brd,curDepth,endEval);
        return endEval;
    }

    static vector<Move> searchBestMove(Board brd, int depth, int *pNodes, bool fastSearch=false) {
        vector<Move> ml(brd.moveList(true, fastSearch)), vml, best_principal, principal, history;
        Board newBoard;
        int bestEval, eval;

        int curMaxDynamicDepth;
        //printMoveList(ml, L"Initial sort");
        for (int d = 1; d < depth; d++) {
            curMaxDynamicDepth = 0;
            bestEval = MIN_EVAL;
            for (Move mv : ml) {
                newBoard = brd.rawApply(mv);
                principal.erase(principal.begin(), principal.end());
                history.erase(history.begin(), history.end());
                history.push_back(mv);
                eval = -negamax(newBoard, d, history, principal, newBoard.activeColor,
                                pNodes, &curMaxDynamicDepth);
                mv.eval = eval;
                vml.push_back(mv);
                if (eval > bestEval) {
                    bestEval = eval;
                    best_principal = principal;
                    if (bestEval == MAX_EVAL)
                        break;
                    //printMoveList(best_principal, L" Current line");
                }
            }
            std::sort(vml.begin(), vml.end(), &Board::move_max_sorter);
            ml = vml;
            //wcout << "Move list, depth=" << d;
            //printMoveList(ml, L"");
            wcout << L"Cache hit: " << evalCacheHit << L", miss: " << evalCacheMiss << L", " << evalCacheHit*100L/(evalCacheHit+evalCacheMiss) << L"%" << L" ECH-size: " << evalCacheHash.size() << L" EC-size: " << evalCache.size() << endl;
            wcout << "Best line, depth=" << d << L"/" << curMaxDynamicDepth << L", nodes=" << *pNodes;
            printMoveList(best_principal, L"");
            vml.erase(vml.begin(), vml.end());
            if (bestEval==MIN_EVAL || bestEval== MAX_EVAL) {
                // wcout << L"Search exhausted" << endl;
                break;
            }
        }
        return ml;
    }
};
