
typedef enum : int8_t {
  EMPTY  = 0,
  PAWN   = 1,
  ROOK   = 2,
  BISHOP = 3,
  KNIGHT = 4,
  KING   = 5,
  QUEEN  = 6,

  NON_VALID = 7

} PieceType;

inline PieceType operator| (PieceType _a, PieceType _b) {
  return PieceType(static_cast<int>(_a) | static_cast<int>(_b));
}

inline PieceType operator& (PieceType _a, PieceType _b) {
  return PieceType(static_cast<int>(_a) & static_cast<int>(_b));
}

// TODO double check this...
inline PieceType& operator&= (PieceType &_a, PieceType _b) {
  _a = _a & _b;
  return _a;
}

// Flags for pieces:

// 0x00 thru 0x07 reserved by enum
#define PMASK   ((PieceType) 0x07)

#define PWHITE  EMPTY
#define PBLACK  ((PieceType) 0x80)

// used for rooks/king:
#define PMOVED  ((PieceType) 0x40)

// TODO use these somehow:
#define PTHREAT ((PieceType) 0x20)
#define PGUARD  ((PieceType) 0x10)

// masks to type and color
#define FULL_MASK (PMASK | PBLACK)

inline int piece_color(PieceType p);

// TODO make this into flags:
typedef enum {
  NO_CHECK,
  CHECK_ON_1,
  CHECK_ON_2,
  CHECKMATE_ON_1,
  CHECKMATE_ON_2,
  STALEMATE
} CheckStatus;

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  Position start;
  Position dest;
} ChessMove;


#define back_row(C) { \
  ROOK   | C, \
  KNIGHT | C, \
  BISHOP | C, \
  QUEEN  | C, \
  KING   | C, \
  BISHOP | C, \
  KNIGHT | C, \
  ROOK   | C  \
}

#define pawn_row(C) { \
  PAWN | C, \
  PAWN | C, \
  PAWN | C, \
  PAWN | C, \
  PAWN | C, \
  PAWN | C, \
  PAWN | C, \
  PAWN | C  \
}

#define empty_row { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY }

// TODO:
// Make king_1/2_pos, en_passant smaller
// Might not need en_passant if we store previous moves
// Make check_status smaller
// *Make PieceType into just 7 values with sign bit == color
// Add flags to board for threat grid and "already moved"
// Make static moves smaller
typedef struct {
  // int current_player = 1; // TODO maybe add this as part of status
  Position king_1_pos = { 4, 0 };
  Position king_2_pos = { 4, 7 };
  Position en_passant = { -1, -1 };
  CheckStatus check_status = NO_CHECK;
  int static_moves = 0;
  PieceType board[8][8] = {
    back_row(PWHITE),
    pawn_row(PWHITE),
    empty_row,
    empty_row,
    empty_row,
    empty_row,
    pawn_row(PBLACK),
    back_row(PBLACK)
  };
  // uint8_t threat_grid[8];
} ChessBoard;


inline int get_other_player(int player);

inline bool equal(Position p1, Position p2);

inline PieceType get_piece(ChessBoard *cb, int x, int y);

inline PieceType get_piece(ChessBoard *cb, Position p);

inline PieceType set_piece(ChessBoard *cb, int x, int y, PieceType p);

inline PieceType set_piece(ChessBoard *cb, Position pos, PieceType p);


typedef struct {
  int count = 0;
  ChessMove  move = {{-1,-1}, {-1,-1}};
  ChessBoard game;
} ChessFrame;

#define NUM_FRAMES 6000

// TODO adjust memory
typedef struct {
  int size = 0;
  ChessFrame frames[NUM_FRAMES];
} ChessStack;

bool equal(ChessBoard *f1, ChessBoard *f2);

inline ChessFrame *alloc_frame(ChessStack *stack);
inline ChessFrame *top_frame(ChessStack *stack);

void update_count(ChessStack *stack);



inline bool check_in_range_low(char letter);

// TODO allow caps?
inline bool check_in_range_cap(char letter);

inline bool check_in_range_num(char letter);


bool collides_diagonal(PieceType board[8][8], Position p1, Position p2);

bool collides_normal(PieceType board[8][8], Position p1, Position p2);

inline bool rook_move(Position p1, Position p2);

inline bool bishop_move(Position p1, Position p2);

inline bool knight_move(Position p1, Position p2);

inline bool king_move(ChessBoard *cb, Position p1, Position p2);


inline bool legal_target(PieceType p, PieceType target);

bool legal_pawn_target(PieceType board[8][8], Position p1, Position p2);

bool is_legal_move(ChessBoard *cb, Position p1, Position p2);

