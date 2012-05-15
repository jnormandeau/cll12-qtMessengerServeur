/*
    A completer:
        -Timeout si un client ne repond pas pour X secondes (est ce que disconnected() est asser?)
        -Segmentation fault dans incomingConnection()
        -Sauvegarder les hash tables avant de terminer (signal destroyed())

        commentaire avec plusieurs ******* = commentaires temporaires a enlever avant la version finale
*/

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
        while(!streamUtil.atEnd())
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
        while(!streamAdmin.atEnd())
        {
            ligne = streamAdmin.readLine();
            separateur = ligne.split("/");
            m_hashAdmin.insert(separateur.at(0), separateur.at(1));
        }
    }
    else
        //Le fichier administrateurs na pas ete ouvert et le programme doit se terminer
        this->deleteLater();
}

void thServeur::incomingConnection(int socketDesc)
{
    //std::cout << "incomming connection..." << std::endl;

    QByteArray baTrameRecu, baTrameEnvoi;
    bool connection = false;

    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDesc);

    //Attend un message
    if(!socket->waitForReadyRead(5000))
    {
        baTrameEnvoi.append('E');
        baTrameEnvoi.append("Délai expiré pour la connexion.");
        socket->write(baTrameEnvoi); //Envoie de l'erreur
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
                        //std::cout << "L'utilisateur " << pseudonyme.toAscii() << " a été créé." << std::endl;
                    }
                    else
                    {
                        baTrameEnvoi.append(CODE_ERR);//ajoute accents*********
                        baTrameEnvoi.append("Le nom choisi est deja utilise.");
                        socket->write(baTrameEnvoi);
                    }
                    break;

                case CODE_LOGIN:
                    if(m_hashAdmin.contains(pseudonyme) && m_hashAdmin.value(pseudonyme) == password)
                    {
                        baTrameEnvoi.append(CODE_ALIVE);
                        baTrameEnvoi.append('A');
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
                            baTrameEnvoi.append("Ceci est un message d'erreur.");
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
                //std::cout << pseudonyme << "est connecté" << std::endl;
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
            baTrameEnvoi.append(message);

            while (iterateurConnections.hasNext())
            {
                iterateurConnections.next();
                socketDestination = iterateurConnections.key();
                socketDestination->write(baTrameEnvoi);
            }
            break;

        case CODE_ALIVE:
            baTrameEnvoi.append(CODE_ALIVE);
            baTrameEnvoi.append(listeConnections());
            socketReception->write(baTrameEnvoi);
            break;

        case CODE_DELETE:
            if(m_hashUtil.contains(pseudonyme))
            {
                while (iterateurConnections.hasNext())
                {
                    iterateurConnections.next();
                    if(iterateurConnections.value() == pseudonyme)
                    {
                        m_hashUtil.remove(pseudonyme);
                        iterateurConnections.key()->disconnectFromHost();
                        //std::cout << pseudonyme << "a ban " << iterateurConnections.value() << std::endl;
                    }
                }
            }
            break;

        case CODE_KICK:
            if(m_hashAdmin.contains(pseudonyme))
            {
                while (iterateurConnections.hasNext())
                {
                    iterateurConnections.next();
                    if(iterateurConnections.value() == pseudonyme)
                    {
                        iterateurConnections.key()->disconnectFromHost();
                        //std::cout << pseudonyme << "a kick " << iterateurConnections.value() << std::endl;
                    }
                }
            }
            break;
    }
}

void thServeur::deconnection()
{
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    m_hashConnections.remove(socket);
    socket->deleteLater();
}

void thServeur::destroyed()
{
    //Sauvegarde le contenu de m_hashUtil et m_hashAdmin dans les fichier utilisateurs et administrateurs
    //... **********
}

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

