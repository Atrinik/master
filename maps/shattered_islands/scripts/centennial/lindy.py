## @file
## Implements Ruri, the fairy helper's dialog.

from Interface import Interface

msg = msg.lower()

inf = Interface(activator, me)

# used in 2 places.  Easier to add options this way.
def main_menu():
	inf.add_link("<a=:book>Find a book.</a>")	
	inf.add_link("<a=:ruri>Ask about Ruri.</a>")
	inf.add_link("<a=close:>Goodbye.</a>")
	

def main():
	if msg == "hello":
		inf.add_msg("Hi, there {}.  Welcome to the school library.  Please try to keep quiet so others can study in peace.".format(activator.name))
		inf.add_link("<a=:book>Find a book.</a>")
	elif msg == "main_menu":
		inf.add_msg("Was there anything else you wanted to talk about {}?".format(activator.name))
		main_menu()
	elif msg == "book":
		inf.add_msg("We have plenty of books.  Please feel free to ask Ruri if you need some assistance.")
		inf.add_link("<a=:ruri>Ask about Ruri.</a>")
		inf.add_link("<a=:eastern project>Ask about the Eastern Project series.</a>")
		inf.add_link("<a=:main_menu>Ask about something else.</a>")
	elif msg == "patchouli" or msg == "patchouli knowledge":
		inf.add_msg("Ah, I see you like the Eastern Project series, too.  It\'s fairly obvious why I should be a fan of her, don\'t you think?")
		inf.add_link("<a=:main_menu>Ask about something else.</a>")
	elif msg == "eastern project":
		inf.add_msg("This name is, of course, translated from its original language, but it basically means the same thing.  You should be able to find a copy in the library but it's very popular here so someone may be reading them right now.")
		inf.add_link("<a=:main_menu>Ask about something else.</a>")
	elif msg == "lindy harlaown" or msg == "Harlaown" or msg == "Nanoha" or msg == "lyrical":
		inf.add_msg("Excellent deduction.  Yes, my parents were, indeed, quite the fans of the Nanoha series.")
		inf.add_link("<a=:main_menu>Ask about something else.</a>")
	elif msg == "ruri":
		inf.add_msg("Don't mind her.  She knows a bit too much but she's a hard worker.  You wouldn't really expect most fairies to be that smart, though... it's kind of weird.  I wonder if maybe I should ask the summoning instructor to look into the spell I used to summon her.")
		inf.add_link("<a=:main_menu>Ask about something else.</a>")

main()
inf.finish()
