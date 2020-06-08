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

typedef struct t_board {
    unsigned char field[120];
    unsigned char castleRights;
    unsigned char fiftyMoves;
    unsigned char epPos;
    unsigned char activeColor;
    unsigned int moveNumber;

    t_board() {
        startPosition();
    }

    // rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    t_board(string fen, bool verbose=false) {
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

    unsigned char asc2piece(char c) {
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

    PieceType asc2piecetype(char c) {
        const string bp="pnbrqk";
        PieceType pt=PieceType::Empty;
        int ind=bp.find(std::tolower(c));
        if (ind!=std::string::npos) {
            pt=(PieceType)(ind+1);
        }
        return pt;
    }

    char piece2asc(unsigned char pc) {
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

    inline unsigned char toPos(unsigned char y, unsigned char x) {
        return ((y + 2) * 10 + x + 1);
    }
    inline unsigned char toPos(string posStr) {
        return ((posStr[1]-'1'+2)*10+std::tolower(posStr[0])-'a'+1);
    }

    void pos2coord(unsigned char pos, int *y, int *x) {
        if (y) *y=pos/10-2;
        if (x) *x=(pos%10)-1;
    }

    string pos2string(unsigned char pos) {
        char c1=pos/10-2+'1';
        char c2=(pos%10)-1+'a';
        string coord=string(1,c2)+string(1,c1);
        return coord;
    }

    void printPos(struct t_board *brd, int py = 0, int px = 0) {
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

} Board;

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