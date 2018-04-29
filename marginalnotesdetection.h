#ifndef MARGINALNOTESDETECTION_H
#define MARGINALNOTESDETECTION_H

#include <QtWidgets/QMainWindow>
#include <qdebug.h>
#include <qmath.h>
#include "DrawableGraphicsScene.h"
#include "qtoolbar.h"
#include "qfiledialog.h"
#include "qgraphicsitem.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "ui_marginalnotesdetection.h"

class MarginalNotesDetection : public QMainWindow
{
	Q_OBJECT

public:
	MarginalNotesDetection(QWidget *parent = 0);
	~MarginalNotesDetection();

private:
	Ui::MarginalNotesDetectionClass ui;
	cv::Mat fImage;
	DrawableGraphicsScene *fScene;
	QAction *fSelectAction;
	QAction *fLineAction;
	QAction *fLineInAction;
	QActionGroup *fActionGroup;

	QVector<cv::Mat> fGaborFilterBank;
	cv::Mat fGaborMixMat;
	cv::Mat fMask;
	int fIdx;

	void printMat(cv::Mat);
	void createGaborBank();
	void showMatOnGraphicsView(cv::Mat imageToShow, DrawableGraphicsScene** gaphicsScene, QGraphicsView* graphicsView);
public slots:
	void grabCutImageSlot();
	void actionGroupClicked(QAction *action);
	void chooseImageSlot();
};

#endif // MARGINALNOTESDETECTION_H
