#pragma once

#include <memory>
#include <string>

#include "PiSubmarine/Error/Api/Result.h"
#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"

namespace grpc
{
    class Server;
}

namespace PiSubmarine::Lease::Server::Grpc
{
    struct TlsConfig
    {
        std::string Address;
        std::string ServerCertificateChain;
        std::string ServerPrivateKey;
        std::string ClientCertificateAuthority;
    };

    class Server
    {
    public:
        Server(Api::ILeaseIssuer& leaseIssuer, TlsConfig tlsConfig);
        ~Server();

        [[nodiscard]] Error::Api::Result<void> Start();
        void Stop();
        [[nodiscard]] bool IsRunning() const noexcept;

    private:
        class Service;

        Api::ILeaseIssuer& m_LeaseIssuer;
        TlsConfig m_TlsConfig;
        std::unique_ptr<Service> m_Service;
        std::unique_ptr<::grpc::Server> m_Server;
    };
}
