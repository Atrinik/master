"""Generates documentation for the Atrinik Python package.

The documentation is generated in the form of Python files that mimic the
C interface. Only the API is exposed, there is no implementation.

Documentation is collected from (and also dynamically generated ) docstrings
found in Atrinik classes, methods, etc.

The generated documentation is in Restructured Text (reST) format, and can be
found in /maps/python/Atrinik.

IDEs that support the reST format (such as PyCharm) can use this generated
Python interface to add type-hinting to functions and methods, and to even
resolve the Atrinik package definitions.
"""

import sys
import inspect
import os
import re
from collections import OrderedDict

from Atrinik import *

PATH = os.path.join(GetSettings()["mapspath"], "python", "Atrinik")


def getargspec(obj):
    if not obj.__doc__:
        return []

    match = re.search(r"[\w_]+\((.*)\)", obj.__doc__, re.M)
    if not match:
        print("Failed to get args for {}".format(obj))
        return []

    args = re.findall(r"([\w+_]+)(?:=(\w+))?", match.group(1))
    return ["=".join(x for x in val if x) for val in args]


def dump_docstring(obj, f, indent=0, cls_name=None, is_getter=False,
                   is_setter=False):
    doc = obj.__doc__
    if not doc:
        return

    ret = None

    if cls_name is not None:
        doc = ".. class:: {}\n\n".format(cls_name) + doc
    elif is_getter or is_setter:
        parts = doc.split(";")
        types = re.match(r"([^\(]+)\s*(\(.*\))?", parts[1])
        if not types:
            print("No types for {}".format(obj))
            return

        tmp_type = types.group(1).strip()
        extra = types.group(2) or ""
        doc = parts[0].strip() + " " + extra.strip()

        ret = []
        types = []
        for val in tmp_type.split(" "):
            if val != "or":
                ret.append(val.replace("Atrinik.", ""))
                types.append(":class:`{}`".format(val))

        if is_getter:
            doc += "\n\n:type: " + " or ".join(types)
        else:
            doc += "\n\n:param value: The value to set.\n"
            doc += ":type value: {}".format(" or ".join(ret))

    f.write(" " * indent * 4)
    f.write('"""\n')

    for line in doc.split("\n"):
        f.write(" " * indent * 4)
        f.write(line + "\n")

    f.write(" " * indent * 4)
    f.write('"""\n')

    return ret


def dump_obj(obj, f, indent=0, defaults=None):
    names = []
    l = dir(obj)
    if defaults is not None:
        l += list(defaults.keys())

    for tmp_name in l:
        if tmp_name.startswith("__") and tmp_name.endswith("__"):
            continue

        if tmp_name == "print":
            continue

        if hasattr(obj, tmp_name):
            tmp = getattr(obj, tmp_name)
        else:
            tmp = defaults[tmp_name]

        names.append(repr(tmp_name))

        if inspect.ismodule(tmp):
            with open(os.path.join(PATH, tmp_name + ".py"), "w") as f2:
                dump_obj(tmp, f2)
        elif inspect.isclass(tmp):
            f.write("\n\n")
            f.write(" " * indent * 4)
            f.write("class {}(object):\n".format(tmp_name))
            dump_docstring(tmp, f, indent + 1, cls_name=tmp_name)
            dump_obj(tmp, f, indent=1)
        elif hasattr(tmp, "__call__"):
            args = getargspec(tmp)
            if inspect.isclass(obj):
                args.insert(0, "self")
            else:
                f.write("\n")

            f.write("\n")
            f.write(" " * indent * 4)
            f.write("def {}({}):\n".format(tmp_name, ", ".join(args)))
            dump_docstring(tmp, f, indent + 1)
            f.write(" " * (indent + 1) * 4)
            f.write("pass\n")
        elif isinstance(tmp, (Object, Map, Archetype, Player)):
            f.write(" " * indent * 4)
            f.write("{} = {}()\n".format(tmp_name, tmp.__class__.__name__))
        elif inspect.isclass(obj):
            f.write("\n")
            f.write(" " * indent * 4)
            f.write("@property\n")
            f.write(" " * indent * 4)
            f.write("def {}(self):\n".format(tmp_name, tmp))
            types = dump_docstring(tmp, f, indent + 1, is_getter=True)
            f.write(" " * (indent + 1) * 4)
            f.write("value = getattr(self, {})\n".format(repr(tmp_name)))

            if types is not None:
                f.write(" " * (indent + 1) * 4)
                f.write("assert isinstance(value, ({},))\n".format(
                    ", ".join(types)))

            f.write(" " * (indent + 1) * 4)
            f.write("return value\n\n")
            f.write(" " * indent * 4)
            f.write("@{}.setter\n".format(tmp_name))
            f.write(" " * indent * 4)
            f.write("def {}(self, value):\n".format(tmp_name, tmp))
            dump_docstring(tmp, f, indent + 1, is_setter=True)
            f.write(" " * (indent + 1) * 4)
            f.write("setattr(self, {}, value)\n".format(repr(tmp_name)))
        else:
            f.write(" " * indent * 4)
            f.write("{} = {}\n".format(tmp_name, repr(tmp)))

    return names


def main():
    defaults = OrderedDict([
        ("activator", Object()),
        ("pl", Player()),
        ("me", Object()),
        ("msg", "hello"),
    ])

    if not os.path.exists(PATH):
        os.makedirs(PATH)

    with open(os.path.join(PATH, "__init__.py"), "w") as f:
        names = dump_obj(sys.modules["Atrinik"], f, defaults=defaults)
        f.write("__all__ = [{}]\n".format(", ".join(names)))

main()
