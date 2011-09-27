import math

_to_19 = ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"]
_tens = ["", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"]
_denom = ["", "thousand", "million", "billion", "trillion", "quadrillion", "quintillion", "sextillion", "septillion", "octillion", "nonillion", "decillion", "undecillion", "duodecillion", "tredecillion", "quattuordecillion", "sexdecillion", "septendecillion", "octodecillion", "novemdecillion", "vigintillion"]

def int2english(number):
	if number < 0:
		return "minus " + int2english(-number)

	unit = int(math.log(number, 1000))

	if unit:
		unit_amount, number = divmod(number, 1000 ** unit)
		res = (int2english(unit_amount) + " " + _denom[unit])

		if number:
			res += " and " + int2english(number) if number < 100 else " " + int2english(number)

		return res
	else:
		if number < 20:
			return _to_19[number]

		hundreds, under_100 = divmod(number, 100)
		ten, unit = divmod(under_100, 10)
		res = int2english(hundreds) + " hundred " if hundreds else ""

		if not under_100:
			return res

		res += "and " if hundreds and under_100 else ""
		res += _tens[ten] if ten > 1 else _to_19[under_100]
		res += "-" + _to_19[unit] if ten > 1 and unit else ""

		return res

_aberrant_plurals = {
	"appendix": "appendices",
	"barracks": "barracks",
	"child": "children",
	"criterion": "criteria",
	"deer": "deer",
	"echo": "echoes",
	"elf": "elves",
	"focus": "foci",
	"fungus": "fungi",
	"goose": "geese",
	"hero": "heroes",
	"hoof": "hooves",
	"index": "indices",
	"knife": "knives",
	"leaf": "leaves",
	"life": "lives",
	"man": "men",
	"mouse": "mice",
	"nucleus": "nuclei",
	"person": "people",
	"phenomenon": "phenomena",
	"potato": "potatoes",
	"self": "selves",
	"syllabus": "syllabi",
	"tomato": "tomatoes",
	"veto": "vetoes",
	"woman": "women",
}

_vowels = set("aeiou")

def pluralize(singular, n = 0, plural = None):
	if n == 1:
		return singular

	if plural:
		return plural

	plural = _aberrant_plurals.get(singular)

	if plural:
		return plural

	root = singular

	try:
		if singular[-1] == "y" and singular[-2] not in _vowels:
			root = singular[:-1]
			suffix = "ies"
		elif singular[-1] == "s":
			if singular[-2] in _vowels:
				if singular[-3:] == "ius":
					root = singular[:-2]
					suffix = "i"
				else:
					root = singular[:-1]
					suffix = "ses"
			else:
				suffix = "es"
		elif singular[-2:] in ("ch", "sh"):
			suffix = "es"
		else:
			suffix = "s"
	except IndexError:
		suffix = "s"

	plural = root + suffix
	return plural
