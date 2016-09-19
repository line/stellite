//
// Copyright 2016 LINE Corporation
//

#include <vector>

#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "stellite/include/http_client.h"
#include "stellite/include/http_client_context.h"
#include "stellite/include/http_request.h"
#include "stellite/server/server_config.h"
#include "stellite/server/server_packet_writer.h"
#include "stellite/server/thread_dispatcher.h"
#include "stellite/server/thread_worker.h"
#include "stellite/test/http_test_server.h"
#include "stellite/test/quic_mock_server.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/crypto/proof_source_chromium.h"
#include "net/quic/crypto/quic_crypto_server_config.h"
#include "net/quic/crypto/quic_random.h"
#include "net/quic/quic_config.h"
#include "net/quic/quic_packet_writer.h"
#include "net/quic/quic_protocol.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace stellite {

class QuicMigrationClientDelegate : public HttpResponseDelegate {
 public:
  QuicMigrationClientDelegate()
      : did_success_(false) {
  }

  void OnHttpResponse(const HttpResponse& response,
                      const char* data, size_t len) override {
    did_success_ = true;
    run_loop_.Quit();
  }

  void OnHttpStream(const HttpResponse& response,
                    const char* data, size_t len) override {
  }

  void OnHttpError(int error_code, const std::string& error_message) override {
    run_loop_.Quit();
  }

  bool WaitForComplete() {
    run_loop_.Run();
    return did_success_;
  }

 private:
  base::RunLoop run_loop_;
  bool did_success_;
};

} // namespace stellite

namespace net {

namespace test {

class ThreadWorkerPeer {
 public:
  static ThreadDispatcher* get_dispatcher(ThreadWorker* worker) {
    return worker->dispatcher_.get();
  }
};

class SwitchableDispatcher : public ThreadDispatcher {
 public:
  SwitchableDispatcher(uint32_t dispatcher_count,
                       scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                       QuicUDPServerSocket* server_socket,
                       const QuicConfig& quic_config,
                       const QuicCryptoServerConfig* crypto_config,
                       const ServerConfig& server_config)
      : ThreadDispatcher(task_runner,
                         quic_config,
                         crypto_config,
                         server_config,
                         QuicSupportedVersions()) {
    CHECK(dispatcher_count > 0);

    for (uint32_t i = 0; i < dispatcher_count; ++i) {
      scoped_ptr<ThreadDispatcher> dispatcher(
          new ThreadDispatcher(task_runner,
                               quic_config,
                               crypto_config,
                               server_config,
                               QuicSupportedVersions()));

      ServerPacketWriter* server_packet_writer =
          new ServerPacketWriter(server_socket, dispatcher.get());
      dispatcher->InitializeWithWriter(server_packet_writer);
      dispatcher_container_.push_back(std::move(dispatcher));
    }

    if (dispatcher_container_.size()) {
      current_dispatcher_ = dispatcher_container_.front().get();
    }
  }

  ~SwitchableDispatcher() override {
  }

  void ProcessPacket(const IPEndPoint& server_address,
                     const IPEndPoint& client_address,
                     const QuicEncryptedPacket& packet) override {
    if (!current_dispatcher_) {
      return;
    }

    current_dispatcher_->ProcessPacket(server_address,
                                       client_address,
                                       packet);
  }

  bool SwitchDispatcher(uint32_t pos) {
    if (dispatcher_container_.size() <= pos) {
      return false;
    }
    current_dispatcher_ = dispatcher_container_[pos].get();
    return true;
  }

  bool SwitchClientAddress(const IPEndPoint& client_address) {
    switched_client_address_ = client_address;
    return true;
  }

 private:
  ThreadDispatcher* current_dispatcher_;

  std::vector<scoped_ptr<ThreadDispatcher>> dispatcher_container_;
  IPEndPoint switched_client_address_;

  DISALLOW_COPY_AND_ASSIGN(SwitchableDispatcher);
};

class QuicSwitchableDispatcherThreadWorker : public ThreadWorker {
 public:
  QuicSwitchableDispatcherThreadWorker(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      const IPEndPoint& bind_address,
      const QuicConfig& quic_config,
      const QuicCryptoServerConfig* crypto_config,
      const ServerConfig& server_config)
      : ThreadWorker(task_runner,
                     bind_address,
                     quic_config,
                     crypto_config,
                     server_config,
                     QuicSupportedVersions()),
        task_runner_(task_runner),
        dispatcher_(nullptr) {
  }

  ~QuicSwitchableDispatcherThreadWorker() override {
  }

  bool SwitchDispatcher(uint32_t pos) {
    return dispatcher_->SwitchDispatcher(pos);
  }

  bool SwitchClientAddress(const IPEndPoint& client_address) {
    return dispatcher_->SwitchClientAddress(client_address);
  }

 protected:
  ThreadDispatcher* CreateDispatcher(QuicUDPServerSocket* socket) override {
    CHECK(dispatcher_ == nullptr);
    dispatcher_ = new SwitchableDispatcher(10,
                                           task_runner_,
                                           socket,
                                           quic_config(),
                                           crypto_config(),
                                           server_config());
    return dispatcher_;
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  SwitchableDispatcher* dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(QuicSwitchableDispatcherThreadWorker);
};

class QuicSwitchableDispatcherServer {
 public:
  QuicSwitchableDispatcherServer(uint16_t port)
      : port_(port) {
  }

