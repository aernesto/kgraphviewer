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


#include "kgrapheditor.h"
#include "kgrapheditorConfigDialog.h"
#include "kgrapheditorsettings.h"
#include "KGraphEditorNodesTreeWidget.h"
#include "KGraphEditorElementTreeWidget.h"
#include "ui_preferencesReload.h"
#include "ui_preferencesOpenInExistingWindow.h"
#include "ui_preferencesReopenPreviouslyOpenedFiles.h"
#include "part/dotgraph.h"
#include "part/kgraphviewer_part.h"

#include <kshortcutsdialog.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kurl.h>
#include <ktabwidget.h>
#include <kparts/partmanager.h>
#include <kedittoolbar.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kconfigdialog.h>
#include <kiconloader.h>
#include <krecentfilesaction.h>
#include <ktoolbar.h>
#include <KActionCollection>

#include <QtDBus/QtDBus>
#include <QDockWidget>
#include <QTreeWidget>

#include <iostream>

KGraphEditor::KGraphEditor() :
    KParts::MainWindow(),
    m_rfa(0),
    m_currentPart(0)
{
  // set the shell's ui resource file
  setXMLFile("kgrapheditorui.rc");

  m_widget = new KTabWidget(this);
  m_widget->setHoverCloseButton(true);
  connect(m_widget, SIGNAL(closeRequest(QWidget*)), this, SLOT(close(QWidget*)));
  connect(m_widget, SIGNAL(currentChanged(QWidget*)), this, SLOT(newTabSelectedSlot(QWidget*)));
  
  setCentralWidget(m_widget);

  QDockWidget* topLeftDockWidget = new QDockWidget(this);
  m_treeWidget = new KGraphEditorNodesTreeWidget(topLeftDockWidget);
  connect(m_treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this,SLOT(slotItemChanged(QTreeWidgetItem*,int)));
  connect(m_treeWidget,SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this,SLOT(slotItemClicked(QTreeWidgetItem*,int)));
  connect(m_treeWidget, SIGNAL(removeNode(const QString&)),
          this, SLOT(slotRemoveNode(const QString&)));
  connect(m_treeWidget, SIGNAL(addAttribute(const QString&)),
          this, SLOT(slotAddAttribute(const QString&)));
  connect(m_treeWidget, SIGNAL(removeAttribute(const QString&,const QString&)),
          this, SLOT(slotRemoveAttribute(const QString&,const QString&)));

//   m_treeWidget->setItemDelegate(new VariantDelegate(m_treeWidget));
  m_treeWidget->setColumnCount(2);
  topLeftDockWidget->setWidget(m_treeWidget);
  addDockWidget ( Qt::LeftDockWidgetArea, topLeftDockWidget );

  QDockWidget* bottomLeftDockWidget = new QDockWidget(this);
  m_newElementAttributesWidget = new KGraphEditorElementTreeWidget(bottomLeftDockWidget);
  connect(m_newElementAttributesWidget,SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this,SLOT(slotNewElementItemChanged(QTreeWidgetItem*,int)));
  connect(m_newElementAttributesWidget, SIGNAL(addAttribute()),
          this, SLOT(slotAddNewElementAttribute()));
  connect(m_newElementAttributesWidget, SIGNAL(removeAttribute(const QString&)),
          this, SLOT(slotRemoveNewElementAttribute(const QString&)));
  m_newElementAttributesWidget->setColumnCount(2);
  bottomLeftDockWidget->setWidget(m_newElementAttributesWidget);
  addDockWidget ( Qt::LeftDockWidgetArea, bottomLeftDockWidget );


  if (QDBusConnection::sessionBus().registerService( "org.kde.kgrapheditor" ))
  {
    kDebug() << "Service Registered successfuly";
    QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAllSlots);
    
  }
  else
  {
    kDebug() << "Failed to register service...";
  }

  // then, setup our actions
  setupActions();

  // and a status bar
  statusBar()->show();
  createStandardStatusBarAction();

  // this routine will find and load our Part.  it finds the Part by
  // name which is a bad idea usually.. but it's alright in this
  // case since our Part is made for this Shell

 // Create a KParts part manager, to handle part activation/deactivation
  m_manager = new KParts::PartManager( this );
  
  // When the manager says the active part changes, the window updates (recreates) the GUI
  connect( m_manager, SIGNAL( activePartChanged( KParts::Part * ) ),
           this, SLOT( createGUI( KParts::Part * ) ) );
    
  // Creates the GUI with a null part to make appear the main app menus and tools
  createGUI(0);
  setupGUI();  
  // apply the saved mainwindow settings, if any, and ask the mainwindow
  // to automatically save settings if changed: window size, toolbar
  // position, icon size, etc.
  setAutoSaveSettings();

  connect( m_manager, SIGNAL( activePartChanged( KParts::Part * ) ),
           this, SLOT( slotSetActiveGraph( KParts::Part * ) ) );
}

