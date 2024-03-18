#ifndef SESSIONLIST_H
#define SESSIONLIST_H

#include <QListView>
#include <QStandardItemModel>

class MainWindow;
class SessionList: public QListView {
    Q_OBJECT

public:
    explicit SessionList(QWidget *parent = nullptr);
    virtual ~SessionList();
    void setMainWindow(MainWindow *win) { m_mainWindow = win; }

    void appendRow(std::string);
    void selectRow(int index);

    void onItemClicked(const QModelIndex &index);

private:
    MainWindow *m_mainWindow;
    QStandardItemModel *m_listmodel;
};

#endif // SESSIONLIST_H
