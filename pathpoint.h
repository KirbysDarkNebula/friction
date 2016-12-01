#ifndef PATHPOINT_H
#define PATHPOINT_H
#include "movablepoint.h"
#include <QSqlQuery>
#include "pointhelpers.h"

class UndoRedoStack;

class VectorPath;

class CtrlPoint;

enum CanvasMode : short;

class PathPoint;

class VectorPathShape {
public:
    VectorPathShape() {
        mInfluence.setValueRange(0., 1.);
        mInfluence.blockPointer();
        mInfluence.setName("influence");
    }

    QString getName() {
        return mName;
    }

    void setName(QString name) {
        mName = name;
        mInfluence.setName(name);
    }

    bool isRelative() {
        return mRelative;
    }

    void setRelative(bool bT) {
        mRelative = bT;
    }

    qreal getCurrentInfluence() {
        return mInfluence.getCurrentValue();
    }

    void setCurrentInfluence(qreal value, bool finish = false) {
        mInfluence.setCurrentValue(value, finish);
    }

    QrealAnimator *getInfluenceAnimator() {
        return &mInfluence;
    }
private:
    bool mRelative = false;
    QString mName;
    QrealAnimator mInfluence;
};

struct PathPointValues {
    PathPointValues(QPointF startPosT,
                    QPointF pointPosT,
                    QPointF endPosT) {
        startRelPos = startPosT;
        pointRelPos = pointPosT;
        endRelPos = endPosT;
    }

    PathPointValues() {}


    QPointF startRelPos;
    QPointF pointRelPos;
    QPointF endRelPos;

    PathPointValues &operator/=(const qreal &val)
    {
        startRelPos /= val;
        pointRelPos /= val;
        endRelPos /= val;
        return *this;
    }
    PathPointValues &operator*=(const qreal &val)
    {
        startRelPos *= val;
        pointRelPos *= val;
        endRelPos *= val;
        return *this;
    }
    PathPointValues &operator+=(const PathPointValues &ppv)
    {
        startRelPos += ppv.startRelPos;
        pointRelPos += ppv.pointRelPos;
        endRelPos += ppv.endRelPos;
        return *this;
    }
    PathPointValues &operator-=(const PathPointValues &ppv)
    {
        startRelPos -= ppv.startRelPos;
        pointRelPos -= ppv.pointRelPos;
        endRelPos -= ppv.endRelPos;
        return *this;
    }
};

PathPointValues operator+(const PathPointValues &ppv1, const PathPointValues &ppv2);
PathPointValues operator-(const PathPointValues &ppv1, const PathPointValues &ppv2);
PathPointValues operator/(const PathPointValues &ppv, const qreal &val);
PathPointValues operator*(const PathPointValues &ppv, const qreal &val);
PathPointValues operator*(const qreal &val, const PathPointValues &ppv);

class PointShapeValues {
public:
    PointShapeValues();
    PointShapeValues(VectorPathShape *shape, PathPointValues values) {
        mShape = shape;
        mValues = values;
    }

    const PathPointValues &getValues() const {
        return mValues;
    }

    VectorPathShape *getParentShape() const {
        return mShape;
    }

    void setPointValues(const PathPointValues &values) {
        mValues = values;
    }

private:
    PathPointValues mValues;
    VectorPathShape *mShape;
};

struct PosExpectation {
    PosExpectation(QPointF posT, qreal influenceT) {
        pos = posT;
        influence = influenceT;
    }

    QPointF getPosMultByInf() {
        return pos*influence;
    }

    QPointF pos;
    qreal influence;
};

class PathPointAnimators : public ComplexAnimator {
public:
    PathPointAnimators() : ComplexAnimator() {
        setName("point");
    }

