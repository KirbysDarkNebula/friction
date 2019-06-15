#ifndef CANVAS_H
#define CANVAS_H

#include "Boxes/containerbox.h"
#include "colorhelpers.h"
#include <QThread>
#include "CacheHandlers/hddcachablecachehandler.h"
#include "skia/skiaincludes.h"
#include "GUI/valueinput.h"
#include "Animators/coloranimator.h"
#include "MovablePoints/segment.h"
#include "MovablePoints/movablepoint.h"
#include "Boxes/canvasrenderdata.h"
#include "Paint/drawableautotiledsurface.h"
#include "canvasbase.h"
#include "Paint/animatedsurface.h"
#include <QAction>
#include "Animators/outlinesettingsanimator.h"

class AnimatedSurface;
class PaintBox;
class TextBox;
class Circle;
class ParticleBox;
class Rectangle;
class PathPivot;
class SoundComposition;
class SkCanvas;
class ImageSequenceBox;
class Brush;
class NodePoint;
class UndoRedoStack;
class ExternalLinkBox;
struct GPURasterEffectCreator;
class CanvasWindow;
class SingleSound;
class VideoBox;
class ImageBox;

enum CtrlsMode : short;

class MouseEvent {
protected:
    MouseEvent(const QPointF& pos,
               const QPointF& lastPos,
               const QPointF& lastPressPos,
               const bool mouseGrabbing,
               const qreal scale,
               const CanvasMode mode,
               const QPoint& globalPos,
               const Qt::MouseButton button,
               const Qt::MouseButtons buttons,
               const Qt::KeyboardModifiers modifiers,
               const ulong& timestamp,
               QWidget * const widget) :
        fPos(pos), fLastPos(lastPos), fLastPressPos(lastPressPos),
        fMouseGrabbing(mouseGrabbing), fScale(scale),
        fMode(mode), fGlobalPos(globalPos),
        fButton(button), fButtons(buttons),
        fModifiers(modifiers), fTimestamp(timestamp),
        fWidget(widget) {}
public:
    MouseEvent(const QPointF& pos,
               const QPointF& lastPos,
               const QPointF& lastPressPos,
               const bool mouseGrabbing,
               const qreal scale,
               const CanvasMode mode,
               const QMouseEvent * const e,
               QWidget * const widget) :
        MouseEvent(pos, lastPos, lastPressPos, mouseGrabbing,
                   scale, mode, e->globalPos(), e->button(),
                   e->buttons(), e->modifiers(), e->timestamp(),
                   widget) {}

    QPointF fPos;
    QPointF fLastPos;
    QPointF fLastPressPos;
    bool fMouseGrabbing;
    qreal fScale;
    CanvasMode fMode;
    QPoint fGlobalPos;
    Qt::MouseButton fButton;
    Qt::MouseButtons fButtons;
    Qt::KeyboardModifiers fModifiers;
    ulong fTimestamp;
    QWidget* fWidget;
};

struct KeyEvent : public MouseEvent {
    KeyEvent(const QPointF& pos,
             const QPointF& lastPos,
             const QPointF& lastPressPos,
             const bool mouseGrabbing,
             const qreal scale,
             const CanvasMode mode,
             const QPoint globalPos,
             const Qt::MouseButtons buttons,
             const QKeyEvent * const e,
             QWidget * const widget) :
      MouseEvent(pos, lastPos, lastPressPos, mouseGrabbing,
                 scale, mode, globalPos, Qt::NoButton,
                 buttons, e->modifiers(), e->timestamp(),
                 widget), fKey(e->key()) {}

    int fKey;
};

class Canvas : public ContainerBox, public CanvasBase {
    Q_OBJECT
    friend class SelfRef;
    friend class CanvasWindow;
protected:
    explicit Canvas(CanvasWindow * const canvasWidget,
                    const int canvasWidth = 1920,
                    const int canvasHeight = 1080,
                    const int frameCount = 200,
                    const qreal fps = 24);
public:
    void selectOnlyLastPressedBox();
    void selectOnlyLastPressedPoint();

    void repaintIfNeeded();
    void setCanvasMode(const CanvasMode mode);
    void startSelectionAtPoint(const QPointF &pos);
    void moveSecondSelectionPoint(const QPointF &pos);
    void setPointCtrlsMode(const CtrlsMode& mode);
    void setCurrentBoxesGroup(ContainerBox * const group);

