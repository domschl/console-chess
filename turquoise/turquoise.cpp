#include <engine.h>

/*
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
*/
void miniAutoGame(string fen="") {
    string start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int depth;
    
    if (fen=="") {
        fen=start_fen;
        depth=4;
    } else {
        depth = 8;
    }
    Board brd(fen);

    for (int i=0; i<50; i++) {
        wcout << endl;
        brd.printPos(&brd,-1);
        
        vector<Board::Move> ml(Board::searchBestMove(brd,depth,true));
        if (ml.size()==0) {
            wcout << L"Game over!" << endl;
            break;
        }
        brd=brd.rawApply(ml[0]); 
        wcout << stringenc(ml[0].toUciWithEval()) << endl;
    }
}

void miniGame() {
    string start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board brd(start_fen);
    brd.printPos(&brd,-1);
    vector<Board::Move> ml;
    string ascMove;
    while (true) {
        wcout << L"> ";
        std::cin >> ascMove;
        ml=brd.moveList();
        bool valid=false;
        for (Board::Move m : ml) {
            if (m.toUci() == ascMove) {
                valid = true;
                brd=brd.rawApply(m);
                brd.printPos(&brd, -1);
                break;
            }
        }
        if (!valid) {
            wcout << L"Invalid move!" << endl;
            continue;
        }
        vector<Board::Move> ml(Board::searchBestMove(brd,6));
        if (ml.size()==0) {
            wcout << L"Game over!" << endl;
            break;
        }
        brd=brd.rawApply(ml[0]); 
        brd.printPos(&brd, -1);
        wcout << stringenc(ml[0].toUciWithEval()) << endl;
    }
}


int main(int argc, char *argv[]) {
#ifndef __APPLE__
    std::setlocale(LC_ALL, "");
#else
    setlocale(LC_ALL, "");
    std::wcout.imbue(std::locale("en_US.UTF-8"));
#endif
    bool doAuto = false;
    string fen="";
    if (argc > 1) {
        if (!strcmp(argv[1], "-a"))
            doAuto = true;
    }
    if (argc > 2) {
        if (!strcmp(argv[1], "-f")) {
            doAuto=true;
            fen=argv[2];
        }
    }
    if (doAuto)
        miniAutoGame(fen);
    else
        miniGame();
    return 0;
}
