## @file
## Implements the smug student witch's card game responses.

from Atrinik import *
from Interface import Interface

activator = WhoIsActivator()
me = WhoAmI()
msg = WhatIsMessage().lower().strip()

inf = Interface(activator, me)

def main():
	if msg == "hello" or msg == "hi" or msg == "hey":
		inf.add_msg("Heh.  This sucker keeps falling for the same trick.")
		inf.add_link("Are you cheating?")
	elif msg == "are you cheating?":
		inf.add_msg("It ain't cheating, I'm just playing creatively.  Haha.~")
	elif msg == "marisa" or msg == "kirisame" or msg == "marisa kirisame":
		inf.add_msg("Marisa's awesome isn't she?")

main()
inf.finish()
