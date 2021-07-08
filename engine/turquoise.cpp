#include <cstring>
#include <climits>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <sstream>
#include <vector>
#include <algorithm>

using std::endl;
using std::string;
using std::wcout;
using std::wstring;
using std::to_wstring;
using std::vector;
using std::stringstream;
using std::getline;

enum PieceType {Empty=0, Pawn=0b00000100, Knight=0b00001000, Bishop=0b00001100, 
                Rook=0b00010000, Queen=0b00010100, King=0b00011000};
enum Color {None=0, White=0b00000001, Black=0b00000010};
enum Piece {bp=(PieceType::Pawn | Color::Black),
            bn=(PieceType::Knight | Color::Black),
            bb=(PieceType::Bishop | Color::Black),
            br=(PieceType::Rook | Color::Black),
            bq=(PieceType::Queen | Color::Black),
            bk=(PieceType::King | Color::Black),
            wp=(PieceType::Pawn | Color::White),
            wn=(PieceType::Knight | Color::White),
            wb=(PieceType::Bishop | Color::White),
            wr=(PieceType::Rook | Color::White),
            wq=(PieceType::Queen | Color::White),
            wk=(PieceType::King | Color::White),
            };
enum CastleRights {NoCastling=0, CWK=1, CWQ=2, CBK=4, CBQ=8};

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
        //sprintf(s, "%c[%d;%dm", esc, c, b);
        sprintf(s, "\033[%d;%dm", c, b);
        wcout << stringenc(s);
    }

    // Clear screen
    static void clr() {
        unsigned char esc = 27;
        char s[256];
        //sprintf(s, "%c[%dJ", esc, 2);
        sprintf(s, "\033[%dJ", 2);
        wcout << stringenc(s);
    }

    // Cursor goto(line,column)
    static void got(int cy, int cx) {
        unsigned char esc = 27;
        char s[256];
        //sprintf(s, "%c[%d;%dH", esc, cy + 1, cx + 1);
        sprintf(s, "\033[%d;%dH", cy + 1, cx + 1);
        wcout << stringenc(s);
    }
};

#define MAX_EVAL INT_MAX
#define MIN_EVAL (INT_MIN+3)
#define INVALID_EVAL INT_MIN

struct Board {
    unsigned char field[120];
    unsigned char castleRights;
    unsigned char fiftyMoves;
    unsigned char epPos;
    Color activeColor;
    unsigned int moveNumber;
    unsigned char hasCastled;

    Board() {
        startPosition();
    }

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    Board(string fen, bool verbose=false) {
        emptyBoard();
        activeColor=Color::White;
        epPos=0;
        fiftyMoves=0;
        moveNumber=0;
        vector<string> parts=split(fen,' ');
        if (parts.size() < 3) {
            if (verbose) wcout << L"Missing parts of fen, invalid!" << endl;
            return;
        }
        vector<string> rows=split(parts[0],'/');
        if (rows.size() != 8) {
            if (verbose) wcout << L"Invalid number of rows in fen, invalid!" << endl;
            return;
        }
        for (int y=7; y>=0; y--) {
            int x=0;

            for (int i=0; i<rows[7-y].length(); i++) {
                if (x>7) {
                    if (verbose) wcout << L"Invalid position encoding! Invalid!" << endl;
                    return;
                }
                char c=rows[7-y][i];
        //wcout << to_wstring(i) << L" | " << to_wstring(x) << endl;
                if (c>='1' && c<='8') {
                    for (int j=0; j<c-'0'; j++) ++x;
                } else {
                    field[toPos(y,x)]=asc2piece(c);
                    x+=1;
                }
            }
        }
        if (parts[1]=="w") {
            activeColor=Color::White;
        } else {
            if (parts[1]=="b") {
                activeColor=Color::Black;
            } else {
                if (verbose) wcout << L"Illegal color, invalid" << endl;
                activeColor=Color::White;
            }
        }
        if (parts[2].find('K')!=std::string::npos) castleRights|=CastleRights::CWK;
        if (parts[2].find('Q')!=std::string::npos) castleRights|=CastleRights::CWQ;
        if (parts[2].find('k')!=std::string::npos) castleRights|=CastleRights::CBK;
        if (parts[2].find('q')!=std::string::npos) castleRights|=CastleRights::CBQ;
        if (parts.size()>3) {
            if (parts[3]!="-") {
                epPos=toPos(parts[3]);
            }
        }
        if (parts.size()>4) {
            if (parts[4]!="-") {
                fiftyMoves=stoi(parts[4]);
            } else fiftyMoves=0;
        }
        if (parts.size()>5) {
            if (parts[5]!="-") {
                moveNumber=stoi(parts[5]);
            } else moveNumber=1;
        }
    }

    string fen() {
        string f="";
        for (int y=7; y>=0; y--) {
            int bl=0;
            for (int x=0; x<8; x++) {
                unsigned char pc=field[toPos(y,x)];
                if (pc==0) ++bl;
                else {
                    if (bl>0) {
                        f+=std::to_string(bl);
                        bl=0;
                    }
                    f+=string(1,piece2asc(pc));
                }
            }
            if (bl>0) {
                f+=std::to_string(bl);
                bl=0;
            }
            if (y>0) f+="/";
        }
        f+=" ";
        if (activeColor==Color::Black) f+="b ";
        else f+="w ";
        string cr="";
        if (castleRights & CastleRights::CWK) cr+="K";
        if (castleRights & CastleRights::CWQ) cr+="Q";
        if (castleRights & CastleRights::CBK) cr+="k";
        if (castleRights & CastleRights::CBQ) cr+="q";
        if (cr=="") cr="-";
        f+=cr+" ";
        string ep="";
        if (epPos!=0) {
            ep=pos2string(epPos);
        } else ep="-";
        f+=ep+" ";
        f+=std::to_string((int)fiftyMoves)+" ";
        f+=std::to_string((int)moveNumber);
        return f;
    }

    static unsigned char asc2piece(char c) {
        const string wp="PNBRQK";
        const string bp="pnbrqk";
        unsigned char pc=0;
        int ind=wp.find(c);
        if (ind!=std::string::npos) {
            pc=((ind+1)<<2) | Color::White;
        } else {
            ind=bp.find(c);
            if (ind!=std::string::npos) {
                pc=((ind+1)<<2) | Color::Black;
            }
        }
        return pc;
    }

    static PieceType asc2piecetype(char c) {
        const string bp="pnbrqk";
        PieceType pt=PieceType::Empty;
        int ind=bp.find(std::tolower(c));
        if (ind!=std::string::npos) {
            pt=(PieceType)(ind+1);
        }
        return pt;
    }

    static char piece2asc(unsigned char pc) {
        const char *wp="PNBRQK";
        const char *bp="pnbrqk";
        if (pc & Color::Black) {
            return bp[(pc>>2)-1];
        } else {
            return wp[(pc>>2)-1];
        }
    }

    vector<string> split (const string &s, char delim) {
        vector<string> result;
        stringstream ss (s);
        string item;
        while (getline(ss, item, delim)) result.push_back (item);
        return result;
    }

