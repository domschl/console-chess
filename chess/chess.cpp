#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>

using std::wcout; using std::endl; using std::string; using std::vector;
using std::wstring;

// ---- Minimal ANSI terminal controls---------------------------

// Set color[0-7] and background-coloe[0-7]
void cc(int col,int bk) {
    unsigned char esc=27;
    int c=col+30;
    int b=bk+40;
    char s[256];
    sprintf(s,"%c[%d;%dm",esc,c,b);
    wcout << s;
}

// Clear screen
void clr() {
    unsigned char esc=27;
    char s[256];
    sprintf(s,"%c[%dJ",esc,2);
    wcout << s;
}

// Cursor goto(line,column)
void got(int cy, int cx) {
    unsigned char esc=27;
    char s[256];
    sprintf(s,"%c[%d;%dH",esc,cy+1,cx+1);
    wcout << s;
}

//----------------Chess------------------------------
// Constant for a chess position
#define POS_SIZE 144 // must be multiple of four

#define CW 1    // White pieces >0
#define CB -1   // black pieces <0

#define FIELD_INV   127   // beyond border: board is 10x12 with borders
#define FIELD_EMPTY 0     // empty
#define FIELD_WP    1     // white pawn
#define FIELD_WN    2     //   knight
#define FIELD_WB    3     //   bishop
#define FIELD_WR    4     //   rock
#define FIELD_WQ    5     //   queen
#define FIELD_WK    6     //   king
#define FIELD_BP    -1    // black pawn
#define FIELD_BN    -2    //   knight
#define FIELD_BB    -3    //   bishop
#define FIELD_BR    -4    //   rock
#define FIELD_BQ    -5    //   queen
#define FIELD_BK    -6    //   king

#define POS_CTRL_OFFSET 120
#define POS_WKC     (POS_CTRL_OFFSET+1)   // Flag is white can castle king-side: 0:no 1: yes 2: done
#define POS_WQC     (POS_CTRL_OFFSET+2)   // queen side:  0:no 1: yes 2: done
#define POS_BKC     (POS_CTRL_OFFSET+3)   // Black costle king side:  0:no 1: yes 2: done
#define POS_BQC     (POS_CTRL_OFFSET+4)   // Queen side:  0:no 1: yes 2: done
#define POS_LASTMV0 (POS_CTRL_OFFSET+5)   // Last move from (for e.p.)
#define POS_LASTMV1 (POS_CTRL_OFFSET+6)   // Last move to (for e.p.)
#define POS_CAPT    (POS_CTRL_OFFSET+7)   // Piece captured by last move
#define POS_WK      (POS_CTRL_OFFSET+8)   // cache for white king pos
#define POS_BK      (POS_CTRL_OFFSET+9)   // cache for black king pos
#define POS_WK_CHK  (POS_CTRL_OFFSET+10)  // 1: white king in check
#define POS_BK_CHK  (POS_CTRL_OFFSET+11)  // 1: black king in check
#define POS_NEXTCOL (POS_CTRL_OFFSET+12)  // 1: white to move, -1 black to move.

#define POS_SCORE   (POS_CTRL_OFFSET+16)  // Score of position as int (uhhh)
#define POS_CURDEP  (POS_CTRL_OFFSET+20)  // Cache stores current depth

void checkStatics() {
    if (POS_CTRL_OFFSET+20+sizeof(int)!=POS_SIZE) {
        wcout << L"Internal error with POS_SIZE!" << endl;
        exit(-1);
    }
    if (sizeof(int)!=4) {
        wcout << L"Integer-size assumed to 4, but is different" << endl;
        exit(-1);
    }
}

#define FOF(y,x) ((y+2)*10+x+1)
#define COL(of) (of%10-1)
#define ROW(of) (of/10-2)

#define SC_MIN (-9999999)
#define SC_MAX ( 9999999)
#define SC_TIMEOUT_INVALID (-3333333)

//sort function: sort two chess positions according to their score
bool scoresort(char *p1, char *p2) {
    int sc1=*(int *)&p1[POS_SCORE];
    int sc2=*(int *)&p2[POS_SCORE];
    return sc1>sc2;
}

unsigned int crc32(const unsigned char *buf, int len) {
    unsigned int crc32=0xffffffff;
    static const unsigned int crc32table[256] = {
        0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
        0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
        0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
        0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
        0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
        0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
        0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
        0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
        0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
        0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
        0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
        0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
        0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
        0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
        0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
        0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
        0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
        0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
        0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
        0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
        0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
        0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
        0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
        0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
        0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
        0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
        0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
        0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
        0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
        0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
        0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
        0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4,
        };

    while (len > 0)
    {
      crc32 = crc32table[*buf ^ ((crc32 >> 24) & 0xff)] ^ (crc32 << 8);
      buf++;
      len--;
    }
    return crc32;
}

class Chess {
public:
    char pos[POS_SIZE];

    int cachesize=10000000;
    int cacheptr=0;

    int cacheclash=0;
    int cacheclashr=0;
    int cachehit=0;
    int cachemiss=0;

