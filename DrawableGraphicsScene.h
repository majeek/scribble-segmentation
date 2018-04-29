#ifndef DRAWABLEGRAPHICSSCENE_HPP_
#define DRAWABLEGRAPHICSSCENE_HPP_

#include "qgraphicsscene.h"
#include "qgraphicssceneevent.h"
#include "qgraphicslinearlayout.h"
#include "qgraphicsview.h"
#include "qdebug.h"


class DrawableGraphicsScene : public QGraphicsScene{

public:
	enum Mode { NoMode, SelectObject, DrawLine, DrawInLine };
	void setMode(Mode mode);
	DrawableGraphicsScene(QObject* parent = 0);

	~DrawableGraphicsScene();

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

private:
	Mode sceneMode;
	QPointF origPoint;
	QGraphicsLineItem* itemToDraw;
	void makeItemsControllable(bool areControllable);

};
#endif // DRAWABLEGRAPHICSSCENE_HPP_