#include "marginalnotesdetection.h"

MarginalNotesDetection::MarginalNotesDetection(QWidget *parent): QMainWindow(parent){
	ui.setupUi(this);
	
	
	fScene = nullptr;
	ui.graphicsViewImage->setRenderHints(QPainter::Antialiasing);
	//setCentralWidget(view);

	fLineInAction = new QAction("Draw in line", this);
	fLineInAction->setData(int(DrawableGraphicsScene::DrawInLine));
	fLineInAction->setIcon(QIcon(QPixmap(":/line.png")));
	fLineInAction->setCheckable(true);

	fLineAction = new QAction("Draw out line", this);
	fLineAction->setData(int(DrawableGraphicsScene::DrawLine));
	fLineAction->setIcon(QIcon(QPixmap(":/line.png")));
	fLineAction->setCheckable(true);

	fSelectAction = new QAction("Select object", this);
	fSelectAction->setData(int(DrawableGraphicsScene::SelectObject));
	fSelectAction->setIcon(QIcon(QPixmap(":/select.png")));
	fSelectAction->setCheckable(true);

	fActionGroup = new QActionGroup(this);
	fActionGroup->setExclusive(true);
	fActionGroup->addAction(fLineAction);
	fActionGroup->addAction(fLineInAction);
	fActionGroup->addAction(fSelectAction);

	QObject::connect(ui.pushButtonGrabCut, SIGNAL(clicked()), this, SLOT(grabCutImageSlot()));
	QObject::connect(fActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(actionGroupClicked(QAction*)));

	QObject::connect(ui.pushButtonChooseImage, SIGNAL(clicked()), this, SLOT(chooseImageSlot()));
	
	ui.toolBar->addAction(fSelectAction);
	ui.toolBar->addAction(fLineAction);
	ui.toolBar->addAction(fLineInAction);
	fIdx = 0;
}

void MarginalNotesDetection::chooseImageSlot() {
	QString imagePath = QFileDialog::getOpenFileName(this, tr("Choose image file"), ui.lineEditImageName->text());//, QFileDialog::DontResolveSymlinks);
	if (!imagePath.isEmpty()) {
		ui.lineEditImageName->setText(imagePath);
		//choose image and load it if chosen
		fImage = cv::imread(imagePath.toStdString());
		showMatOnGraphicsView(fImage, &fScene, ui.graphicsViewImage);
	}
}

void MarginalNotesDetection::showMatOnGraphicsView(cv::Mat imageToShow, DrawableGraphicsScene** gaphicsScene, QGraphicsView* graphicsView) {

	if (imageToShow.channels() == 1)
		cv::cvtColor(imageToShow, imageToShow, CV_GRAY2RGB);
	
	QImage qImage((uchar*)imageToShow.data, imageToShow.cols, imageToShow.rows, imageToShow.step, QImage::Format_RGB888);
	QPixmap graphicsViewImagePixmap = QPixmap::fromImage(qImage);
	if (!graphicsViewImagePixmap.isNull()) {
		if (*gaphicsScene != nullptr)
			delete *gaphicsScene;

		*gaphicsScene = new DrawableGraphicsScene();

		graphicsView->fitInView((*gaphicsScene)->sceneRect(), Qt::KeepAspectRatio);
		graphicsView->setScene(*gaphicsScene);
		(*gaphicsScene)->addPixmap(graphicsViewImagePixmap);// .scaled(graphicsView->size(), Qt::KeepAspectRatio));
		
	}
}


void MarginalNotesDetection::actionGroupClicked(QAction *action) {
	fScene->setMode(DrawableGraphicsScene::Mode(action->data().toInt()));
}

MarginalNotesDetection::~MarginalNotesDetection(){

}

