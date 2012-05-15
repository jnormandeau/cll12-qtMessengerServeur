#ifndef THSERVEUR_H
#define THSERVEUR_H

#include "codes.h"

#include <iostream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QList>
#include <QHash>
#include <QBuffer>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>

class thServeur : public QTcpServer
{
    Q_OBJECT
public:
    explicit thServeur(QObject *parent = 0);

protected:
    void incomingConnection(int socketDesc);
    void destroyed();

private:
    QHash<QString, QString> m_hashUtil;
    QHash<QString, QString> m_hashAdmin;

    QHash<QTcpSocket*, QString> m_hashConnections;
    QByteArray listeConnections();

private slots:
    void messageRecu();
    void deconnection();
};

#endif // THSERVEUR_H
