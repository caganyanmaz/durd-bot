import ctypes
lib = ctypes.cdll.LoadLibrary("./liba.so")
import flask
import logging

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
log = logging.getLogger('werkzeug')
log.disabled = True

app = flask.Flask(__name__)
@app.route("/")
def hello_world():
    return flask.render_template("mainpage.j2")

@app.route("/new-game")
def new_game():
    lib.init()
    return flask.redirect("/game")
    
@app.route("/game")
def game():
    if not lib.opponent_has_remaining_cards() and lib.get_current_player() == HUMAN:
        return flask.redirect("/computer-won")
    deck = []
    for suit in range(SUIT_COUNT):
        for card in range(SUIT_SIZE):
            if lib.is_card_on_human_hand(suit, card):
                deck.append((suit, card, get_card_file(card, suit)))
    table = []
    for suit in range(SUIT_COUNT):
        line = []
        low, high = lib.get_low_card(suit), lib.get_high_card(suit)
        if high == SUIT_SIZE:
            continue
        for card in range(low, high+1):
            line.append((suit, card, get_card_file(card, suit)))
        table.append(line)
    winner = lib.find_winner()
    return flask.render_template("game.j2", deck=deck, table=table, current_player=lib.get_current_player(), player_has_available_moves=lib.player_has_available_moves(), human_wins=(winner == HUMAN))

@app.route("/play")
def play():
    if lib.get_current_player() != HUMAN:
        return flask.redirect("/game")
    suit, card = flask.request.args.get("suit"), flask.request.args.get("card")
    if not (suit.isnumeric() and card.isnumeric()):
        return flask.redirect("/game")
    suit, card = int(suit), int(card)
    if not (0 <= suit < SUIT_COUNT and 0 <= card < SUIT_SIZE):
        return flask.redirect("/game")
    if not lib.is_card_valid_to_play(suit, card):
        return flask.redirect("/game")
    lib.apply_move(suit, card)
    lib.switch_players()
    return flask.redirect("/game")

@app.route("/advance-computer")
def advance_computer():
    if lib.get_current_player() == HUMAN and not lib.player_has_available_moves():
        lib.switch_players()
    if lib.get_current_player() == COMPUTER and not lib.opponent_has_remaining_cards():
        return flask.redirect("/human-won")
    if lib.get_current_player() == COMPUTER:
        progress_computer_turn()
        lib.switch_players()
    return flask.redirect("/game")

@app.route("/computer-won")
def computer_won():
    return flask.render_template("computer_won.j2")

@app.route("/human-won")
def human_won():
    return flask.render_template("human_won.j2")

def get_card_file(card, suit):
    return "/static/img/" + cards[card] + "_of_" + suits[suit] + ".png"

def is_card_valid(card, suit):
    return 0 <= card < SUIT_SIZE and 0 <= suit < SUIT_COUNT

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
        return
    lib.apply_move(res.a, res.c)
    

def print_deck(deck_type):
    for i in range(SUIT_COUNT):
        for j in range(SUIT_SIZE):
            print(1&deck_type(i, j), end="")
        print(" ", end="")
    print()

if __name__ == "__main__":
    app.run(debug=True)
