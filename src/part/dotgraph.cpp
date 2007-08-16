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
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "dotgraph.h"
#include "dotgrammar.h"
#include "graphexporter.h"
#include "DotGraphParsingHelper.h"


// #include <iostream>
// #include <math.h>
#include "fdstream.hpp"
#include <boost/spirit/utility/confix.hpp>

#include <kdebug.h>
#include <QFile>
#include <QPair>
#include <QByteArray>
#include <QProcess>



using namespace boost;
using namespace boost::spirit;

extern DotGraphParsingHelper* phelper;

const distinct_parser<> keyword_p("0-9a-zA-Z_");

DotGraph::DotGraph(const QString& command, const QString& fileName) :
  GraphElement(),
  m_dotFileName(fileName),m_width(0.0), m_height(0.0),m_scale(1.0),
  m_directed(true),m_strict(false),
  m_layoutCommand(command),
  m_readWrite(false)
{ 
}

DotGraph::~DotGraph()  
{
  GraphNodeMap::iterator itn, itn_end;
  itn = m_nodesMap.begin(); itn_end = m_nodesMap.end();
  for (; itn != itn_end; itn++)
  {
    delete *itn;
  }

  GraphEdgeMap::iterator ite, ite_end;
  ite = m_edgesMap.begin(); ite_end = m_edgesMap.end();
  for (; ite != ite_end; ite++)
  {
    delete (*ite);
  }
}

QString DotGraph::chooseLayoutProgramForFile(const QString& str)
{
  if (m_layoutCommand == "")
  {
    QFile iFILE(str);
    
    if (!iFILE.open(QIODevice::ReadOnly))
    {
      kError() << "Can't test dot file. Will try to use the dot command on the file: '" << str << "'" << endl;
      return "dot";// -Txdot";
    }
    
    QByteArray fileContent = iFILE.readAll();
    if (fileContent.isEmpty()) return "";
    std::string s =  fileContent.data();
    std::string cmd = "dot";
    parse(s.c_str(),
          (
            !(keyword_p("strict")) >> (keyword_p("graph")[assign_a(cmd,"neato")])
          ), (space_p|comment_p("/*", "*/")) );
    
    m_layoutCommand = QString::fromStdString(cmd);// + " -Txdot" ;
  }
//   std::cerr << "File " << str << " will be layouted by '" << m_layoutCommand << "'" << std::endl;
  return m_layoutCommand;
}

bool DotGraph::parseDot(const QString& str)
{
  kDebug() << k_funcinfo << str;
  if (m_layoutCommand.isEmpty())
  {
    return false;
  }

  kDebug() << "Running " << m_layoutCommand  << str;
  QProcess dot;
  QStringList options;
  if (m_readWrite)
  {
    options << "-Tdot";
  }
  else
  {
    options << "-Txdot";
  }
  options << str;
  dot.start(m_layoutCommand, options);
  if (!dot.waitForFinished(-1))
  {
    kDebug() << "dot finished with error" << endl;
    return false;
  }
//   QFile dot(str);
//   if (!dot.open(QIODevice::ReadOnly | QIODevice::Text))
//   {
//     return false;
//   }
  QByteArray result = dot.readAll();
  result.replace("\\\n","");

  kDebug() << k_funcinfo << "string content is:" << endl << result << endl << "=====================";
  std::string s =  result.data();
  if (phelper != 0)
  {
    phelper->graph = 0;
    delete phelper;
  }
  phelper = new DotGraphParsingHelper;
  phelper->graph = this;
  phelper->z = 1;
  phelper->maxZ = 1;
  phelper->uniq = 0;
  
  bool parsingResult = parse(s);
  kDebug() << k_funcinfo << "parsed " << parsingResult;
  kDebug() << k_funcinfo << "width and height=" << m_width << m_height;
  if (parsingResult)
  {
    if (m_readWrite)
    {
      storeOriginalAttributes();
      update();
    }
    computeCells();
  }
  delete phelper;
  phelper = 0;
  kDebug() << k_funcinfo << "return parsing result";
  return parsingResult;
}

