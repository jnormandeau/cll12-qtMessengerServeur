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
#include <QTimer>

class thServeur : public QTcpServer
{
    Q_OBJECT
public:
    explicit thServeur(QObject *parent = 0);
    //~thServeur();

protected:
    void incomingConnection(int socketDesc);
    void destroyed();

private:
    QHash<QString, QString> m_hashUtil;
    QHash<QString, QString> m_hashAdmin;

    QHash<QTcpSocket*, QString> m_hashConnections;
    QByteArray listeConnections();
    void rafraichiFichierUtil();
    QTimer *m_TAlive;

private slots:
    void messageRecu();
    void deconnection();
    void TAlive();
};

#endif // THSERVEUR_H