    std::map<int, int[2]> cache{};
    vector <char *>cachepositions{};

    unsigned int cacheHash(char *pos) {
        unsigned int h=crc32((const unsigned char *)pos,POS_SIZE);
        return h;
    }

    void cacheInsert(const char *pos, int score, int curdepth) {
        char *p;
        p=cachepositions[cacheptr];
        memcpy(p,pos,POS_SIZE*sizeof(char));
        *(int *)&p[POS_CURDEP]=curdepth;
        p[POS_LASTMV0]=0;
        p[POS_LASTMV1]=0;
        unsigned int h=cacheHash(p);
        if (cache.find(h)==cache.end()) {
            cache[h][0]=score;
            cache[h][1]=cacheptr;
            cacheptr = (cacheptr+1)%cachesize;
        } else {
            memset(p,sizeof(char)*POS_SIZE,0);
            ++cacheclash;
        }
    }

    bool cacheRead(const char *pos, int *pscore, int curdepth) {
        char pc[POS_SIZE];
        memcpy(pc,pos,POS_SIZE*sizeof(char));
        pc[POS_LASTMV0]=0;
        pc[POS_LASTMV1]=0;
        *(int *)&pc[POS_CURDEP]=curdepth;
        unsigned int h=cacheHash(pc);
        if (cache.find(h)!=cache.end()) {
            int idx=cache[h][1];
            if (idx>cachepositions.size()) {
                wcout << L"Illegal index: " << idx << endl;
                exit(-1);
            }

            if (!memcmp(cachepositions[idx],pc,sizeof(char)*POS_SIZE)) {
                ++cachehit;
                *pscore=cache[h][0];
                return true;
            } else {
                ++cacheclashr;
                return false;
            }
        } else {
            ++cachemiss;
            return false;
        }
    }

    Chess(int cachsize=10000000) {
        int i;
        cachesize=cachsize;
        memset(pos,0,POS_SIZE*sizeof(char));
        for (i=0; i<POS_CTRL_OFFSET; i++) pos[i]=FIELD_INV;
        int cx,cy;
        for (cy=1; cy<7; cy++) {
            for (cx=0; cx<8; cx++) {
                switch(cy) {
                    case 1:
                        pos[FOF(cy,cx)]=FIELD_BP;
                        break;
                    case 6:
                        pos[FOF(cy,cx)]=FIELD_WP;
                        break;
                    default:
                        pos[FOF(cy,cx)]=FIELD_EMPTY;
                        break;
                }
            }
        }
        char iF[]={FIELD_WR, FIELD_WN, FIELD_WB, FIELD_WQ, FIELD_WK, FIELD_WB, FIELD_WN, FIELD_WR};
        for (cx=0; cx<8; cx++) {
            pos[FOF(0,cx)]=iF[cx] * CB;
            pos[FOF(7,cx)]=iF[cx] * CW;
        }
        pos[POS_WKC]=1; //White can castle king side
        pos[POS_WQC]=1; // queen side
        pos[POS_BKC]=1; // black castle
        pos[POS_BQC]=1;
        pos[POS_LASTMV0]=0; // last move (for ep)
        pos[POS_LASTMV1]=0; // last move (for ep)
        pos[POS_WK]=FOF(7,4); // cache of white king pos
        pos[POS_BK]=FOF(0,4); // bl king
        pos[POS_NEXTCOL]=CW; // White to move
        //Cache init
        for (int i=0; i<cachesize; i++) {
            cachepositions.push_back((char *)malloc(POS_SIZE*sizeof(char)));
        }
        wcout << L"cache-size:" << cachepositions.size() << endl;
    }
    ~Chess() {
        for (int i=0; i<cachesize; i++) {
            free(cachepositions[i]);
        }
    }

    void move2String(char *pos, wstring& strmov) {
        wstring cPw{L"♟♞♝♜♛♚"};
        wstring cPb{L"♙♘♗♖♕♔"};
        int p1 = pos[POS_LASTMV0];
        int p2 = pos[POS_LASTMV1];
        int f=pos[p2];
        wstring fi{L"?"};
        if (f<0) fi=cPb.substr(-f-1,1);
        if (f>0) fi=cPw.substr(f-1,1);
        wstring out;
        wstring y0,x0,y1,x1;
        wstring sp{L"-"};
        if (pos[POS_CAPT]!=FIELD_EMPTY) sp=L"x";
        x0=((wchar_t)(L'A'+COL(p1)));
        y0=std::to_wstring(8-ROW(p1));
        x1=((wchar_t)(L'A'+COL(p2)));
        y1=std::to_wstring(8-ROW(p2));
        out=fi+x0+y0+sp+x1+y1;
        strmov=out;
    }

