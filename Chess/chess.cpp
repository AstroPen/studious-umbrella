
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <climits>
#include "chess.h"

#define KNRM  "\x1B[0m"
#define KBLK  "\x1B[30m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define KBGBLK  "\x1B[40m"
#define KBGRED  "\x1B[41m"
#define KBGGRN  "\x1B[42m"
#define KBGYEL  "\x1B[43m"
#define KBGBLU  "\x1B[44m"
#define KBGMAG  "\x1B[45m"
#define KBGCYN  "\x1B[46m"
#define KBGWHT  "\x1B[47m"

#define DEBUG_FILE 1

FILE *output_file;
bool dev_mode = false;
int com_player = -1;
int global_player = 1;
int traversal_count = 0;

// TODO factor existing code:
inline int get_other_player(int player) {
  assert(player);
  assert(player < 3);

  return (!(player - 1)) + 1;
}

inline int piece_color(PieceType p) {
  assert((p & PMASK) != NON_VALID);
  if (p < 0) return 2;
  else if (p > 0) return 1;
  else return 0;
}


inline PieceType get_piece(ChessBoard *cb, int x, int y) {
  return cb->board[y][x];
}

inline PieceType get_piece(ChessBoard *cb, Position p) {
  return cb->board[p.y][p.x];
}

inline PieceType set_piece(ChessBoard *cb, int x, int y, PieceType p) {
  return cb->board[y][x] = p;
}

inline PieceType set_piece(ChessBoard *cb, Position pos, PieceType p) {
  return cb->board[pos.y][pos.x] = p;
}

inline bool equal(Position p1, Position p2) {
  return p1.x == p2.x && p1.y == p2.y;
}

void print_board(PieceType board[8][8], FILE *file) {

  fprintf(file, KGRN "   A  B  C  D  E  F  G  H  \n");
  bool white = true;
  for (int y = 7; y >= 0; y--) {
    fprintf(file, KGRN "%d  ", y + 1);
    for (int x = 0; x < 8; x++) {

      // Used to alternate background colors:
      if (white) fprintf(file, KBGYEL);
      else       fprintf(file, KNRM);

      // get the color and mask the piece:
      PieceType p = board[y][x];
      bool black_piece = p & PBLACK;
      p &= PMASK;

      if (!black_piece) {

        // white:
        switch (p) {
          case EMPTY :
            fprintf(file, "   "); break;
          case PAWN :
            fprintf(file, KYEL UNI_WP " "); break;
          case ROOK :
            fprintf(file, KYEL UNI_WR " "); break;
          case BISHOP :
            fprintf(file, KYEL UNI_WB " "); break;
          case KNIGHT :
            fprintf(file, KYEL UNI_WN " "); break;
          case KING :
            fprintf(file, KYEL UNI_WK " "); break;
          case QUEEN :
            fprintf(file, KYEL UNI_WQ " "); break;
          case NON_VALID :
            fprintf(file, KBGRED " - "); break;
        }

      } else {

        // black:
        switch (p) {
          case EMPTY :
            assert(p != EMPTY || !black_piece); break;
          case PAWN :
            fprintf(file, KBLU UNI_BP " "); break;
          case ROOK :
            fprintf(file, KBLU UNI_BR " "); break;
          case BISHOP :
            fprintf(file, KBLU UNI_BB " "); break;
          case KNIGHT :
            fprintf(file, KBLU UNI_BN " "); break;
          case KING :
            fprintf(file, KBLU UNI_BK " "); break;
          case QUEEN :
            fprintf(file, KBLU UNI_BQ " "); break;
          case NON_VALID :
            assert(p != NON_VALID || !black_piece); break;
        }
      }

      // Alternate backing color:
      white = !white;
    }
    fprintf(file, KNRM);
    fprintf(file, KGRN " %d ", y + 1);
    fprintf(file, KNRM "\n");
    white = !white;
  }
  fprintf(file, KGRN "   A  B  C  D  E  F  G  H  \n" KNRM);
}

inline void print_board(PieceType board[8][8]) {
  print_board(board, stdout);
}

inline bool check_in_range_low(char letter) {
  return letter >= 'a' && letter <= 'h';
}

// TODO allow caps?
inline bool check_in_range_cap(char letter) {
  return letter >= 'A' && letter <= 'H';
}

inline bool check_in_range_num(char letter) {
  return letter >= '1' && letter <= '8';
}

inline bool legal_target(PieceType p, PieceType target) {
  int color = piece_color(p);
  int target_color = piece_color(target);
  assert(color); // Do not call this with a EMPTY piece
  return color != target_color;
}

bool collides_diagonal(PieceType board[8][8], Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;

  while (x1 != x2 && y1 != y2) {
    if (x1 > x2) x1--;
    else x1++;
    if (y1 > y2) y1--;
    else y1++;
    if (board[y1][x1] != EMPTY) {
      return x1 != x2;
    }
  }

  return false;
}

bool collides_normal(PieceType board[8][8], Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;

  while (x1 != x2) {
    if (x1 > x2) x1--;
    else x1++;
    if (board[y1][x1] != EMPTY) {
      return x1 != x2;
    }
  }
  while (y1 != y2) {
    if (y1 > y2) y1--;
    else y1++;
    if (board[y1][x1] != EMPTY) {
      return y1 != y2;
    }
  }

  return false;
}

inline bool rook_move(Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;
  return (x1 == x2 || y1 == y2);
}

inline bool bishop_move(Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;
  return (x1 - x2) == (y1 - y2) || (x1 - x2) == -(y1 - y2);
}

inline bool knight_move(Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;
  if ((x1 - x2) == 1 || (x1 - x2) == -1) {
    return (y1 - y2) == 2 || (y1 - y2) == -2;
  } else if ((x1 - x2) == 2 || (x1 - x2) == -2) {
    return (y1 - y2) == 1 || (y1 - y2) == -1;
  } else return false;
}

