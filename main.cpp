#include "marginalnotesdetection.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::addLibraryPath("./");
	QApplication a(argc, argv);
	MarginalNotesDetection w;
	w.show();
	return a.exec();
}