KGraphEditor::~KGraphEditor()
{
  kDebug() ;
}

void KGraphEditor::reloadPreviousFiles()
{
  QStringList previouslyOpenedFiles = KGraphEditorSettings::previouslyOpenedFiles();
  if ( (previouslyOpenedFiles.empty() == false) 
       && (KMessageBox::questionYesNo(this, 
              i18n("Do you want to reload files from previous session?"),
              i18n("Reload Confirmation"),
              KStandardGuiItem::yes(),
              KStandardGuiItem::no(),
              "reopenPreviouslyOpenedFilesMode"   ) == KMessageBox::Yes) )
  {
    QStringList::const_iterator it, it_end;
    it = previouslyOpenedFiles.constBegin(); it_end = previouslyOpenedFiles.constEnd();
    for (; it != it_end; it++)
    {
      openUrl(*it);
    }
  }
  
}

void KGraphEditor::openUrl(const KUrl& url)
{
  kDebug() << url;
  KLibFactory *factory = KLibLoader::self()->factory("kgraphviewerpart");
  if (factory)
  {
    KParts::ReadOnlyPart* part = static_cast<KParts::ReadOnlyPart*>(factory->create(this, "kgraphviewerpart"));
    
    if (part)
    {
      connect(this,SIGNAL(setReadWrite()),part,SLOT(setReadWrite()));
      emit(setReadWrite());
      
      QString label = url.url().section('/',-1,-1);
      m_widget-> insertTab(part->widget(), QIcon( DesktopIcon("kgraphviewer") ), label);
      m_widget->setCurrentPage(m_widget->indexOf(part->widget()));
      createGUI(part);
      part->openUrl( url );
      m_openedFiles.push_back(url.url());
      m_manager->addPart( part, true );
      m_tabsPartsMap[m_widget->currentPage()] = part;
      m_tabsFilesMap[m_widget->currentPage()] = url.url();
      connect(this,SIGNAL(hide(KParts::Part*)),part,SLOT(slotHide(KParts::Part*)));

    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, i18n("Could not find the KGraphViewer part."));
    kapp->quit();
    // we return here, cause kapp->quit() only means "exit the
    // next time we enter the event loop...
    return;
  }
}

void KGraphEditor::fileOpen()
{
  kDebug() ;
  // this slot is called whenever the File->Open menu is selected,
  // the Open shortcut is pressed (usually CTRL+O) or the Open toolbar
  // button is clicked
  QStringList file_names = KFileDialog::getOpenFileNames(KUrl(QString()), QString("*.dot"), 0, QString::null);
  
  if (!file_names.empty())
  {
    foreach (const QString &fileName, file_names)
    {
      if (m_rfa != 0)
      {
        m_rfa->addUrl(fileName);
      }
      openUrl(fileName);
    }
  }
}