inline bool king_move(ChessBoard *cb, Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;

  bool result = (x1 - x2) <= 1 && (x1 - x2) >= -1;
  result = result && (y1 - y2) <= 1 && (y1 - y2) >= -1;

  if (!result) {
    // castling

    PieceType king_p = get_piece(cb, p1);
    bool has_moved = king_p & PMOVED;
    
    if (has_moved) return false;

    int color = piece_color(king_p);
    assert(color);

    if (get_piece(cb, p2)) return false;

    // TODO factor this better
    // TODO test this for moving across threatened space
    if (color == 1) {

      // White king
      if (cb->check_status == CHECK_ON_1)  return false;
      if (p2.y != 0)                       return false;

      if (p2.x == 2) {

        // Castling with Rook at A1
        PieceType rook = get_piece(cb, 0, 0);
        if (rook & PMOVED) return false;
        if ((rook & PMASK) != ROOK) return false;

        Position in_between = { 3, 0 };
        CheckStatus status = test_for_checks(cb, p1, in_between);
        if (status == CHECK_ON_1)  return false;

      } else if (p2.x == 6) {

        // Castling with Rook at H1
        PieceType rook = get_piece(cb, 7, 0);
        if (rook & PMOVED) return false;
        if ((rook & PMASK) != ROOK) return false;

        Position in_between = { 5, 0 };
        CheckStatus status = test_for_checks(cb, p1, in_between);
        if (status == CHECK_ON_1)  return false;

      } else {
        return false;
      }

    } else {

      // Black king
      if (cb->check_status == CHECK_ON_2)  return false;
      if (p2.y != 7)                       return false;

      if (p2.x == 2) {

        // Castling with Rook at A8
        PieceType rook = get_piece(cb, 0, 7);
        if (rook & PMOVED) return false;
        if ((rook & PMASK) != ROOK) return false;

        Position in_between = { 3, 7 };
        CheckStatus status = test_for_checks(cb, p1, in_between);
        if (status == CHECK_ON_2)  return false;

      } else if (p2.x == 6) {

        // Castling with Rook at H8
        PieceType rook = get_piece(cb, 7, 7);
        if (rook & PMOVED) return false;
        if ((rook & PMASK) != ROOK) return false;

        Position in_between = { 5, 7 };
        CheckStatus status = test_for_checks(cb, p1, in_between);
        if (status == CHECK_ON_2)  return false;

      } else {
        return false;
      }
    }

    if (collides_normal(cb->board, p1, p2)) return false;

    return true;
    
  }
  return result;
}

bool legal_pawn_target(ChessBoard *cb, Position p1, Position p2) {
  // Promotion
  // - prompt user for input, handle with 1 letter command
  // - Not sure where to do this...
  // - keep data structure that records lost pieces...
  // En Passant
  // - Add position to cb, set whenever pawn moves two spaces
  // - Implement capture in apply_move
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;

  PieceType p = get_piece(cb, x1, y1);
  PieceType target = get_piece(cb, x2, y2);

  int p_color = piece_color(p);
  int t_color = piece_color(target);

  p &= PMASK;
  target &= PMASK;

  assert(p == PAWN);
  bool result;

  // TODO use piece_color here to make things less confusing
  if (p_color == 1) {
    // legal target, diagnal move
    result = t_color == 2 || (p2.y == 5 && equal(p2, cb->en_passant));
    result = result && x1 != x2 && x2 - x1 < 2 && x2 - x1 > -2;
    // small move
    result = result && y2-y1 == 1;
    // or moving straight to empty space
    result = result || (target == EMPTY && x1 == x2);
    return result;

  } else { // PAWN_2
    // legal target, diagnal move
    result = t_color == 1 || (p2.y == 2 && equal(p2, cb->en_passant));
    result = result && x1 != x2 && x2 - x1 < 2 && x2 - x1 > -2;
    // small move
    result = result && y2-y1 == -1;
    // or moving straight to empty space
    result = result || (target == EMPTY && x1 == x2);
    return result;
  }
}

// Asks the given player to choose a promotion.
// If the player is a human, this is done through
// the command line. If they are a computer, it 
// returns QUEEN.
PieceType get_promotion_input(int player) {

  assert(player);
  // TODO let the com actually choose:
  // TODO handle white com player
  if (com_player == player) return QUEEN | PBLACK;

  printf("Choose promotion\n");
  PieceType result = EMPTY;
  while (!result) {
    int choice = getchar();
    switch (choice) {
      case 'b' : 
        result = BISHOP; break;
      case 'k' :
        printf("Invalid choice\n"); break;
      case 'n' : 
        result = KNIGHT; break;
      case 'p' : 
        printf("Invalid choice\n"); break;
      case 'q' : 
        result = QUEEN; break;
      case 'r' : 
        result = ROOK | PMOVED; break;
      case '\n' : 
        break;
      default  :
        printf("Invalid choice\n");
    }
  }

  if (player == 2) {
    result = result | PBLACK;
  }
  return result;
}

// Modifies cb, moving the piece at p1 to p2. It does NOT
// check to see that the move is legal. It DOES apply en
// passant.
void apply_move(ChessBoard *cb, Position p1, Position p2) {
  cb->static_moves++;
  if (get_piece(cb, p2) != EMPTY) cb->static_moves = 0;

  PieceType pt = get_piece(cb, p1);
  int p_color = piece_color(pt);
  PieceType type = pt & PMASK;

  assert(pt != EMPTY);
  assert(pt != NON_VALID);

  // TODO we might not want to set PMOVED for all pieces
  set_piece(cb, p2, pt | PMOVED); 
  set_piece(cb, p1, EMPTY); 

  if (type == KING) {

    if (p_color == 1) {

      // white king
      cb->king_1_pos = p2;

      if ((p2.x - p1.x) == 2) {
        // Castling with h1 rook

        set_piece(cb, 7, 0, EMPTY);
        set_piece(cb, 5, 0, ROOK | PMOVED);

      } else if ((p2.x - p1.x) == -2) {
        // Castling with a1 rook

        set_piece(cb, 0, 0, EMPTY);
        set_piece(cb, 3, 0, ROOK | PMOVED);
      }

    } else {

      // black king
      cb->king_2_pos = p2;

      if ((p2.x - p1.x) == 2) {
        // Castling with h8 rook

        set_piece(cb, 7, 7, EMPTY);
        set_piece(cb, 5, 7, ROOK | PBLACK | PMOVED);
        
      } else if ((p2.x - p1.x) == -2) {
        // Castling with a8 rook

        set_piece(cb, 0, 7, EMPTY);
        set_piece(cb, 3, 7, ROOK | PBLACK | PMOVED);
      }
    }
  }

  // TODO thoroughly test en passant
  if (type == PAWN) {

    cb->static_moves = 0;

    if (p_color == 1) {
      // white pawn

      if ((p2.y - p1.y) == 2) {
        // first move, 2 spaces
        cb->en_passant = { p2.x, 2 };

      } else if (equal(p2, cb->en_passant)) {
        // move is en_passant, need to capture

        set_piece(cb, p2.x, 4, EMPTY);

      } else if (p2.y == 7) {
        // Pawn promotion happens only in user mode
        // PieceType promo = get_promotion_input(1);
        // set_piece(cb, p2, promo);
      }

    } else {
      // black pawn

      if ((p2.y - p1.y) == -2) {
        // first move, 2 spaces
        cb->en_passant = { p2.x, 5 };

      } else if (equal(p2, cb->en_passant)) {
        // move is en_passant, need to capture

        set_piece(cb, p2.x, 3, EMPTY);

      } else if (p2.y == 0) {
        // Pawn promotion happens only in user mode
        // PieceType promo = get_promotion_input(2);
        // set_piece(cb, p2, promo);
      }
    }
  }
}