    void startPosition() {
        unsigned char startFigs[]={PieceType::Rook, PieceType::Knight, PieceType::Bishop,
                                 PieceType::Queen, PieceType::King, PieceType::Bishop,
                                 PieceType::Knight, PieceType::Rook};
        emptyBoard();
        castleRights=CastleRights::CWK | CastleRights::CWQ | CastleRights::CBK | CastleRights::CBQ;
        activeColor=Color::White;
        epPos=0;
        fiftyMoves=0;
        moveNumber=1;
        for (int i=0; i<8; i++) {
            field[toPos(0,i)]=startFigs[i]+Color::White;
            field[toPos(7,i)]=startFigs[i]+Color::Black;
            field[toPos(1,i)]=Piece::wp;
            field[toPos(6,i)]=Piece::bp;
        }
    }

    void emptyBoard() {
        memset(field,0xff,120);
        for (int y=0; y<8; y++) {
            for (int x=0; x<8; x++) {
                field[toPos(y,x)]=0;
            }
        }
        castleRights=0;
        activeColor=None;
        epPos=0;
        fiftyMoves=0;
        moveNumber=0;
        hasCastled=0;
    }

    static inline unsigned char toPos(unsigned char y, unsigned char x) {
        return ((y + 2) * 10 + x + 1);
    }
    static inline unsigned char toPos(string posStr) {
        return ((posStr[1]-'1'+2)*10+std::tolower(posStr[0])-'a'+1);
    }

    static inline Color invertColor(unsigned char c) {
        if (c & White)
            return Black;
        if (c & Black)
            return White;
        return None;
    }

    static void pos2coord(unsigned char pos, int *y, int *x) {
        if (y) *y=pos/10-2;
        if (x) *x=(pos%10)-1;
    }