void KGraphEditor::setupActions()
{
  // create our actions

  actionCollection()->addAction( KStandardAction::New, "file_new", this, SLOT( fileNew() ) );
  actionCollection()->addAction( KStandardAction::Open, "file_open", this, SLOT( fileOpen() ) );
  m_rfa = (KRecentFilesAction*) actionCollection()->addAction(KStandardAction::OpenRecent, "file_open_recent", this, SLOT( slotURLSelected(const KUrl&) ) );
  m_rfa->loadEntries(KGlobal::config()->group("kgrapheditor"));
  actionCollection()->addAction( KStandardAction::Save, "file_save", this, SLOT( fileSave() ) );
  actionCollection()->addAction( KStandardAction::SaveAs, "file_save_as", this, SLOT( fileSaveAs() ) );

  actionCollection()->addAction( KStandardAction::Quit, "file_quit", this, SLOT( quit() ) );

  m_statusbarAction = KStandardAction::showStatusbar(this, SLOT(optionsShowStatusbar()), this);

  actionCollection()->addAction( KStandardAction::KeyBindings, "options_configure_keybinding", this, SLOT( optionsConfigureKeys() ) );
//   KStandardAction::keyBindings(this, SLOT(optionsConfigureKeys()), this);
  actionCollection()->addAction( KStandardAction::ConfigureToolbars, "options_configure_toolbars", this, SLOT( optionsConfigureToolbars() ) );
  actionCollection()->addAction( KStandardAction::Preferences, "options_configure", this, SLOT( optionsConfigure() ) );

  QAction* edit_new_vertex = actionCollection()->addAction( "edit_new_vertex" );
  edit_new_vertex->setText(i18n("Create a New Vertex"));
  edit_new_vertex->setIcon(QPixmap(KGlobal::dirs()->findResource("data","kgraphviewerpart/pics/kgraphviewer-newnode.png")));
  connect( edit_new_vertex, SIGNAL(triggered(bool)), this, SLOT( slotEditNewVertex() ) );

  QAction* edit_new_edge = actionCollection()->addAction( "edit_new_edge" );
  edit_new_edge->setText(i18n("Create a New Edge"));
  edit_new_edge->setIcon(QPixmap(KGlobal::dirs()->findResource("data","kgraphviewerpart/pics/kgraphviewer-newedge.png")));
  connect( edit_new_edge, SIGNAL(triggered(bool)), this, SLOT( slotEditNewEdge() ) );
}

bool KGraphEditor::queryExit()
{
  kDebug() ;
  KGraphEditorSettings::setPreviouslyOpenedFiles(m_openedFiles);
  m_rfa->saveEntries(KGlobal::config()->group("kgrapheditor"));

  KGraphEditorSettings::self()->writeConfig();
  return true;
}

void KGraphEditor::fileNew()
{
  // this slot is called whenever the File->New menu is selected,
  // the New shortcut is pressed (usually CTRL+N) or the New toolbar
  // button is clicked

  (new KGraphEditor)->show();
}


void KGraphEditor::optionsShowToolbar()
{
  // this is all very cut and paste code for showing/hiding the
  // toolbar
  if (m_toolbarAction->isChecked())
      toolBar()->show();
  else
      toolBar()->hide();
}

void KGraphEditor::optionsShowStatusbar()
{
  // this is all very cut and paste code for showing/hiding the
  // statusbar
  if (m_statusbarAction->isChecked())
      statusBar()->show();
  else
      statusBar()->hide();
}

void KGraphEditor::optionsConfigureKeys()
{
  KShortcutsDialog::configure(actionCollection());
}

void KGraphEditor::optionsConfigureToolbars()
{
    saveMainWindowSettings(KGlobal::config()->group("kgrapheditor") );

  // use the standard toolbar editor
  KEditToolBar dlg(factory());
  connect(&dlg, SIGNAL(newToolbarConfig()),
          this, SLOT(applyNewToolbarConfig()));
  dlg.exec();
}