// Pushes a move to the stack
inline void push_move(ChessStack *stack, Position p1, Position p2) {

  // Update ChessStack:
  int size = stack->size;
  assert(size);
  ChessFrame *current = top_frame(stack);
  ChessFrame *next    = alloc_frame(stack);
  next->game = current->game;
  next->move = { p1, p2 };
  ChessBoard *cb = &next->game;

  PieceType pt = get_piece(cb, p1);
  assert(pt != EMPTY);
  assert(pt != NON_VALID);

  apply_move(cb, p1, p2);

  int p_color = piece_color(pt);
  assert(p_color);
  PieceType type = pt & PMASK;

  if (type == PAWN) {

    if ((p_color == 1 && p2.y == 7) ||
        (p_color == 2 && p2.y == 0)) {

      // Pawn promotion
      PieceType promo = get_promotion_input(p_color);
      set_piece(cb, p2, promo);
      // TODO push this onto the stack as another board state
    }
  }

  update_count(stack);

  // TODO move/remove this
  printf("Static moves : %d\n", cb->static_moves);
}

// Returns true if p1 to p2 is a legal move on cb
// Does NOT consider check or checkmate
bool is_legal_move(ChessBoard *cb, Position p1, Position p2) {
  int x1 = p1.x;
  int y1 = p1.y;
  int x2 = p2.x;
  int y2 = p2.y;

  auto board = cb->board;

  if (x1 == x2 && y1 == y2) return false;


  PieceType p = get_piece(cb, p1);
  if (p == EMPTY) return false;
  PieceType type = p & PMASK;
  PieceType target = get_piece(cb, p2);


  if (!legal_target(p, target)) return false;

  bool all_moves, normal_move, long_move, collides;

  switch (type) {
    case EMPTY :
      return false;

    case PAWN :
      // TODO factor this more
      if (piece_color(p) == 1) {

        // white pawn
        all_moves = y2 > y1 && (y2 - y1) <= 2;
        normal_move = y2 - y1 == 1;
        long_move = y1 == 1 && get_piece(cb, x1, 2) == EMPTY;
        return all_moves && (normal_move || long_move) 
                         && legal_pawn_target(cb, p1, p2);

      } else {

        // black pawn
        all_moves = y2 < y1 && (y2 - y1) >= -2;
        normal_move = y2 - y1 == -1;
        long_move = y1 == 6 && get_piece(cb, x1, 5) == EMPTY;
        return all_moves && (normal_move || long_move) 
                         && legal_pawn_target(cb, p1, p2);
      }

    case ROOK :
      all_moves = rook_move(p1, p2);
      collides = collides_normal(board, p1, p2);
      return all_moves && !collides;

    case BISHOP :
      all_moves = bishop_move(p1, p2);
      collides = collides_diagonal(board, p1, p2);
      return all_moves && !collides;

    case KNIGHT :
      return knight_move(p1, p2);

    case KING :
      return king_move(cb, p1, p2);

    case QUEEN :
      if (bishop_move(p1, p2)) {
        return !collides_diagonal(board, p1, p2);
      } else if (rook_move(p1, p2)) {
        return !collides_normal(board, p1, p2);
      } else return false;

    case NON_VALID :
      printf("Error : NON_VALID piece selected\n");
      return false;

    default :
      printf("Error : Unknown piecetype.\n");
      assert(false);
  }

  return false;
}

#if 0
void update_threat_grid(ChessBoard *cb) {

  for (int y1 = 0; y1 < 8; y1++) {
    for (int x1 = 0; x1 < 8; x1++) {

      Position p1 = { x1, y1 };

      for (int y2 = 0; y2 < 8; y2++) {
        for (int x2 = 0; x2 < 8; x2++) {

          Position p2 = { x2, y2 };
          if (is_legal_move(cb, p1, p2)) {
            uint8_t mask = 1 << (7 - x2);
            cb->threat_grid[y2] |= mask;
          }

        }
      }
    }
  }
}
#endif

// TODO consider renaming all this garbage...
// Tests for checks, return the correct status
inline CheckStatus test_for_checks(ChessBoard *cb, int acting_player) {
  // TODO check only necessary squres relative to king
  CheckStatus result = NO_CHECK;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {

      Position p = { x, y };
      int color = piece_color(get_piece(cb, x, y));

      if (color == 2 && is_legal_move(cb, p, cb->king_1_pos)) {
        if (acting_player == 1) return CHECK_ON_1;
        else result = CHECK_ON_1;
      }

      if (color == 1 && is_legal_move(cb, p, cb->king_2_pos)) {
        if (acting_player == 2) return CHECK_ON_2;
        else result = CHECK_ON_2;
      }
    }
  }

  return result;
}

inline void print_move(Position p1, Position p2, FILE *file) {
  fprintf(file, "%c%d to %c%d\n", p1.x + 'a', p1.y + 1, p2.x + 'a', p2.y + 1);
}

inline void print_move(Position p1, Position p2) {
  print_move(p1, p2, stdout);
}

bool is_in_checkmate(ChessBoard *cb, int player) {
  assert(player == 1 || player == 2);
  // if player is in check, looks for checkmate
  // otherwise, looks for stalemate

  // iterate through every move by piece with player color
  int other_player = get_other_player(player);
  assert(other_player != player);
  assert(other_player == 1 || other_player == 2);

  // TODO make these loops simpler maybe
  // TODO test early for check by threatening piece

  // Iterate through starting pos
  for (int y1 = 0; y1 < 8; y1++) {
    for (int x1 = 0; x1 < 8; x1++) {
      // start pos must be player's color
      Position p1 = { x1, y1 };
      if (piece_color(get_piece(cb, p1)) != player) continue;

      // Iterate through target pos
      for (int y2 = 0; y2 < 8; y2++) {
        for (int x2 = 0; x2 < 8; x2++) {
          Position p2 = { x2, y2 };
          if (is_legal_move(cb, p1, p2)) {

            // apply this move to a copy
            ChessBoard temp = *cb;
            apply_move(&temp, p1, p2);

            CheckStatus status = test_for_checks(&temp, player);

            if ((player == 1 && status == CHECK_ON_1) || 
                (player == 2 && status == CHECK_ON_2)) {
              // Still in check, this move fails
            } else {
              // They are no longer in check
              // this is not checkmate or stalemate
              return false;
            } 

          }
        }
      }

    }
  }

  // We have tried all possible moves for this player and none were valid
  // player is in checkmate or stalemate
  return true;
}

