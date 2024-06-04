import ctypes
lib = ctypes.cdll.LoadLibrary("./liba.so")
L_SIZE     = 7
R_SIZE     = 7
SUIT_COUNT = 4 
MCARD      = L_SIZE - 1
SUIT_SIZE  = L_SIZE + R_SIZE - 1
DECK_SIZE = SUIT_SIZE * SUIT_COUNT

suits = ["clubs", "diamonds", "hearts", "spades"]
cards = ["Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"]

COMPUTER_WINNING = 1
HUMAN_WINNING    = 2
UNPROCESSED      = 0
COMPUTER         = 1
HUMAN            = 2
CLUBS            = 0 

class Triplet(ctypes.Structure):
    _fields_ = [("a", ctypes.c_int),
                ("b", ctypes.c_int),
                ("c", ctypes.c_int)]

lib.get_best_move.restype = Triplet

def main():
    lib.init()
    print("My deck:", end=" ")
    print_deck(lib.computer_deck)
    print("Your deck:", end=" ")
    print_deck(lib.human_deck)
    cnt = 0
    while 1&lib.opponent_has_remaining_cards():
        progress_turn()

def progress_turn():
    if lib.get_current_player() == COMPUTER:
        progress_computer_turn()
    else:
        progress_human_turn()
    lib.switch_players()

def progress_human_turn():
    if not lib.player_has_available_moves():
        print("You have no moves to play!")
        return
    while True:
        suit = ask_choice("suit", suits)
        card = ask_choice("card", cards)
        if lib.is_card_valid_to_play(suit, card):
            break
        print("You can't play that card, silly :)")
    lib.apply_move(suit, card)

def ask_choice(name, arr):
    print("Pick a", name + ":")
    for i, e in enumerate(arr):
        print("(" + str(i+1) + ")", arr[i])
    while True:
        res = input()
        if res.isnumeric() and 1 <= int(res) <= len(arr):
            break
        print("Invalid input, please enter a valid number:", end=" ")
    return int(res) - 1

def progress_computer_turn():
    res = lib.get_best_move()
    if res.a == -1:
        print("Durd!")
        return
    print("I play", cards[res.c], suits[res.a])
    lib.apply_move(res.a, res.c)
    

def print_deck(deck_type):
    for i in range(SUIT_COUNT):
        for j in range(SUIT_SIZE):
            print(1&deck_type(i, j), end="")
        print(" ", end="")
    print()

if __name__ == "__main__":
    main()