void KGraphEditor::optionsConfigure()
{
  //An instance of your dialog could be already created and could be cached, 
  //in which case you want to display the cached dialog instead of creating 
  //another one 
  if ( KgeConfigurationDialog::showDialog( "settings" ) )
    return; 
 
  //KConfigDialog didn't find an instance of this dialog, so lets create it : 
  KPageDialog::FaceType ft = KPageDialog::Auto;
  KgeConfigurationDialog* dialog = new KgeConfigurationDialog( this, "settings",
                                             KGraphEditorSettings::self(),ft );
  
/*  KGraphViewerPreferencesReloadWidget*  reloadWidget =  
      new KGraphViewerPreferencesReloadWidget( 0, "KGraphViewer Settings" );
  if (KGraphEditorSettings::reloadOnChangeMode() == "yes")
  {
    reloadWidget->reloadOnChangeMode->setButton(0);
  }
  else if (KGraphEditorSettings::reloadOnChangeMode() == "no")
  {
    reloadWidget->reloadOnChangeMode->setButton(1);
  }
  else if (KGraphEditorSettings::reloadOnChangeMode() == "ask")
  {
    reloadWidget->reloadOnChangeMode->setButton(2);
  }
  
  connect((QObject*)reloadWidget->reloadOnChangeMode, SIGNAL(clicked(int)), this, SLOT(reloadOnChangeMode_pressed(int)) );
  
  dialog->addPage( reloadWidget, i18n("Reloading"), "kgraphreloadoptions", i18n("Reloading"), true); 
 
  KGraphViewerPreferencesOpenInExistingWindowWidget*  openingWidget =  
    new KGraphViewerPreferencesOpenInExistingWindowWidget( 0, "KGraphViewer Settings" );
  if (KGraphEditorSettings::openInExistingWindowMode() == "yes")
  {
    openingWidget->openInExistingWindowMode->setButton(0);
  }
  else if (KGraphEditorSettings::openInExistingWindowMode() == "no")
  {
    openingWidget->openInExistingWindowMode->setButton(1);
  }
  else if (KGraphEditorSettings::openInExistingWindowMode() == "ask")
  {
    openingWidget->openInExistingWindowMode->setButton(2);
  }
  
  connect((QObject*)openingWidget->openInExistingWindowMode, SIGNAL(clicked(int)), this, SLOT(openInExistingWindowMode_pressed(int)) );
  
  dialog->addPage( openingWidget, i18n("Opening"), "kgraphopeningoptions", i18n("Opening"), true); 
  
  KGraphViewerPreferencesReopenPreviouslyOpenedFilesWidget*  reopeningWidget =  
    new KGraphViewerPreferencesReopenPreviouslyOpenedFilesWidget( 0, "KGraphViewer Settings" );
  if (KGraphEditorSettings::reopenPreviouslyOpenedFilesMode() == "yes")
  {
    reopeningWidget->reopenPreviouslyOpenedFilesMode->setButton(0);
  }
  else if (KGraphEditorSettings::reopenPreviouslyOpenedFilesMode() == "no")
  {
    reopeningWidget->reopenPreviouslyOpenedFilesMode->setButton(1);
  }
  else if (KGraphEditorSettings::reopenPreviouslyOpenedFilesMode() == "ask")
  {
    reopeningWidget->reopenPreviouslyOpenedFilesMode->setButton(2);
  }
  
  connect((QObject*)reopeningWidget->reopenPreviouslyOpenedFilesMode, SIGNAL(clicked(int)), this, SLOT(reopenPreviouslyOpenedFilesMode_pressed(int)) );

  dialog->addPage( reopeningWidget, i18n("Session Management"), "kgraphreopeningoptions", i18n("Session Management"), true); 
  */
//   connect(dialog, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

  dialog->show();
}

void KGraphEditor::applyNewToolbarConfig()
{
  applyMainWindowSettings(KGlobal::config()->group("kgrapheditor"));
}

// void KGraphViewer::reloadOnChangeMode_pressed(int value)
// {
//   kDebug() << "reloadOnChangeMode_pressed " << value;
//   switch (value)
//   {
//   case 0:
//     KGraphEditorSettings::setReloadOnChangeMode("yes");
//     break;
//   case 1:
//     KGraphEditorSettings::setReloadOnChangeMode("no");
//     break;
//   case 2:
//     KGraphEditorSettings::setReloadOnChangeMode("ask");
//     break;
//   default:
//   kError() << "Invalid reload on change mode value: " << value;
//     return;
//   }
//   kDebug() << "emiting";
//   emit(settingsChanged());
//   KGraphEditorSettings::writeConfig();
// }
// 
// void KGraphViewer::openInExistingWindowMode_pressed(int value)
// {
//   std::cerr << "openInExistingWindowMode_pressed " << value << std::endl;
//   switch (value)
//   {
//   case 0:
//     KGraphEditorSettings::setOpenInExistingWindowMode("yes");
//     break;
//   case 1:
//     KGraphEditorSettings::setOpenInExistingWindowMode("no");
//     break;
//   case 2:
//     KGraphEditorSettings::setOpenInExistingWindowMode("ask");
//     break;
//   default:
//   kError() << "Invalid OpenInExistingWindow value: " << value << endl;
//     return;
//   }
// 
//   std::cerr << "emiting" << std::endl;
//   emit(settingsChanged());
//   KGraphEditorSettings::writeConfig();
// }
// 
// void KGraphViewer::reopenPreviouslyOpenedFilesMode_pressed(int value)
// {
//   std::cerr << "reopenPreviouslyOpenedFilesMode_pressed " << value << std::endl;
//   switch (value)
//   {
//   case 0:
//     KGraphEditorSettings::setReopenPreviouslyOpenedFilesMode("yes");
//     break;
//   case 1:
//     KGraphEditorSettings::setReopenPreviouslyOpenedFilesMode("no");
//     break;
//   case 2:
//     KGraphEditorSettings::setReopenPreviouslyOpenedFilesMode("ask");
//     break;
//   default:
//   kError() << "Invalid ReopenPreviouslyOpenedFilesMode value: " << value << endl;
//     return;
//   }
// 
//   std::cerr << "emiting" << std::endl;
//   emit(settingsChanged());
//   KGraphEditorSettings::writeConfig();
// }


