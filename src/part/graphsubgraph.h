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

/*
 * Subgraph model
 */

#ifndef GRAPH_SUBGRAPH_H
#define GRAPH_SUBGRAPH_H

#include <QMap>
#include <QTextStream>

#include "dotgrammar.h"
#include "graphelement.h"
#include "dotrenderop.h"

class CanvasSubgraph;

/**
 * Colors and styles are dot names
 */
class GraphSubgraph : public GraphElement
{
//   Q_OBJECT
public:
  GraphSubgraph();
  
  virtual ~GraphSubgraph() {}  
  
  void updateWithSubgraph(const GraphSubgraph& subgraph);

  CanvasSubgraph* canvasSubgraph() { return (CanvasSubgraph*)canvasElement();  }
  void setCanvasSubgraph(CanvasSubgraph* cs) { setCanvasElement((CanvasElement*)cs); }

  virtual QString backColor() const;

  inline const QList<GraphElement*>& content() const {return m_content;}
  inline QList<GraphElement*>& content() {return m_content;}
  inline void setContent(QList<GraphElement*>& c) {m_content=c;}

  void removeElement(GraphElement* element);


 private:
  QList<GraphElement*> m_content;
};

typedef QMap<QString, GraphSubgraph*> GraphSubgraphMap;

QTextStream& operator<<(QTextStream& stream, const GraphSubgraph& s);

#endif



