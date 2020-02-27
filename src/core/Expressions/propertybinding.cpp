#include "propertybinding.h"

#include "Animators/complexanimator.h"

PropertyBinding::PropertyBinding(const Validator& validator,
                                 const Property* const context) :
    pathChanged([this]() {
        const auto oldBind = mBindProperty.get();
        updateBindPath();
        const auto newSource = sFindPropertyToBind(mPath, mValidator,
                                                   mContext.data());
        if(!newSource) setBindPathValid(false);
        else if(newSource != oldBind) bindProperty(newSource);
        else setBindPathValid(true);
    }), mValidator(validator), mContext(context) {
    connect(context, &Property::prp_pathChanged,
            this, [this]() { pathChanged(); });
}

#include "qrealanimatorbinding.h"
#include "Animators/qrealanimator.h"

#include "qpointfanimatorbinding.h"
#include "Animators/qpointfanimator.h"

qsptr<PropertyBinding> PropertyBinding::sCreate(
        const QString& binding,
        const Validator& validator,
        const Property* const context) {
    const auto prop = sFindPropertyToBind(binding, validator, context);
    if(!prop) return nullptr;
    PropertyBinding* result = nullptr;
    if(const auto qa = enve_cast<QrealAnimator*>(prop)) {
        result = new QrealAnimatorBinding(validator, context);
        result->bindProperty(qa);
    } else if(const auto pa = enve_cast<QPointFAnimator*>(prop)) {
        result = new QPointFAnimatorBinding(validator, context);
        result->bindProperty(pa);
    } else return nullptr;
    result->bindProperty(prop);
    return qsptr<PropertyBinding>(result);
}

void PropertyBinding::setPath(const QString& path) {
    mPath = path;
    reloadBindProperty();
}

bool PropertyBinding::setAbsFrame(const qreal absFrame) {
    if(mBindPathValid && mBindProperty && mContext) {
        const qreal oldRelFrame = mRelFrame;
        mRelFrame = mBindProperty->prp_absFrameToRelFrameF(absFrame);
        const auto oldRange = mBindProperty->prp_getIdenticalAbsRange(oldRelFrame);
        if(oldRange.inRange(absFrame)) return false;
        mValueUpToDate = false;
        return true;
    }
    return false;
}

bool PropertyBinding::dependsOn(const Property* const prop) {
    if(!mBindProperty) return false;
    return mBindProperty == prop || mBindProperty->prp_dependsOn(prop);
}

FrameRange PropertyBinding::identicalRange(const qreal absFrame) {
    if(mBindPathValid && mBindProperty && mContext) {
        const qreal relFrame = mBindProperty->prp_absFrameToRelFrame(absFrame);
        return mBindProperty->prp_getIdenticalRelRange(relFrame);
    }
    return FrameRange::EMINMAX;
}

void PropertyBinding::updateValueIfNeeded() {
    if(mValueUpToDate) return;
    updateValue();
    mValueUpToDate = true;
}

void PropertyBinding::reloadBindProperty() {
    const auto prop = sFindPropertyToBind(mPath, mValidator, mContext.data());
    bindProperty(prop);
}

Property *PropertyBinding::sFindPropertyToBind(const QString& binding,
                                               const Validator& validator,
                                               const Property* const context) {
    if(!context) return nullptr;
    const auto searchCtxt = context->getParent();
    if(!searchCtxt) return nullptr;
    const auto objs = binding.split('.');
    const auto found = searchCtxt->ca_findPropertyWithPathRec(0, objs);
    if(!found || found == context) return nullptr;
    if(validator && !validator(found)) return nullptr;
    return found;
}

void PropertyBinding::updateBindPath() {
    if(!mBindProperty || !mContext) return;
    QStringList prntPath;
    mContext->prp_getFullPath(prntPath);
    QStringList srcPath;
    mBindProperty->prp_getFullPath(srcPath);
    const int iMax = qMin(prntPath.count(), srcPath.count());
    for(int i = 0; i < iMax; i++) {
        const auto& iPrnt = prntPath.first();
        const auto& iSrc = srcPath.first();
        if(iPrnt == iSrc) {
            srcPath.removeFirst();
            prntPath.removeFirst();
        } else break;
    }
    mPath = srcPath.join('.');
}

bool PropertyBinding::bindProperty(Property * const newBinding) {
    if(newBinding && !mValidator(newBinding)) return false;
    auto& conn = mBindProperty.assign(newBinding);
    if(newBinding) {
        setBindPathValid(true);
        conn << connect(newBinding, &Property::prp_absFrameRangeChanged,
                        this, [this](const FrameRange& absRange) {
            const auto relRange = mContext->prp_absRangeToRelRange(absRange);
            emit relRangeChanged(relRange);
        });
        conn << connect(newBinding, &Property::prp_pathChanged,
                        this, [this]() { pathChanged(); });
    }
    emit relRangeChanged(FrameRange::EMINMAX);
    return true;
}

void PropertyBinding::setBindPathValid(const bool valid) {
    if(mBindPathValid == valid) return;
    mBindPathValid = valid;
    emit relRangeChanged(FrameRange::EMINMAX);
}