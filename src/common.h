/*
 librange
 Copyright (C) 2011 Marco Leogrande
 
 This file is part of librange.
 
 librange is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 librange is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

/* == ancillary declarations == */
enum RangeOperator_t
  {
    LESS_THAN = 0,
    LESS_EQUAL_THAN,
    GREAT_THAN,
    GREAT_EQUAL_THAN,
    EQUAL,
    INVALID = -1
  };

#endif /* COMMON_H_INCLUDED */