    void updatePivot();

    void updatePivotIfNeeded();

    //void updateAfterFrameChanged(const int currentFrame);

    QSize getCanvasSize();

    void centerPivotPosition() {}

    //
    void finishSelectedPointsTransform();
    void finishSelectedBoxesTransform();
    void moveSelectedPointsByAbs(const QPointF &by,
                                 const bool startTransform);
    void moveSelectedBoxesByAbs(const QPointF &by,
                                const bool startTransform);
    void groupSelectedBoxes();

    //void selectAllBoxes();
    void deselectAllBoxes();

    void applyShadowToSelected();

    void selectedPathsUnion();
    void selectedPathsDifference();
    void selectedPathsIntersection();
    void selectedPathsDivision();
    void selectedPathsExclusion();
    void makeSelectedPointsSegmentsCurves();
    void makeSelectedPointsSegmentsLines();

    void centerPivotForSelected();
    void resetSelectedScale();
    void resetSelectedTranslation();
    void resetSelectedRotation();
    void convertSelectedBoxesToPath();
    void convertSelectedPathStrokesToPath();

    void applySampledMotionBlurToSelected();
    void applyLinesEffectToSelected();
    void applyCirclesEffectToSelected();
    void applySwirlEffectToSelected();
    void applyOilEffectToSelected();
    void applyImplodeEffectToSelected();
    void applyDesaturateEffectToSelected();
    void applyColorizeEffectToSelected();
    void applyReplaceColorEffectToSelected();
    void applyContrastEffectToSelected();
    void applyBrightnessEffectToSelected();

    void rotateSelectedBy(const qreal rotBy,
                          const QPointF &absOrigin,
                          const bool startTrans);

    QPointF getSelectedBoxesAbsPivotPos();
    bool isBoxSelectionEmpty() const;

    void ungroupSelectedBoxes();
    void scaleSelectedBy(const qreal scaleBy,
                         const QPointF &absOrigin,
                         const bool startTrans);
    void cancelSelectedBoxesTransform();
    void cancelSelectedPointsTransform();

    void setSelectedCapStyle(const Qt::PenCapStyle& capStyle);
    void setSelectedJoinStyle(const Qt::PenJoinStyle &joinStyle);
    void setSelectedStrokeWidth(const qreal strokeWidth);
    void setSelectedStrokeBrush(SimpleBrushWrapper * const brush);
    void setSelectedStrokeBrushWidthCurve(
            const qCubicSegment1D& curve);
    void setSelectedStrokeBrushTimeCurve(
            const qCubicSegment1D& curve);
    void setSelectedStrokeBrushPressureCurve(
            const qCubicSegment1D& curve);
    void setSelectedStrokeBrushSpacingCurve(
            const qCubicSegment1D& curve);

    void startSelectedStrokeWidthTransform();
    void startSelectedStrokeColorTransform();
    void startSelectedFillColorTransform();

    void getDisplayedFillStrokeSettingsFromLastSelected(
            PaintSettingsAnimator*& fillSetings, OutlineSettingsAnimator*& strokeSettings);
    void scaleSelectedBy(const qreal scaleXBy, const qreal scaleYBy,
                         const QPointF &absOrigin, const bool startTrans);

    void grabMouseAndTrack();

    qreal getResolutionFraction();
    void setResolutionFraction(const qreal percent);

    void applyCurrentTransformationToSelected();
    QPointF getSelectedPointsAbsPivotPos();
    bool isPointSelectionEmpty() const;
    void scaleSelectedPointsBy(const qreal scaleXBy,
                               const qreal scaleYBy,
                               const QPointF &absOrigin,
                               const bool startTrans);
    void rotateSelectedPointsBy(const qreal rotBy,
                                const QPointF &absOrigin,
                                const bool startTrans);
    int getPointsSelectionCount() const ;

    void clearPointsSelectionOrDeselect();
    NormalSegment getSegment(const MouseEvent &e) const;

    void createLinkBoxForSelected();
    void startSelectedPointsTransform();

    void mergePoints();
    void disconnectPoints();
    void connectPoints();