bool DotGraph::update()
{
  kDebug() << k_funcinfo;
  GraphExporter exporter;
  QString str = exporter.writeDot(this);

  kDebug() << k_funcinfo << "wrote to " << str;
  if (m_layoutCommand.isEmpty())
  {
    return false;
  }

  kDebug() << "Running" << m_layoutCommand << str;
  QProcess dot;
  dot.start(m_layoutCommand, QStringList() << "-Txdot" << str);
  if (!dot.waitForFinished(-1))
  {
    kError() << "dot finished with error";
    return false;
  }
  QByteArray result = dot.readAll();
  result.replace("\\\n","");
  std::string s(result.data());
  kDebug() << k_funcinfo << "string content is:" << endl
      << result << endl << "=====================";
  if (phelper != 0)
  {
    phelper->graph = 0;
    delete phelper;
  }

  DotGraph newGraph(m_layoutCommand, m_dotFileName);
  phelper = new DotGraphParsingHelper;
  phelper->graph = &newGraph;
  phelper->z = 1;
  phelper->maxZ = 1;
  phelper->uniq = 0;
  
  kDebug() << k_funcinfo << "parsing new dot";
  bool parsingResult = parse(s);
  if (parsingResult)
  {
    m_width=newGraph.width();
    m_height=newGraph.height();
    m_scale=newGraph.scale();
    m_directed=newGraph.directed();
    m_strict=newGraph.strict();
    computeCells();  
  }
  kDebug() << k_funcinfo << "parsing new dot done " << parsingResult;
  delete phelper;
  phelper = 0;
  kDebug() << k_funcinfo << "phelper deleted";
  if (parsingResult)
  {
    foreach (GraphNode* ngn, newGraph.nodes())
    {
      kDebug() << k_funcinfo << "node " << ngn->id();
      if (nodes().contains(ngn->id()))
      {
        kDebug() << k_funcinfo << "known";
        nodes()[ngn->id()]->updateWith(*ngn);
      }
      else
      {
        kDebug() << k_funcinfo << "new";
        nodes().insert(ngn->id(), new GraphNode(*ngn));
      }
    }
    foreach (GraphEdge* nge, newGraph.edges())
    {
      kDebug() << k_funcinfo << "an edge";
      QPair<GraphNode*,GraphNode*> pair(nodes()[nge->fromNode()->id()],nodes()[nge->toNode()->id()]);
      if (edges().contains(pair))
      {
        edges().value(pair)->updateWith(*nge);
      }
      else
      {
        edges().insert(pair, new GraphEdge(*nge));
      }
    }
  }
  kDebug() << k_funcinfo << "done";
  return parsingResult;
}

unsigned int DotGraph::cellNumber(int x, int y)
{
/*  kDebug() << "x= " << x << ", y= " << y << ", m_width= " << m_width << ", m_height= " << m_height << ", m_horizCellFactor= " << m_horizCellFactor << ", m_vertCellFactor= " << m_vertCellFactor  << ", m_wdhcf= " << m_wdhcf << ", m_hdvcf= " << m_hdvcf;*/
  
  unsigned int nx = (unsigned int)(( x - ( x % int(m_wdhcf) ) ) / m_wdhcf);
  unsigned int ny = (unsigned int)(( y - ( y % int(m_hdvcf) ) ) / m_hdvcf);
/*  kDebug() << "nx = " << (unsigned int)(( x - ( x % int(m_wdhcf) ) ) / m_wdhcf);
  kDebug() << "ny = " << (unsigned int)(( y - ( y % int(m_hdvcf) ) ) / m_hdvcf);
  kDebug() << "res = " << ny * m_horizCellFactor + nx;*/
  
  unsigned int res = ny * m_horizCellFactor + nx;
  return res;
}

#define MAXCELLWEIGHT 800

void DotGraph::computeCells()
{
  kDebug() << k_funcinfo << m_width << m_height << endl;
  m_horizCellFactor = m_vertCellFactor = 1;
  m_wdhcf = (int)ceil(((double)m_width) / m_horizCellFactor)+1;
  m_hdvcf = (int)ceil(((double)m_height) / m_vertCellFactor)+1;
  bool stop = true;
  do
  {
    stop = true;
    m_cells.clear();
//     m_cells.resize(m_horizCellFactor * m_vertCellFactor);
    
    GraphNodeMap::iterator it, it_end;
    it = m_nodesMap.begin(); it_end = m_nodesMap.end();
    for (; it != it_end; it++)
    {
      GraphNode* gn = *it;
      int cellNum = cellNumber(int(gn->x()), int(gn->y()));
      kDebug() << "Found cell number " << cellNum;

      if (m_cells.size() <= cellNum)
      {
        m_cells.resize(cellNum+1);
      }
      m_cells[cellNum].insert(gn);
      
      kDebug() << "after insert";
      if ( m_cells[cellNum].size() > MAXCELLWEIGHT )
      {
        kDebug() << "cell number " << cellNum  << " contains " << m_cells[cellNum].size() << " nodes";
        if ((m_width/m_horizCellFactor) > (m_height/m_vertCellFactor))
        {
          m_horizCellFactor++;
          m_wdhcf = m_width / m_horizCellFactor;
        }
        else
        {
          m_vertCellFactor++;
          m_hdvcf = m_height / m_vertCellFactor;
        }
        kDebug() << "cell factor is now " << m_horizCellFactor << " / " << m_vertCellFactor;
        stop = false;
        break;
      }
    }
  } while (!stop);
  kDebug() << k_funcinfo << "m_wdhcf=" << m_wdhcf << "; m_hdvcf=" << m_hdvcf << endl;
  kDebug() << k_funcinfo << "finished" << endl;
}

QSet< GraphNode* >& DotGraph::nodesOfCell(unsigned int id)
{
  return m_cells[id];
}

void DotGraph::storeOriginalAttributes()
{
  foreach (GraphNode* node, nodes().values())
  {
    node->storeOriginalAttributes();
  }
  foreach (GraphEdge* edge, edges().values())
  {
    edge->storeOriginalAttributes();
  }
  foreach (GraphSubgraph* subgraph, subgraphs().values())
  {
    subgraph->storeOriginalAttributes();
  }
  GraphElement::storeOriginalAttributes();
}

void DotGraph::saveTo(const QString& fileName)
{
  kDebug() << k_funcinfo << fileName;
  GraphExporter exporter;
  exporter.writeDot(this, fileName);
}