// Creates a temp, tests for CHECKS ONLY
inline CheckStatus test_for_checks(ChessBoard *cb, Position p1, Position p2) {
  // Copy only if necessary :

  ChessBoard temp = *cb;

  apply_move(&temp, p1, p2);
  int current_color = piece_color(get_piece(&temp, p2));
  assert(current_color != 0);

  CheckStatus result = test_for_checks(&temp, current_color);

  return result;
}

// Creates a temp, tests for checks and checkmates
// This doesn't currently assign to cb, but maybe it should
CheckStatus update_check_status(ChessBoard *cb, Position p1, Position p2) {
  ChessBoard temp = *cb;

  apply_move(&temp, p1, p2);
  int current_color = piece_color(get_piece(&temp, p2));
  assert(current_color != 0);
  int other_color = get_other_player(current_color);

  if (temp.static_moves == 50) return STALEMATE;

  CheckStatus result = test_for_checks(&temp, current_color);
 
  if (is_in_checkmate(&temp, other_color)) {
    if (result == CHECK_ON_1)      result = CHECKMATE_ON_1;
    else if (result == CHECK_ON_2) result = CHECKMATE_ON_2;
    else                           result = STALEMATE;
  }

  return result;
}

void print_legal_moves(ChessBoard *cb, Position pos) {
  PieceType temp[8][8];
  int x = pos.x;
  int y = pos.y;

  for (int y2 = 0; y2 < 8; y2++) {
    for (int x2 = 0; x2 < 8; x2++) {
      if (x == x2 && y == y2) {
        temp[y2][x2] = get_piece(cb, x2, y2);
        continue;
      }
      Position pos2 = {x2, y2};
      if (is_legal_move(cb, pos, pos2)) {
        CheckStatus status = test_for_checks(cb, pos, pos2);

        int color = piece_color(get_piece(cb, pos));
        if (color == 1 && status == CHECK_ON_1) {
          temp[y2][x2] = NON_VALID;
          continue;
        }
        if (color == 2 && status == CHECK_ON_2) {
          temp[y2][x2] = NON_VALID;
          continue;
        }
        temp[y2][x2] = get_piece(cb, x2, y2);
      } else {
        temp[y2][x2] = NON_VALID;
      }
    }
  }

  print_board(temp);
}

void print_check_status(CheckStatus check, FILE *file) {
  switch (check) {
    case NO_CHECK :
      return;
    case CHECK_ON_1 :
    case CHECK_ON_2 :
      fprintf(file, "Check.\n");
      return;
    case CHECKMATE_ON_1 :
      fprintf(file, "Checkmate, black wins.\n");
      return;
    case CHECKMATE_ON_2 :
      fprintf(file, "Checkmate, white wins.\n");
      return;
    case STALEMATE :
      fprintf(file, "Stalemate.\n");
      return;
  }
}

inline void print_check_status(CheckStatus check) {
  print_check_status(check, stdout);
}

void handle_4_chars(ChessStack *stack, char *line) {

  ChessBoard *cb = &top_frame(stack)->game;

  if (global_player == 0) { 
    printf("The game has ended. Use res to restart.\n");
    return;
  }

  Position p1;
  Position p2;
  if (!(check_in_range_low(line[0]) && 
        check_in_range_num(line[1]) &&
        check_in_range_low(line[2]) &&
        check_in_range_num(line[3]))
      ) {
    printf("Not valid : %s\n", line);
    return;
  }
  p1.x = line[0] - 'a';
  p1.y = line[1] - '1';
  p2.x = line[2] - 'a';
  p2.y = line[3] - '1';
  assert(p1.x >= 0 && p1.x < 8);
  assert(p1.y >= 0 && p1.y < 8);
  assert(p2.x >= 0 && p2.x < 8);
  assert(p2.y >= 0 && p2.y < 8);


  if (!is_legal_move(cb, p1, p2)) {
    printf("Illegal Move : %s\n", line);
    return;
  }

  int p1_color = piece_color(get_piece(cb, p1));
  if (p1_color != global_player && !dev_mode) {
    if (p1_color == 1) printf("Illegal Move, it's black's turn : %s\n", line);
    else               printf("Illegal Move, it's white's turn : %s\n", line);
    return;
  }

  assert(p1_color);

  CheckStatus check = update_check_status(cb, p1, p2);
  assert(p1_color == piece_color(get_piece(cb, p1)));

  if (p1_color == 2) {
    // piece is black
    if (check == CHECK_ON_2) {
      printf("Illegal Move, check on black king : %s\n", line);
      return;
    }
  } else if (p1_color == 1) {
    // piece is white
    if (check == CHECK_ON_1) {
      printf("Illegal Move, check on white king : %s\n", line);
      return;
    }
  }

  // TODO handle check_status and current_player in a more rigorous way
  cb->check_status = check;

  push_move(stack, p1, p2);
  cb = &top_frame(stack)->game;

  print_move(p1, p2);
  print_board(cb->board);
  print_check_status(check);

  
  // Game is over:
  if (check == CHECKMATE_ON_1 || check == CHECKMATE_ON_2 || check == STALEMATE) {
    global_player = 0;
  } else {
    assert(global_player);
    global_player = get_other_player(global_player);
  }
  
  return;
}

void handle_3_chars(ChessBoard *cb, char *line) {
  if (!strcmp(line, "dev\n")) {
    dev_mode = !dev_mode;
    return;
  } 
  if (!strcmp(line, "res\n")) {
    ChessBoard temp;
    *cb = temp;
    global_player = 1;
    print_board(cb->board);
    return;
  } 
  if (!strcmp(line, "com\n")) {
    // TODO add a way to toggle/set to white
    com_player = 2;
    return;
  } 
  if (line[0] == 'k') {
    if (!(
          check_in_range_low(line[1]) && 
          check_in_range_num(line[2]))
        ) {
      printf("Not valid : %s\n", line);
      return;
    }
    Position pos;
    pos.x = line[1] - 'a';
    pos.y = line[2] - '1';

    cb->board[pos.y][pos.x] = EMPTY;
  }
  printf("Not valid : %s\n", line);
} 

void handle_1_char(ChessBoard *cb, char *line) {
  printf("Not valid : %s\n", line);
} 

