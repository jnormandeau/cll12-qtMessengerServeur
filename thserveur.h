#ifndef THSERVEUR_H
#define THSERVEUR_H

#include <QTcpServer>
#include <QList>
#include <QTcpSocket>

class thServeur : public QTcpServer
{
    Q_OBJECT
public:
    explicit thServeur(QObject *parent = 0);

protected:
    void incomingConnection(int);

private:
    unsigned int m_MessageCourant;
    QList *m_MessageListe;
    QByteArray m_baTrame;

    //Codes messages
    char codeErr,codeLogin,codeCreate,codeDelete,codeAlive,codeMessage;
    
signals:
    
public slots:
    
};

#endif // THSERVEUR_H
