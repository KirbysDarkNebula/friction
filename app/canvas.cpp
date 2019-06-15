﻿#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include "undoredo.h"
#include "GUI/mainwindow.h"
#include "MovablePoints/pathpivot.h"
#include "Boxes/imagebox.h"
#include "Sound/soundcomposition.h"
#include "Boxes/textbox.h"
#include "GUI/BoxesList/OptimalScrollArea/scrollwidgetvisiblepart.h"
#include "Sound/singlesound.h"
#include "global.h"
#include "pointhelpers.h"
#include "Boxes/linkbox.h"
#include "clipboardcontainer.h"
#include "Boxes/paintbox.h"
#include <QFile>
#include "renderinstancesettings.h"
#include "videoencoder.h"
#include "PropertyUpdaters/displayedfillstrokesettingsupdater.h"
#include "Animators/effectanimators.h"
#include "PixmapEffects/pixmapeffect.h"
#include "PixmapEffects/rastereffects.h"
#include "MovablePoints/smartnodepoint.h"
#include "Boxes/internallinkcanvas.h"
#include "pointtypemenu.h"
#include "Animators/transformanimator.h"
#include "GUI/canvaswindow.h"

Canvas::Canvas(CanvasWindow * const canvasWidget,
               const int canvasWidth, const int canvasHeight,
               const int frameCount, const qreal fps) :
    ContainerBox(TYPE_CANVAS) {
    mMainWindow = MainWindow::getInstance();
    setCurrentBrush(mMainWindow->getCurrentBrush());
    std::function<bool(int)> changeFrameFunc =
    [this](const int undoRedoFrame) {
        if(undoRedoFrame != mMainWindow->getCurrentFrame()) {
            mMainWindow->setCurrentFrame(undoRedoFrame);
            return true;
        }
        return false;
    };
    mUndoRedoStack = SPtrCreate(UndoRedoStack)(changeFrameFunc);
    mFps = fps;
    connect(this, &Canvas::nameChanged, this, &Canvas::emitCanvasNameChanged);
    mBackgroundColor->qra_setCurrentValue(QColor(75, 75, 75));
    ca_addChildAnimator(mBackgroundColor);
    mBackgroundColor->prp_setInheritedUpdater(
                SPtrCreate(DisplayedFillStrokeSettingsUpdater)(this));
    mSoundComposition = qsptr<SoundComposition>::create(this);
    auto soundsAnimatorContainer = mSoundComposition->getSoundsAnimatorContainer();
    ca_addChildAnimator(GetAsSPtr(soundsAnimatorContainer, Property));

    mMaxFrame = frameCount;

    mResolutionFraction = 1;

    mWidth = canvasWidth;
    mHeight = canvasHeight;
    mActiveWindow = canvasWidget;
    mActiveWidget = mActiveWindow->getCanvasWidget();

    mCurrentBoxesGroup = this;
    mIsCurrentGroup = true;

    mRotPivot = SPtrCreate(PathPivot)(this);

    mTransformAnimator->SWT_hide();

    //anim_setAbsFrame(0);

    //setCanvasMode(MOVE_PATH);
}

QRectF Canvas::getRelBoundingRect(const qreal ) {
    return QRectF(0, 0, mWidth, mHeight);
}

void Canvas::emitCanvasNameChanged() {
    emit canvasNameChanged(this, prp_mName);
}

qreal Canvas::getResolutionFraction() {
    return mResolutionFraction;
}

void Canvas::setResolutionFraction(const qreal percent) {
    mResolutionFraction = percent;
}

void Canvas::setCurrentGroupParentAsCurrentGroup() {
    setCurrentBoxesGroup(mCurrentBoxesGroup->getParentGroup());
}

#include "GUI/BoxesList/boxscrollwidget.h"
void Canvas::setCurrentBoxesGroup(ContainerBox * const group) {
    if(mCurrentBoxesGroup) {
        mCurrentBoxesGroup->setIsCurrentGroup_k(false);
    }
    clearBoxesSelection();
    clearPointsSelection();
    clearCurrentSmartEndPoint();
    clearLastPressedPoint();
    mCurrentBoxesGroup = group;
    group->setIsCurrentGroup_k(true);

    //mMainWindow->getObjectSettingsList()->setMainTarget(mCurrentBoxesGroup);
    SWT_scheduleContentUpdate(mCurrentBoxesGroup,
                                               SWT_TARGET_CURRENT_GROUP);
}

void Canvas::updateHoveredBox(const MouseEvent& e) {
    mHoveredBox = mCurrentBoxesGroup->getBoxAt(e.fPos);
}

void Canvas::updateHoveredPoint(const MouseEvent& e) {
    mHoveredPoint_d = getPointAtAbsPos(e.fPos, e.fMode, 1/e.fScale);
}

