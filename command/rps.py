from command import command

S_FINISH = [
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "        Game Finish         ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
]


@command("rps")
def rps(state, chat_id, me, op, hand):
    if chat_id in state:
        game = state[chat_id]
        game[me] = hand
        if me in game and op in game:
            return S_FINISH
        else:
            return S_FINISH
    else:
        game = {}
        game[me] = hand
        state[chat_id] = game
        return S_FINISH
