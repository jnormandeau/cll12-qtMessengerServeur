/*
  Trames:

  Erreur:   ['E'][Message]
  Login:    ['L'][Username]['/'][Password]
  Create:   ['L'][Username]['/'][Password]
  Alive:    ['A']['A' || 'U'][Liste utilisateur] *Le 2ieme parametre('A' || 'U') est seulement envoyer dans le premier message alive
  Message:  ['M'][Message]

  Trames admin:

  Delete:   ['D'][Username]
  Kick:     ['K'][Username]
*/

#include "thserveur.h"

thServeur::thServeur(QObject *parent) :
    QTcpServer(parent)
{
    QString ligne;
    QStringList separateur;

    QFile fichierUtil("utilisateurs");
    QFile fichierAdmin("administrateurs");

    //Verifi si les fichiers contenant les comptes se sont ouverts correctement
    if (fichierUtil.open(QIODevice::ReadWrite))
    {
        QTextStream streamUtil(&fichierUtil);
        while (!streamUtil.atEnd())
        {
            ligne = streamUtil.readLine();
            separateur = ligne.split("/");
            m_hashUtil.insert(separateur.at(0), separateur.at(1));
        }
    }
    else
        //Le fichier utilisateurs na pas ete ouvert et le programme doit se terminer
        this->deleteLater();
    if (fichierAdmin.open(QIODevice::ReadWrite))
    {
        QTextStream streamAdmin(&fichierAdmin);
        while (!streamAdmin.atEnd())
        {
            ligne = streamAdmin.readLine();
            separateur = ligne.split("/");
            m_hashAdmin.insert(separateur.at(0), separateur.at(1));
        }
    }
    else
        //Le fichier administrateurs na pas ete ouvert et le programme doit se terminer
        this->deleteLater();

    m_TAlive = new QTimer(this);
    connect(m_TAlive, SIGNAL(timeout()), this, SLOT(TAlive()));
    m_TAlive->start(100);
}

//Traite les nouvelles connections
void thServeur::incomingConnection(int socketDesc)
{
    QByteArray baTrameRecu, baTrameEnvoi;
    bool connection = false;

    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDesc);

    std::cout << "Connection entrante de: " << qPrintable(socket->peerAddress().toString()) << std::endl;

    //Attend un message
    if(!socket->waitForReadyRead(5000))
    {
        baTrameEnvoi.append('E');
        baTrameEnvoi.append("Délai expiré pour la connexion.");
        socket->write(baTrameEnvoi);
    }
    else
    {
        baTrameRecu = socket->readAll();

        QString ligne(baTrameRecu), pseudonyme, password;
        QStringList separateur;

        separateur = ligne.split("/");
        if(separateur.length() > 1)
        {
            pseudonyme = separateur.at(0);
            pseudonyme.remove(0, 1);
            password = separateur.at(1);

            switch(baTrameRecu[0])
            {
                case CODE_CREATE:
                    if(!m_hashUtil.contains(pseudonyme))
                    {
                        m_hashUtil.insert(pseudonyme, password);
                        baTrameEnvoi.append(CODE_ALIVE);
                        baTrameEnvoi.append('U');
                        connection = true;
                        rafraichiFichierUtil();
                        std::cout << "L'utilisateur " << qPrintable(pseudonyme) << " a été créé." << std::endl;
                    }
                    else
                    {
                        baTrameEnvoi.append(CODE_ERR);
                        baTrameEnvoi.append("Le nom choisi est déja utilisé.");
                        socket->write(baTrameEnvoi);
                    }
                    break;

                case CODE_LOGIN:
                    if(m_hashAdmin.contains(pseudonyme) && m_hashAdmin.value(pseudonyme) == password)
                    {
                        baTrameEnvoi.append(CODE_ALIVE);
                        baTrameEnvoi.append('A');
                        pseudonyme.prepend("@");
                        connection = true;
                    }
                    else
                    {
                        if(m_hashUtil.contains(pseudonyme) && m_hashUtil.value(pseudonyme) == password)
                        {
                            baTrameEnvoi.append(CODE_ALIVE);
                            baTrameEnvoi.append('U');
                            connection = true;
                        }
                        else
                        {
                            baTrameEnvoi.append(CODE_ERR);
                            baTrameEnvoi.append("Login incorrect.");
                            socket->write(baTrameEnvoi);
                        }
                    }

                    break;

                default:
                    baTrameEnvoi.append(CODE_ERR);
                    baTrameEnvoi.append("Message non reconnu.");
                    socket->write(baTrameEnvoi);
                    break;
            }

            if(connection)
            {
                connect(socket, SIGNAL(readyRead()), SLOT(messageRecu()));
                connect(socket, SIGNAL(disconnected()), SLOT(deconnection()));

                m_hashConnections.insert(socket, pseudonyme);

                baTrameEnvoi.append(listeConnections());
                socket->write(baTrameEnvoi);
                std::cout << qPrintable(pseudonyme) << " est connecté" << std::endl;
            }
        }
        else
        {
            baTrameEnvoi.append(CODE_ERR);
            baTrameEnvoi.append("Message non reconnu.");
            socket->write(baTrameEnvoi);
        }
    }
}

