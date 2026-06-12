#include "PiSubmarine/Lease/Server/Grpc/Server.h"

#include <chrono>
#include <stdexcept>
#include <utility>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <spdlog/spdlog.h>

#include "PiSubmarine/Lease/Grpc/Api/LeaseService.h"
#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Lease/Api/ErrorCode.h"
#include "PiSubmarine/Lease/Server/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    namespace
    {
        [[nodiscard]] Error::Api::Error MakeServerError(const ErrorCode code)
        {
            return Error::Api::MakeError(Error::Api::ErrorCondition::ContractError, make_error_code(code));
        }

        [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(Logging::Api::IFactory& loggerFactory)
        {
            auto logger = loggerFactory.CreateLogger("Lease.Server.Grpc");
            if (!logger)
            {
                throw std::invalid_argument("Lease.Server.Grpc requires a logger factory that returns a logger");
            }

            return logger;
        }

        [[nodiscard]] bool IsPeerAuthenticated(::grpc::ServerContext* context)
        {
            const auto authContext = context->auth_context();
            return static_cast<bool>(authContext) && authContext->IsPeerAuthenticated();
        }

        [[nodiscard]] ::pisubmarine::lease::grpc::api::ErrorPayload ToProtoError(const Error::Api::Error& error)
        {
            ::pisubmarine::lease::grpc::api::ErrorPayload protoError;
            protoError.set_error_condition(static_cast<int>(error.Condition));

            if (std::string_view(error.Cause.category().name()) == "PiSubmarine.Lease.Api")
            {
                protoError.set_lease_error_code(error.Cause.value());
            }

            return protoError;
        }

        [[nodiscard]] ::pisubmarine::lease::grpc::api::LeasePayload ToProtoLease(const Api::Lease& lease)
        {
            ::pisubmarine::lease::grpc::api::LeasePayload protoLease;
            protoLease.set_id(lease.Id.Value);
            protoLease.set_resource(lease.Resource.Value);
            protoLease.set_duration_ms(lease.Duration.count());
            return protoLease;
        }
    }

    class Server::Service final : public ::pisubmarine::lease::grpc::api::LeaseService::Service
    {
    public:
        Service(Api::ILeaseIssuer& leaseIssuer, std::shared_ptr<spdlog::logger> logger)
            : m_LeaseIssuer(leaseIssuer)
            , m_Logger(std::move(logger))
        {
        }

        ::grpc::Status AcquireLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::AcquireLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::LeaseResult* response) override
        {
            if (!IsPeerAuthenticated(context))
            {
                SPDLOG_LOGGER_WARN(m_Logger, "Rejected AcquireLease request because peer is not authenticated");
                return ::grpc::Status(::grpc::StatusCode::UNAUTHENTICATED, "client certificate required");
            }

            const auto result = m_LeaseIssuer.AcquireLease(Api::LeaseRequest{
                .Resource = Api::ResourceId{.Value = request->resource()}});
            if (!result.has_value())
            {
                SPDLOG_LOGGER_WARN(
                    m_Logger,
                    "AcquireLease failed for resource '{}': {}",
                    request->resource(),
                    result.error().Cause.message());
                *response->mutable_error() = ToProtoError(result.error());
                return ::grpc::Status::OK;
            }

            SPDLOG_LOGGER_INFO(
                m_Logger,
                "AcquireLease succeeded for resource '{}' with lease '{}'",
                result->Resource.Value,
                result->Id.Value);
            *response->mutable_lease() = ToProtoLease(*result);
            return ::grpc::Status::OK;
        }

        ::grpc::Status RenewLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::RenewLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::LeaseResult* response) override
        {
            if (!IsPeerAuthenticated(context))
            {
                SPDLOG_LOGGER_WARN(m_Logger, "Rejected RenewLease request because peer is not authenticated");
                return ::grpc::Status(::grpc::StatusCode::UNAUTHENTICATED, "client certificate required");
            }

            const auto result = m_LeaseIssuer.RenewLease(Api::LeaseId{.Value = request->lease_id()});
            if (!result.has_value())
            {
                SPDLOG_LOGGER_WARN(
                    m_Logger,
                    "RenewLease failed for lease '{}': {}",
                    request->lease_id(),
                    result.error().Cause.message());
                *response->mutable_error() = ToProtoError(result.error());
                return ::grpc::Status::OK;
            }

            SPDLOG_LOGGER_INFO(
                m_Logger,
                "RenewLease succeeded for lease '{}' on resource '{}'",
                result->Id.Value,
                result->Resource.Value);
            *response->mutable_lease() = ToProtoLease(*result);
            return ::grpc::Status::OK;
        }

        ::grpc::Status ReleaseLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::ReleaseLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::VoidResult* response) override
        {
            if (!IsPeerAuthenticated(context))
            {
                SPDLOG_LOGGER_WARN(m_Logger, "Rejected ReleaseLease request because peer is not authenticated");
                return ::grpc::Status(::grpc::StatusCode::UNAUTHENTICATED, "client certificate required");
            }

            const auto result = m_LeaseIssuer.ReleaseLease(Api::LeaseId{.Value = request->lease_id()});
            if (!result.has_value())
            {
                SPDLOG_LOGGER_WARN(
                    m_Logger,
                    "ReleaseLease failed for lease '{}': {}",
                    request->lease_id(),
                    result.error().Cause.message());
                *response->mutable_error() = ToProtoError(result.error());
                return ::grpc::Status::OK;
            }

            SPDLOG_LOGGER_INFO(m_Logger, "ReleaseLease succeeded for lease '{}'", request->lease_id());
            response->mutable_success();
            return ::grpc::Status::OK;
        }

    private:
        Api::ILeaseIssuer& m_LeaseIssuer;
        std::shared_ptr<spdlog::logger> m_Logger;
    };

    Server::Server(Api::ILeaseIssuer& leaseIssuer, Logging::Api::IFactory& loggerFactory, TlsConfig tlsConfig)
        : m_LeaseIssuer(leaseIssuer)
        , m_Logger(CreateLogger(loggerFactory))
        , m_TlsConfig(std::move(tlsConfig))
        , m_Service(std::make_unique<Service>(m_LeaseIssuer, m_Logger))
    {
        SPDLOG_LOGGER_INFO(m_Logger, "Initialized gRPC lease server for address '{}'", m_TlsConfig.Address);
    }

    Server::~Server()
    {
        Stop();
    }

    Error::Api::Result<void> Server::Start()
    {
        if (m_Server)
        {
            SPDLOG_LOGGER_WARN(m_Logger, "Rejected duplicate start request for gRPC lease server");
            return std::unexpected(MakeServerError(ErrorCode::AlreadyStarted));
        }

        if (m_TlsConfig.Address.empty() ||
            m_TlsConfig.ServerCertificateChain.empty() ||
            m_TlsConfig.ServerPrivateKey.empty() ||
            m_TlsConfig.ClientCertificateAuthority.empty())
        {
            SPDLOG_LOGGER_ERROR(m_Logger, "Rejected start because mutual TLS configuration is incomplete");
            return std::unexpected(MakeServerError(ErrorCode::InvalidTlsConfiguration));
        }

        ::grpc::SslServerCredentialsOptions sslOptions;
        sslOptions.pem_root_certs = m_TlsConfig.ClientCertificateAuthority;
        ::grpc::SslServerCredentialsOptions::PemKeyCertPair keyCertPair;
        keyCertPair.private_key = m_TlsConfig.ServerPrivateKey;
        keyCertPair.cert_chain = m_TlsConfig.ServerCertificateChain;
        sslOptions.pem_key_cert_pairs.push_back(std::move(keyCertPair));
        sslOptions.client_certificate_request =
            GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;

        ::grpc::ServerBuilder builder;
        builder.AddListeningPort(m_TlsConfig.Address, ::grpc::SslServerCredentials(sslOptions));
        builder.RegisterService(m_Service.get());

        m_Server = builder.BuildAndStart();
        if (!m_Server)
        {
            SPDLOG_LOGGER_ERROR(m_Logger, "Failed to start gRPC lease server on '{}'", m_TlsConfig.Address);
            return std::unexpected(MakeServerError(ErrorCode::FailedToStart));
        }

        SPDLOG_LOGGER_INFO(m_Logger, "Started gRPC lease server on '{}'", m_TlsConfig.Address);
        return {};
    }

    void Server::Stop()
    {
        if (!m_Server)
        {
            return;
        }

        SPDLOG_LOGGER_INFO(m_Logger, "Stopping gRPC lease server on '{}'", m_TlsConfig.Address);
        m_Server->Shutdown(std::chrono::system_clock::now());
        m_Server.reset();
    }

    bool Server::IsRunning() const noexcept
    {
        return static_cast<bool>(m_Server);
    }
}
