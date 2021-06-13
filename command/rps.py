from command import command

ROCK = [
    "   ______   ",
    "--'   ____)  ",
    "      (_____)",
    "  R   (_____)",
    "      (____) ",
    "--.__(___)   ",
]

PAPER = [
    "   _____",
    "--' ____)___",
    "        _____)",
    "  P    ______)",
    "      ______)",
    "--.______)",
]

SCISSORS = [
    "   _____",
    "--' ____)____",
    "       ______)",
    "  S _________)",
    "   (____)",
    "--.(___)",
]


def reverse_drawing(str):
    return str[::-1].replace("(", ")").replace(")", "(")


def lets_play():
    return [
        "                            ",
        "                            ",
        "  [Rock, Paper, Scissors!]  ",
        "                            ",
        "                            ",
        "                            ",
        "        Please Type         ",
        "        /rps [r|p|s]        ",
        "        To play RPS!        ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
    ]


def ch_to_drawing(ch):
    if ch == "r":
        return ROCK
    elif ch == "p":
        return PAPER
    else:
        return SCISSORS


def result(p1, p2, winner):
    template = [
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
        "         Winner is:         ",
        "                            ",
        "                            ",
        "                            ",
        "                            ",
    ]

    d1 = ch_to_drawing(p1)
    d2 = ch_to_drawing(p2)

    for i in range(6):
        margin = " " * (28 - len(d1[i]) - len(d2[i]))
        template[i] = d1[i] + margin + reverse_drawing(d2[i])

    template[10] = (
        winner.center(28) if winner is not None else "Both of you :)".center(28)
    )

    return template


@command("rps")
def rps(state, chat_id, me, op, hand):
    if chat_id in state:
        game = state[chat_id]
        game[me] = hand
        if me in game and op in game:
            if game[me] == game[op]:
                winner = None
            elif (
                (game[me] == "r" and game[op] == "s")
                or (game[me] == "p" and game[op] == "r")
                or (game[me] == "s" and game[op] == "p")
            ):
                winner = me
            else:
                winner = op

            del state[chat_id]
            return result(game[me], game[op], winner)
        else:
            return lets_play()
    else:
        game = {}
        game[me] = hand
        state[chat_id] = game
        return lets_play()