    void printPos(char *pos, int py=0, int px=0) {
        wchar_t *cPw=(wchar_t *)L"♟♞♝♜♛♚";
        wchar_t *cPb=(wchar_t *)L"♙♘♗♖♕♔";
        int cy,cx,fo,cl;
        char f;
        wchar_t c;
        if (py!=-1) got(py,px);
        for (cy=0; cy<8; cy++) {
            if (py!=-1) got(py+cy,px);
            for (cx=0; cx<8; cx++) {
                f=pos[FOF(cy,cx)];
                cl=(cx+cy)%2;
                if (cl) cc(7,0);
                else cc(0,7);
                if (cl==0) cl=-1;
                if (f==0) wcout << "   ";
                else {
                    if (f<0) fo=-f;
                    else fo=f;
                    if (f*cl>0) c=(wchar_t)cPw[fo-1];
                    else c=(wchar_t)cPb[fo-1];
                    wcout << L" " << c << L" ";
                }
            }
            cc(7,0);
            wcout << endl;
         }

    }

    bool isThreatened(char *pos,int cy, int cx, int col) {
        int no[]{21,-21,19,-19,8,12,-8,-12};
        int bo[]{11,-11,9,-9};
        int ro[]{10,-10,1,-1};
        int ko[]{11,-11,9,-9,10,-10,1,-1};
        int d,of;
        of=FOF(cy,cx);
        if (pos[FOF(cy-col,cx+1)]==FIELD_BP * col) return true;
        if (pos[FOF(cy-col,cx-1)]==FIELD_BP * col) return true;
        for (d=0; d<sizeof(no)/sizeof(no[0]); d++) {
            if (pos[of+no[d]]==FIELD_BN * col) return true;
        }
        for (d=0; d<sizeof(bo)/sizeof(bo[0]); d++) {
            bool val=true;
            int of0=of;
            while (val) {
                of0 += bo[d];
                if (pos[of0]==FIELD_BB * col || pos[of0]==FIELD_BQ * col) return true;
                if (pos[of0]!=FIELD_EMPTY) val=false;
            }
        }
        for (d=0; d<sizeof(ro)/sizeof(ro[0]); d++) {
            bool val=true;
            int of0=of;
            while (val) {
                of0 += ro[d];
                if (pos[of0]==FIELD_BR * col || pos[of0]==FIELD_BQ * col) return true;
                if (pos[of0]!=FIELD_EMPTY) val=false;
            }
        }
        for (d=0; d<sizeof(ko)/sizeof(ko[0]); d++) {
            if (pos[of+ko[d]]==FIELD_BK * col) return true;
        }
        return false;
    }

    void addMove(vector<char *>&ml,char *pos, int of,int ofn,int f=0,int of1=0,int ofn1=0) {
        char *posn=(char *)malloc(POS_SIZE*sizeof(char));
        memcpy((void *)posn,pos,POS_SIZE*sizeof(char));
        posn[POS_CAPT]=FIELD_EMPTY;
        if (posn[ofn]!=FIELD_EMPTY) posn[POS_CAPT]=posn[ofn];
        posn[ofn]=pos[of];
        posn[of]=FIELD_EMPTY;
        posn[POS_LASTMV0]=of;
        posn[POS_LASTMV1]=ofn;
        posn[POS_NEXTCOL]=pos[POS_NEXTCOL]*(-1);
        if (f!=0) posn[ofn]=f;
        int f0=posn[ofn];
        int bk, wk;
        //int n=ofn;
        if (of1!=0) {
            posn[ofn1]=posn[of1];
            posn[of1]=FIELD_EMPTY;
            //posn[POS_LASTMV1]=ofn1;
            //n=ofn1;
        }
        if (f0==FIELD_WK && (posn[POS_WKC]==1 || posn[POS_WQC]==1)) {
            if (of==FOF(7,4) && (ofn==FOF(7,6) || ofn==FOF(7,2))) {
                posn[POS_WKC]=2;
                posn[POS_WQC]=2;
            } else {
                posn[POS_WKC]=0;
                posn[POS_WQC]=0;
            }
            posn[POS_WK]=ofn;
        }
        if (f0==FIELD_BK && (posn[POS_BKC]==1 || posn[POS_BQC]==1)) {
            if (of==FOF(0,4) && (ofn==FOF(0,6) || ofn==FOF(0,2))) {
                posn[POS_BKC]=2;
                posn[POS_BQC]=2;
            } else {
                posn[POS_BKC]=0;
                posn[POS_BQC]=0;
            }
            posn[POS_BK]=ofn;
        }
        if (f0==FIELD_BK) {
            posn[POS_BK]=ofn;
        }
        if (f0==FIELD_WK) {
            posn[POS_WK]=ofn;
        }
        if (f0==FIELD_WR && of==FOF(7,0)) posn[POS_WQC]=0;
        if (f0==FIELD_WR && of==FOF(7,7)) posn[POS_WKC]=0;
        if (f0==FIELD_BR && of==FOF(0,0)) posn[POS_BQC]=0;
        if (f0==FIELD_BR && of==FOF(0,7)) posn[POS_BKC]=0;
        bool val=true;
        posn[POS_WK_CHK]=0;
        posn[POS_BK_CHK]=0;
        bk=posn[POS_BK];
        if (isThreatened(posn, ROW(bk), COL(bk), CB)) posn[POS_BK_CHK]=1;
        wk=posn[POS_WK];
        if (isThreatened(posn, ROW(wk), COL(wk), CW)) posn[POS_WK_CHK]=1;
        if (f0<0) {
            if (posn[POS_BK_CHK]) val=false;
        }
        if (f0>0) {
            if (posn[POS_WK_CHK]) val=false;
        }
        //wcout << ROW(of) << COL(of) << " " << ROW(ofn) << COL(ofn) << " " << val << endl;
        if (val) ml.push_back(posn);
        else free(posn);
    }


