#include "canvas.h"
#include "GUI/mainwindow.h"
#include "GUI/canvaswindow.h"
#include "Boxes/paintbox.h"
#include "Boxes/textbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/circle.h"

#include "MovablePoints/pathpivot.h"

void Canvas::mousePressEvent(const MouseEvent &e) {
    if(isPreviewingOrRendering()) return;
    if(mIsMouseGrabbing && e.fButton == Qt::LeftButton) return;
    if(mCurrentMode == PAINT_MODE) {
        if(mStylusDrawing) return;
        if(e.fButton == Qt::LeftButton) {
            paintPress(e.fPos, e.fTimestamp, 0.5, 0, 0);
        }
    } else {
        if(e.fButton == Qt::LeftButton) {
            handleLeftButtonMousePress(e);
        } else if(e.fButton == Qt::RightButton) {
            handleRightButtonMousePress(e.fGlobalPos);
        }
    }
}

void Canvas::mouseMoveEvent(const MouseEvent &e) {
    if(isPreviewingOrRendering()) return;

    const bool leftPressed = e.fButtons & Qt::LeftButton;

    if(!leftPressed && !mIsMouseGrabbing) {
        const auto lastHoveredBox = mHoveredBox;
        const auto lastHoveredPoint = mHoveredPoint_d;
        const auto lastNSegment = mHoveredNormalSegment;

        updateHovered(e);
        return;
    }

    if(mCurrentMode == PAINT_MODE && leftPressed)  {
        paintMove(e.fPos, e.fTimestamp, 1, 0, 0);
        return mActiveWindow->requestUpdate();
    } else if(leftPressed || mIsMouseGrabbing) {
        if(mMovesToSkip > 0) {
            mMovesToSkip--;
            return;
        }
        if(mFirstMouseMove && leftPressed) {
            if((mCurrentMode == CanvasMode::MOVE_POINT &&
                !mHoveredPoint_d && !mHoveredNormalSegment.isValid()) ||
               (mCurrentMode == CanvasMode::MOVE_BOX &&
                !mHoveredBox && !mHoveredPoint_d)) {
                startSelectionAtPoint(e.fPos);
            }
        }
        if(mSelecting) {
            moveSecondSelectionPoint(e.fPos);
        } else if(mCurrentMode == CanvasMode::MOVE_POINT ||
                  mCurrentMode == CanvasMode::ADD_PARTICLE_BOX ||
                  mCurrentMode == CanvasMode::ADD_PAINT_BOX) {
            handleMovePointMouseMove(e);
        } else if(mCurrentMode == CanvasMode::MOVE_BOX) {
            if(!mLastPressedPoint) {
                handleMovePathMouseMove(e);
            } else {
                handleMovePointMouseMove(e);
            }
        } else if(mCurrentMode == CanvasMode::ADD_POINT) {
            handleAddSmartPointMouseMove(e);
        } else if(mCurrentMode == CanvasMode::ADD_CIRCLE) {
            if(isShiftPressed()) {
                const qreal lenR = pointToLen(e.fPos - e.fLastPressPos);
                mCurrentCircle->moveRadiusesByAbs({lenR, lenR});
            } else {
                mCurrentCircle->moveRadiusesByAbs(e.fPos - e.fLastPressPos);
            }
        } else if(mCurrentMode == CanvasMode::ADD_RECTANGLE) {
            if(isShiftPressed()) {
                const QPointF trans = e.fPos - e.fLastPressPos;
                const qreal valF = qMax(trans.x(), trans.y());
                mCurrentRectangle->moveSizePointByAbs({valF, valF});
            } else {
                mCurrentRectangle->moveSizePointByAbs(e.fPos - e.fLastPressPos);
            }
        }
    }
    mFirstMouseMove = false;

    if(!mSelecting && !mIsMouseGrabbing && leftPressed) grabMouseAndTrack();
}

void Canvas::mouseReleaseEvent(const MouseEvent &e) {
    if(isPreviewingOrRendering()) return;
    if(e.fButton != Qt::LeftButton) return;
    schedulePivotUpdate();
    if(mCurrentMode == PAINT_MODE) return;
    if(mValueInput.inputEnabled()) mFirstMouseMove = false;
    mValueInput.clearAndDisableInput();

    handleMouseRelease(e);

    mLastPressedBox = nullptr;
    mHoveredPoint_d = mLastPressedPoint;
    mLastPressedPoint = nullptr;
}

void Canvas::mouseDoubleClickEvent(const MouseEvent &e) {
    if(e.fModifiers & Qt::ShiftModifier) return;
    mDoubleClick = true;

    const auto boxAt = mCurrentBoxesGroup->getBoxAt(e.fPos);
    if(!boxAt) {
        if(!mHoveredPoint_d && !mHoveredNormalSegment.isValid()) {
            if(mCurrentBoxesGroup != this) {
                setCurrentBoxesGroup(mCurrentBoxesGroup->getParentGroup());
            }
        }
    } else {
        if(boxAt->SWT_isContainerBox()) {
            setCurrentBoxesGroup(static_cast<ContainerBox*>(boxAt));
            updateHovered(e);
        } else if((mCurrentMode == MOVE_BOX ||
                   mCurrentMode == MOVE_POINT) &&
                  boxAt->SWT_isTextBox()) {
            releaseMouseAndDontTrack();
            GetAsPtr(boxAt, TextBox)->openTextEditor(mMainWindow);
        } else if(mCurrentMode == MOVE_BOX &&
                  boxAt->SWT_isSmartVectorPath()) {
            mActiveWindow->setCanvasMode(MOVE_POINT);
        }
    }
}

void Canvas::tabletEvent(const QTabletEvent * const e,
                         const QPointF &pos) {
    if(mCurrentMode != PAINT_MODE) return;
    const auto type = e->type();
    if(type == QEvent::TabletRelease ||
       e->buttons() & Qt::MiddleButton) {
        mStylusDrawing = false;
    } else if(e->type() == QEvent::TabletPress) {
        if(e->button() == Qt::RightButton) return;
        if(e->button() == Qt::LeftButton) {
            mStylusDrawing = true;
            paintPress(pos, e->timestamp(), e->pressure(),
                       e->xTilt(), e->yTilt());
        }
    } else if(type == QEvent::TabletMove && mStylusDrawing) {
        paintMove(pos, e->timestamp(), e->pressure(),
                  e->xTilt(), e->yTilt());
    }
}