void Canvas::updateHoveredEdge(const MouseEvent& e) {
    if(e.fMode != MOVE_POINT || mHoveredPoint_d)
        return mHoveredNormalSegment.clear();
    mHoveredNormalSegment = getSegment(e);
    if(mHoveredNormalSegment.isValid())
        mHoveredNormalSegment.generateSkPath();
}

void Canvas::clearHovered() {
    mHoveredBox.clear();
    mHoveredPoint_d.clear();
    mHoveredNormalSegment.clear();
}

void Canvas::updateHovered(const MouseEvent& e) {
    updateHoveredPoint(e);
    updateHoveredEdge(e);
    updateHoveredBox(e);
}

void Canvas::drawTransparencyMesh(SkCanvas * const canvas,
                                  const SkRect &viewRect,
                                  const qreal scale) {
    if(mBackgroundColor->getColor().alpha() != 255) {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kFill_Style);
        paint.setColor(SkColorSetARGB(125, 255, 255, 255));
        SkScalar currX = viewRect.left();
        SkScalar currY = viewRect.top();
        SkScalar widthT = static_cast<SkScalar>(
                    MIN_WIDGET_HEIGHT*0.5*scale);
        SkScalar heightT = widthT;
        bool isOdd = false;
        while(currY < viewRect.bottom()) {
            widthT = heightT;
            if(currY + heightT > viewRect.bottom()) {
                heightT = viewRect.bottom() - currY;
            }
            currX = viewRect.left();
            if(isOdd) currX += widthT;

            while(currX < viewRect.right()) {
                if(currX + widthT > viewRect.right()) {
                    widthT = viewRect.right() - currX;
                }
                canvas->drawRect(SkRect::MakeXYWH(currX, currY,
                                                  widthT, heightT),
                                 paint);
                currX += 2*widthT;
            }

            isOdd = !isOdd;
            currY += heightT;
        }
    }
}

void Canvas::renderSk(SkCanvas * const canvas,
                      GrContext* const grContext,
                      const QRect& drawRect,
                      const QMatrix& viewTrans) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    const qreal scale = viewTrans.m11();
    const SkRect canvasRect = SkRect::MakeWH(mWidth, mHeight);

    canvas->concat(toSkMatrix(viewTrans));
    const SkScalar reversedRes = toSkScalar(1/mResolutionFraction);
    if(isPreviewingOrRendering()) {
        if(mCurrentPreviewContainer) {
            canvas->save();
            drawTransparencyMesh(canvas, canvasRect, scale);
            canvas->scale(reversedRes, reversedRes);
            mCurrentPreviewContainer->drawSk(canvas, grContext);
            canvas->restore();
        }
    } else {
        if(!mClipToCanvasSize) {
            paint.setColor(SkColorSetARGB(255, 75, 75, 75));
            const auto bgRect = getMaxBoundsRect();
            canvas->drawRect(toSkRect(bgRect), paint);
        }
        const bool drawCanvas = mCurrentPreviewContainer &&
                !mCurrentPreviewContainerOutdated;
        drawTransparencyMesh(canvas, canvasRect, scale);

        if(!mClipToCanvasSize || !drawCanvas) {
            canvas->saveLayer(nullptr, nullptr);
            paint.setColor(toSkColor(mBackgroundColor->getColor()));
            canvas->drawRect(canvasRect, paint);
            for(const auto& box : mContainedBoxes) {
                if(box->isVisibleAndInVisibleDurationRect())
                    box->drawPixmapSk(canvas, grContext);
            }
            canvas->restore();
        }
        if(drawCanvas) {
            canvas->save();
            canvas->scale(reversedRes, reversedRes);
            mCurrentPreviewContainer->drawSk(canvas, grContext);
            canvas->restore();
        }

        const qreal qInvZoom = 1/viewTrans.m11();
        const SkScalar invZoom = toSkScalar(qInvZoom);
        if(!mCurrentBoxesGroup->SWT_isCanvas())
            mCurrentBoxesGroup->drawBoundingRect(canvas, invZoom);
        if(!mPaintDrawableBox) {
            for(const auto& box : mSelectedBoxes) {
                canvas->save();
                box->drawBoundingRect(canvas, invZoom);
                box->drawAllCanvasControls(canvas, mCurrentMode, invZoom);
                canvas->restore();
            }
        }

        if(mCurrentMode == CanvasMode::MOVE_BOX ||
           mCurrentMode == CanvasMode::MOVE_POINT) {

            if(mTransMode == MODE_ROTATE || mTransMode == MODE_SCALE) {
                mRotPivot->drawTransforming(canvas, mCurrentMode, invZoom,
                                            MIN_WIDGET_HEIGHT*0.25f*invZoom);
            } else if(!mIsMouseGrabbing || mRotPivot->isSelected()) {
                mRotPivot->drawSk(canvas, mCurrentMode, invZoom, false);
            }
        }

        if(mPaintDrawableBox) {
            const auto canvasRect = viewTrans.inverted().mapRect(drawRect);
            const auto pDrawTrans = mPaintDrawableBox->getTotalTransform();
            const auto relDRect = pDrawTrans.inverted().mapRect(canvasRect);
            canvas->concat(toSkMatrix(pDrawTrans));
            mPaintOnion.draw(canvas);
            mPaintDrawable->drawOnCanvas(canvas, {0, 0}, &relDRect);
        } else {
            if(mSelecting) {
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setColor(SkColorSetARGB(255, 0, 0, 255));
                paint.setStrokeWidth(2*invZoom);
                const SkScalar intervals[2] = {MIN_WIDGET_HEIGHT*0.25f*invZoom,
                                               MIN_WIDGET_HEIGHT*0.25f*invZoom};
                paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
                canvas->drawRect(toSkRect(mSelectionRect), paint);
                paint.setPathEffect(nullptr);
            }

            if(mHoveredPoint_d) {
                mHoveredPoint_d->drawHovered(canvas, invZoom);
            } else if(mHoveredNormalSegment.isValid()) {
                mHoveredNormalSegment.drawHoveredSk(canvas, invZoom);
            } else if(mHoveredBox) {
                if(!mCurrentNormalSegment.isValid()) {
                    mHoveredBox->drawHoveredSk(canvas, invZoom);
                }
            }
        }

        canvas->resetMatrix();

        if(!mClipToCanvasSize) {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(2);
            paint.setColor(SK_ColorBLACK);
            canvas->drawRect(canvasRect.makeInset(1, 1), paint);
        }
        if(mTransMode != MODE_NONE || mValueInput.inputEnabled())
            mValueInput.draw(canvas, drawRect.height() - MIN_WIDGET_HEIGHT);
    }
}