void KGraphEditor::slotURLSelected(const KUrl& url)
{
  openUrl(url);
}

void KGraphEditor::close(QWidget* tab)
{
  m_openedFiles.remove(m_tabsFilesMap[tab]);
  m_widget->removePage(tab);
  tab->hide();
  KParts::Part* part = m_tabsPartsMap[tab];
  m_manager->removePart(part);
  m_tabsPartsMap.remove(tab);
  m_tabsFilesMap.remove(tab);
  delete part; part=0;
/*  delete tab;
  tab = 0;*/
}

void KGraphEditor::close()
{
  QWidget* currentPage = m_widget->currentPage();
  if (currentPage != 0)
  {
    close(currentPage);
  }
}

void KGraphEditor::fileSave()
{
  QWidget* currentPage = m_widget->currentPage();
  if (currentPage != 0)
  {
    emit(saveTo(QUrl(m_tabsFilesMap[currentPage]).path()));
  }
}

void KGraphEditor::fileSaveAs()
{
  QWidget* currentPage = m_widget->currentPage();
  if (currentPage != 0)
  {
    QString fileName = KFileDialog::getSaveFileName(KUrl(),
                "*.dot", currentPage,
                i18n("Save current graph to..."));
    m_tabsFilesMap[currentPage] = fileName;
    emit(saveTo(fileName));
  }
}

void KGraphEditor::newTabSelectedSlot(QWidget* tab)
{
//   kDebug() << tab;
  emit(hide((KParts::Part*)(m_manager->activePart())));
  if (!m_tabsPartsMap.isEmpty())
  {
    m_manager->setActivePart(m_tabsPartsMap[tab]);
  }
}

