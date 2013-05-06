/*
 * enums.h
 * Copyright (c) 2006-2011, James Athey
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   In addition, as a special exception, the author gives permission to   *
 *   link the code of his release of Comical with Rarlabs' "unrar"         *
 *   library (or with modified versions of it that use the same license    *
 *   as the "unrar" library), and distribute the linked executables. You   *
 *   must obey the GNU General Public License in all respects for all of   *
 *   the code used other than "unrar". If you modify this file, you may    *
 *   extend this exception to your version of the file, but you are not    *
 *   obligated to do so. If you do not wish to do so, delete this          *
 *   exception statement from your version.                                *
 *                                                                         *
 ***************************************************************************/

#ifndef __enums_h__
#define __enums_h__

enum COMICAL_MODE {ONEPAGE, TWOPAGE, CONTINUOUS};
enum COMICAL_ZOOM {ZOOM_FIT, ZOOM_HEIGHT, ZOOM_WIDTH, ZOOM_FULL, ZOOM_CUSTOM};
enum COMICAL_ROTATE {NORTH, EAST, SOUTH, WEST};
enum COMICAL_DIRECTION {COMICAL_LTR, COMICAL_RTL};
enum COMICAL_PAGETYPE {EMPTY_PAGE, FULL_PAGE, LEFT_HALF, RIGHT_HALF};

#endif