void Canvas::setMaxFrame(const int frame) {
    mMaxFrame = frame;
}

stdsptr<BoundingBoxRenderData> Canvas::createRenderData() {
    return SPtrCreate(CanvasRenderData)(this);
}

const SimpleBrushWrapper *Canvas::getCurrentBrush() const {
    return mCurrentBrush;
}

void Canvas::setCurrentBrush(const SimpleBrushWrapper * const brush) {
    mCurrentBrush = brush;
    if(brush) brush->setColor(mCurrentBrushColor);
}

void Canvas::setBrushColor(const QColor &color) {
    mCurrentBrushColor = color;
    mCurrentBrushColor.setBlueF(color.redF());
    mCurrentBrushColor.setRedF(color.blueF());
    if(!mCurrentBrush) return;
    mCurrentBrush->setColor(mCurrentBrushColor);
}

void Canvas::incBrushRadius() {
    mCurrentBrush->incPaintBrushSize(0.3);
}

void Canvas::decBrushRadius() {
    mCurrentBrush->decPaintBrushSize(0.3);
}

QSize Canvas::getCanvasSize() {
    return QSize(mWidth, mHeight);
}

void Canvas::setPreviewing(const bool bT) {
    mPreviewing = bT;
}

void Canvas::setRenderingPreview(const bool bT) {
    mRenderingPreview = bT;
}

void Canvas::anim_scaleTime(const int pivotAbsFrame, const qreal scale) {
    ContainerBox::anim_scaleTime(pivotAbsFrame, scale);
    //        int newAbsPos = qRound(scale*pivotAbsFrame);
    //        anim_shiftAllKeys(newAbsPos - pivotAbsFrame);
    setMaxFrame(qRound((mMaxFrame - pivotAbsFrame)*scale));
    mActiveWindow->setCurrentCanvas(this);
}

void Canvas::setOutputRendering(const bool bT) {
    mRenderingOutput = bT;
}

void Canvas::setCurrentPreviewContainer(const int relFrame) {
    auto cont = mCacheHandler.atFrame(relFrame);
    setCurrentPreviewContainer(GetAsSPtr(cont, ImageCacheContainer));
}

void Canvas::setCurrentPreviewContainer(const stdsptr<ImageCacheContainer>& cont) {
    setLoadingPreviewContainer(nullptr);
    if(cont == mCurrentPreviewContainer) return;
    if(mCurrentPreviewContainer) {
        if(!mRenderingPreview)
            mCurrentPreviewContainer->setBlocked(false);
    }
    if(!cont) {
        mCurrentPreviewContainer.reset();
        return;
    }
    mCurrentPreviewContainer = cont;
    mCurrentPreviewContainer->setBlocked(true);
}

void Canvas::setLoadingPreviewContainer(
        const stdsptr<ImageCacheContainer>& cont) {
    if(cont == mLoadingPreviewContainer) return;
    if(mLoadingPreviewContainer.get()) {
        mLoadingPreviewContainer->setLoadTargetCanvas(nullptr);
        if(!mRenderingPreview || mRenderingOutput) {
            mLoadingPreviewContainer->setBlocked(false);
        }
    }
    if(!cont) {
        mLoadingPreviewContainer.reset();
        return;
    }
    mLoadingPreviewContainer = cont;
    cont->setLoadTargetCanvas(this);
    if(!cont->storesDataInMemory()) {
        cont->scheduleLoadFromTmpFile();
    }
    mLoadingPreviewContainer->setBlocked(true);
}

