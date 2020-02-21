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

#include "zipfileloader.h"

ZipFileLoader::ZipFileLoader() {}

void ZipFileLoader::setZipPath(const QString &path) {
    mZip.setZipName(path);
    if(!mZip.open(QuaZip::mdUnzip))
        RuntimeThrow("Could not open " + path);
    mFile.setZip(&mZip);
}

void ZipFileLoader::process(const QString &file, const bool text,
                            const Processor &func) {
    if(!mZip.setCurrentFile(file))
        RuntimeThrow("No " + file + " found in " + mZip.getZipName());
    QIODevice::OpenMode openMode = QIODevice::ReadOnly;
    if(text) openMode |= QIODevice::Text;
    if(!mFile.open(openMode))
        RuntimeThrow("Could not open " + file + " from " + mZip.getZipName());
    try {
        func(&mFile);
    } catch(...) {
        mFile.close();
        RuntimeThrow("Could not parse " + file + " from " + mZip.getZipName());
    }
    mFile.close();
}