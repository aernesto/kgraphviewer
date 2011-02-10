/* This file is part of KGraphViewer.
   Copyright (C) 2005-2007 Gael de Chalendar <kleag@free.fr>

   KGraphViewer is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA
*/

/* This file was callgraphview.h, part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.
*/

#ifndef KGRAPHVIZ_CANVAS_EDGE_H
#define KGRAPHVIZ_CANVAS_EDGE_H

#include "canvaselement.h"

namespace KGraphViz
{

class CanvasEdgePrivate;
class GraphEdge;

class CanvasEdge : public CanvasElement
{
public:
  explicit CanvasEdge(DotGraphView* v,
                      GraphEdge*,
                      QGraphicsScene* scene,
                      QGraphicsItem* parent = 0);

  virtual ~CanvasEdge();

  virtual QPainterPath shape() const;

  GraphEdge* edge() const;

protected:
  virtual void computeBoundingRect();

  CanvasEdgePrivate* const d_ptr;

private:
  Q_DECLARE_PRIVATE(CanvasEdge);
};

}

#endif
