/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "addmaskdialog.h"
#include "widgets/twocolumnlayout.h"

#include <QDialogButtonBox>

AddMaskDialog::AddMaskDialog(int &val, QWidget *parent) :
    QDialog(parent), mVal(val){
   
    const auto mainLayout = new QVBoxLayout(this);
    const auto twoColumnLayout = new TwoColumnLayout();
    
    QStringList maskTypes = {"Square", "Circle", "Custom Path"}; //, "Image (Luma)" };
    const auto MTLabel = new QLabel("Mask shape:");
    mMaskTypeBox = new QComboBox(this);
    mMaskTypeBox->insertItems(0, maskTypes);
    mMaskTypeBox->setItemIcon(0, QIcon::fromTheme("rectCreate"));
    mMaskTypeBox->setItemIcon(1, QIcon::fromTheme("circleCreate"));
    mMaskTypeBox->setItemIcon(2, QIcon::fromTheme("pathCreate"));
    twoColumnLayout->addPair(MTLabel, mMaskTypeBox);

    const auto MMLabel = new QLabel("Inverted Mask:");
    mMaskModeCheck = new QCheckBox(this);
    twoColumnLayout->addPair(MMLabel, mMaskModeCheck);

    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        mVal = 0;
        mVal = getMaskType()+1;
        mVal = (mVal << 0b1) + (int)isInvertedMask();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addLayout(twoColumnLayout);
    mainLayout->addWidget(buttons);
    setLayout(mainLayout);
}

int AddMaskDialog::getMaskType() const{
    return mMaskTypeBox->currentIndex();
}

bool AddMaskDialog::isInvertedMask() const{
    return mMaskModeCheck->isChecked();
}