    vector<char *> moveList(char *pos, int col) {
        int no[]{21,-21,19,-19,8,12,-8,-12};
        int bo[]{11,-11,9,-9};
        int ro[]{10,-10,1,-1};
        int qo[]{11,-11,9,-9,10,-10,1,-1};
        vector<char *>ml;
        int cx, cy, of, n, ofn, fig;
        bool val;
        for (cy=0; cy<8; cy++) {
            for (cx=0; cx<8; cx++) {
                of=FOF(cy,cx);
                fig=pos[of];
                if (!fig) continue;
                fig = fig * col;
                // wcout << cy << " " << cx << " " << of << " " << ROW(of) << " " << COL(of) << " " << fig << endl;
                if (fig<0) continue;
                if (fig==FIELD_WP) {
                    ofn=of-10*col;
                    if (pos[ofn]==FIELD_EMPTY) {
                        if (ROW(ofn)==0) {
                            addMove(ml,pos,of,ofn,FIELD_WQ,0,0);
                            addMove(ml,pos,of,ofn,FIELD_WN,0,0);
                        } else {
                            if (ROW(ofn)==7) {
                                addMove(ml,pos,of,ofn,FIELD_BQ,0,0);
                                addMove(ml,pos,of,ofn,FIELD_BN,0,0);
                            } else {
                                addMove(ml,pos,of,ofn,0,0,0);
                                if ((ROW(of)==6 && col==CW) || (ROW(of)==1 && col==CB)) {
                                    ofn=of-20*col;
                                    if (pos[ofn]==FIELD_EMPTY) addMove(ml,pos,of,ofn,0,0,0);
                                }
                            }
                        }
                    }
                    ofn=of-9*col;
                    if (pos[ofn]!=FIELD_INV && pos[ofn] * col < 0) {
                        if (ROW(ofn)==0) {
                            addMove(ml,pos,of,ofn,FIELD_WQ,0,0);
                            addMove(ml,pos,of,ofn,FIELD_WN,0,0);
                        } else {
                            if (ROW(ofn)==7) {
                                addMove(ml,pos,of,ofn,FIELD_BQ,0,0);
                                addMove(ml,pos,of,ofn,FIELD_BN,0,0);
                            } else {
                                addMove(ml,pos,of,ofn,0,0,0);
                            }
                        }
                    }
                    ofn=of-11*col;
                    if (pos[ofn]!=FIELD_INV && pos[ofn] * col < 0) {
                        if (ROW(ofn)==0) {
                            addMove(ml,pos,of,ofn,FIELD_WQ,0,0);
                            addMove(ml,pos,of,ofn,FIELD_WN,0,0);
                        } else {
                            if (ROW(ofn)==7) {
                                addMove(ml,pos,of,ofn,FIELD_BQ,0,0);
                                addMove(ml,pos,of,ofn,FIELD_BN,0,0);
                            } else {
                                addMove(ml,pos,of,ofn,0,0,0);
                            }
                        }
                    }
                    // XXX: e.p.
                }
                if (fig==FIELD_WN) {
                    for (n=0; n<sizeof(no)/sizeof(no[0]); n++) {
                        ofn=of+no[n];
                        if (pos[ofn]!=FIELD_INV && pos[ofn]*col <= 0) {
                            addMove(ml,pos,of,ofn);
                        }
                    }
                }
                if (fig==FIELD_WB) {
                    for (n=0; n<sizeof(bo)/sizeof(bo[0]); n++) {
                        val=true;
                        ofn=of;
                        while (val) {
                            ofn=ofn+bo[n];
                            if (pos[ofn]!=FIELD_INV && pos[ofn]*col <= 0) {
                                addMove(ml,pos,of,ofn);
                            }
                            if (pos[ofn] !=FIELD_EMPTY) val=false;
                        }
                    }
                }
                if (fig==FIELD_WR) {
                    for (n=0; n<sizeof(ro)/sizeof(ro[0]); n++) {
                        val=true;
                        ofn=of;
                        while (val) {
                            ofn=ofn+ro[n];
                            if (pos[ofn]!=FIELD_INV && pos[ofn]*col <= 0) {
                                addMove(ml,pos,of,ofn);
                            }
                            if (pos[ofn] !=0) val=false;
                        }
                    }
                }
                if (fig==FIELD_WQ) {
                    for (n=0; n<sizeof(qo)/sizeof(qo[0]); n++) {
                        val=true;
                        ofn=of;
                        while (val) {
                            ofn=ofn+qo[n];
                            if (pos[ofn]!=FIELD_INV && pos[ofn]*col <= 0) {
                                addMove(ml,pos,of,ofn);
                            }
                            if (pos[ofn] !=0) val=false;
                        }
                    }
                }
                if (fig==FIELD_WK) {
                    for (n=0; n<sizeof(qo)/sizeof(qo[0]); n++) {
                        ofn=of+qo[n];
                        if (pos[ofn]!=FIELD_INV && pos[ofn]*col <= 0) {
                            addMove(ml,pos,of,ofn);
                        }
                    }
                    if (col==CW) {
                        if (pos[POS_WKC]==1) {
                            if (pos[FOF(7,5)]==0 && pos[FOF(7,6)]==0 && !isThreatened(pos,7,4,CW) && !isThreatened(pos,7,5,1)) {
                                addMove(ml,pos,FOF(7,4),FOF(7,6),0,FOF(7,7),FOF(7,5));
                            }
                        }
                        if (pos[POS_WQC]==1) {
                            if (pos[FOF(7,1)]==0 && pos[FOF(7,2)]==0 && pos[FOF(7,3)]==0 && !isThreatened(pos,7,3,1) && !isThreatened(pos,7,4,1)) {
                                addMove(ml,pos,FOF(7,4),FOF(7,2),0,FOF(7,0),FOF(7,3));
                            }
                        }
                    }
                    if (col==CB) {
                        if (pos[POS_BKC]==1) {
                            if (pos[FOF(0,5)]==0 && pos[FOF(0,6)]==0 && !isThreatened(pos,0,4,CB) && !isThreatened(pos,0,5,CB)) {
                                addMove(ml,pos,FOF(0,4),FOF(0,6),0,FOF(0,7),FOF(0,5));
                            }
                        }
                        if (pos[POS_BQC]==1) {
                            if (pos[FOF(0,1)]==0 && pos[FOF(0,2)]==0 && pos[FOF(0,3)]==0 && !isThreatened(pos,0,3,CB) && !isThreatened(pos,0,4,CB)) {
                                addMove(ml,pos,FOF(0,4),FOF(0,2),0,FOF(0,0),FOF(0,3));
                            }
                        }
                    }
                }
            }
        }
        int sc;
        for (auto pi : ml) {
            sc=evalPos(pi,col);
            *(int *)&pi[POS_SCORE]=sc;   // uhhh 1982..
        }
        std::sort(ml.begin(), ml.end(), scoresort);
        return ml;
    }