void handle_2_chars(ChessBoard *cb, char *line) {
  Position pos;
  if (!(
        check_in_range_low(line[0]) && 
        check_in_range_num(line[1]))
      ) {
    printf("Not valid : %s\n", line);
    return;
  }
  pos.x = line[0] - 'a';
  pos.y = line[1] - '1';
  assert(pos.x >= 0 && pos.x < 8);
  assert(pos.y >= 0 && pos.y < 8);

  print_legal_moves(cb, pos);
  print_board(cb->board);
}

// AI STUFF STARTS HERE

#define evaluate better_eval

int simple_eval(ChessBoard *cb) {

  int result = 0;
  switch (cb->check_status) {
    case NO_CHECK       : break;
    case CHECK_ON_1     : result -= 1; break;
    case CHECK_ON_2     : result += 1; break;
    case CHECKMATE_ON_1 : return INT_MIN;
    case CHECKMATE_ON_2 : return INT_MAX;
    case STALEMATE      : return 0;
  }

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {

      PieceType p = get_piece(cb, x, y);
      int sign = (piece_color(p) == 1) ? 1 : -1;
      PieceType type = p & PMASK;

      switch (type) {
        case EMPTY    : break;

        case PAWN   : result += 1 * sign; break;
        case ROOK   : result += 5 * sign; break;
        case BISHOP : result += 3 * sign; break;
        case KNIGHT : result += 3 * sign; break;
        case KING   : break;
        case QUEEN  : result += 9 * sign; break;

        case NON_VALID: break;
      }
    }
  }
  return result;
}

int better_eval(ChessBoard *cb) {

  int result = 0;
  switch (cb->check_status) {
    case NO_CHECK       : break;
    case CHECK_ON_1     : result -= 1; break;
    case CHECK_ON_2     : result += 1; break;
    case CHECKMATE_ON_1 : return INT_MIN;
    case CHECKMATE_ON_2 : return INT_MAX;
    case STALEMATE      : return 0;
  }

  int white_advancements = 0;
  int black_advancements = 0;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {

      PieceType p = get_piece(cb, x, y);
      int sign = (piece_color(p) == 1) ? 1 : -1;
      PieceType type = p & PMASK;

      assert(piece_color(p) != 0 || type == EMPTY);
      assert(type != EMPTY || piece_color(p) == 0);
      assert(type != NON_VALID);

      switch (type) {
        case EMPTY    : continue;

        case PAWN   : result += 2  * sign; break;
        case ROOK   : result += 10 * sign; break;
        case BISHOP : result += 7  * sign; break;
        case KNIGHT : result += 6  * sign; break;
        case KING   : break;
        case QUEEN  : result += 18 * sign; break;

        case NON_VALID: continue;
      }
      if (sign == 1) white_advancements += y;
      else           black_advancements += 7 - y;
    }
  }
  result += white_advancements / 4;
  result -= black_advancements / 4;
  return result;
}

bool is_sorted(int *arr, int len) {
  int prev = INT_MAX;
  for (int i = 0; i < len; i++) {
    int val = arr[i];
    if (val > prev) return false;
    prev = val;
  }
  return true;
}

bool is_sorted(ChessNode *children, int *order, int len) {
  int prev = INT_MAX;
  for (int i = 0; i < len; i++) {
    int child_idx = order[i];
    int val = children[child_idx].value;
    if (val > prev) return false;
    prev = val;
  }
  return true;
}

inline void sorted_insert(ComAllocator *allocator, int start_idx, int child_idx, int val) {
  assert(start_idx + child_idx < MAX_NODES);
  assert(start_idx + child_idx == allocator->num_nodes);
  assert(child_idx < 256);
  assert(child_idx >= 0);
  assert(start_idx >= 0);

  int       *arr      = allocator->order + start_idx;
  ChessNode *children = allocator->nodes + start_idx;
  // int num_children    = child_idx + 1;

  assert(children[child_idx].value == val);

  int idx  = child_idx; // starts at the end
  arr[idx] = child_idx;

  // Swaps to the left until the array is sorted:
  while (idx > 0 && val > children[arr[idx - 1]].value) {
    assert(arr[idx-1] >= 0);
    assert(arr[idx-1] < child_idx);

    arr[idx]   = arr[idx-1];
    arr[idx-1] = child_idx;
    idx--;
  } 
}

void print_child_order(ComAllocator *allocator, int node_idx) {

  ChessNode     *node = allocator->nodes + node_idx;
  int   *sorted_order = allocator->order + node->children;
  ChessNode *children = allocator->nodes + node->children;
  int prev = INT_MAX;
  for (int i = 0; i < node->num_children; i++) {
    int value = children[sorted_order[i]].value;
    printf("(%d, %d); ", sorted_order[i], value);
    //assert(value <= prev);
    prev = value;
  }
  printf("\n");
}

// TODO test the shit out of this
// TODO test to see if check mate info is correct
// * test to see if sorting works
void generate_children(int node_idx, ComAllocator *allocator, int player) {

  ChessNode *node = allocator->nodes + node_idx;

  node->children = allocator->num_nodes;
  assert(node->children > node_idx);
  node->num_children = 0;

  // out of memory, might want to handle this in parent:
  if (allocator->num_nodes == MAX_NODES) {
    printf("Com Player Error: out of memory\n");
    return; 
  }

  ChessBoard *cb = allocator->games + node_idx;

  for (int y1 = 0; y1 < 8; y1++) {
    for (int x1 = 0; x1 < 8; x1++) {

      Position p1 = { x1, y1 };
      // start pos must be player's color
      if (piece_color(get_piece(cb, p1)) != player) continue;

      // Iterate through target pos
      for (int y2 = 0; y2 < 8; y2++) {
        for (int x2 = 0; x2 < 8; x2++) {

          Position p2 = { x2, y2 };
          if (is_legal_move(cb, p1, p2)) {

            // The newest node should be this child
            assert(node->children + node->num_children == allocator->num_nodes);
            ChessNode *child = allocator->nodes + allocator->num_nodes;
            ChessBoard *game = allocator->games + allocator->num_nodes;


            // apply this move to a copy
            *game = *cb;
            apply_move(game, p1, p2);

            CheckStatus status = test_for_checks(game, player);


            if ((player == 1 && status == CHECK_ON_1) || 
                (player == 2 && status == CHECK_ON_2)) {
              // In check, this move fails
            
            } else {
              // This move is valid

              // Copy and paste from update_check_status:

              if (game->static_moves == 50) {
                status = STALEMATE;
              } else {
                assert(player);
                if (is_in_checkmate(game, get_other_player(player))) {
                  if (status == CHECK_ON_1)      status = CHECKMATE_ON_1;
                  else if (status == CHECK_ON_2) status = CHECKMATE_ON_2;
                  else                           status = STALEMATE;
                }
              }


              game->check_status = status;
              child->value = evaluate(game);
              node->value = DEBUG_VAL; // TODO this may not be needed
              // TODO I suspect that the parent is not getting the value always...
              child->move.start = p1;
              child->move.dest  = p2;
              child->children   = -1;
              child->num_children = -1;

              sorted_insert(allocator, node->children, node->num_children, child->value);
              node->num_children++;
              assert(node->num_children <= 256);
              allocator->num_nodes++;
              if (allocator->num_nodes == MAX_NODES) {
                printf("Com Player Error: out of memory\n");
                return;
              }
            }

          }
        }
      }

    }
  }

  //print_child_order(allocator, node_idx);
}

