/* Copyright (C) 2012 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#ifndef _WP7_INJECTOR_H_
#define _WP7_INJECTOR_H_

#include "Injector.h"
#include <string>

namespace MoSync {

/**
 * The WP7 icon injector.
 */
class WP7Injector : public Injector {
public:
	void inject(const Icon* icon, const std::map<std::string, std::string>& params);
};

}

#endif // _WP7_INJECTOR_H_