    void setAllVars(PathPoint *parentPathPointT,
                    QPointFAnimator *endPosAnimatorT,
                    QPointFAnimator *startPosAnimatorT,
                    QPointFAnimator *pathPointPosAnimatorT,
                    QrealAnimator *influenceAnimatorT,
                    QrealAnimator *influenceTAnimatorT) {
        parentPathPoint = parentPathPointT;
        endPosAnimator = endPosAnimatorT;
        endPosAnimator->setName("ctrl pt 1 pos");
        startPosAnimator = startPosAnimatorT;
        startPosAnimator->setName("ctrl pt 2 pos");
        pathPointPosAnimator = pathPointPosAnimatorT;
        pathPointPosAnimator->setName("point pos");
        influenceAnimatorT->setName("influence");
        influenceAnimatorT->setValueRange(0., 1.);
        influenceAnimatorT->setPrefferedValueStep(0.01);
        influenceTAnimatorT->setName("influence T");
        influenceTAnimatorT->setValueRange(0., 1.);
        influenceTAnimatorT->setPrefferedValueStep(0.01);

        influenceAnimatorT->setCurrentValue(1.);
        influenceTAnimatorT->setCurrentValue(0.5);

        influenceAnimator = influenceAnimatorT;
        influenceAnimator->incNumberPointers();
        influenceTAnimator = influenceTAnimatorT;
        influenceTAnimator->incNumberPointers();

        addChildAnimator(pathPointPosAnimator);
        addChildAnimator(endPosAnimator);
        addChildAnimator(startPosAnimator);
    }

    bool isOfPathPoint(PathPoint *checkPoint) {
        return parentPathPoint == checkPoint;
    }

    void enableInfluenceAnimators() {
        addChildAnimator(influenceAnimator);
        addChildAnimator(influenceTAnimator);
    }

    void disableInfluenceAnimators() {
        removeChildAnimator(influenceAnimator);
        removeChildAnimator(influenceTAnimator);
    }
private:
    PathPoint *parentPathPoint;
    QPointFAnimator *endPosAnimator;
    QPointFAnimator *startPosAnimator;
    QPointFAnimator *pathPointPosAnimator;

    QrealAnimator *influenceAnimator;
    QrealAnimator *influenceTAnimator;
}; 

class PathPoint : public MovablePoint
{
public:
    PathPoint(VectorPath *vectorPath);

    ~PathPoint();

    void startTransform();
    void finishTransform();

    void moveBy(QPointF relTranslation);

    QPointF getShapesInfluencedRelPos();

    QPointF getStartCtrlPtAbsPos() const;
    QPointF getStartCtrlPtValue() const;
    CtrlPoint *getStartCtrlPt();

    QPointF getEndCtrlPtAbsPos();
    QPointF getEndCtrlPtValue() const;
    CtrlPoint *getEndCtrlPt();

    void draw(QPainter *p, CanvasMode mode);

    PathPoint *getNextPoint();
    PathPoint *getPreviousPoint();

    bool isEndPoint();

    void setPointAsPrevious(PathPoint *pointToSet, bool saveUndoRedo = true);
    void setPointAsNext(PathPoint *pointToSet, bool saveUndoRedo = true);
    void setNextPoint(PathPoint *mNextPoint, bool saveUndoRedo = true);
    void setPreviousPoint(PathPoint *mPreviousPoint, bool saveUndoRedo = true);

    bool hasNextPoint();
    bool hasPreviousPoint();

    PathPoint *addPointAbsPos(QPointF absPos);
    PathPoint *addPoint(PathPoint *pointToAdd);

    void connectToPoint(PathPoint *point);
    void disconnectFromPoint(PathPoint *point);

    void remove();
    void removeApproximate();

    MovablePoint *getPointAtAbsPos(QPointF absPos, CanvasMode canvasMode);
    void rectPointsSelection(QRectF absRect, QList<MovablePoint *> *list);
    void updateStartCtrlPtVisibility();
    void updateEndCtrlPtVisibility();

    void setSeparatePathPoint(bool separatePathPoint);
    bool isSeparatePathPoint();