void heapify(ChessNode *children, int *order, int idx, int len) {

  int smallest = idx;
  int l = 2*idx + 1;
  int r = 2*idx + 2;

  // If left child is larger than root
  if (l < len) {
    int l_val = children[order[l]].value;
    if (l_val < children[order[smallest]].value) smallest = l;
  }

  // If right child is larger than largest so far
  if (r < len) {
    int r_val = children[order[r]].value;
    if (r_val < children[order[smallest]].value) smallest = r;
  }

  // If largest is not root
  if (smallest != idx) {

    int temp = order[idx];
    order[idx] = order[smallest];
    order[smallest] = temp;

    // Recursively heapify the affected sub-tree
    heapify(children, order, smallest, len);
  }
}

bool is_heap(ChessNode *children, int *order, int idx, int len) {
  int l = 2*idx + 1;
  int r = 2*idx + 2;

  assert(idx < len);
  int current_val = children[order[idx]].value;

  if (l < len) {
    int l_val = children[order[l]].value;
    if (l_val < current_val) return false;
    if (!is_heap(children, order, l, len)) return false;
  }

  // If right child is larger than largest so far
  if (r < len) {
    int r_val = children[order[r]].value;
    if (r_val < current_val) return false;
    if (!is_heap(children, order, r, len)) return false;
  }

  return true; 
}
 
inline void sort_children(ComAllocator *allocator, int node_idx) {
  ChessNode *node     = allocator->nodes + node_idx;
  ChessNode *children = allocator->nodes + node->children;
  int       *order    = allocator->order + node->children;
  int len = node->num_children;

  assert(len >= 0);
  assert(node->children >= 0);

  for (int i = len / 2 - 1; i >= 0; i--) {
    heapify(children, order, i, len);
  }

  // assert(is_heap(children, order, 0, len));

  // One by one extract an element from heap
  for (int i = len - 1; i > 0; i--) {

    int temp = order[0];
    order[0] = order[i];
    order[i] = temp;

    heapify(children, order, 0, i);

#if 0
    if (!is_heap(children, order, 0, i)) {

      printf("i : %d\n", i);
      print_child_order(allocator, node_idx);
    }
#endif

  }

#if 0
  if (!is_sorted(children, order, len)) {
    print_child_order(allocator, node_idx);
    assert(false);
  }
#endif
}

// TODO check to see if this is actually generating moves properly
int alpha_beta(ComAllocator *allocator, int node_idx, int depth, int alpha, int beta, int player) {
  traversal_count++;
  ChessNode *node = allocator->nodes + node_idx;
  ChessBoard *game = allocator->games + node_idx;

  if (depth == 0) { 
    int v = evaluate(game);
    node->value = v;
    node->num_children = -1;
    node->children = -1;
    node->value = v;
    return v;
  }

  if (depth == 1 || node->num_children == -1) {
    generate_children(node_idx, allocator, player);
  }
  else {
    // Test the sort
    sort_children(allocator, node_idx);
  }

  if (node->num_children == 0) { 
    int v = evaluate(game);
    node->value = v;
    return v;
  }

  int *sorted_order = allocator->order + node->children;
  //assert(is_sorted(allocator->nodes + node->children, sorted_order, node->num_children));

  if (player == 1) {
    // Maximizing

    int v = INT_MIN;
    for (int i = 0; i < node->num_children; i++) {
      int sorted_idx = sorted_order[i];
      v = max(v, alpha_beta(allocator, node->children + sorted_idx, depth - 1, alpha, beta, 2));
      alpha = max(alpha, v);
      if (beta <= alpha) break;
    }
    node->value = v;
    return v;

  } else {
    // Minimizing
    assert(player == 2);

    int v = INT_MAX;
    for (int i = 0; i < node->num_children; i++) {
      int sorted_idx = sorted_order[node->num_children - 1 - i];
      v = min(v, alpha_beta(allocator, node->children + sorted_idx, depth - 1, alpha, beta, 1));
      beta = min(beta, v);
      if (beta <= alpha) break;
    }
    node->value = v;
    return v;
  }
}

void print_int_array(int *arr, int len, FILE *file) {
  // TODO fix the appearance
  fprintf(file, "[ ");
  for (int i = 0; i < len; i++) {
    fprintf(file, "%d ", arr[i]);
  }
  fprintf(file, "]\n");
}

inline void print_int_array(int *arr, int len) {
  print_int_array(arr, len, stdout);
}

void print_move_tree(ComAllocator *allocator, int node_idx, int current_player, int depth) {
  ChessNode *root = allocator->nodes + node_idx;
  ChessBoard *game = allocator->games + node_idx;

  fprintf(output_file, " ------ Printing Node %d at depth %d -------- \n", node_idx, depth);
  fprintf(output_file, "Current player : %d\n", current_player);
  fprintf(output_file, "Move was : "); print_move(root->move.start, root->move.dest, output_file);
  print_check_status(game->check_status, output_file);
  fprintf(output_file, "Num children : %d\n", root->num_children);
  fprintf(output_file, "Sort order : "); print_int_array(allocator->order + root->children, root->num_children, output_file);
  fprintf(output_file, "Value : %d\n", root->value);
  // TODO check that children values are sane
  
  print_board(game->board, output_file);
  fprintf(output_file, "\n\n");

  for (int i = 0; i < root->num_children; i++) {
    print_move_tree(allocator, root->children + i, get_other_player(current_player), depth + 1);
  }
}

