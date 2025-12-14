#pragma once

#include <LicenseSpring/Configuration.h>

struct AppConfig
{
    AppConfig() {}
    AppConfig( const std::string& name, const std::string& version )
        : appName( name ), appVersion( version ) {}

    // Create LicenseSpring configuration
    LicenseSpring::Configuration::ptr_t createLicenseSpringConfig() const;

    std::string appName;
    std::string appVersion;
};
