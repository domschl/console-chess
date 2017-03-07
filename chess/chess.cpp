#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
/*
 * TODO that is it
 */
using std::wcout; using std::endl; using std::string; using std::vector;
using std::wstring;

void cc(int col,int bk) {
    unsigned char esc=27;
    int c=col+30;
    int b=bk+40;
    char s[256];
    sprintf(s,"%c[%d;%dm",esc,c,b);
    wcout << s;
}
//
void clr() {
    unsigned char esc=27;
    char s[256];
    sprintf(s,"%c[%dJ",esc,2);
    wcout << s;
}
void got(int cy, int cx) {
    unsigned char esc=27;
    char s[256];
    sprintf(s,"%c[%d;%dH",esc,cy+1,cx+1);
    wcout << s;
}

#define POS_SIZE 150

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

#define POS_SCORE   (POS_CTRL_OFFSET+12)  // Score of position as float (uhhh)

#define FOF(y,x) ((y+2)*10+x+1)
#define COL(of) (of%10-1)
#define ROW(of) (of/10-2)

#define SC_MIN (-9999999)
#define SC_MAX ( 9999999)

//class Chess;
//bool Chess::isThreatened(char *pos,int cy, int cx, int col);
bool scoresort(char *p1, char *p2) {
    float sc1=*(int *)&p1[POS_SCORE];
    float sc2=*(int *)&p2[POS_SCORE];
    return sc1>sc2;
}

class Chess {
public:
    char pos[POS_SIZE];
    Chess() {
        int i;
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
    }


    vector<char *> moveList(char *pos, int col=CW) {
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
        char figAttack[]{4,20,4,3,3,1,2,2,2,2,1,0,0};
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
                        if (pos[of0]==FIELD_WQ * col) sval[of] += 2; // atc+=vf;
                        if (pos[of0]==FIELD_BR * col) sval[of] -= 3; // atc-=vf;
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
        //float anorm=4.0;
        mov=0;
        int ac;
        for (int i=0; i<POS_SIZE; i++) {
            mov += sval[i];
            if (sval[i]) {
                //atc+=sval[i]*anorm*col
                if (sval[i]<0) ac=-1; else ac=1;
                vf=figAttack[pos[i]*ac+6];
                //if (sval[i]<0) atc -= vf*norm;
                //else atc += vf*norm;
                atc += vf*norm*sval[i];
            }
        }
        float movnorm=4;
        atc +=mov*movnorm;

        int znorm=3;
        atc +=(sval[FOF(3,3)] + sval[FOF(3,4)] + sval[FOF(4,3)] + sval[FOF(4,4)]) * znorm;
        atc +=(sval[FOF(3,2)] + sval[FOF(3,5)] + sval[FOF(4,2)] + sval[FOF(4,5)]) * znorm;

        int ofx, ofy;
        ofy=ROW(pos[POS_WK]);
        ofx=COL(pos[POS_WK]);
        int ks=0;
        for (cy=-2; cy<3; cy++) {
            for (cy=-2; cy<3; cy++) {
                if (std::abs(cx)>std::abs(cy)) d=std::abs(cx);
                else d=std::abs(cy);
                d=3-d;
                ks += (sval[FOF(ofy+cy,ofx+cx)]*d);
            }
        }

        ofy=ROW(pos[POS_BK]);
        ofx=COL(pos[POS_BK]);
        //ks=0.0;
        int knorm=1;
        int kdiv=3;
        for (cy=-2; cy<3; cy++) {
            for (cy=-2; cy<3; cy++) {
                if (std::abs(cx)>std::abs(cy)) d=std::abs(cx);
                else d=std::abs(cy);
                d=3-d;
                ks += (sval[FOF(ofy+cy,ofx+cx)]*d)*knorm;
            }
        }
        ks /= kdiv;

        sc=(knorm+atc)/3;

        int cnorm=1;
        if (pos[POS_WKC]==0) sc-=col*20*cnorm;
        if (pos[POS_WQC]==0) sc-=col*20*cnorm;
        if (pos[POS_BKC]==0) sc+=col*30*cnorm;
        if (pos[POS_BQC]==0) sc+=col*20*cnorm;
        if (pos[POS_WKC]==2) sc+=col*40*cnorm;
        if (pos[POS_BKC]==2) sc-=col*40*cnorm;
        return sc;
    }