    int stratVal(char *pos, int col) {
        int cx,cy;
        int sc=0;
        int sval[POS_SIZE];
        char figAttack[]{2,5,4,3,3,0,0,1,1,1,1,0,0};
        int no[]{21,-21,19,-19,8,12,-8,-12};
        int bo[]{11,-11,9,-9};
        int ro[]{10,-10,1,-1};
        int ko[]{11,-11,9,-9,10,-10,1,-1};
        int d,of,f;
        int vf;
        int atc=0;
        int mov;
        memset(sval,0,POS_SIZE*sizeof(int));
        for (cy=0; cy<8; cy++) {
            for (cx=0; cx<8; cx++) {
                of=FOF(cy,cx);
                f=pos[of];
                if (pos[FOF(cy-col,cx+1)]==FIELD_BP * col) {sval[of] -= 4;}//atc-=vf;
                if (pos[FOF(cy-col,cx-1)]==FIELD_BP * col) {sval[of] -=4;}///atc-=vf;
                if (pos[FOF(cy+col,cx+1)]==FIELD_WP * col) sval[of] += 4; //atc+=vf;
                if (pos[FOF(cy+col,cx-1)]==FIELD_WP * col) sval[of] += 4; //atc+=vf;
                for (d=0; d<sizeof(no)/sizeof(no[0]); d++) {
                    if (pos[of+no[d]]==FIELD_BN * col) sval[of] -= 3; // atc-=vf;
                    if (pos[of+no[d]]==FIELD_WN * col) sval[of] += 3; // atc+=vf;
                }
                for (d=0; d<sizeof(bo)/sizeof(bo[0]); d++) {
                    bool val=true;
                    int of0=of;
                    while (val) {
                        of0 += bo[d];
                        if (pos[of0]==FIELD_BB * col) sval[of] -= 3; //atc-=vf;
                        if (pos[of0]==FIELD_BQ * col) sval[of] -= 2; //atc-=vf;
                        if (pos[of0]==FIELD_WB * col) sval[of] += 3; //atc+=vf;
                        if (pos[of0]==FIELD_WQ * col) sval[of] += 2; //atc+=vf;
                        if (pos[of0]!=FIELD_EMPTY) val=false;
                    }
                }
                for (d=0; d<sizeof(ro)/sizeof(ro[0]); d++) {
                    bool val=true;
                    int of0=of;
                    while (val) {
                        of0 += ro[d];
                        if (pos[of0]==FIELD_BR * col) sval[of] -= 3; // atc-=vf;
                        if (pos[of0]==FIELD_BQ * col) sval[of] -= 2; // atc+=vf;
                        if (pos[of0]==FIELD_WR * col) sval[of] += 3; // atc-=vf;
                        if (pos[of0]==FIELD_WQ * col) sval[of] += 2; // atc+=vf;
                        if (pos[of0]!=FIELD_EMPTY) val=false;
                    }
                }
                for (d=0; d<sizeof(ko)/sizeof(ko[0]); d++) {
                    if (pos[of+ko[d]]==FIELD_BK * col) sval[of] -= 2; //atc-=vf;
                    if (pos[of+ko[d]]==FIELD_WK * col) sval[of] += 2; // atc+=vf;
                }
            }
        }

        int norm=3;
        int normdiv=4;
        mov=0;
        int ac;
        for (int i=0; i<POS_SIZE; i++) {
            mov += sval[i];
            if (sval[i]) {
                //atc+=sval[i]*anorm*col
                if (sval[i]<0) ac=-1; else ac=1;
                vf=figAttack[pos[i]*col*ac+6];
                //if (sval[i]<0) atc -= vf*norm;
                //else atc += vf*norm;
                atc += vf*norm*sval[i]/normdiv;
            }
        }
        int movnorm=2;
        atc +=mov*movnorm;

        int spcnorm=4;
        int spn=0;
        int csp;
        int lsp;
        int s0pr=4;
        int s1div=3;
        for (cx=0; cx<8; cx++) {
            lsp=0;
            for (cy=7; cy>=0; cy--) {
                csp=pos[FOF(cy,cx)]*col;
                if (csp<0) break;
                if (csp>0) lsp=7-cy;
            }
            spn+=s0pr*lsp;
            lsp=0;
            for (cy=0; cy<8; cy++) {
                csp=pos[FOF(cy,cx)]*col;
                if (csp>0) break;
                if (csp<0) lsp=cy;
            }
            spn-=s0pr*lsp;
        }
        for (cx=0; cx<8; cx++) {
            lsp=0;
            for (cy=7; cy>=0; cy--) {
                csp=sval[FOF(cy,cx)]*col;
                if (csp<0) break;
                if (csp>0) lsp=7-cy;
            }
            spn+=lsp/s1div;
            lsp=0;
            for (cy=0; cy<8; cy++) {
                csp=sval[FOF(cy,cx)]*col;
                if (csp>0) break;
                if (csp<0) lsp=cy;
            }
            spn-=lsp/s1div;
        }
        atc+=spcnorm*spn;

        int znorm=4;
        atc +=(sval[FOF(3,3)] + sval[FOF(3,4)] + sval[FOF(4,3)] + sval[FOF(4,4)]) * znorm;
        atc +=(sval[FOF(3,2)] + sval[FOF(3,5)] + sval[FOF(4,2)] + sval[FOF(4,5)]) * znorm;

        int ofx, ofy;
        ofy=ROW(pos[POS_WK]);
        ofx=COL(pos[POS_WK]);
        int ks=0;
        int knorm=1;
        int kdiv=1;
        for (cy=-2; cy<3; cy++) {
            for (cy=-2; cy<3; cy++) {
                if (std::abs(cx)>std::abs(cy)) d=std::abs(cx);
                else d=std::abs(cy);
                d=3-d;
                if (d>0)
                    ks += (sval[FOF(ofy+cy,ofx+cx)]*d)*knorm;
            }
        }

        ofy=ROW(pos[POS_BK]);
        ofx=COL(pos[POS_BK]);
        //ks=0.0;
        for (cy=-2; cy<3; cy++) {
            for (cy=-2; cy<3; cy++) {
                if (std::abs(cx)>std::abs(cy)) d=std::abs(cx);
                else d=std::abs(cy);
                d=3-d;
                if (d>0)
                    ks += (sval[FOF(ofy+cy,ofx+cx)]*d)*knorm;
            }
        }
        ks /= kdiv;

        int scdiv=4;
        sc=(knorm+atc)/scdiv;

        int cnorm=1;
        if (pos[POS_WKC]==0) sc-=col*15*cnorm;
        if (pos[POS_WQC]==0) sc-=col*10*cnorm;
        if (pos[POS_BKC]==0) sc+=col*15*cnorm;
        if (pos[POS_BQC]==0) sc+=col*10*cnorm;
        if (pos[POS_WKC]==2) sc+=col*30*cnorm;
        if (pos[POS_BKC]==2) sc-=col*30*cnorm;
        return sc;
    }

