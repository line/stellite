//
// 2017 writen by snibug@gmail.com
//

#include "node_binder/base/message_pump_node.h"

#include <algorithm>
#include <set>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/threading/thread_local.h"
#include "node/uv.h"
#include "base/debug/stack_trace.h"

namespace base {

namespace {

base::LazyInstance<base::ThreadLocalPointer<MessagePumpNode>>::Leaky
    tls_ptr = LAZY_INSTANCE_INITIALIZER;

void IgnoreReleaseHandle(uv_handle_t* handle) {}

void ReleaseHandle(uv_handle_t* handle) {
  delete handle;
}

void OnPollNotification(uv_poll_t* event, int status, int flags) {
  DCHECK(event && event->data);
  MessagePumpNode::FileDescriptorWatcher* controller =
      static_cast<MessagePumpNode::FileDescriptorWatcher*>(event->data);
  DCHECK(controller);
  controller->OnFileEventNotification(event->io_watcher.fd, flags);
}

}  // anonymouse namespace

MessagePumpNode::FileDescriptorWatcher::FileDescriptorWatcher()
    : pump_(nullptr),
      watcher_(nullptr),
      persistent_(false),
      event_mask_(0) {
}

MessagePumpNode::FileDescriptorWatcher::~FileDescriptorWatcher() {
  StopWatchingFileDescriptor();
}

bool MessagePumpNode::FileDescriptorWatcher::WatchFileDescriptor(
    int fd, bool persistent, int event_mask, MessagePumpNode* pump,
    Watcher* watcher) {
  DCHECK(pump);
  DCHECK(watcher);
  DCHECK_GT(event_mask, 0);
  DCHECK_GT(fd, 0);

  if (event_.get()) {
    event_mask |= event_mask_;
    StopWatchingFileDescriptor();
  }

  event_.reset(new uv_poll_t);
  event_->data = this;

  if (uv_poll_init(uv_default_loop(), event_.get(), fd)) {
    LOG(ERROR) << "uv_poll_init failed";
    return false;
  }

  if (uv_poll_start(event_.get(), event_mask, &OnPollNotification)) {
    uv_close(reinterpret_cast<uv_handle_t*>(event_.release()),
             &ReleaseHandle);
    LOG(ERROR) << "watch file descriptor are failed";
    return false;
  }

  event_mask_ = event_mask;
  persistent_ = persistent;
  pump_ = pump;
  watcher_ = watcher;

  return true;
}

bool MessagePumpNode::FileDescriptorWatcher::StopWatchingFileDescriptor() {
  // poll event already released
  if (!event_.get()) {
    return true;
  }

  if (uv_poll_stop(event_.get())) {
    LOG(ERROR) << "uv_poll_stop are failed";
    return false;
  }

  uv_close(reinterpret_cast<uv_handle_t*>(event_.release()), &ReleaseHandle);

  event_mask_ = 0;
  persistent_ = false;
  pump_ = nullptr;
  watcher_ = nullptr;

  return true;
}

void MessagePumpNode::FileDescriptorWatcher::OnFileEventNotification(
    int fd, int event_mask) {
  DCHECK(pump_);

  if (event_mask & UV_READABLE) {
    OnFileCanReadWithoutBlocking(fd, pump_);
  }

  if (event_mask & UV_WRITABLE) {
    OnFileCanWriteWithoutBlocking(fd, pump_);
  }

  if (!persistent_) {
    StopWatchingFileDescriptor();
  }
}

void MessagePumpNode::FileDescriptorWatcher::OnFileCanReadWithoutBlocking(
    int fd, MessagePumpNode* pump) {
  DCHECK(watcher_);
  watcher_->OnFileCanReadWithoutBlocking(fd);
}

void MessagePumpNode::FileDescriptorWatcher::OnFileCanWriteWithoutBlocking(
    int fd, MessagePumpNode* pump) {
  DCHECK(watcher_);
  watcher_->OnFileCanWriteWithoutBlocking(fd);
}

MessagePumpNode::MessagePumpNode()
  : delegate_(nullptr),
    async_handle_(new uv_async_t),
    timer_handle_(new uv_timer_t),
    message_pump_state_(STATE_IDLE) {
  tls_ptr.Pointer()->Set(this);
}

MessagePumpNode::~MessagePumpNode() {
  tls_ptr.Pointer()->Set(nullptr);
}

// static
MessagePumpNode* MessagePumpNode::current() {
  return tls_ptr.Pointer()->Get();
}

// static
void MessagePumpNode::OnWakeup(uv_async_t* handle) {
  MessagePumpNode* pump = static_cast<MessagePumpNode*>(handle->data);
  DCHECK(pump);
  pump->DoWork();
}

// static
void MessagePumpNode::OnTimeout(uv_timer_t* handle) {
  MessagePumpNode* pump = static_cast<MessagePumpNode*>(handle->data);
  DCHECK(pump);
  pump->DoDelayedWork();
}

// static
void MessagePumpNode::Run(Delegate* delegate) {
  NOTREACHED();
}

void MessagePumpNode::Quit() {
  NOTREACHED();
}

void MessagePumpNode::ScheduleWork() {
  if (uv_async_send(async_handle_.get())) {
    LOG(ERROR) << "wakeup  async task are failed";
  }
}

void MessagePumpNode::ScheduleDelayedWork(
    const base::TimeTicks& new_delayed_work_time) {
  // if wait a another timeout event
  if (!delayed_work_time_.is_null()) {
    DCHECK_EQ(message_pump_state_, STATE_RUNNING);
    if (new_delayed_work_time > delayed_work_time_) {
      return;
    }
  }
  delayed_work_time_ = new_delayed_work_time;

  base::TimeDelta delay = delayed_work_time_ - base::TimeTicks::Now();
  delay = std::max(delay, base::TimeDelta());

  if (message_pump_state_ == STATE_RUNNING) {
    uv_timer_stop(timer_handle_.get());
    message_pump_state_ = STATE_STOP;
  }

  DCHECK(message_pump_state_ == STATE_INIT ||
         message_pump_state_ == STATE_STOP);
  uv_timer_start(timer_handle_.get(), OnTimeout, delay.InMilliseconds(), 0);
  message_pump_state_ = STATE_RUNNING;
}

bool MessagePumpNode::WatchFileDescriptor(int fd,
                                          bool persistent,
                                          int mode,
                                          FileDescriptorWatcher* controller,
                                          Watcher* delegate) {
  DCHECK_GT(fd, 0);
  DCHECK(controller);
  DCHECK(delegate);
  DCHECK(mode == WATCH_READ || mode == WATCH_WRITE || mode == WATCH_READ_WRITE);
  DCHECK(thread_checker_.CalledOnValidThread());

  uint32_t event_mask = 0;
  if (mode & WATCH_READ) {
    event_mask |= UV_READABLE;
  }

  if (mode & WATCH_WRITE) {
    event_mask |= UV_WRITABLE;
  }

  controller->WatchFileDescriptor(fd, persistent, event_mask, this, delegate);

  return true;
}

void MessagePumpNode::DoWork() {
  DCHECK(delegate_);

  for (;;) {
    bool did_work = false;
    did_work |= delegate_->DoIdleWork();
    did_work |= delegate_->DoWork();
    if (!did_work) {
      break;
    }
  }
}

void MessagePumpNode::DoDelayedWork() {
  //DCHECK_EQ(message_pump_state_, STATE_RUNNING);
  DCHECK(delegate_);
  if (delayed_work_time_.is_null()) {
    return;
  }

  for (;;) {
    base::TimeDelta delay = delayed_work_time_ - base::TimeTicks::Now();
    if (delay > base::TimeDelta()) {
      uv_timer_start(timer_handle_.get(), OnTimeout,
                     delay.InMilliseconds(), 0);
      break;
    }

    delegate_->DoDelayedWork(&delayed_work_time_);
    if (delayed_work_time_.is_null()) {
      uv_timer_stop(timer_handle_.get());
      message_pump_state_ = STATE_STOP;
      break;
    }
  }
}

void MessagePumpNode::StartAsyncTask() {
  DCHECK_EQ(message_pump_state_, STATE_IDLE);

  async_handle_->data = this;
  CHECK_EQ(uv_async_init(uv_default_loop(), async_handle_.get(),
                         &MessagePumpNode::OnWakeup), 0);

  timer_handle_->data = this;
  CHECK_EQ(uv_timer_init(uv_default_loop(), timer_handle_.get()), 0);

  message_pump_state_ = STATE_INIT;
}

void MessagePumpNode::StopAsyncTask() {
  DCHECK_NE(message_pump_state_, STATE_IDLE);
  uv_close(reinterpret_cast<uv_handle_t*>(async_handle_.get()),
           &IgnoreReleaseHandle);

  CHECK_EQ(uv_timer_stop(timer_handle_.get()), 0);
  uv_close(reinterpret_cast<uv_handle_t*>(timer_handle_.get()),
           &IgnoreReleaseHandle);

  message_pump_state_ = STATE_IDLE;
}

void MessagePumpNode::CloseAsyncTask() {
  if (async_handle_.get()) {
    uv_close(reinterpret_cast<uv_handle_t*>(async_handle_.release()),
             &ReleaseHandle);
  }

  if (timer_handle_.get()) {
    uv_close(reinterpret_cast<uv_handle_t*>(timer_handle_.release()),
             &ReleaseHandle);
  }

  message_pump_state_ = STATE_TERMINATE;
}

}  // namespace base