ChessMove get_best_move(ComAllocator *allocator, int current_player) {
  // TODO For now this is fine, but I might want to use a data structure that 
  // doesn't need to be rebuilt each time eventually

  ChessNode *root = allocator->nodes;

  
  for (int i = 1; i <= MAX_DEPTH; i++) {
    alpha_beta(allocator, 0, i, INT_MIN, INT_MAX, current_player);
  }
 
  /*
  auto error = tree_is_malformed(allocator);
  if (error) print_error(error);
  */

  root = allocator->nodes;
  assert(root->children == 1);
  ChessNode *children = allocator->nodes + 1; // Children of the root node
  int num_children = root->num_children;
  ChessMove best_move = {{ -1, -1 }, { -1, -1 }};

  // TODO select based on sorting instead
  // Note that Com is a minimizing player
  assert(root->children == 1);
  int *sorted_order = allocator->order + root->children;
  sort_children(allocator, 0);
  assert(is_sorted(children, sorted_order, num_children));
  int best_child_idx = sorted_order[num_children - 1];
  ChessNode *sorted_child = children + best_child_idx;
  int sorted_child_val = sorted_child->value;

  // TODO cleanup ugly old code when done debugging
  int best_val = INT_MAX;
  /*
  for (int i = 0; i < num_children; i++) {
    ChessNode *child = children + i;
    if (best_val >= child->value) {
      best_val = child->value;
      best_move = child->move;
    }
  }
  assert(sorted_child_val == best_val);
  */

  best_move = sorted_child->move;
  best_val  = sorted_child_val;


  printf("Total traversed : %d\n", traversal_count);
  printf("Value : %d\n", best_val);

#if DEBUG_FILE
  fprintf(output_file, "\n\nNum nodes was %d.\n", allocator->num_nodes);
  // print_move_tree(allocator, 0, current_player, 0);
  print_predicted_boards(allocator, output_file);
  fflush(output_file);
#endif

  return best_move;
}


// Only compares piece types and colors
bool equal(ChessBoard *cb1, ChessBoard *cb2) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      PieceType p1 = get_piece(cb1, x, y);
      PieceType p2 = get_piece(cb2, x, y);
      p1 &= FULL_MASK;
      p2 &= FULL_MASK;
      if (p1 != p2) return false;
    }
  }
  return true;
}

inline ChessFrame *alloc_frame(ChessStack *stack) {
  stack->size++;
  auto top = top_frame(stack);
  top->count = 1;
  assert(stack->size < NUM_FRAMES);
  return top;
}

inline ChessFrame *top_frame(ChessStack *stack) {
  return stack->frames + stack->size - 1;
}

// TODO it is not correctly updating the board count for black
void update_count(ChessStack *stack) {
  ChessFrame *latest = top_frame(stack);
  ChessFrame *current = latest - 2; // first current should be -4

  // TODO eventually remove debug code
  int loop_count = 0;
  // TODO maybe switch to using static moves
  while (true) {
    loop_count++;
    assert(loop_count <= 25);

    // TODO possibly need to handle promotion in the future
    current = current - 2; // skip other players turn

    // We hit the end of the stack:
    if (current < stack->frames) break;

    if (current != stack->frames) {
      // break if a pawn moved or a piece was taken
      Position dest = current->move.dest;
      PieceType moved_piece = get_piece(&current->game, dest) & PMASK;
      if (moved_piece == PAWN) break;

      ChessFrame *prev = current - 1;
      // TODO make sure this is correct, may be old/new value for en_passant
      if (equal(dest, prev->game.en_passant)) break;

      PieceType taken_piece = get_piece(&prev->game, dest);
      if (taken_piece != EMPTY) break;  
    }

    // TODO check the last moved piece first
    // TODO maybe make further improvments

    // TODO check that this doesn't miss anything or compare incorrectly
    if (equal(&current->game, &latest->game)) {
      latest->count = current->count + 1;

      printf("loop_count : %d\n", loop_count);
      printf("board count : %d\n", latest->count);
      return;
    }
  }
  latest->count = 1;
  printf("loop_count : %d\n", loop_count);
}


// TODO finish this?
MalformedTreeError tree_is_malformed(ComAllocator *allocator) {
  ChessNode *node = allocator->nodes;
  bool white_player = false;
  int total_children = 0;
  for (int i = 0; i < allocator->num_nodes; i++, node++) {
    ChessNode *children = allocator->nodes + node->children;
    int num_children = node->num_children;
    int value = node->value;
    if (value == DEBUG_VAL) return DEFAULT_VALUE_HEURISTIC;
    if (num_children + node->children > allocator->num_nodes) return CHILDREN_EXCEED_NUM_NODES;


    if (num_children == 0) {
      if (value != INT_MAX && value != INT_MIN && value != 0) return NO_CHILDREN_FOR_NON_MATE;
      continue;
    } else if (num_children == -1) {
      if (value != evaluate(allocator->games + i)) return INCORRECT_TERMINAL_VALUE;
      continue;
    } else if (num_children < -1) {
      return MALFORMED_NUM_CHILDREN;
    }

    assert(node->children > 0);
    // TODO make these return probably
    assert(node->children != i); // NODE IS OWN CHILD
    assert(node->children > i); // CHILD IDX PRECEDES NODE IDX

    total_children += num_children; // only do this after we know it isn't -1

    // true if last move was black
    if (i != 0) white_player = piece_color(get_piece(allocator->games + i, node->move.dest)) == 2;

    ChessNode *best_child;
    // When the nodes are meant to be sorted :
#if 0
    int *sorted_order = allocator->order + node->children;
    if (!is_sorted(children, sorted_order, num_children)) return UNSORTED_CHILDREN;

    if (white_player) {
      // Move made by white
      int max_child_idx = sorted_order[0];
      best_child = children + max_child_idx;
    } else {
      // Move made by black
      int min_child_idx = sorted_order[num_children - 1];
      best_child = children + min_child_idx;
    }
#else
    // Loop to find best child :
    best_child = children;
    for (int child_idx = 1; child_idx < num_children; child_idx++) {
      ChessNode *current = children + child_idx;

      if ((white_player  && current->value > best_child->value) ||
          (!white_player && current->value < best_child->value)) {

        best_child = current;
      }
    }
#endif

    if (best_child->value != value) {
      continue; // TODO consider removing this
      printf("i = %d\n", i);
      if (white_player) printf("white player\n");
      else              printf("black player\n");
      printf("child v = %d\n", best_child->value);
      printf("parent v = %d\n", value);
      printf("num_children = %d\n", num_children);
      printf("num child children = %d\n", best_child->num_children);
      return UNPROPAGATED_CHILD_VALUE;
    }
  }
  if (total_children + 1 != allocator->num_nodes) return MISMATCHED_NODE_COUNT;
  return NO_ERROR;
}

#define print_error_enum(E) printf("Error : " #E "\n")