    int evalPos(char *pos, int col) {
        int cx,cy;
        int f,fn,m;
        float sc=0.0;
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

    int miniMax(char *pos, int col, int depth, int curdepth, wstring &movstack, char **pnpos) {
        vector<char *>ml;
        int sc,bs;
        char *bp;
        ml=moveList(pos,col);
        bs=SC_MIN;
        bp=nullptr;
        wstring stack,stack0,beststack;
        stack0=movstack;
        int nr=0;
        if (curdepth==0) {
            for (auto pi : ml) {
                wstring ms=movSc(pi);
                wcout << ms << "  ";
            }
            wcout << endl;
        }
        wstring ms;
        for (auto pi : ml) {
            stack=stack0;
            if (bp==nullptr) bp=pi;
            if (stack.size()>0) stack += L", ";
            ms=movSc(pi);
            stack +=  ms;
            movstack=stack;
            ++nr;
            if (depth>0 || (curdepth<12 && pi[POS_CAPT]!=FIELD_EMPTY)) {
                sc=miniMax(pi,col*(-1),depth-1, curdepth+1, movstack, nullptr);
                sc = sc  * (-1);
                *(int *)&pi[POS_SCORE]=sc;
            } else {
                sc = *(int *)&pi[POS_SCORE];
            }
            if (sc>bs) {
                bs=sc;
                bp=pi;
                beststack=movstack;
                //for (int i=0; i<curdepth; i++ ) wcout << L" ";
                if (curdepth<1) wcout << movstack << " Score: " << bs << " @ " << curdepth <<  endl;
            }
            //if (curdepth>1 && nr>4) break;
            if (curdepth>1 && nr>4) break;
            if (curdepth>2 && nr>3) break;
            if (curdepth>3 && nr>2) break;
        }
        if (pnpos!=nullptr) {
            *pnpos=(char *)malloc(POS_SIZE*sizeof(char));
            memcpy((void *)*pnpos,bp,POS_SIZE*sizeof(char));
        }
        if (curdepth==0) {
            std::sort(ml.begin(), ml.end(), scoresort);
            for (auto pi : ml) {
                wstring ms=movSc(pi);
                wcout << ms << "  ";
            }
            wcout << endl;
        }
        for (auto pi : ml) {
            delete pi;
        }
        movstack=beststack;
        return bs;
    }
    int posScore(char *pos) {
        return *(int *)&pos[POS_SCORE];
    }
    /*
    01 function alphabeta(node, depth, α, β, maximizingPlayer)
    02      if depth = 0 or node is a terminal node
    03          return the heuristic value of node
    04      if maximizingPlayer
    05          v := -∞
    06          for each child of node
    07              v := max(v, alphabeta(child, depth – 1, α, β, FALSE))
    08              α := max(α, v)
    09              if β ≤ α
    10                  break (* β cut-off *)
    11          return v
    12      else
    13          v := ∞
    14          for each child of node
    15              v := min(v, alphabeta(child, depth – 1, α, β, TRUE))
    16              β := min(β, v)
    17              if β ≤ α
    18                  break (* α cut-off *)
    19          return v

    (* Initial call *)
    alphabeta(origin, depth, -∞, +∞, TRUE)
    */
    int TO_INV_VAL=-3333333;

    int alphabeta(char *pos, int a, int b, int col, bool bMax, int depth, int curdepth, time_t maxtime, wstring &movstack, char **pnpos, vector<char *>*pmlo) {
        int bsc,sc,bv,v;
        char *bp=nullptr;
        int nr,silnr;
        bool bSortShow=true;
        bool bSortShowO=false;
        vector<char *> ml{};
        wstring stack,stack0,beststack,ms;
        stack0=movstack;
        if (depth<=0 && pos[POS_CAPT]==FIELD_EMPTY)
            return posScore(pos)*(-1);


        /*
        if (depth>6) {
            float aopt=SC_MIN;
            float bopt=SC_MAX;
            bool bmax;
            wstring bufopt=L"";
            if ((curdepth+0)%2) bmax=false;
            else bmax=true;
            alphabeta(pos,aopt,bopt,col,bmax,4,0,maxtime,bufopt,nullptr,&ml);
        } else {
            ml=moveList(pos,col);
        } */
        if (pmlo!=nullptr) {
            ml=*pmlo;
        } else {
            ml=moveList(pos,col);
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
        if (true) {
            if (bMax) {
                bsc=SC_MIN;
                bv=SC_MIN;
                nr=0;
                silnr=0;
                for (auto pi : ml) {
                    ++nr;
                    if (pi[POS_CAPT]==FIELD_EMPTY) ++silnr;
                    stack=stack0;
                    if (stack.size()>0) stack += L", ";
                    ms=movSc(pi);
                    stack +=  ms;
                    movstack=stack;

                    v=alphabeta(pi,a,b,CB,false,depth-1,curdepth+1,maxtime,movstack,nullptr,nullptr);
                    if (v==TO_INV_VAL) break;
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

                    //if (curdepth>2 && silnr>4) break;
                    //if (curdepth>3 && silnr>3) break;
                    //if (curdepth>4 && silnr>2) break;
                }
            } else {
                bsc=SC_MIN;
                bv=SC_MAX;
                nr=0;
                silnr=0;
                for (auto pi : ml) {
                    ++nr;
                    if (pi[POS_CAPT]==FIELD_EMPTY) ++silnr;
                    stack=stack0;
                    if (stack.size()>0) stack += L", ";
                    ms=movSc(pi);
                    stack +=  ms;
                    movstack=stack;

                    v=alphabeta(pi,a,b,CW,true,depth-1,curdepth+1,maxtime,movstack,nullptr,nullptr);
                    if (v==TO_INV_VAL) break;
                    sc=v*(-1);
                    //v=v*(-1);
                    //v=v*col*(-1);
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

                    //if (curdepth>2 && silnr>4) break;
                    //if (curdepth>3 && silnr>3) break;
                    //if (curdepth>4 && silnr>2) break;
                }
            }

            if (curdepth>0 && v==TO_INV_VAL) return TO_INV_VAL;
            if (curdepth==0 && v==TO_INV_VAL) {
                return TO_INV_VAL;
            }
        }

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
                delete pi;
            }
        }
        movstack=beststack;

        return bsc;
    }

