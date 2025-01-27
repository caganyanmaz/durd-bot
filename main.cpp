#include <iostream>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>

#define DEBUGGING

const int L_SIZE     = 7; // How many cards are in the left  (including the middle), for development it's 7 (A, 2, 3,  4, 5, 6, 7)
const int R_SIZE     = 7; // How many cards are in the right (including the middle), for development it's 7 (7, 8, 9, 10, J, Q, K)
const int SUIT_COUNT = 4; // How many suits are in the game (always 4, don't change it for debugging)
const int MCARD      = L_SIZE-1;
const int SUIT_SIZE  = L_SIZE + R_SIZE - 1;
const int DECK_SIZE = SUIT_SIZE * SUIT_COUNT;

typedef std::array<std::bitset<SUIT_SIZE>, SUIT_COUNT> Deck;
typedef std::array<std::array<int, 2>, SUIT_COUNT> PlayerBound;

inline int opponent(int player)
{
    return 3 - player;
}

struct GameState
{
    int current_player;
    std::array<std::array<int, 2>, SUIT_COUNT> table_state;
    bool suit_opened(int suit) const { return table_state[suit][1] != R_SIZE; }
    int get_left(int suit) const { return table_state[suit][0]-1; }
    int get_right(int suit) const { return table_state[suit][1]+L_SIZE; }
    void switch_players() { current_player = opponent(current_player); }
    void apply_move(int suit, int side, int val) { table_state[suit][side] = val - side * MCARD;}
    void apply_move(std::array<int, 3> arr) { apply_move(arr[0], arr[1], arr[2]); }
    void apply_move(int suit, int card)
    {
        if (card < MCARD)
            apply_move(suit, 0, card);
        else
            apply_move(suit, 1, card);
    }
    std::string to_string() {
        std::ostringstream os;
        os << "{" << current_player << " ";
        for (int i = 0; i < SUIT_COUNT; i++)
            os << "(" << (int)table_state[i][0] << ", " << (int)table_state[i][1] << ") ";
        os << "} ";
        return os.str();
    }
    GameState()
    { 
        current_player = 1;
        for (int i = 0; i < SUIT_COUNT; i++)
        {
            table_state[i][0] = L_SIZE - 1;
            table_state[i][1] = R_SIZE;
        }
    }
};

int8_t& get_memory(const GameState& game_state);
Deck create_randomized_deck();
Deck inverse(Deck deck);
std::array<int, 4> calculate_best_move(GameState& game_state);
int find_winner(GameState& game_state);
int process_move(GameState& game_state, int suit, int side, int new_val);
bool player_has_available_moves(GameState& game_state);
bool is_card_valid_to_play(const GameState& game_state, int suit, int card);
bool opponent_has_remaining_cards(const GameState& game_state);
bool is_card_on_human_hand(const GameState& game_state, int suit, int card);
void calculate_player_bounds();
void calculate_player_bounds(int player);

const int COMPUTER_WINNING = 1;
const int HUMAN_WINNING    = 2;
const int UNPROCESSED      = 0;
const int COMPUTER         = 1;
const int HUMAN            = 2;
const int CLUBS            = 0; // The frontend may arbitarily assign suits to numbers, but 0 must be clubs (since player with 7 clubs starts the game)

int8_t memory[2][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1]; // (l, r) range, if r = R_SIZE, the set is not opened yet
Deck decks[3];
PlayerBound player_bounds[3];

GameState game_state;

