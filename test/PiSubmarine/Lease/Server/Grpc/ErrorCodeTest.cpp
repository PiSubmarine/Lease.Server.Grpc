#include <gtest/gtest.h>

#include "PiSubmarine/Lease/Server/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    TEST(ErrorCodeTest, ReportsReadableMessage)
    {
        EXPECT_EQ(make_error_code(ErrorCode::InvalidTlsConfiguration).message(),
                  "gRPC lease server TLS configuration is invalid");
    }
}