FrameRange Canvas::prp_getIdenticalRelRange(const int relFrame) const {
    const auto groupRange = ContainerBox::prp_getIdenticalRelRange(relFrame);
    //FrameRange canvasRange{0, mMaxFrame};
    return groupRange;//*canvasRange;
}

void Canvas::renderDataFinished(BoundingBoxRenderData *renderData) {
    if(renderData->fBoxStateId < mLastStateId) return;
    mLastStateId = renderData->fBoxStateId;
    const auto range = prp_getIdenticalRelRange(renderData->fRelFrame);
    auto cont = mCacheHandler.atFrame<ImageCacheContainer>(range.fMin);
    if(cont) {
        cont->replaceImageSk(renderData->fRenderedImage);
        cont->setRange(range);
    } else {
        const auto sCont = SPtrCreate(ImageCacheContainer)(
                    renderData->fRenderedImage, range, &mCacheHandler);
        mCacheHandler.add(sCont);
        cont = sCont.get();
    }
    if((mPreviewing || mRenderingOutput) &&
       mCurrRenderRange.inRange(renderData->fRelFrame)) {
        cont->setBlocked(true);
    } else {
        auto currentRenderData = mDrawRenderContainer.getSrcRenderData();
        bool newerSate = true;
        bool closerFrame = true;
        if(currentRenderData) {
            newerSate = currentRenderData->fBoxStateId < renderData->fBoxStateId;
            const int finishedFrameDist =
                    qAbs(anim_getCurrentRelFrame() - renderData->fRelFrame);
            const int oldFrameDist =
                    qAbs(anim_getCurrentRelFrame() - currentRenderData->fRelFrame);
            closerFrame = finishedFrameDist < oldFrameDist;
        }
        if(newerSate || closerFrame) {
            mDrawRenderContainer.setSrcRenderData(renderData);
            const bool currentState =
                    renderData->fBoxStateId == mStateId;
            const bool currentFrame =
                    renderData->fRelFrame == anim_getCurrentRelFrame();
            mDrawRenderContainer.setExpired(!currentState || !currentFrame);
            mCurrentPreviewContainerOutdated =
                    mDrawRenderContainer.isExpired();
            setCurrentPreviewContainer(GetAsSPtr(cont, ImageCacheContainer));
        } else if(mRenderingPreview &&
                  mCurrRenderRange.inRange(renderData->fRelFrame)) {
            cont->setBlocked(true);
        }
    }
}

void Canvas::prp_afterChangedAbsRange(const FrameRange &range) {
    Property::prp_afterChangedAbsRange(range);
    const int minId = prp_getIdenticalRelRange(range.fMin).fMin;
    const int maxId = prp_getIdenticalRelRange(range.fMax).fMax;
    mCacheHandler.remove({minId, maxId});
    if(!mCacheHandler.atFrame(anim_getCurrentRelFrame())) {
        mCurrentPreviewContainerOutdated = true;
        planScheduleUpdate(Animator::USER_CHANGE);
    }
}

qsptr<BoundingBox> Canvas::createLink() {
    return SPtrCreate(InternalLinkCanvas)(this);
}

ImageBox *Canvas::createImageBox(const QString &path) {
    const auto img = SPtrCreate(ImageBox)(path);
    img->planCenterPivotPosition();
    mCurrentBoxesGroup->addContainedBox(img);
    return img.get();
}

#include "Boxes/imagesequencebox.h"
ImageSequenceBox* Canvas::createAnimationBoxForPaths(const QStringList &paths) {
    const auto aniBox = SPtrCreate(ImageSequenceBox)();
    aniBox->planCenterPivotPosition();
    aniBox->setListOfFrames(paths);
    mCurrentBoxesGroup->addContainedBox(aniBox);
    return aniBox.get();
}

#include "Boxes/videobox.h"
VideoBox* Canvas::createVideoForPath(const QString &path) {
    const auto vidBox = SPtrCreate(VideoBox)();
    vidBox->planCenterPivotPosition();
    vidBox->setFilePath(path);
    mCurrentBoxesGroup->addContainedBox(vidBox);
    return vidBox.get();
}

#include "Boxes/linkbox.h"
ExternalLinkBox* Canvas::createLinkToFileWithPath(const QString &path) {
    const auto extLinkBox = SPtrCreate(ExternalLinkBox)();
    extLinkBox->setSrc(path);
    mCurrentBoxesGroup->addContainedBox(extLinkBox);
    return extLinkBox.get();
}

SingleSound* Canvas::createSoundForPath(const QString &path) {
    const auto singleSound = SPtrCreate(SingleSound)();
    getSoundComposition()->addSoundAnimator(singleSound);
    singleSound->setFilePath(path);
    return singleSound.get();
}

