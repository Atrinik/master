## @file
## Script for Terria temple priests.

from Atrinik import *
import Temple

activator = WhoIsActivator()
me = WhoAmI()
msg = WhatIsMessage().strip().lower()

def main():
	Temple.handle_temple(Temple.TempleTerria, me, activator, msg)

main()
