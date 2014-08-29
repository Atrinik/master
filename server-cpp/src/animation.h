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
 * Animation.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <face.h>
#include <manager.h>

namespace atrinik {

class Animation {
public:
    typedef std::uint16_t AnimationId;
    typedef std::vector<Face::FaceId> AnimationFrames;

    static int uid;

    Animation(const std::string& name) : name_(name), id_(uid++)
    {
    }

    ~Animation()
    {
    }

    const std::string& name() const
    {
        return name_;
    }

    inline const AnimationId id() const
    {
        return id_;
    }

    inline const uint16_t facings() const
    {
        return facings_;
    }

    inline void facings(uint16_t facings)
    {
        facings_ = facings;
    }

    inline void push_back(AnimationFrames::value_type val)
    {
        frames.push_back(val);
    }

    inline void push_back(const std::string& val)
    {
        frames.push_back(FaceManager::get(val)->id());
    }

    AnimationFrames::value_type operator [](AnimationFrames::size_type i) const
    {
        return frames[i];
    }

    AnimationFrames::size_type size() const
    {
        return frames.size();
    }

private:
    AnimationFrames frames;

    AnimationId id_;

    uint16_t facings_ = 1;

    std::string name_;
};

typedef std::shared_ptr<Animation> AnimationPtr;
typedef std::shared_ptr<const Animation> AnimationPtrConst;

class AnimationManager : public Manager<AnimationManager> {
public:
    typedef std::vector<AnimationPtr> AnimationVector;
    typedef std::unordered_map<std::string, AnimationPtr> AnimationMap;

    static const char* path();
    static void load();
    static void add(AnimationPtr animation);
    static AnimationPtrConst get(const std::string& name);
    static AnimationPtrConst get(Animation::AnimationId id);
    static AnimationVector::size_type count();
    void clear();
private:
    AnimationVector animations_vector;
    AnimationMap animations_map;
};

};
