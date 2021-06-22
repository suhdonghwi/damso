import os
import pathlib
import pickle


def command(name):
    def command_deco(func):
        def wrapper(*args, **kwargs):
            filename = (
                str(pathlib.Path(__file__).parent.absolute())
                + "/state/"
                + name
                + ".pkl"
            )

            state = {}
            if os.path.exists(filename):
                with open(filename, "rb") as f:
                    state = pickle.load(f)

            ret = func(
                state,
                *args,
                **kwargs,
            )

            with open(filename, "wb+") as f:
                pickle.dump(state, f)

            return ret

        return wrapper

    return command_deco
