//
// 2017 writen by snibug@gmail.com
//

#ifndef NODE_BINDER_BASE_MESSAGE_PUMP_NODE_H_
#define NODE_BINDER_BASE_MESSAGE_PUMP_NODE_H_

#include "base/macros.h"
#include "base/message_loop/message_pump.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "node/uv.h"
#include "stellite/include/stellite_export.h"

namespace base {

class STELLITE_EXPORT MessagePumpNode : public MessagePump {
 public:
  class Watcher {
   public:
    virtual void OnFileCanReadWithoutBlocking(int fd) = 0;
    virtual void OnFileCanWriteWithoutBlocking(int fd) = 0;

   protected:
    virtual ~Watcher() {}
  };

  class FileDescriptorWatcher {
   public:
    FileDescriptorWatcher();
    ~FileDescriptorWatcher();

    bool WatchFileDescriptor(int fd, bool persistent, int event_mask,
                             MessagePumpNode* pump, Watcher* watcher);

    bool StopWatchingFileDescriptor();

    void OnFileEventNotification(int fd, int event_mask);

   private:
    friend class MessagePumpNode;

    void OnFileCanReadWithoutBlocking(int fd, MessagePumpNode* pump);
    void OnFileCanWriteWithoutBlocking(int fd, MessagePumpNode* pump);

    std::unique_ptr<uv_poll_t> event_;
    MessagePumpNode* pump_;
    Watcher* watcher_;
    bool persistent_;
    uint32_t event_mask_;

    DISALLOW_COPY_AND_ASSIGN(FileDescriptorWatcher);
  };

  static MessagePumpNode* current();

  enum Mode {
    WATCH_READ = 1 << 0,
    WATCH_WRITE = 1 << 1,
    WATCH_READ_WRITE = WATCH_READ | WATCH_WRITE,
  };

  MessagePumpNode();
  ~MessagePumpNode() override;

  // @snibug: controll nodejs message loop at NodeMessagePumpManager
  // there is no necessary keeping nodejs thread loop when shutdown quic server
  // socket or there is no http request using http fetcher. StartAsyncTask
  // start message pump and can affect a libuv's uv_run(ONCE) blocking.
  // but if there is no task NodeMessagePumpManager call StopAsyncTask then
  // finally nodejs's main thread loop are break.
  void StartAsyncTask();
  void StopAsyncTask();

  // implements MessagePump interface
  void Run(Delegate* delegate)  override;
  void Quit() override;
  void ScheduleWork() override;
  void ScheduleDelayedWork(const base::TimeTicks& delayed_work_time) override;

  bool WatchFileDescriptor(int fd,
                           bool persistent,
                           int mode,
                           FileDescriptorWatcher* controller,
                           Watcher* delegate);

  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() { return delegate_; }

 private:
  enum MessagePumpState {
    STATE_IDLE,
    STATE_INIT,
    STATE_RUNNING,
    STATE_STOP,
    STATE_TERMINATE,
  };

  static void OnWakeup(uv_async_t* handle);
  static void OnTimeout(uv_timer_t* handle);
  static void OnFileEventNotification(uv_poll_t* event, int status,
                                      int flags);

  void DoWork();
  void DoDelayedWork();
  void CloseAsyncTask();

  Delegate* delegate_;

  std::unique_ptr<uv_async_t> async_handle_;
  std::unique_ptr<uv_timer_t> timer_handle_;
  std::unique_ptr<uv_poll_t> event_base_;

  base::TimeTicks delayed_work_time_;
  MessagePumpState message_pump_state_;
  ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpNode);
};

}  // namespace base

#endif  // NODE_BINDER_BASE_MESSAGE_PUMP_NODE_H_