void MarginalNotesDetection::createGaborBank() {
	fGaborFilterBank.clear();
	cv::Mat src_f;
	fImage.convertTo(src_f, CV_32F);
	cv::cvtColor(src_f, src_f, CV_RGB2GRAY);

	int kernel_size = 9; // size of filter 
	double sigma = 1; // standard deviation of gaussian envelope
	QVector<double> thetas; //the orientation of the filter
	//for (size_t i = -1; i < 2; i++) { // the orientations, 0, 45, 90, 135 for the gabor filter
	//	thetas.push_back(i * M_PI / 4);
	//}
	thetas.push_back(1 * M_PI / 4);
	thetas.push_back(2 * M_PI / 4);
	thetas.push_back(3 * M_PI / 4);
	//calculating lambda's values // wavelength of sinusoidal factor
	QVector<double> J;
	for (size_t i = 0; i <= std::log2(fImage.cols / 8); i++) {
		J.push_back(0.25 - ((qPow(2, i) - 0.5) / fImage.cols));
		J.push_back(0.25 + ((qPow(2, i) - 0.5) / fImage.cols));
	}
	QVector<double> F;
	for (size_t i = 0; i < J.size(); i++) {
		double min = DBL_MAX;
		int minIdx = -1;
		for (size_t o = 0; o < J.size(); o++) {
			if (min > J[o]) {
				minIdx = o;
				min = J[o];
			}
		}
		F.push_back(J[minIdx]);
		J[minIdx] = DBL_MAX;
	}
	QVector<double> lambdas;
	for (size_t i = 0; i < F.size(); i++) {
		lambdas.push_back(1.0 / F[i]);
	}
	double gamma = 1; // spatial aspect ratio
	double psi = 0; // phase offset // 0 is the real parts of the gabor filter //pi/2 is the imaginary parts 
					//Step 1: Gabor Filter Bank Generation
					//now we generate the gabor filter bank from the parameters

	for (double theta : thetas) {
		cv::Mat matToPush;
		int idx = 0;
		for (double lambda : lambdas) {
			cv::Mat dest;
			cv::Mat kernel = cv::getGaborKernel(cv::Size(kernel_size, kernel_size), sigma, theta, lambda, gamma, psi);
			cv::filter2D(src_f, dest, CV_32FC1, kernel);
			cv::Mat dest2;
			if (idx == 0)
				matToPush = dest;
			else
				matToPush += dest;

			idx++;
		}
		cv::Mat dst;
		fGaborFilterBank.push_back(matToPush);
	}
	//Step 2: Feature Extraction 

	//Nonlinearity
	for (cv::Mat entry : fGaborFilterBank) {
		for (int i = 0; i < entry.rows; i++)
			for (int j = 0; j < entry.cols; j++)
				entry.at<float>(i, j) = std::tanh(0.25*entry.at<float>(i, j));
	}
	//Smoothing
	double b = 1;
	for (size_t i = 0; i < fGaborFilterBank.size(); i++) {
		cv::Mat dest;
		cv::Mat dst;
		double lambda = lambdas[std::floor(i / thetas.size())];
		double sigma = 3 * ((1 / M_PI) * std::sqrt(std::log(2) / 2) * (qPow(2, b) + 1) / (qPow(2, b) - 1) * lambda);
		cv::GaussianBlur(fGaborFilterBank[i], dest, cv::Size(2 * std::floor(sigma) + 1, 2 * std::floor(sigma) + 1), sigma);
		fGaborFilterBank[i] = dest;
	}
}
void MarginalNotesDetection::grabCutImageSlot() {
	fIdx++;
	cv::Mat msk;
	if (fGaborMixMat.empty()) {
		ui.statusBar->showMessage("Generating Gabor Filter Bank...");
		createGaborBank();
		
		std::vector<cv::Mat> images(fGaborFilterBank.size());
		for (int i = 0; i < fGaborFilterBank.size(); i++) {
			cv::Mat mat;
			cv::normalize(fGaborFilterBank[i], mat, 0, 255, cv::NORM_MINMAX, CV_8UC1);
			images.at(i) = mat;
		}

		cv::merge(images, fGaborMixMat);

	}
	
	cv::namedWindow("gaborMat", 1);
	cv::imshow("gaborMat", fGaborMixMat);
	
	cv::Rect rect(1, 1, fImage.cols - 1, fImage.rows - 1);
	cv::Mat bgdModel;
	cv::Mat fgdModel;
	int iterCount = 1;
	if (fMask.empty()) {
		ui.statusBar->showMessage("Applying initial GrabCut...");
		//initilize mask with broad bounding box
		cv::grabCut(fGaborMixMat, fMask, rect, bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_RECT);
		fMask.copyTo(msk);
	}
	else {
		//now we take all lines and mark them as cv::GC_BGD 
		QRectF sceneRect = ui.graphicsViewImage->sceneRect();
		for (QGraphicsItem* item : ui.graphicsViewImage->scene()->items()) {
			QRectF itemRect = item->sceneBoundingRect();
			if ((sceneRect.height() != itemRect.height()) || (sceneRect.width() != itemRect.width()) || (sceneRect.x() != itemRect.x()) || (sceneRect.y() != itemRect.y())) {
				QGraphicsLineItem* lineItem = (QGraphicsLineItem*)item;
				
				QPointF pos = lineItem->pos();
				QLineF line = lineItem->line();
				QColor color = lineItem->pen().color();
				QVector<QPointF> lineCoords;
				for (int i = 0; i <= line.length(); i++) {
					QPointF pnt = line.pointAt(i / line.length());
					lineCoords.push_back(pnt);
					lineCoords.push_back(QPointF(pnt.x() + 1, pnt.y()));
					lineCoords.push_back(QPointF(pnt.x() - 1, pnt.y()));
					lineCoords.push_back(QPointF(pnt.x(), pnt.y() + 1));
					lineCoords.push_back(QPointF(pnt.x(), pnt.y() + 1));
				}
				for (QPointF coord : lineCoords) {
					QPointF pnt = pos + coord;
					if ((pnt.y() < fMask.rows) && (pnt.x() < fMask.cols) &&
						(pnt.y() >= 0) && (pnt.x() >= 0)) {
						if (color == Qt::blue)
							fMask.at<uchar>(pnt.y(), pnt.x()) = cv::GC_FGD;
						else if (color == Qt::black)
							fMask.at<uchar>(pnt.y(), pnt.x()) = cv::GC_BGD;
					}
				}
			}
		}
		ui.statusBar->showMessage("Applying Stroke-Assisted GrabCut...");
		if (ui.checkBoxInteractive->isChecked()) 
			cv::grabCut(fGaborMixMat, fMask, rect, bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_MASK);
		else {
			fMask.copyTo(msk);
			cv::grabCut(fGaborMixMat, msk, rect, bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_MASK);
		}
		
	}
	
	cv::Mat theMask;
	if (ui.checkBoxInteractive->isChecked())
		theMask = fMask;
	else
		theMask = msk;
	cv::Mat toShowMask = cv::Mat::zeros(theMask.size(), theMask.type());
	for (int row = 0; row < theMask.rows; row++) {
		for (int col = 0; col < theMask.cols; col++) {
			if (theMask.at<uchar>(row, col) == 2)
				toShowMask.at<uchar>(row, col) = 55;
			if (theMask.at<uchar>(row, col) == 1)
				toShowMask.at<uchar>(row, col) = 255;
			if (theMask.at<uchar>(row, col) == 3)
				toShowMask.at<uchar>(row, col) = 200;
		}
	}

	cv::namedWindow("mask", 1);
	cv::imshow("mask", toShowMask);
	cv::Mat foreground = cv::Mat::zeros(toShowMask.size(), CV_8UC3);
	
	for (int row = 0; row < toShowMask.rows; row++)
		for (int col = 0; col < toShowMask.cols; col++)
			if (toShowMask.at<uchar>(row, col) > 199) // possible foreground / forground
				foreground.at<cv::Vec3b>(row, col) = fImage.at<cv::Vec3b>(row, col);
	cv::namedWindow("result", 1);
	cv::imshow("result", foreground);
	cv::imwrite("result" + QString::number(fIdx).toStdString() + ".png", foreground);
	ui.statusBar->showMessage("done.");
}

void MarginalNotesDetection::printMat(cv::Mat dst) {
	for (int i = 0; i < dst.rows; i++) {
		QString str;
		for (int j = 0; j < dst.cols; j++) {
			if (j == 0)
				str += QString::number(dst.at<uchar>(i, j));
			else
				str += ", " + QString::number(dst.at<uchar>(i, j));
		}
		qDebug() << str << "\n";
	}
}

