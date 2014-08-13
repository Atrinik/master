/*******************************************************************************
 *               Atrinik, a Multiplayer Online Role Playing Game               *
 *                                                                             *
 *       Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team       *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the Free  *
 * Software Foundation; either version 2 of the License, or (at your option)   *
 * any later version.                                                          *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
 * more details.                                                               *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program; if not, write to the Free Software Foundation, Inc.,     *
 * 675 Mass Ave, Cambridge, MA 02139, USA.                                     *
 *                                                                             *
 * The author can be reached at admin@atrinik.org                              *
 ******************************************************************************/

/**
 * @file
 * Message object type implementation.
 */

#include <boost/lexical_cast.hpp>

#include <msg_object_type.h>

using namespace atrinik;
using namespace boost;
using namespace std;

namespace atrinik {

REGISTER_GAME_OBJECT_TYPE(MsgObjectType);

bool MsgObjectType::load(const std::string& key, const std::string& val)
{
    if (key == "msg") {
        msg(val);
        return true;
    }

    return false;
}

std::string MsgObjectType::dump(const GameObjectType* base)
{
    auto base_type = dynamic_cast<const MsgObjectType*> (base);
    string s = "";

    if ((!base_type && !msg().empty()) ||
            (base_type && msg() != base_type->msg())) {
        s += "msg\n" + msg() + "\nendmsg\n";
    }

    return s;
}

}