#include <QtCore/QCoreApplication>
#include <QHostAddress>
#include "thserveur.h"
using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    thServeur serveur;

    serveur.listen(QHostAddress::Any, 31331);


    return a.exec();
}
