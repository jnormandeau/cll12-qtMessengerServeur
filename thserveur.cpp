#include "thserveur.h"

thServeur::thServeur(QObject *parent) :
    QTcpServer(parent)
{
    //Initialisation de la liste des messages
    m_MessageCourant = 0;
    m_MessageListe = new QList<QString>();

    //Initialisation des codes de messages
    codeErr = 21;//NAK
    codeLogin = 'L';
    codeCreate = 'C';
    codeDelete = 'D';
    codeAlive = 'A';
    codeMessage = 'M';

}

void thServeur::incomingConnection(int socketDesc)
{
    QTcpSocket socket(this);
    socket.setSocketDescriptor(socketDesc);

    if(!socket.waitForBytesWritten(5000)) //Attendre le nom d'utilisateur et le mot de passe
    {
        m_baTrame.clear();
        m_baTrame.append(codeErr + "Délai expiré pour la connexion.");
        socket.write(m_baTrame); //Envoie de l'erreur
    }

    //Login

}