void KGraphEditor::slotSetActiveGraph( KParts::Part* part)
{
  kDebug();
  if (m_currentPart != 0)
  {
    disconnect(this,SIGNAL(prepareAddNewElement(QMap<QString,QString>)),m_currentPart,SLOT(prepareAddNewElement(QMap<QString,QString>)));
    disconnect(this,SIGNAL(prepareAddNewEdge(QMap<QString,QString>)),m_currentPart,SLOT(prepareAddNewEdge(QMap<QString,QString>)));
    disconnect(this,SIGNAL(saveTo(const QString&)),m_currentPart,SLOT(saveTo(const QString&)));
    disconnect(this,SIGNAL(removeNode(const QString&)),m_currentPart,SLOT(slotRemoveNode(const QString&)));
    disconnect(this,SIGNAL(addAttribute(const QString&)),m_currentPart,SLOT(slotAddAttribute(const QString&)));
    disconnect(this,SIGNAL(removeAttribute(const QString&,const QString&)),m_currentPart,SLOT(slotRemoveAttribute(const QString&,const QString&)));
    disconnect(this,SIGNAL(update()),m_currentPart,SLOT(slotUpdate()));
    disconnect(this,SIGNAL(selectNode(const QString&)),m_currentPart,SLOT(slotSelectNode(const QString&)));
    disconnect(this,SIGNAL(saddNewEdge(QString,QString,QMap<QString,QString>)),
           m_currentPart,SLOT(slotAddNewEdge(QString,QString,QMap<QString,QString>)));
  }
  m_currentPart = ((kgraphviewerPart*) part);
  m_treeWidget->clear();
  if (m_currentPart == 0)
  {
    return;
  }
  connect(this,SIGNAL(prepareAddNewElement(QMap<QString,QString>)),part,SLOT(prepareAddNewElement(QMap<QString,QString>)));
  connect(this,SIGNAL(prepareAddNewEdge(QMap<QString,QString>)),part,SLOT(prepareAddNewEdge(QMap<QString,QString>)));
  connect(this,SIGNAL(saveTo(const QString&)),part,SLOT(saveTo(const QString&)));
  connect(this,SIGNAL(removeNode(const QString&)),part,SLOT(slotRemoveNode(const QString&)));
  connect(this,SIGNAL(addAttribute(const QString&)),part,SLOT(slotAddAttribute(const QString&)));
  connect(this,SIGNAL(removeAttribute(const QString&,const QString&)),part,SLOT(slotRemoveAttribute(const QString&,const QString&)));
  connect(this,SIGNAL(update()),part,SLOT(slotUpdate()));
  connect(this,SIGNAL(selectNode(const QString&)),part,SLOT(slotSelectNode(const QString&)));
  connect( this, SIGNAL( removeElement(const QString&) ),
            m_currentPart, SLOT( slotRemoveElement(const QString&) ) );
  connect(this,SIGNAL(saddNewEdge(QString,QString,QMap<QString,QString>)),
            m_currentPart,SLOT(slotAddNewEdge(QString,QString,QMap<QString,QString>)));

  DotGraph* graph = m_currentPart->graph();
  QList<QTreeWidgetItem *> items;
  GraphNodeMap& nodesMap = graph->nodes();
  foreach (GraphNode* node, nodesMap)
  {
    kDebug()<< "new item " << node->id();
    QTreeWidgetItem* item = new QTreeWidgetItem((QTreeWidget*)0, QStringList(node->id()));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    foreach (const QString &attrib, node->attributes().keys())
    {
      if (attrib != "_draw_" && attrib != "_ldraw_")
      {
        QStringList list(attrib);
        list << node->attributes()[attrib];
        QTreeWidgetItem* child = new QTreeWidgetItem((QTreeWidget*)0, list);
        child->setFlags(child->flags() | Qt::ItemIsEditable);
        item->addChild(child);
      }
    }
    items.append(item);
  }
  kDebug() << "inserting";
  m_treeWidget->insertTopLevelItems(0, items);


  connect( m_currentPart, SIGNAL( graphLoaded() ),
           this, SLOT( slotGraphLoaded() ) );

  connect( m_currentPart, SIGNAL( newNodeAdded(const QString&) ),
          this, SLOT( slotNewNodeAdded(const QString&) ) );

  connect( m_currentPart, SIGNAL( newEdgeAdded(const QString&, const QString&) ),
            this, SLOT( slotNewEdgeAdded(const QString&, const QString&) ) );

  connect( m_currentPart, SIGNAL( removeElement(const QString&) ),
            this, SLOT( slotRemoveElement(const QString&) ) );

  connect( m_currentPart, SIGNAL( selectionIs(const QList<QString>&) ),
            this, SLOT( slotSelectionIs(const QList<QString>&) ) );

  connect( m_currentPart, SIGNAL( newEdgeFinished( const QString&, const QString&, const QMap<QString, QString>&) ),
            this, SLOT( slotNewEdgeFinished( const QString&, const QString&, const QMap<QString, QString>&) ) );
}

void KGraphEditor::slotNewNodeAdded(const QString& id)
{
  kDebug() << id;
  update();
}

void KGraphEditor::slotNewEdgeAdded(const QString& ids, const QString& idt)
{
  kDebug() << ids << idt;
  update();
}

void KGraphEditor::slotNewEdgeFinished( const QString& srcId, const QString& tgtId, const QMap<QString, QString>&attribs)
{
  kDebug() << srcId << tgtId << attribs;
  emit saddNewEdge(srcId,tgtId,attribs);
  update();
}

void KGraphEditor::slotGraphLoaded()
{
  kDebug();
  disconnect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
           this,SLOT(slotItemChanged(QTreeWidgetItem*,int)));
  disconnect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
           this,SLOT(slotItemClicked(QTreeWidgetItem*,int)));

  DotGraph* graph = m_currentPart->graph();
  QList<QTreeWidgetItem *> items;
  GraphNodeMap& nodesMap = graph->nodes();
  foreach (GraphNode* node, nodesMap)
  {
    kDebug()<< "item " << node->id();
    QTreeWidgetItem* item;
    QList<QTreeWidgetItem*> existingItems = m_treeWidget->findItems(node->id(),Qt::MatchRecursive|Qt::MatchExactly);
    if (existingItems.isEmpty())
    {
      item = new QTreeWidgetItem((QTreeWidget*)0, QStringList(node->id()));
      items.append(item);
    }
    else
    {
      item = existingItems[0];
    }
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    QList<QString> keys = node->attributes().keys();
    for (int i=0; i < item->childCount();i++)
    {
      if (keys.contains(item->child(i)->text(0)))
      {
        item->child(i)->setText(1,node->attributes()[item->child(i)->text(0)]);
        keys.removeAll(item->child(i)->text(0));
      }
    }
    foreach (const QString &attrib, keys)
    {
      if (attrib != "_draw_" && attrib != "_ldraw_")
      {
        QStringList list(attrib);
        list << node->attributes()[attrib];
        QTreeWidgetItem* child = new QTreeWidgetItem((QTreeWidget*)0, list);
        child->setFlags(child->flags() | Qt::ItemIsEditable);
        item->addChild(child);
      }
    }
}
  kDebug() << "inserting";
  m_treeWidget->insertTopLevelItems(0, items);
  connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
           this,SLOT(slotItemChanged(QTreeWidgetItem*,int)));
  connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
           this,SLOT(slotItemClicked(QTreeWidgetItem*,int)));
}

