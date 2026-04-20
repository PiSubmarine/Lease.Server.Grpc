#include <gtest/gtest.h>

#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Server/Grpc/Server.h"

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
    }

    TEST(ServerTest, RejectsInvalidTlsConfiguration)
    {
        DummyIssuer issuer;
        Server server(issuer, TlsConfig{});

        const auto result = server.Start();
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().Cause, make_error_code(ErrorCode::InvalidTlsConfiguration));
    }
}