void Canvas::scheduleDisplayedFillStrokeSettingsUpdate() {
    mMainWindow->scheduleDisplayedFillStrokeSettingsUpdate();
}

void Canvas::schedulePivotUpdate() {
    if(mTransMode == MODE_ROTATE ||
       mTransMode == MODE_SCALE ||
       mRotPivot->isSelected()) return;
    mPivotUpdateNeeded = true;
}

void Canvas::updatePivotIfNeeded() {
    if(mPivotUpdateNeeded) {
        mPivotUpdateNeeded = false;
        updatePivot();
    }
}

void Canvas::makePointCtrlsSymmetric() {
    setPointCtrlsMode(CtrlsMode::CTRLS_SYMMETRIC);
}

void Canvas::makePointCtrlsSmooth() {
    setPointCtrlsMode(CtrlsMode::CTRLS_SMOOTH);
}

void Canvas::makePointCtrlsCorner() {
    setPointCtrlsMode(CtrlsMode::CTRLS_CORNER);
}

void Canvas::makeSegmentLine() {
    makeSelectedPointsSegmentsLines();
}

void Canvas::makeSegmentCurve() {
    makeSelectedPointsSegmentsCurves();
}

void Canvas::moveSecondSelectionPoint(const QPointF &pos) {
    mSelectionRect.setBottomRight(pos);
}

void Canvas::startSelectionAtPoint(const QPointF &pos) {
    mSelecting = true;
    mSelectionRect.setTopLeft(pos);
    mSelectionRect.setBottomRight(pos);
}

void Canvas::updatePivot() {
    if(mCurrentMode == MOVE_POINT) {
        mRotPivot->setAbsolutePos(getSelectedPointsAbsPivotPos());
    } else if(mCurrentMode == MOVE_BOX) {
        mRotPivot->setAbsolutePos(getSelectedBoxesAbsPivotPos());
    }
}

void Canvas::setCanvasMode(const CanvasMode mode) {
    mCurrentMode = mode;
    mSelecting = false;
    clearPointsSelection();
    clearCurrentSmartEndPoint();
    clearLastPressedPoint();
    updatePivot();
    updatePaintBox();
}

void Canvas::afterPaintAnimSurfaceChanged() {
    if(mPaintPressedSinceUpdate && mPaintAnimSurface) {
        mPaintAnimSurface->prp_afterChangedRelRange(
                    mPaintAnimSurface->prp_getIdenticalRelRange(
                        mPaintAnimSurface->anim_getCurrentRelFrame()));
        mPaintPressedSinceUpdate = false;
    }
}

void Canvas::setPaintDrawable(DrawableAutoTiledSurface * const surf) {
    mPaintDrawable = surf;
    mPaintPressedSinceUpdate = false;
    mPaintAnimSurface->setupOnionSkinFor(20, mPaintOnion);
}

void Canvas::setPaintBox(PaintBox * const box) {
    if(box == mPaintDrawableBox) return;
    if(mPaintDrawableBox) {
        //mPaintDrawableBox->setVisibile(mPaintBoxWasVisible);
        disconnect(mPaintAnimSurface, nullptr, this, nullptr);
        afterPaintAnimSurfaceChanged();
    }
    if(box) {
        mPaintDrawableBox = box;
        mPaintBoxWasVisible = mPaintDrawableBox->isVisible();
        //mPaintDrawableBox->hide();
        mPaintAnimSurface = mPaintDrawableBox->getSurface();
        connect(mPaintAnimSurface, &AnimatedSurface::currentSurfaceChanged,
                this, &Canvas::setPaintDrawable);
        setPaintDrawable(mPaintAnimSurface->getCurrentSurface());
    } else {
        mPaintDrawableBox = nullptr;
        mPaintAnimSurface = nullptr;
        mPaintDrawable = nullptr;
    }
}

void Canvas::updatePaintBox() {
    setPaintBox(nullptr);
    if(mCurrentMode != PAINT_MODE) return;
    for(int i = mSelectedBoxes.count() - 1; i >= 0; i--) {
        const auto& iBox = mSelectedBoxes.at(i);
        if(iBox->SWT_isPaintBox()) {
            setPaintBox(GetAsPtr(iBox, PaintBox));
            break;
        }
    }
}

void Canvas::grabMouseAndTrack() {
    mIsMouseGrabbing = true;
    mActiveWindow->grabMouse();
}

void Canvas::releaseMouseAndDontTrack() {
    mTransMode = MODE_NONE;
    mIsMouseGrabbing = false;
    mActiveWindow->releaseMouse();
}

bool Canvas::handlePaintModeKeyPress(const KeyEvent &e) {
    if(mCurrentMode != PAINT_MODE) return false;
    if(e.fKey == Qt::Key_N && mPaintAnimSurface) {
        mPaintAnimSurface->newEmptyFrame();
    } else return false;
    return true;
}

