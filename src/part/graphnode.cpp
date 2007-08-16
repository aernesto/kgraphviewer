/* This file is part of KGraphViewer.
   Copyright (C) 2005 Gaël de Chalendar <kleag@free.fr>

   KGraphViewer is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* This file was callgraphview.h, part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.
*/


/*
 * Graph Node
 */

#include <math.h>

#include <kdebug.h>
#include <kconfig.h>

#include "dotgraphview.h"
#include "pannerview.h"
#include "canvasnode.h"
#include "graphnode.h"
#include "dotdefaults.h"

//
// GraphNode
//

GraphNode::GraphNode() :
    GraphElement(),
    m_cn(0), m_visible(false),
    m_x(0), m_y(0), m_w(0), m_h(0)
{
}

void GraphNode::updateWith(const GraphNode& node)
{
  kDebug() << k_funcinfo;
  if (m_cn)
  {
    m_cn->modelChanged();
  }
  GraphElement::updateWith(node);
  kDebug() << k_funcinfo << "done";
}

QTextStream& operator<<(QTextStream& s, const GraphNode& n)
{
  s << n.id() << "  ["
    << dynamic_cast<const GraphElement&>(n)
    <<"];"<<endl;
  return s;
}
