#include "DrawableGraphicsScene.h"


DrawableGraphicsScene::DrawableGraphicsScene(QObject* parent) : QGraphicsScene(parent){
	sceneMode = NoMode;
	itemToDraw = 0;
}


DrawableGraphicsScene::~DrawableGraphicsScene(){
}

void DrawableGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if ((sceneMode == DrawLine) || (sceneMode == DrawInLine))
		origPoint = event->scenePos();
	QGraphicsScene::mousePressEvent(event);
}

void DrawableGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (sceneMode == DrawLine) {
		if (!itemToDraw) {
			itemToDraw = new QGraphicsLineItem;
			this->addItem(itemToDraw);
			itemToDraw->setPen(QPen(Qt::black, 3, Qt::SolidLine));
			itemToDraw->setPos(origPoint);
		}
		itemToDraw->setLine(0, 0,
			event->scenePos().x() - origPoint.x(),
			event->scenePos().y() - origPoint.y());
	}else
		if (sceneMode == DrawInLine) {
			if (!itemToDraw) {
				itemToDraw = new QGraphicsLineItem;
				this->addItem(itemToDraw);
				itemToDraw->setPen(QPen(Qt::blue, 3, Qt::SolidLine));
				itemToDraw->setPos(origPoint);
			}
			itemToDraw->setLine(0, 0,
				event->scenePos().x() - origPoint.x(),
				event->scenePos().y() - origPoint.y());
		}
	else
		QGraphicsScene::mouseMoveEvent(event);
}

void DrawableGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	itemToDraw = 0;
	QGraphicsScene::mouseReleaseEvent(event);
}

void DrawableGraphicsScene::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Delete)
		foreach(QGraphicsItem* item, selectedItems()) {
		removeItem(item);
		delete item;
	}
	else
		QGraphicsScene::keyPressEvent(event);
}

void DrawableGraphicsScene::setMode(Mode mode) {
	sceneMode = mode;
	QGraphicsView::DragMode vMode =
		QGraphicsView::NoDrag;
	if ((mode == DrawLine) || (mode == DrawInLine)){
		makeItemsControllable(false);
		vMode = QGraphicsView::NoDrag;
	}
	else if (mode == SelectObject) {
		makeItemsControllable(true);
		vMode = QGraphicsView::RubberBandDrag;
	}
	QGraphicsView* mView = views().at(0);
	if (mView)
		mView->setDragMode(vMode);
}


void DrawableGraphicsScene::makeItemsControllable(bool areControllable) {
	foreach(QGraphicsItem* item, items()) {
		item->setFlag(QGraphicsItem::ItemIsSelectable,
			areControllable);
		item->setFlag(QGraphicsItem::ItemIsMovable,
			areControllable);
	}
}