bool Canvas::handleTransormationInputKeyEvent(const KeyEvent &e) {
    if(mValueInput.handleTransormationInputKeyEvent(e.fKey)) {
        if(mTransMode == MODE_ROTATE)
            mValueInput.setupRotate();
        updateTransformation(e);
    } else if(e.fKey == Qt::Key_Escape) {
        cancelCurrentTransform();
    } else if(e.fKey == Qt::Key_Return ||
              e.fKey == Qt::Key_Enter) {
        handleMouseRelease(e);
        mValueInput.clearAndDisableInput();
    } else if(e.fKey == Qt::Key_X) {
        mValueInput.switchXOnlyMode();
        updateTransformation(e);
    } else if(e.fKey == Qt::Key_Y) {
        mValueInput.switchYOnlyMode();
        updateTransformation(e);
    } else {
        return false;
    }

    return true;
}

void Canvas::deleteAction() {
    if(mCurrentMode == MOVE_POINT) {
        removeSelectedPointsAndClearList();
    } else if(mCurrentMode == MOVE_BOX) {
        removeSelectedBoxesAndClearList();
    }
}

void Canvas::copyAction() {
    const auto container = SPtrCreate(BoxesClipboardContainer)();
    mMainWindow->replaceClipboard(container);
    QBuffer target(container->getBytesArray());
    target.open(QIODevice::WriteOnly);
    const int nBoxes = mSelectedBoxes.count();
    target.write(rcConstChar(&nBoxes), sizeof(int));

    for(const auto& box : mSelectedBoxes) {
        box->writeBoundingBox(&target);
    }
    target.close();

    BoundingBox::sClearWriteBoxes();
}

void Canvas::pasteAction() {
    const auto container =
            static_cast<BoxesClipboardContainer*>(
            mMainWindow->getClipboardContainer(CCT_BOXES));
    if(!container) return;
    clearBoxesSelection();
    container->pasteTo(mCurrentBoxesGroup);
}

void Canvas::cutAction() {
    copyAction();
    deleteAction();
}

void Canvas::duplicateAction() {
    copyAction();
    pasteAction();
}

void Canvas::selectAllAction() {
    if(mCurrentMode == MOVE_POINT) {
        selectAllPointsAction();
    } else {//if(mCurrentMode == MOVE_PATH) {
        selectAllBoxesFromBoxesGroup();
    }
}

void Canvas::invertSelectionAction() {
    if(mCurrentMode == MOVE_POINT) {
        QList<MovablePoint*> selectedPts = mSelectedPoints_d;
        selectAllPointsAction();
        for(const auto& pt : selectedPts) {
            removePointFromSelection(pt);
        }
    } else {//if(mCurrentMode == MOVE_PATH) {
        QList<BoundingBox*> boxes = mSelectedBoxes;
        selectAllBoxesFromBoxesGroup();
        for(const auto& box : boxes) {
            box->removeFromSelection();
        }
    }
}

void Canvas::anim_setAbsFrame(const int frame) {
    if(frame == anim_getCurrentAbsFrame()) return;
    afterPaintAnimSurfaceChanged();
    const int oldRelFrame = anim_getCurrentRelFrame();
    ComplexAnimator::anim_setAbsFrame(frame);
    const int newRelFrame = anim_getCurrentRelFrame();

    const auto cont = mCacheHandler.atFrame<ImageCacheContainer>(newRelFrame);
    if(cont) {
        if(cont->storesDataInMemory()) { // !!!
            setCurrentPreviewContainer(GetAsSPtr(cont, ImageCacheContainer));
        } else {// !!!
            setLoadingPreviewContainer(GetAsSPtr(cont, ImageCacheContainer));
        }// !!!
        mCurrentPreviewContainerOutdated = !cont->storesDataInMemory();
    } else {
        const bool difference =
                prp_differencesBetweenRelFrames(oldRelFrame, newRelFrame);
        if(difference) {
            mCurrentPreviewContainerOutdated = true;
        }
        if(difference) planScheduleUpdate(Animator::FRAME_CHANGE);
    }

    for(const auto &box : mContainedBoxes)
        box->anim_setAbsFrame(frame);
    mUndoRedoStack->setFrame(frame);

    if(mCurrentMode == PAINT_MODE)
        mPaintAnimSurface->setupOnionSkinFor(20, mPaintOnion);
}

void Canvas::clearSelectionAction() {
    if(mCurrentMode == MOVE_POINT) {
        clearPointsSelection();
    } else {//if(mCurrentMode == MOVE_PATH) {
        clearPointsSelection();
        clearBoxesSelection();
    }
}

void Canvas::clearParentForSelected() {
    for(int i = 0; i < mSelectedBoxes.count(); i++) {
        mSelectedBoxes.at(i)->clearParent();
    }
}

void Canvas::setParentToLastSelected() {
    if(mSelectedBoxes.count() > 1) {
        const auto& lastBox = mSelectedBoxes.last();
        const auto trans = lastBox->getTransformAnimator();
        for(int i = 0; i < mSelectedBoxes.count() - 1; i++) {
            mSelectedBoxes.at(i)->setParentTransform(trans);
        }
    }
}

