import sys

from rps import rps

(u1, u2, sender, command, *args) = sys.argv[1:]
sender = int(sender)

print(u1, u2, sender, command, args)

commands = {"rps": rps}

print(commands[command](u1, u2, sender, *args))
