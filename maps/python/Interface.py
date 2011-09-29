from Atrinik import SetReturnValue

class Interface:
	def __init__(self, activator, npc):
		self._msg = ""
		self._links = []
		self._text_input = None
		self._activator = activator
		self._npc = npc
		self._icon = npc.face[0][:-1] + "1"
		self._title = npc.name

	def add_msg(self, msg, color = None, newline = True):
		if newline and self._msg:
			self._msg += "\n\n"

		if color:
			self._msg += "<c=#" + color + ">"

		self._msg += msg

		if color:
			self._msg += "</c>"

	def add_msg_icon(self, icon, desc = "", fit = False):
		self._msg += "\n\n"
		self._msg += "<bar=#000000 52 52><border=#606060 52 52><x=1><y=1><icon="
		self._msg += icon
		self._msg += " 50 50"

		if fit:
			self._msg += " 1"

		self._msg += "><x=-1><y=-1>"
		self._msg += "<padding=60><hcenter=50>"
		self._msg += desc
		self._msg += "</hcenter></padding>"

	def add_msg_icon_object(self, obj):
		self.add_msg_icon(obj.face[0], obj.GetName())

	def add_link(self, link, action = "", dest = ""):
		if dest or action:
			self._links.append("<a=" + action + ":" + dest + ">" + link + "</a>")
		elif not link.startswith("<a"):
			self._links.append("<a>" + link + "</a>")
		else:
			self._links.append(link)

	def set_icon(self, icon):
		self._icon = icon

	def set_title(self, title):
		self._title = title

	def set_text_input(self, text = "", prepend = None):
		self._text_input = text
		self._text_input_prepend = prepend

	def add_objects(self, objs):
		if type(objs) != list:
			objs = [objs]

		for obj in objs:
			obj = obj.Clone()
			self.add_msg_icon_object(obj)
			obj.InsertInto(self._activator)

	def dialog_close(self):
		self._activator.Controller().SendPacket(39, "", None)

	def finish(self):
		if not self._msg:
			return

		pl = self._activator.Controller()
		SetReturnValue(1)

		# Construct the base data packet; contains the interface message,
		# the icon and the title.
		fmt = "BsBsBs"
		data = [0, self._msg, 2, self._icon, 3, self._title]

		# Add links to the data packet, if any.
		for link in self._links:
			fmt += "Bs"
			data += [1, link]

		# Add the text input, if any.
		if self._text_input != None:
			fmt += "Bs"
			data += [4, self._text_input]

			if self._text_input_prepend:
				fmt += "Bs"
				data += [5, self._text_input_prepend]

		# Send the data.
		pl.SendPacket(39, fmt, *data)

		# If there is any movement behavior, update the amount of time
		# the NPC should pause moving for.
		if self._npc.move_type or self._npc.f_random_move:
			from Atrinik import GetTicks, INTERFACE_TIMEOUT_CHARS, INTERFACE_TIMEOUT_SECONDS, INTERFACE_TIMEOUT_INITIAL, MAX_TIME

			timeout = self._npc.ReadKey("npc_move_timeout")
			ticks = GetTicks() + ((int(max(INTERFACE_TIMEOUT_CHARS, len(self._msg)) / INTERFACE_TIMEOUT_CHARS * INTERFACE_TIMEOUT_SECONDS)) - INTERFACE_TIMEOUT_SECONDS + INTERFACE_TIMEOUT_INITIAL) * (1000000 // MAX_TIME)

			if not timeout or ticks > int(timeout):
				self._npc.WriteKey("npc_move_timeout", str(ticks))