bool Canvas::startRotatingAction(const KeyEvent &e) {
    if(e.fMode != MOVE_BOX &&
       e.fMode != MOVE_POINT) return false;
    if(mSelectedBoxes.isEmpty()) return false;
    if(e.fMode == MOVE_POINT) {
        if(mSelectedPoints_d.isEmpty()) return false;
    }
    mTransformationFinishedBeforeMouseRelease = false;
    mValueInput.clearAndDisableInput();
    mValueInput.setupRotate();

    mRotPivot->setMousePos(e.fPos);
    mTransMode = MODE_ROTATE;
    mRotHalfCycles = 0;
    mLastDRot = 0;

    mDoubleClick = false;
    mFirstMouseMove = true;

    grabMouseAndTrack();
    return true;
}

bool Canvas::startScalingAction(const KeyEvent &e) {
    if(e.fMode != MOVE_BOX &&
       e.fMode != MOVE_POINT) return false;

    if(mSelectedBoxes.isEmpty()) return false;
    if(e.fMode == MOVE_POINT) {
        if(mSelectedPoints_d.isEmpty()) return false;
    }
    mTransformationFinishedBeforeMouseRelease = false;
    mValueInput.clearAndDisableInput();
    mValueInput.setupScale();

    mRotPivot->setMousePos(e.fPos);
    mTransMode = MODE_SCALE;
    mDoubleClick = false;
    mFirstMouseMove = true;

    grabMouseAndTrack();
    return true;
}

bool Canvas::startMovingAction(const KeyEvent &e) {
    if(e.fMode != MOVE_BOX &&
       e.fMode != MOVE_POINT) return false;
    mTransformationFinishedBeforeMouseRelease = false;
    mValueInput.clearAndDisableInput();
    mValueInput.setupMove();

    mTransMode = MODE_MOVE;
    mDoubleClick = false;
    mFirstMouseMove = true;

    grabMouseAndTrack();
    return true;
}

void Canvas::selectAllBoxesAction() {
    mCurrentBoxesGroup->selectAllBoxesFromBoxesGroup();
}

void Canvas::deselectAllBoxesAction() {
    mCurrentBoxesGroup->deselectAllBoxesFromBoxesGroup();
}

void Canvas::selectAllPointsAction() {
    for(const auto& box : mSelectedBoxes)
        box->selectAllCanvasPts(mSelectedPoints_d, mCurrentMode);
}

void Canvas::selectOnlyLastPressedBox() {
    clearBoxesSelection();
    if(mLastPressedBox)
        addBoxToSelection(mLastPressedBox);
}

void Canvas::selectOnlyLastPressedPoint() {
    clearPointsSelection();
    if(mLastPressedPoint)
        addPointToSelection(mLastPressedPoint);
}

//void Canvas::updateAfterFrameChanged(const int currentFrame) {
//    anim_mCurrentAbsFrame = currentFrame;

//    for(const auto& box : mChildBoxes) {
//        box->anim_setAbsFrame(currentFrame);
//    }

//    BoxesGroup::anim_setAbsFrame(currentFrame);
//    //mSoundComposition->getSoundsAnimatorContainer()->anim_setAbsFrame(currentFrame);
//}

bool Canvas::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                 const bool parentSatisfies,
                                 const bool parentMainTarget) const {
    Q_UNUSED(parentSatisfies);
    Q_UNUSED(parentMainTarget);
    const SWT_BoxRule &rule = rules.fRule;
    const bool alwaysShowChildren = rules.fAlwaysShowChildren;
    if(alwaysShowChildren) {
        return false;
    } else {
        if(rules.fType == SWT_TYPE_SOUND) return false;

        if(rule == SWT_BR_ALL) {
            return true;
        } else if(rule == SWT_BR_SELECTED) {
            return false;
        } else if(rule == SWT_BR_ANIMATED) {
            return false;
        } else if(rule == SWT_BR_NOT_ANIMATED) {
            return false;
        } else if(rule == SWT_BR_VISIBLE) {
            return true;
        } else if(rule == SWT_BR_HIDDEN) {
            return false;
        } else if(rule == SWT_BR_LOCKED) {
            return false;
        } else if(rule == SWT_BR_UNLOCKED) {
            return true;
        }
    }
    return false;
}

void Canvas::setIsCurrentCanvas(const bool bT) {
    mIsCurrentCanvas = bT;
}

int Canvas::getCurrentFrame() {
    return anim_getCurrentAbsFrame();
}

int Canvas::getFrameCount() {
    return mMaxFrame + 1;
}

int Canvas::getMaxFrame() {
    return mMaxFrame;
}

HDDCachableCacheHandler &Canvas::getSoundCacheHandler() {
    return mSoundComposition->getCacheHandler();
}

void Canvas::startDurationRectPosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->startDurationRectPosTransform();
}

void Canvas::finishDurationRectPosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->finishDurationRectPosTransform();
}