    int makeMove(char *pos,int col, std::map<string, int> &heuristic, char **pnpos) {
        int sc=0;
        int mode=1; // 0: minimax, 1: alphabeta pruning
        int depth=heuristic["depth"];
        time_t maxtime, t;
        int sco;
        time(&maxtime);
        maxtime += heuristic["maxtime"];
        if (mode==0) {
            wstring buf=L"";
            sc=miniMax(pos,col,depth-1,0,buf,pnpos);
        } else {
            vector<char *>ml;
            int depthi;
            ml=moveList(pos,col);
            for (depthi=4; depthi<=depth; depthi+=1) {
                wcout << L"[L:" << depthi << L"] ";
                int a=SC_MIN;
                int b=SC_MAX;
                wstring buf=L"";
                sc=alphabeta(pos,a,b,col,true,depthi,0,maxtime,buf,pnpos,&ml);
                if (sc==TO_INV_VAL) {
                    sc=sco;
                    break;
                } else {
                    sco=sc;
                }
                time(&t);
                if (t>=maxtime) break;
            }
        }
        return sc;
    }

};

int main(int argc, char *arv[]) {
    std::setlocale (LC_ALL, "");
    Chess c; // = new Chess();
    //wcout << c.stratVal(c.pos,CW) << endl;
    //wcout << c.stratVal(c.pos,CB) << endl;

    int sc=0;
    char *npos;
    int col=1;
    char *pos;
    wstring buf{L""};
    wstring move,inp;
    bool val;
    vector<char *> ml;
    std::map<string, int> heuristic;
    heuristic["depth"]=25;
    heuristic["maxtime"]=60;
    pos=(char *)malloc(POS_SIZE*sizeof(char));
    memcpy((void *)pos,c.pos,POS_SIZE*sizeof(char));
    bool bOk=true;
    while (bOk) {
        buf=L"";
        sc=c.makeMove(pos, col, heuristic, &npos);
        delete pos;
        if (npos!=nullptr) {
            pos=npos;
            c.printPos(pos,-1);
            move=c.movSc(pos);
            wcout << move << endl;
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
            for (auto mi : ml) delete mi;
            c.printPos(pos,-1);
            c.move2String(pos,move);
            wcout << move << endl;
            col=col*(-1);
        } else {
            wcout << L"END: Score: " << sc << endl;
            bOk=false;
        }
    }
    if (npos!=nullptr) delete npos;

    return 0;
}
