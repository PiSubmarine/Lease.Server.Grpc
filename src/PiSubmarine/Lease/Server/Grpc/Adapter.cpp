#include "PiSubmarine/Lease/Server/Grpc/Adapter.h"

#include <cstddef>
#include <string>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "PiSubmarine/Error/Api/MakeError.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    namespace
    {
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

        [[nodiscard]] std::string ToProtoLeaseSecret(const Api::LeaseSecret& secret)
        {
            std::string bytes;
            bytes.reserve(secret.Value.size());
            for (const auto value : secret.Value)
            {
                bytes.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
            }

            return bytes;
        }
    }

    Adapter::Adapter(Api::ILeaseIssuer& leaseIssuer, Logging::Api::IFactory& loggerFactory)
        : m_LeaseIssuer(leaseIssuer)
        , m_Logger(CreateLogger(loggerFactory))
    {
    }

    ::grpc::Status Adapter::AcquireLease(
        ::grpc::ServerContext* context,
        const ::pisubmarine::lease::grpc::api::AcquireLeaseRequest* request,
        ::pisubmarine::lease::grpc::api::LeaseGrantResult* response)
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
            result->Lease.Resource.Value,
            result->Lease.Id.Value);
        *response->mutable_lease_grant()->mutable_lease() = ToProtoLease(result->Lease);
        response->mutable_lease_grant()->set_secret(ToProtoLeaseSecret(result->Secret));
        return ::grpc::Status::OK;
    }

    ::grpc::Status Adapter::RenewLease(
        ::grpc::ServerContext* context,
        const ::pisubmarine::lease::grpc::api::RenewLeaseRequest* request,
        ::pisubmarine::lease::grpc::api::LeaseResult* response)
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

    ::grpc::Status Adapter::ReleaseLease(
        ::grpc::ServerContext* context,
        const ::pisubmarine::lease::grpc::api::ReleaseLeaseRequest* request,
        ::pisubmarine::lease::grpc::api::VoidResult* response)
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
}
