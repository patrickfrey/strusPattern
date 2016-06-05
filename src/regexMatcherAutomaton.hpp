/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\brief Definition of a regex matcher automaton
#ifndef _STRUS_REGEX_MATCHER_AUTOMATON_HPP_INCLUDED
#define _STRUS_REGEX_MATCHER_AUTOMATON_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "utils.hpp"
#include "podStructArrayBase.hpp"
#include "podStructTableBase.hpp"
#include "podStackPoolBase.hpp"
#include "errorUtils.hpp"
#include "internationalization.hpp"
#include <vector>
#include <string>
#include <stdexcept>


struct EventTrigger
{
	unsigned char loevent;
	unsigned char hievent;
	uint32_t stateIdx;
};

struct State
{
	uint32_t eventTriggerListIdx;
};

struct Program
{
	uint32_t stateArIdx;
};

