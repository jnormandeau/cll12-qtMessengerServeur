#ifndef THSERVEUR_H
#define THSERVEUR_H

#include <QTcpServer>
#include <QList>
#include <QTcpSocket>
#include <QFile>

class thServeur : public QTcpServer
{
    Q_OBJECT
public:
    explicit thServeur(QObject *parent = 0);

protected:
    void incomingConnection(int);

private:
    unsigned int m_MessageCourant;
    QList<QString> * m_MessageListe;
    QByteArray m_baTrame, m_baTrameRecu;
    QFile * m_FichierUtilisateur;
    QFile * m_FichierAdmin;

    //Codes messages
    static const char codeErr; //, codeLogin,codeCreate,codeDelete,codeAlive,codeMessage;

    //Méthodes privées
    
signals:
    
public slots:
    
};

#endif // THSERVEUR_H
