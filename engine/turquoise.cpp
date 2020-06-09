#include <cstring>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <sstream>
#include <vector>

using std::endl;
using std::string;
using std::wcout;
using std::wstring;
using std::vector;
using std::stringstream;
using std::getline;

enum PieceType {Empty=0, Pawn=0b00000100, Knight=0b00001000, Bishop=0b00001100, 
                Rook=0b00010000, Queen=0b00010100, King=0b00011000};
enum Color {None=0, White=0b00000001, Black=0b00000010};
enum Piece {bp=PieceType::Pawn+Color::Black,
            bn=PieceType::Knight+Color::Black,
            bb=PieceType::Bishop+Color::Black,
            br=PieceType::Rook+Color::Black,
            bq=PieceType::Queen+Color::Black,
            bk=PieceType::King+Color::Black,
            wp=PieceType::Pawn+Color::White,
            wn=PieceType::Knight+Color::White,
            wb=PieceType::Bishop+Color::White,
            wr=PieceType::Rook+Color::White,
            wq=PieceType::Queen+Color::White,
            wk=PieceType::King+Color::White,
            };
enum CastleRights {NoCastling=0, WK=1, WQ=2, BK=4, BQ=8};

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
        sprintf(s, "%c[%d;%dm", esc, c, b);
        wcout << s;
    }

    // Clear screen
    static void clr() {
        unsigned char esc = 27;
        char s[256];
        sprintf(s, "%c[%dJ", esc, 2);
        wcout << s;
    }

    // Cursor goto(line,column)
    static void got(int cy, int cx) {
        unsigned char esc = 27;
        char s[256];
        sprintf(s, "%c[%d;%dH", esc, cy + 1, cx + 1);
        wcout << s;
    }
};

struct Board {
    unsigned char field[120];
    unsigned char castleRights;
    unsigned char fiftyMoves;
    unsigned char epPos;
    Color activeColor;
    unsigned int moveNumber;