    void setSelectedFontFamilyAndStyle(
            const QString& family, const QString& style);
    void setSelectedFontSize(const qreal size);
    void removeSelectedPointsAndClearList();
    void removeSelectedBoxesAndClearList();

    void addBoxToSelection(BoundingBox *box);
    void removeBoxFromSelection(BoundingBox *box);
    void clearBoxesSelection();
    void clearBoxesSelectionList();

    void addPointToSelection(MovablePoint * const point);
    void removePointFromSelection(MovablePoint * const point);

    void clearPointsSelection();
    void raiseSelectedBoxesToTop();
    void lowerSelectedBoxesToBottom();
    void raiseSelectedBoxes();
    void lowerSelectedBoxes();

    void selectAndAddContainedPointsToSelection(const QRectF &absRect);
//

    void mousePressEvent(const MouseEvent &e);
    void mouseReleaseEvent(const MouseEvent &e);
    void mouseMoveEvent(const MouseEvent &e);
    void mouseDoubleClickEvent(const MouseEvent &e);

    struct TabletEvent {
        TabletEvent(const QPointF& pos, QTabletEvent * const e) :
            fPos(pos), fType(e->type()),
            fButton(e->button()), fButtons(e->buttons()),
            fModifiers(e->modifiers()), fTimestamp(e->timestamp()) {}

        QPointF fPos;
        QEvent::Type fType;
        Qt::MouseButton fButton;
        Qt::MouseButtons fButtons;
        Qt::KeyboardModifiers fModifiers;
        ulong fTimestamp;
        qreal fPressure;
        int fXTilt;
        int fYTilt;
    };

    void tabletEvent(const QTabletEvent * const e,
                     const QPointF &pos);

    bool keyPressEvent(QKeyEvent *event);

    qsptr<BoundingBox> createLink();
    ImageBox* createImageBox(const QString &path);
    ImageSequenceBox* createAnimationBoxForPaths(const QStringList &paths);
    VideoBox* createVideoForPath(const QString &path);
    ExternalLinkBox *createLinkToFileWithPath(const QString &path);
    SingleSound* createSoundForPath(const QString &path);

    void setPreviewing(const bool bT);
    void setOutputRendering(const bool bT);

    const CanvasMode &getCurrentCanvasMode() const {
        return mCurrentMode;
    }

    Canvas *getParentCanvas() {
        return this;
    }

    bool SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                             const bool parentSatisfies,
                             const bool parentMainTarget) const;

    ContainerBox *getCurrentBoxesGroup() {
        return mCurrentBoxesGroup;
    }

    void updateTotalTransform() {}

    QMatrix getTotalTransform() const {
        return QMatrix();
    }

    QMatrix getRelativeTransformAtCurrentFrame() {
        return QMatrix();
    }

    QPointF mapAbsPosToRel(const QPointF &absPos) {
        return absPos;
    }

    void setIsCurrentCanvas(const bool bT);

    void scheduleEffectsMarginUpdate() {}

    void renderSk(SkCanvas * const canvas,
                  GrContext * const grContext,
                  const QRect &drawRect, const QMatrix &viewTrans);

    void setCanvasSize(const int width, const int height) {
        mWidth = width;
        mHeight = height;
    }

    int getCanvasWidth() const {
        return mWidth;
    }

    QRectF getMaxBoundsRect() const {
        if(mClipToCanvasSize) {
            return QRectF(0, 0, mWidth, mHeight);
        } else {
            return QRectF(-mWidth, - mHeight, 3*mWidth, 3*mHeight);
        }
    }

    int getCanvasHeight() const {
        return mHeight;
    }

    void setMaxFrame(const int frame);

    ColorAnimator *getBgColorAnimator() {
        return mBackgroundColor.get();
    }

    stdsptr<BoundingBoxRenderData> createRenderData();

    void setupRenderData(const qreal relFrame,
                         BoundingBoxRenderData * const data) {
        ContainerBox::setupRenderData(relFrame, data);
        auto canvasData = GetAsPtr(data, CanvasRenderData);
        canvasData->fBgColor = toSkColor(mBackgroundColor->getColor());
        canvasData->fCanvasHeight = mHeight*mResolutionFraction;
        canvasData->fCanvasWidth = mWidth*mResolutionFraction;
    }

    bool clipToCanvas() { return mClipToCanvasSize; }

    const SimpleBrushWrapper * getCurrentBrush() const;
    void setCurrentBrush(const SimpleBrushWrapper * const brush);
    void setBrushColor(const QColor& color);

    void incBrushRadius();
    void decBrushRadius();

    void schedulePivotUpdate();
    void setClipToCanvas(const bool bT) { mClipToCanvasSize = bT; }
    void setRasterEffectsVisible(const bool bT) { mRasterEffectsVisible = bT; }
    void setPathEffectsVisible(const bool bT) { mPathEffectsVisible = bT; }