void KGraphEditor::slotItemChanged ( QTreeWidgetItem * item, int column )
{
  kDebug() ;
  DotGraph* graph = m_currentPart->graph();
  /* values column */
  if (column == 0)
  {
    QString oldNodeName = m_currentTreeWidgetItemText;
    QString newNodeName = item->text(0);
    if (oldNodeName != newNodeName)
    {
      kDebug() << "Renaming " << oldNodeName << " into " << newNodeName;
      GraphNode* node = graph->nodes()[oldNodeName];
      graph->nodes().remove(oldNodeName);
      node->setId(newNodeName);
      graph->nodes()[newNodeName] = node;
    }
  }
  else if (column == 1)
  {
    /* there is a parent ; it is an attribute line */
    if (item->parent() != 0)
    {
      QString nodeLabel = item->parent()->text(0);
      QString attributeName = item->text(0);
      QString attributeValue = item->text(1);
      graph->nodes()[nodeLabel]->attributes()[attributeName] = attributeValue;
    }
  }
  emit update();
}

void KGraphEditor::slotItemClicked ( QTreeWidgetItem * item, int column )
{
  kDebug() << column;
  m_currentTreeWidgetItemText = item->text(0);

  QString nodeName = item->parent() != 0 ?
                        item->parent()->text(0) :
                        item->text(0);
  emit selectNode(nodeName);
}

void KGraphEditor::slotEditNewVertex()
{
  kDebug() ;
  if (m_currentPart == 0)
  {
    return;
  }
  emit(prepareAddNewElement(m_newElementAttributes));
}

void KGraphEditor::slotEditNewEdge()
{
  kDebug() ;
  if (m_currentPart == 0)
  {
    return;
  }
  emit(prepareAddNewEdge(m_newElementAttributes));
}

void KGraphEditor::slotRemoveNode(const QString& nodeName)
{
  emit removeNode(nodeName);
  emit update();
}

void KGraphEditor::slotAddAttribute(const QString& attribName)
{
  emit addAttribute(attribName);
  emit update();
}

void KGraphEditor::slotRemoveAttribute(const QString& nodeName, const QString& attribName)
{
  kDebug();
  emit removeAttribute(nodeName,attribName);
  emit update();
}

void KGraphEditor::slotNewElementItemChanged(QTreeWidgetItem* item ,int column)
{
  kDebug();
  if (column == 0)
  {
    kError() << "Item id change not handled";
    return;
  }
  else if (column == 1)
  {
    m_newElementAttributes[item->text(0)] = item->text(1);
  }
  else
  {
    kError() << "Unknonw column" << column;
    return;
  }
}

void KGraphEditor::slotAddNewElementAttribute(const QString& attrib)
{
  kDebug();
  m_newElementAttributes[attrib] = QString();
}

void KGraphEditor::slotRemoveNewElementAttribute(const QString& attrib)
{
  kDebug();
  m_newElementAttributes.remove(attrib);
}

void KGraphEditor::slotRemoveElement(const QString& id)
{
  kDebug() << id;
  m_treeWidget->slotRemoveElement(id);
  emit(removeElement(id));
}

void KGraphEditor::slotSelectionIs(const QList<QString>& elements)
{
  kDebug();
  QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
  foreach (QTreeWidgetItem* item, items)
  {
    item->setSelected(false);
  }
  foreach (const QString &elementName, elements)
  {
    QList<QTreeWidgetItem*> items = m_treeWidget->findItems(elementName,Qt::MatchExactly,0);
    foreach (QTreeWidgetItem* item, items)
    {
      item->setSelected(true);
    }
  }
}

#include "kgrapheditor.moc"
