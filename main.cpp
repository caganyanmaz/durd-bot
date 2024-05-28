#include <iostream>
#include <array>
#include <bitset>
#include <cassert>

#define DEBUGGING

const int L_SIZE     = 2; // How many cards are in the left  (including the middle), for development it's 7 (A, 2, 3,  4, 5, 6, 7)
const int R_SIZE     = 2; // How many cards are in the right (including the middle), for development it's 7 (7, 8, 9, 10, J, Q, K)
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
    void switch_players() { current_player = opponent(current_player); }
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
int8_t find_winner(GameState game_state);
int8_t calculate_winner(GameState game_state);
bool opponent_has_remaining_cards(const GameState& game_state);
void calculate_player_bounds();
void calculate_player_bounds(int player);

const int8_t FIRST_PLAYER_WINNING  = 1;
const int8_t SECOND_PLAYER_WINNING = 2;
const int8_t UNPROCESSED           = 0;

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

#endif

int main()
{
    // Getting the first player's deck
    decks[1] = { std::bitset<3>("111"), std::bitset<3>("111"), std::bitset<3>("111"), std::bitset<3>("010") };
    // Calculating the second player's deck
    decks[2] = inverse(decks[1]);
    assert(decks[1][0][L_SIZE-1]); // First player must have the starting card
    print_deck(decks[1]);
    print_deck(decks[2]);
    calculate_player_bounds();
    GameState game_state;
    find_winner(game_state);
    std::cout <<  (int)find_winner(GameState()) << "\n";
}

// player is one or zero
int8_t find_winner(GameState game_state)
{
    if (get_memory(game_state) == UNPROCESSED)
        get_memory(game_state) = calculate_winner(game_state);
    return get_memory(game_state);
}

int8_t calculate_winner(GameState game_state)
{
    int player = game_state.current_player;
    if (!opponent_has_remaining_cards(game_state))
        return opponent(player);
    bool has_potential_moves = false;
    for (int i = 0; i < SUIT_COUNT; i++)
    {
        if (!game_state.suit_opened(i) && decks[player][i][MCARD])
        {
            has_potential_moves = true;
            game_state.table_state[i][1] = 0;
            game_state.switch_players();
            int8_t res = find_winner(game_state);
            game_state.switch_players();
            game_state.table_state[i][1] = R_SIZE;
            if (res == player)
                return res;
        }
        if (!game_state.suit_opened(i))
            continue;
        int left = game_state.table_state[i][0]-1;
        if (left >= 0 && decks[player][i][left])
        {
            has_potential_moves = true;
            game_state.table_state[i][0]--;
            game_state.switch_players();
            int8_t res = find_winner(game_state);
            game_state.table_state[i][0]++;
            game_state.switch_players();
            if (res == player)
                return res;
        }
        int right = game_state.table_state[i][1]+L_SIZE;
        if (right < SUIT_SIZE && decks[player][i][right])
        {
            has_potential_moves = true;
            game_state.table_state[i][1]++;
            game_state.switch_players();
            int8_t res = find_winner(game_state);
            game_state.table_state[i][1]--;
            game_state.switch_players();
            if (res == player)
                return res;
        }
    }
    if (has_potential_moves)
        return opponent(player);
    game_state.switch_players();
    int8_t res = find_winner(game_state);
    game_state.switch_players();
    return res;
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