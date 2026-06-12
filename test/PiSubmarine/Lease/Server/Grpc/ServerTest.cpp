#include <gtest/gtest.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>

#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Server/Grpc/Server.h"
#include "PiSubmarine/Lease/Server/Grpc/ErrorCode.h"
#include "PiSubmarine/Logging/Api/IFactory.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    namespace
    {
        class DummyIssuer final : public Api::ILeaseIssuer
        {
        public:
            Error::Api::Result<Api::Lease> AcquireLease(const Api::LeaseRequest&) override
            {
                return std::unexpected(Error::Api::MakeError(Error::Api::ErrorCondition::UnknownError));
            }

            Error::Api::Result<Api::Lease> RenewLease(const Api::LeaseId&) override
            {
                return std::unexpected(Error::Api::MakeError(Error::Api::ErrorCondition::UnknownError));
            }

            Error::Api::Result<void> ReleaseLease(const Api::LeaseId&) override
            {
                return std::unexpected(Error::Api::MakeError(Error::Api::ErrorCondition::UnknownError));
            }
        };

        class LoggerFactoryStub final : public Logging::Api::IFactory
        {
        public:
            [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(std::string_view name) override
            {
                return std::make_shared<spdlog::logger>(
                    std::string(name),
                    std::make_shared<spdlog::sinks::null_sink_mt>());
            }
        };
    }

    TEST(ServerTest, RejectsInvalidTlsConfiguration)
    {
        DummyIssuer issuer;
        LoggerFactoryStub loggerFactory;
        Server server(issuer, loggerFactory, TlsConfig{});

        const auto result = server.Start();
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().Cause, make_error_code(ErrorCode::InvalidTlsConfiguration));
    }
}