    Board() {
        startPosition();
    }

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    Board(string fen, bool verbose=false) {
        memset(field,0,120);
        activeColor=Color::White;
        epPos=0;
        fiftyMoves=0;
        moveNumber=1;
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
        //wcout << std::to_wstring(y) << endl;
        //wcout << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(rows[y]) << endl;
        //wcout << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(wp) << endl;

            for (int i=0; i<rows[7-y].length(); i++) {
                if (x>7) {
                    if (verbose) wcout << L"Invalid position encoding! Invalid!" << endl;
                    return;
                }
                char c=rows[7-y][i];
        //wcout << std::to_wstring(i) << L" | " << std::to_wstring(x) << endl;
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
        if (parts[2].find('K')!=std::string::npos) castleRights|=CastleRights::WK;
        if (parts[2].find('Q')!=std::string::npos) castleRights|=CastleRights::WQ;
        if (parts[2].find('k')!=std::string::npos) castleRights|=CastleRights::BK;
        if (parts[2].find('q')!=std::string::npos) castleRights|=CastleRights::BQ;
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
                    f+=piece2asc(pc);
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
        if (castleRights & CastleRights::WK) cr+="K";
        if (castleRights & CastleRights::WQ) cr+="Q";
        if (castleRights & CastleRights::BK) cr+="k";
        if (castleRights & CastleRights::BQ) cr+="q";
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
            pc=(ind+1)<<2 | Color::White;
        } else {
            ind=bp.find(c);
            if (ind!=std::string::npos) {
                pc=(ind+1)<<2 | Color::Black;
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
        if (pc & Color::White) {
            return wp[(pc>>2)-1];
        } else {
            return bp[(pc>>2)-1];
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
        memset(field,0,120);
        castleRights=CastleRights::WK | CastleRights::WQ | CastleRights::BK | CastleRights::BQ;
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

    static inline unsigned char toPos(unsigned char y, unsigned char x) {
        return ((y + 2) * 10 + x + 1);
    }
    static inline unsigned char toPos(string posStr) {
        return ((posStr[1]-'1'+2)*10+std::tolower(posStr[0])-'a'+1);
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
        if (castleRights & CastleRights::WK) wcout << L"White-King ";
        if (castleRights & CastleRights::WQ) wcout << L"White-Queen ";
        if (castleRights & CastleRights::BK) wcout << L"Black-King ";
        if (castleRights & CastleRights::BQ) wcout << L"Black-Queen ";
        wcout << endl;

        if (epPos==0) wcout << L"No enpassant" << endl;
        else {
            string ep=pos2string(epPos);
            wcout << L"Enpassant: [" << std::to_wstring((int)epPos) << L"] " << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ep) << endl;
        }
        wcout << L"Fifty-move-state: " << std::to_wstring((int)fiftyMoves) << endl;
        wcout << L"Fifty-move-state: " << std::to_wstring((int)moveNumber) << endl;
    }

    struct Move {
        unsigned char from;
        unsigned char to;
        PieceType promote;

        Move() {
            from=0;
            to=0;
            promote=Empty;
        }
        Move(unsigned char from, unsigned char to, PieceType promote=Empty): from(from), to(to), promote(promote) {
        }
        // Decode uci and algebraic-notation strings to move [TODO: incomplete]
        Move(string alg, Board *brd=nullptr) {
            promote=Empty;
            from=0;
            to=0;
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

        if (activeColor==White) {
            if (field[pos+11]==Piece::bp) {
                pl.push_back(pos+11);
            }
            if (field[pos+9]==Piece::bp) {
                pl.push_back(pos+9);
            }
        } else if (activeColor==Black) {
            if (field[pos-11]==Piece::wp) {
                pl.push_back(pos-11);
            }
            if (field[pos-9]==Piece::wp) {
                pl.push_back(pos-9);
            }
        }
        char knightMoves[]{21, -21, 19, -19, 8, 12, -8, -12};
        char bishopMoves[]{11, -11, 9, -9};
        char rookMoves[]{10, -10, 1, -1};
        char kingQueenMoves[]{11, -11, 9, -9, 10, -10, 1, -1};
        for (char kn : knightMoves) {
            if (field[pos+kn]==PieceType::Knight | attColor) pl.push_back(pos+kn);
        }
        for (char km : kingQueenMoves) {
            if (field[pos+km]==PieceType::King | attColor) pl.push_back(pos+km);
        }
        for (char bm : bishopMoves) {
            unsigned char pn=pos+bm;
            while (true) {
                if ((field[pn]==PieceType::Bishop | attColor) || (field[pn]==PieceType::Queen | attColor)) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn]!=0) break;
                pn=pn+bm;
            }
        }
        for (char bm : rookMoves) {
            unsigned char pn=pos+bm;
            while (true) {
                if ((field[pn]==PieceType::Rook | attColor) || (field[pn]==PieceType::Queen | attColor)) {
                    pl.push_back(pn);
                    break;
                }
                if (field[pn]!=0) break;
                pn=pn+bm;
            }
        }
        return pl;
    }

    vector<unsigned char> attackedBy(unsigned char pos, Color col, PieceType p) {
        vector<unsigned char> ml;
        vector<unsigned char> pl=attackers(pos, col);
        for (unsigned char pi : pl) {
            if (field[pi]>>2 == p>>2) ml.push_back(pi);
        }
        return ml;
    }

    unsigned char kingsPos(Color col) {
        for (unsigned char c=21; c<99; c++) {
            if (field[c]==PieceType::King | col) return c;
        }
        return 0;
    }

    bool inCheck(Color col) {
        return attacked(kingsPos(col),col);
    }

    vector<Move> rawMoveList() {
        vector<Move> ml;
        int x,y,xt,yt;
        int dy,promoteRank,startPawn;
        const PieceType pp[]={Knight, Bishop, Rook, Queen};
        const char pawnCapW[]={9,11};
        const char pawnCapB[]={-9,-11};
        const char *pawnCap;
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
        const char kingQueenMoves[]={-1,-11,-10,-9,1,11,10,9};
        const char knightMoves[]={8,12,21,19,-8,-12,-21,-19};
        const char bishopMoves[]={-9,-11,9,11};
        const char rookMoves[]={-1,-10,1,10};
        unsigned char f,to;
        Color attColor;
        if (activeColor==Color::White) attColor=Black;
        for (unsigned char c=21; c<99; c++) {
            f=field[c];
            if (!f) continue;
            if (f & attColor) continue;
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
                        to=pawnCap[pci];
                        pos2coord(to,&yt,&xt);
                        if (field[to] & attColor || to==epPos) {
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
                        if (castleRights & CastleRights::BK) {
                            if ((field[c_e8]==King & Black) && (field[c_f8]==0) && (field[c_g8]==0) && (field[c_h8]==Rook & Black)) {
                                if (!attacked(c_e8, activeColor) && !attacked(c_f8, activeColor) && !attacked(c_g8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_g8, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::BQ) {
                            if ((field[c_e8]==King & Black) && (field[c_d8]==0) && (field[c_c8]==0) && (field[c_b8]==0) && (field[c_a8]==Rook & Black)) {
                                if (!attacked(c_c8, activeColor) && !attacked(c_d8, activeColor) && !attacked(c_e8, activeColor)) {
                                    ml.push_back(Move(c_e8, c_c8, Empty));
                                }
                            }
                        }
                    } else {
                        if (castleRights & CastleRights::WK) {
                            if ((field[c_e1]==King & White) && (field[c_f1]==0) && (field[c_g1]==0) && (field[c_h1]==Rook & White)) {
                                if (!attacked(c_e1, activeColor) && !attacked(c_f1, activeColor) && !attacked(c_g1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_g1, Empty));
                                }
                            }
                        }
                        if (castleRights & CastleRights::WQ) {
                            if ((field[c_e1]==King & White) && (field[c_d1]==0) && (field[c_c1]==0) && (field[c_b1]==0) && (field[c_a1]==Rook & White)) {
                                if (!attacked(c_c1, activeColor) && !attacked(c_d1, activeColor) && !attacked(c_e1, activeColor)) {
                                    ml.push_back(Move(c_e1, c_c1, Empty));
                                }
                            }
                        }
                    }
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        if ((field[to]==0) || (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Knight:
                    for (int di=0; di<8; ++di) {
                        to=c+knightMoves[di];
                        if ((field[to]==0) || (field[to]&attColor)) {
                            ml.push_back(Move(c,to,Empty));
                        }
                    }
                    break;
                case PieceType::Bishop:
                    for (int di=0; di<4; ++di) {
                        to=c+bishopMoves[di];
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=knightMoves[di];
                        }
                    }
                    break;
                case PieceType::Rook:
                    for (int di=0; di<4; ++di) {
                        to=c+rookMoves[di];
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=knightMoves[di];
                        }
                    }
                    break;
                case PieceType::Queen:
                    for (int di=0; di<8; ++di) {
                        to=c+kingQueenMoves[di];
                        while (true) {
                            if ((field[to]==0) || (field[to]&attColor)) {
                                ml.push_back(Move(c,to,Empty));
                            } else break;
                            if (field[to]&attColor) break;
                            to+=knightMoves[di];
                        }
                    }
                    break;
                default:
                    wcout << L"Unidentified flying object" << endl;
                    break;
            }
        }
        return ml;
    }

    Board apply(Move mv) {
        Board brd=*this;
        // XXX
        return brd;
    }

    vector<Move> moveList() {
        vector<Move> ml=rawMoveList();
        vector<Move> vml;
        for (auto m : ml) {
            Board brd=apply(m);
            if (!brd.inCheck(activeColor)) {
                vml.push_back(m);
            }
        }
        return vml;
    }

};

int main(int argc, char *argv[]) {
#ifndef __APPLE__
    std::setlocale(LC_ALL, "");
#else
    setlocale(LC_ALL, "");
#endif
    string fen="8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    string start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board brd(start_fen,true);
    //Board brd;
    Term::clr();
    brd.printPos(&brd);
    brd.printInfo();
    string fen2=brd.fen();
    wcout << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(start_fen) << endl;
    wcout << std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(fen2) << endl;
    return 0;
}