extern "C"
{
    struct Triplet { int a, b, c; };
    struct Triplet get_best_move()
    { 
        std::array<int, 4> arr = calculate_best_move(game_state); 
        struct Triplet res;
        res.a = arr[0];
        res.b = arr[1];
        res.c = arr[2];
        return res;
    }
    void init();
    int human_deck(int suit, int card) { return decks[HUMAN][suit][card]; }
    int is_card_on_human_hand(int suit, int card) { return is_card_on_human_hand(game_state, suit, card); }
    int computer_deck(int suit, int card) { return decks[COMPUTER][suit][card]; }
    int player_has_available_moves() { return player_has_available_moves(game_state); }
    int is_card_valid_to_play(int suit, int card) { return is_card_valid_to_play(game_state, suit, card); }
    void apply_move(int suit, int card) { game_state.apply_move(suit, card); }
    int is_computer_winning() { return find_winner(game_state) == COMPUTER_WINNING; }
    int get_current_player() { return game_state.current_player; }
    int get_low_card(int suit) { return game_state.table_state[suit][0]; }
    int get_high_card(int suit) { return game_state.table_state[suit][1]+L_SIZE-1; }
    int opponent_has_remaining_cards() { return opponent_has_remaining_cards(game_state); }
    void switch_players() { game_state.switch_players(); }
    int find_winner() { return find_winner(game_state); }
}

void init()
{
    std::cout << "Development version\n";
    srand((unsigned) time(NULL));
    decks[COMPUTER] = create_randomized_deck();
    decks[HUMAN] = inverse(decks[COMPUTER]);
    memset(memory, 0, sizeof(memory));
    calculate_player_bounds();
    game_state = GameState();
    game_state.current_player = decks[COMPUTER][CLUBS][MCARD] ? COMPUTER : HUMAN;
    // This lines are just for testing that computer always wins given better hand
    if (find_winner(game_state) == HUMAN)
    {
        swap(decks[COMPUTER], decks[HUMAN]);
        memset(memory, 0, sizeof(memory));
        calculate_player_bounds();
        game_state.current_player = decks[COMPUTER][CLUBS][MCARD] ? COMPUTER : HUMAN;
    }
}

bool player_has_available_moves(GameState& game_state)
{
    return calculate_best_move(game_state)[0] != -1;
}

// Return value: (Suit, side, move) if there's a move (-1, -1, -1) if no move exists
std::array<int, 4> calculate_best_move(GameState& game_state)
{
    int player = game_state.current_player;
    if (!opponent_has_remaining_cards(game_state))
        return {-1, -1, -1, opponent(player)}; // Clearly defeated
    std::array<int, 4> best_move = {-1, -1, -1, -1};
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        if (!game_state.suit_opened(i) && decks[player][i][MCARD])
        {
            best_move = {i, 1, MCARD, process_move(game_state, i, 1, 0)};
            if (best_move[3] == player)
                return best_move;
        }
        if (!game_state.suit_opened(i))
            continue;
        if (game_state.get_left(i) >= 0 && decks[player][i][game_state.get_left(i)])
        {
            best_move = {i, 0, game_state.get_left(i), process_move(game_state, i, 0, game_state.table_state[i][0]-1)};
            if (best_move[3] == player)
                return best_move;
        }
        if (game_state.get_right(i) < SUIT_SIZE && decks[player][i][game_state.get_right(i)])
        {
            best_move = {i, 1, game_state.get_right(i), process_move(game_state, i, 1, game_state.table_state[i][1]+1)};
            if (best_move[3] == player)
                return best_move;
        }
    }
    if (best_move[0] == -1)
    {
        game_state.switch_players();
        best_move[3] = find_winner(game_state);
        game_state.switch_players();
    }
    return best_move;
}

// player is one or zero
int find_winner(GameState& game_state)
{
    if (static_cast<int>(get_memory(game_state)) == UNPROCESSED)
        get_memory(game_state) = (calculate_best_move(game_state))[3];
    assert(get_memory(game_state) == HUMAN || get_memory(game_state) == COMPUTER);
    return get_memory(game_state);
}