    static string pos2string(unsigned char pos) {
        char c1=pos/10-2+'1';
        char c2=(pos%10)-1+'a';
        string coord=string(1,c2)+string(1,c1);
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
                Term::got(py + 7-cy, px);
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
                    fo = f>>2;
                    if (f & Color::Black)
                        cf = -1;
                    else cf = 1;
                    if (cl*cf > 0)
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
        if (activeColor==Color::White) wcout << L"Active: White" << endl;
        else if (activeColor==Color::Black) wcout << L"Active: Black" << endl;
        else wcout << L"Active color is undefined!" << endl;

        wcout << L"Castle-rights: ";
        if (castleRights & CastleRights::CWK) wcout << L"White-King ";
        if (castleRights & CastleRights::CWQ) wcout << L"White-Queen ";
        if (castleRights & CastleRights::CBK) wcout << L"Black-King ";
        if (castleRights & CastleRights::CBQ) wcout << L"Black-Queen ";
        wcout << endl;

        if (epPos==0) wcout << L"No enpassant" << endl;
        else {
            string ep=pos2string(epPos);
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
        Move() {
            from=0;
            to=0;
            promote=Empty;
            eval=INVALID_EVAL;
        }
        Move(unsigned char from, unsigned char to, PieceType promote=Empty): from(from), to(to), promote(promote) {
        }
        // Copy constructor 
        Move(const Move &mv) {from=mv.from; to=mv.to; promote=mv.promote; eval=mv.eval; } 
        // Decode uci and algebraic-notation strings to move [TODO: incomplete]
        Move(string alg, Board *brd=nullptr) {
            promote=Empty;
            from=0;
            to=0;
            eval=INVALID_EVAL;
            if (alg.length()>=5 && alg[2]=='-') { // UCI Format
                from=Board::toPos(alg.substr(0,2));
                to=Board::toPos(alg.substr(3,2));
                if (alg.length()>=6 && alg[5]!='+' && alg[5]!='#') {
                    promote=Board::asc2piecetype(alg[5]);
                }
            } else { // Algebraic notation, needs board
                if (brd==nullptr) return;
                string al=alg;
                if (alg=="O-O") {
                    if (brd->activeColor==Color::White) {
                        from=Board::toPos("e1");
                        to=Board::toPos("g1");
                    } else if (brd->activeColor==Color::Black) {
                        from=Board::toPos("e8");
                        to=Board::toPos("g8");
                    }
                } else if (alg=="O-O-O") {
                    if (brd->activeColor==Color::White) {
                        from=Board::toPos("e1");
                        to=Board::toPos("c1");
                    } else if (brd->activeColor==Color::Black) {
                        from=Board::toPos("e8");
                        to=Board::toPos("c8");
                    }
                } else {
                    if (al.length()<2) return;
                    const string pt="NBRQK";
                    PieceType p;
                    if (pt.find(al[0])!=std::string::npos) {
                        p=Board::asc2piecetype(al[0]);
                        al=al.substr(1);                        
                    } else {
                        p=PieceType::Pawn;
                    }
                    if (al.length()<2) return;
                    if (al[0]=='x') {
                        al=al.substr(1);                        
                    }
                    if (al.length()<2) return;
                    int fx=-1, fy=-1, tx=-1, ty=-1;
                    if (al[1]>='a' && al[1]<='h') {
                        tx=al[1]-'a';
                        fx=al[0]-'a';
                        al=al.substr(2);
                    } else {
                        tx=al[0]-'a';
                        al=al.substr(1);
                    }
                    if (al.length()<1) return;
                    if (al.length()>=2) {
                        if (al[1]>='1' && al[1]<='8') {
                            ty=al[1]-'1';
                            fy=al[1]-'1';
                            al=al.substr(2);
                        } else {
                            ty=al[0]-'1';
                            al=al.substr(1);
                        }
                    }
                    if (al.length()>=2) {
                        if (al[0]=='=') {
                            promote=Board::asc2piecetype(al[1]);
                        }
                    }
                    // TODO: algebraic decoding missing.
                }
            }
        }
        string toUci() {
            string uci=Board::pos2string(from)+"-"+pos2string(to);
            if (promote!=Empty) {
                uci+=string(1,piece2asc(promote));
            }
            return uci;
        }
        string toUciWithEval() {
            string uci=Board::pos2string(from)+"-"+pos2string(to);
            if (promote!=Empty) {
                uci+=string(1,piece2asc(promote));
            }
            if (eval!=INVALID_EVAL) {
                uci+=" ("+std::to_string(eval)+")";
            }
            return uci;
        }
    };

    bool attacked(unsigned char pos, Color col) {
        // TODO: optimize by copying code and directly returning bool
        vector<unsigned char> pl=attackers(pos, col);
        if (pl.size()>0) return true;
        else return false;
    }

    vector<unsigned char> attackers(unsigned char pos, Color col) {
        vector<unsigned char> pl;
        Color attColor;
        if (col==Color::White) attColor=Black;
        else attColor=White;

        if (col==White) {
            if (field[pos+11]==(Pawn | Black)) {
                pl.push_back(pos+11);
            }
            if (field[pos+9]==(Pawn | Black)) {
                pl.push_back(pos+9);
            }
        } else {
            if (field[pos-11]==(Pawn | White)) {
                pl.push_back(pos-11);
            }
            if (field[pos-9]==(Pawn | White)) {
                pl.push_back(pos-9);
            }
        }
        signed char knightMoves[]{21, -21, 19, -19, 8, 12, -8, -12};
        signed char bishopMoves[]{11, -11, 9, -9};
        signed char rookMoves[]{10, -10, 1, -1};
        signed char kingQueenMoves[]{11, -11, 9, -9, 10, -10, 1, -1};
        for (signed char kn : knightMoves) {
            if (field[pos+kn]==(PieceType::Knight | attColor)) pl.push_back(pos+kn);
        }
        for (signed char km : kingQueenMoves) {
            if (field[pos+km]==(PieceType::King | attColor)) pl.push_back(pos+km);
        }
        for (signed char bm : bishopMoves) {
            unsigned char pn=pos+bm;
            if (field[pn]==0xff) continue;
            while (true) {
                if ((field[pn]==(PieceType::Bishop | attColor)) || (field[pn]==(PieceType::Queen | attColor))) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn]!=0) break;
                pn=pn+bm;
                if (field[pn]==0xff) break;
            }
        }
        for (signed char bm : rookMoves) {
            unsigned char pn=pos+bm;
            if (field[pn]==0xff) continue;
            while (true) {
                if ((field[pn]==(PieceType::Rook | attColor)) || (field[pn]==(PieceType::Queen | attColor))) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn]!=0) break;
                pn=pn+bm;
                if (field[pn]==0xff) break;
            }
        }
        // wcout << "Attackers: " << to_wstring(pl.size()) << endl;
        return pl;
    }

    vector<unsigned char> attackedBy(unsigned char pos, Color col) {
        vector<unsigned char> apl;
        vector<unsigned char> posl=attackers(pos, col);
        for (unsigned char pi : posl) apl.push_back(field[pi]>>2);
        std::sort(apl.begin(),apl.end());
        return apl;
    }

    unsigned char kingsPos(Color col) {
        for (unsigned char c=21; c<99; c++) {
            if (field[c]==(PieceType::King | col)) return c;
        }
        return 0;
    }

    bool inCheck(Color col) {
        unsigned char kpos=kingsPos(col);
        vector<unsigned char> pl=attackers(kpos, col);
        if (pl.size()==0) return false;
        else return true;
    }

    vector<Move> rawMoveList() {
        vector<Move> ml;
        int x,y,xt,yt;
        int dy,promoteRank,startPawn;
        const PieceType pp[]={Knight, Bishop, Rook, Queen};
        const signed char pawnCapW[]={9,11};
        const signed char pawnCapB[]={-9,-11};
        const signed char *pawnCap;
        const unsigned char c_a1=toPos("a1");
        const unsigned char c_b1=toPos("b1");
        const unsigned char c_c1=toPos("c1");
        const unsigned char c_d1=toPos("d1");
        const unsigned char c_e1=toPos("e1");
        const unsigned char c_f1=toPos("f1");
        const unsigned char c_g1=toPos("g1");
        const unsigned char c_h1=toPos("h1");
        const unsigned char c_a8=toPos("a8");
        const unsigned char c_b8=toPos("b8");
        const unsigned char c_c8=toPos("c8");
        const unsigned char c_d8=toPos("d8");
        const unsigned char c_e8=toPos("e8");
        const unsigned char c_f8=toPos("f8");
        const unsigned char c_g8=toPos("g8");
        const unsigned char c_h8=toPos("h8");
        const signed char kingQueenMoves[]={-1,-11,-10,-9,1,11,10,9};
        const signed char knightMoves[]={8,12,21,19,-8,-12,-21,-19};
        const signed char bishopMoves[]={-9,-11,9,11};
        const signed char rookMoves[]={-1,-10,1,10};
        unsigned char f,to;
        Color attColor;
        if (activeColor==Color::White) attColor=Black;
        else attColor=White;
        for (unsigned char c=21; c<99; c++) {
            f=field[c];
            if (!f) continue;
            if (f==0xff) continue;
            if (f & attColor) continue;
            // wcout << L"Field: " << to_wstring(c) << endl;
            switch (field[c] & 0b00011100) {
                case PieceType::Pawn:
                    pos2coord(c, &y, &x);
                    if (f & Color::Black) {
                        dy=-10;
                        pawnCap=pawnCapB;
                        promoteRank=0;
                        startPawn=6;
                    } else {
                        dy=10;
                        pawnCap=pawnCapW;
                        promoteRank=7;
                        startPawn=1;
                    }
                    to=c+dy;
                    if (field[to]==Empty) {
                        pos2coord(to,&yt,&xt);
                        if (yt==promoteRank) {
                            for (PieceType p: pp) {
                                ml.push_back(Move(c,to,p));
                            }
                        } else {
                            ml.push_back(Move(c,to,Empty));
                            if (y==startPawn) {
                                to+=dy;
                                if (field[to]==Empty) {
                                    ml.push_back(Move(c,to,Empty));
                                }
                            }
                        }
                    }
                    for (int pci=0; pci<2; pci++) {
                        to=c+pawnCap[pci];
                        if (field[to]==0xff) continue;
                        pos2coord(to,&yt,&xt);
                        if ((field[to] & attColor) || to==epPos) {
                            if (yt==promoteRank) {
                                for (PieceType p: pp) {
                                    ml.push_back(Move(c,to,p));
                                }
                            } else {
                                ml.push_back(Move(c,to,Empty));
                            }
                        }
                    }
                    break;
                case PieceType::King:
                    if (f & Color::Black) {
                        if (castleRights & CastleRights::CBK) {
                            if ((field[c_e8]==(King | Black)) && (field[c_f8]==0) && (field[c_g8]==0) && (field[c_h8]==(Rook | Black))) {
                                if (!attacked(c_e8, activeColor) && !attacked(c_f8, activeColor) && !attacked(c_g8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_g8, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::CBQ) {
                            if ((field[c_e8]==(King | Black)) && (field[c_d8]==0) && (field[c_c8]==0) && (field[c_b8]==0) && (field[c_a8]==(Rook | Black))) {
                                if (!attacked(c_c8, activeColor) && !attacked(c_d8, activeColor) && !attacked(c_e8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_c8, Empty));
                                }
                            }
                        }
                    } else {
                        if (castleRights & CastleRights::CWK) {
                            if ((field[c_e1]==(King | White)) && (field[c_f1]==0) && (field[c_g1]==0) && (field[c_h1]==(Rook | White))) {
                                if (!attacked(c_e1, activeColor) && !attacked(c_f1, activeColor) && !attacked(c_g1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_g1, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::CWQ) {
                            if ((field[c_e1]==(King | White)) && (field[c_d1]==0) && (field[c_c1]==0) && (field[c_b1]==0) && (field[c_a1]==(Rook | White))) {
                                if (!attacked(c_c1, activeColor) && !attacked(c_d1, activeColor) && !attacked(c_e1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_c1, Empty));
                                }
                            }
                        }
                    }
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        if (field[to]==0xff) continue;
                        if ((field[to]==0) || (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Knight:
                    for (int di=0; di<8; ++di) {
                        to=c+knightMoves[di];
                        if (field[to]==0xff) continue;
                        if ((field[to]==0) || (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Bishop:
                    for (int di=0; di<4; ++di) {
                        to=c+bishopMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=bishopMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                case PieceType::Rook:
                    for (int di=0; di<4; ++di) {
                        to=c+rookMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=rookMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                case PieceType::Queen:
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=kingQueenMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                default:
                    wcout << L"Unidentified flying object in rawML at: " << to_wstring(c) << L"->" << to_wstring(field[c]) << endl;
                    break;
            }
        }
        // wcout << L"Move list size: " << to_wstring(ml.size()) << endl;
        return ml;
    }

vector<Move> rawCaptureList() {
        vector<Move> ml;
        int x,y,xt,yt;
        int dy,promoteRank,startPawn;
        const PieceType pp[]={Knight, Bishop, Rook, Queen};
        const signed char pawnCapW[]={9,11};
        const signed char pawnCapB[]={-9,-11};
        const signed char *pawnCap;
        const unsigned char c_a1=toPos("a1");
        const unsigned char c_b1=toPos("b1");
        const unsigned char c_c1=toPos("c1");
        const unsigned char c_d1=toPos("d1");
        const unsigned char c_e1=toPos("e1");
        const unsigned char c_f1=toPos("f1");
        const unsigned char c_g1=toPos("g1");
        const unsigned char c_h1=toPos("h1");
        const unsigned char c_a8=toPos("a8");
        const unsigned char c_b8=toPos("b8");
        const unsigned char c_c8=toPos("c8");
        const unsigned char c_d8=toPos("d8");
        const unsigned char c_e8=toPos("e8");
        const unsigned char c_f8=toPos("f8");
        const unsigned char c_g8=toPos("g8");
        const unsigned char c_h8=toPos("h8");
        const signed char kingQueenMoves[]={-1,-11,-10,-9,1,11,10,9};
        const signed char knightMoves[]={8,12,21,19,-8,-12,-21,-19};
        const signed char bishopMoves[]={-9,-11,9,11};
        const signed char rookMoves[]={-1,-10,1,10};
        unsigned char f,to;
        Color attColor;
        if (activeColor==Color::White) attColor=Black;
        else attColor=White;
        for (unsigned char c=21; c<99; c++) {
            f=field[c];
            if (!f) continue;
            if (f==0xff) continue;
            if (f & attColor) continue;
            // wcout << L"Field: " << to_wstring(c) << endl;
            switch (field[c] & 0b00011100) {
                case PieceType::Pawn:
                    pos2coord(c, &y, &x);
                    if (f & Color::Black) {
                        dy=-10;
                        pawnCap=pawnCapB;
                        promoteRank=0;
                        startPawn=6;
                    } else {
                        dy=10;
                        pawnCap=pawnCapW;
                        promoteRank=7;
                        startPawn=1;
                    }
                    /*
                    to=c+dy;
                    if (field[to]==Empty) {
                        pos2coord(to,&yt,&xt);
                        if (yt==promoteRank) {
                            for (PieceType p: pp) {
                                ml.push_back(Move(c,to,p));
                            }
                        } else {
                            ml.push_back(Move(c,to,Empty));
                            if (y==startPawn) {
                                to+=dy;
                                if (field[to]==Empty) {
                                    ml.push_back(Move(c,to,Empty));
                                }
                            }
                        }
                    }
                    */
                    for (int pci=0; pci<2; pci++) {
                        to=c+pawnCap[pci];
                        if (field[to]==0xff) continue;
                        pos2coord(to,&yt,&xt);
                        if ((field[to] & attColor) || to==epPos) {
                            if (yt==promoteRank) {
                                for (PieceType p: pp) {
                                    ml.push_back(Move(c,to,p));
                                }
                            } else {
                                ml.push_back(Move(c,to,Empty));
                            }
                        }
                    }
                    break;
                case PieceType::King:
                    /*
                    if (f & Color::Black) {
                        if (castleRights & CastleRights::CBK) {
                            if ((field[c_e8]==(King | Black)) && (field[c_f8]==0) && (field[c_g8]==0) && (field[c_h8]==(Rook | Black))) {
                                if (!attacked(c_e8, activeColor) && !attacked(c_f8, activeColor) && !attacked(c_g8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_g8, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::CBQ) {
                            if ((field[c_e8]==(King | Black)) && (field[c_d8]==0) && (field[c_c8]==0) && (field[c_b8]==0) && (field[c_a8]==(Rook | Black))) {
                                if (!attacked(c_c8, activeColor) && !attacked(c_d8, activeColor) && !attacked(c_e8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_c8, Empty));
                                }
                            }
                        }
                    } else {
                        if (castleRights & CastleRights::CWK) {
                            if ((field[c_e1]==(King | White)) && (field[c_f1]==0) && (field[c_g1]==0) && (field[c_h1]==(Rook | White))) {
                                if (!attacked(c_e1, activeColor) && !attacked(c_f1, activeColor) && !attacked(c_g1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_g1, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::CWQ) {
                            if ((field[c_e1]==(King | White)) && (field[c_d1]==0) && (field[c_c1]==0) && (field[c_b1]==0) && (field[c_a1]==(Rook | White))) {
                                if (!attacked(c_c1, activeColor) && !attacked(c_d1, activeColor) && !attacked(c_e1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_c1, Empty));
                                }
                            }
                        }
                    }
                    */
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        if (field[to]==0xff) continue;
                        if (/*(field[to]==0) ||*/ (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Knight:
                    for (int di=0; di<8; ++di) {
                        to=c+knightMoves[di];
                        if (field[to]==0xff) continue;
                        if (/*(field[to]==0) ||*/ (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Bishop:
                    for (int di=0; di<4; ++di) {
                        to=c+bishopMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                if ((field[to]&attColor)) ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=bishopMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                case PieceType::Rook:
                    for (int di=0; di<4; ++di) {
                        to=c+rookMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                if ((field[to]&attColor)) ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=rookMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                case PieceType::Queen:
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        if (field[to]==0xff) continue;
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                if ((field[to]&attColor)) ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=kingQueenMoves[di];
                            if (field[to]==0xff) break;
                        }
                    }
                    break;
                default:
                    wcout << L"Unidentified flying object in rawML at: " << to_wstring(c) << L"->" << to_wstring(field[c]) << endl;
                    break;
            }
        }
        return ml;
    }

    Board rawApply(Move mv, bool sanityChecks=true) {
        Board brd=*this;
        const unsigned char c_a1=toPos("a1");
        const unsigned char c_b1=toPos("b1");
        const unsigned char c_c1=toPos("c1");
        const unsigned char c_d1=toPos("d1");
        const unsigned char c_e1=toPos("e1");
        const unsigned char c_f1=toPos("f1");
        const unsigned char c_g1=toPos("g1");
        const unsigned char c_h1=toPos("h1");
        const unsigned char c_a8=toPos("a8");
        const unsigned char c_b8=toPos("b8");
        const unsigned char c_c8=toPos("c8");
        const unsigned char c_d8=toPos("d8");
        const unsigned char c_e8=toPos("e8");
        const unsigned char c_f8=toPos("f8");
        const unsigned char c_g8=toPos("g8");
        const unsigned char c_h8=toPos("h8");
        if (sanityChecks) {
            if ((field[mv.from] & activeColor)!=activeColor) {
                wcout << L"can't apply move: from-field is not occupied by piece of active color" << endl;
                string uci=mv.toUci();
                wcout << L"move: " << stringenc(uci) << endl;                           
                emptyBoard();
                return *this;
            }
            if (field[mv.from]==Empty) {
                wcout << L"can't apply move: from-field is empty" << endl;
                string uci=mv.toUci();
                wcout << L"move: " << stringenc(uci) << endl;                           
                emptyBoard();
                return *this;
            }
            if (field[mv.to] & activeColor) {
                wcout << L"can't apply move: to-field is occupied by piece of active color" << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.from]==0xff) {
                wcout << L"can't apply move: from-field is off-board" << endl;
                emptyBoard();
                return *this;
            }
            if (field[mv.to]==0xff) {
                wcout << L"can't apply move: to-field is off-board" << endl;
                emptyBoard();
                return *this;
            }
            unsigned char pf=brd.field[mv.from];
            unsigned char pt=brd.field[mv.to];
            if (pt!=Empty) brd.fiftyMoves=0;
            else brd.fiftyMoves+=1;
            if (brd.activeColor==Black) brd.moveNumber+=1;
            switch (pf & 0b00011100) {
                case King:
                    brd.epPos=0;
                    brd.field[mv.from]=0;
                    brd.field[mv.to]=pf;
                    if (pf & White) {
                        brd.castleRights &= (CastleRights::CBK | CastleRights::CBQ);
                        if (mv.from==c_e1 && mv.to==c_g1) {
                            brd.field[c_f1]=brd.field[c_h1];
                            brd.field[c_h1]=0;
                            brd.hasCastled |= White;
                        }
                        if (mv.from==c_e1 && mv.to==c_c1) {
                            brd.field[c_d1]=brd.field[c_a1];
                            brd.field[c_a1]=0;
                            brd.hasCastled |= White;
                        }
                    } else {
                        brd.castleRights &= (CastleRights::CWK | CastleRights::CWQ);
                        if (mv.from==c_e8 && mv.to==c_g8) {
                            brd.field[c_f8]=brd.field[c_h8];
                            brd.field[c_h8]=0;
                            brd.hasCastled |= Black;
                        }
                        if (mv.from==c_e8 && mv.to==c_c8) {
                            brd.field[c_d8]=brd.field[c_a8];
                            brd.field[c_a8]=0;
                            brd.hasCastled |= White;
                        }
                    }
                    break;
                case Pawn:
                    int x,y,xt,yt;
                    brd.field[mv.from]=0;
                    brd.field[mv.to]=pf;
                    pos2coord(mv.from,&y,&x);
                    pos2coord(mv.to,&yt,&xt);
                    if (x != xt) {
                        if (mv.to==brd.epPos) {
                            brd.field[toPos(y,xt)]=0;
                        }
                        brd.epPos=0;
                    } else {
                        brd.epPos=0;
                        if (y==1 && yt==3) {
                            brd.epPos=toPos(2,x);
                        }
                        if (y==6 && yt==4) {
                            brd.epPos=toPos(5,x);
                        }
                    }
                    if (yt==7 || yt==0) {
                        brd.field[mv.to]=(mv.promote | activeColor);
                    }
                    brd.fiftyMoves=0;
                    break;
                case Rook:
                    brd.epPos=0;
                    brd.field[mv.from]=0;
                    brd.field[mv.to]=pf;
                    if (pf & White) {
                        if (mv.from==c_h1) brd.castleRights &= (CastleRights::CWQ | CastleRights::CBK | CastleRights::CBQ);
                        if (mv.from==c_a1) brd.castleRights &= (CastleRights::CWK | CastleRights::CBK | CastleRights::CBQ);
                    } else {
                        if (mv.from==c_h8) brd.castleRights &= (CastleRights::CBQ | CastleRights::CWK | CastleRights::CWQ);
                        if (mv.from==c_a8) brd.castleRights &= (CastleRights::CBK | CastleRights::CWK | CastleRights::CWQ);
                    }
                    break;
                case Knight:
                case Bishop:
                case Queen:
                    brd.epPos=0;
                    brd.field[mv.from]=0;
                    brd.field[mv.to]=pf;
                    break;
                default:
                    string uci=mv.toUci();
                    wcout << L"Unidentified flying object in rawApply at: " << to_wstring(pf) << L"->" << stringenc(uci) << endl;
                    emptyBoard();
                    return *this;
                    break;
            }
            if (brd.activeColor == White) brd.activeColor=Black;
            else brd.activeColor=White;
        }
        // brd.printPos(&brd,-1);
        return brd;
    }

    static bool move_white_sorter(Move const& m1, Move const& m2) {
        return m1.eval > m2.eval;
    }
    static bool move_black_sorter(Move const& m1, Move const& m2) {
        return m1.eval < m2.eval;
    }

    vector<Move> moveList(bool eval=false, bool relativeEval=false) {
        vector<Move> ml=rawMoveList();
        vector<Move> vml;
        for (auto m : ml) {
            Board nbrd=rawApply(m);
            if (!nbrd.inCheck(activeColor)) {
                if (eval) {
                    m.eval=nbrd.eval(relativeEval);
                }
                vml.push_back(m);
            }
        }
        if (eval) {
            if (activeColor==White || relativeEval) std::sort(vml.begin(), vml.end(), &Board::move_white_sorter);
            else std::sort(vml.begin(), vml.end(), &Board::move_black_sorter);
        }
        return vml;
    }

    vector<Move> captureList(bool eval=false, bool relativeEval=false) {
        vector<Move> ml=rawCaptureList();
        vector<Move> vml;
        for (auto m : ml) {
            Board nbrd=rawApply(m);
            if (!nbrd.inCheck(activeColor)) {
                if (eval) {
                    m.eval=nbrd.eval(relativeEval);
                }
                vml.push_back(m);
            }
        }
        if (eval) {
            if (activeColor==White || relativeEval) std::sort(vml.begin(), vml.end(), &Board::move_white_sorter);
            else std::sort(vml.begin(), vml.end(), &Board::move_black_sorter);
        }
        return vml;
    }

    unsigned long int calcPerft(Board brd, int depth, int curDepth=0, vector<Move> moveHistory={}) {
        vector<Move> ml=brd.moveList();
        unsigned long int cnt=0;
        if (depth==curDepth+1) {
            return ml.size();
        }
        else {
            for (Move mv : ml) {
                Board new_brd=brd.rawApply(mv,true); // sanity checks on
                cnt+=calcPerft(new_brd, depth, curDepth+1, moveHistory);
            }
            return cnt;
        }
    }

    /*
    int eval(bool relativeEval=false, bool verbose=false) {
        Board brd=*this;
        int val=0,attval,defval,mlval,pieceval;
        int v1,v2;
        int fTyp;
        int pCol;
        vector<unsigned char> atl,atlx;
        vector<Move> ml;
        unsigned char f;

        //wcout << endl;
        //printPos(&brd, -1);

        int pieceVals[]={0,100,290,300,500,900,100000};
        int attVals[]={12,10,10,10,15,20,30};
        int defVals[]={10,5,10,10,5,2,3};

        attval=0;
        defval=0;
        pieceval=0;
        for (int c=21; c<99; c++) {
            f=brd.field[c];
            if (f==0xff) continue;
            fTyp=brd.field[c]>>2;
            pCol=brd.field[c]&3;

            if (pCol & White) pieceval+=pieceVals[fTyp];
            else pieceval-=pieceVals[fTyp];

            switch (pCol) {
                case Black:
                case None:
                    v1 = attackers(c, Black).size() * attVals[fTyp];
                    v2 = attackers(c, White).size() * defVals[fTyp];
                    break;
                case White:
                    v1 = attackers(c, Black).size() * defVals[fTyp];
                    v2 = attackers(c, White).size() * attVals[fTyp];
                    break;
            } 
            attval += v1;
            defval += v2;
        }
        attval /= 20;
        defval /= 20;


        brd.activeColor=White;
        ml=brd.rawMoveList();
        mlval=ml.size();
        brd.activeColor=Black;
        ml=brd.rawMoveList();
        mlval -=ml.size();

        val = attval + defval + pieceval + mlval;
        if (verbose) wcout << L"Val: " << to_wstring(val) << ", pieceval: " << to_wstring(pieceval) << ", attval: " << to_wstring(attval) << ", defval: " << to_wstring(defval) << ", mlval: " << to_wstring(mlval) << endl;
        return val;
    }
    */

    /*
    int exchangeTree(Board brd, int depth, vector<Move>&history, vector<Move>&principal, int col, int alpha=MIN_EVAL, int beta=MAX_EVAL, int curDepth=1, int maxD=0) {
        bool gameOver=false;
        int maxEval,minEval,eval,sil;
        Board new_brd;
        vector<Move> ml(brd.moveList(true));
        if (depth>maxD) maxD=depth;
        if (ml.size()==0) {
            gameOver=true;
        }
        if (gameOver) { 
            if (!brd.inCheck(brd.activeColor)) return 0;
            if (brd.activeColor==White) return MIN_EVAL;
            else return MAX_EVAL;
        }
        if ((depth<=0) && ((brd.field[ml[0].to]==0) || depth < -3)) {
            return ml[0].eval;
        }
        sil=2;
        if (curDepth<maxD) {
            if (principal.size()<curDepth+1) principal.push_back(Move());
        }
        if (col==White) {
            maxEval=MIN_EVAL;
            for (Move mv : ml) {
                if (depth<=0 && brd.field[mv.to]==0 && sil==0) continue;
                if (sil>0) --sil;
                new_brd=brd.rawApply(mv);
                eval = new_brd.minimax(new_brd, depth-1, history, principal, Black, alpha, beta, curDepth+1, maxD);
                if (eval>maxEval) {
                    maxEval=eval;
                    if (curDepth<principal.size()) principal[curDepth]=mv;
                }
                if (eval>alpha) alpha=eval;
                if (alpha>=beta) break;
            }
            return maxEval;
        } else {
            minEval=MAX_EVAL;
            for (Move mv : ml) {
                if (depth<=0 && brd.field[mv.to]==0 && sil==0) continue;
                if (sil>0) --sil;
                new_brd=brd.rawApply(mv);
                eval = minimax(new_brd, depth-1, history, principal, White, alpha, beta, curDepth+1, maxD);
                if (eval<minEval) {
                    minEval=eval;
                    if (curDepth<principal.size()) principal[curDepth]=mv;
                }
                if (eval<beta) beta=eval;
                if (beta<=alpha) break;
            }
            return minEval;
        }
    }
*/
    

    int eval(bool relativeEval=false, bool verbose=false) {
        Board brd=*this;
        int evl=0;
        int pieceEvl=0;
        int attEvl=0;
        int defEvl=0;
        int kingEvl=0;
        int moveEvl=0;
        unsigned char fig,figTyp,pos;
        Color figCol;
        int pieceVals[]={0,100,290,300,500,900,100000};
        //int attVals[]={12,10,10,10,15,20,30};
        //int defVals[]={11,5,10,10,5,2,3};
        //                              attacked
        //                        -  p  n  b  r  q  k
        const int attMat[7][7]={{ 0, 0, 0, 0, 0, 0, 0},  // -
                                {25, 0,25,25,30,30,50},  // p
                                {10,10, 0, 0,25,30,40},  // n
                                {10,10, 0, 0,25,30,40},  // b   attacker
                                {10, 5, 5, 5, 0,25,30},  // r
                                { 2, 2, 2, 2, 2, 0,15},  // q
                                { 2, 4, 2, 2, 2, 2, 0},  // k
                        };
        //                             defended
        //                        -  p  n  b  r  q  k
        const int defMat[7][7]={{ 0, 0, 0, 0, 0, 0, 0},  // -
                                {25,25,25,25, 5, 0, 0},  // p
                                {10, 5, 5, 5, 5, 2, 0},  // n
                                {10, 5, 5, 5, 5, 2, 0},  // b   defender
                                {10, 5, 5, 5,20, 2, 0},  // r
                                { 2, 2, 2, 2, 2, 0, 0},  // q
                                { 2,10, 2, 2, 2, 2, 0},  // k
                        };
        
        vector<unsigned char>attW,attB;
        //unsigned char matrix[120];
        //memset(matrix,0xff,120);
        
        for (int fInd=21; fInd<99; fInd++) {
            fig=brd.field[fInd];
            if (fig==0xff) continue;
            figTyp=brd.field[fInd]>>2;
            figCol=(Color)(brd.field[fInd]&3);

            if (figCol == White) {
                pieceEvl += pieceVals[figTyp];

            } else {
                if (figCol == Black) {
                    pieceEvl -= pieceVals[figTyp];
                }
            }
        }
        /*
            attW=brd.attackers(fInd,figCol);
            for (unsigned char pos : attW) {
                int attackerTyp=brd.field[pos]>>2;
                if (figCol==Black) {
                    attEvl += attMat[attackerTyp][figTyp];  
                } else {
                    attEvl -= attMat[attackerTyp][figTyp];
                }
            }
            Color dC=White;
            if (figCol==White) dC=Black;
            attB=brd.attackers(fInd,dC);
            for (unsigned char pos : attB) {
                int defenderTyp=brd.field[pos]>>2;
                if (figCol==Black) {
                    defEvl -= defMat[defenderTyp][figTyp];  
                } else {
                    defEvl += defMat[defenderTyp][figTyp];
                }
            }

        }
        */
        Board brdw(brd),brdb(brd);
        brdw.activeColor=White;
        brdb.activeColor=Black;
        moveEvl=brdw.moveList().size()-brdb.moveList().size();

        if (brd.castleRights & CastleRights::CWK) kingEvl += 30; 
        if (brd.castleRights & CastleRights::CWQ) kingEvl += 30; 
        if (brd.castleRights & CastleRights::CBK) kingEvl += -30; 
        if (brd.castleRights & CastleRights::CBQ) kingEvl += -30;
        if (brd.hasCastled & White) kingEvl +=80; 
        if (brd.hasCastled & Black) kingEvl -=80; 
        evl=pieceEvl*3+kingEvl*2+moveEvl*4; // *8+attEvl/2+defEvl;
        if (relativeEval) {
            if (brd.activeColor == Black)
                evl = (-evl);
        }
        return evl;
    }

    /*
    int minimax(Board brd, int depth, vector<Move>&history, vector<Move>&principal, int col, int alpha=MIN_EVAL, int beta=MAX_EVAL, int curDepth=1, int maxD=0) {
        bool gameOver=false;
        int maxEval,minEval,eval,sil;
        int maxCaptures = 0;
        Board new_brd;
        vector<Move> ml(brd.moveList(true));
        vector<Move> history_state;

        //printf("(%d,%d) ",depth,curDepth);
        
        if (depth>maxD) maxD=depth;
        if (ml.size()==0) {
            gameOver=true;
        }
        if (gameOver) { 
            if (!brd.inCheck(brd.activeColor)) return 0;
            if (brd.activeColor==White) return MIN_EVAL;
            else return MAX_EVAL;
        }
        if ((depth<=0)) { // && ((brd.field[ml[0].to]==0) || depth <= maxCaptures) ) {
            return ml[0].eval;
        }

        //sil=2;

        //if (curDepth<maxD) {
        //    if (principal.size()<curDepth+1) principal.push_back(Move());
        //}

        //if (curDepth < 3) {
        //    wcout << " Cur d=" << curDepth << "hlen=" << history.size() << ": ";
        //    for (Move mv : history) {
        //        wcout << stringenc(mv.toUciWithEval()) << " ";
        //    }
        //    wcout << endl;
        //}
        if (col==White) {
            maxEval=MIN_EVAL;
            for (Move mv : ml) {
                //if (depth<=0 && brd.field[mv.to]==0 && sil==0) continue;
                //if (sil>0 && brd.field[mv.to]==0) --sil;
                new_brd=brd.rawApply(mv);
                history_state=history;
                history.push_back(mv);
                if (new_brd.activeColor != Black) {
                    wcout << "Internal error active Black expected!" << endl;
                    exit(-1);
                }
                eval = minimax(new_brd, depth-1, history, principal, Black, alpha, beta, curDepth+1, maxD);
                if (eval>maxEval) {
                    maxEval=eval;
                    //if (eval<minEval) {
                    // minEval=eval;
                    principal=history;
                }
                history=history_state;
                if (eval>alpha) alpha=eval;
                if (alpha>=beta) break;
            }
            return maxEval;
            //return minEval;
        } else {
            minEval=MAX_EVAL;
            for (Move mv : ml) {
                //if (depth<=0 && brd.field[mv.to]==0 && sil==0) continue;
                //if (sil>0 && brd.field[mv.to]==0) --sil;
                new_brd=brd.rawApply(mv);
                history_state=history;
                history.push_back(mv);
                if (new_brd.activeColor != White) {
                    wcout << "Internal error active White expected!" << endl;
                    exit(-1);
                }
                eval = minimax(new_brd, depth-1, history, principal, White, alpha, beta, curDepth+1, maxD);
                if (eval<minEval) {
                    minEval=eval;
                    //if (eval>maxEval) {
                    //maxEval=eval;
                    principal=history;
                }
                history=history_state;
                if (eval<beta) beta=eval;
                if (beta<=alpha) break;
            }
            return minEval;
            //return maxEval;
        }
    }
*/
    void printMoveList(vector<Move> &mv, const wchar_t *title) {
        wcout << title << ": ";
        for (Move m : mv) {
             wcout << stringenc(m.toUciWithEval()) << " ";
        }
        wcout << endl;
    }

    int negamax(Board brd, int depth, vector<Move> &history, vector<Move> &principal, int col,
                int alpha = MIN_EVAL, int beta = MAX_EVAL, int curDepth = 1, int maxD = 0) {
        bool gameOver=false;
        int endEval,eval,nextCol;
        int maxCaptures=-6;
        Board new_brd;
        vector<Move> ml(brd.moveList(true, true)); // negamax relative eval
        vector<Move> history_state;
        int sil;

        if (depth>maxD) maxD=depth;
        if (ml.size()==0) {
            gameOver=true;
        }
        if (gameOver) { 
            if (!brd.inCheck(brd.activeColor)) return 0;
            return MIN_EVAL;
        }
        if (depth<=0  && ((brd.field[ml[0].to]==0) || depth <= maxCaptures)) {
            return ml[0].eval;
        }

        endEval = MIN_EVAL;
        nextCol = invertColor(brd.activeColor);
        sil = 1;
        for (Move mv : ml) {
            if (depth <= 0 && brd.field[ml[0].to] == 0) {
                if (sil>0) --sil;
                else break;
            }
            new_brd=brd.rawApply(mv);
            history_state=history;
            history.push_back(mv);
            eval = negamax(new_brd, depth-1, history, principal, nextCol, alpha, beta, curDepth+1, maxD);
            if (col == White) {
                if (eval>endEval) {
                    endEval=eval;
                    if (curDepth==1) principal=history;
                    //wcout << L"(" << eval << L") wd=[" << curDepth << L"] -> ";
                    //printMoveList(principal,L"WH");
                }
                history=history_state;
                if (endEval>alpha) alpha=endEval;
                if (endEval>=beta) break;
            } else {
                if (eval<endEval) {
                    endEval=eval;
                    if (curDepth==1) principal=history;
                    //wcout << L"(" << eval << L") bd=[" << curDepth << L"] -> ";
                    //printMoveList(principal,L"BH");
                }
                history=history_state;
                if (endEval<beta) beta=endEval;
                if (endEval<=alpha) break;
            }
        }
        return endEval;
    }
    
    int minimax(Board brd, int depth, vector<Move>&history, vector<Move>&principal, int col,
                int alpha=MIN_EVAL, int beta=MAX_EVAL, int curDepth=1, int maxD=0) {
        bool gameOver=false;
        int endEval,eval,nextCol;
        int maxCaptures=-6;
        Board new_brd;
        vector<Move> ml(brd.moveList(true));
        vector<Move> history_state;
        int sil;

        if (depth>maxD) maxD=depth;
        if (ml.size()==0) {
            gameOver=true;
        }
        if (gameOver) { 
            if (!brd.inCheck(brd.activeColor)) return 0;
            if (brd.activeColor==White) return MIN_EVAL;
            else return MAX_EVAL;
        }
        if (depth<=0  && ((brd.field[ml[0].to]==0) || depth <= maxCaptures)) {
            return ml[0].eval;
        }

        if (col == White) {
            endEval = alpha; // MIN_EVAL;
            nextCol = Black;
        } else {
            endEval = beta; // MAX_EVAL;
            nextCol = White;
        }
        sil=1;
        for (Move mv : ml) {
            if (depth <= 0 && brd.field[ml[0].to] == 0) {
                if (sil>0) --sil;
                else break;
            }
            new_brd=brd.rawApply(mv);
            history_state=history;
            history.push_back(mv);
            eval = minimax(new_brd, depth-1, history, principal, nextCol, alpha, beta, curDepth+1, maxD);
            if (col == White) {
                if (eval>endEval) {
                    endEval=eval;
                    if (curDepth==1) principal=history;
                    //wcout << L"(" << eval << L") wd=[" << curDepth << L"] -> ";
                    //printMoveList(principal,L"WH");
                }
                history=history_state;
                if (endEval>alpha) alpha=endEval;
                if (endEval>=beta) break;
            } else {
                if (eval<endEval) {
                    endEval=eval;
                    if (curDepth==1) principal=history;
                    //wcout << L"(" << eval << L") bd=[" << curDepth << L"] -> ";
                    //printMoveList(principal,L"BH");
                }
                history=history_state;
                if (endEval<beta) beta=endEval;
                if (endEval<=alpha) break;
            }            
        }
        return endEval;
    }

    vector<Move> searchBestMove(Board brd, int depth, bool useNegamax=false) {
        vector<Move> ml(brd.moveList(true)), vml, best_principal, principal, history;
        Board newBoard;
        int bestEval, eval, vars;

        int curMaxDynamicDepth;
        for (int d=1; d<depth; d++) {
            curMaxDynamicDepth=0;
            if (brd.activeColor==White) bestEval=MIN_EVAL;
            else bestEval=MAX_EVAL;
            for (Move mv: ml) {
                newBoard=brd.rawApply(mv);
                principal.erase(principal.begin(),principal.end());
                //principal.push_back(mv);
                history.erase(history.begin(),history.end());
                history.push_back(mv);
                if (useNegamax) {
                    eval = negamax(newBoard, d, history, principal, newBoard.activeColor);
                    mv.eval = eval;
                    vml.push_back(mv);
                } else {
                    eval = minimax(newBoard, d, history, principal, newBoard.activeColor);
                    mv.eval = eval;
                    vml.push_back(mv);
                    if (brd.activeColor == White) {
                        if (eval > bestEval) {
                            bestEval = eval;
                            best_principal = principal;
                            printMoveList(best_principal, L" Current Wline");
                        }
                    } else {
                        if (eval < bestEval) {
                            bestEval = eval;
                            best_principal = principal;
                            printMoveList(best_principal, L" Current Bline");
                        }
                    }
                }
            }
            if (brd.activeColor==White || useNegamax) std::sort(vml.begin(), vml.end(), &Board::move_white_sorter);
            else std::sort(vml.begin(), vml.end(), &Board::move_black_sorter);
            ml=vml;
            //wcout << "Move list, depth=" << d;
            //printMoveList(ml,L"");            
            wcout << "Best line, depth=" << d;
            printMoveList(best_principal, L"");
            vml.erase(vml.begin(), vml.end());
        }
        return ml;
    }
};


struct PerftData {
    string name;
    string fen;
    vector<long unsigned int> perftcnt;

    PerftData(string name, string fen, vector<long unsigned int> perftcnt): name(name), fen(fen), perftcnt(perftcnt) {}
};

PerftData perftData[] = {
        // Reference: https://www.chessprogramming.org/Perft_Results
        {"end games", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", {14,191,2812,43238, 674624, 11030083, 178633661}},
        {"strange bugs", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {44, 1486, 62379, 2103487, 89941194}},
        {"start pos", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {20, 400, 8902, 197281, 4865609, 119060324, 3195901860, 84998978956, 2439530234167}}, //, 69352859712417, 2097651003696806, 62854969236701747, 1981066775000396239, 61885021521585529237])
        {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", {48, 2039, 97862, 4085603, 193690690, 8031647685}},
        {"position-4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {6, 264, 9467, 422333, 15833292, 706045033}},
        {"position-4-mirror", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", {6, 264, 9467, 422333, 15833292, 706045033}},
        {"position-6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", {46, 2079, 89890, 3894594, 164075551, 6923051137, 287188994746, 11923589843526, 490154852788714}},
        // Reference: https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9
        {"pej-1", "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b QK - 3 2", {8}},
        {"pej-2", "r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w QqKk - 2 2", {19}},
        {"pej-3", "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b QqKk - 3 2", {5}},
        {"pej-4", "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b QK - 3 2", {44}},
        {"pej-5", "rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w QK - 3 9", {39}},
        {"pej-6", "2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4", {9}},
        {"pej-7", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {44, 1486, 62379}},
        {"pej-8", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", {46, 2079, 89890}},
        {"pej-9", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", {18, 92, 1670, 10138, 185429, 1134888}},
        {"pej-10", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", {13, 102, 1266, 10276, 135655, 1015133}},
        {"pej-11", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", {15, 126, 1928, 13931, 206379, 1440467}},
        {"pej-12", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", {15, 66, 1198, 6399, 120330, 661072}},
        {"pej-13", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", {16, 71, 1286, 7418, 141077, 803711}},
        {"pej-14", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", {26, 1141, 27826, 1274206}},
        {"pej-15", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", {44, 1494, 50509, 1720476}},
        {"pej-16", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", {11, 133, 1442, 19174, 266199, 3821001}},
        {"pej-17", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", {29, 165, 5160, 31961, 1004658}},
        {"pej-18", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", {9, 40, 472, 2661, 38983, 217342}},
        {"pej-19", "8/P1k5/K7/8/8/8/8/8 w - - 0 1", {6, 27, 273, 1329, 18135, 92683}},
        {"pej-20", "K1k5/8/P7/8/8/8/8/8 w - - 0 1", {2, 6, 13, 63, 382, 2217}},
        {"pej-21", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", {10, 25, 268, 926, 10857, 43261, 567584}},
        {"pej-22", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", {37, 183, 6559, 23527}}
};


int perftTests() {
        int err=0;
    int ok=0;
    long knps=0;
    long start,end;
    Term::clr();
    for (PerftData perftSample : perftData) {
        //PerftData perftSample = perftData[20];
        wcout << stringenc(perftSample.name) << endl;
        Board brd(perftSample.fen);
        brd.printPos(&brd,-1);
        brd.printInfo();
        wcout << L"Depth: " << std::flush;
        int max=7;
        unsigned long maxNodes=1000000000;
        int maxd=max;
        if (perftSample.perftcnt.size() < max) maxd=perftSample.perftcnt.size();
        for (int i=0; i<maxd; i++) {
            if (perftSample.perftcnt[i]>maxNodes) break;
            wcout << to_wstring(i+1) << L" " << std::flush; 
            start=clock();
            unsigned long pc=brd.calcPerft(brd,i+1,0);
            end=clock();
            knps=(long)((double)(pc/1000)/((double)(end-start)/(double)CLOCKS_PER_SEC));
            if (pc!=perftSample.perftcnt[i]) {
                wcout << "Error: cnt: " << to_wstring(pc) << L" ground truth: " << to_wstring(perftSample.perftcnt[i]) << endl;
                err += 1;
                break;
            } else {
                if (pc>100000)
                    wcout << to_wstring(pc) << L" nodes, " << to_wstring(knps) << L" knps " << std::flush; 
                ok+=1;
            }
        }
        wcout << endl;
    }
    wcout << L"OK: " << to_wstring(ok) << L" Error: " << to_wstring(err) << endl;
    return err;
}
vector<Board::Move> doShowMoves(Board brd) {
    brd.printPos(&brd,-1);
    brd.printInfo();
    vector<Board::Move> ml(brd.moveList(true));
    for (Board::Move mv : ml) {
        string uci=mv.toUciWithEval();
        wcout << stringenc(uci) << L" ";
    }
    wcout << endl;
    return ml;
}

void miniGame() {
    
    string start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board brd(start_fen);

    for (int i=0; i<50; i++) {
        //ml=doShowMoves(brd);
        wcout << endl;
        brd.printPos(&brd,-1);
        vector<Board::Move> ml(brd.searchBestMove(brd,3));
        if (ml.size()==0) {
            wcout << L"Game over!" << endl;
            break;
        }
        brd=brd.rawApply(ml[0]); 
        wcout << stringenc(ml[0].toUciWithEval()) << endl;
    }
}

int main(int argc, char *argv[]) {
#ifndef __APPLE__
    std::setlocale(LC_ALL, "");
#else
    wcout << "Apple" << endl;
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale("en_US.UTF-8"));
#endif
    bool doPerf=false;
    if (argc>1) {
        if (!strcmp(argv[1],"-p")) doPerf=true;
    }

    if (!doPerf) miniGame();   
    else {
     int err=perftTests();
     exit(err);
    }
   return 0;
}
