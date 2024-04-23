#include "mainwindow.h"
#include "sessionlist.h"

SessionList::SessionList(QWidget *parent)
    : QListView(parent)
{
    m_listmodel = new QStandardItemModel();
    setModel(m_listmodel);

    connect(this, &QListView::clicked, this, &SessionList::onItemClicked);
}

SessionList::~SessionList()
{
    delete m_listmodel;
}

void SessionList::appendRow(QString name)
{
    QStandardItem *item = new QStandardItem(name);
    m_listmodel->appendRow(item);
    selectRow(m_listmodel->rowCount()-1);
}

void SessionList::selectRow(int index)
{
    QModelIndex idx = model()->index(index, 0);
    setCurrentIndex(idx);
}

void SessionList::onItemClicked(const QModelIndex &index)
{
  QString data = m_listmodel->data(index, Qt::DisplayRole).toString();
  // qDebug() << data;
  m_mainWindow->m_ws->setSession(data);
}
