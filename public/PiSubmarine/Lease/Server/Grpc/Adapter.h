#pragma once

#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Grpc/Api/LeaseService.h"
#include "PiSubmarine/Logging/Api/IFactory.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    class Adapter final : public ::pisubmarine::lease::grpc::api::LeaseService::Service
    {
    public:
        Adapter(Api::ILeaseIssuer& leaseIssuer, Logging::Api::IFactory& loggerFactory);

        ::grpc::Status AcquireLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::AcquireLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::LeaseGrantResult* response) override;
        ::grpc::Status RenewLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::RenewLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::LeaseResult* response) override;
        ::grpc::Status ReleaseLease(
            ::grpc::ServerContext* context,
            const ::pisubmarine::lease::grpc::api::ReleaseLeaseRequest* request,
            ::pisubmarine::lease::grpc::api::VoidResult* response) override;

    private:
        Api::ILeaseIssuer& m_LeaseIssuer;
        std::shared_ptr<spdlog::logger> m_Logger;
    };
}
