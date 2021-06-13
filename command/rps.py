from command import command


@command("rps")
def rps(state, u1, u2, sender, hand):
    if (u1, u2) in state:
        game = state[(u1, u2)]
        game[sender] = hand
        if game[0] is not None and game[1] is not None:
            return "Game finish!"
        else:
            return "Continue"
    else:
        game = [None, None]
        game[sender] = hand
        state[(u1, u2)] = game
        return "Continue"
