#include "PreviewViewModel.h"

#include "core/services/SliceServiceMock.h"

PreviewViewModel::PreviewViewModel(SliceServiceMock *sliceService, QObject *parent)
    : QObject(parent), sliceService_(sliceService)
{
  connect(sliceService_, &SliceServiceMock::progressChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceServiceMock::slicingChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceServiceMock::sliceFinished, this, [this](const QString &time)
          {
        estimatedTime_ = time;
        emit stateChanged(); });
}

int PreviewViewModel::progress() const
{
  return sliceService_->progress();
}

bool PreviewViewModel::slicing() const
{
  return sliceService_->slicing();
}

QString PreviewViewModel::estimatedTime() const
{
  return estimatedTime_;
}
