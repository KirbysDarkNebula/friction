// enve - 2D animations software
// Copyright (C) 2016-2020 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "etaskbase.h"

#include "etask.h"

void eTaskBase::finishedProcessing() {
    mState = eTaskState::finished;
    if(mCancel) {
        mCancel = false;
        cancel();
    } else if(unhandledException()) {
        handleException();
        if(unhandledException()) {
            gPrintExceptionCritical(takeException());
            cancel();
        }
    } else {
        afterProcessing();
        tellDependentThatFinished();
    }
}

void eTaskBase::addDependent(eTask * const task) {
    if(!task) return;
    if(mState == eTaskState::finished) {
        return;
    } else if(mState == eTaskState::canceled) {
        task->cancel();
    } else {
        if(mDependent.contains(task)) return;
        mDependent << task;
        task->incDependencies();
    }
}

void eTaskBase::addDependent(const Dependent &func) {
    if(mState == eTaskState::finished) {
        if(func.fFinished) func.fFinished();
    } else if(mState == eTaskState::canceled) {
        if(func.fCanceled) func.fCanceled();
    } else mDependentF << func;
}

void eTaskBase::cancel() {
    if(mState == eTaskState::processing) {
        mCancel = true;
        return;
    }
    mState = eTaskState::canceled;
    cancelDependent();
    afterCanceled();
}

void eTaskBase::setException(const std::exception_ptr& exception) {
    mUpdateException = exception;
}

bool eTaskBase::unhandledException() const {
    return static_cast<bool>(mUpdateException);
}

std::__exception_ptr::exception_ptr eTaskBase::takeException() {
    std::exception_ptr exc;
    mUpdateException.swap(exc);
    return exc;
}

void eTaskBase::tellDependentThatFinished() {
    for(const auto& dependent : mDependent) {
        if(dependent) dependent->decDependencies();
    }
    mDependent.clear();
    for(const auto& dependent : mDependentF) {
        if(dependent.fFinished) dependent.fFinished();
    }
    mDependentF.clear();
}

void eTaskBase::cancelDependent() {
    for(const auto& dependent : mDependent) {
        if(dependent) dependent->cancel();
    }
    mDependent.clear();
    for(const auto& dependent : mDependentF) {
        if(dependent.fCanceled) dependent.fCanceled();
    }
    mDependentF.clear();
}