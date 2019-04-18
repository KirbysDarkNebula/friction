#include "canvas.h"
#include "MovablePoints/smartnodepoint.h"
#include "Boxes/smartvectorpath.h"

void Canvas::clearCurrentSmartEndPoint() {
    setCurrentSmartEndPoint(nullptr);
}

void Canvas::setCurrentSmartEndPoint(SmartNodePoint * const point) {
    if(mCurrentSmartEndPoint) mCurrentSmartEndPoint->deselect();
    if(point) point->select();
    mCurrentSmartEndPoint = point;
}
#include "MovablePoints/pathpointshandler.h"
#include "Animators/SmartPath/smartpathcollection.h"

void Canvas::handleAddSmartPointMousePress() {
    if(mCurrentSmartEndPoint ? mCurrentSmartEndPoint->isHidden() : false) {
        clearCurrentSmartEndPoint();
    }
    qptr<BoundingBox> test;

    auto nodePointUnderMouse = GetAsPtr(mLastPressedPoint, SmartNodePoint);
    if(nodePointUnderMouse ? !nodePointUnderMouse->isEndPoint() : false) {
        nodePointUnderMouse = nullptr;
    }
    if(nodePointUnderMouse == mCurrentSmartEndPoint &&
            nodePointUnderMouse) return;
    if(!mCurrentSmartEndPoint && !nodePointUnderMouse) {
        const auto newPath = SPtrCreate(SmartVectorPath)();
        mCurrentBoxesGroup->addContainedBox(newPath);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());
        const auto newHandler = newPath->getPathAnimator();
        const auto node = newHandler->createNewSubPathAtPos(
                    mLastMouseEventPosRel);
        setCurrentSmartEndPoint(node);
    } else {
        if(!nodePointUnderMouse) {
            SmartNodePoint * const newPoint =
                    mCurrentSmartEndPoint->actionAddPointAbsPos(mLastMouseEventPosRel);
            //newPoint->startTransform();
            setCurrentSmartEndPoint(newPoint);
        } else if(!mCurrentSmartEndPoint) {
            setCurrentSmartEndPoint(nodePointUnderMouse);
        } else { // mCurrentSmartEndPoint
            const auto targetNode = nodePointUnderMouse->getTargetNode();
            const auto handler = nodePointUnderMouse->getHandler();
            const bool success =
                    mCurrentSmartEndPoint->actionConnectToNormalPoint(
                        nodePointUnderMouse);
            if(success) {
                const int newTargetId = targetNode->getNodeId();
                const auto sel = handler->getPointWithId<SmartNodePoint>(newTargetId);
                setCurrentSmartEndPoint(sel);
            }
        }
    } // pats is not null
}


void Canvas::handleAddSmartPointMouseMove() {
    if(!mCurrentSmartEndPoint) return;
    if(mFirstMouseMove) mCurrentSmartEndPoint->startTransform();
    if(mCurrentSmartEndPoint->hasNextNormalPoint() &&
       mCurrentSmartEndPoint->hasPrevNormalPoint()) {
        if(mCurrentSmartEndPoint->getCtrlsMode() != CtrlsMode::CTRLS_CORNER) {
            mCurrentSmartEndPoint->setCtrlsMode(CtrlsMode::CTRLS_CORNER);
        }
        if(mCurrentSmartEndPoint->isSeparateNodePoint()) {
            mCurrentSmartEndPoint->moveC0ToAbsPos(mLastMouseEventPosRel);
        } else {
            mCurrentSmartEndPoint->moveC2ToAbsPos(mLastMouseEventPosRel);
        }
    } else {
        if(!mCurrentSmartEndPoint->hasNextNormalPoint() &&
           !mCurrentSmartEndPoint->hasPrevNormalPoint()) {
            if(mCurrentSmartEndPoint->getCtrlsMode() != CtrlsMode::CTRLS_CORNER) {
                mCurrentSmartEndPoint->setCtrlsMode(CtrlsMode::CTRLS_CORNER);
            }
        } else {
            if(mCurrentSmartEndPoint->getCtrlsMode() != CtrlsMode::CTRLS_SYMMETRIC) {
                mCurrentSmartEndPoint->setCtrlsMode(CtrlsMode::CTRLS_SYMMETRIC);
            }
        }
        if(mCurrentSmartEndPoint->hasNextNormalPoint()) {
            mCurrentSmartEndPoint->moveC0ToAbsPos(mLastMouseEventPosRel);
        } else {
            mCurrentSmartEndPoint->moveC2ToAbsPos(mLastMouseEventPosRel);
        }
    }
}

void Canvas::handleAddSmartPointMouseRelease() {
    if(mCurrentSmartEndPoint) {
        if(!mFirstMouseMove) mCurrentSmartEndPoint->finishTransform();
        //mCurrentSmartEndPoint->prp_updateInfluenceRangeAfterChanged();
        if(!mCurrentSmartEndPoint->isEndPoint())
            clearCurrentSmartEndPoint();
    }
}
