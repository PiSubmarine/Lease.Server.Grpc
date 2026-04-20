#pragma once

#include <system_error>

namespace PiSubmarine::Lease::Server::Grpc
{
    enum class ErrorCode
    {
        AlreadyStarted = 1,
        InvalidTlsConfiguration,
        FailedToStart
    };

    [[nodiscard]] std::error_code make_error_code(ErrorCode errorCode) noexcept;
}

namespace std
{
    template<>
    struct is_error_code_enum<PiSubmarine::Lease::Server::Grpc::ErrorCode> : true_type
    {
    };
}