    int evalPos(char *pos, int col) {
        int cx,cy;
        int f,fn,m;
        int sc=0;
        int figVals[]{100,290,300,500,900,10000};
        for (cy=0; cy<8; cy++) {
            for (cx=0; cx<8; cx++) {
                f=pos[FOF(cy,cx)];
                if (f==FIELD_EMPTY) continue;
                fn=f;
                if (fn<0) fn=(-fn);
                if (col*f > 0) m=1;
                else m = (-1);
                sc += figVals[fn-1] * m;
            }
        }
        sc += stratVal(pos,col);
        return sc;
    }

    wstring movSc(char *pos) {
        wstring strmov,sig;
        move2String(pos,strmov);
        int fc=((int)(*(int *)&pos[POS_SCORE]));
        if (pos[POS_LASTMV1]<0) fc=fc*(-1);
        if (fc<0) sig=L"-"; else sig=L"";
        int fc1=std::abs(fc/100);
        int fc2=std::abs(fc%100);
        wstring res=strmov+L" ("+sig+std::to_wstring(fc1)+L"."+std::to_wstring(fc2)+L")";
        return res;
    }

    int posScore(char *pos) {
        return *(int *)&pos[POS_SCORE];
    }


    int alphabeta(char *pos, int a, int b, int col, bool bMax, int depth, int curdepth, int *pcurnodes, time_t maxtime, wstring &movstack, char **pnpos, vector<char *>*pmlo) {
        int bsc,sc,bv,v;
        char *bp=nullptr;
        int nr,silnr;
        bool bTo=false;
        bool bSortShow=false;
        bool bSortShowO=false;
        vector<char *> ml{};
        wstring stack,stack0,beststack,ms;
        stack0=movstack;
        bool notCheck=true;
        if (col==CW && pos[POS_WK_CHK]) notCheck=false;
        if (col==CB && pos[POS_BK_CHK]) notCheck=false;
        if (depth<=0 && pos[POS_CAPT]==FIELD_EMPTY && notCheck)
            return posScore(pos)*(-1);

        if (pmlo!=nullptr) {
            ml=*pmlo; // A pre-sorted move-list is already given
        } else {
            ml=moveList(pos,col); // generate new move-list
        }

        if (ml.size()==0) {
            if (col==CW) {
                if (pos[POS_WK_CHK]) return -100000-depth*1000;
                else return 0.0;
            } else {
                if (pos[POS_BK_CHK]) return -100000-depth*1000;
                else return 0.0;
            }
        } else {
            bp=ml[0];
            bsc=*(int *)&ml[0][POS_SCORE];
        }
        if (bSortShow) {
            if (curdepth==0) {
                std::sort(ml.begin(), ml.end(), scoresort);
                wcout << L"[[";
                for (auto pi : ml) {
                    wstring ms=movSc(pi);
                    wcout << ms << "  ";
                }
                wcout << L"]]" << endl;
            }
        }

        time_t t;
        time(&t);

        if (bMax) {
            bsc=SC_MIN;
            bv=SC_MIN;
            nr=0;
            silnr=0;
            for (auto pi : ml) {
                ++nr;
                ++*pcurnodes;
                if (pi[POS_CAPT]==FIELD_EMPTY) ++silnr;
                stack=stack0;
                if (stack.size()>0) stack += L", ";
                ms=movSc(pi);
                stack +=  ms;
                movstack=stack;

                if (!cacheRead(pi,&v,depth-curdepth)) {
                    v=alphabeta(pi,a,b,CB,false,depth-1,curdepth+1, pcurnodes, maxtime,movstack,nullptr,nullptr);
                    if (v!=SC_TIMEOUT_INVALID)
                        cacheInsert(pi,v,depth-curdepth);
                    else bTo=true;
                }
                if (!bTo) {
                    sc=v*(-1);
                    v=v*(-1);
                    *(int *)&pi[POS_SCORE]=sc;
                    if (sc>bsc) {
                        bsc=sc;

                        bp=pi;
                        beststack=movstack;
                        if (curdepth<1) wcout << movstack <<  endl;
                    }
                    if (v>bv) bv=v;
                    if (bv>a) {
                        a=bv;
                    }
                    if (b <= a) { // beta cut-off
                        break;
                    }
                }
            }
        } else {
            bsc=SC_MIN;
            bv=SC_MAX;
            nr=0;
            silnr=0;
            for (auto pi : ml) {
                ++nr;
                ++*pcurnodes;
                if (pi[POS_CAPT]==FIELD_EMPTY) ++silnr;
                stack=stack0;
                if (stack.size()>0) stack += L", ";
                ms=movSc(pi);
                stack +=  ms;
                movstack=stack;

                if (!cacheRead(pi,&v,depth-curdepth+1)) {
                    v=alphabeta(pi,a,b,CW,true,depth-1,curdepth+1,pcurnodes,maxtime,movstack,nullptr,nullptr);
                    if (v!=SC_TIMEOUT_INVALID)
                        cacheInsert(pi,v,depth-curdepth+1);
                    else bTo=true;
                }
                if (!bTo) {
                    sc=v*(-1);
                    *(int *)&pi[POS_SCORE]=sc;
                    if (sc>bsc) {
                        bsc=sc;

                        bp=pi;
                        beststack=movstack;
                        if (curdepth<1) wcout << movstack << " Score: " << bsc << " @ " << curdepth <<  endl;
                    }
                    if (v<bv) bv=v;
                    if (bv<b) {
                        b=bv;
                    }
                    if (b <= a) { // alpha cut-off
                        break;
                    }
                }
            }
        }

        if (curdepth>0 && v==SC_TIMEOUT_INVALID) return SC_TIMEOUT_INVALID;

        if (pnpos!=nullptr) {
            if (bp!=nullptr) {
                *pnpos=(char *)malloc(POS_SIZE*sizeof(char));
                memcpy((void *)*pnpos,bp,POS_SIZE*sizeof(char));
            } else {
                *pnpos=nullptr;
                wcout << L"Game over!" << endl;
            }
        }

        if (curdepth==0) {
            std::sort(ml.begin(), ml.end(), scoresort);
            if (bSortShowO) {
                wcout << L">[[";
                for (auto pi : ml) {
                    wstring ms=movSc(pi);
                    wcout << ms << "  ";
                }
                wcout << L"]]" << endl;
            }
        }

        if (pmlo!=nullptr && curdepth==0) {
            *pmlo=ml;
        } else {
            for (auto pi : ml) {
                free(pi);
            }
        }
        movstack=beststack;

        return bsc;
    }