protected:
//    void updateAfterTotalTransformationChanged() {
////        for(const auto& child : mChildBoxes) {
////            child->updateTotalTransformTmp();
////            child->scheduleSoftUpdate();
////        }
//    }

    void setCurrentSmartEndPoint(SmartNodePoint * const point);
    NodePoint *getCurrentPoint();

    void handleMovePathMouseRelease(const MouseEvent &e);
    void handleMovePointMouseRelease(const MouseEvent &e);

    void handleRightButtonMousePress(const QPoint &globalPos);
    void handleLeftButtonMousePress(const MouseEvent &e);
signals:
    void canvasNameChanged(Canvas *, QString);
private slots:
    void emitCanvasNameChanged();
public:
    void scheduleDisplayedFillStrokeSettingsUpdate();

    void prp_afterChangedAbsRange(const FrameRange &range);

    void makePointCtrlsSymmetric();
    void makePointCtrlsSmooth();
    void makePointCtrlsCorner();

    void makeSegmentLine();
    void makeSegmentCurve();

    MovablePoint *getPointAtAbsPos(const QPointF &absPos,
                                   const CanvasMode &mode,
                                   const qreal invScale);
    void duplicateSelectedBoxes();
    void clearLastPressedPoint();
    void clearCurrentSmartEndPoint();
    void applyPaintSettingToSelected(const PaintSettingsApplier &setting);
    void setSelectedFillColorMode(const ColorMode &mode);
    void setSelectedStrokeColorMode(const ColorMode &mode);
    int getCurrentFrame();
    int getFrameCount();

    SoundComposition *getSoundComposition();

    void updateHoveredBox(const MouseEvent& e);
    void updateHoveredPoint(const MouseEvent& e);
    void updateHoveredEdge(const MouseEvent &e);
    void updateHovered(const MouseEvent &e);
    void clearHoveredEdge();
    void clearHovered();

    void setLocalPivot(const bool localPivot) {
        mLocalPivot = localPivot;
        updatePivot();
    }

    bool getPivotLocal() const {
        return mLocalPivot;
    }

    int getMaxFrame();

    //void updatePixmaps();
    HDDCachableCacheHandler& getCacheHandler() {
        return mCacheHandler;
    }

    HDDCachableCacheHandler& getSoundCacheHandler();

    void setCurrentPreviewContainer(const int relFrame);
    void setCurrentPreviewContainer(const stdsptr<ImageCacheContainer> &cont);
    void setLoadingPreviewContainer(
            const stdsptr<ImageCacheContainer> &cont);

    void setRenderingPreview(const bool bT);

    bool isPreviewingOrRendering() const {
        return mPreviewing || mRenderingPreview || mRenderingOutput;
    }

    qreal getFps() const { return mFps; }
    void setFps(const qreal fps) { mFps = fps; }

    BoundingBox *getBoxAt(const QPointF &absPos) {
        if(mClipToCanvasSize) {
            if(!getMaxBoundsRect().contains(absPos)) return nullptr;
        }
        return ContainerBox::getBoxAt(absPos);
    }

    void anim_scaleTime(const int pivotAbsFrame, const qreal scale);

    void changeFpsTo(const qreal fps) {
        anim_scaleTime(0, fps/mFps);
        setFps(fps);
    }
    void drawTransparencyMesh(SkCanvas * const canvas, const SkRect &viewRect, const qreal scale);

    bool SWT_isCanvas() const { return true; }

    void addSelectedBoxesActions(QMenu * const qMenu);
    void addActionsToMenu(BoxTypeMenu * const menu) { Q_UNUSED(menu); }
    void addActionsToMenu(QMenu* const menu);

    void deleteAction();
    void copyAction();
    void pasteAction();
    void cutAction();
    void duplicateAction();
    void selectAllAction();
    void clearSelectionAction();
    void rotateSelectedBoxesStartAndFinish(const qreal rotBy);
    bool shouldPlanScheduleUpdate() {
        return mCurrentPreviewContainerOutdated;
    }

    void renderDataFinished(BoundingBoxRenderData *renderData);
    FrameRange prp_getIdenticalRelRange(const int relFrame) const;
    void setPickingFromPath(const bool pickFill,
                            const bool pickStroke) {
        mPickFillFromPath = pickFill;
        mPickStrokeFromPath = pickStroke;
    }

    QRectF getRelBoundingRect(const qreal );
    void writeBoundingBox(QIODevice * const target);
    void readBoundingBox(QIODevice * const target);
    bool anim_prevRelFrameWithKey(const int relFrame, int &prevRelFrame);
    bool anim_nextRelFrameWithKey(const int relFrame, int &nextRelFrame);

    void shiftAllPointsForAllKeys(const int by);
    void revertAllPointsForAllKeys();
    void shiftAllPoints(const int by);
    void revertAllPoints();
    void flipSelectedBoxesHorizontally();
    void flipSelectedBoxesVertically();
    int getByteCountPerFrame() {
        return qCeil(mWidth*mResolutionFraction)*
                qCeil(mHeight*mResolutionFraction)*4;
        //return mCurrentPreviewContainer->getByteCount();
    }
    int getMaxPreviewFrame(const int minFrame, const int maxFrame);
    void selectedPathsCombine();
    void selectedPathsBreakApart();
    void invertSelectionAction();

    bool getRasterEffectsVisible() const {
        return mRasterEffectsVisible;
    }

    bool getPathEffectsVisible() const {
        return mPathEffectsVisible;
    }

    void anim_setAbsFrame(const int frame);

    void moveDurationRectForAllSelected(const int dFrame);
    void startDurationRectPosTransformForAllSelected();
    void finishDurationRectPosTransformForAllSelected();
    void startMinFramePosTransformForAllSelected();
    void finishMinFramePosTransformForAllSelected();
    void moveMinFrameForAllSelected(const int dFrame);
    void startMaxFramePosTransformForAllSelected();
    void finishMaxFramePosTransformForAllSelected();
    void moveMaxFrameForAllSelected(const int dFrame);

    UndoRedoStack *getUndoRedoStack() {
        return mUndoRedoStack.get();
    }

    void blockUndoRedo();
    void unblockUndoRedo();

    void setParentToLastSelected();
    void clearParentForSelected();

    bool startRotatingAction(const KeyEvent &e);
    bool startScalingAction(const KeyEvent &e);
    bool startMovingAction(const KeyEvent &e);

    void deselectAllBoxesAction();
    void selectAllBoxesAction();
    void selectAllPointsAction();
    bool handlePaintModeKeyPress(const KeyEvent &e);
    bool handleTransormationInputKeyEvent(const KeyEvent &e);

    void setCurrentGroupParentAsCurrentGroup();

    void setCurrentRenderRange(const FrameRange& range) {
        mCurrRenderRange = range;
    }
