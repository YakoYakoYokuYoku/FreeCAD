// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QFile>
#include <QFileIconProvider>
#include <QImageReader>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QLabel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QGuiApplication>
#endif

#include "FileCardDelegate.h"
#include "../App/DisplayedFilesModel.h"
#include "App/Application.h"
#include <3rdParty/GSL/include/gsl/pointers>

using namespace Start;

FileCardDelegate::FileCardDelegate(QObject* parent)
    : QAbstractItemDelegate(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
}


void FileCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    auto thumbnailSize =
        static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
    auto cardWidth = thumbnailSize;
    auto baseName = index.data(static_cast<int>(DisplayedFilesModelRoles::baseName)).toString();
    auto size = index.data(static_cast<int>(DisplayedFilesModelRoles::size)).toString();
    auto image = index.data(static_cast<int>(DisplayedFilesModelRoles::image)).toByteArray();
    auto path = index.data(static_cast<int>(DisplayedFilesModelRoles::path)).toString();
    painter->save();
    auto widget = gsl::owner<QWidget*>(new QWidget());
    auto layout = gsl::owner<QVBoxLayout*>(new QVBoxLayout());
    widget->setLayout(layout);
    auto thumbnail = gsl::owner<QLabel*>(new QLabel());
    auto pixmap = gsl::owner<QPixmap*>(new QPixmap());
    if (!image.isEmpty()) {
        pixmap->loadFromData(image);
        if (!pixmap->isNull()) {
            auto scaled = pixmap->scaled(QSize(thumbnailSize, thumbnailSize),
                                         Qt::AspectRatioMode::KeepAspectRatio,
                                         Qt::TransformationMode::SmoothTransformation);
            thumbnail->setPixmap(scaled);
        }
    }
    else {
        thumbnail->setPixmap(generateThumbnail(path));
    }
    thumbnail->setFixedSize(thumbnailSize, thumbnailSize);
    thumbnail->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    auto elided =
        painter->fontMetrics().elidedText(baseName, Qt::TextElideMode::ElideRight, cardWidth);
    auto name = gsl::owner<QLabel*>(new QLabel(elided));
    layout->addWidget(thumbnail);
    layout->addWidget(name);
    auto sizeLabel = gsl::owner<QLabel*>(new QLabel(size));
    layout->addWidget(sizeLabel);
    layout->addStretch();
    layout->setSpacing(0);
    widget->resize(option.rect.size());
    painter->translate(option.rect.topLeft());
    widget->render(painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter->restore();
    delete pixmap;
}


QSize FileCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    auto thumbnailSize = _parameterGroup->GetInt("FileThumbnailIconsSize", 128);  // NOLINT
    auto cardSpacing = _parameterGroup->GetInt("FileCardSpacing", 20);            // NOLINT
    auto cardWidth = thumbnailSize + cardSpacing;

    auto font = QGuiApplication::font();
    auto qfm = QFontMetrics(font);
    auto textHeight = 2 * qfm.lineSpacing();
    auto cardHeight = thumbnailSize + textHeight + cardSpacing;

    return {static_cast<int>(cardWidth), static_cast<int>(cardHeight)};
}

namespace
{
QPixmap pixmapToSizedQImage(const QImage& pixmap, int size)
{
    return QPixmap::fromImage(pixmap).scaled(size,
                                             size,
                                             Qt::AspectRatioMode::KeepAspectRatio,
                                             Qt::TransformationMode::SmoothTransformation);
}
}  // namespace

QPixmap FileCardDelegate::generateThumbnail(const QString& path) const
{
    auto thumbnailSize =
        static_cast<int>(_parameterGroup->GetInt("FileThumbnailIconsSize", 128));  // NOLINT
    if (path.endsWith(QLatin1String(".fcstd"), Qt::CaseSensitivity::CaseInsensitive)) {
        QImageReader reader(QLatin1String(":/icons/freecad-doc.svg"));
        reader.setScaledSize({thumbnailSize, thumbnailSize});
        return QPixmap::fromImage(reader.read());
    }
    if (path.endsWith(QLatin1String(".fcmacro"), Qt::CaseSensitivity::CaseInsensitive)) {
        QImageReader reader(QLatin1String(":/icons/MacroEditor.svg"));
        reader.setScaledSize({thumbnailSize, thumbnailSize});
        return QPixmap::fromImage(reader.read());
    }
    if (!QImageReader::imageFormat(path).isEmpty()) {
        // It is an image: it can be its own thumbnail
        QImageReader reader(path);
        auto image = reader.read();
        if (!image.isNull()) {
            return pixmapToSizedQImage(image, thumbnailSize);
        }
    }
    QIcon icon = QFileIconProvider().icon(QFileInfo(path));
    if (!icon.isNull()) {
        QPixmap pixmap = icon.pixmap(thumbnailSize);
        if (!pixmap.isNull()) {
            return pixmap;
        }
    }
    QPixmap pixmap = QPixmap(thumbnailSize, thumbnailSize);
    pixmap.fill();
    return pixmap;
}