/*
int calculate_suit(GameState& game_state, int suit)
{
    int player = game_state.current_player;
    if (!game_state.suit_opened(suit) && decks[player][suit][MCARD])
        return process_move(game_state, suit, 1, 0);
    if (!game_state.suit_opened(suit))
        return UNPROCESSED;
    bool has_potential_moves = false;
    if (game_state.get_left(suit) >= 0 && decks[player][suit][game_state.get_left(suit)])
    {
        has_potential_moves = true;
        if (process_move(game_state, suit, 0, game_state.table_state[suit][0]-1) == player)
            return player;
    }
    if (game_state.get_right(suit) < SUIT_SIZE && decks[player][suit][game_state.get_right(suit)])
    {
        has_potential_moves = true;
        if (process_move(game_state, suit, 1, game_state.table_state[suit][1]+1) == player)
            return player;
    }
    if (has_potential_moves)
        return opponent(player);
    return UNPROCESSED;
}
*/

int process_move(GameState& game_state, int suit, int side, int new_val)
{
    int old_val = game_state.table_state[suit][side];
    game_state.table_state[suit][side] = new_val;
    game_state.switch_players();
    int res = find_winner(game_state);
    game_state.table_state[suit][side] = old_val;
    game_state.switch_players();
    return res;
}

bool is_card_valid_to_play(const GameState& game_state, int suit, int card)
{
    return
        0 <= suit && suit < SUIT_COUNT &&
        0 <= card && card < SUIT_SIZE && 
        decks[game_state.current_player][suit][card] && 
        ((!game_state.suit_opened(suit) && card == MCARD) || 
        (game_state.suit_opened(suit) && (card == game_state.get_left(suit) || card == game_state.get_right(suit)))); 
}

bool opponent_has_remaining_cards(const GameState& game_state)
{
    int opp = opponent(game_state.current_player);
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        if (!game_state.suit_opened(i) && decks[opp][i].count() > 0)
            return true;
        if (
            player_bounds[opp][i][0] < game_state.table_state[i][0] || 
            player_bounds[opp][i][1] > (MCARD + game_state.table_state[i][1]))
            return true;
    }
    return false;
}

bool is_card_on_human_hand(const GameState& game_state, int suit, int card)
{
    return human_deck(suit, card) && (!game_state.suit_opened(suit) || card <= game_state.get_left(suit) || card >= game_state.get_right(suit)); 
}


void calculate_player_bounds()
{
    calculate_player_bounds(COMPUTER);
    calculate_player_bounds(HUMAN);
}

void calculate_player_bounds(int player)
{
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        player_bounds[player][i][0] = SUIT_SIZE;
        player_bounds[player][i][1] = -1;
        for (int j = 0; j < SUIT_SIZE; j++)
        {
            if (!decks[player][i][j])
                continue;
            player_bounds[player][i][0] = std::min<int>(player_bounds[player][i][0], j);
            player_bounds[player][i][1] = std::max<int>(player_bounds[player][i][1], j);
        }
    }
}

Deck inverse(Deck deck)
{
    Deck ndeck;
    for (int i = 0; i < SUIT_COUNT; i++)
        ndeck[i] = ~deck[i];
    return ndeck;
}

int8_t& get_memory(const GameState& game_state)
{
    auto table_state = game_state.table_state;
    int player_flag = game_state.current_player-1;
    return memory[player_flag][table_state[0][0]][table_state[0][1]][table_state[1][0]][table_state[1][1]][table_state[2][0]][table_state[2][1]][table_state[3][0]][table_state[3][1]];
}

Deck create_randomized_deck()
{
    std::bitset<DECK_SIZE> deck_bits;
    int half_size = DECK_SIZE / 2; 
    for (int i = 0; i < half_size; i++)
        deck_bits[i] = 1;
    for (int i = 0; i < DECK_SIZE; i++)
    {
        int j = rand() % (i+1);
        bool tmp = deck_bits[i];
        deck_bits[i] = deck_bits[j];
        deck_bits[j] = tmp;
    }
    Deck new_deck;
    for (int i = 0; i < SUIT_COUNT; i++)
        for (int j = 0; j < SUIT_SIZE; j++)
            new_deck[i][j] = deck_bits[i*SUIT_SIZE + j];
    return new_deck;
}