// TODO consider renaming all this garbage...
inline CheckStatus test_for_checks(ChessBoard *cb, int acting_player);

inline CheckStatus test_for_checks(ChessBoard *cb, Position p1, Position p2);

bool is_in_checkmate(ChessBoard *cb, int player);

CheckStatus update_check_status(ChessBoard *cb, Position p1, Position p2);


inline void apply_move(ChessBoard *cb, Position p1, Position p2);
inline void push_move(ChessStack *stack, Position p1, Position p2);


void print_board(PieceType board[8][8], FILE *file);
void print_board(PieceType board[8][8]);

inline void print_move(Position p1, Position p2, FILE *file);
inline void print_move(Position p1, Position p2);

void print_legal_moves(ChessBoard *cb, Position pos);

void print_check_status(CheckStatus check, FILE *file);
void print_check_status(CheckStatus check);

void print_int_array(int *arr, int len, FILE *file);
void print_int_array(int *arr, int len);

void handle_4_chars(ChessBoard *cb, char *line);

void handle_3_chars(ChessBoard *cb, char *line);

void handle_1_char(char *line);

void handle_2_chars(ChessBoard *cb, char *line);

PieceType get_promotion_input(int player);

int simple_eval(ChessBoard *cb);
int better_eval(ChessBoard *cb);


// TODO think a bit harder about memory
// NOTE: game is associated by index

#define DEBUG_VAL 1234567

typedef struct {   
  ChessMove move;
  int children; // index
  int num_children;
  int value = DEBUG_VAL; // TODO remove this when done debugging
} ChessNode;

#define MAX_NODES (65536*128*2)
#define MAX_DEPTH 6

typedef struct {
  ChessNode  nodes[MAX_NODES];
  ChessBoard games[MAX_NODES];
  int        order[MAX_NODES];
  int num_nodes;
} ComAllocator;

inline int max(int a, int b) {
  return (a > b) ? a : b;
}

inline int min(int a, int b) {
  return (a < b) ? a : b;
}

bool is_sorted(int *arr, int len);

bool is_sorted(ChessNode *children, int *order, int len);


inline void sorted_insert(ComAllocator *allocator, int start_idx, int child_idx, int val);

void print_child_order(ComAllocator *allocator, int node_idx);


void generate_children(int node_idx, ComAllocator *allocator, int player);

void heapify(ChessNode *children, int *order, int idx, int len);


bool is_heap(ChessNode *children, int *order, int idx, int len);
 
inline void sort_children(ComAllocator *allocator, int node_idx);

int alpha_beta(ComAllocator *allocator, int node_idx, int depth, int alpha, int beta, int player);


void print_move_tree(ComAllocator *allocator, int node_idx, int current_player, int depth);
void print_predicted_boards(ComAllocator *allocator, FILE *file);

typedef enum {
  NO_ERROR = 0,
  CHILDREN_EXCEED_NUM_NODES,
  NO_CHILDREN_FOR_NON_MATE,
  INCORRECT_TERMINAL_VALUE,
  MALFORMED_NUM_CHILDREN,
  UNPROPAGATED_CHILD_VALUE,
  MISMATCHED_NODE_COUNT,
  UNSORTED_CHILDREN,
  DEFAULT_VALUE_HEURISTIC
} MalformedTreeError;

MalformedTreeError tree_is_malformed(ComAllocator *allocator);
void print_error(MalformedTreeError error);

ChessMove get_best_move(ComAllocator *allocator, int current_player);



#define UNI_WP "WP"
#define UNI_WR "WR"
#define UNI_WN "WN"
#define UNI_WB "WB"
#define UNI_WK "WK"
#define UNI_WQ "WQ"

#define UNI_BP "BP"
#define UNI_BR "BR"
#define UNI_BN "BN"
#define UNI_BB "BB"
#define UNI_BK "BK"
#define UNI_BQ "BQ"

// TODO need to make it so that piece type alternates with 
// background color
/*
#define UNI_WP "\u265F "
#define UNI_WR "WR"
#define UNI_WN "WN"
#define UNI_WB "WB"
#define UNI_WK "\u2654 "
#define UNI_WQ "WQ"

#define UNI_BP "BP"
#define UNI_BR "BR"
#define UNI_BN "BN"
#define UNI_BB "BB"
#define UNI_BK "\u265A "
#define UNI_BQ "BQ"

*/

char unicodePieces[2][6][4] = 
{
    { "\u2654", "\u2655", "\u2656", "\u2657", "\u2658", "\u2659" },
    { "\u265A", "\u265B", "\u265C", "\u265D", "\u265E", "\u265F" }
};