  void Start() {
    server_config_.proxy_pass("http://example.com");

    base::FilePath data_path;
    EXPECT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &data_path));

    data_path = data_path.Append(FILE_PATH_LITERAL("stellite")).
                          Append(FILE_PATH_LITERAL("test_tools")).
                          Append(FILE_PATH_LITERAL("res"));

    base::FilePath keyfile = data_path.Append("example.com.cert.pkcs8");
    base::FilePath certfile = data_path.Append("example.com.cert.pem");

    ProofSourceChromium* proof_source = new ProofSourceChromium();
    EXPECT_TRUE(proof_source->Initialize(certfile, keyfile, base::FilePath()));
    crypto_config_.reset(
        new QuicCryptoServerConfig("secret", QuicRandom::GetInstance(),
                                   proof_source));

    crypto_config_->set_strike_register_no_startup_period();
    scoped_ptr<CryptoHandshakeMessage> scfg(
        crypto_config_->AddDefaultConfig(
            QuicRandom::GetInstance(),
            &clock_,
            QuicCryptoServerConfig::ConfigOptions()));

    IPAddress server_ip;
    CHECK(ParseIPLiteralToNumber("::", &server_ip));
    worker_.reset(new QuicSwitchableDispatcherThreadWorker(
            task_runner(),
            IPEndPoint(server_ip, port_),
            quic_config_,
            crypto_config_.get(),
            server_config_));
    worker_->Start();
  }

  void Shutdown() {
    CHECK(worker_.get());
    worker_->Shutdown();
  }

  bool SwitchDispatcher(uint pos) {
    return worker_->SwitchDispatcher(pos);
  }

  bool SwitchClientAddress(const IPEndPoint& client_address) {
    return worker_->SwitchClientAddress(client_address);
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner() {
    return base::ThreadTaskRunnerHandle::Get();
  }

 private:
  // QUIC server
  uint16_t port_;
  QuicClock clock_;
  QuicConfig quic_config_;
  ServerConfig server_config_;

  scoped_ptr<QuicCryptoServerConfig> crypto_config_;
  scoped_ptr<QuicSwitchableDispatcherThreadWorker> worker_;

  DISALLOW_COPY_AND_ASSIGN(QuicSwitchableDispatcherServer);
};

class QuicConnectionMigrationTest : public testing::Test {
 public:
  QuicConnectionMigrationTest()
      : client_(nullptr) {
  }

  void SetUp() override {
    StartServer();
    InitClient();
  }

  void TearDown() override {
    ShutdownClient();
    StopServer();
  }

  void StartServer() {
    HttpTestServer::Params http_server_params;
    http_server_params.server_type = HttpTestServer::TYPE_HTTP;
    http_server_params.port = 9988;
    http_server_.reset(new HttpTestServer(http_server_params));
    EXPECT_TRUE(http_server_->Start());

    quic_server_.reset(new QuicSwitchableDispatcherServer(9988));
    quic_server_->Start();
  }

  void StopServer() {
    CHECK(quic_server_.get());
    quic_server_->Shutdown();

    CHECK(http_server_.get());
    http_server_->Shutdown();
  }

  void InitClient() {
    stellite::HttpClientContext::Params client_context_params;
    client_context_params.using_quic = true;
    client_context_params.origin_to_force_quic_on = "example.com:9988";
    http_client_context_.reset(
        new stellite::HttpClientContext(client_context_params));
    EXPECT_TRUE(http_client_context_->Initialize());

    client_ = http_client_context_->CreateHttpClient();
  }

  void ShutdownClient() {
    if (client_) {
      http_client_context_->ReleaseHttpClient(client_);
    }
  }

  bool RequestAndWaitToComplete(const stellite::HttpRequest& request) {
    sp<stellite::HttpResponseDelegate> delegate(
        new stellite::QuicMigrationClientDelegate());
    client_->Request(request, delegate);

    stellite::QuicMigrationClientDelegate* waitor =
        static_cast<stellite::QuicMigrationClientDelegate*>(delegate.get());

    return waitor->WaitForComplete();
  }

  // HTTP server
  scoped_ptr<HttpTestServer> http_server_;

  // QUIC server
  scoped_ptr<QuicSwitchableDispatcherServer> quic_server_;

  // QUIC test client
  IPEndPoint server_address_;
  scoped_ptr<stellite::HttpClientContext> http_client_context_;
  stellite::HttpClient* client_;
};

TEST_F(QuicConnectionMigrationTest, SwitchDispatcherMigration) {
  stellite::HttpRequest request;
  request.method = "GET";
  request.url = "https://example.com:9988";

  // Change dispatcher and modify IP address and port
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(quic_server_->SwitchDispatcher(i));
    EXPECT_TRUE(RequestAndWaitToComplete(request));
  }
}

} // namespace test
} // namespace net