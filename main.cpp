#include <iostream>
#include <array>
#include <bitset>
#include <cassert>

#define DEBUGGING

const int L_SIZE     = 7; // How many cards are in the left  (including the middle), for development it's 7 (A, 2, 3,  4, 5, 6, 7)
const int R_SIZE     = 7; // How many cards are in the right (including the middle), for development it's 7 (7, 8, 9, 10, J, Q, K)
const int SUIT_COUNT = 4; // How many suits are in the game (always 4, don't change it for debugging)
const int MCARD      = L_SIZE-1;
const int SUIT_SIZE  = L_SIZE + R_SIZE - 1;

typedef std::array<std::bitset<SUIT_SIZE>, SUIT_COUNT> Deck;
typedef std::array<std::array<int8_t, 2>, SUIT_COUNT> PlayerBound;

inline int8_t opponent(int8_t player)
{
    return 3 - player;
}

struct GameState
{
    int8_t current_player;
    std::array<std::array<int8_t, 2>, SUIT_COUNT> table_state;
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
Deck inverse(Deck deck);
std::array<int, 3> get_best_move(GameState& game_state);
int8_t find_winner(GameState& game_state);
int8_t calculate_winner(GameState& game_state);
int8_t calculate_suit(GameState& game_state, int suit);
int8_t process_move(GameState& game_state, int suit, int side, int new_val);
bool player_has_available_moves(GameState& game_state);
bool is_card_valid_to_play(const GameState& game_state, int suit, int card);
bool opponent_has_remaining_cards(const GameState& game_state);
void calculate_player_bounds();
void calculate_player_bounds(int player);

const int8_t FIRST_PLAYER_WINNING  = 1;
const int8_t SECOND_PLAYER_WINNING = 2;
const int8_t UNPROCESSED           = 0;
const int8_t FIRST_PLAYER          = 1;
const int8_t SECOND_PLAYER         = 2;

int8_t memory[2][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1][L_SIZE][R_SIZE+1]; // (l, r) range, if r = R_SIZE, the set is not opened yet
Deck decks[3];
PlayerBound player_bounds[3];

#ifdef DEBUGGING

void print_deck(const Deck& deck)
{
    for (int i = 0; i < SUIT_COUNT; i++)
        std::cout << deck[i] << " ";
    std::cout << "\n"; 
}

void print_game_state(const GameState& game_state)
{
    std::cout << "------- \n";
    std::cout << "Current player: " << static_cast<int>(game_state.current_player) << "\n";
    
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        std::cout << static_cast<int>(game_state.table_state[i][0]) << " "<< static_cast<int>(game_state.table_state[i][1]) << "\n";
    }    
    std::cout << "------- \n";

}

int ask_choice(std::string q, std::string names[], int n)
{
    std::cout << "Pick " << q << ":\n";
    for (int i = 0; i < n; i++)
        std::cout << "(" << (i+1) << ") " << names[i] << "\n";
    int res;
    while (true)
    {
        std::cin >> res;
        res--;
        if (0 <= res && res < n)
            break;
        std::cout << "\nInvalid query, please enter a valid number: ";
    }
    return res;
}

#endif