    int makeMove(char *pos,int col, std::map<string, int> &heuristic, int *pcurnodes, char **pnpos) {
        int sc=0;
        int depth=heuristic["depth"];
        time_t maxtime, t;
        int sco;
        time(&maxtime);
        maxtime += heuristic["maxtime"];
        vector<char *>ml;
        int depthi;
        ml=moveList(pos,col);
        for (depthi=4; depthi<=depth; depthi++) {
            wcout << L"[L:" << depthi << L"] ";
            int a=SC_MIN;
            int b=SC_MAX;
            wstring buf=L"";
            sc=alphabeta(pos,a,b,col,true,depthi,0,pcurnodes,maxtime,buf,pnpos,&ml);
            if (sc==SC_TIMEOUT_INVALID) {
                sc=sco;
                break;
            } else {
                sco=sc;
            }
            time(&t);
            if (t>=maxtime) break;
            //for (auto pi : ml) {
            //    *(int *)&pi[POS_SCORE] = *(int *)&pi[POS_SCORE] * (-1);
            //}
        }
        for (auto pi : ml) {
            free(pi);
        }
        return sc;
    }

};

int main(int argc, char *arv[]) {
    std::setlocale (LC_ALL, "");
    checkStatics();

    Chess c(30000000); //no. of cache entries
    //wcout << c.stratVal(c.pos,CW) << endl;
    //wcout << c.stratVal(c.pos,CB) << endl;

    int sc=0;
    char *npos;
    int col=1;
    int dt,nps;
    char *pos;
    int curnodes;
    wstring buf{L""};
    wstring move,inp;
    bool val;
    vector<char *> ml;
    std::map<string, int> heuristic;
    heuristic["depth"]=25;
    heuristic["maxtime"]=30;
    time_t starttime,endtime;
    pos=(char *)malloc(POS_SIZE*sizeof(char));
    memcpy((void *)pos,c.pos,POS_SIZE*sizeof(char));
    bool bOk=true;
    while (bOk) {
        buf=L"";
        time(&starttime);
        c.cachehit=0; c.cachemiss=0; c.cacheclash=0; c.cacheclashr=0;
        curnodes=0;
        sc=c.makeMove(pos, col, heuristic, &curnodes, &npos);
        time(&endtime);
        dt=endtime-starttime;
        if (dt>0) nps=curnodes/dt;
        else nps=0;
        free(pos);
        if (npos!=nullptr) {
            pos=npos;
            c.printPos(pos,-1);
            move=c.movSc(pos);
            wcout << move << L" Nodes: " << curnodes << L", Nodes/sec: " << nps << L", time:" << dt << L"s" << endl;
            wcout << L" cache-hits:"<<c.cachehit<<L", cachemiss:"<<c.cachemiss<<L", cacheconfl:"<<c.cacheclash<<endl;
            col=col*(-1);

            ml=c.moveList(pos,col);
            if (ml.size()==0) {
                wcout << "Game over!" << endl;
                break;
            }
            val=false;
            while (!val) {
                wcout << L"Your move: ";
                std::wcin >> inp;
                if (inp==L"q") {
                    bOk=false;
                    wcout << L"Quit." << endl;
                    break;
                }
                int p1=(inp[0]-'a')+1+(8-(inp[1]-'0'))*10+20;
                int p2=(inp[2]-'a')+1+(8-(inp[3]-'0'))*10+20;
                val=false;
                for (auto mi : ml) {
                    if (p1==mi[POS_LASTMV0] && p2==mi[POS_LASTMV1]) {
                        memcpy((void *)pos,mi,POS_SIZE*sizeof(char));
                        val=true;
                        break;
                    }
                }
                if (!val) wcout << L"Invalid move!" << endl;
            }
            for (auto mi : ml) free(mi);
            c.printPos(pos,-1);
            c.move2String(pos,move);
            wcout << move << endl;
            col=col*(-1);
        } else {
            wcout << L"END: Score: " << sc << endl;
            bOk=false;
        }
    }
    if (npos!=nullptr) free(npos);

    return 0;
}
