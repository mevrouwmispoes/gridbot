#include "pch.h"
#include "tictactoe.h"

#define FIRST_PLAYER 'X'
#define SECOND_PLAYER 'O'

#define LOWER_HEIGHT 25
#define ENTRY_LENGTH 20

static const double SQUARE_HALF_LENGTH = 1.37;

TicTacToe::TicTacToe(bool first_) {
	FIRST = first_;
	if (!FIRST) suggestion = -2;
}
void TicTacToe::initialise() {
	if(FIRST) Beta::handleAIMove(getSuggestion());
}
Board* TicTacToe::createBoard() { return new Board(3, 3); }

void TicTacToe::allow() { suggestion = -1; }
void TicTacToe::calculate(Board *board) {
	if (board == NULL) return; //If the pointer isn't set yet, DONT

	//Where we figure out our next move
	if (suggestion != -1) return; //If the suggestion is -1 we've already calculated our move
	Logger::info("[TICTACTOE] Starting calculation...");
	if (!isTie(board) && !isGameOver(board)) {
		suggestion = calculateBestMove(board, true, true);
	} else {
		Logger::info("[TICTACTOE] Game is a tie or has ended.");
		Beta::shutdown();
		return;
	}
	if (suggestion < 0 || suggestion > 8) {
		Logger::info("[TICTACTOE] Invalid suggested move...");
		Beta::shutdown();
		return;
	}
	char buf[256];
	buf[0] = 0;
	strcat(buf, "[TICTACTOE] Determined next move to be index '");
	strcat(buf, std::to_string(suggestion).c_str());
	strcat(buf, "'.");
	Logger::info(buf);
}

// Our recursive method to find the best outcome.
int TicTacToe::calculateBestMove(Board *board, bool surface, bool ai) {
	std::vector<Entry> cdf;
	//Firstly if the state of the game is tied or a win were at the end of our tree and we return back up.
	if (isTie(board)) return 0;
	else if (isGameOver(board)) return -1;
	else {
		//Try all possible moves and grade them.
		for (int s = 0; s < board->getMaxIndex(); s++) {
			if (board->atIndex(s) != Figure::EMPTY) continue;
			board->set(s, ai ? Figure::AI : Figure::HUMAN);
			cdf.push_back(Entry(s, (-1 * calculateBestMove(board, false, !ai)))); //We switch the value of the other player, min->max, max->min
			board->set(s, Figure::EMPTY);
		}

		if (surface) {
			Entry max = Entry(-2, -2);
			std::vector<Entry> maxs = std::vector<Entry>();
			for (unsigned int i = cdf.size()-1; i >= 0; --i) {
				if (cdf[i].value > max.value) {
					max = cdf[i];
					maxs.clear();
				}
				if (cdf[i].value == max.value) maxs.push_back(cdf[i]);
			}
			return maxs.at(std::rand() % maxs.size()).key; //Pick random option which is still optimal.
		} else {
			//We're getting the best entry, the move which results in the best situations
			int value = 0;
			for (unsigned int i = 0; i < cdf.size(); i++) {
				value += cdf[i].value;
				if (cdf[i].value > 0) value += 1; //If this is a positive result, we will increase it slightly so risk-free
			}
			return value;
		}
	}
}

int TicTacToe::getSuggestion() {
	if (suggestion == -1) {
		Logger::info("[TICTACTOE] Waiting on calculation for next move...");
	}
	while (suggestion == -1) {}
	return suggestion;
}

bool TicTacToe::isTie(Board *board) {
	//We determine if it's a tie by seeing if there's no moves to be made. (no winner, no moves, a tie)
	for (int i = 0; i < board->getMaxIndex(); i++) {
		if (board->atIndex(i) == Figure::EMPTY) return false;
	}
	return true;
}
bool TicTacToe::isGameOver(Board *board) {
	//We determine the winner by absolute mad methods, we're going to assume two wins can't happen at the same time.
	Figure ret = static_cast<Figure>(0);
	//X-ways
	testLine(board, &ret, 0, 3, 6);
	testLine(board, &ret, 1, 4, 7);
	testLine(board, &ret, 2, 5, 8);

	//Y-ways
	testLine(board, &ret, 0, 1, 2);
	testLine(board, &ret, 3, 4, 5);
	testLine(board, &ret, 6, 7, 8);

	//Diagonal
	testLine(board, &ret, 0, 4, 8);
	testLine(board, &ret, 2, 4, 6);
	return (ret == Figure::HUMAN || ret == Figure::AI);
}

void TicTacToe::testLine(Board *board, Figure *ret, int i, int j, int k) {
	Figure r = board->atIndex(i);
	if (r != board->atIndex(j)) r = static_cast<Figure>(0);
	if (r != board->atIndex(k)) r = static_cast<Figure>(0);
	if (r != 0) *ret = r;
}

//Executes the suggestion by sending cm-based commands to the motors.
//It is assumed we are hovering over the bottom left corner of the bottom left square, x0, y0 and at z100%, magnet off
//It is also assumed that there is a disc or object for it to move at x-1, y0
void TicTacToe::execute(Motor *x, Motor *y, Motor *z) {	
	x->queue(ENTRY_LENGTH);

	//Move to pickup stone.
	x->queue(-3*SQUARE_HALF_LENGTH);
	z->queue(-LOWER_HEIGHT);
	//Enable magnet
	GPIO::set(10, true);
	z->queue(LOWER_HEIGHT);

	//Move to square
	x->queue(SQUARE_HALF_LENGTH*4); //We're at x0.5, y0.5 or above square 0,0
	// The tic tac toe board is on x2 y2
	
	int moveX = suggestion / 3, moveZ = suggestion % 3;
	x->queue(SQUARE_HALF_LENGTH*(2*(moveZ+2)));
	y->queue(SQUARE_HALF_LENGTH*(2*(moveX+2)));

	//Place here
	z->queue(-LOWER_HEIGHT);
	GPIO::set(10, false);
	z->queue(LOWER_HEIGHT);

	//Move back
	x->queue(-SQUARE_HALF_LENGTH*(2 * (moveZ + 2)));
	y->queue(-SQUARE_HALF_LENGTH*(2 * (moveX + 2)));

	//Move back to base?
	x->queue(-SQUARE_HALF_LENGTH);
	x->queue(-ENTRY_LENGTH);
}