//Déclencher lorsqu'un message est recu d'un socket connecté au serveur
void thServeur::messageRecu()
{
    QTcpSocket* socketReception = static_cast<QTcpSocket*>(sender());
    QTcpSocket* socketDestination;
    QByteArray baTrameEnvoi, baTrameRecu = socketReception->readAll();
    QString pseudonyme, message(baTrameRecu);
    QHashIterator<QTcpSocket*, QString> iterateurConnections(m_hashConnections);

    pseudonyme = m_hashConnections.value(socketReception);
    message.remove(0, 1);

    switch(baTrameRecu[0])
    {
        case CODE_MESSAGE:
            message.prepend(pseudonyme + ": ");
            message.prepend('M');
            baTrameEnvoi.append(message);

            while (iterateurConnections.hasNext())
            {
                iterateurConnections.next();
                socketDestination = iterateurConnections.key();
                socketDestination->write(baTrameEnvoi);
            }
            std::cout << qPrintable(baTrameEnvoi) << std::endl;
            break;

        case CODE_DELETE:
            pseudonyme.remove(0, 1); //Enleve le @ des admins
            if(m_hashAdmin.contains(pseudonyme))
            {
                    if(m_hashUtil.contains(message))
                    {
                        m_hashUtil.remove(message);
                        std::cout << qPrintable(pseudonyme) << " a delete " << qPrintable(message) << std::endl;
                        rafraichiFichierUtil();

                        //Kick l'utilisateur qui vient d'etre effacer
                        while (iterateurConnections.hasNext())
                        {
                            iterateurConnections.next();
                            if(iterateurConnections.value() == message)
                            {
                                iterateurConnections.key()->close();
                                break;
                            }
                        }
                    }
                    else
                    {
                        baTrameEnvoi.append('E');
                        baTrameEnvoi.append("L'utilisateur n'existe pas.");
                        socketReception->write(baTrameEnvoi);
                    }
            }
            break;

        case CODE_KICK:
            pseudonyme.remove(0, 1); //Enleve le @ des admins
            if(m_hashAdmin.contains(pseudonyme))
            {
                while (iterateurConnections.hasNext())
                {
                    iterateurConnections.next();
                    if(iterateurConnections.value() == message)
                    {
                        iterateurConnections.key()->close();
                        std::cout << qPrintable(pseudonyme) << " a kick " << qPrintable(iterateurConnections.value()) << std::endl;
                        break;
                    }
                }
            }
            break;
    }
}

//Déclencher lorsqu'un scoket connecté au serveur se déconnecte
void thServeur::deconnection()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    m_hashConnections.remove(socket);
    socket->deleteLater();
}

//Sauvegarde le contenu de m_hashUtil dans le fichier utilisateurs
void thServeur::rafraichiFichierUtil()
{
    QFile fichierUtil("utilisateurs");
    QHashIterator<QString, QString> iterateurUtilisateur(m_hashUtil);
    QString listeUtilisateur;

    if (fichierUtil.open(QIODevice::ReadWrite))
    {
        while (iterateurUtilisateur.hasNext())
        {
            iterateurUtilisateur.next();
            listeUtilisateur.append(iterateurUtilisateur.key() + "/" + iterateurUtilisateur.value() + "\n");
        }

        fichierUtil.write(listeUtilisateur.toAscii());
        fichierUtil.resize(fichierUtil.pos());
    }
}

//retourne un QByteArray contenant la liste des utilisateurs connectés
QByteArray thServeur::listeConnections()
{
    QHashIterator<QTcpSocket*, QString> iterateurConnections(m_hashConnections);
    QString listeConnections;
    QByteArray baListeConnections;
    while (iterateurConnections.hasNext())
    {
        iterateurConnections.next();
        listeConnections.append(iterateurConnections.value() + "/");
    }
    baListeConnections.append(listeConnections);
    return baListeConnections;
}

void thServeur::TAlive()
{
    QByteArray baTrameEnvoi;
    QTcpSocket* socketDestination;
    QHashIterator<QTcpSocket*, QString> iterateurConnections(m_hashConnections);

    baTrameEnvoi.append(CODE_ALIVE);
    baTrameEnvoi.append(listeConnections());

    while (iterateurConnections.hasNext())
    {
        iterateurConnections.next();
        socketDestination = iterateurConnections.key();
        socketDestination->write(baTrameEnvoi);
    }
}
