#include "PiSubmarine/Lease/Server/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Server::Grpc
{
    namespace
    {
        class ErrorCategory final : public std::error_category
        {
        public:
            [[nodiscard]] const char* name() const noexcept override
            {
                return "PiSubmarine.Lease.Server.Grpc";
            }

            [[nodiscard]] std::string message(const int condition) const override
            {
                switch (static_cast<ErrorCode>(condition))
                {
                case ErrorCode::AlreadyStarted:
                    return "gRPC lease server is already started";
                case ErrorCode::InvalidTlsConfiguration:
                    return "gRPC lease server TLS configuration is invalid";
                case ErrorCode::FailedToStart:
                    return "gRPC lease server failed to start";
                }

                return "unknown lease server grpc error";
            }
        };
    }

    std::error_code make_error_code(const ErrorCode errorCode) noexcept
    {
        static const ErrorCategory Category;
        return {static_cast<int>(errorCode), Category};
    }
}
