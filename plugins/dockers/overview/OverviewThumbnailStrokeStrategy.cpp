/*
 *  Copyright (c) 2016 Eugene Ingerman geneing at gmail dot com
 *  Copyright (c) 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "OverviewThumbnailStrokeStrategy.h"

#include <kis_paint_device.h>
#include <kis_painter.h>
#include "krita_utils.h"
#include "kis_transform_worker.h"
#include "kis_filter_strategy.h"
#include <KoColorSpaceRegistry.h>
#include <KoUpdater.h>

const qreal oversample = 2.;
const int thumbnailTileDim = 128;


class OverviewThumbnailStrokeStrategy::ProcessData : public KisStrokeJobData
{
public:
    ProcessData(const QRect &_rect)
        : KisStrokeJobData(CONCURRENT),
          tileRect(_rect)
    {}

    QRect tileRect;
};

OverviewThumbnailStrokeStrategy::OverviewThumbnailStrokeStrategy(KisPaintDeviceSP device, const QRect& rect, const QSize& thumbnailSize)
    : KisSimpleStrokeStrategy(QLatin1String("OverviewThumbnail")),
      m_device(device),
      m_rect(rect),
      m_thumbnailSize(thumbnailSize)
{
    enableJob(KisSimpleStrokeStrategy::JOB_INIT, true, KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
    enableJob(KisSimpleStrokeStrategy::JOB_DOSTROKE);
    enableJob(KisSimpleStrokeStrategy::JOB_FINISH, true, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);
    enableJob(KisSimpleStrokeStrategy::JOB_CANCEL, true, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);

    setRequestsOtherStrokesToEnd(false);
    setClearsRedoOnStart(false);
    setCanForgetAboutMe(true);
}

OverviewThumbnailStrokeStrategy::~OverviewThumbnailStrokeStrategy()
{
}

KisStrokeStrategy *OverviewThumbnailStrokeStrategy::createLodClone(int levelOfDetail)
{
    /**
     * We do not generate preview for Instant Preview mode. Even though we
     * could do that, it is not very needed, because KisIdleWatcher ensures
     * that overview preview is generated only when all the background jobs
     * are completed.
     *
     * The only thing we should do about Instant Preview is to avoid resetting
     * LoDN planes, when the thumbnail is running. Therefore we should return
     * a fake noop strategy as our LoDN clone (that is a marker of non-legacy
     * stroke for the scheduler)
     */
    return new KisSimpleStrokeStrategy(QLatin1String("OverviewThumbnail_FakeLodN"));
}

void OverviewThumbnailStrokeStrategy::initStrokeCallback()
{
    const QRect imageRect = m_device->defaultBounds()->bounds();

    m_thumbnailOversampledSize = oversample * m_thumbnailSize;

    if ((m_thumbnailOversampledSize.width() > imageRect.width()) || (m_thumbnailOversampledSize.height() > imageRect.height())) {
        m_thumbnailOversampledSize.scale(imageRect.size(), Qt::KeepAspectRatio);
    }

    m_thumbnailDevice = new KisPaintDevice(m_device->colorSpace());

    QVector<KisStrokeJobData*> jobsData;

    QVector<QRect> tileRects = KritaUtils::splitRectIntoPatches(QRect(QPoint(0, 0), m_thumbnailOversampledSize), QSize(thumbnailTileDim, thumbnailTileDim));
    Q_FOREACH (const QRect &tileRect, tileRects) {
        jobsData << new OverviewThumbnailStrokeStrategy::ProcessData(tileRect);
    }

    addMutatedJobs(jobsData);
}

void OverviewThumbnailStrokeStrategy::doStrokeCallback(KisStrokeJobData *data)
{
    ProcessData *d_pd = dynamic_cast<ProcessData*>(data);
    if (d_pd) {
        //we aren't going to use oversample capability of createThumbnailDevice because it recomputes exact bounds for each small patch, which is
        //slow. We'll handle scaling separately.
        KisPaintDeviceSP thumbnailTile = m_device->createThumbnailDeviceOversampled(m_thumbnailOversampledSize.width(), m_thumbnailOversampledSize.height(), 1, m_device->defaultBounds()->bounds(), d_pd->tileRect);
        KisPainter::copyAreaOptimized(d_pd->tileRect.topLeft(), thumbnailTile, m_thumbnailDevice, d_pd->tileRect);
    }
}

void OverviewThumbnailStrokeStrategy::finishStrokeCallback()
{
    QImage overviewImage;

    KoDummyUpdater updater;
    KisTransformWorker worker(m_thumbnailDevice, 1 / oversample, 1 / oversample, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                              &updater, KisFilterStrategyRegistry::instance()->value("Bilinear"));
    worker.run();

    overviewImage = m_thumbnailDevice->convertToQImage(KoColorSpaceRegistry::instance()->rgb8()->profile(),
                                                       QRect(QPoint(0,0), m_thumbnailSize));
    emit thumbnailUpdated(overviewImage);
}

void OverviewThumbnailStrokeStrategy::cancelStrokeCallback()
{
}