    void setCtrlsMode(CtrlsMode mode, bool saveUndoRedo = true);
    QPointF symmetricToAbsPos(QPointF absPosToMirror);
    QPointF symmetricToAbsPosNewLen(QPointF absPosToMirror, qreal newLen);
    void ctrlPointPosChanged(bool startPtChanged);
    void moveEndCtrlPtToAbsPos(QPointF endCtrlPt);
    void moveStartCtrlPtToAbsPos(QPointF startCtrlPt);
    void moveEndCtrlPtToRelPos(QPointF endCtrlPt);
    void moveStartCtrlPtToRelPos(QPointF startCtrlPt);
    void setCtrlPtEnabled(bool enabled, bool isStartPt, bool saveUndoRedo = true);
    VectorPath *getParentPath();

    void saveToSql(int boundingBoxId);

    void cancelTransform();

    void setEndCtrlPtEnabled(bool enabled);
    void setStartCtrlPtEnabled(bool enabled);

    bool isEndCtrlPtEnabled();
    bool isStartCtrlPtEnabled();

    void setPosAnimatorUpdater(AnimatorUpdater *updater);

    void updateAfterFrameChanged(int frame);

    PathPointAnimators *getPathPointAnimatorsPtr();
    void setPointId(int idT);
    int getPointId();

    qreal getCurrentInfluence();
    qreal getCurrentInfluenceT();

    CtrlsMode getCurrentCtrlsMode();
    bool hasNoInfluence();
    bool hasFullInfluence();
    bool hasSomeInfluence();

    PathPointValues getPointValues() const;
    void savePointValuesToShapeValues(VectorPathShape *shape);

    void clearInfluenceAdjustedPointValues();
    PathPointValues getInfluenceAdjustedPointValues();
    bool updateInfluenceAdjustedPointValues();
    void setInfluenceAdjustedEnd(QPointF newEnd, qreal infl);
    void setInfluenceAdjustedStart(QPointF newStart, qreal infl);
    void finishInfluenceAdjusted();
    void enableInfluenceAnimators();
    void disableInfluenceAnimators();
    QPointF getInfluenceAbsolutePos();
    bool isNeighbourSelected();
    void moveByAbs(QPointF absTranslatione);
    void loadFromSql(int pathPointId, int movablePointId);
    QPointF getInfluenceRelativePos();
    PathPointValues getShapesInfluencedPointValues() const;
    void removeShapeValues(VectorPathShape *shape);

    void setPointValues(const PathPointValues &values);

    void editShape(VectorPathShape *shape);
    void finishEditingShape(VectorPathShape *shape);
    void cancelEditingShape();
private:
    QList<PointShapeValues*> mShapeValues;

    bool mEditingShape = true;
    PathPointValues mBasisShapeSavedValues;

    bool mStartExternalInfluence = false;
    QPointF mStartAdjustedForExternalInfluence;
    bool mEndExternalInfluence = false;
    QPointF mEndAdjustedForExternalInfluence;
    PathPointValues mInfluenceAdjustedPointValues;

    QrealAnimator mInfluenceAnimator;
    QrealAnimator mInfluenceTAnimator;

    int mPointId;
    PathPointAnimators mPathPointAnimators;

    VectorPath *mVectorPath;
    CtrlsMode mCtrlsMode = CtrlsMode::CTRLS_SYMMETRIC;

    bool mSeparatePathPoint = false;
    PathPoint *mNextPoint = NULL;
    PathPoint *mPreviousPoint = NULL;
    bool mStartCtrlPtEnabled = false;
    CtrlPoint *mStartCtrlPt;
    bool mEndCtrlPtEnabled = false;
    CtrlPoint *mEndCtrlPt;
    void ctrlPointPosChanged(CtrlPoint *pointChanged, CtrlPoint *pointToUpdate);
};

#endif // PATHPOINT_H