std::string suits[SUIT_COUNT] =  {"clubs", "diamonds", "hearts", "spades"};
std::string cards[SUIT_SIZE] = {"Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};

template <std::size_t N>
void reverse(std::bitset<N>& b)
{
    for (int i = 0; i < N/2; i++)
    {
        bool tmp = b[i];
        b[i] = b[N-i-1];
        b[N-i-1] = tmp;
    }
}

int main()
{
    // Getting the first player's deck
    decks[1] = { 
    std::bitset<SUIT_SIZE>("0111011011011"),  // clubs
    std::bitset<SUIT_SIZE>("1011000000101"), // diamonds
    std::bitset<SUIT_SIZE>("1000100010001"), // hearts
    std::bitset<SUIT_SIZE>("0110100011011") }; // spades
    for (int i = 0; i < SUIT_COUNT; i++)
        reverse(decks[1][i]);
    // Calculating the second player's deck
    decks[2] = inverse(decks[1]);
    assert(decks[1][0][MCARD]); // First player must have the starting card
    std::cout << "My deck: ";
    print_deck(decks[1]);
    std::cout << "Your deck: ";
    print_deck(decks[2]);
    calculate_player_bounds();
    GameState game_state;
    bool has_won = false;
    if (find_winner(game_state) == FIRST_PLAYER_WINNING)
    {
        std::cout << "I'll definitely win\n";
        has_won = true;
    }
    else
    {
        std::cout << "I might lose if you play well.\n";
    }
    while (opponent_has_remaining_cards(game_state))
    {
        if (game_state.current_player == FIRST_PLAYER)
        {
            if (!has_won && find_winner(game_state) == FIRST_PLAYER_WINNING)
            {
                std::cout << "You lost your chance, I'll definitely win :)\n";
                has_won = true;
            }
            std::array<int, 3> ai_move = get_best_move(game_state);
            if (ai_move[0] == -1)
            {
                std::cout << "durd!\n";
            }
            else
            {
                std::cout << "I play " << cards[ai_move[2]] << " " << suits[ai_move[0]] << "\n";
                game_state.apply_move(ai_move);
            }
        }
        else if (!player_has_available_moves(game_state))
        {
            std::cout << "You've no moves to play!\n";
        }
        else
        {
            int suit, card;
            while (true)
            {
                suit = ask_choice("suit", suits, SUIT_COUNT);
                card = ask_choice("card", cards, SUIT_SIZE);
                if (is_card_valid_to_play(game_state, suit, card))
                    break;
                std::cout << "You can't play that card silly :)\n";
            }
            game_state.apply_move(suit, card);
        }
        game_state.switch_players();
    }
    if (opponent(game_state.current_player) == FIRST_PLAYER_WINNING)
        std::cout << "I won!\n";
    else
        std::cout << "I lost!\n";
}

// player is one or zero
int8_t find_winner(GameState& game_state)
{
    if (get_memory(game_state) == UNPROCESSED)
        get_memory(game_state) = calculate_winner(game_state);
    return get_memory(game_state);
}

bool player_has_available_moves(GameState& game_state)
{
    return get_best_move(game_state)[0] != -1;
}

// Return value: (Suit, side, move) if there's a move (-1, -1, -1) if no move exists
std::array<int, 3> get_best_move(GameState& game_state)
{
    int player = game_state.current_player;
    std::array<int, 3> best_move = {-1, -1, -1};
    if (!opponent_has_remaining_cards(game_state))
        return best_move; // Clearly defeated
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        if (!game_state.suit_opened(i) && decks[player][i][MCARD])
        {
            best_move = {i, 1, MCARD};
            if (process_move(game_state, i, 1, 0) == player)
                return best_move;
        }
        if (!game_state.suit_opened(i))
            continue;
        if (game_state.get_left(i) >= 0 && decks[player][i][game_state.get_left(i)])
        {
            best_move = {i, 0, game_state.get_left(i)};
            if (process_move(game_state, i, 0, game_state.table_state[i][0]-1) == player)
                return best_move;
        }
        if (game_state.get_right(i) < SUIT_SIZE && decks[player][i][game_state.get_right(i)])
        {
            best_move = {i, 1, game_state.get_right(i)};
            if (process_move(game_state, i, 1, game_state.table_state[i][0]+1) == player)
                return best_move;
        }
    }
    return best_move;
}

int8_t calculate_winner(GameState& game_state)
{
    int player = game_state.current_player;
    if (!opponent_has_remaining_cards(game_state))
        return opponent(player);
    bool has_potential_moves = false;
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        uint8_t res = calculate_suit(game_state, i);
        if (res == player)
            return player;
        has_potential_moves = has_potential_moves || (res == opponent(player));
    }
    if (has_potential_moves)
        return opponent(player);
    game_state.switch_players();
    int8_t res = find_winner(game_state);
    game_state.switch_players();
    return res;
}

int8_t calculate_suit(GameState& game_state, int suit)
{
    int player = game_state.current_player;
    if (!game_state.suit_opened(suit) && decks[player][suit][MCARD])
    {
        return process_move(game_state, suit, 1, 0);
    }
    bool has_potential_moves = false;
    if (!game_state.suit_opened(suit))
        return UNPROCESSED;
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

int8_t process_move(GameState& game_state, int suit, int side, int new_val)
{
    int old_val = game_state.table_state[suit][side];
    game_state.table_state[suit][side] = new_val;
    game_state.switch_players();
    int8_t res = find_winner(game_state);
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


void calculate_player_bounds()
{
    calculate_player_bounds(1); // player 1
    calculate_player_bounds(2); // player 2
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
    int8_t player_flag = game_state.current_player-1;
    return memory[player_flag][table_state[0][0]][table_state[0][1]][table_state[1][0]][table_state[1][1]][table_state[2][0]][table_state[2][1]][table_state[3][0]][table_state[3][1]];
}