void Canvas::moveDurationRectForAllSelected(const int dFrame) {
    for(const auto& box : mSelectedBoxes)
        box->moveDurationRect(dFrame);
}

void Canvas::startMinFramePosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->startMinFramePosTransform();
}

void Canvas::finishMinFramePosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->finishMinFramePosTransform();
}

void Canvas::moveMinFrameForAllSelected(const int dFrame) {
    for(const auto& box : mSelectedBoxes)
        box->moveMinFrame(dFrame);
}

void Canvas::startMaxFramePosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->startMaxFramePosTransform();
}

void Canvas::finishMaxFramePosTransformForAllSelected() {
    for(const auto& box : mSelectedBoxes)
        box->finishMaxFramePosTransform();
}

void Canvas::moveMaxFrameForAllSelected(const int dFrame) {
    for(const auto& box : mSelectedBoxes)
        box->moveMaxFrame(dFrame);
}

void Canvas::blockUndoRedo() {
    mUndoRedoStack->blockUndoRedo();
}

void Canvas::unblockUndoRedo() {
    mUndoRedoStack->unblockUndoRedo();
}

bool Canvas::isShiftPressed() {
    return mMainWindow->isShiftPressed();
}

bool Canvas::isShiftPressed(QKeyEvent *event) {
    return event->modifiers() & Qt::ShiftModifier;
}

bool Canvas::isCtrlPressed() {
    return mMainWindow->isCtrlPressed();
}

bool Canvas::isCtrlPressed(QKeyEvent *event) {
    return event->modifiers() & Qt::ControlModifier;
}

bool Canvas::isAltPressed() {
    return mMainWindow->isAltPressed();
}

bool Canvas::isAltPressed(QKeyEvent *event) {
    return event->modifiers() & Qt::AltModifier;
}

void Canvas::paintPress(const QPointF& pos,
                        const ulong ts, const qreal pressure,
                        const qreal xTilt, const qreal yTilt) {
    if(mPaintAnimSurface) {
        if(mPaintAnimSurface->anim_isRecording())
            mPaintAnimSurface->anim_saveCurrentValueAsKey();
    }
    mPaintPressedSinceUpdate = true;

    if(mPaintDrawable && mCurrentBrush) {
        const auto& target = mPaintDrawable->surface();
        const auto pDrawTrans = mPaintDrawableBox->getTotalTransform();
        const auto drawPos = pDrawTrans.inverted().map(pos);
        const auto roi =
                target.paintPressEvent(mCurrentBrush->getBrush(),
                                       drawPos, 1, pressure, xTilt, yTilt);
        const QRect qRoi(roi.x, roi.y, roi.width, roi.height);
        mPaintDrawable->pixelRectChanged(qRoi);
        mLastTs = ts;
    }
}

void Canvas::paintMove(const QPointF& pos,
                       const ulong ts, const qreal pressure,
                       const qreal xTilt, const qreal yTilt) {
    if(mPaintDrawable && mCurrentBrush) {
        const auto& target = mPaintDrawable->surface();
        const double dt = (ts - mLastTs);
        const auto pDrawTrans = mPaintDrawableBox->getTotalTransform();
        const auto drawPos = pDrawTrans.inverted().map(pos);
        const auto roi =
                target.paintMoveEvent(mCurrentBrush->getBrush(),
                                      drawPos, dt/1000, pressure,
                                      xTilt, yTilt);
        const QRect qRoi(roi.x, roi.y, roi.width, roi.height);
        mPaintDrawable->pixelRectChanged(qRoi);
    }
    mLastTs = ts;
}

SoundComposition *Canvas::getSoundComposition() {
    return mSoundComposition.get();
}


void Canvas::writeBoundingBox(QIODevice * const target) {
    ContainerBox::writeBoundingBox(target);
    const int currFrame = getCurrentFrame();
    target->write(rcConstChar(&currFrame), sizeof(int));
    target->write(rcConstChar(&mClipToCanvasSize), sizeof(bool));
    target->write(rcConstChar(&mWidth), sizeof(int));
    target->write(rcConstChar(&mHeight), sizeof(int));
    target->write(rcConstChar(&mFps), sizeof(qreal));
    target->write(rcConstChar(&mMaxFrame), sizeof(int));
    mSoundComposition->writeSounds(target);
}

void Canvas::readBoundingBox(QIODevice * const target) {
    ContainerBox::readBoundingBox(target);
    int currFrame;
    target->read(rcChar(&currFrame), sizeof(int));
    target->read(rcChar(&mClipToCanvasSize), sizeof(bool));
    target->read(rcChar(&mWidth), sizeof(int));
    target->read(rcChar(&mHeight), sizeof(int));
    target->read(rcChar(&mFps), sizeof(qreal));
    target->read(rcChar(&mMaxFrame), sizeof(int));
    anim_setAbsFrame(currFrame);
    mSoundComposition->readSounds(target);
}
