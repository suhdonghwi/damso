import sys

from rps import rps

(uid1, uid2, me, op, command, *args) = sys.argv[1:]
uid1 = int(uid1)
uid2 = int(uid2)


def pair(x, y):
    if x < y:
        return x * (y - 1) + (y - x - 2) ** 2 / 4
    else:
        return (x - 1) * y + (x - y - 2) ** 2 / 4


def check_args():
    if command == "rps" and len(args) == 1 and args[0] in ["r", "p", "s"]:
        return
    else:
        message = [
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "       Invalid Command      ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
            "                            ",
        ]
        print("\n".join(message))
        exit()


check_args()

commands = {"rps": rps}
print("\n".join(commands[command](pair(uid1, uid2), me, op, *args)))