private:
    void openTextEditorForTextBox(TextBox *textBox);

    bool isShiftPressed();

    bool isShiftPressed(QKeyEvent *event);
    bool isCtrlPressed();
    bool isCtrlPressed(QKeyEvent *event);
    bool isAltPressed();
    bool isAltPressed(QKeyEvent *event);

    void scaleSelected(const MouseEvent &e);
    void rotateSelected(const MouseEvent &e);
    qreal mLastDRot = 0;
    int mRotHalfCycles = 0;
    TransformMode mTransMode = MODE_NONE;
protected:
    stdsptr<UndoRedoStack> mUndoRedoStack;

    void paintPress(const QPointF &pos, const ulong ts,
                    const qreal pressure,
                    const qreal xTilt, const qreal yTilt);
    void paintMove(const QPointF &pos, const ulong ts,
                   const qreal pressure,
                   const qreal xTilt, const qreal yTilt);
    void updatePaintBox();
    void setPaintBox(PaintBox * const box);
    void setPaintDrawable(DrawableAutoTiledSurface * const surf);
    void afterPaintAnimSurfaceChanged();

    ulong mLastTs;
    bool mPaintBoxWasVisible;
    qptr<PaintBox> mPaintDrawableBox;
    qptr<AnimatedSurface> mPaintAnimSurface;
    AnimatedSurface::OnionSkin mPaintOnion;
    bool mPaintPressedSinceUpdate = false;
    DrawableAutoTiledSurface * mPaintDrawable = nullptr;

    QColor mCurrentBrushColor;
    const SimpleBrushWrapper * mCurrentBrush = nullptr;
    bool mStylusDrawing = false;
    bool mPickFillFromPath = false;
    bool mPickStrokeFromPath = false;

    uint mLastStateId = 0;
    HDDCachableCacheHandler mCacheHandler;

    qsptr<ColorAnimator> mBackgroundColor =
            SPtrCreate(ColorAnimator)();

    SmartVectorPath *getPathResultingFromOperation(const SkPathOp &pathOp);

    void sortSelectedBoxesAsc();
    void sortSelectedBoxesDesc();

    qsptr<SoundComposition> mSoundComposition;

    bool mLocalPivot = false;
    bool mIsCurrentCanvas = true;
    int mMaxFrame = 0;

    qreal mResolutionFraction;

    MainWindow *mMainWindow;
    CanvasWindow *mActiveWindow;
    QWidget *mActiveWidget;

    qptr<Circle> mCurrentCircle;
    qptr<Rectangle> mCurrentRectangle;
    qptr<TextBox> mCurrentTextBox;
    qptr<ParticleBox> mCurrentParticleBox;
    qptr<ContainerBox> mCurrentBoxesGroup;

    stdptr<MovablePoint> mHoveredPoint_d;
    qptr<BoundingBox> mHoveredBox;

    qptr<BoundingBox> mLastPressedBox;
    stdsptr<PathPivot> mRotPivot;

    stdptr<SmartNodePoint> mLastEndPoint;

    NormalSegment mHoveredNormalSegment;
    NormalSegment mCurrentNormalSegment;
    qreal mCurrentNormalSegmentT;

    bool mTransformationFinishedBeforeMouseRelease = false;

    ValueInput mValueInput;

    bool mPreviewing = false;
    bool mRenderingPreview = false;
    bool mRenderingOutput = false;
    FrameRange mCurrRenderRange;

    bool mCurrentPreviewContainerOutdated = false;
    stdsptr<ImageCacheContainer> mCurrentPreviewContainer;
    stdsptr<ImageCacheContainer> mLoadingPreviewContainer;

    int mCurrentPreviewFrameId;
    int mMaxPreviewFrameId = 0;

    bool mClipToCanvasSize = false;
    bool mRasterEffectsVisible = true;
    bool mPathEffectsVisible = true;

    bool mIsMouseGrabbing = false;

    bool mDoubleClick = false;
    int mMovesToSkip = 0;

    QColor mFillColor;
    QColor mOutlineColor;

    int mWidth;
    int mHeight;

    qreal mFps = 24;

    bool mPivotUpdateNeeded = false;

    bool mFirstMouseMove = false;
    bool mSelecting = false;
//    bool mMoving = false;

    QRectF mSelectionRect;
    CanvasMode mCurrentMode = MOVE_BOX;

    void handleMovePointMousePressEvent(const MouseEvent& e);
    void handleMovePointMouseMove(const MouseEvent& e);

    void handleMovePathMousePressEvent(const MouseEvent &e);
    void handleMovePathMouseMove(const MouseEvent &e);

    void handleMouseRelease(const MouseEvent &e);

    void handleAddSmartPointMousePress(const MouseEvent &e);
    void handleAddSmartPointMouseMove(const MouseEvent &e);
    void handleAddSmartPointMouseRelease(const MouseEvent &e);

    void updateTransformation(const KeyEvent &e);
    QPointF getMoveByValueForEvent(const MouseEvent &e);
    void cancelCurrentTransform();
    void releaseMouseAndDontTrack();
};

#endif // CANVAS_H
