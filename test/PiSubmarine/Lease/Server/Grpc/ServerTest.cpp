#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>

#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Server/Grpc/Adapter.h"
#include "PiSubmarine/Logging/Api/IFactory.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    namespace
    {
        class DummyIssuer final : public Api::ILeaseIssuer
        {
        public:
            Error::Api::Result<Api::LeaseGrant> AcquireLease(const Api::LeaseRequest&) override
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

        class NullLoggerFactory final : public Logging::Api::IFactory
        {
        public:
            [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(std::string_view) override
            {
                return {};
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

    TEST(ServerTest, RequiresLoggerFactoryToReturnLogger)
    {
        DummyIssuer issuer;
        NullLoggerFactory loggerFactory;

        EXPECT_THROW((Adapter(issuer, loggerFactory)), std::invalid_argument);
    }

    TEST(ServerTest, ConstructsWithValidLoggerFactory)
    {
        DummyIssuer issuer;
        LoggerFactoryStub loggerFactory;

        EXPECT_NO_THROW((Adapter(issuer, loggerFactory)));
    }
}