void print_error(MalformedTreeError error) {
  assert(error);
  switch (error) {
    case NO_ERROR :
      assert(NO_ERROR);
    case CHILDREN_EXCEED_NUM_NODES :
      print_error_enum(CHILDREN_EXCEED_NUM_NODES);
      break;
    case NO_CHILDREN_FOR_NON_MATE :
      print_error_enum(NO_CHILDREN_FOR_NON_MATE);
      break;
    case INCORRECT_TERMINAL_VALUE :
      print_error_enum(INCORRECT_TERMINAL_VALUE);
      break;
    case MALFORMED_NUM_CHILDREN :
      print_error_enum(MALFORMED_NUM_CHILDREN);
      break;
    case UNPROPAGATED_CHILD_VALUE :
      print_error_enum(UNPROPAGATED_CHILD_VALUE);
      break;
    case MISMATCHED_NODE_COUNT :
      print_error_enum(MISMATCHED_NODE_COUNT);
      break;
    case UNSORTED_CHILDREN :
      print_error_enum(UNSORTED_CHILDREN);
      break;
    case DEFAULT_VALUE_HEURISTIC :
      print_error_enum(DEFAULT_VALUE_HEURISTIC);
      break;
  }
}

void print_predicted_boards(ComAllocator *allocator, FILE *file) {
  fprintf(file, "\n");
  ChessNode  *nodes = allocator->nodes;
  ChessBoard *games = allocator->games;

  ChessNode *root = nodes;
  int children = root->children;
  int num_children = root->num_children;
  int *sorted_order = allocator->order + children;
  //sort_children(allocator, 0);
  assert(children == 1);
  assert(is_sorted(root + 1, sorted_order, num_children));

  print_board(games->board, file);

  // TODO adjust this so it works with white com player
  bool white_player = false;
  // Black minimizes:
  int node_idx = children + sorted_order[num_children - 1];

  while (true) {

    ChessNode *node = nodes + node_idx;
    ChessBoard *cb  = games + node_idx;
    children     = node->children;
    // TODO not sure about this termination condition...
    if (children < 1) break;
    num_children = node->num_children;
    sorted_order = allocator->order + children;
    sort_children(allocator, node_idx);

    int child_idx;

    if (white_player) {
      fprintf(file, "\nWhite's move : ");
      assert(piece_color(get_piece(cb, node->move.dest)) == 1);
      // Black's move:
      child_idx = sorted_order[num_children - 1];
    } else {
      fprintf(file, "\nBlacks's move : ");
      assert(piece_color(get_piece(cb, node->move.dest)) == 2);
      // White's move:
      child_idx = sorted_order[0];
    }

    print_move(node->move.start, node->move.dest, file);
    print_board(cb->board, file);
    fprintf(file, "\nPredicted value : %d\n", node->value);
    fprintf(file, "Real value : %d\n", evaluate(cb));

    node_idx = children + child_idx;
    white_player = !white_player;
  }

  auto error = tree_is_malformed(allocator);
  if (error) {
    print_error(error);
  }
}

// TODO flesh this out more?
void print_stack(ChessStack *stack, FILE *file) {
  bool white_player = true;
  fprintf(file, "%d board stack :\n", stack->size);
  print_board(stack->frames[0].game.board, file);
  for (int i = 1; i < stack->size; i++) {
    ChessFrame *current = stack->frames + i;

    if (white_player) {
      fprintf(file, "\nWhite's move : ");
    } else {
      fprintf(file, "\nBlacks's move : ");
    }

    print_move(current->move.start, current->move.dest, file);
    fprintf(file, "\n");
    print_board(current->game.board, file);
    white_player = !white_player;
  }
}

// TODO:
//
// * retraverse move tree for better pruning
// * store all previous board states with moves (maximum ~6000 moves)
// * print stored boards to file
// - print out boards predicted by com player
// - traverse backwards to find repeated board states
// - switch to correct counting for static moves
// - factor out mainline
// - implment undo/redo
// - rename/refactor all the check/checkmate functions
// - improve test interface
// - output completed, labeled games to a file
// - automate black vs white games
// - machine learned eval function
// - Improve interface
// - speed up check/checkmate by saving attacked positions?
// * Handle draws?
// * Count moves?
// * Make a com command that activates ai
// * alpha-beta pruning and simple evaluation function
//

int main() {

#if DEBUG_FILE
  output_file = fopen("predicted_boards.txt", "w");
#endif
  FILE *stack_file = fopen("game_log.txt", "w");

  // TODO this stuff should maybe not be done if there is no computer
  ComAllocator *com_allocator = (ComAllocator *) malloc(sizeof(ComAllocator));
  // TODO remove this when done debugging:
  for (int i = 0; i < MAX_NODES; i++) {
    com_allocator->nodes[i].value = DEBUG_VAL;
  }

  ChessBoard start;
  ChessStack *stack = (ChessStack *) malloc(sizeof(ChessStack));
  stack->frames[0].game = start;
  stack->size = 1;
  ChessBoard *cb = &top_frame(stack)->game;

  print_board(cb->board);

  while (true) {

    // TODO probably do this elsewhere
    ChessBoard *cb = &top_frame(stack)->game;
    int current_val = evaluate(cb);
    printf("Current value : %d\n", current_val);

    char line[6];

    if (global_player == com_player) {
      assert(global_player);
      // update allocator :
      com_allocator->games[0] = *cb;
      com_allocator->nodes[0].num_children = 0;
      com_allocator->nodes[0].children = 1; // idx of first child

      com_allocator->num_nodes = 1;

      // have ai pick move:

      ChessMove move = get_best_move(com_allocator, global_player);
      if (move.start.x == -1) {
        printf("No legal moves found.\n");
        break;
      }

      if (get_piece(cb, move.start) == EMPTY) {
        printf("Com Player Error: %d, %d to %d, %d\n", move.start.x, move.start.y, move.dest.x, move.dest.y);
      }

      cb->check_status = update_check_status(cb, move.start, move.dest);
      // make move
      push_move(stack, move.start, move.dest);
      cb = &top_frame(stack)->game;

      // switch current player
      global_player = get_other_player(global_player);
      assert(global_player != com_player);
      print_move(move.start, move.dest);
      print_board(cb->board);
      print_check_status(cb->check_status);
      continue;
    }
    if (fgets(line, sizeof(line), stdin)) { 
      if (line[0] == 'q' || line[0] == 'Q') break;
      int len = strlen(line);
      switch (len - 1) {
        case 0 :
          continue;
        case 1 :
          handle_1_char(cb, line); break;
        case 2 :
          handle_2_chars(cb, line); break;
        case 3 :
          handle_3_chars(cb, line); break;
        case 4 :
          handle_4_chars(stack, line); 
          break;
          // TODO look for newline char and eat rest of line
      }
    }
  }

  print_stack(stack, stack_file);
  // TODO close files?
  return 0;
}


