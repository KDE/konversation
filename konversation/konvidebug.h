/*************************************************************************
 * Copyright (C) 2004 by Eli MacKenzie                                   *
 * argonel@sympatico.ca                                                  *
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the                         *
 * Free Software Foundation, Inc.,                                       *
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
**************************************************************************/

#include <kdebug.h>

#define KX kdDebug()
#define KY kndDebug()

#define _S(x) #x << ": " << (x) << ", "
#define _T(x) (x) << ", "

#define LOON __LINE__ << ' '
#define FLOON __FILE__ << ' ' << __LINE__ << ' '

#define SHOW KX << FLOON << endl;
