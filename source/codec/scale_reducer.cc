//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "codec/scale_reducer.h"

#include <libyuv/scale_row.h>
#include <libyuv/scale_argb.h>

#include "base/logging.h"
#include "desktop_capture/desktop_frame_aligned.h"

namespace aspia {

namespace {

const int kMinScaleFactor = 50;
const int kMaxScaleFactor = 150;
const int kDefScaleFactor = 100;

int div(int num, int div)
{
    return (num + div - 1) / div;
}

DesktopSize scaledSize(const DesktopSize& source_size, int scale_factor)
{
    return DesktopSize(div(source_size.width() * scale_factor, kDefScaleFactor),
                       div(source_size.height() * scale_factor, kDefScaleFactor));
}

DesktopRect scaledRect(const DesktopRect& source_rect, int scale_factor)
{
    int left = (source_rect.left() * scale_factor) / kDefScaleFactor;
    int top = (source_rect.top() * scale_factor) / kDefScaleFactor;
    int right = div(source_rect.right() * scale_factor, kDefScaleFactor);
    int bottom = div(source_rect.bottom() * scale_factor, kDefScaleFactor);

    static const int kPadding = 1;

    return DesktopRect::makeLTRB(left - kPadding, top - kPadding,
                                 right + kPadding, bottom + kPadding);
}

} // namespace

ScaleReducer::ScaleReducer(int scale_factor)
    : scale_factor_(scale_factor)
{
    // Nothing
}

// static
ScaleReducer* ScaleReducer::create(int scale_factor)
{
    if (scale_factor < kMinScaleFactor || scale_factor > kMaxScaleFactor)
        return nullptr;

    return new ScaleReducer(scale_factor);
}

const DesktopFrame* ScaleReducer::scaleFrame(const DesktopFrame* source_frame)
{
    DCHECK(source_frame);
    DCHECK(!source_frame->constUpdatedRegion().isEmpty());
    DCHECK(source_frame->format() == PixelFormat::ARGB());

    if (scale_factor_ == kDefScaleFactor)
        return source_frame;

    if (screen_settings_tracker_.isSizeChanged(source_frame->size()))
        scaled_frame_.reset();

    if (!scaled_frame_)
    {
        DesktopSize size = scaledSize(source_frame->size(), scale_factor_);

        scaled_frame_ = DesktopFrameAligned::create(size, source_frame->format(), 32);
        if (!scaled_frame_)
            return nullptr;
    }

    const DesktopSize& source_size = source_frame->size();
    const DesktopSize& scaled_size = scaled_frame_->size();

    DesktopRect scaled_frame_rect = DesktopRect::makeSize(scaled_size);
    DesktopRegion* updated_region = scaled_frame_->updatedRegion();

    updated_region->clear();

    for (DesktopRegion::Iterator it(source_frame->constUpdatedRegion()); !it.isAtEnd(); it.advance())
    {
        DesktopRect scaled_rect = scaledRect(it.rect(), scale_factor_);

        scaled_rect.intersectWith(scaled_frame_rect);

#if 1
        int height = scaled_rect.height();

        // FIXME: A bug in libyuv? If the coordinate |bottom| of rectangle is equal to coordinate
        // |bottom| in the frame, then a crash occurs.
        if (scaled_rect.bottom() == scaled_frame_rect.bottom())
            height -= 1;
#endif

        if (libyuv::ARGBScaleClip(source_frame->frameData(),
                                  source_frame->stride(),
                                  source_size.width(),
                                  source_size.height(),
                                  scaled_frame_->frameData(),
                                  scaled_frame_->stride(),
                                  scaled_size.width(),
                                  scaled_size.height(),
                                  scaled_rect.x(),
                                  scaled_rect.y(),
                                  scaled_rect.width(),
                                  height,
                                  libyuv::kFilterBox) == -1)
        {
            LOG(LS_WARNING) << "libyuv::ARGBScaleClip failed";
        }

        updated_region->addRect(scaled_rect);
    }

    return scaled_frame_.get();
}

} // namespace aspia
