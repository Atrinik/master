## @file
## Script to be executed when Hierro dies.

from Atrinik import *

me = WhoAmI()
activator = WhoIsActivator()

# X/Y locations of various objects to remove/adjust when Hierro dies.
locations = [
	# Torches to activate all at once.
	(8, 20), (8, 16), (8, 12), (8, 8), (8, 4), (15, 20), (15, 16), (15, 12), (15, 8), (15, 4),
	# Magic mouths to remove.
	(9, 5), (10, 5), (11, 5),
	# Door to unlock.
	(10, 4),
	# May as well remove the markers that were used to allow player
	# to open the above door.
	(9, 2), (10, 2), (11, 2), (12, 2), (10, 3), (11, 3),
]

if activator.owner:
	activator = activator.owner

me.map.Message(me.x, me.y, 20, "You hear a booming voice...", COLOR_GREEN)
me.map.Message(me.x, me.y, 20, "Noooooooooooooooo! Curse you {}, curse yoouuu!...".format(activator.race), COLOR_RED)

# Remove the spawn point... no more Hierro for this player.
me.FindObject(type = Type.SPAWN_POINT_INFO).owner.Remove()

for (x, y) in locations:
	for obj in me.map.GetFirstObject(x, y):
		# Remove markers, inventory checkers and signs (magic mouths).
		if obj.type in (Type.MARKER, Type.CHECK_INV, Type.SIGN):
			obj.Remove()
		# Unlock doors.
		elif obj.type == Type.DOOR:
			obj.slaying = None
		# Change torches back to normal and light them up.
		elif obj.name == "attached torch":
			obj.type = obj.arch.clone.type
			obj.msg = None
			obj.f_is_animated = True
			obj.direction = 7
			obj.f_is_turnable = True
			me.Apply(obj, APPLY_TOGGLE | APPLY_NO_EVENT)
